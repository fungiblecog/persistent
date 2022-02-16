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

#ifndef _PERSISTENT_HASHMAP_H
#define _PERSISTENT_HASHMAP_H

#include "../../iterator/iterator.h"

/* External Interface */

/* type for hashes */
typedef unsigned int hash_t;

/* type for Hashmaps */
typedef struct Hashmap Hashmap;

/* type signature for generic equality function */
typedef int (*equal_fn)(void *val1, void *val2);

/* type signature for generic hash function */
typedef hash_t (*hash_fn)(void *key);

/* type signature for a generic function to apply to all nodes */
typedef void (*visit_fn)(void *key, void *val, void **result);

/*
create a new hashmap. provide three functions that:
- returns a hash given a key
- tests two keys for equality (and returns 1 (true) or 0 (false))
- tests two values for equality (and returns 1 (true) or 0 (false))
*/
Hashmap *hashmap_make(hash_fn hash, equal_fn eql_keys, equal_fn eql_vals);

/* true if there are no key/value pairs */
int hashmap_empty(Hashmap *map);

/* returns a count of key/value pairs stored in map */
int hashmap_count(Hashmap* map);

/* returns a hashmap that is the same as map but with key and val added */
Hashmap *hashmap_assoc(Hashmap* map, void* key, void* val);

/* returns a hashmap that is the same as map but with key
   (and associated val) removed if it exists */
Hashmap *hashmap_dissoc(Hashmap* map, void* key);

/* returns the value associated with key if it exists in map or NULL */
void *hashmap_get(Hashmap* map, void* key);

/* return an iterator for a hashmap */
Iterator *hashmap_iterator_make(Hashmap *map);

/* applies fn to every key/value pair along with acc which accumulates the result */
void hashmap_visit(Hashmap* map, visit_fn fn, void **acc);
#endif
