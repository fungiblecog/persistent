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

#ifndef _PERSISTENT_VECTOR_H
#define _PERSISTENT_VECTOR_H

#include "../../iterator/iterator.h"

/* external interface */

typedef struct Vector_s Vector;

/* create a new vector */
Vector *vector_make(void);

/* returns the number of elements in the vector */
int vector_count(Vector* vec);

/* returns 1 if the vector is empty or 0 otherwise */
int vector_empty(Vector* vec);

/* add an element on the end */
Vector *vector_push(Vector *vec, void *data);

/* remove an element from the end */
Vector *vector_pop(Vector *vec);

/* retrieve an element by index */
void *vector_get(Vector *vec, int idx);

/* update an existing element */
Vector *vector_set(Vector *vec, int idx, void *data);

/* return an iterator */
Iterator *vector_iterator_make(Vector *vec);

/* type signature for generic equality function */
typedef int (*equal_fn)(void*, void*);
#endif
