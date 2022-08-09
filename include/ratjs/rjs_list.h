/*****************************************************************************
 *                             Rat Javascript                                *
 *                                                                           *
 * Copyright 2022 Gong Ke                                                    *
 *                                                                           *
 * Permission is hereby granted, free of charge, to any person obtaining a   *
 * copy of this software and associated documentation files (the             *
 * "Software"), to deal in the Software without restriction, including       *
 * without limitation the rights to use, copy, modify, merge, publish,       *
 * distribute, sublicense, and/or sell copies of the Software, and to permit *
 * persons to whom the Software is furnished to do so, subject to the        *
 * following conditions:                                                     *
 *                                                                           *
 * The above copyright notice and this permission notice shall be included   *
 * in all copies or substantial portions of the Software.                    *
 *                                                                           *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN *
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                    *
 *****************************************************************************/

/**
 * \file
 * Double linked list.
 */

#ifndef _RJS_LIST_H_
#define _RJS_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup list List
 * Double linked list
 * @{
 */

/**
 * Initialize a list.
 * \param l The list to be initialized.
 */
static inline void
rjs_list_init (RJS_List *l)
{
    l->prev = l;
    l->next = l;
}

/**
 * Check if the list is empty.
 * \param l The list.
 * \retval RJS_TRUE The list is empty.
 * \retval RJS_FALSE The list is not empty.
 */
static inline RJS_Bool
rjs_list_is_empty (RJS_List *l)
{
    return l->next == l;
}

/**
 * Check if the list only has one node.
 * \param l The list.
 * \retval RJS_TRUE The list has only 1 node.
 * \retval RJS_FALSE The list has 0 or more than 1 node.
 */
static inline RJS_Bool
rjs_list_has_1_node (RJS_List *l)
{
    if (rjs_list_is_empty(l))
        return RJS_FALSE;

    return l->next == l->prev;
}

/**
 * Add a node to the end of the list.
 * \param l The list.
 * \param n The node to be added.
 */
static inline void
rjs_list_append (RJS_List *l, RJS_List *n)
{
    n->prev = l->prev;
    n->next = l;
    l->prev->next = n;
    l->prev = n;
}

/**
 * Add a node to the head of the list.
 * \param l The list.
 * \param n The node to be added.
 */
static inline void
rjs_list_prepend (RJS_List *l, RJS_List *n)
{
    n->prev = l;
    n->next = l->next;
    l->next->prev = n;
    l->next = n;
}

/**
 * Remove a node from the list.
 * \param n The node to be removed.
 */
static inline void
rjs_list_remove (RJS_List *n)
{
    n->prev->next = n->next;
    n->next->prev = n->prev;
}

/**
 * Join 2 lists.
 * \param l1 List 1.
 * \param l2 List 2. The nodes in it willbe added to the tail of l1.
 */
static inline void
rjs_list_join (RJS_List *l1, RJS_List *l2)
{
    RJS_List *h1, *h2, *t1, *t2;

    if (rjs_list_is_empty(l2))
        return;

    h1 = l1;
    t1 = l1->prev;
    h2 = l2->next;
    t2 = l2->prev;

    h1->prev = t2;
    h2->prev = t1;
    t1->next = h2;
    t2->next = h1;
}

/**Traverse the nodes in the list.*/
#define rjs_list_foreach(_l, _n) \
    for ((_n) = (_l)->next; (_n) != (_l); (_n) = (_n)->next)

/**Traverse the nodes in the list reversely.*/
#define rjs_list_foreach_r(_l, _n) \
    for ((_n) = (_l)->prev; (_n) != (_l); (_n) = (_n)->prev)

/**Traverse the nodes' container pointers in the list.*/
#define rjs_list_foreach_c(_l, _n, _s, _m)\
    for ((_n) = RJS_CONTAINER_OF((_l)->next, _s, _m);\
            (_n) != RJS_CONTAINER_OF(_l, _s, _m);\
            (_n) = RJS_CONTAINER_OF((_n)->_m.next, _s, _m))

/**Traverse the nodes' container pointers in the list reversely.*/
#define rjs_list_foreach_c_r(_l, _n, _s, _m)\
    for ((_n) = RJS_CONTAINER_OF((_l)->prev, _s, _m);\
            (_n) != RJS_CONTAINER_OF(_l, _s, _m);\
            (_n) = RJS_CONTAINER_OF((_n)->_m.prev, _s, _m))

/**Traverse the nodes in the list safely.*/
#define rjs_list_foreach_safe(_l, _n, _t)\
    for ((_n) = (_l)->next; ((_n) != (_l)) ? ((_t) = (_n)->next) : 0; (_n) = (_t))

/**Traverse the nodes in the list safely and reversely.*/
#define rjs_list_foreach_safe_r(_l, _n, _t)\
    for ((_n) = (_l)->prev; ((_n) != (_l)) ? ((_t) = (_n)->prev) : 0; (_n) = (_t))

/**Traverse the nodes' container pointers in the list safely.*/
#define rjs_list_foreach_safe_c(_l, _n, _t, _s, _m)\
    for ((_n) = RJS_CONTAINER_OF((_l)->next, _s, _m);\
            (_n) != RJS_CONTAINER_OF(_l, _s, _m)\
                    ? ((_t) = RJS_CONTAINER_OF((_n)->_m.next, _s, _m)) : 0;\
            (_n) = (_t))

/**Traverse the nodes' container pointers in the list safely and reversely.*/
#define rjs_list_foreach_safe_c_r(_l, _n, _t, _s, _m)\
    for ((_n) = RJS_CONTAINER_OF((_l)->prev, _s, _m);\
            (_n) != RJS_CONTAINER_OF(_l, _s, _m)\
                    ? ((_t) = RJS_CONTAINER_OF((_n)->_m.prev, _s, _m)) : 0;\
            (_n) = (_t))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

