#ifndef _MAL_ITERATOR_H
#define _MAL_ITERATOR_H

typedef struct Iterator_s Iterator;

/* a function that takes an Iterator* and returns
   an Iterator* for the next in the sequence */
typedef Iterator *((*iter_fn)(Iterator *iter));

struct Iterator_s {
  /* the current value pointed at (mandatory) */
  void *value;
  /* a function to get the next iterator (mandatory) */
  iter_fn next_fn;
  /* a reference to the current position (optional) */
  void *current;
  /* a place to store anything needed between calls to next (optional) */
  void *data;
};

/*
   any collection can implement the iterator interface.
   each collection must provide:
   a) a constructor function for the first element and
   b) a function that takes an iterator and returns the next (returns NULL at the end)
*/

/* public interface */
Iterator *iterator_next(Iterator *iter);
void *iterator_value(Iterator *iter);
Iterator *iterator_copy(Iterator *iter);
#endif
