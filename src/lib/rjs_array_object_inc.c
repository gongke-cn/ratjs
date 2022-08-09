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

/*Array.*/
static RJS_NF(Array_constructor)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *proto = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!nt)
        nt = f;

    if ((r = rjs_get_prototype_from_constructor(rt, nt, RJS_O_Array_prototype, proto)) == RJS_ERR)
        goto end;

    if (argc == 0) {
        if ((r = rjs_array_new(rt, rv, 0, proto)) == RJS_ERR)
            goto end;
    } else if (argc == 1) {
        RJS_Value *lenv = rjs_argument_get(rt, args, argc, 0);

        if ((r = rjs_array_new(rt, rv, 0, proto)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_number(rt, lenv)) {
            rjs_create_data_property_or_throw_index(rt, rv, 0, lenv);
            rjs_value_set_number(rt, lenv, 1);
        } else {
            uint32_t ilen;

            if ((r = rjs_to_uint32(rt, lenv, &ilen)) == RJS_ERR)
                goto end;

            if (rjs_value_get_number(rt, lenv) != ilen) {
                r = rjs_throw_range_error(rt, _("invalid array length"));
                goto end;
            }
        }

        rjs_set(rt, rv, rjs_pn_length(rt), lenv, RJS_TRUE);
    } else {
        size_t i;

        if ((r = rjs_array_new(rt, rv, argc, proto)) == RJS_ERR)
            goto end;

        for (i = 0; i < argc; i ++) {
            RJS_Value *arg = rjs_value_buffer_item(rt, args, i);

            rjs_create_data_property_or_throw_index(rt, rv, i, arg);
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
array_constructor_desc = {
    "Array",
    1,
    Array_constructor
};

/*Array.from*/
static RJS_NF(Array_from)
{
    RJS_Value   *items    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value   *map_fn   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value   *this_arg = rjs_argument_get(rt, args, argc, 2);
    size_t       top      = rjs_value_stack_save(rt);
    RJS_Value   *iter     = rjs_value_stack_push(rt);
    RJS_Value   *array    = rjs_value_stack_push(rt);
    RJS_Value   *lenv     = rjs_value_stack_push(rt);
    RJS_Value   *kv       = rjs_value_stack_push(rt);
    RJS_Value   *iv       = rjs_value_stack_push(rt);
    RJS_Value   *mappedv  = rjs_value_stack_push(rt);
    RJS_Value   *next     = rjs_value_stack_push(rt);
    RJS_Bool     use_iter = RJS_FALSE;
    RJS_Bool     mapping;
    RJS_Iterator iter_rec;
    size_t       k;
    RJS_Result   r;

    rjs_iterator_init(rt, &iter_rec);

    if (rjs_value_is_undefined(rt, map_fn)) {
        mapping = RJS_FALSE;
    } else if (!rjs_is_callable(rt, map_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    } else {
        mapping = RJS_TRUE;
    }

    if ((r = rjs_get_method(rt, items, rjs_pn_s_iterator(rt), iter)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, iter)) {
        if (rjs_is_constructor(rt, thiz)) {
            r = rjs_construct(rt, thiz, NULL, 0, NULL, rv);
        } else {
            r = rjs_array_new(rt, rv, 0, NULL);
        }

        if (r == RJS_ERR)
            goto end;

        if ((r = rjs_get_iterator(rt, items, RJS_ITERATOR_SYNC, iter, &iter_rec)) == RJS_ERR)
            goto end;
        use_iter = RJS_TRUE;

        k = 0;
        while (1) {
            if (k > RJS_MAX_INT) {
                r = rjs_throw_type_error(rt, _("illegal array length"));
                goto end;
            }

            if ((r = rjs_iterator_step(rt, &iter_rec, next)) == RJS_ERR)
                goto end;

            if (!r) {
                rjs_value_set_number(rt, lenv, k);

                if ((r = rjs_set(rt, rv, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
                    goto end;
                break;
            }

            if ((r = rjs_iterator_value(rt, next, kv)) == RJS_ERR)
                goto end;

            if (mapping) {
                rjs_value_set_number(rt, iv, k);

                if ((r = rjs_call(rt, map_fn, this_arg, kv, 2, mappedv)) == RJS_ERR)
                    goto end;
            } else {
                rjs_value_copy(rt, mappedv, kv);
            }

            if ((r = rjs_create_data_property_or_throw_index(rt, rv, k, mappedv)) == RJS_ERR)
                goto end;

            k ++;
        }
    } else {
        int64_t len;

        rjs_to_object(rt, items, array);

        if ((r = rjs_length_of_array_like(rt, array, &len)) == RJS_ERR)
            goto end;

        if (rjs_is_constructor(rt, thiz)) {
            rjs_value_set_number(rt, lenv, len);

            r = rjs_construct(rt, thiz, lenv, 1, NULL, rv);
        } else {
            r = rjs_array_new(rt, rv, len, NULL);
        }

        if (r == RJS_ERR)
            goto end;

        for (k = 0; k < len; k ++) {
            if ((r = rjs_get_index(rt, array, k, kv)) == RJS_ERR)
                goto end;

            if (mapping) {
                rjs_value_set_number(rt, iv, k);

                if ((r = rjs_call(rt, map_fn, this_arg, kv, 2, mappedv)) == RJS_ERR)
                    goto end;
            } else {
                rjs_value_copy(rt, mappedv, kv);
            }

            if ((r = rjs_create_data_property_or_throw_index(rt, rv, k, mappedv)) == RJS_ERR)
                goto end;
        }

        rjs_value_set_number(rt, lenv, len);
        if ((r = rjs_set(rt, rv, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    if (use_iter && (r == RJS_ERR))
        rjs_iterator_close(rt, &iter_rec);

    rjs_iterator_deinit(rt, &iter_rec);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.isArray*/
static RJS_NF(Array_isArray)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if ((r = rjs_is_array(rt, arg)) == RJS_ERR)
        return r;

    rjs_value_set_boolean(rt, rv, r);
    return RJS_OK;
}

/*Array.of*/
static RJS_NF(Array_of)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *lenv = rjs_value_stack_push(rt);
    size_t     k;
    RJS_Result r;

    rjs_value_set_number(rt, lenv, argc);

    if (rjs_is_constructor(rt, thiz)) {
        r = rjs_construct(rt, thiz, lenv, 1, NULL, rv);
    } else {
        r = rjs_array_new(rt, rv, argc, NULL);
    }

    if (r == RJS_ERR)
        goto end;

    for (k = 0; k < argc; k ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, k);

        if ((r = rjs_create_data_property_or_throw_index(rt, rv, k, arg)) == RJS_ERR)
            goto end;
    }

    if ((r = rjs_set(rt, rv, rjs_pn_length(rt), lenv, RJS_TRUE)))
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
array_function_descs[] = {
    {
        "from",
        1,
        Array_from
    },
    {
        "isArray",
        1,
        Array_isArray
    },
    {
        "of",
        0,
        Array_of
    },
    {NULL}
};

static const RJS_BuiltinAccessorDesc
array_accessor_descs[] = {
    {
        "@@species",
        rjs_return_this
    },
    {NULL}
};

/*Array.prototype.at*/
static RJS_NF(Array_prototype_at)
{
    RJS_Value *idx = rjs_argument_get(rt, args, argc, 0);
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *o   = rjs_value_stack_push(rt);
    int64_t    len;
    RJS_Number n, k, l;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;
    l = len;

    if ((r = rjs_to_integer_or_infinity(rt, idx, &n)) == RJS_ERR)
        goto end;

    if (n >= 0)
        k = n;
    else
        k = l + n;

    if ((k < 0) || (k >= len)) {
        rjs_value_set_undefined(rt, rv);
    } else {
        if ((r = rjs_get_index(rt, o, k, rv)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create the species array.*/
static RJS_Result
array_species_create (RJS_Runtime *rt, RJS_Value *orig, int64_t len, RJS_Value *arr)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *c    = rjs_value_stack_push(rt);
    RJS_Value *sc   = rjs_value_stack_push(rt);
    RJS_Value *lenv = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!rjs_is_array(rt, orig)) {
        r = rjs_array_new(rt, arr, len, NULL);
    } else {
        if ((r = rjs_get(rt, orig, rjs_pn_constructor(rt), c)) == RJS_ERR)
            goto end;

        if (rjs_is_constructor(rt, c)) {
            RJS_Realm *this_realm = rjs_realm_current(rt);
            RJS_Realm *realm      = rjs_get_function_realm(rt, c);

            if (!realm) {
                r = RJS_ERR;
                goto end;
            }

            if (this_realm != realm) {
                if (rjs_same_value(rt, c, rjs_o_Array(realm))) {
                    rjs_value_set_undefined(rt, c);
                }
            }
        }

        if (rjs_value_is_object(rt, c)) {
            if ((r = rjs_get(rt, c, rjs_pn_s_species(rt), sc)) == RJS_ERR)
                goto end;

            if (rjs_value_is_null(rt, sc))
                rjs_value_set_undefined(rt, c);
            else
                rjs_value_copy(rt, c, sc);
        }

        if (rjs_value_is_undefined(rt, c)) {
            r = rjs_array_new(rt, arr, len, NULL);
        } else if (!rjs_is_constructor(rt, c)) {
            r = rjs_throw_type_error(rt, _("the value is not a constructor"));
        } else {
            rjs_value_set_number(rt, lenv, len);

            r = rjs_construct(rt, c, lenv, 1, NULL, arr);
        }
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Check if the object is spreadable.*/
static RJS_Result
is_concat_spreadable (RJS_Runtime *rt, RJS_Value *o)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *flag = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!rjs_value_is_object(rt, o)) {
        r = RJS_FALSE;
        goto end;
    }

    if ((r = rjs_get(rt, o, rjs_pn_s_isConcatSpreadable(rt), flag)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, flag))
        r = rjs_to_boolean(rt, flag);
    else
        r = rjs_is_array(rt, o);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.concat*/
static RJS_NF(Array_prototype_concat)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *o    = rjs_value_stack_push(rt);
    RJS_Value *lenv = rjs_value_stack_push(rt);
    RJS_Value *idx  = rjs_value_stack_push(rt);
    RJS_Value *key  = rjs_value_stack_push(rt);
    RJS_Value *sub  = rjs_value_stack_push(rt);
    int64_t    n    = 0;
    size_t     aid  = 0;
    RJS_Value *e;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = array_species_create(rt, o, 0, rv)) == RJS_ERR)
        goto end;

    e = o;
    while (1) {
        if ((r = is_concat_spreadable(rt, e)) == RJS_ERR)
            goto end;

        if (r) {
            int64_t k, len;

            if ((r = rjs_length_of_array_like(rt, e, &len)) == RJS_ERR)
                goto end;

            if ((n + len) > RJS_MAX_INT) {
                r = rjs_throw_type_error(rt, _("illegal array length"));
                goto end;
            }

            for (k = 0; k < len; n ++, k ++) {
                rjs_value_set_number(rt, idx, k);
                rjs_to_string(rt, idx, key);

                if ((r = rjs_has_property(rt, e, key)) == RJS_ERR)
                    goto end;

                if (r) {
                    RJS_PropertyName pn;

                    rjs_property_name_init(rt, &pn, key);
                    r = rjs_get(rt, e, &pn, sub);
                    rjs_property_name_deinit(rt, &pn);
                    if (r == RJS_ERR)
                        goto end;

                    if ((r = rjs_create_data_property_or_throw_index(rt, rv, n, sub)) == RJS_ERR)
                        goto end;
                }
            }
        } else {
            if (n >= RJS_MAX_INT) {
                r = rjs_throw_type_error(rt, _("illegal array length"));
                goto end;
            }

            if ((r = rjs_create_data_property_or_throw_index(rt, rv, n, e)) == RJS_ERR)
                goto end;

            n ++;
        }

        if (aid >= argc)
            break;

        e = rjs_value_buffer_item(rt, args, aid);
        aid ++;
    }

    rjs_value_set_number(rt, lenv, n);

    if ((r = rjs_set(rt, rv, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.copyWithin*/
static RJS_NF(Array_prototype_copyWithin)
{
    RJS_Value *target = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *start  = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *end    = rjs_argument_get(rt, args, argc, 2);
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *o      = rjs_value_stack_push(rt);
    RJS_Value *nv     = rjs_value_stack_push(rt);
    RJS_Value *fromk  = rjs_value_stack_push(rt);
    RJS_Value *tok    = rjs_value_stack_push(rt);
    RJS_Value *iv     = rjs_value_stack_push(rt);
    int64_t    len, from, to, final, count;
    RJS_Number rel_target, rel_start, rel_end;
    int        dir;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, target, &rel_target)) == RJS_ERR)
        goto end;
    if (rel_target == -INFINITY)
        to = 0;
    else if (rel_target < 0)
        to = RJS_MAX(0, len + rel_target);
    else
        to = RJS_MIN(rel_target, len);

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;
    if (rel_start == -INFINITY)
        from = 0;
    else if (rel_start < 0)
        from = RJS_MAX(0, len + rel_start);
    else
        from = RJS_MIN(rel_start, len);

    if (rjs_value_is_undefined(rt, end)) {
        rel_end = len;
    } else {
        if ((r = rjs_to_integer_or_infinity(rt, end, &rel_end)) == RJS_ERR)
            goto end;
    }
    if (rel_end == -INFINITY)
        final = 0;
    else if (rel_end < 0)
        final = RJS_MAX(0, len + rel_end);
    else
        final = RJS_MIN(rel_end, len);

    count = RJS_MIN(final - from, len - to);

    if ((from < to) && (to < from + count)) {
        from = from + count - 1;
        to   = to + count - 1;
        dir  = -1;
    } else {
        dir  = 1;
    }

    while (count > 0) {
        RJS_PropertyName from_pn, to_pn;

        rjs_value_set_number(rt, nv, from);
        rjs_to_string(rt, nv, fromk);

        rjs_value_set_number(rt, nv, to);
        rjs_to_string(rt, nv, tok);

        if ((r = rjs_has_property(rt, o, fromk)) == RJS_ERR)
            goto end;

        if (r) {
            rjs_property_name_init(rt, &from_pn, fromk);
            rjs_property_name_init(rt, &to_pn, tok);

            if ((r = rjs_get(rt, o, &from_pn, iv)) == RJS_OK)
                r = rjs_set(rt, o, &to_pn, iv, RJS_TRUE);

            rjs_property_name_deinit(rt, &from_pn);
            rjs_property_name_deinit(rt, &to_pn);
        } else {
            rjs_property_name_init(rt, &to_pn, tok);

            r = rjs_delete_property_or_throw(rt, o, &to_pn);

            rjs_property_name_deinit(rt, &to_pn);
        }

        if (r == RJS_ERR)
            goto end;

        from += dir;
        to   += dir;
        count --;
    }

    rjs_value_copy(rt, rv, o);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**Array iterator type.*/
typedef enum {
    RJS_ARRAY_ITERATOR_KEY,      /**< Key only.*/
    RJS_ARRAY_ITERATOR_VALUE,    /**< Value only.*/
    RJS_ARRAY_ITERATOR_KEY_VALUE /**< Key and value.*/
} RJS_ArrayIteratorType;

/**Array iterator object.*/
typedef struct {
    RJS_Object            object; /**< Base object data.*/
    RJS_Value             array;  /**< The array.*/
    RJS_ArrayIteratorType type;   /**< Iterator flags.*/
    int64_t               curr;   /**< The current index.*/
} RJS_ArrayIterator;

/*Scan the referenced things in the array iterator.*/
static void
array_iter_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ArrayIterator *ai = ptr;

    rjs_object_op_gc_scan(rt, &ai->object);
    rjs_gc_scan_value(rt, &ai->array);
}

/*Free the array iterator.*/
static void
array_iter_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ArrayIterator *ai = ptr;

    rjs_object_deinit(rt, &ai->object);

    RJS_DEL(rt, ai);
}

/**Array iterator operation functions.*/
static const RJS_ObjectOps
array_iter_ops = {
    {
        RJS_GC_THING_ARRAY_ITERATOR,
        array_iter_op_gc_scan,
        array_iter_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Create the array iterator.*/
static RJS_Result
create_array_iterator (RJS_Runtime *rt, RJS_Value *a, RJS_ArrayIteratorType type, RJS_Value *iter)
{
    RJS_Realm         *realm = rjs_realm_current(rt);
    RJS_ArrayIterator *ai;
    RJS_Result         r;

    RJS_NEW(rt, ai);

    rjs_value_copy(rt, &ai->array, a);

    ai->type = type;
    ai->curr = 0;

    r = rjs_object_init(rt, iter, &ai->object, rjs_o_ArrayIteratorPrototype(realm), &array_iter_ops);
    if (r == RJS_ERR)
        RJS_DEL(rt, ai);

    return r;
}

/*Array.prototype.entries*/
static RJS_NF(Array_prototype_entries)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *o   = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    r = create_array_iterator(rt, o, RJS_ARRAY_ITERATOR_KEY_VALUE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.every*/
static RJS_NF(Array_prototype_every)
{
    RJS_Value *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *kv       = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    RJS_Value *tr       = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    for (k = 0; k < len; k ++) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, kv);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_call(rt, cb_fn, this_arg, kv, 3, tr)) == RJS_ERR)
                goto end;

            if (!rjs_to_boolean(rt, tr)) {
                rjs_value_set_boolean(rt, rv, RJS_FALSE);
                goto end;
            }
        }
    }

    rjs_value_set_boolean(rt, rv, RJS_TRUE);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.fill*/
static RJS_NF(Array_prototype_fill)
{
    RJS_Value *value = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *start = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *end   = rjs_argument_get(rt, args, argc, 2);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *o     = rjs_value_stack_push(rt);
    int64_t    len, k, final;
    RJS_Number rel_start, rel_end;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;

    if (rel_start == -INFINITY)
        k = 0;
    else if (rel_start < 0)
        k = RJS_MAX(len + rel_start, 0);
    else
        k = RJS_MIN(rel_start, len);

    if (rjs_value_is_undefined(rt, end))
        rel_end = len;
    else if ((r = rjs_to_integer_or_infinity(rt, end, &rel_end)) == RJS_ERR)
        goto end;

    if (rel_end == - INFINITY)
        final = 0;
    else if (rel_end < 0)
        final = RJS_MAX(len + rel_end, 0);
    else
        final = RJS_MIN(rel_end, len);

    while (k < final) {
        if ((r = rjs_set_index(rt, o, k, value, RJS_TRUE)) == RJS_ERR)
            goto end;

        k ++;
    }

    rjs_value_copy(rt, rv, o);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.filter*/
static RJS_NF(Array_prototype_filter)
{
    RJS_Value *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *a        = rjs_value_stack_push(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    RJS_Value *item     = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *sel      = rjs_value_stack_push(rt);
    int64_t    len, k, to;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = array_species_create(rt, o, 0, a)) == RJS_ERR)
        goto end;

    to = 0;

    for (k = 0; k < len; k ++) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, item);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_call(rt, cb_fn, this_arg, item, 3, sel)) == RJS_ERR)
                goto end;

            if (rjs_to_boolean(rt, sel)) {
                if ((r = rjs_create_data_property_or_throw_index(rt, a, to, item)) == RJS_ERR)
                    goto end;

                to ++;
            }
        }
    }

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.find*/
static RJS_NF(Array_prototype_find)
{
    RJS_Value *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *item     = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *test     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    for (k = 0; k < len; k ++) {
        if ((r = rjs_get_index(rt, o, k, item)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, pred, this_arg, item, 3, test)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, test)) {
            rjs_value_copy(rt, rv, item);
            r = RJS_OK;
            goto end;
        }
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.findIndex*/
static RJS_NF(Array_prototype_findIndex)
{
    RJS_Value *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *item     = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *test     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    for (k = 0; k < len; k ++) {
        if ((r = rjs_get_index(rt, o, k, item)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, pred, this_arg, item, 3, test)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, test)) {
            rjs_value_set_number(rt, rv, k);
            r = RJS_OK;
            goto end;
        }
    }

    rjs_value_set_number(rt, rv, -1);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.findLast*/
static RJS_NF(Array_prototype_findLast)
{
    RJS_Value *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *item     = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *test     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    for (k = len; k > 0; k --) {
        if ((r = rjs_get_index(rt, o, k - 1, item)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, key, k - 1);

        if ((r = rjs_call(rt, pred, this_arg, item, 3, test)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, test)) {
            rjs_value_copy(rt, rv, item);
            r = RJS_OK;
            goto end;
        }
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.findLastIndex*/
static RJS_NF(Array_prototype_findLastIndex)
{
    RJS_Value *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *item     = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *test     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    for (k = len; k > 0; k --) {
        if ((r = rjs_get_index(rt, o, k - 1, item)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, key, k - 1);

        if ((r = rjs_call(rt, pred, this_arg, item, 3, test)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, test)) {
            rjs_value_set_number(rt, rv, k - 1);
            r = RJS_OK;
            goto end;
        }
    }

    rjs_value_set_number(rt, rv, -1);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Flatten the array items.*/
static RJS_Result
flatten_into_array (RJS_Runtime *rt, RJS_Value *target, RJS_Value *source, int64_t source_len,
        int64_t *pstart, RJS_Number depth, RJS_Value *map_fn, RJS_Value *this_arg)
{
    int64_t    target_index = *pstart;
    size_t     top          = rjs_value_stack_save(rt);
    RJS_Value *p            = rjs_value_stack_push(rt);
    RJS_Value *elem         = rjs_value_stack_push(rt);
    RJS_Value *key          = rjs_value_stack_push(rt);
    RJS_Value *src          = rjs_value_stack_push(rt);
    RJS_Value *telem        = rjs_value_stack_push(rt);
    int64_t    source_index;
    RJS_Result r;

    rjs_value_copy(rt, src, source);

    for (source_index = 0; source_index < source_len; source_index ++) {
        RJS_Bool flatten;

        rjs_value_set_number(rt, key, source_index);
        rjs_to_string(rt, key, p);

        if ((r = rjs_has_property(rt, source, p)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, p);
            r = rjs_get(rt, source, &pn, elem);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if (map_fn) {
                if ((r = rjs_call(rt, map_fn, this_arg, elem, 3, telem)) == RJS_ERR)
                    goto end;
            } else {
                rjs_value_copy(rt, telem, elem);
            }

            flatten = RJS_FALSE;

            if (depth > 0) {
                if ((r = rjs_is_array(rt, telem)) == RJS_ERR)
                    goto end;

                flatten = r;
            }

            if (flatten) {
                RJS_Number ndepth;
                int64_t    elen;

                if (depth == INFINITY)
                    ndepth = INFINITY;
                else
                    ndepth = depth - 1;

                if ((r = rjs_length_of_array_like(rt, telem, &elen)) == RJS_ERR)
                    goto end;

                if ((r = flatten_into_array(rt, target, telem, elen, &target_index,
                        ndepth, NULL, NULL)) == RJS_ERR)
                    goto end;
            } else {
                if (target_index >= RJS_MAX_INT) {
                    r = rjs_throw_type_error(rt, _("illegal array length"));
                    goto end;
                }

                if ((r = rjs_create_data_property_or_throw_index(rt, target, target_index, telem)) == RJS_ERR)
                    goto end;

                target_index ++;
            }
        }
    }

    *pstart = target_index;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.flat*/
static RJS_NF(Array_prototype_flat)
{
    RJS_Value *depth = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *o     = rjs_value_stack_push(rt);
    RJS_Value *a     = rjs_value_stack_push(rt);
    RJS_Number depth_num;
    int64_t    len, start;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    depth_num = 1;

    if (!rjs_value_is_undefined(rt, depth)) {
        if ((r = rjs_to_integer_or_infinity(rt, depth, &depth_num)) == RJS_ERR)
            goto end;

        if (depth_num < 0)
            depth_num = 0;
    }

    if ((r = array_species_create(rt, o, 0, a)) == RJS_ERR)
        goto end;

    start = 0;
    if ((r = flatten_into_array(rt, a, o, len, &start, depth_num, NULL, NULL)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.flatMap*/
static RJS_NF(Array_prototype_flatMap)
{
    RJS_Value *map_fn   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *a        = rjs_value_stack_push(rt);
    int64_t    len, start;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, map_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = array_species_create(rt, o, 0, a)) == RJS_ERR)
        goto end;

    start = 0;
    if ((r = flatten_into_array(rt, a, o, len, &start, 1, map_fn, this_arg)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.forEach*/
static RJS_NF(Array_prototype_forEach)
{
    RJS_Value *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *kv       = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    for (k = 0; k < len; k ++) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, kv);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_call(rt, cb_fn, this_arg, kv, 3, NULL)) == RJS_ERR)
                goto end;
        }
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.includes*/
static RJS_NF(Array_prototype_includes)
{
    RJS_Value *searche  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *from_idx = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *elem     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (len == 0) {
        rjs_value_set_boolean(rt, rv, RJS_FALSE);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_to_integer_or_infinity(rt, from_idx, &n)) == RJS_ERR)
        goto end;

    if (n == INFINITY) {
        rjs_value_set_boolean(rt, rv, RJS_FALSE);
        r = RJS_OK;
        goto end;
    } else if (n == -INFINITY) {
        n = 0;
    }

    if (n >= 0) {
        k = n;
    } else {
        k = len + n;

        if (k < 0)
            k = 0;
    }

    while (k < len) {
        if ((r = rjs_get_index(rt, o, k, elem)) == RJS_ERR)
            goto end;

        if (rjs_same_value_0(rt, elem, searche)) {
            rjs_value_set_boolean(rt, rv, RJS_TRUE);
            r = RJS_OK;
            goto end;
        }

        k ++;
    }

    rjs_value_set_boolean(rt, rv, RJS_FALSE);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.indexOf*/
static RJS_NF(Array_prototype_indexOf)
{
    RJS_Value *searche  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *from_idx = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    RJS_Value *elem     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (len == 0) {
        rjs_value_set_number(rt, rv, -1);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_to_integer_or_infinity(rt, from_idx, &n)) == RJS_ERR)
        goto end;

    if (n == INFINITY) {
        rjs_value_set_number(rt, rv, -1);
        r = RJS_OK;
        goto end;
    } else if (n == -INFINITY) {
        n = 0;
    }

    if (n >= 0) {
        k = n;
    } else {
        k = len + n;

        if (k < 0)
            k = 0;
    }

    while (k < len) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, elem);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if (rjs_is_strictly_equal(rt, elem, searche)) {
                rjs_value_set_number(rt, rv, k);
                r = RJS_OK;
                goto end;
            }
        }

        k ++;
    }

    rjs_value_set_number(rt, rv, -1);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.join*/
static RJS_NF(Array_prototype_join)
{
    RJS_Value *sep  = rjs_argument_get(rt, args, argc, 0);
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *o    = rjs_value_stack_push(rt);
    RJS_Value *ss   = rjs_value_stack_push(rt);
    RJS_Value *elem = rjs_value_stack_push(rt);
    RJS_Value *es   = rjs_value_stack_push(rt);
    RJS_UCharBuffer ucb;
    int64_t    len, k;
    RJS_Result r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, sep)) {
        rjs_value_copy(rt, ss, rjs_s_comma(rt));
    } else {
        if ((r = rjs_to_string(rt, sep, ss)) == RJS_ERR)
            goto end;
    }

    for (k = 0; k < len; k ++) {
        if (k > 0)
            rjs_uchar_buffer_append_string(rt, &ucb, ss);

        if ((r = rjs_get_index(rt, o, k, elem)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, elem) && !rjs_value_is_null(rt, elem)) {
            if ((r = rjs_to_string(rt, elem, es)) == RJS_ERR)
                goto end;

            rjs_uchar_buffer_append_string(rt, &ucb, es);
        }
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.keys*/
static RJS_NF(Array_prototype_keys)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *o   = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    r = create_array_iterator(rt, o, RJS_ARRAY_ITERATOR_KEY, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.lastIndexOf*/
static RJS_NF(Array_prototype_lastIndexOf)
{
    RJS_Value *searche  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *from_idx = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    RJS_Value *elem     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (len == 0) {
        rjs_value_set_number(rt, rv, -1);
        r = RJS_OK;
        goto end;
    }

    if (argc > 1) {
        if ((r = rjs_to_integer_or_infinity(rt, from_idx, &n)) == RJS_ERR)
            goto end;
    } else {
        n = len - 1;
    }

    if (n == -INFINITY) {
        rjs_value_set_number(rt, rv, -1);
        r = RJS_OK;
        goto end;
    }

    if (n >= 0) {
        k = RJS_MIN(n, len - 1);
    } else {
        k = len + n;
    }

    while (k >= 0) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, elem);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if (rjs_is_strictly_equal(rt, elem, searche)) {
                rjs_value_set_number(rt, rv, k);
                r = RJS_OK;
                goto end;
            }
        }

        k --;
    }

    rjs_value_set_number(rt, rv, -1);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.map*/
static RJS_NF(Array_prototype_map)
{
    RJS_Value *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *a        = rjs_value_stack_push(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    RJS_Value *item     = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *mapv     = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = array_species_create(rt, o, len, a)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, item);
            if (r == RJS_OK) {
                r = rjs_call(rt, cb_fn, this_arg, item, 3, mapv);
                if (r == RJS_OK)
                    r = rjs_create_data_property_or_throw(rt, a, &pn, mapv);
            }
            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;
        }
    }

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.pop*/
static RJS_NF(Array_prototype_pop)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *o    = rjs_value_stack_push(rt);
    RJS_Value *lenv = rjs_value_stack_push(rt);
    RJS_Value *idx  = rjs_value_stack_push(rt);
    int64_t    len;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (len == 0) {
        rjs_value_set_number(rt, lenv, 0);

        if ((r = rjs_set(rt, o, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
            goto end;

        rjs_value_set_undefined(rt, rv);
    } else {
        RJS_PropertyName pn;

        rjs_value_set_number(rt, lenv, len - 1);
        rjs_to_string(rt, lenv, idx);

        rjs_property_name_init(rt, &pn, idx);
        r = rjs_get(rt, o, &pn, rv);
        if (r == RJS_OK) {
            r = rjs_delete_property_or_throw(rt, o, &pn);
        }
        rjs_property_name_deinit(rt, &pn);
        if (r == RJS_ERR)
            goto end;

        if ((r = rjs_set(rt, o, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.push*/
static RJS_NF(Array_prototype_push)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *o    = rjs_value_stack_push(rt);
    RJS_Value *lenv = rjs_value_stack_push(rt);
    int64_t    len;
    size_t     i;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (len + argc > RJS_MAX_INT) {
        r = rjs_throw_type_error(rt, _("illegal array length"));
        goto end;
    }

    for (i = 0; i < argc; i ++) {
        RJS_Value *item = rjs_value_buffer_item(rt, args, i);

        if ((r = rjs_set_index(rt, o, len, item, RJS_TRUE)) == RJS_ERR)
            goto end;

        len ++;
    }

    rjs_value_set_number(rt, lenv, len);

    if ((r = rjs_set(rt, o, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, rv, len);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.reduce*/
static RJS_NF(Array_prototype_reduce)
{
    RJS_Value *cb_fn = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *initv = rjs_argument_get(rt, args, argc, 1);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *acc   = rjs_value_stack_push(rt);
    RJS_Value *kv    = rjs_value_stack_push(rt);
    RJS_Value *key   = rjs_value_stack_push(rt);
    RJS_Value *o     = rjs_value_stack_push(rt);
    RJS_Value *pk    = rjs_value_stack_push(rt);
    RJS_Value *res   = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_PropertyName pn;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((len == 0) && (argc < 2)) {
        r = rjs_throw_type_error(rt, _("initial value is not present"));
        goto end;
    }

    k = 0;

    if (argc >= 2) {
        rjs_value_copy(rt, acc, initv);
    } else {
        RJS_Bool present = RJS_FALSE;

        rjs_value_set_undefined(rt, acc);

        while (k < len) {
            rjs_value_set_number(rt, key, k);
            rjs_to_string(rt, key, pk);

            if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
                goto end;

            k ++;

            if (r) {
                rjs_property_name_init(rt, &pn, pk);
                r = rjs_get(rt, o, &pn, acc);
                rjs_property_name_deinit(rt, &pn);
                if (r == RJS_ERR)
                    goto end;

                present = RJS_TRUE;
                break;
            }
        }

        if (!present) {
            r = rjs_throw_type_error(rt, _("initial value is not present"));
            goto end;
        }
    }

    while (k < len) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, kv);
            if (r == RJS_OK) {
                r = rjs_call(rt, cb_fn, rjs_v_undefined(rt), acc, 4, res);
            }
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            rjs_value_copy(rt, acc, res);
        }

        k ++;
    }

    rjs_value_copy(rt, rv, acc);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.reduceRight*/
static RJS_NF(Array_prototype_reduceRight)
{
    RJS_Value *cb_fn = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *initv = rjs_argument_get(rt, args, argc, 1);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *acc   = rjs_value_stack_push(rt);
    RJS_Value *kv    = rjs_value_stack_push(rt);
    RJS_Value *key   = rjs_value_stack_push(rt);
    RJS_Value *o     = rjs_value_stack_push(rt);
    RJS_Value *pk    = rjs_value_stack_push(rt);
    RJS_Value *res   = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_PropertyName pn;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((len == 0) && (argc < 2)) {
        r = rjs_throw_type_error(rt, _("initial value is not present"));
        goto end;
    }

    k = len - 1;

    if (argc >= 2) {
        rjs_value_copy(rt, acc, initv);
    } else {
        RJS_Bool present = RJS_FALSE;

        rjs_value_set_undefined(rt, acc);

        while (k >= 0) {
            rjs_value_set_number(rt, key, k);
            rjs_to_string(rt, key, pk);

            if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
                goto end;

            k --;

            if (r) {
                rjs_property_name_init(rt, &pn, pk);
                r = rjs_get(rt, o, &pn, acc);
                rjs_property_name_deinit(rt, &pn);
                if (r == RJS_ERR)
                    goto end;

                present = RJS_TRUE;
                break;
            }
        }

        if (!present) {
            r = rjs_throw_type_error(rt, _("initial value is not present"));
            goto end;
        }
    }

    while (k >= 0) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, kv);
            if (r == RJS_OK) {
                r = rjs_call(rt, cb_fn, rjs_v_undefined(rt), acc, 4, res);
            }
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            rjs_value_copy(rt, acc, res);
        }

        k --;
    }

    rjs_value_copy(rt, rv, acc);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.reverse*/
static RJS_NF(Array_prototype_reverse)
{
    size_t     top     = rjs_value_stack_save(rt);
    RJS_Value *o       = rjs_value_stack_push(rt);
    RJS_Value *idx     = rjs_value_stack_push(rt);
    RJS_Value *lower_p = rjs_value_stack_push(rt);
    RJS_Value *upper_p = rjs_value_stack_push(rt);
    RJS_Value *lower_v = rjs_value_stack_push(rt);
    RJS_Value *upper_v = rjs_value_stack_push(rt);
    int64_t    len, lower, mid;
    RJS_PropertyName lower_pn, upper_pn;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    mid   = len / 2;
    lower = 0;

    while (lower < mid) {
        int64_t upper = len - lower - 1;
        RJS_Bool lower_exist, upper_exist;
        RJS_Bool has_lower_pn = RJS_FALSE;
        RJS_Bool has_upper_pn = RJS_FALSE;

        rjs_value_set_number(rt, idx, lower);
        rjs_to_string(rt, idx, lower_p);

        if ((r = rjs_has_property(rt, o, lower_p)) == RJS_ERR)
            goto item_end;
        lower_exist = r;
        rjs_property_name_init(rt, &lower_pn, lower_p);
        has_lower_pn = RJS_TRUE;
        if (lower_exist) {
            if ((r = rjs_get(rt, o, &lower_pn, lower_v)) == RJS_ERR)
                goto item_end;
        }

        rjs_value_set_number(rt, idx, upper);
        rjs_to_string(rt, idx, upper_p);

        if ((r = rjs_has_property(rt, o, upper_p)) == RJS_ERR)
            goto item_end;
        upper_exist = r;
        rjs_property_name_init(rt, &upper_pn, upper_p);
        has_upper_pn = RJS_TRUE;
        if (upper_exist) {
            if ((r = rjs_get(rt, o, &upper_pn, upper_v)) == RJS_ERR)
                goto item_end;
        }

        if (lower_exist && upper_exist) {
            if ((r = rjs_set(rt, o, &lower_pn, upper_v, RJS_TRUE)) == RJS_ERR)
                goto item_end;
            if ((r = rjs_set(rt, o, &upper_pn, lower_v, RJS_TRUE)) == RJS_ERR)
                goto item_end;
        } else if (upper_exist) {
            if ((r = rjs_set(rt, o, &lower_pn, upper_v, RJS_TRUE)) == RJS_ERR)
                goto item_end;
            if ((r = rjs_delete_property_or_throw(rt, o, &upper_pn)) == RJS_ERR)
                goto item_end;
        } else if (lower_exist) {
            if ((r = rjs_delete_property_or_throw(rt, o, &lower_pn)) == RJS_ERR)
                goto item_end;
            if ((r = rjs_set(rt, o, &upper_pn, lower_v, RJS_TRUE)) == RJS_ERR)
                goto item_end;
        }
item_end:
        if (has_lower_pn)
            rjs_property_name_deinit(rt, &lower_pn);
        if (has_upper_pn)
            rjs_property_name_deinit(rt, &upper_pn);
        if (r == RJS_ERR)
            goto end;

        lower ++;
    }

    rjs_value_copy(rt, rv, o);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.shift*/
static RJS_NF(Array_prototype_shift)
{
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *o      = rjs_value_stack_push(rt);
    RJS_Value *lenv   = rjs_value_stack_push(rt);
    RJS_Value *idx    = rjs_value_stack_push(rt);
    RJS_Value *from_p = rjs_value_stack_push(rt);
    RJS_Value *from_v = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (len == 0) {
        rjs_value_set_number(rt, lenv, 0);

        if ((r = rjs_set(rt, o, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
            goto end;

        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_get_index(rt, o, 0, rv)) == RJS_ERR)
        goto end;

    for (k = 1; k < len; k ++) {
        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, from_p);

        if ((r = rjs_has_property(rt, o, from_p)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, from_p);
            r = rjs_get(rt, o, &pn, from_v);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_set_index(rt, o, k - 1, from_v, RJS_TRUE)) == RJS_ERR)
                goto end;
        } else {
            if ((r = rjs_delete_property_or_throw_index(rt, o, k - 1)) == RJS_ERR)
                goto end;
        }
    }

    if ((r = rjs_delete_property_or_throw_index(rt, o, len - 1)) == RJS_ERR)
        goto end;

    rjs_value_set_number(rt, lenv, len - 1);
    if ((r = rjs_set(rt, o, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.slice*/
static RJS_NF(Array_prototype_slice)
{
    RJS_Value *start  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *end    = rjs_argument_get(rt, args, argc, 1);
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *o      = rjs_value_stack_push(rt);
    RJS_Value *a      = rjs_value_stack_push(rt);
    RJS_Value *lenv   = rjs_value_stack_push(rt);
    RJS_Value *idx    = rjs_value_stack_push(rt);
    RJS_Value *pk     = rjs_value_stack_push(rt);
    RJS_Value *pv     = rjs_value_stack_push(rt);
    RJS_Number rel_start, rel_end;
    int64_t    len, k, final, count, n;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;

    if (rel_start == -INFINITY)
        k = 0;
    else if (rel_start < 0)
        k = RJS_MAX(len + rel_start, 0);
    else
        k = RJS_MIN(rel_start, len);

    if (rjs_value_is_undefined(rt, end)) {
        rel_end = len;
    } else {
        if ((r = rjs_to_integer_or_infinity(rt, end, &rel_end)) == RJS_ERR)
            goto end;
    }

    if (rel_end == -INFINITY)
        final = 0;
    else if (rel_end < 0)
        final = RJS_MAX(len + rel_end, 0);
    else
        final = RJS_MIN(rel_end, len);

    if (final < k)
        count = 0;
    else
        count = final - k;

    if ((r = array_species_create(rt, o, count, a)) == RJS_ERR)
        goto end;

    n = 0;

    while (k < final) {
        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, pv);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_create_data_property_or_throw_index(rt, a, n, pv)) == RJS_ERR)
                goto end;
        }

        k ++;
        n ++;
    }

    rjs_value_set_number(rt, lenv, n);
    if ((r = rjs_set(rt, a, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.some*/
static RJS_NF(Array_prototype_some)
{
    RJS_Value *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    RJS_Value *item     = rjs_value_stack_push(rt);
    RJS_Value *key      = rjs_value_stack_push(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *tr       = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    for (k = 0; k < len; k ++) {
        rjs_value_set_number(rt, key, k);
        rjs_to_string(rt, key, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, item);
            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_call(rt, cb_fn, this_arg, item, 3, tr)) == RJS_ERR)
                goto end;

            if (rjs_to_boolean(rt, tr)) {
                rjs_value_set_boolean(rt, rv, RJS_TRUE);
                r = RJS_OK;
                goto end;
            }
        }
    }

    rjs_value_set_boolean(rt, rv, RJS_FALSE);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**Value buffer.*/
typedef struct {
    RJS_GcThing  gc_thing; /**< Base GC thing data.*/
    size_t       len;      /**< Length of the buffer.*/
    RJS_Value    value[0]; /**< The values.*/
} RJS_ValueBuffer;

/*Scan the referenced things in the value buffer.*/
static void
value_buffer_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ValueBuffer *vb = ptr;

    rjs_gc_scan_value_buffer(rt, vb->value, vb->len);
}

/*Free the value buffer.*/
static void
value_buffer_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ValueBuffer *vb   = ptr;
    size_t           size = sizeof(RJS_ValueBuffer) + sizeof(RJS_Value) * vb->len;

    rjs_free(rt, vb, size);
}

/**Value buffer operation functions.*/
static const RJS_GcThingOps
value_buffer_ops = {
    RJS_GC_THING_VALUE_BUFFER,
    value_buffer_op_gc_scan,
    value_buffer_op_gc_free
};

/**Array item compare parameters.*/
typedef struct {
    RJS_Runtime *rt;  /**< The current runtime.*/
    RJS_Value *cmp_fn; /**< Compare function.*/
} RJS_ArrayCompareParams;

/*Compare 2 array items.*/
static RJS_CompareResult
array_item_compare_fn (const void *p1, const void *p2, void *params)
{
    RJS_Value              *v1    = (RJS_Value*)p1;
    RJS_Value              *v2    = (RJS_Value*)p2;
    RJS_ArrayCompareParams *acp   = params;
    RJS_Runtime              *rt = acp->rt;
    RJS_Value              *cmp   = acp->cmp_fn;
    size_t                  top   = rjs_value_stack_save(rt);
    RJS_Value              *x     = rjs_value_stack_push(rt);
    RJS_Value              *y     = rjs_value_stack_push(rt);
    RJS_Value              *res   = rjs_value_stack_push(rt);
    RJS_CompareResult       r;

    if (rjs_value_is_undefined(rt, v1) && rjs_value_is_undefined(rt, v2)) {
        r = RJS_COMPARE_EQUAL;
        goto end;
    }
    if (rjs_value_is_undefined(rt, v1)) {
        r = RJS_COMPARE_GREATER;
        goto end;
    }
    if (rjs_value_is_undefined(rt, v2)) {
        r = RJS_COMPARE_LESS;
        goto end;
    }

    if (!rjs_value_is_undefined(rt, cmp)) {
        RJS_Number n;

        rjs_value_copy(rt, x, v1);
        rjs_value_copy(rt, y, v2);

        if ((r = rjs_call(rt, cmp, rjs_v_undefined(rt), x, 2, res)) == RJS_ERR)
            goto end;

        if ((r = rjs_to_number(rt, res, &n)) == RJS_ERR)
            goto end;

        if (isnan(n))
            n = 0;

        if (n < 0)
            r = RJS_COMPARE_LESS;
        else if (n == 0)
            r = RJS_COMPARE_EQUAL;
        else
            r = RJS_COMPARE_GREATER;
    } else {
        if ((r = rjs_to_string(rt, v1, x)) == RJS_ERR)
            goto end;

        if ((r = rjs_to_string(rt, v2, y)) == RJS_ERR)
            goto end;

        r = rjs_string_compare(rt, x, y);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.sort*/
static RJS_NF(Array_prototype_sort)
{
    RJS_Value *cmp_fn = rjs_argument_get(rt, args, argc, 0);
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *o      = rjs_value_stack_push(rt);
    RJS_Value *idx    = rjs_value_stack_push(rt);
    RJS_Value *pk     = rjs_value_stack_push(rt);
    RJS_Value *items  = rjs_value_stack_push(rt);
    size_t     size;
    RJS_ValueBuffer       *vb;
    RJS_ArrayCompareParams params;
    int64_t    len, k, nitem;
    RJS_Result r;

    if (!rjs_value_is_undefined(rt, cmp_fn) && !rjs_is_callable(rt, cmp_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    size = sizeof(RJS_ValueBuffer) + sizeof(RJS_Value) * len;
    vb   = rjs_alloc_assert(rt, size);

    vb->len = len;
    rjs_value_buffer_fill_undefined(rt, vb->value, vb->len);
    rjs_value_set_gc_thing(rt, items, vb);
    rjs_gc_add(rt, vb, &value_buffer_ops);

    nitem = 0;

    for (k = 0; k < len; k ++) {
        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, &vb->value[nitem]);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            nitem ++;
        }
    }

    params.rt     = rt;
    params.cmp_fn = cmp_fn;

    if ((r = rjs_sort(vb->value, vb->len, sizeof(RJS_Value), array_item_compare_fn, &params)) == RJS_ERR)
        goto end;

    for (k = 0; k < nitem; k ++) {
        if ((r = rjs_set_index(rt, o, k, &vb->value[k], RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    for (; k < len; k ++) {
        if ((r = rjs_delete_property_or_throw_index(rt, o, k)) == RJS_ERR)
            goto end;
    }

    rjs_value_copy(rt, rv, o);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.splice*/
static RJS_NF(Array_prototype_splice)
{
    RJS_Value *start   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *del_cnt = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *items   = rjs_argument_get(rt, args, argc, 2);
    size_t     top     = rjs_value_stack_save(rt);
    RJS_Value *o       = rjs_value_stack_push(rt);
    RJS_Value *a       = rjs_value_stack_push(rt);
    RJS_Value *idx     = rjs_value_stack_push(rt);
    RJS_Value *lenv    = rjs_value_stack_push(rt);
    RJS_Value *pk      = rjs_value_stack_push(rt);
    RJS_Value *kv      = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Number rel_start;
    int64_t    act_start, act_del_cnt, insert_cnt, len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;

    if (rel_start == -INFINITY)
        act_start = 0;
    else if (rel_start < 0)
        act_start = RJS_MAX(len + rel_start, 0);
    else
        act_start = RJS_MIN(rel_start, len);

    if (argc > 2)
        insert_cnt = argc - 2;
    else
        insert_cnt = 0;

    if (argc < 1)
        act_del_cnt = 0;
    else if (argc < 2)
        act_del_cnt = len - act_start;
    else {
        RJS_Number dc;

        if ((r = rjs_to_integer_or_infinity(rt, del_cnt, &dc)) == RJS_ERR)
            goto end;

        act_del_cnt = RJS_CLAMP(dc, 0, len - act_start);
    }

    if (len + insert_cnt - act_del_cnt > RJS_MAX_INT) {
        r = rjs_throw_type_error(rt, _("illegal array length"));
        goto end;
    }

    if ((r = array_species_create(rt, o, act_del_cnt, a)) == RJS_ERR)
        goto end;

    for (k = 0; k < act_del_cnt; k ++) {
        rjs_value_set_number(rt, idx, act_start + k);
        rjs_to_string(rt, idx, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, kv);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_create_data_property_or_throw_index(rt, a, k, kv)) == RJS_ERR)
                goto end;
        }
    }

    rjs_value_set_number(rt, lenv, act_del_cnt);
    if ((r = rjs_set(rt, a, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
        goto end;

    if (insert_cnt < act_del_cnt) {
        for (k = act_start; k < len; k ++) {
            rjs_value_set_number(rt, idx, k + act_del_cnt);
            rjs_to_string(rt, idx, pk);

            if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
                goto end;

            if (r) {
                rjs_property_name_init(rt, &pn, pk);
                r = rjs_get(rt, o, &pn, kv);
                rjs_property_name_deinit(rt, &pn);
                if (r == RJS_ERR)
                    goto end;

                if ((r = rjs_set_index(rt, o, k + insert_cnt, kv, RJS_TRUE)) == RJS_ERR)
                    goto end;
            } else {
                if ((r = rjs_delete_property_or_throw_index(rt, o, k + insert_cnt)) == RJS_ERR)
                    goto end;
            }
        }

        for (k = len; k > len - act_del_cnt + insert_cnt; k --) {
            if ((r = rjs_delete_property_or_throw_index(rt, o, k - 1)) == RJS_ERR)
                goto end;
        }
    } else if (insert_cnt > act_del_cnt) {
        for (k = len - act_del_cnt; k >= act_start; k --) {
            rjs_value_set_number(rt, idx, k + act_del_cnt - 1);
            rjs_to_string(rt, idx, pk);

            if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
                goto end;

            if (r) {
                rjs_property_name_init(rt, &pn, pk);
                r = rjs_get(rt, o, &pn, kv);
                rjs_property_name_deinit(rt, &pn);
                if (r == RJS_ERR)
                    goto end;

                if ((r = rjs_set_index(rt, o, k + insert_cnt - 1, kv, RJS_TRUE)) == RJS_ERR)
                    goto end;
            } else {
                if ((r = rjs_delete_property_or_throw_index(rt, o, k + insert_cnt - 1)) == RJS_ERR)
                    goto end;
            }
        }
    }

    for (k = 0; k < insert_cnt; k ++) {
        RJS_Value *item = rjs_value_buffer_item(rt, items, k);

        if ((r = rjs_set_index(rt, o, k + act_start, item, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    rjs_value_set_number(rt, lenv, len - act_del_cnt + insert_cnt);
    if ((r = rjs_set(rt, o, rjs_pn_length(rt), lenv, RJS_TRUE)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.toLocaleString*/
static RJS_NF(Array_prototype_toLocaleString)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *o    = rjs_value_stack_push(rt);
    RJS_Value *elem = rjs_value_stack_push(rt);
    RJS_Value *er   = rjs_value_stack_push(rt);
    RJS_Value *es   = rjs_value_stack_push(rt);
    RJS_UCharBuffer ucb;
    int64_t    len, k;
    RJS_Result r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        if (k > 0)
            rjs_uchar_buffer_append_string(rt, &ucb, rjs_s_comma(rt));

        if ((r = rjs_get_index(rt, o, k, elem)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_undefined(rt, elem) && !rjs_value_is_null(rt, elem)) {
            if ((r = rjs_invoke(rt, elem, rjs_pn_toLocaleString(rt), NULL, 0, er)) == RJS_ERR)
                goto end;

            if ((r = rjs_to_string(rt, er, es)) == RJS_ERR)
                goto end;

            rjs_uchar_buffer_append_string(rt, &ucb, es);
        }
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.toReversed*/
static RJS_NF(Array_prototype_toReversed)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *o    = rjs_value_stack_push(rt);
    RJS_Value *a    = rjs_value_stack_push(rt);
    RJS_Value *idx  = rjs_value_stack_push(rt);
    RJS_Value *from = rjs_value_stack_push(rt);
    RJS_Value *pk   = rjs_value_stack_push(rt);
    RJS_Value *pv   = rjs_value_stack_push(rt);
    int64_t    len, k;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_array_new(rt, a, len, NULL)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        RJS_PropertyName from_pn, pn;

        rjs_value_set_number(rt, idx, len - k - 1);
        rjs_to_string(rt, idx, from);

        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, pk);

        rjs_property_name_init(rt, &from_pn, from);
        rjs_property_name_init(rt, &pn, pk);

        if ((r = rjs_get(rt, o, &from_pn, pv)) == RJS_OK) {
            r = rjs_create_data_property_or_throw(rt, a, &pn, pv);
        }

        rjs_property_name_deinit(rt, &from_pn);
        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.toSorted*/
static RJS_NF(Array_prototype_toSorted)
{
    RJS_Value *cmp_fn = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *o     = rjs_value_stack_push(rt);
    RJS_Value *a     = rjs_value_stack_push(rt);
    RJS_Value *items = rjs_value_stack_push(rt);
    RJS_Value *idx   = rjs_value_stack_push(rt);
    RJS_Value *pk    = rjs_value_stack_push(rt);
    size_t     size;
    RJS_ValueBuffer       *vb;
    RJS_ArrayCompareParams params;
    int64_t    len, k, nitem;
    RJS_Result r;

    if (!rjs_value_is_undefined(rt, cmp_fn) && !rjs_is_callable(rt, cmp_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_array_new(rt, a, len, NULL)) == RJS_ERR)
        goto end;

    size = sizeof(RJS_ValueBuffer) + sizeof(RJS_Value) * len;
    vb   = rjs_alloc_assert(rt, size);

    vb->len = len;
    rjs_value_buffer_fill_undefined(rt, vb->value, vb->len);
    rjs_value_set_gc_thing(rt, items, vb);
    rjs_gc_add(rt, vb, &value_buffer_ops);

    nitem = 0;

    for (k = 0; k < len; k ++) {
        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, pk);

        if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
            goto end;

        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, pk);
            r = rjs_get(rt, o, &pn, &vb->value[nitem]);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            nitem ++;
        }
    }

    params.rt     = rt;
    params.cmp_fn = cmp_fn;

    if ((r = rjs_sort(vb->value, vb->len, sizeof(RJS_Value), array_item_compare_fn, &params)) == RJS_ERR)
        goto end;

    for (k = 0; k < nitem; k ++) {
        if ((r = rjs_set_index(rt, a, k, &vb->value[k], RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    for (; k < len; k ++) {
        if ((r = rjs_set_index(rt, a, k, rjs_v_undefined(rt), RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.toSpliced*/
static RJS_NF(Array_prototype_toSpliced)
{
    RJS_Value *start    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *skip_cnt = rjs_argument_get(rt, args, argc, 1);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *o        = rjs_value_stack_push(rt);
    RJS_Value *a        = rjs_value_stack_push(rt);
    RJS_Value *idx      = rjs_value_stack_push(rt);
    RJS_Value *pk       = rjs_value_stack_push(rt);
    RJS_Value *to_pk    = rjs_value_stack_push(rt);
    RJS_Value *pv       = rjs_value_stack_push(rt);
    RJS_PropertyName pn, to_pn;
    int64_t    len, new_len, i, j, k, act_start, act_skip, insert_cnt;
    RJS_Number rel_start;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;

    if (rel_start == -INFINITY)
        act_start = 0;
    else if (rel_start < 0)
        act_start = RJS_MAX(rel_start + len, 0);
    else
        act_start = RJS_MIN(rel_start, len);

    if (argc > 2)
        insert_cnt = argc - 2;
    else
        insert_cnt = 0;

    if (argc == 0)
        act_skip = 0;
    else if (argc == 1)
        act_skip = len - act_start;
    else {
        RJS_Number sc;

        if ((r = rjs_to_integer_or_infinity(rt, skip_cnt, &sc)) == RJS_ERR)
            goto end;

        act_skip = RJS_CLAMP(sc, 0, len - act_start);
    }

    new_len = len - act_skip + insert_cnt;

    if (new_len > RJS_MAX_INT) {
        r = rjs_throw_type_error(rt, _("new array's length is too long"));
        goto end;
    }

    if ((r = rjs_array_new(rt, a, new_len, NULL)) == RJS_ERR)
        goto end;

    i = 0;
    j = act_start + act_skip;

    for (; i < act_start; i ++) {
        rjs_value_set_number(rt, idx, i);
        rjs_to_string(rt, idx, pk);

        rjs_property_name_init(rt, &pn, pk);
        if ((r = rjs_get(rt, o, &pn, pv)) == RJS_OK)
            r = rjs_create_data_property_or_throw(rt, a, &pn, pv);
        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    for (k = 0; k < insert_cnt; k ++, i ++) {
        RJS_Value *item = rjs_value_buffer_item(rt, args, k + 2);

        rjs_value_set_number(rt, idx, i);
        rjs_to_string(rt, idx, pk);

        rjs_property_name_init(rt, &pn, pk);
        r = rjs_create_data_property_or_throw(rt, a, &pn, item);
        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    for (; i < new_len; i ++, j ++) {
        rjs_value_set_number(rt, idx, j);
        rjs_to_string(rt, idx, pk);

        rjs_value_set_number(rt, idx, i);
        rjs_to_string(rt, idx, to_pk);

        rjs_property_name_init(rt, &pn, pk);
        rjs_property_name_init(rt, &to_pn, to_pk);

        if ((r = rjs_get(rt, o, &pn, pv)) == RJS_OK)
            r = rjs_create_data_property_or_throw(rt, a, &to_pn, pv);

        rjs_property_name_deinit(rt, &pn);
        rjs_property_name_deinit(rt, &to_pn);

        if (r == RJS_ERR)
            goto end;
    }

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.toString*/
static RJS_NF(Array_prototype_toString)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *a     = rjs_value_stack_push(rt);
    RJS_Value *fn    = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, a)) == RJS_ERR)
        goto end;

    if ((r = rjs_get(rt, a, rjs_pn_join(rt), fn)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, fn)) {
        rjs_value_copy(rt, fn, rjs_o_Object_prototype_toString(realm));
    }

    r = rjs_call(rt, fn, a, NULL, 0, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.unshift*/
static RJS_NF(Array_prototype_unshift)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *o    = rjs_value_stack_push(rt);
    RJS_Value *idx  = rjs_value_stack_push(rt);
    RJS_Value *pk   = rjs_value_stack_push(rt);
    RJS_Value *kv   = rjs_value_stack_push(rt);
    int64_t    len, k;
    size_t     aid;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if (argc > 0) {
        if (len + argc > RJS_MAX_INT) {
            r = rjs_throw_type_error(rt, _("illegal array length"));
            goto end;
        }

        for (k = len; k >= 0; k --) {
            rjs_value_set_number(rt, idx, k - 1);
            rjs_to_string(rt, idx, pk);

            if ((r = rjs_has_property(rt, o, pk)) == RJS_ERR)
                goto end;

            if (r) {
                RJS_PropertyName pn;

                rjs_property_name_init(rt, &pn, pk);
                r = rjs_get(rt, o, &pn, kv);
                rjs_property_name_deinit(rt, &pn);
                if (r == RJS_ERR)
                    goto end;

                if ((r = rjs_set_index(rt, o, k + argc - 1, kv, RJS_TRUE)) == RJS_ERR)
                    goto end;
            } else {
                if ((r = rjs_delete_property_or_throw_index(rt, o, k + argc - 1)) == RJS_ERR)
                    goto end;
            }
        }

        for (aid = 0; aid < argc; aid ++) {
            RJS_Value *arg = rjs_value_buffer_item(rt, args, aid);

            if ((r = rjs_set_index(rt, o, aid, arg, RJS_TRUE)) == RJS_ERR)
                goto end;
        }
    }

    rjs_value_set_number(rt, rv, len + argc);

    if ((r = rjs_set(rt, o, rjs_pn_length(rt), rv, RJS_TRUE)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.values*/
static RJS_NF(Array_prototype_values)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *o   = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    r = create_array_iterator(rt, o, RJS_ARRAY_ITERATOR_VALUE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Array.prototype.with*/
static RJS_NF(Array_prototype_with)
{
    RJS_Value *index = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *value = rjs_argument_get(rt, args, argc, 1);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *o     = rjs_value_stack_push(rt);
    RJS_Value *a     = rjs_value_stack_push(rt);
    RJS_Value *idx   = rjs_value_stack_push(rt);
    RJS_Value *pk    = rjs_value_stack_push(rt);
    RJS_Value *pv    = rjs_value_stack_push(rt);
    int64_t    len, k;
    int64_t    act_index;
    RJS_Number rel_index;
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, o, &len)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_integer_or_infinity(rt, index, &rel_index)) == RJS_ERR)
        goto end;

    if (rel_index >= 0)
        act_index = rel_index;
    else
        act_index = len + rel_index;

    if ((act_index < 0) || (act_index >= len)) {
        r = rjs_throw_range_error(rt, _("index value overflow"));
        goto end;
    }

    if ((r = rjs_array_new(rt, a, len, NULL)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        RJS_PropertyName pn;

        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, pk);

        rjs_property_name_init(rt, &pn, pk);

        if (k == act_index) {
            rjs_value_copy(rt, pv, value);
            r = RJS_OK;
        } else {
            r = rjs_get(rt, o, &pn, pv);
        }
        if (r == RJS_OK)
            rjs_create_data_property_or_throw(rt, a, &pn, pv);

        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    rjs_value_copy(rt, rv, a);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create the unscopable property.*/
static RJS_Result
create_unscopable_prop (RJS_Runtime *rt, RJS_Value *o, const char *p)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *pk  = rjs_value_stack_push(rt);
    RJS_Value *pv  = rjs_value_stack_push(rt);
    RJS_PropertyName pn;

    rjs_string_from_chars(rt, pk, p, -1);
    rjs_string_to_property_key(rt, pk);
    rjs_value_set_boolean(rt, pv, RJS_TRUE);

    rjs_property_name_init(rt, &pn, pk);
    rjs_create_data_property_or_throw(rt, o, &pn, pv);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*Array.prototype.unscopables*/
static void
add_Array_prototype_unscopables (RJS_Runtime *rt, RJS_Realm *realm)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *o     = rjs_value_stack_push(rt);

    rjs_ordinary_object_create(rt, rjs_v_null(rt), o);

    create_unscopable_prop(rt, o, "at");
    create_unscopable_prop(rt, o, "copyWithin");
    create_unscopable_prop(rt, o, "entries");
    create_unscopable_prop(rt, o, "fill");
    create_unscopable_prop(rt, o, "find");
    create_unscopable_prop(rt, o, "findIndex");
    create_unscopable_prop(rt, o, "findLast");
    create_unscopable_prop(rt, o, "findLastIndex");
    create_unscopable_prop(rt, o, "flat");
    create_unscopable_prop(rt, o, "flatMap");
    create_unscopable_prop(rt, o, "includes");
    create_unscopable_prop(rt, o, "keys");
    create_unscopable_prop(rt, o, "toReversed");
    create_unscopable_prop(rt, o, "toSorted");
    create_unscopable_prop(rt, o, "toSpliced");
    create_unscopable_prop(rt, o, "values");

    rjs_create_data_property_attrs(rt, rjs_o_Array_prototype(realm),
            rjs_pn_s_unscopables(rt), o, RJS_PROP_ATTR_CONFIGURABLE);

    rjs_value_stack_restore(rt, top);
}

static const RJS_BuiltinFuncDesc
array_prototype_function_descs[] = {
    {
        "at",
        1,
        Array_prototype_at
    },
    {
        "concat",
        1,
        Array_prototype_concat
    },
    {
        "copyWithin",
        2,
        Array_prototype_copyWithin
    },
    {
        "entries",
        0,
        Array_prototype_entries
    },
    {
        "every",
        1,
        Array_prototype_every
    },
    {
        "fill",
        1,
        Array_prototype_fill
    },
    {
        "filter",
        1,
        Array_prototype_filter
    },
    {
        "find",
        1,
        Array_prototype_find
    },
    {
        "findIndex",
        1,
        Array_prototype_findIndex
    },
    {
        "findLast",
        1,
        Array_prototype_findLast
    },
    {
        "findLastIndex",
        1,
        Array_prototype_findLastIndex
    },
    {
        "flat",
        0,
        Array_prototype_flat
    },
    {
        "flatMap",
        1,
        Array_prototype_flatMap
    },
    {
        "forEach",
        1,
        Array_prototype_forEach
    },
    {
        "includes",
        1,
        Array_prototype_includes
    },
    {
        "indexOf",
        1,
        Array_prototype_indexOf
    },
    {
        "join",
        1,
        Array_prototype_join
    },
    {
        "keys",
        0,
        Array_prototype_keys
    },
    {
        "lastIndexOf",
        1,
        Array_prototype_lastIndexOf
    },
    {
        "map",
        1,
        Array_prototype_map
    },
    {
        "pop",
        0,
        Array_prototype_pop
    },
    {
        "push",
        1,
        Array_prototype_push
    },
    {
        "reduce",
        1,
        Array_prototype_reduce
    },
    {
        "reduceRight",
        1,
        Array_prototype_reduceRight
    },
    {
        "reverse",
        0,
        Array_prototype_reverse
    },
    {
        "shift",
        0,
        Array_prototype_shift
    },
    {
        "slice",
        2,
        Array_prototype_slice
    },
    {
        "some",
        1,
        Array_prototype_some
    },
    {
        "sort",
        1,
        Array_prototype_sort
    },
    {
        "splice",
        2,
        Array_prototype_splice
    },
    {
        "toLocaleString",
        0,
        Array_prototype_toLocaleString
    },
    {
        "toReversed",
        0,
        Array_prototype_toReversed
    },
    {
        "toSorted",
        1,
        Array_prototype_toSorted
    },
    {
        "toSpliced",
        2,
        Array_prototype_toSpliced
    },
    {
        "toString",
        0,
        Array_prototype_toString,
        "Array_prototype_toString"
    },
    {
        "unshift",
        1,
        Array_prototype_unshift
    },
    {
        "values",
        0,
        Array_prototype_values,
        "Array_prototype_values"
    },
    {
        "with",
        2,
        Array_prototype_with
    },
    {
        "@@iterator",
        0,
        NULL,
        "Array_prototype_values"
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
array_prototype_desc = {
    "Array",
    NULL,
    NULL,
    NULL,
    NULL,
    array_prototype_function_descs,
    NULL,
    NULL,
    "Array_prototype"
};

static const RJS_BuiltinFieldDesc
array_iterator_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Array Iterator",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*ArrayIteratorPrototype.next*/
static RJS_NF(ArrayIteratorPrototype_next)
{
    size_t             top = rjs_value_stack_save(rt);
    RJS_Value         *iv  = rjs_value_stack_push(rt);
    RJS_Value         *idx = rjs_value_stack_push(rt);
    RJS_Value         *key = rjs_value_stack_push(rt);
    RJS_Value         *kv  = rjs_value_stack_push(rt);
    RJS_ArrayIterator *ai;
    int64_t            len;
    RJS_Bool           done;
    RJS_Result         r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_ARRAY_ITERATOR) {
        r = rjs_throw_type_error(rt, _("the value is not an array iterator"));
        goto end;
    }

    ai = (RJS_ArrayIterator*)rjs_value_get_object(rt, thiz);

#if ENABLE_INT_INDEXED_OBJECT
    if (rjs_value_get_gc_thing_type(rt, &ai->array) == RJS_GC_THING_INT_INDEXED_OBJECT) {
        RJS_IntIndexedObject *iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, &ai->array);

        if (rjs_is_detached_buffer(rt, &iio->buffer)) {
            r = rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        len = iio->array_length;
    } else
#endif /*ENABLE_INT_INDEXED_OBJECT*/
    {
        if ((r = rjs_length_of_array_like(rt, &ai->array, &len)) == RJS_ERR)
            goto end;
    }

    if (ai->curr >= len) {
        rjs_value_set_undefined(rt, iv);

        done = RJS_TRUE;
    } else {
        RJS_PropertyName pn;

        switch (ai->type) {
        case RJS_ARRAY_ITERATOR_KEY:
            rjs_value_set_number(rt, iv, ai->curr);
            break;
        case RJS_ARRAY_ITERATOR_VALUE:
            rjs_value_set_number(rt, idx, ai->curr);
            rjs_to_string(rt, idx, key);

            rjs_property_name_init(rt, &pn, key);
            r = rjs_get(rt, &ai->array, &pn, iv);
            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;
            break;
        case RJS_ARRAY_ITERATOR_KEY_VALUE:
            rjs_value_set_number(rt, idx, ai->curr);
            rjs_to_string(rt, idx, key);

            rjs_property_name_init(rt, &pn, key);
            r = rjs_get(rt, &ai->array, &pn, kv);
            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;

            if ((r = rjs_create_array_from_elements(rt, iv, idx, kv, NULL)) == RJS_ERR)
                goto end;
            break;
        }

        done = RJS_FALSE;
    }

    ai->curr ++;

    r = rjs_create_iter_result_object(rt, iv, done, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
array_iterator_prototype_function_descs[] = {
    {
        "next",
        0,
        ArrayIteratorPrototype_next
    },
    {NULL}
};
