#ifndef _MAL_LINKED_LIST_H
#define _MAL_LINKED_LIST_H

#include "../../iterator/iterator.h"

/* a function pointer that compares two gptrs */
typedef int (*cmp_fn)(void *, void *);

typedef struct pair_s pair;
typedef pair List;

/* linked list is constructed of pairs */
struct pair_s {
  void *data;
  pair *next;
};

/* Note: an empty list is NULL */

/* interface */
List *list_make(void *val);
int list_count(List *lst);
int list_empty(List *lst);
List *list_cons(List *lst, void *val);
void *list_nth(List *lst, int n);
void *list_first(List *lst);
List *list_rest(List *lst);
List *list_reverse(List *lst); /* note: destructive */
List *list_copy(List *lst);
List *list_concatenate(List *lst1, List *lst2);
int list_find(List *lst, void *key, cmp_fn fn);
Iterator *list_iterator_make(List *lst);
#endif
