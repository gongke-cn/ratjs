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

#include "ratjs_internal.h"

/**Set's entry.*/
typedef struct {
    RJS_HashEntry he;  /**< hash table entry.*/
    RJS_List      ln;  /**< List node data.*/
    RJS_Value     key; /**< Key value.*/
} RJS_SetEntry;

/**Map's entry.*/
typedef struct {
    RJS_SetEntry  se;    /**< Base set entry data.*/
    RJS_Value     value; /**< Value of the entry.*/
} RJS_MapEntry;

#if ENABLE_WEAK_SET
/**Weak set's entry.*/
typedef struct {
    RJS_SetEntry  se;       /**< Base set entry data.*/
    RJS_WeakRef  *weak_ref; /**< Weak reference.*/
} RJS_WeakSetEntry;
#endif /*ENABLE_WEAK_SET*/

#if ENABLE_WEAK_MAP
/**Weak map's entry.*/
typedef struct {
    RJS_MapEntry  me;       /**< Base map entry data.*/
    RJS_WeakRef  *weak_ref; /**< Weak reference.*/
} RJS_WeakMapEntry;
#endif /*ENABLE_WEAK_MAP*/

/**Hash object.*/
typedef struct {
    RJS_Object object; /**< Base object.*/
    RJS_Hash   hash;   /**< Entries hash table.*/
    RJS_List   list;   /**< Entries list.*/
    RJS_List   iters;  /**< Iterators list.*/
} RJS_HashObject;

/**Hash iterator type.*/
typedef enum {
    RJS_HASH_ITER_KEY,       /**< Key only.*/
    RJS_HASH_ITER_VALUE,     /**< Value only.*/
    RJS_HASH_ITER_KEY_VALUE  /**< Key and value.*/
} RJS_HashIterType;

/**Hash iterator.*/
typedef struct {
    RJS_Object       object; /**< Base object.*/
    RJS_HashIterType type;   /**< Iterator type.*/
    RJS_List         ln;     /**< List node data.*/
    RJS_Value        hash;   /**< The hash object.*/
    RJS_List        *curr;   /**< The current list node.*/
    RJS_Bool         done;   /**< The iterator is done.*/
} RJS_HashIter;

#if ENABLE_SET
/*Scan the referenced things in the set.*/
static void
set_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_HashObject *ho = ptr;
    RJS_SetEntry   *se;

    rjs_list_foreach_c(&ho->list, se, RJS_SetEntry, ln) {
        rjs_gc_scan_value(rt, &se->key);
    }

    rjs_object_op_gc_scan(rt, &ho->object);
}
#endif /*ENABLE_SET*/

#if ENABLE_MAP
/*Scan the referenced things in the map.*/
static void
map_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_HashObject *ho = ptr;
    RJS_MapEntry   *me;

    rjs_list_foreach_c(&ho->list, me, RJS_MapEntry, se.ln) {
        rjs_gc_scan_value(rt, &me->value);
        rjs_gc_scan_value(rt, &me->se.key);
    }

    rjs_object_op_gc_scan(rt, &ho->object);
}
#endif /*ENABLE_MAP*/

/*Free the hash object.*/
static void
hash_op_gc_free (RJS_Runtime *rt, RJS_HashObject *ho, size_t esize)
{
    RJS_SetEntry *se, *nse;

    rjs_list_foreach_safe_c(&ho->list, se, nse, RJS_SetEntry, ln) {
        rjs_free(rt, se, esize);
    }

    rjs_hash_deinit(&ho->hash, &rjs_hash_value_ops_0, rt);
    rjs_object_deinit(rt, &ho->object);

    RJS_DEL(rt, ho);
}

#if ENABLE_WEAK_MAP || ENABLE_WEAK_SET
/*Scan referenced things in the weak hash object.*/
static void
weak_hash_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_HashObject *ho = ptr;

    rjs_object_op_gc_scan(rt, &ho->object);
}
#endif /*ENABLE_WEAK_MAP || ENABLE_WEAK_SET*/

/*Create a new hash table object.*/
static RJS_Result
hash_new (RJS_Runtime *rt, RJS_Value *rv, RJS_Value *nt, int proto_idx, const RJS_ObjectOps *ops)
{
    RJS_HashObject *ho;
    RJS_Result      r;

    RJS_NEW(rt, ho);

    rjs_hash_init(&ho->hash);
    rjs_list_init(&ho->list);
    rjs_list_init(&ho->iters);

    r = rjs_ordinary_init_from_constructor(rt, &ho->object, nt, proto_idx, ops, rv);
    if (r == RJS_ERR)
        RJS_DEL(rt, ho);

    return r;
}

#if ENABLE_SET || ENABLE_WEAK_SET
/*Create a new set.*/
static RJS_Result
set_new (RJS_Runtime *rt, RJS_Value *rv, RJS_Value *nt, int proto_idx, const RJS_ObjectOps *ops, RJS_Value *iterable)
{
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *adder = rjs_value_stack_push(rt);
    RJS_Value   *ir    = rjs_value_stack_push(rt);
    RJS_Value   *iv    = rjs_value_stack_push(rt);
    RJS_Value   *res   = rjs_value_stack_push(rt);
    RJS_Iterator iter;
    RJS_Result   r;

    rjs_iterator_init(rt, &iter);

    if (!nt) {
        r = rjs_throw_type_error(rt, _("the function must be used as a constructor"));
        goto end;
    }

    if ((r = hash_new(rt, rv, nt, proto_idx, ops)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, iterable) || rjs_value_is_null(rt, iterable)) {
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_get(rt, rv, rjs_pn_add(rt), adder)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, adder)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iter)) == RJS_ERR)
        goto end;

    while (1) {
        if ((r = rjs_iterator_step(rt, &iter, ir)) == RJS_ERR)
            goto iter_end;

        if (!r)
            break;

        if ((r = rjs_iterator_value(rt, ir, iv)) == RJS_ERR)
            goto iter_end;

        if ((r = rjs_call(rt, adder, rv, iv, 1, res)) == RJS_ERR)
            goto iter_end;
    }

iter_end:
    if (r == RJS_ERR)
        rjs_iterator_close(rt, &iter);

    if (r == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_iterator_deinit(rt, &iter);
    rjs_value_stack_restore(rt, top);
    return r;
}
#endif /*ENABLE_SET || ENABLE_WEAK_SET*/

#if ENABLE_MAP || ENABLE_WEAK_MAP
/*Create a new map.*/
static RJS_Result
map_new (RJS_Runtime *rt, RJS_Value *rv, RJS_Value *nt, int proto_idx, const RJS_ObjectOps *ops, RJS_Value *iterable)
{
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *adder = rjs_value_stack_push(rt);
    RJS_Value   *ir    = rjs_value_stack_push(rt);
    RJS_Value   *iv    = rjs_value_stack_push(rt);
    RJS_Value   *k     = rjs_value_stack_push(rt);
    RJS_Value   *v     = rjs_value_stack_push(rt);
    RJS_Value   *res   = rjs_value_stack_push(rt);
    RJS_Iterator iter;
    RJS_Result   r;

    rjs_iterator_init(rt, &iter);

    if (!nt) {
        r = rjs_throw_type_error(rt, _("the function must be used as a constructor"));
        goto end;
    }

    if ((r = hash_new(rt, rv, nt, proto_idx, ops)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, iterable) || rjs_value_is_null(rt, iterable)) {
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_get(rt, rv, rjs_pn_set(rt), adder)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, adder)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iter)) == RJS_ERR)
        goto end;

    while (1) {
        if ((r = rjs_iterator_step(rt, &iter, ir)) == RJS_ERR)
            goto iter_end;

        if (!r)
            break;

        if ((r = rjs_iterator_value(rt, ir, iv)) == RJS_ERR)
            goto iter_end;

        if (!rjs_value_is_object(rt, iv)) {
            r = rjs_throw_type_error(rt, _("the result is not an object"));
            goto iter_end;
        }

        if ((r = rjs_get_index(rt, iv, 0, k)) == RJS_ERR)
            goto iter_end;

        if ((r = rjs_get_index(rt, iv, 1, v)) == RJS_ERR)
            goto iter_end;

        if ((r = rjs_call(rt, adder, rv, k, 2, res)) == RJS_ERR)
            goto iter_end;
    }

iter_end:
    if (r == RJS_ERR)
        rjs_iterator_close(rt, &iter);

    if (r == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_iterator_deinit(rt, &iter);
    rjs_value_stack_restore(rt, top);
    return r;
}
#endif /*ENABLE_MAP || ENABLE_WEAK_MAP*/

/*Add an entry to the set.*/
static RJS_SetEntry*
hash_add (RJS_Runtime *rt, RJS_Value *hashv, RJS_Value *k, size_t esize)
{
    RJS_HashObject *ho = (RJS_HashObject*)rjs_value_get_object(rt, hashv);
    RJS_HashEntry  *he, **phe;
    RJS_SetEntry   *se;
    RJS_Result      r;

    r = rjs_hash_lookup(&ho->hash, k, &he, &phe, &rjs_hash_value_ops_0, rt);
    if (!r) {
        RJS_HashIter *iter;

        se = rjs_alloc_assert_0(rt, esize);

        rjs_value_copy(rt, &se->key, k);

        rjs_hash_insert(&ho->hash, &se->key, &se->he, phe, &rjs_hash_value_ops_0, rt);
        rjs_list_append(&ho->list, &se->ln);

        rjs_list_foreach_c(&ho->iters, iter, RJS_HashIter, ln) {
            if (!iter->done && (iter->curr == &ho->list)) {
                iter->curr = &se->ln;
            }
        }
    } else {
        se = RJS_CONTAINER_OF(he, RJS_SetEntry, he);
    }

    return se;
}

/*Lookup the entry in the hash table.*/
static RJS_SetEntry*
hash_get (RJS_Runtime *rt, RJS_Value *v, RJS_Value *k)
{
    RJS_HashObject *ho = (RJS_HashObject*)rjs_value_get_object(rt, v);
    RJS_HashEntry  *he;
    RJS_Result      r;

    r = rjs_hash_lookup(&ho->hash, k, &he, NULL, &rjs_hash_value_ops_0, rt);

    return r ? RJS_CONTAINER_OF(he, RJS_SetEntry, he) : NULL;
}

/*Delete an entry from the hash table.*/
static RJS_SetEntry*
hash_delete (RJS_Runtime *rt, RJS_Value *v, RJS_Value *k)
{
    RJS_HashObject *ho = (RJS_HashObject*)rjs_value_get_object(rt, v);
    RJS_HashEntry  *he, **phe;
    RJS_HashIter   *iter;
    RJS_SetEntry   *se;
    RJS_Result      r;

    r = rjs_hash_lookup(&ho->hash, k, &he, &phe, &rjs_hash_value_ops_0, rt);
    if (!r)
        return NULL;

    se = RJS_CONTAINER_OF(he, RJS_SetEntry, he);

    rjs_hash_remove(&ho->hash, phe, rt);
    rjs_list_remove(&se->ln);

    rjs_list_foreach_c(&ho->iters, iter, RJS_HashIter, ln) {
        if (iter->curr == &se->ln) {
            iter->curr = se->ln.next;
        }
    }

    return se;
}

/*Clear the hash table.*/
static RJS_Result
hash_clear (RJS_Runtime *rt, RJS_Value *v, size_t esize)
{
    RJS_HashObject *ho = (RJS_HashObject*)rjs_value_get_object(rt, v);
    RJS_SetEntry   *se, *nse;
    RJS_HashIter   *hi;

    rjs_list_foreach_safe_c(&ho->list, se, nse, RJS_SetEntry, ln) {
        rjs_free(rt, se, esize);
    }

    rjs_hash_deinit(&ho->hash, &rjs_hash_value_ops_0, rt);

    rjs_list_foreach_c(&ho->iters, hi, RJS_HashIter, ln) {
        hi->curr = &ho->list;
    }

    rjs_list_init(&ho->list);
    rjs_hash_init(&ho->hash);

    return RJS_OK;
}

#if ENABLE_MAP || ENABLE_SET

/*Scan the reference in the hash iterator.*/
static void
hash_iter_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_HashIter *hi = ptr;

    rjs_object_op_gc_scan(rt, &hi->object);

    rjs_gc_scan_value(rt, &hi->hash);
}

/*Free the hash iterator.*/
static void
hash_iter_op_gc_free (RJS_Runtime *rt, void *ptr)
{
     RJS_HashIter *hi = ptr;

     rjs_object_deinit(rt, &hi->object);

     rjs_list_remove(&hi->ln);

     RJS_DEL(rt, hi);
}

/*Hash iterator operation functions.*/
static const RJS_ObjectOps
hash_iter_ops = {
    {
        RJS_GC_THING_HASH_ITERATOR,
        hash_iter_op_gc_scan,
        hash_iter_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Create a new hash iterator.*/
static RJS_Result
hash_iter_new (RJS_Runtime *rt, RJS_Value *iterv, RJS_Value *hashv,
        RJS_Value *proto, RJS_HashIterType type)
{
    RJS_HashObject *ho = (RJS_HashObject*)rjs_value_get_object(rt, hashv);
    RJS_HashIter   *hi;
    RJS_Result      r;

    RJS_NEW(rt, hi);

    hi->type = type;
    hi->curr = ho->list.next;
    hi->done = RJS_FALSE;

    rjs_value_copy(rt, &hi->hash, hashv);

    r = rjs_object_init(rt, iterv, &hi->object, proto, &hash_iter_ops);
    if (r == RJS_ERR) {
        RJS_DEL(rt, hi);
        return r;
    }

    rjs_list_append(&ho->iters, &hi->ln);
    return RJS_OK;
}

/*Hash iterator next method.*/
static RJS_NF(hash_iter_next)
{
    size_t          top = rjs_value_stack_save(rt);
    RJS_Value      *v   = rjs_value_stack_push(rt);
    RJS_HashObject *ho;
    RJS_HashIter   *hi;
    RJS_Result      r;
    RJS_Bool        done;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_HASH_ITERATOR) {
        r = rjs_throw_type_error(rt, _("the value is not a set/map iterator"));
        goto end;
    }

    hi = (RJS_HashIter*)rjs_value_get_object(rt, thiz);
    ho = (RJS_HashObject*)rjs_value_get_object(rt, &hi->hash);

    if (hi->curr == &ho->list) {
        rjs_value_set_undefined(rt, v);

        hi->done = RJS_TRUE;

        done = RJS_TRUE;
    } else {
        RJS_SetEntry *se;
        RJS_MapEntry *me;

        switch (hi->type) {
        case RJS_HASH_ITER_KEY:
            se = RJS_CONTAINER_OF(hi->curr, RJS_SetEntry, ln);
            rjs_value_copy(rt, v, &se->key);
            break;
        case RJS_HASH_ITER_VALUE:
#if ENABLE_MAP && ENABLE_SET
            if (rjs_value_get_gc_thing_type(rt, &hi->hash) == RJS_GC_THING_SET)
#endif /*ENABLE_MAP && ENABLE_SET*/
#if ENABLE_SET
            {
                se = RJS_CONTAINER_OF(hi->curr, RJS_SetEntry, ln);
                rjs_value_copy(rt, v, &se->key);
            }
#endif /*ENABLE_SET*/
#if ENABLE_MAP && ENABLE_SET
            else
#endif /*ENABLE_MAP && ENABLE_SET*/
#if ENABLE_MAP
            {
                me = RJS_CONTAINER_OF(hi->curr, RJS_MapEntry, se.ln);
                rjs_value_copy(rt, v, &me->value);
            }
#endif /*ENABLE_MAP*/
            break;
        case RJS_HASH_ITER_KEY_VALUE:
            rjs_array_new(rt, v, 2, NULL);

#if ENABLE_MAP && ENABLE_SET
            if (rjs_value_get_gc_thing_type(rt, &hi->hash) == RJS_GC_THING_SET)
#endif /*ENABLE_MAP && ENABLE_SET*/
#if ENABLE_SET
            {
                se = RJS_CONTAINER_OF(hi->curr, RJS_SetEntry, ln);
                rjs_set_index(rt, v, 0, &se->key, RJS_TRUE);
                rjs_set_index(rt, v, 1, &se->key, RJS_TRUE);
            }
#endif /*ENABLE_SET*/
#if ENABLE_MAP && ENABLE_SET
            else
#endif /*ENABLE_MAP && ENABLE_SET*/
#if ENABLE_MAP
            {
                me = RJS_CONTAINER_OF(hi->curr, RJS_MapEntry, se.ln);
                rjs_set_index(rt, v, 0, &me->se.key, RJS_TRUE);
                rjs_set_index(rt, v, 1, &me->value, RJS_TRUE);
            }
#endif /*ENABLE_MAP*/
            break;
        }

        hi->curr = hi->curr->next;

        done = RJS_FALSE;
    }

    r = rjs_create_iter_result_object(rt, v, done, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_MAP || ENABLE_SET*/