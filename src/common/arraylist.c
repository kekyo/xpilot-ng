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

#include "xpcommon.h"

#define ARRAYLIST_INITIAL_NUM_ELEMENTS 16
#define ARRAYLIST_ELEMENT_ALIGNMENT sizeof(double)

void Arraylist_clear(arraylist_t *alp)
{
    alp->num_elements = 0;
}

void *Arraylist_get(arraylist_t *alp, int ind)
{
    if (ind < 0 || ind >= alp->num_elements)
	return NULL;
    return Arraylist_get_element_pointer(alp, ind);
}

int Arraylist_add(arraylist_t *alp, void *element)
{
    size_t n;
    void *p;

    if (alp->num_elements < alp->max_elements) {
	p = Arraylist_get_element_pointer(alp, alp->num_elements);
	memcpy(p, element, alp->element_size);
	return alp->num_elements++;
    }

    if (alp->max_elements >= ARRAYLIST_INITIAL_NUM_ELEMENTS)
	n = alp->max_elements * 2;
    else
	n = ARRAYLIST_INITIAL_NUM_ELEMENTS;
    p = realloc(alp->elements, n * alp->element_padded_size);
    if (p == NULL)
	return -1;

    alp->elements = p;
    alp->max_elements = n;

    return Arraylist_add(alp, element);
}

void Arraylist_fast_remove(arraylist_t *alp, int ind)
{
    void *p, *last_p;

    /* get pointer to the element to remove */
    p = Arraylist_get(alp, ind);
    if (!p)
	/* warn or return error ? */
	return;

    /* last element? */
    if (ind == alp->num_elements - 1) {
	/* no need to copy anything */
	alp->num_elements--;
	return;
    }

    /* copy last element to index 'ind' */
    last_p = Arraylist_get(alp, alp->num_elements - 1);
    assert(last_p != NULL);
    memcpy(p, last_p, alp->element_padded_size);
    alp->num_elements--;
}

arraylist_t *Arraylist_alloc(size_t element_size)
{
    arraylist_t *alp = XCALLOC(arraylist_t, 1);
    size_t padded_size = element_size;

    if (((element_size % ARRAYLIST_ELEMENT_ALIGNMENT) != 0)
	&& element_size != 1
	&& element_size != 2
	&& element_size != 4) {
	padded_size /= ARRAYLIST_ELEMENT_ALIGNMENT;
	padded_size *= ARRAYLIST_ELEMENT_ALIGNMENT;
	padded_size += ARRAYLIST_ELEMENT_ALIGNMENT;

	/*warn("Arraylist_create: Increasing element size from %d to %d.",
	  element_size, padded_size);*/
    }

    if (alp == NULL)
	goto failed;

    alp->elements = calloc(ARRAYLIST_INITIAL_NUM_ELEMENTS, padded_size);
    if (alp->elements == NULL)
	goto failed;

    alp->element_size = element_size;
    alp->element_padded_size = padded_size;
    alp->num_elements = 0;
    alp->max_elements = ARRAYLIST_INITIAL_NUM_ELEMENTS;

    return alp;

 failed:
    return NULL;
}

void Arraylist_free(arraylist_t *alp)
{
    assert(alp != NULL);
    assert(alp->elements != NULL);
    XFREE(alp->elements);
    XFREE(alp);
}
