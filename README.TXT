Persistent Hashmap and Vector in C
======

Depends on the Boehm garbage collector

======
This is an implementation of Clojure-style persistent hashmaps and vectors implemented in C.
For an explanation see

http://blog.higher-order.net/2009/09/08/understanding-clojures-persistenthashmap-deftwice and
http://blog.higher-order.net/2010/08/16/assoc-and-clojures-persistenthashmap-part-ii.html

and

https://hypirion.com/musings/understanding-persistent-vector-pt-1
https://hypirion.com/musings/understanding-persistent-vector-pt-2
https://hypirion.com/musings/understanding-persistent-vector-pt-3

Sample usage
----------

#include <string.h>
#include <stdio.h>

#include "path/to/hashmap.h"
#include "path/to/vector.h"

/* create a function to hash your keys */
hash_t hash_str(void *obj) {

  hash_t hash = 5381;

  int c;
  while ((c = *(char*)obj++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

/* create a function to compare your keys  for equality
   and another to compare your values if they're different */
int equal_str(void *obj1, void *obj2) {
  return (strcmp((char *)obj1, (char *)obj2) == 0);
}

int main (int argc, char **argv) {

 /* create a hashmap by calling new with your functions for hash and equality */
 Hashmap *h1 = hashmap_make(hash_str, equal_str, equal_str);

 /* add a key/value pair */
 h1 = hashmap_assoc(h1, "key_1", "val_1");

 /* add another key/value pair */
 h1 = hashmap_assoc(h1, "key_2", "val_2");

 /* remove a key/value pair */
 Hashmap *h2 = hashmap_dissoc(h1, "key_1");

 /* get a value using the key */
 char *val_h1 = hashmap_get(h1, "key_1");
 printf("val_h1 is: %s\n", val_h1);
 // ==> "val_ha is: val_1" (h1 not affected by dissoc)

 /* get a value using the key */
 char *val_h2 = hashmap_get(h2, "key_1");
 printf("val_h2 is: %s\n", val_h2);
 // ==> "val_ha is: (null)" (not found in h2)

 int count_h1 = hashmap_count(h1);
 printf("count_h1 is: %d\n", count_h1);
 // ==> "count_h1 is: 2"

 int count_h2 = hashmap_count(h2);
 printf("count_h2 is: %d\n", count_h2);
 // ==> "count_h2 is: 1"

/* create a vector */
Vector *v = vector_make();

/* add items at the end of the vector */
v = vector_push(v, "item0");
v = vector_push(v, "item1");
v = vector_push(v, "item2");
v = vector_push(v, "item3");
printf("count is: %i\n", vector_count(v));
//=> 4

/* save a reference to the original vector */
Vector *v_original = v;

/* remove an item from the end */
v = vector_pop(v);
printf("count is: %i\n", vector_count(v));
//=> 3;

/* get the value at index 1 */
char* item1 = vector_get(v, 1);
printf("item at index 1 is: %s\n", item1);
//=> "item1"

/* change the value at index 1 */
v = vector_set(v, 1, "item1_updated");
item1 = vector_get(v, 1);
printf("item at index 1 is: %s\n", item1);
//=> "item1_updated"

/* original vector is unaffected */
printf("original count is: %i\n", vector_count(v_original));
//=> 4

item1 = vector_get(v_original, 1);
printf("item at index 1 is: %s\n", item1);
//=> "item1"
}
