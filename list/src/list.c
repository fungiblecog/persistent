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

#include <gc.h>
#include <assert.h>
#include "list.h"

/* create a new 1 element list. note an empty list is just NULL */
List *list_make(void *data)
{
  return list_cons(NULL, data);
}

/* returns true if list is empty (NULL) */
int list_empty(List *lst)
{
  return (!lst);
}

/* return a list with an element added at the head */
List *list_cons(List *lst, void *val)
{
  pair *head = GC_malloc(sizeof(*head));
  head->data = val;
  head->next = lst;

  return head;
}

/* return the first element in the list (the head) */
void *list_first(List *lst)
{
  return (lst ? lst->data : NULL);
}

/* return a list of all emements except the head */
List *list_rest(List *lst)
{
  return (lst ? lst->next : NULL);
}

/* return a count of the elements in the list */
int list_count(List *lst)
{
  /* handle empty case */
  if (!lst) { return 0; }

  int ctr = 1;

  /* walk the list counting elements */
  while(lst->next) {
    ctr++;
    lst = lst->next;
  }
  return ctr;
}

/* return the nth element counting from the head */
void *list_nth(List *lst, int n)
{
  /* handle empty case */
  if (!lst) { return NULL; }

  int idx = 0;

  /* walk the list to get nth element */
  while (lst) {

    /* found */
    if (n == idx) {
      return lst->data;
    }
    /* keep walking */
    idx++;
    lst = lst->next;
  }
  /* reached end of list before n */
  return NULL;
}

/* return the list reversed *destructive* */
List *list_reverse(List *lst)
{
  /* list is not empty and has more than one element */
  if (lst && lst->next) {

    pair *prev = NULL, *next = NULL, *current = lst;

    while (current) {

      /* stash current value of next pointer --> */
      next = current->next;

      /* reverse the next pointer on current pair <-- */
      current->next = prev;

      /* move on to next pair and repeat --> */
      prev = current;
      current = next;
    }
    /* new head of list is in prev when current = NULL */
    lst = prev;
  }
  return lst;
}

/* return a new list with identical contents */
List *list_copy(List *lst)
{
  /* handle empty case */
  if(!lst) { return NULL; }

  List *copy = NULL;

  /* walk the list */
  while (lst) {

    /* push the elements to the new list */
    copy = list_cons(copy, lst->data);
    lst = lst->next;
  }
  /* note that the copy is backwards */
  return list_reverse(copy);
}

/* return a list which is the input lists joined together */
List *list_concatenate(List *lst_1, List *lst_2)
{
  /* make a copy of the second list */
  List *lst_cat = NULL;
  lst_cat = list_copy(lst_2);

  /* push the reverse of the first list onto the copy of the second list */
  lst_1 = list_reverse(lst_1);

  List *lst_iter = lst_1;
  while (lst_iter) {

    void * val = lst_iter->data;
    lst_cat = list_cons(lst_cat, val);
    lst_iter = lst_iter->next;
  }

  /* restore the original first list */
  lst_1 = list_reverse(lst_1);

  /* return the concatenated list */
  return lst_cat;
}

/* return the position of the element matching
   keystring when fn is applied to the data */
int list_find(List *lst, void *val, cmp_fn fn)
{
  /* handle empty cases */
  if (!lst || !val) { return -1; }

  List *current = lst;
  long ctr = 0;
  /* walk the list */
  while (current) {

    if (fn(val, current->data)) {
      /* return the 0-indexed position of the first match */
      return ctr;
    }
    else {
      /* skip to the next item in the list */
      current = current->next;
      ctr++;
    }
  }
  /* not found */
  return -1;
}

static Iterator *list_next_fn(Iterator *iter)
{
  assert(iter);

  /* iter->current points to the current pair */
  List *current = iter->current;

  /* advance the pointer */
  current = current->next;

  /* check for end of the list */
  if (!current) { return NULL; }

  Iterator *new = iterator_copy(iter);

  /* set the next pair */
  new->current = current;

  /* set the next value */
  new->value = current->data;

  return new;
}

Iterator *list_iterator_make(List *lst)
{
  /* empty list returns a NULL iterator */
  if (!lst) { return NULL; }

  /* create an iterator */
  Iterator *iter = GC_MALLOC(sizeof(*iter));

  /* install the next function for a list */
  iter->next_fn = list_next_fn;

  /* set current to a pointer to the head of the list */
  iter->current = lst;

  /* set value to the value of list head */
  iter->value = lst->data;

  return iter;
}
