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

/**String property entry.*/
typedef struct {
    RJS_HashEntry he;    /**< Hash table entry.*/
    RJS_Value     value; /**< Value.*/
} RJS_StringPropEntry;

/*Calculate the string property's hash code.*/
static size_t
hash_op_string_prop_key (void *data, void *key)
{
    RJS_Runtime *rt = data;
    RJS_Value *v     = key;

    if (rjs_value_is_index_string(rt, v))
        return rjs_value_get_index_string(rt, v);

    return rjs_string_hash_key(rt, v);
}

/*Check if 2 string property keys are equal.*/
static RJS_Bool
hash_op_string_prop_equal (void *data, void *k1, void *k2)
{
    RJS_Runtime *rt = data;
    RJS_Value *v1    = k1;
    RJS_Value *v2    = k2;

    return rjs_string_equal(rt, v1, v2);
}

/**String property key type hash table operation functions.*/
static const RJS_HashOps
hash_string_prop_ops = {
    rjs_hash_op_realloc,
    hash_op_string_prop_key,
    hash_op_string_prop_equal
};

/*Scan referenced things in the for in iterator.*/
static void
for_in_iterator_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ForInIterator   *fii = ptr;
    size_t               i;
    RJS_StringPropEntry *spe;

    rjs_object_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &fii->v);
    rjs_gc_scan_value(rt, &fii->keys);

    rjs_hash_foreach_c(&fii->key_hash, i, spe, RJS_StringPropEntry, he) {
        rjs_gc_scan_value(rt, &spe->value);
    }
}

/*Free the for in iterator.*/
static void
for_in_iterator_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ForInIterator   *fii = ptr;
    RJS_StringPropEntry *spe, *nspe;
    size_t               i;

    rjs_hash_foreach_safe_c(&fii->key_hash, i, spe, nspe, RJS_StringPropEntry, he) {
        RJS_DEL(rt, spe);
    }

    rjs_hash_deinit(&fii->key_hash, &hash_string_prop_ops, rt);

    rjs_object_deinit(rt, &fii->object);

    RJS_DEL(rt, fii);
}

/*For in iterator operation functions.*/
static const RJS_ObjectOps
for_in_iterator_ops = {
    {
        RJS_GC_THING_OBJECT,
        for_in_iterator_op_gc_scan,
        for_in_iterator_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/**
 * Create a new for in iterator.
 * \param rt The current runtime.
 * \param[out] iterv Return the iterator value.
 * \param v The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_for_in_iterator_new (RJS_Runtime *rt, RJS_Value *iterv, RJS_Value *v)
{
    RJS_Realm         *realm = rjs_realm_current(rt);
    RJS_ForInIterator *fii;
    RJS_Result         r;

    RJS_NEW(rt, fii);

    if (rjs_value_is_null(rt, v) || rjs_value_is_undefined(rt, v)) {
        fii->visited = RJS_TRUE;
        rjs_value_set_null(rt, &fii->v);
    } else {
        fii->visited = RJS_FALSE;

        rjs_to_object(rt, v, &fii->v);
    }

    rjs_value_set_undefined(rt, &fii->keys);
    rjs_hash_init(&fii->key_hash);

    r = rjs_object_init(rt, iterv, &fii->object, rjs_o_ForInIteratorPrototype(realm), &for_in_iterator_ops);

    return r;
}

/*%ForInIteratorPrototype.next()%*/
static RJS_Result
for_in_iterator_proto_next (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_ForInIterator *fii;
    RJS_Result         r;
    RJS_PropertyDesc   pd;
    size_t             top   = rjs_value_stack_save(rt);
    RJS_Bool           found = RJS_FALSE;

    fii = (RJS_ForInIterator*)rjs_value_get_object(rt, thiz);

    rjs_property_desc_init(rt, &pd);

    while (!rjs_value_is_null(rt, &fii->v) && !found) {
        if (!fii->visited) {
            if ((r = rjs_object_own_property_keys(rt, &fii->v, &fii->keys)) == RJS_ERR)
                goto end;
            
            fii->visited = RJS_TRUE;
            fii->id      = 0;
        }

        if (!rjs_value_is_undefined(rt, &fii->keys)) {
            RJS_PropertyKeyList *pkl;

            pkl = rjs_value_get_gc_thing(rt, &fii->keys);

            while (fii->id < pkl->keys.item_num) {
                RJS_Value *v = &pkl->keys.items[fii->id];

                if (rjs_value_is_string(rt, v)) {
                    RJS_HashEntry *he, **phe;

                    r = rjs_hash_lookup(&fii->key_hash, v, &he, &phe, &hash_string_prop_ops, rt);
                    if (!r) {
                        RJS_PropertyName pn;

                        rjs_property_name_init(rt, &pn, v);
                        r = rjs_object_get_own_property(rt, &fii->v, &pn, &pd);
                        rjs_property_name_deinit(rt, &pn);

                        if (r == RJS_ERR)
                            goto end;

                        if (r) {
                            RJS_StringPropEntry *spe;

                            RJS_NEW(rt, spe);

                            rjs_value_copy(rt, &spe->value, v);
                            rjs_hash_insert(&fii->key_hash, &spe->value, &spe->he, phe, &hash_string_prop_ops, rt);

                            if (pd.flags & RJS_PROP_FL_ENUMERABLE) {
                                rjs_create_iter_result_object(rt, v, RJS_FALSE, rv);
                                found = RJS_TRUE;
                                break;
                            }
                        }
                    }
                } else {
                    break;
                }

                fii->id ++;
            }
        }

        if (!found) {
            if ((r = rjs_object_get_prototype_of(rt, &fii->v, &fii->v)) == RJS_ERR)
                goto end;

            fii->visited = RJS_FALSE;
        }
    }

    if (!found)
        rjs_create_iter_result_object(rt, rjs_v_undefined(rt), RJS_TRUE, rv);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*ForInIteratorPrototype functions.*/
static const RJS_BuiltinFuncDesc
for_in_iterator_func_descs[] = {
    {
        "next",
        0,
        for_in_iterator_proto_next
    },
    {NULL}
};

/*ForInIteratorPrototype*/
static const RJS_BuiltinObjectDesc
for_in_iterator_prototype_desc = {
    "%ForInIteratorPrototype%",
    "IteratorPrototype",
    NULL,
    NULL,
    NULL,
    for_in_iterator_func_descs,
    NULL,
    NULL
};

/**
 * Initialize the for in iterator data in the realm.
 * \param rt The current runtime.
 * \param realm The realm to be initialized.
 */
void
rjs_realm_for_in_iterator_init (RJS_Runtime *rt, RJS_Realm *realm)
{
    rjs_load_builtin_object_desc(rt, realm, &for_in_iterator_prototype_desc,
            rjs_o_ForInIteratorPrototype(realm));
}
