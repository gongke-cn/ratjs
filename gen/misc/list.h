#ifndef _LIST_H_
#define _LIST_H_

/**List node.*/
typedef struct List_s List;
/**List node.*/
struct List_s {
    List *prev; /**< The previous node.*/
    List *next; /**< the next node.*/
};

/**Check if the list is empty.*/
static inline int
list_is_empty (List *l)
{
    return (l->next == l);
}

/**Initialize the list.*/
static inline void
list_init (List *l)
{
    l->prev = l;
    l->next = l;
}

/*Append a node to the list.*/
static inline void
list_append (List *l, List *n)
{
    n->prev = l->prev;
    n->next = l;
    l->prev->next = n;
    l->prev = n;
}

#define offset_of(_s, _m)\
    ((size_t)(&((_s*)0)->_m))
#define container_of(_p, _s, _m)\
    ((_s*)(((size_t)(_p)) - offset_of(_s, _m)))
#define list_foreach(_l, _n, _s, _m)\
    for (_n = container_of((_l)->next, _s, _m);\
            _n != container_of(_l, _s, _m);\
            _n = container_of((_n)->_m.next, _s, _m))
#define list_foreach_safe(_l, _n, _t, _s, _m)\
    for (_n = container_of((_l)->next, _s, _m);\
            (_n != container_of(_l, _s, _m)) ? (_t = container_of((_n)->_m.next, _s, _m)) : 0;\
            _n = _t)

#endif