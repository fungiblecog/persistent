/*
    Copyright (C) 2020 Duncan Watts

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 or later.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gc.h>
#include <assert.h>

#include "hashmap.h"

#define BITS_PER_LEVEL 5

/* indicates if the map changed during the operation */
#define UNCHANGED 0
#define ADDED 1
#define UPDATED 2
#define REMOVED 3
/*
   UNCHANGED - no change
   ADDED     - added a key/value
   UPDATED   - updated an existing value
   REMOVED   - removed a key/value
*/

/*
   for explanations see:
   http://blog.higher-order.net/2009/09/08/understanding-clojures-persistenthashmap-deftwice
*/

/* Implementation details */
typedef struct NodeType NodeType;
typedef struct Node Node;
typedef struct LeafNode LeafNode;
typedef struct BitmapIndexedNode BitmapIndexedNode;
typedef struct HashCollisionNode HashCollisionNode;

typedef void *(*get_fn)(Node *self, int level, void *key, \
			hash_t hash, equal_fn eq_key, equal_fn eq_val);

typedef Node *(*assoc_fn)(Node *self, int level, void *key, void *val, \
			  hash_t hash, equal_fn eq_key, equal_fn eq_val, int *result);

typedef Node *(*dissoc_fn)(Node *self, int level, void *key, hash_t hash, \
			   equal_fn eq_key, equal_fn eq_val, int *result);

typedef void (*visitor_fn)(Node *self, visit_fn fn, void **acc);

/* hashmap links to nodes and holds functions for
   operating on otherwise generic keys and vals */
struct Hashmap {
  hash_fn hash;
  equal_fn eq_key;
  equal_fn eq_val;
  int count;
  Node* root;
};

/* polymorphic dispatch through the common NodeType */
struct NodeType {
  get_fn get;
  assoc_fn assoc;
  dissoc_fn dissoc;
  visitor_fn visitor;
};

/* generic node */
struct Node {
  NodeType *type;
};

/* holds a key/value pair */
struct LeafNode {
  NodeType *type;
  void *key;
  void *val;
  hash_t hash;
};

/* a linked list of nodes with the same hash */
struct HashCollisionNode {
  NodeType *type;
  void *key;
  void *val;
  hash_t hash;

  HashCollisionNode *next;
};

/* a dynamically sized array of up to 32 child nodes */
struct BitmapIndexedNode {
  NodeType *type;
  int bitmap;
  Node **children;
};

/* basic list structure for implementing an iterator */
typedef struct list {
  void *data;
  struct list *next;
} list;

/* forward references */
static hash_t hash_str(void *obj);

static int equal_str(void *obj1, void *obj2);

static LeafNode *new_leaf_node(void *key, void *val, hash_t hash);

static HashCollisionNode *new_hash_collision_node(void *key, void* val, hash_t hash);

static BitmapIndexedNode *new_bitmap_indexed_node();

static BitmapIndexedNode *copy_bitmap_indexed_node(BitmapIndexedNode *node);

static Hashmap *copy_hashmap(Hashmap *map);

static void *leaf_get(Node *self, int level, void *key,                 \
                      hash_t hash, equal_fn eq_key, equal_fn eq_val);

static void *bitmap_indexed_get(Node *self, int level, void *key, \
                                hash_t hash, equal_fn eq_key, equal_fn eq_val);

static void *hash_collision_get(Node *self, int level, void *key,	\
                                hash_t hash, equal_fn eq_key, equal_fn eq_val);

static Node *leaf_assoc(Node *self, int level, void *key, void *val, \
                        hash_t hash, equal_fn eq_key, equal_fn eq_val, int *result);

static Node *bitmap_indexed_assoc(Node *self, int level, void *key, void *val, \
                                  hash_t hash, equal_fn eq_key, equal_fn eq_val, int *result);

static Node *hash_collision_assoc(Node *self, int level, void *key, void *val, \
                                  hash_t hash, equal_fn eq_key, equal_fn eq_val, int *result);

static Node *leaf_dissoc(Node *self, int level, void *key, hash_t hash, \
                         equal_fn eq_key, equal_fn eq_val, int *result);

static Node *bitmap_indexed_dissoc(Node *self, int level, void *key, hash_t hash, \
                                   equal_fn eq_key, equal_fn eq_val, int *result);

static Node *hash_collision_dissoc(Node *self, int level, void *key, hash_t hash, \
                                   equal_fn eq_key, equal_fn eq_val, int *result);

static void leaf_visit(Node *self, visit_fn fn, void **acc);

static void bitmap_indexed_visit(Node *self, visit_fn fn, void **acc);

static void hash_collision_visit(Node *self, visit_fn fn, void **acc);

static void iterator_visit(void *key, void *val, void **acc);

static Iterator *hashmap_next_fn(Iterator *iter);

/* constant NodeTypes */

/* Leaf nodes store key/value pairs */
NodeType NT_LEAF = {leaf_get, leaf_assoc, leaf_dissoc, leaf_visit};

/* Bitmap indexed nodes hold pointers to up to 32 sub-nodes */
NodeType NT_BITMAP_INDEXED = {bitmap_indexed_get, bitmap_indexed_assoc, \
                              bitmap_indexed_dissoc, bitmap_indexed_visit};

/* Hash collision nodes replace leaf nodes when there is a collision */
NodeType NT_HASH_COLLISION = {hash_collision_get, hash_collision_assoc, \
                              hash_collision_dissoc, hash_collision_visit};

/* count 1's in x efficiently */
#define popcount(x) __builtin_popcount(x)

/* uncomment this if on something other than gcc */
/* int popcount(unsigned x)  */
/* { */
/*   int c = 0; */
/*   for (; x != 0; x &= x - 1) { c++; } */
/*   return c; */
/* } */

/* shift hash by 'level'. Each level is BITS_PER_LEVEL bits
   which is 5 by default giving [0-31] children */
static int mask(hash_t hash, int level)
{
  return (hash >> (BITS_PER_LEVEL * level)) & 0x01f;
}

/* map a hash code [0-31] to a bit in the bitmap 2^[0-31] */
static int bitpos(hash_t hash, int level)
{
  return 1 << mask(hash, level);
}

/* return the number of 1's in bitmap less than bit */
static int bit_index(int bitmap, int bit)
{
  return popcount(bitmap & (bit - 1));
}

/* external interface */
Hashmap *hashmap_make(hash_fn hash, equal_fn eq_keys, equal_fn eq_vals)
{
  Hashmap *map = GC_MALLOC(sizeof(*map));

  map->count = 0;
  map->root = NULL;

  map->hash = hash ? hash : hash_str;
  map->eq_key = eq_keys ? eq_keys : equal_str;
  map->eq_val = eq_vals ? eq_vals : equal_str;

  return map;
}

Hashmap *hashmap_assoc(Hashmap* map, void *key, void *val)
{
  /* result is a boolean that indicates if the map changed during the operation */
  int result = UNCHANGED;
  Node *root = NULL;

  /* if there are no entries create a leaf node */
  if (!map->root) {

    LeafNode* leaf = new_leaf_node(key, val, map->hash(key));
    result = ADDED;
    root = (Node*)leaf;
  }
  /* otherwise call assoc on the root node */
  else {
      root = (map->root)->type->assoc(map->root, 0, key, val, map->hash(key), \
                                      map->eq_key, map->eq_val, &result);
  }
  /* if there was a change create a new hashmap */
  if (result != UNCHANGED) {

    Hashmap *new = copy_hashmap(map);
    new->root = root;
    new->count += (result == ADDED) ? 1 : 0;

    return new;
  }
  /* no change */
  return map;
}

Hashmap *hashmap_dissoc(Hashmap *map, void *key)
{
  /* result is a boolean that indicates if the map changed during the operation */
  int result = UNCHANGED;

  /* if there are no entries there's nothing to dissoc */
  if (!map->root) { return map; }

  /* otherwise call dissoc on the root node */
  Node *root = (map->root)->type->dissoc(map->root, 0, key, map->hash(key), \
                                         map->eq_key, map->eq_val, &result);

  /* if there was a change create a new hashmap */
  if (result != UNCHANGED) {

    Hashmap *new = copy_hashmap(map);
    new->root = root;
    new->count -= (result == REMOVED) ? 1 : 0;

    return new;
  }
  /* no change */
  return map;
}

void *hashmap_get(Hashmap *map, void *key)
{
  if (!map->root) { return NULL; }
  return (map->root)->type->get(map->root, 0, key, map->hash(key), \
                                map->eq_key, map->eq_val);
}

int hashmap_count(Hashmap *map)
{
  return map->count;
}

int hashmap_empty(Hashmap *map)
{
  return (map->count == 0);
}

void hashmap_visit(Hashmap *map, visit_fn fn, void** acc)
{
  if (!map->root) { return; }
  (map->root)->type->visitor(map->root, fn, acc);
}

Iterator *hashmap_iterator_make(Hashmap *map)
{
  assert(map);

  /* empty hashmap returns a NULL iterator */
  if (hashmap_empty(map)) { return NULL; }

  /* create an iterator */
  Iterator *iter = GC_MALLOC(sizeof(*iter));

  /* install the next function for a hashmap */
  iter->next_fn = hashmap_next_fn;

  /* create a list of key/val pairs */
  list *lst = NULL;
  hashmap_visit(map, iterator_visit, (void **)&lst);

  /* set the entry for the current element */
  iter->current = lst;
  iter->value = lst->data;

  return iter;
}

/* internal implementation */

static LeafNode *new_leaf_node(void *key, void *val, hash_t hash)
{
  LeafNode *node = GC_MALLOC(sizeof(*node));
  node->type = &NT_LEAF;
  node->key = key;
  node->val = val;
  node->hash = hash;

  return node;
}

static BitmapIndexedNode *new_bitmap_indexed_node()
{
  BitmapIndexedNode *node = GC_MALLOC(sizeof(*node));
  node->type = &NT_BITMAP_INDEXED;
  node->bitmap = 0;

  return node;
}

static HashCollisionNode *new_hash_collision_node(void *key, void* val, hash_t hash)
{
  HashCollisionNode *node = GC_MALLOC(sizeof(*node));
  node->type = &NT_HASH_COLLISION;
  node->key = key;
  node->val = val;
  node->hash = hash;

  node->next = NULL;
  return node;
}

static Hashmap *copy_hashmap(Hashmap *map)
{
  Hashmap *new = GC_MALLOC(sizeof(*new));
  memcpy(new, map, sizeof(*new));

  return new;
}

static BitmapIndexedNode *copy_bitmap_indexed_node(BitmapIndexedNode *node)
{
  BitmapIndexedNode *copy = GC_MALLOC(sizeof(*copy));
  memcpy(copy, node, sizeof(*copy));
  return copy;
}

static void *leaf_get(Node *self, int level, void *key, hash_t hash, \
                      equal_fn eq_key, equal_fn eq_val)
{

  LeafNode* node = (LeafNode*)self;

  /* if found - return the value */
  if (eq_key(node->key, key)) {
    return node->val;
  } else {
    return NULL;
  }
}

static void *bitmap_indexed_get(Node *self, int level, void *key, hash_t hash, \
                                equal_fn eq_key, equal_fn eq_val)
{
  BitmapIndexedNode *node = (BitmapIndexedNode*)self;

  int bit = bitpos(hash, level);

  /* if there is a matching bit in bitpos keep searching for a leaf node */
  if(node->bitmap & bit) {
    int idx = bit_index(node->bitmap, bit);
    Node *child = node->children[idx];

    /* look down a level */
    return child->type->get(child, (level + 1), key, hash, eq_key, eq_val);
  }
  /* not found */
  else {
    return NULL;
  }
}

static void *hash_collision_get(Node *self, int level, void *key, hash_t hash, \
                                equal_fn eq_key, equal_fn eq_val)
{
  HashCollisionNode *node = (HashCollisionNode*)self;

  /* check if the key exists */
  while (node) {

    /* if found - return the value */
    if (eq_key(node->key, key)) {
      return node->val;
    }
    node = node->next;
  }
  /* not found */
  return NULL;
}

static Node *leaf_assoc(Node *self, int level, void *key, void *val, hash_t hash, \
                        equal_fn eq_key, equal_fn eq_val, int *result)
{
  LeafNode *node = (LeafNode*)self;

  /* if the hash doesn't match this leaf create a BitmapIndexedNode to
     hold the existing LeafNode and the new LeafNode and return it */
  if (node->hash != hash) {

    BitmapIndexedNode *parent = new_bitmap_indexed_node();

    /* calculate bit-positions of new nodes */
    int node_bit = bitpos(node->hash, level);
    int new_bit = bitpos(hash, level);

    /* can't put two leaves in the same bitmap if they
       have the same bit position */
    if (node_bit != new_bit) {

      /* allocate enough space for two nodes */
      parent->children = GC_MALLOC(sizeof(LeafNode*) * 2);

      /* update the bitmap */
      parent->bitmap |= node_bit;
      parent->bitmap |= new_bit;

      /* work out the new indices */
      int node_idx = bit_index(parent->bitmap, node_bit);
      int new_idx = bit_index(parent->bitmap, new_bit);

      /* create new child node */
      LeafNode *new = new_leaf_node(key, val, hash);

      /* add the leaf nodes */
      parent->children[node_idx] = self;
      parent->children[new_idx] = (Node*)new;

      *result = ADDED;
      return (Node*)parent;
    }
    /* add a node at level + 1 */
    else {
      /* allocate enough space for single node */
      parent->children = GC_MALLOC(sizeof(Node*));

      /* update the bitmap */
      parent->bitmap |= node_bit;

      /* add the original leaf node */
      parent->children[0] = self;

      /* call assoc again on the new bitmap_indexed_node */
      return (Node*)(parent->type->assoc((Node*)parent, level, key, val, hash, \
                                         eq_key, eq_val, result));
    }
  }

  /* if the key/value pair already exists return the original node */
  else if (eq_key(node->key, key) && eq_val(node->val, val)) {

    *result = UNCHANGED;
    return self;
  }
 /* if the key exists with a different value return a copy with the new value */
  else if (eq_key(node->key, key)) {

    LeafNode *new = new_leaf_node(key, val, hash);
    *result = UPDATED;
    return (Node*)new;
  }
  /* if the key is different but has the same hash we have a collision
     so create a pair of linked HashCollisionNodes and return the first one */
  else {

    HashCollisionNode *original = new_hash_collision_node(node->key, node->val, node->hash);
    HashCollisionNode *new = new_hash_collision_node(key, val, hash);
    new->next = original;

    *result = ADDED;
    return (Node*)new;
  }
}

static Node *bitmap_indexed_assoc(Node *self, int level, void *key, void *val, hash_t hash, \
                                  equal_fn eq_key, equal_fn eq_val, int *result)
{
  BitmapIndexedNode *node = (BitmapIndexedNode*)self;

  int bit = bitpos(hash, level);
  int idx = bit_index(node->bitmap, bit);

  /* if a child node already exists at this bitpos */
  if (node->bitmap & bit) {

    /* assoc at the existing child */
    Node *child = node->children[idx];
    Node *new = child->type->assoc(child, (level + 1), key, val, hash,  \
                                   eq_key, eq_val, result);

    /* change was made to the child node */
    if (*result != UNCHANGED) {

      /* copy the node */
      BitmapIndexedNode *copy = copy_bitmap_indexed_node(node);

      /* count the number of 1s in the bitmap */
      int nodes = popcount(node->bitmap);

      /* allocate storage dynamically */
      copy->children = GC_MALLOC(sizeof(Node*) * nodes);

      /* copy all the existing child nodes */
      memcpy(copy->children, node->children, sizeof(Node*) * nodes);
      copy->children[idx] = new;

      return (Node*)copy;
    }
    /* key not found or key/value pair already exists */
    else {
      return self;
    }
  }
  /* create a new child node */
  else {

    /* copy the node */
    BitmapIndexedNode *new= copy_bitmap_indexed_node(node);

    /* set the new node in the bitmap */
    new->bitmap |= bit;

    /* count the number of 1s in the bitmap */
    int nodes = popcount(new->bitmap);

    /* allocate storage dynamically */
    new->children = GC_MALLOC(sizeof(Node*) * nodes);

    /* copy all the existing child nodes */
    memcpy(new->children, node->children, sizeof(Node*) * idx);
    /* shifting over the ones at > idx to make room for the new one */
    memcpy(&new->children[idx + 1], &node->children[idx], sizeof(Node*) * (nodes - idx));

    /* create a new leaf node at idx */
    new->children[idx] = (Node*)new_leaf_node(key, val, hash);

    *result = ADDED;
    return (Node*)new;
  }
}

static Node *hash_collision_assoc(Node *self, int level, void *key, void *val, hash_t hash, \
                                  equal_fn eq_key, equal_fn eq_val, int *result)
{
  HashCollisionNode *node = (HashCollisionNode*)self;

  /* check if the key already exists */
  while (node) {

    if (eq_key(node->key, key)) {
      /* found - exit the loop */
      break;
    }
    /* keep looking */
    node = node->next;
  }

  /* not found */
  if (!node) {
    /* add a new HashCollisionNode to the head of the linked list */
    HashCollisionNode *new = new_hash_collision_node(key, val, hash);
    new->next = (HashCollisionNode*)self;

    *result = ADDED;
    return (Node*)new;
  }

  /* if the key/value pair already exists return the original node */
  if (eq_key(node->key, key) && eq_val(node->val, val)) {

    *result = UNCHANGED;
    return self;
  }

  /* if the key exists with a different value we replace it */

  /* create new node to replace current value */
  HashCollisionNode *new = new_hash_collision_node(key, val, hash);
  /* point it at the tail */
  new->next = node->next;

  /* if we modified the head of the list we're done */
  if ((Node*)node == (Node*)self) {
    *result = UPDATED;
    return (Node*)new;
  }

  /* if we're not at the head of the list then we need to
     copy all the nodes up to the one we changed */

  /* make a copy of the list up to the replaced node */
  HashCollisionNode *prev = new;
  HashCollisionNode *curr= (HashCollisionNode*)self;
  HashCollisionNode *copy = NULL;

  /* copy the nodes */
  while (curr != node) {

    copy = new_hash_collision_node(curr->key, curr->val, curr->hash);
    copy->next = prev;
    prev = copy;
    curr = curr->next;
  }
  *result = UPDATED;
  return (Node*)copy;
}

static Node *leaf_dissoc(Node *self, int level, void *key, hash_t hash, \
                         equal_fn eq_key, equal_fn eq_val, int *result)
{
  /* dissocing a leaf node creates a NULL Node */
  LeafNode* node = (LeafNode*)self;

  if (eq_key(node->key, key)) {
    *result = REMOVED;
    return NULL;
  }
  else {
    /* not found */
    *result = UNCHANGED;
    return self;
  }
}

static Node *bitmap_indexed_dissoc(Node *self, int level, void *key, hash_t hash, \
                                   equal_fn eq_key, equal_fn eq_val, int *result)
{
  BitmapIndexedNode *node = (BitmapIndexedNode*)self;

  int bit = bitpos(hash, level);
  int idx = bit_index(node->bitmap, bit);

  /* a child node already exists */
  if (node->bitmap & bit) {

    /* dissoc a child node at the correct index */
    Node *child = node->children[idx];
    Node *new = child->type->dissoc(child, (level + 1), key, hash, \
                                    eq_key, eq_val, result);

    if (*result != UNCHANGED) {

      /* copy the node */
      BitmapIndexedNode *copy = copy_bitmap_indexed_node(node);

      /* if the new node is now empty */
      if (!new) {
        /* clear bit */
	copy->bitmap &= ~bit;

        /* if the whole BitmapIndexedNode is now empty return NULL instead */
        if (!copy->bitmap) { return NULL; }

        /* count the number of 1s in the whole bitmap */
	int nodes = popcount(copy->bitmap);

        /* allocate storage dynamically */
	copy->children = GC_MALLOC(sizeof(Node*) * nodes);

        /* copy all the existing child nodes */
        memcpy(copy->children, node->children, sizeof(Node*) * idx);
        /* shifting over the ones at > idx to replace the deleted one */
        memcpy(&copy->children[idx], &node->children[idx + 1], sizeof(Node*) * (nodes - idx));

        return (Node*)copy;
      }
      else {
        /* count the number of 1s in the bitmap */
	int nodes = popcount(copy->bitmap);

        /* allocate storage dynamically */
	copy->children = GC_MALLOC(sizeof(Node*) * nodes);

        /* copy all the existing child nodes */
        memcpy(copy->children, node->children, sizeof(Node*) * nodes);
        /* replace the changed one */
        copy->children[idx] = new;

        return (Node*)copy;
      }
    }
    else {
      return self;
    }
  }
  /* node doesn't exist in bitmap */
  else {
    *result = UNCHANGED;
    return self;
  }
}

static Node *hash_collision_dissoc(Node *self, int level, void *key, hash_t hash, \
                                   equal_fn eq_key, equal_fn eq_val, int *result)
{
  HashCollisionNode *head = (HashCollisionNode*)self;
  HashCollisionNode *node = head;

  /* check if the key exists */
  while (node) {
    if (eq_key(node->key, key)) {
      /* found it - exit the loop */
      break;
    }
    /* keep looking */
    node = node->next;
  }
  /* not found */
  if (!node) {
    *result = UNCHANGED;
    return (Node*)node;
  }

  /* if the key is at the head and there are only two entries we
     replace the HashCollisioNode with a LeaftNode */
  if ((node == head) && node->next && !node->next->next) {

    *result = REMOVED;
    LeafNode *new = new_leaf_node(node->next->key, node->next->val, node->next->hash);
    return (Node*)new;
  }

  /* if the key is at the tail and there are only two entries we
     replace the HashCollisioNnode with a LeaftNode */
  if ((node == head->next) && !node->next) {

    *result = REMOVED;
    LeafNode *new = new_leaf_node(head->key, head->val, head->hash);
    return (Node*)new;
  }

  /* if the key exists at the head of the list of more than
     two entries we return the rest of the HashCollisionNodes */
  if (node == (HashCollisionNode*)self) {

    *result = REMOVED;
    return (Node*)node->next;
  }

  /* otherwise the key is in the middle of a list so we
     have to make a copy of the list from the head to
     the removed key and connect it to the tail */
  HashCollisionNode *prev = node->next;
  HashCollisionNode *curr = head;
  HashCollisionNode *copy = NULL;

  /* copy the nodes */
  while (curr != node) {

    copy = new_hash_collision_node(curr->key, curr->val, curr->hash);
    copy->next = prev;
    prev = copy;
    curr = curr->next;
  }
  *result = REMOVED;
  return (Node*)copy;
}

static void leaf_visit(Node *self, visit_fn fn, void **acc)
{
  LeafNode* node = (LeafNode*)self;
  fn(node->key, node->val, acc);
}

static void bitmap_indexed_visit(Node *self, visit_fn fn, void **acc)
{
  BitmapIndexedNode *node = (BitmapIndexedNode*)self;
  int nodes = popcount(node->bitmap);

  for (int i=0; i < nodes; i++) {
    Node *child = node->children[i];
    child->type->visitor(child, fn, acc);
  }
}

static void hash_collision_visit(Node *self, visit_fn fn, void **acc)
{
  HashCollisionNode *node = (HashCollisionNode*)self;

  while (node) {
    fn(node->key, node->val, acc);
    node = node->next;
  }
}

/* function to visit each node and create a list of key/val pairs */
static void iterator_visit(void *key, void *val, void **acc)
{
  /* acc holds a pointer to the list being generated */
  list *lst = *(struct list **)acc;

  /* create a list node to hold the next value */
  list *lst_val = GC_MALLOC(sizeof(*lst_val));
  lst_val->data = val;
  lst_val->next = lst;

  /* create a list node to hold the next key */
  list *lst_key = GC_MALLOC(sizeof(*lst_key));
  lst_key->data = key;
  lst_key->next = lst_val;

  /* return the updated list in acc */
  *acc = lst_key;
}

/* function to advance the iterator */
static Iterator *hashmap_next_fn(Iterator *iter)
{
  assert(iter);

  /* iter->current points to a hashmap entry */
  list *current = iter->current;

  /* check for the end of the data */
  if (!current->next) { return NULL; }

  /* advance the pointer */
  current = current->next;

  /* create a new iterator */
  Iterator *new = iterator_copy(iter);

  /* set the iterator values */
  new->current = current;
  new->value = current->data;

  return new;
}

/*
   default hash implementation djb2 * this algorithm was
   first reported by Dan Bernstein * many years ago in comp.lang.c
*/
static hash_t hash_str(void *obj)
{
  hash_t hash = 5381;

  int c;
  while ((c = *(char*)obj++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

/* default comparison operation */
static int equal_str(void *obj1, void *obj2)
{
  return (strcmp((char *)obj1, (char *)obj2) == 0);
}
