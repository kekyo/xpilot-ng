/* 
 * XPilot NG, a multiplayer space war game.
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell        <bjoern@xpilot.org>
 *      Ken Ronny Schouten   <ken@xpilot.org>
 *      Bert Gijsbers        <bert@xpilot.org>
 *      Dick Balaska         <dick@xpilot.org>
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

/*
 * A double linked list similar to the STL list, but implemented in C.
 */

#include "xpcommon.h"

/* store a list node. */
struct ListNode {
    struct ListNode	*next;
    struct ListNode	*prev;
    void		*data;
};
typedef struct ListNode list_node_t;

/* store the list header. */
struct List {
    list_node_t		tail;
    int			size;
};
/* typedef struct List *list_t; */
/* typedef struct ListNode *list_iter_t; */

static int		lists_allocated;
static int		nodes_allocated;

/* create a new list. */
list_t List_new(void)
{
    list_t		list = (list_t) malloc(sizeof(*list));

    if (list) {
	lists_allocated++;

	list->tail.next = &list->tail;
	list->tail.prev = &list->tail;
	list->tail.data = NULL;
	list->size = 0;
    }

    return list;
}

/* delete a list. */
void List_delete(list_t list)
{
    if (list) {
	List_clear(list);
	list->tail.next = list->tail.prev = NULL;
	free(list);

	lists_allocated--;
    }
}

/* return a list iterator pointing to the first element of the list. */
list_iter_t List_begin(list_t list)
{
    return list->tail.next;
}

/* return a list iterator pointing to the one past the last element of the list. */
list_iter_t List_end(list_t list)
{
    return &list->tail;
}

/* return a pointer to the last list element. */
void* List_back(list_t list)
{
    return list->tail.prev->data;
}

/* return a pointer to the first list element. */
void* List_front(list_t list)
{
    return list->tail.next->data;
}

/* erase all elements from the list. */
void List_clear(list_t list)
{
    while (!List_empty(list)) {
	List_pop_front(list);
    }
}

/* return true if list is empty. */
int List_empty(list_t list)
{
    return (list->size == 0);
}

/* erase element at list position and return next position */
list_iter_t List_erase(list_t list, list_iter_t pos)
{
    list_iter_t next, prev;

    if (pos == &list->tail) {
	return List_end(list);
    }

    next = pos->next;
    prev = pos->prev;
    prev->next = next;
    next->prev = prev;
    list->size--;

    pos->prev = NULL;
    pos->next = NULL;
    pos->data = NULL;
    free(pos);

    nodes_allocated--;

    return next;
}

/* erase a range of list elements excluding last. */
list_iter_t List_erase_range(list_t list, list_iter_t first, list_iter_t last)
{
    while (first != last) {
	first = List_erase(list, first);
    }
    return first;
}

/* insert a new element into the list at position
 * and return new position or NULL on failure. */
list_iter_t List_insert(list_t list, list_iter_t pos, void *data)
{
    list_iter_t		node = (list_iter_t) malloc(sizeof(*node));

    if (node) {
	node->next = pos;
	node->prev = pos->prev;
	node->data = data;
	node->prev->next = node;
	node->next->prev = node;
	list->size++;

	nodes_allocated++;
    }

    return node;
}

/* remove the first element from the list and return a pointer to it. */
void* List_pop_front(list_t list)
{
    void *data = list->tail.next->data;
    List_erase(list, list->tail.next);
    return data;
}

/* remove the last element from the list and return a pointer to it. */
void* List_pop_back(list_t list)
{
    void *data = list->tail.prev->data;
    List_erase(list, list->tail.prev);
    return data;
}

/* add a new element to the beginning of the list.
 * and return the new position or NULL on failure. */
list_iter_t List_push_front(list_t list, void *data)
{
    return List_insert(list, list->tail.next, data);
}

/* append a new element at the end of the list.
 * and return the new position or NULL on failure. */
list_iter_t List_push_back(list_t list, void *data)
{
    return List_insert(list, &list->tail, data);
}

/*
 * Find an element in the list and return an iterator pointing to it.
 * Note that this is very slow because it traverses the entire list
 * searching for an element.
 */
list_iter_t List_find(list_t list, void *data)
{
    return List_find_range(List_begin(list), List_end(list), data);
}

/*
 * Find an element in a range of elements (excluding last) and return
 * an iterator pointing to it.  Note that this is a very slow operation.
 */
list_iter_t List_find_range(list_iter_t first, list_iter_t last, void *data)
{
    list_iter_t		pos = first;

    while (pos != last && pos->data != data) {
	pos = pos->next;
    }

    return pos;
}

/*
 * Remove all elements from the list which are equal to data.
 * Note that this is very slow because it traverses the entire list.
 * The return value is the number of successful removals.
 */
int List_remove(list_t list, void *data)
{
    list_iter_t		pos = List_begin(list);
    list_iter_t		end = List_end(list);
    int			count = 0;

    while (pos != end) {
	pos = List_find_range(pos, end, data);
	if (pos != end) {
	    pos = List_erase(list, pos);
	    count++;
	}
    }

    return count;
}

/* return the number of elements in the list. */
int List_size(list_t list)
{
    return list->size;
}

/* advance list iterator one position and return new position. */
list_iter_t List_iter_forward(list_iter_t *pos)
{
    (*pos) = (*pos)->next;
    return (*pos);
}

/* move list iterator one position backwards and return new position. */
list_iter_t List_iter_backward(list_iter_t *pos)
{
    (*pos) = (*pos)->prev;
    return (*pos);
}

/* return data at list position. */
void* List_iter_data(list_iter_t pos)
{
    return pos->data;
}


