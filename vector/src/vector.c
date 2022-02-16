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

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <gc.h>

#include "vector.h"

#define BITS 5
#define WIDTH (1 << BITS)
#define MASK (WIDTH - 1)

typedef struct Node {
  /* a Node either holds child nodes or data elements */
  union {
    struct Node *children[WIDTH];
    void *elements[WIDTH];
  };
} Node;

struct Vector_s {
  Node *head;
  int count;
  int levels;

  /* the tail is filled before being appended as a child node */
  Node *tail;
  int tail_count;
};

/* forward declarations */
static Node *node_new(void);
static Node *node_copy(Node *node);
static Node *pop_from_head(Vector *vec, Node *node, int level, int idx);
static void vector_append_tail(Vector *vec);
static void vector_pop_from_head(Vector *vec);

/* internal functions */
static Node *node_new(void)
{
  return GC_MALLOC(sizeof(Node));
}

static Node *node_copy(Node *node)
{
  Node *new = GC_MALLOC(sizeof(Node));
  memcpy(new->children, node->children, sizeof(Node*) * WIDTH);

  return new;
}

static Vector *vector_copy(Vector *vec)
{
  Vector *copy = GC_MALLOC(sizeof(Vector));

  copy->levels = vec->levels;
  copy->count = vec->count;
  copy->tail_count = vec->tail_count;
  copy->head = node_copy(vec->head);
  copy->tail = vec->tail;

  return copy;
}

static void vector_append_tail(Vector *vec)
{
  /* The number of elements that can be stored
     without adding a new level */
  int capacity = (1 << (BITS * (vec->levels + 1)));

  /* if the tree is full, add a level */
  if(vec->count == capacity) {

    Node *new_root = node_new();
    new_root->children[0] = vec->head;
    vec->head = new_root;
    vec->levels++;
  }

  /* loop down the levels */
  int idx = vec->count - vec->tail_count;
  Node *cur = NULL;
  Node *prev = vec->head;

  for(int level = (BITS * vec->levels); level > 0; level -= BITS) {

    int index = (idx >> level) & MASK;
    cur = prev->children[index];

    /* at the bottom of the tree insert the tail */
    if(!cur && (level - BITS) == 0) {
      prev->children[index] = vec->tail;
      break;
    }
    /* if there is a NULL node create a new one */
    if(!cur) {
      prev->children[index] = node_new();
      cur = prev->children[index];
    }
    prev = cur;
  }

  /* add a new tail */
  vec->tail = node_new();
  vec->tail_count = 0;

  return;
}

static void vector_pop_from_head(Vector *vec)
{
  /* pop the last element (vec->count - 1) from the head node */
  vec->head = pop_from_head(vec, vec->head, (BITS * vec->levels), vec->count - 1);

  /* if the head only has a single child (and there is more than one level)
     then it is redundant so get rid of it and decrese the number of levels by one */
  if (!vec->head->children[1] && vec->levels > 1) {
    vec->head = vec->head->children[0];
    vec->levels--;
  }
  return;
}

static Node *pop_from_head(Vector *vec, Node *node, int level, int idx)
{
  int index = (idx >> level) & MASK;

  /* at the bottom push the node to the tail */
  if (level - BITS == 0) {
    vec->tail = node->children[index];
    vec->tail_count = WIDTH;
    node->children[index] = NULL;

    /* if the node is empty (and we have more than one level
       in the tree) return NULL instead of an empty node */
    if (!node->children[0] && vec->levels > 1) {
      node = NULL;
    }
    return node;

  } /* if not at the bottom of the tree call pop_from_head on the next level down
     and assign the returned tree to the right child node */
  else {
    node->children[index] = pop_from_head(vec, node->children[index], level - BITS, idx);

    /* if the node is empty return NULL instead of an empty node */
    if (!node->children[0]) {
      node = NULL;
    }
    return node;
  }
}

/* external API */
Vector *vector_make(void)
{
  Vector *vec = GC_MALLOC(sizeof(Vector));

  /* start with one empty level */
  vec->head = node_new();
  vec->levels = 1;

  /* start with an empty tail */
  vec->tail = node_new();
  vec->tail_count = 0;

  vec->count = 0;
  return vec;
}

int vector_count(Vector *vec)
{
  return vec->count;
}

int vector_empty(Vector *vec)
{
  return (vec->count == 0);
}

Vector *vector_push(Vector *vec, void *data)
{
  Vector *copy = vector_copy(vec);

  /* if the tail is full, append it to the head */
  if (copy->tail_count == WIDTH) {
    vector_append_tail(copy);
  }

  /* add the new item to the tail */
  copy->tail->elements[copy->tail_count] = data;
  copy->count++;
  copy->tail_count++;

  return copy;
}

Vector *vector_pop(Vector *vec)
{
  /* check the vector isn't empty */
  if (vec->count == 0) { return vec; }

  Vector *copy = vector_copy(vec);

  /* if the tail is empty move the last node
     from the head up to the tail */
  if (copy->tail_count == 0) {
    vector_pop_from_head(copy);
  }

  /* remove the item from the tail */
  copy->tail->elements[copy->tail_count - 1] = NULL;
  copy->count--;
  copy->tail_count--;

  return copy;
}

void *vector_get(Vector *vec, int idx)
{
  /* check the bounds */
  if (idx < 0 || idx >= vec->count) {
    return NULL;
  }

  Node *next = NULL;
  Node *prev = vec->head;

  /* if idx is in the tail */
  int tail_offset = vec->count - vec->tail_count;
  if (idx >= tail_offset) {
    return vec->tail->elements[idx - tail_offset];
  }
  else {
    /* loop down the levels */
    for(int level = (BITS * vec->levels); level > 0; level -= BITS) {
      next = prev->children[(idx >> level) & MASK];
      prev = next;
    }
    return prev->elements[idx & MASK];
  }
}

Vector *vector_set(Vector *vec, int idx, void *data)
{
  /* check the bounds */
  if (idx < 0 || idx >= vec->count) {
    return vec;
  }

  /* copy the root node */
  Vector *copy = vector_copy(vec);

  /* if idx is in the tail, copy and update the tail */
  int tail_offset = vec->count - vec->tail_count;
  if (idx >= tail_offset) {
      copy->tail = node_copy(vec->tail);
      copy->tail->elements[idx - tail_offset] = data;
      return copy;
  }
  else {
    /* loop down the levels */
    Node *cur = NULL;
    Node *prev = copy->head;

    for(int level = (BITS * vec->levels); level > 0; level -= BITS) {
      cur = prev->children[(idx >> level) & MASK];
      prev = cur;
    }
    cur->elements[idx & MASK] = data;
    return copy;
  }
}

static Iterator *vector_next_fn(Iterator *iter)
{
  assert(iter);

  Vector *vec = iter->data;
  uintptr_t idx = (uintptr_t)++iter->current;

  /* check for end of the array */
  if (idx == vec->count) { return NULL; }

  Iterator *new = iterator_copy(iter);

  /* increment the current pointer */
  new->current = (void *)idx;

  /* set the next value */
  new->value = vector_get(vec, idx);

  return new;
}

Iterator *vector_iterator_make(Vector *vec)
{
  assert(vec);

  /* empty vector returns a NULL iterator */
  if (vector_empty(vec)) { return NULL; }

  /* create an iterator */
  Iterator *iter = GC_MALLOC(sizeof(*iter));

  /* install the next function for a vector */
  iter->next_fn = vector_next_fn;

  /* save a reference to the source data */
  iter->data = vec;

  /* set current to the intial array index */
  iter->current = (void *)0;

  /* set value to the value of the first array element */
  iter->value = vector_get(vec, 0);

  return iter;
}
