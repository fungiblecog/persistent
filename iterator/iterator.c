#include <stddef.h>
#include <assert.h>
#include <gc.h>
#include "iterator.h"

inline Iterator *iterator_next(Iterator *iter) {
  assert(iter != NULL);
  return (*iter->next_fn)(iter);
}

inline void *iterator_value(Iterator *iter) {
  assert(iter != NULL);
  return (iter->value);
}

inline Iterator *iterator_copy(Iterator *iter) {
  assert(iter != NULL);

  Iterator *new = GC_MALLOC(sizeof(*new));
  new->next_fn = iter->next_fn;
  new->current = iter->current;
  new->value = iter->value;
  new->data = iter->data;

  return new;
}
