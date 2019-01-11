/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 2005 Kristian Söderblom <kps@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#ifndef TYPES_H
# include "types.h"
#endif

#include "xpcommon.h"

typedef struct {
    void	*elements;
    size_t	element_size;
    size_t	element_padded_size;
    int 	num_elements;
    int 	max_elements;
} arraylist_t;

static inline void *Arraylist_get_element_pointer(arraylist_t *alp, int ind)
{
    return (uint8_t *)alp->elements + alp->element_padded_size * ind;
}

static inline int Arraylist_get_num_elements(arraylist_t *alp)
{
    return alp->num_elements;
}

static inline int Arraylist_get_max_elements(arraylist_t *alp)
{
    return alp->max_elements;
}

arraylist_t *Arraylist_alloc(size_t element_size);
void Arraylist_free(arraylist_t *alp);

/*
 * Removes all of the elements from this list by setting number
 * of elements to 0.
 */
void Arraylist_clear(arraylist_t *alp);

/* Get element at index 'ind'. */
void *Arraylist_get(arraylist_t *alp, int ind);

/* Set element at index 'ind' (possibly overwriting old element). */
void Arraylist_set(arraylist_t *alp, int ind, void *element);

/* Add element at end of list, return its index or -1 if failed. */
int Arraylist_add(arraylist_t *alp, void *element);

/* Insert element at position 'ind', return its 'ind' or -1 if failed. */
int Arraylist_insert(arraylist_t *alp, int ind, void *element);

/*
 * Remove element at 'ind', require preservation of element order.
 * Slower than Arraylist_fast_remove, because this function might
 * move other elements. Doesn't free any memory.
 */
void Arraylist_remove(arraylist_t *alp, int ind);

/*
 * Remove element at 'ind', don't require preservation of element order.
 * Doesn't free any memory.
 */
void Arraylist_fast_remove(arraylist_t *alp, int ind);

/* Make max elements equal num elements. */
void Arraylist_trim(arraylist_t *alp);

/* Make sure max elements is at least 'capacity', return 'capacity' or -1. */
int Arraylist_ensure_capacity(arraylist_t *alp, int capacity);

/* Returns true if this list contains the specified element. */
bool Arraylist_contains(arraylist_t *alp, void *element);

/* Sort arraylist entries, check qsort(3) for explanation of 'cmp'. */
void Arraylist_sort(arraylist_t *alp, int (*cmp)(const void *, const void *));

typedef struct {
    uint8_t 	*current_element;
    int 	current_ind;
    int 	num_elements;
    size_t	element_padded_size;
} arraylist_iterator_t;


/* Might implement something like this, but don't count on it. */
arraylist_iterator_t *Arraylist_iterator(arraylist_t *alp);
void Arraylist_iterator_free(arraylist_iterator_t *ip);
/* Initialise static iterator whose pointer is 'ip' */
void Arraylist_static_iterator(arraylist_t *alp, arraylist_iterator_t *ip);
bool Iterator_has_next(arraylist_iterator_t *ip);
void *Iterator_get_next(arraylist_iterator_t *ip);


#endif
