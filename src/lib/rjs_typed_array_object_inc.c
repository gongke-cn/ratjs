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

/*%TypedArray%*/
static RJS_NF(TypedArray_constructor)
{
    return rjs_throw_type_error(rt, _("\"%TypedArray%\" cannot be invoked directly"));
}

static const RJS_BuiltinFuncDesc
typed_array_constructor_desc = {
    "TypedArray",
    0,
    TypedArray_constructor
};

/*Check if the typed array content is big integer.*/
static RJS_Bool
content_is_big_int (RJS_ArrayElementType type)
{
#if ENABLE_BIG_INT
    return (type == RJS_ARRAY_ELEMENT_BIGINT64) || (type == RJS_ARRAY_ELEMENT_BIGUINT64);
#else
    return RJS_FALSE;
#endif
}

/*Check if the value is a valid typed array.*/
static RJS_Result
valid_typed_array (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_IntIndexedObject *iio;

    if (rjs_value_get_gc_thing_type(rt, v) != RJS_GC_THING_INT_INDEXED_OBJECT)
        return rjs_throw_type_error(rt, _("the value is not a TypedArray"));

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, v);
    if (rjs_is_detached_buffer(rt, &iio->buffer))
        return rjs_throw_type_error(rt, _("the array buffer is detached"));

    return RJS_OK;
}

/*Create a typed array.*/
static RJS_Result
typed_array_create (RJS_Runtime *rt, RJS_Value *c, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_Result r;

    if ((r = rjs_construct(rt, c, args, argc, NULL, rv)) == RJS_ERR)
        return r;

    if ((r = valid_typed_array(rt, rv)) == RJS_ERR)
        return r;

    if ((argc == 1) && rjs_value_is_number(rt, args)) {
        RJS_IntIndexedObject *iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, rv);
        RJS_Number            len = rjs_value_get_number(rt, args);

        if (iio->array_length < len)
            return rjs_throw_type_error(rt, _("typed array length < expected length"));
    }

    return RJS_OK;
}

/*%TypedArray%.from*/
static RJS_NF(TypedArray_from)
{
    RJS_Value *source   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *map_fn   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 2);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *iter_fn  = rjs_value_stack_push(rt);
    RJS_Value *array    = rjs_value_stack_push(rt);
    RJS_Value *kv       = rjs_value_stack_push(rt);
    RJS_Value *idx      = rjs_value_stack_push(rt);
    RJS_Value *mappedv  = rjs_value_stack_push(rt);
    RJS_Value *vlv      = rjs_value_stack_push(rt);
    RJS_Value *ir       = rjs_value_stack_push(rt);
    RJS_Iterator          iter;
    RJS_ValueList        *vl;
    RJS_ValueListSegment *vls;
    RJS_Bool   mapping;
    int64_t    i, k, len;
    RJS_Result r;

    rjs_iterator_init(rt, &iter);

    if (!rjs_is_constructor(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("this is not a constructor"));
        goto end;
    }

    if (rjs_value_is_undefined(rt, map_fn)) {
        mapping = RJS_FALSE;
    } else {
        if (!rjs_is_callable(rt, map_fn)) {
            r = rjs_throw_type_error(rt, _("the value is not a function"));
            goto end;
        }

        mapping = RJS_TRUE;
    }

    if ((r = rjs_get_method(rt, source, rjs_pn_s_iterator(rt), iter_fn)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, iter_fn)) {
        if ((r = rjs_get_iterator(rt, source, RJS_ITERATOR_SYNC, iter_fn, &iter)) == RJS_ERR)
            goto end;

        vl = rjs_value_list_new(rt, vlv);

        len = 0;
        while (1) {
            if ((r = rjs_iterator_step(rt, &iter, ir)) == RJS_ERR)
                goto end;

            if (!r)
                break;

            if ((r = rjs_iterator_value(rt, ir, kv)) == RJS_ERR)
                goto end;

            rjs_value_list_append(rt, vl, kv);

            len ++;
        }

        rjs_value_set_number(rt, idx, len);
        if ((r = typed_array_create(rt, thiz, idx, 1, rv)) == RJS_ERR)
            goto end;

        k = 0;
        rjs_list_foreach_c(&vl->seg_list, vls, RJS_ValueListSegment, ln) {
            for (i = 0; i < vls->num; i ++) {
                if (mapping) {
                    rjs_value_copy(rt, kv, &vls->v[i]);
                    rjs_value_set_number(rt, idx, k);

                    if ((r = rjs_call(rt, map_fn, this_arg, kv, 2, mappedv)) == RJS_ERR)
                        goto end;
                } else {
                    rjs_value_copy(rt, mappedv, &vls->v[i]);
                }

                if ((r = rjs_set_index(rt, rv, k, mappedv, RJS_TRUE)) == RJS_ERR)
                    goto end;

                k ++;
            }
        }
    } else {
        rjs_to_object(rt, source, array);

        if ((r = rjs_length_of_array_like(rt, array, &len)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, idx, len);
        if ((r = typed_array_create(rt, thiz, idx, 1, rv)) == RJS_ERR)
            goto end;

        for (k = 0; k < len; k ++) {
            if ((r = rjs_get_index(rt, source, k, kv)) == RJS_ERR)
                goto end;

            if (mapping) {
                rjs_value_set_number(rt, idx, k);

                if ((r = rjs_call(rt, map_fn, this_arg, kv, 2, mappedv)) == RJS_ERR)
                    goto end;
            } else {
                rjs_value_copy(rt, mappedv, kv);
            }

            if ((r = rjs_set_index(rt, rv, k, mappedv, RJS_TRUE)) == RJS_ERR)
                goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_iterator_deinit(rt, &iter);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*%TypedArray%.of*/
static RJS_NF(TypedArray_of)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *len = rjs_value_stack_push(rt);
    size_t     k;
    RJS_Result r;

    if (!rjs_is_constructor(rt, thiz)) {
        r = rjs_throw_type_error(rt, _("this is not a constructor"));
        goto end;
    }

    rjs_value_set_number(rt, len, argc);
    if ((r = typed_array_create(rt, thiz, len, 1, rv)) == RJS_ERR)
        goto end;

    for (k = 0; k < argc; k ++) {
        RJS_Value *arg = rjs_value_buffer_item(rt, args, k);

        if ((r = rjs_set_index(rt, rv, k, arg, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
typed_array_function_descs[] = {
    {
        "from",
        1,
        TypedArray_from
    },
    {
        "of",
        0,
        TypedArray_of
    },
    {NULL}
};

static const RJS_BuiltinAccessorDesc
typed_array_accessor_descs[] = {
    {
        "@@species",
        rjs_return_this
    },
    {NULL}
};

/*TypedArray.prototype.at*/
static RJS_NF(TypedArray_prototype_at)
{
    RJS_Value            *index = rjs_argument_get(rt, args, argc, 0);
    RJS_IntIndexedObject *iio;
    RJS_Number            rel_index, k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    if ((r = rjs_to_integer_or_infinity(rt, index, &rel_index)) == RJS_ERR)
        goto end;

    if (rel_index >= 0)
        k = rel_index;
    else
        k = iio->array_length + rel_index;

    if ((k < 0) || (k >= iio->array_length))
        rjs_value_set_undefined(rt, rv);
    else
        rjs_get_index(rt, thiz, k, rv);

    r = RJS_OK;
end:
    return r;
}

/*TypedArray.prototype.copyWithin*/
static RJS_NF(TypedArray_prototype_copyWithin)
{
    RJS_Value            *target = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *start  = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *end    = rjs_argument_get(rt, args, argc, 2);
    RJS_IntIndexedObject *iio;
    RJS_Result            r;
    RJS_Number            rel_target, rel_start, rel_end;
    ssize_t               to, from, final, count, len;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;

    if ((r = rjs_to_integer_or_infinity(rt, target, &rel_target)) == RJS_ERR)
        goto end;

    if (rel_target == -INFINITY)
        to = 0;
    else if (rel_target < 0)
        to = RJS_MAX(rel_target + len, 0);
    else
        to = RJS_MIN(rel_target, len);

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;

    if (rel_start == -INFINITY)
        from = 0;
    else if (rel_start < 0)
        from = RJS_MAX(rel_start + len, 0);
    else
        from = RJS_MIN(rel_start, len);

    if (rjs_value_is_undefined(rt, end))
        rel_end = len;
    else if ((r = rjs_to_integer_or_infinity(rt, end, &rel_end)) == RJS_ERR)
        goto end;

    if (rel_end == -INFINITY)
        final = 0;
    else if (rel_end < 0)
        final = RJS_MAX(rel_end + len, 0);
    else
        final = RJS_MIN(rel_end, len);

    count = RJS_MIN(final - from, len - to);

    if (count > 0) {
        ssize_t          esize, boff, to_byte_idx, from_byte_idx, count_bytes;
        uint8_t         *buf;
        RJS_ArrayBuffer *ab;

        if (rjs_is_detached_buffer(rt, &iio->buffer)) {
            r = rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        esize = rjs_typed_array_element_size(iio->type);
        boff  = iio->byte_offset;

        from_byte_idx = boff + esize * from;
        to_byte_idx   = boff + esize * to;
        count_bytes   = esize * count;

        ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);

        rjs_array_buffer_lock(rt, &iio->buffer);

        buf = rjs_data_block_get_buffer(ab->data_block);

        memmove(buf + to_byte_idx, buf + from_byte_idx, count_bytes);

        rjs_array_buffer_unlock(rt, &iio->buffer);
    }

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

/*TypedArray.prototype.entries*/
static RJS_NF(TypedArray_prototype_entries)
{
    RJS_Result r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        return r;

    return create_array_iterator(rt, thiz, RJS_ARRAY_ITERATOR_KEY_VALUE, rv);
}

/*TypedArray.prototype.every*/
static RJS_NF(TypedArray_prototype_every)
{
    RJS_Value            *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, o, thiz);

    for (k = 0; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);

        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, cb_fn, this_arg, kv, 3, res)) == RJS_ERR)
            goto end;

        if (!rjs_to_boolean(rt, res)) {
            rjs_value_set_boolean(rt, rv, RJS_FALSE);
            r = RJS_OK;
            goto end;
        }
    }

    rjs_value_set_boolean(rt, rv, RJS_TRUE);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.fill*/
static RJS_NF(TypedArray_prototype_fill)
{
    RJS_Value            *value = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *start = rjs_argument_get(rt, args, argc, 1);
    RJS_Value            *end   = rjs_argument_get(rt, args, argc, 2);
    size_t                top   = rjs_value_stack_save(rt);
    RJS_Value            *v     = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    RJS_Result            r;
    RJS_Number            rel_start, rel_end;
    ssize_t               k, final, len;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;
    
#if ENABLE_BIG_INT
    if (content_is_big_int(iio->type)) {
        if ((r = rjs_to_big_int(rt, value, v)) == RJS_ERR)
            goto end;
    } else
#endif /*ENABLE_BIG_INT*/
    {
        RJS_Number n;

        if ((r = rjs_to_number(rt, value, &n)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, v, n);
    }

    if ((r = rjs_to_integer_or_infinity(rt, start, &rel_start)) == RJS_ERR)
        goto end;

    if (rel_start == -INFINITY)
        k = 0;
    else if (rel_start < 0)
        k = RJS_MAX(rel_start + len, 0);
    else
        k = RJS_MIN(rel_start, len);

    if (rjs_value_is_undefined(rt, end))
        rel_end = len;
    else if ((r = rjs_to_integer_or_infinity(rt, end, &rel_end)) == RJS_ERR)
        goto end;

    if (rel_end == -INFINITY)
        final = 0;
    else if (rel_end < 0)
        final = RJS_MAX(rel_end + len, 0);
    else
        final = RJS_MIN(rel_end, len);

    if (rjs_is_detached_buffer(rt, &iio->buffer)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    for (; k < final; k ++) {
        rjs_set_index(rt, thiz, k, v, RJS_TRUE);
    }

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the constructor from the element type.*/
static RJS_Value*
typed_array_get_constructor (RJS_Runtime *rt, RJS_ArrayElementType type)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Value *c;

    switch (type) {
    case RJS_ARRAY_ELEMENT_UINT8:
        c = rjs_o_Uint8Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT8:
        c = rjs_o_Int8Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT8C:
        c = rjs_o_Uint8ClampedArray(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT16:
        c = rjs_o_Uint16Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT16:
        c = rjs_o_Int16Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_UINT32:
        c = rjs_o_Uint32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_INT32:
        c = rjs_o_Int32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_FLOAT32:
        c = rjs_o_Float32Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_FLOAT64:
        c = rjs_o_Float64Array(realm);
        break;
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGUINT64:
        c = rjs_o_BigUint64Array(realm);
        break;
    case RJS_ARRAY_ELEMENT_BIGINT64:
        c = rjs_o_BigInt64Array(realm);
        break;
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
        c = NULL;
    }

    return c;
}

/*Create the species typed array.*/
static RJS_Result
typed_array_species_create (RJS_Runtime *rt, RJS_Value *exemplar,
        RJS_Value *args, size_t argc, RJS_Value *rv)
{
    size_t                top   = rjs_value_stack_save(rt);
    RJS_Value            *c     = rjs_value_stack_push(rt);
    RJS_Value            *defc  = NULL;
    RJS_IntIndexedObject *iio, *niio;
    RJS_Result            r;

    iio  = (RJS_IntIndexedObject*)rjs_value_get_object(rt, exemplar);
    defc = typed_array_get_constructor(rt, iio->type);

    if ((r = rjs_species_constructor(rt, exemplar, defc, c)) == RJS_ERR)
        goto end;

    if ((r = typed_array_create(rt, c, args, argc, rv)) == RJS_ERR)
        goto end;

    niio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, rv);

    if (content_is_big_int(iio->type) != content_is_big_int(niio->type)) {
        r = rjs_throw_type_error(rt, _("typed arrays' content type mismatch"));
        goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.filter*/
static RJS_NF(TypedArray_prototype_filter)
{
    RJS_Value            *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *vlv      = rjs_value_stack_push(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *seleted  = rjs_value_stack_push(rt);
    RJS_Value            *len      = rjs_value_stack_push(rt);
    RJS_ValueList        *vl;
    RJS_ValueListSegment *vls;
    RJS_IntIndexedObject *iio;
    size_t                captured, i, k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    vl = rjs_value_list_new(rt, vlv);

    rjs_value_copy(rt, o, thiz);

    captured = 0;

    for (k = 0; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, cb_fn, this_arg, kv, 3, seleted)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, seleted)) {
            rjs_value_list_append(rt, vl, kv);
            captured ++;
        }
    }

    rjs_value_set_number(rt, len, captured);
    if ((r = typed_array_species_create(rt, thiz, len, 1, rv)) == RJS_ERR)
        goto end;

    k = 0;
    rjs_list_foreach_c(&vl->seg_list, vls, RJS_ValueListSegment, ln) {
        for (i = 0; i < vls->num; i ++) {
            rjs_set_index(rt, rv, k, &vls->v[i], RJS_TRUE);
            k ++;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.find*/
static RJS_NF(TypedArray_prototype_find)
{
    RJS_Value            *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, o, thiz);

    for (k = 0; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, pred, this_arg, kv, 3, res)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, res)) {
            rjs_value_copy(rt, rv, kv);
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

/*TypedArray.prototype.findIndex*/
static RJS_NF(TypedArray_prototype_findIndex)
{
    RJS_Value            *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, o, thiz);

    for (k = 0; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, pred, this_arg, kv, 3, res)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, res)) {
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

/*TypedArray.prototype.findLast*/
static RJS_NF(TypedArray_prototype_findLast)
{
    RJS_Value            *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    ssize_t               k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, o, thiz);

    for (k = iio->array_length - 1; k >= 0; k --) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, pred, this_arg, kv, 3, res)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, res)) {
            rjs_value_copy(rt, rv, kv);
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

/*TypedArray.prototype.findLastIndex*/
static RJS_NF(TypedArray_prototype_findLastIndex)
{
    RJS_Value            *pred     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    ssize_t               k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, pred)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, o, thiz);

    for (k = iio->array_length - 1; k >= 0; k --) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, pred, this_arg, kv, 3, res)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, res)) {
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

/*TypedArray.prototype.forEach*/
static RJS_NF(TypedArray_prototype_forEach)
{
    RJS_Value            *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, o, thiz);

    for (k = 0; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);

        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, cb_fn, this_arg, kv, 3, res)) == RJS_ERR)
            goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.includes*/
static RJS_NF(TypedArray_prototype_includes)
{
    RJS_Value            *searche  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *form_idx = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *v        = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    RJS_Number            n;
    RJS_Result            r;
    size_t                len, k;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;

    if (len == 0) {
        rjs_value_set_boolean(rt, rv, RJS_FALSE);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_to_integer_or_infinity(rt, form_idx, &n)) == RJS_ERR)
        goto end;

    if (n == INFINITY) {
        rjs_value_set_boolean(rt, rv, RJS_FALSE);
        r = RJS_OK;
        goto end;
    }

    if (n == -INFINITY)
        n = 0;

    if (n >= 0) {
        k = n;
    } else {
        k = RJS_MAX(len + n, 0);
    }

    for (; k < len; k ++) {
        rjs_get_index(rt, thiz, k, v);

        if (rjs_same_value_0(rt, searche, v)) {
            rjs_value_set_boolean(rt, rv, RJS_TRUE);
            r = RJS_OK;
            goto end;
        }
    }

    rjs_value_set_boolean(rt, rv, RJS_FALSE);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.indexOf*/
static RJS_NF(TypedArray_prototype_indexOf)
{
    RJS_Value            *searche  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *form_idx = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *v        = rjs_value_stack_push(rt);
    RJS_Value            *idx      = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    RJS_Number            n;
    RJS_Result            r;
    size_t                len, k;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;

    if (len == 0) {
        rjs_value_set_number(rt, rv, -1);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_to_integer_or_infinity(rt, form_idx, &n)) == RJS_ERR)
        goto end;

    if (n == INFINITY) {
        rjs_value_set_number(rt, rv, -1);
        r = RJS_OK;
        goto end;
    }

    if (n == -INFINITY)
        n = 0;

    if (n >= 0) {
        k = n;
    } else {
        k = RJS_MAX(len + n, 0);
    }

    for (; k < len; k ++) {
        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, key);

        r = rjs_has_property(rt, thiz, key);
        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, key);
            r = rjs_get(rt, thiz, &pn, v);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if (rjs_is_strictly_equal(rt, searche, v)) {
                rjs_value_set_number(rt, rv, k);
                r = RJS_OK;
                goto end;
            }
        }
    }

    rjs_value_set_number(rt, rv, -1);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.join*/
static RJS_NF(TypedArray_prototype_join)
{
    RJS_Value            *separator = rjs_argument_get(rt, args, argc, 0);
    size_t                top       = rjs_value_stack_save(rt);
    RJS_Value            *sep       = rjs_value_stack_push(rt);
    RJS_Value            *kv        = rjs_value_stack_push(rt);
    RJS_Value            *kstr      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    RJS_Result            r;
    size_t                len, k;
    RJS_UCharBuffer       ucb;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;

    if (rjs_value_is_undefined(rt, separator))
        rjs_value_copy(rt, sep, rjs_s_comma(rt));
    else if ((r = rjs_to_string(rt, separator, sep)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        if (k > 0)
            rjs_uchar_buffer_append_string(rt, &ucb, sep);

        rjs_get_index(rt, thiz, k, kv);

        if (!rjs_value_is_undefined(rt, kv)) {
            if ((r = rjs_to_string(rt, kv, kstr)) == RJS_ERR)
                goto end;

            rjs_uchar_buffer_append_string(rt, &ucb, kstr);
        }
    }

    r = rjs_string_from_uchars(rt, rv, ucb.items, ucb.item_num);
end:
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.keys*/
static RJS_NF(TypedArray_prototype_keys)
{
    RJS_Result r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        return r;

    return create_array_iterator(rt, thiz, RJS_ARRAY_ITERATOR_KEY, rv);
}

/*TypedArray.prototype.lastIndexOf*/
static RJS_NF(TypedArray_prototype_lastIndexOf)
{
    RJS_Value            *searche  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *form_idx = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *v        = rjs_value_stack_push(rt);
    RJS_Value            *idx      = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    RJS_Number            n;
    RJS_Result            r;
    ssize_t               len, k;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;

    if (len == 0) {
        rjs_value_set_number(rt, rv, -1);
        r = RJS_OK;
        goto end;
    }

    if (argc > 1) {
        if ((r = rjs_to_integer_or_infinity(rt, form_idx, &n)) == RJS_ERR)
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
        k = RJS_MIN(n, len -1);
    } else {
        k = len + n;
    }

    for (; k >= 0; k --) {
        rjs_value_set_number(rt, idx, k);
        rjs_to_string(rt, idx, key);

        r = rjs_has_property(rt, thiz, key);
        if (r) {
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, key);
            r = rjs_get(rt, thiz, &pn, v);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;

            if (rjs_is_strictly_equal(rt, searche, v)) {
                rjs_value_set_number(rt, rv, k);
                r = RJS_OK;
                goto end;
            }
        }
    }

    rjs_value_set_number(rt, rv, -1);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.map*/
static RJS_NF(TypedArray_prototype_map)
{
    RJS_Value            *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *mappedv  = rjs_value_stack_push(rt);
    RJS_Value            *len      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_set_number(rt, len, iio->array_length);
    if ((r = typed_array_species_create(rt, thiz, len, 1, rv)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, o, thiz);

    for (k = 0; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, cb_fn, this_arg, kv, 3, mappedv)) == RJS_ERR)
            goto end;

        if ((r = rjs_set_index(rt, rv, k, mappedv, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.reduce*/
static RJS_NF(TypedArray_prototype_reduce)
{
    RJS_Value            *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *initv    = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *accv     = rjs_value_stack_push(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    if ((iio->array_length == 0) && (argc < 2)) {
        r = rjs_throw_type_error(rt, _("initial value is not present"));
        goto end;
    }

    k = 0;

    if (argc >= 2) {
        rjs_value_copy(rt, accv, initv);
    } else {
        rjs_get_index(rt, thiz, k, accv);
        k ++;
    }

    rjs_value_copy(rt, o, thiz);

    for (; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, cb_fn, rjs_v_undefined(rt), accv, 4, res)) == RJS_ERR)
            goto end;

        rjs_value_copy(rt, accv, res);
    }

    rjs_value_copy(rt, rv, accv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.reduceRight*/
static RJS_NF(TypedArray_prototype_reduceRight)
{
    RJS_Value            *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *initv    = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *accv     = rjs_value_stack_push(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    ssize_t               k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    if ((iio->array_length == 0) && (argc < 2)) {
        r = rjs_throw_type_error(rt, _("initial value is not present"));
        goto end;
    }

    k = iio->array_length - 1;

    if (argc >= 2) {
        rjs_value_copy(rt, accv, initv);
    } else {
        rjs_get_index(rt, thiz, k, accv);
        k --;
    }

    rjs_value_copy(rt, o, thiz);

    for (; k >= 0; k --) {
        rjs_get_index(rt, thiz, k, kv);
        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, cb_fn, rjs_v_undefined(rt), accv, 4, res)) == RJS_ERR)
            goto end;

        rjs_value_copy(rt, accv, res);
    }

    rjs_value_copy(rt, rv, accv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.reverse*/
static RJS_NF(TypedArray_prototype_reverse)
{
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *lowerv   = rjs_value_stack_push(rt);
    RJS_Value            *upperv   = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                len, mid, lower, upper;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio   = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len   = iio->array_length;
    mid   = len / 2;
    lower = 0;

    while (lower != mid) {
        upper = len - lower - 1;

        rjs_get_index(rt, thiz, lower, lowerv);
        rjs_get_index(rt, thiz, upper, upperv);

        rjs_set_index(rt, thiz, lower, upperv, RJS_TRUE);
        rjs_set_index(rt, thiz, upper, lowerv, RJS_TRUE);

        lower ++;
    }

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create a typed array from another typed array.*/
static RJS_Result
set_typed_array_from_typed_array (RJS_Runtime *rt, RJS_Value *target, RJS_Number offset, RJS_Value *source)
{
    size_t                top = rjs_value_stack_save(rt);
    RJS_Value            *nab = rjs_value_stack_push(rt);
    RJS_Value            *ev  = rjs_value_stack_push(rt);
    RJS_Result            r;
    RJS_IntIndexedObject *tiio, *siio;
    RJS_ArrayElementType  ttype, stype;
    size_t                tesize, sesize, tlen, slen, tboff, sboff, tbidx, sbidx, limit;
    RJS_Value            *tbuf, *sbuf;
    RJS_Bool              same;

    tiio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, target);
    tbuf = &tiio->buffer;
    if (rjs_is_detached_buffer(rt, tbuf)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    siio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, source);
    sbuf = &siio->buffer;
    if (rjs_is_detached_buffer(rt, sbuf)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    ttype  = tiio->type;
    tesize = rjs_typed_array_element_size(ttype);
    tboff  = tiio->byte_offset;
    tlen   = tiio->array_length;

    stype  = siio->type;
    sesize = rjs_typed_array_element_size(stype);
    sboff  = siio->byte_offset;
    slen   = siio->array_length;

    if ((offset == INFINITY) || (slen + offset > tlen)) {
        r = rjs_throw_range_error(rt, _("target offset is out of range"));
        goto end;
    }

    if (content_is_big_int(ttype) != content_is_big_int(stype)) {
        r = rjs_throw_type_error(rt, _("typed arrays' content type mismatch"));
        goto end;
    }

    if ((rjs_value_get_gc_thing_type(rt, &tiio->buffer) == RJS_GC_THING_ARRAY_BUFFER)
            && (rjs_value_get_gc_thing_type(rt, &siio->buffer) == RJS_GC_THING_ARRAY_BUFFER)) {
        RJS_ArrayBuffer *sab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &siio->buffer);
        RJS_ArrayBuffer *tab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &tiio->buffer);

        same = sab->data_block == tab->data_block;
    } else {
        same = rjs_same_value(rt, &siio->buffer, &tiio->buffer);
    }

    if (same) {
        if ((r = rjs_clone_array_buffer(rt, sbuf, sboff, siio->byte_length, nab)) == RJS_ERR)
            goto end;

        sbuf  = nab;
        sbidx = 0;
    } else {
        sbidx = sboff;
    }

    tbidx = ((size_t)offset) * tesize + tboff;
    limit = tbidx + tesize * slen;

    if (stype == ttype) {
        RJS_ArrayBuffer *sab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, sbuf);
        RJS_ArrayBuffer *tab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &tiio->buffer);

        rjs_array_buffer_lock(rt, sbuf);
        rjs_array_buffer_lock(rt, &tiio->buffer);

        memcpy(rjs_data_block_get_buffer(tab->data_block) + tbidx,
                rjs_data_block_get_buffer(sab->data_block) + sbidx,
                limit - tbidx);

        rjs_array_buffer_unlock(rt, &tiio->buffer);
        rjs_array_buffer_unlock(rt, sbuf);
    } else {
        while (tbidx < limit) {
            rjs_get_value_from_buffer(rt, sbuf, sbidx, stype, rjs_is_little_endian(), ev);
            rjs_set_value_in_buffer(rt, tbuf, tbidx, ttype, ev, rjs_is_little_endian());

            sbidx += sesize;
            tbidx += tesize;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create a typed array from an array like object.*/
static RJS_Result
set_typed_array_from_array_like (RJS_Runtime *rt, RJS_Value *target, RJS_Number offset, RJS_Value *source)
{
    size_t                top = rjs_value_stack_save(rt);
    RJS_Value            *src = rjs_value_stack_push(rt);
    RJS_Value            *ev  = rjs_value_stack_push(rt);
    RJS_Result            r;
    RJS_IntIndexedObject *tiio;
    RJS_Value            *tbuf;
    size_t                tlen, toff, k;
    int64_t               slen;

    tiio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, target);
    tbuf = &tiio->buffer;
    if (rjs_is_detached_buffer(rt, tbuf)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    tlen = tiio->array_length;

    if ((r = rjs_to_object(rt, source, src)) == RJS_ERR)
        goto end;

    if ((r = rjs_length_of_array_like(rt, src, &slen)) == RJS_ERR)
        goto end;

    if ((offset == INFINITY) || (slen + offset > tlen)) {
        r = rjs_throw_range_error(rt, _("target offset is out of range"));
        goto end;
    }
    toff = offset;

    for (k = 0; k < slen; k ++) {
        if ((r = rjs_get_index(rt, src, k, ev)) == RJS_ERR)
            goto end;

        if ((r = rjs_int_indexed_element_set(rt, target, k + toff, ev)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.set*/
static RJS_NF(TypedArray_prototype_set)
{
    RJS_Value  *source = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *offset = rjs_argument_get(rt, args, argc, 1);
    RJS_Number  target_offset;
    RJS_Result  r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_INT_INDEXED_OBJECT) {
        r = rjs_throw_type_error(rt, _("the value is not a typed array"));
        goto end;
    }

    if ((r = rjs_to_integer_or_infinity(rt, offset, &target_offset)) == RJS_ERR)
        goto end;

    if (target_offset < 0) {
        r = rjs_throw_range_error(rt, _("offset must >= 0"));
        goto end;
    }

    if (rjs_value_get_gc_thing_type(rt, source) == RJS_GC_THING_INT_INDEXED_OBJECT) {
        r = set_typed_array_from_typed_array(rt, thiz, target_offset, source);
    } else {
        r = set_typed_array_from_array_like(rt, thiz, target_offset, source);
    }

    if (r == RJS_OK)
        rjs_value_set_undefined(rt, rv);
end:
    return r;
}

/*TypedArray.prototype.slice*/
static RJS_NF(TypedArray_prototype_slice)
{
    RJS_Value            *start = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *end   = rjs_argument_get(rt, args, argc, 1);
    size_t                top   = rjs_value_stack_save(rt);
    RJS_Value            *kv    = rjs_value_stack_push(rt);
    RJS_Value            *lenv  = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *siio, *tiio;
    RJS_Number            rel_start, rel_end;
    ssize_t               len, k, final, count, n;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    siio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = siio->array_length;

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

    if (rel_end == -INFINITY)
        final = 0;
    else if (rel_end < 0)
        final = RJS_MAX(len + rel_end, 0);
    else
        final = RJS_MIN(rel_end, len);

    count = RJS_MAX(0, final - k);

    rjs_value_set_number(rt, lenv, count);
    if ((r = typed_array_species_create(rt, thiz, lenv, 1, rv)) == RJS_ERR)
        goto end;

    if (count > 0) {
        RJS_ArrayElementType stype, ttype;

        if (rjs_is_detached_buffer(rt, &siio->buffer)) {
            r = rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        tiio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, rv);

        stype = siio->type;
        ttype = tiio->type;

        if (stype != ttype) {
            n = 0;

            while (k < final) {
                rjs_get_index(rt, thiz, k, kv);
                rjs_set_index(rt, rv, n, kv, RJS_TRUE);

                k ++;
                n ++;
            }
        } else {
            RJS_ArrayBuffer *sab, *tab;
            size_t           esize;

            sab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &siio->buffer);
            tab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &tiio->buffer);

            esize = rjs_typed_array_element_size(siio->type);

            rjs_array_buffer_lock(rt, &siio->buffer);
            rjs_array_buffer_lock(rt, &tiio->buffer);

            memcpy(rjs_data_block_get_buffer(tab->data_block) + tiio->byte_offset,
                    rjs_data_block_get_buffer(sab->data_block) + siio->byte_offset + k * esize,
                    count * esize);

            rjs_array_buffer_unlock(rt, &tiio->buffer);
            rjs_array_buffer_unlock(rt, &siio->buffer);
        }
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.some*/
static RJS_NF(TypedArray_prototype_some)
{
    RJS_Value            *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t                top      = rjs_value_stack_save(rt);
    RJS_Value            *kv       = rjs_value_stack_push(rt);
    RJS_Value            *key      = rjs_value_stack_push(rt);
    RJS_Value            *o        = rjs_value_stack_push(rt);
    RJS_Value            *res      = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, o, thiz);

    for (k = 0; k < iio->array_length; k ++) {
        rjs_get_index(rt, thiz, k, kv);

        rjs_value_set_number(rt, key, k);

        if ((r = rjs_call(rt, cb_fn, this_arg, kv, 3, res)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, res)) {
            rjs_value_set_boolean(rt, rv, RJS_TRUE);
            r = RJS_OK;
            goto end;
        }
    }

    rjs_value_set_boolean(rt, rv, RJS_FALSE);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**Typed array elements compare parameters.*/
typedef struct {
    RJS_Runtime           *rt; /**< The current runtime.*/
    RJS_ArrayElementType type;  /**< Element type.*/
    RJS_Value           *cmp;   /**< The compare function.*/
} RJS_TypedArrayCmpParams;

/*Typed array element compare function.*/
static RJS_CompareResult
typed_array_element_cmp (const void *p1, const void *p2, void *data)
{
    RJS_TypedArrayCmpParams *params = data;
    RJS_Runtime             *rt     = params->rt;
    size_t                   top    = rjs_value_stack_save(rt);
    RJS_Value               *x      = rjs_value_stack_push(rt);
    RJS_Value               *y      = rjs_value_stack_push(rt);
    RJS_Value               *res    = rjs_value_stack_push(rt);
    RJS_Number               n;
    RJS_Result               r;

    rjs_get_value_from_raw(rt, p1, params->type, rjs_is_little_endian(), x);
    rjs_get_value_from_raw(rt, p2, params->type, rjs_is_little_endian(), y);

    if (!rjs_value_is_undefined(rt, params->cmp)) {
        if ((r = rjs_call(rt, params->cmp, rjs_v_undefined(rt), x, 2, res)) == RJS_ERR)
            goto end;

        if ((r = rjs_to_number(rt, res, &n)) == RJS_ERR)
            goto end;

        if (isnan(n))
            n = 0;

        if (n < 0)
            r = RJS_COMPARE_LESS;
        else if (n > 0)
            r = RJS_COMPARE_GREATER;
        else
            r = RJS_COMPARE_EQUAL;
    }
#if ENABLE_BIG_INT
    else if (content_is_big_int(params->type)) {
        r = rjs_big_int_compare(rt, x, y);
    }
#endif /*ENABLE_BIG_INT*/
    else {
        RJS_Number nx, ny;

        nx = rjs_value_get_number(rt, x);
        ny = rjs_value_get_number(rt, y);

        if (isnan(nx) && isnan(ny))
            r = RJS_COMPARE_EQUAL;
        else if (isnan(nx))
            r = RJS_COMPARE_GREATER;
        else if (isnan(ny))
            r = RJS_COMPARE_LESS;
        else if (nx < ny)
            r = RJS_COMPARE_LESS;
        else if (nx > ny)
            r = RJS_COMPARE_GREATER;
        else if (signbit(nx) && !signbit(ny))
            r = RJS_COMPARE_LESS;
        else if (!signbit(nx) && signbit(ny))
            r = RJS_COMPARE_GREATER;
        else
            r = RJS_COMPARE_EQUAL;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.sort*/
static RJS_NF(TypedArray_prototype_sort)
{
    RJS_Value              *cmp_fn = rjs_argument_get(rt, args, argc, 0);
    RJS_IntIndexedObject   *iio;
    RJS_ArrayBuffer        *ab;
    RJS_DataBlock          *db;
    size_t                  esize;
    RJS_TypedArrayCmpParams params; 
    RJS_Result              r;

    if (!rjs_value_is_undefined(rt, cmp_fn) && !rjs_is_callable(rt, cmp_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    ab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &iio->buffer);

    esize = rjs_typed_array_element_size(iio->type);

    params.rt    = rt;
    params.type  = iio->type;
    params.cmp   = cmp_fn;

    db = ab->data_block;

    rjs_data_block_ref(db);

    r = rjs_sort(rjs_data_block_get_buffer(db) + iio->byte_offset,
            iio->array_length, esize, typed_array_element_cmp, &params);

    rjs_data_block_unref(db);

    if (r == RJS_OK)
        rjs_value_copy(rt, rv, thiz);
end:
    return r;
}

/*TypedArray.prototype.subarray*/
static RJS_NF(TypedArray_prototype_subarray)
{
    RJS_Value            *begin = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *end   = rjs_argument_get(rt, args, argc, 1);
    size_t                top   = rjs_value_stack_save(rt);
    RJS_Value            *buf   = rjs_value_stack_push(rt);
    RJS_Value            *boff  = rjs_value_stack_push(rt);
    RJS_Value            *nlen  = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    ssize_t               slen, begin_idx, end_idx, esize;
    RJS_Number            rel_begin, rel_end;
    RJS_Result            r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_INT_INDEXED_OBJECT) {
        r = rjs_throw_type_error(rt, _("the value is not a typed array"));
        goto end;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, buf, &iio->buffer);

    slen = iio->array_length;

    if ((r = rjs_to_integer_or_infinity(rt, begin, &rel_begin)) == RJS_ERR)
        goto end;

    if (rel_begin == -INFINITY)
        begin_idx = 0;
    else if (rel_begin < 0)
        begin_idx = RJS_MAX(rel_begin + slen, 0);
    else
        begin_idx = RJS_MIN(rel_begin, slen);

    if (rjs_value_is_undefined(rt, end))
        rel_end = slen;
    else if ((r = rjs_to_integer_or_infinity(rt, end, &rel_end)) == RJS_ERR)
        goto end;

    if (rel_end == -INFINITY)
        end_idx = 0;
    else if (rel_end < 0)
        end_idx = RJS_MAX(rel_end + slen, 0);
    else
        end_idx = RJS_MIN(rel_end, slen);

    esize = rjs_typed_array_element_size(iio->type);

    rjs_value_set_number(rt, boff, iio->byte_offset + esize * begin_idx);
    rjs_value_set_number(rt, nlen, RJS_MAX(end_idx - begin_idx, 0));

    r = typed_array_species_create(rt, thiz, buf, 3, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.toLocaleString*/
static RJS_NF(TypedArray_prototype_toLocaleString)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *elem = rjs_value_stack_push(rt);
    RJS_Value *er   = rjs_value_stack_push(rt);
    RJS_Value *es   = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    RJS_UCharBuffer       ucb;
    int64_t    len, k;
    RJS_Result r;

    rjs_uchar_buffer_init(rt, &ucb);

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;

    for (k = 0; k < len; k ++) {
        if (k > 0)
            rjs_uchar_buffer_append_string(rt, &ucb, rjs_s_comma(rt));

        if ((r = rjs_get_index(rt, thiz, k, elem)) == RJS_ERR)
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

/*Create a typed array in same type.*/
static RJS_Result
typed_array_create_same_type (RJS_Runtime *rt, RJS_Value *e, RJS_Value *args,
        size_t argc, RJS_Value *a)
{
    RJS_IntIndexedObject *iio;
    RJS_Value            *c;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, e);
    c   = typed_array_get_constructor(rt, iio->type);

    return typed_array_create(rt, c, args, argc, a);
}

/*TypedArray.prototype.toReversed*/
static RJS_NF(TypedArray_prototype_toReversed)
{
    size_t                top  = rjs_value_stack_save(rt);
    RJS_Value            *v    = rjs_value_stack_push(rt);
    RJS_Value            *lenv = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                len, k;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len = iio->array_length;

    rjs_value_set_number(rt, lenv, len);

    if ((r = typed_array_create_same_type(rt, thiz, lenv, 1, rv)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        rjs_get_index(rt, thiz, len - k - 1, v);
        rjs_set_index(rt, rv, k, v, RJS_TRUE);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.toSorted*/
static RJS_NF(TypedArray_prototype_toSorted)
{
    RJS_Value              *cmp  = rjs_argument_get(rt, args, argc, 0);
    size_t                  top  = rjs_value_stack_save(rt);
    RJS_Value              *lenv = rjs_value_stack_push(rt);
    RJS_IntIndexedObject   *oiio, *niio;
    RJS_ArrayBuffer        *oab, *nab;
    uint8_t                *obuf, *nbuf;
    size_t                  len, esize;
    RJS_TypedArrayCmpParams params; 
    RJS_Result              r;

    if (!rjs_value_is_undefined(rt, cmp) && !rjs_is_callable(rt, cmp)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    oiio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len  = oiio->array_length;

    rjs_value_set_number(rt, lenv, len);

    if ((r = typed_array_create_same_type(rt, thiz, lenv, 1, rv)) == RJS_ERR)
        goto end;

    if (len) {
        oab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &oiio->buffer);
        obuf = rjs_data_block_get_buffer(oab->data_block);
        niio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, rv);
        nab  = (RJS_ArrayBuffer*)rjs_value_get_object(rt, &niio->buffer);
        nbuf = rjs_data_block_get_buffer(nab->data_block);

        esize = rjs_typed_array_element_size(niio->type);

        memcpy(nbuf + niio->byte_offset, obuf + oiio->byte_offset, esize * len);

        params.rt    = rt;
        params.type  = niio->type;
        params.cmp   = cmp;

        r = rjs_sort(nbuf + niio->byte_offset, niio->array_length, esize,
                typed_array_element_cmp, &params);
    } else {
        r = RJS_OK;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*TypedArray.prototype.values*/
static RJS_NF(TypedArray_prototype_values)
{
    RJS_Result r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        return r;

    return create_array_iterator(rt, thiz, RJS_ARRAY_ITERATOR_VALUE, rv);
}

/*TypedArray.prototype.with*/
static RJS_NF(TypedArray_prototype_with)
{
    RJS_Value            *index = rjs_argument_get(rt, args, argc, 0);
    RJS_Value            *value = rjs_argument_get(rt, args, argc, 1);
    size_t                top   = rjs_value_stack_save(rt);
    RJS_Value            *nv    = rjs_value_stack_push(rt);
    RJS_Value            *lenv  = rjs_value_stack_push(rt);
    RJS_Value            *fv    = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    size_t                len, k, real_idx;
    RJS_Number            rel_index, act_index;
    RJS_Result            r;

    if ((r = valid_typed_array(rt, thiz)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);
    len  = iio->array_length;

    if ((r = rjs_to_integer_or_infinity(rt, index, &rel_index)) == RJS_ERR)
        goto end;

    if (rel_index >= 0)
        act_index = rel_index;
    else
        act_index = len + rel_index;

#if ENABLE_BIG_INT
    if (content_is_big_int(iio->type)) {
        if ((r = rjs_to_big_int(rt, value, nv)) == RJS_ERR)
            goto end;
    } else
#endif /*ENABLE_BIG_INT*/
    {
        RJS_Number n;

        if ((r = rjs_to_number(rt, value, &n)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, nv, n);
    }

    if (!rjs_is_valid_int_index(rt, thiz, act_index, &real_idx)) {
        r = rjs_throw_range_error(rt, _("the index is not valid"));
        goto end;
    }

    rjs_value_set_number(rt, lenv, len);
    if ((r = typed_array_create_same_type(rt, thiz, lenv, 1, rv)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        if (k == real_idx)
            rjs_value_copy(rt, fv, nv);
        else
            rjs_get_index(rt, thiz, k, fv);

        rjs_set_index(rt, rv, k, fv, RJS_TRUE);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
typed_array_prototype_function_descs[] = {
    {
        "at",
        1,
        TypedArray_prototype_at
    },
    {
        "copyWithin",
        2,
        TypedArray_prototype_copyWithin
    },
    {
        "entries",
        0,
        TypedArray_prototype_entries
    },
    {
        "every",
        1,
        TypedArray_prototype_every
    },
    {
        "fill",
        1,
        TypedArray_prototype_fill
    },
    {
        "filter",
        1,
        TypedArray_prototype_filter
    },
    {
        "find",
        1,
        TypedArray_prototype_find
    },
    {
        "findIndex",
        1,
        TypedArray_prototype_findIndex
    },
    {
        "findLast",
        1,
        TypedArray_prototype_findLast
    },
    {
        "findLastIndex",
        1,
        TypedArray_prototype_findLastIndex
    },
    {
        "forEach",
        1,
        TypedArray_prototype_forEach
    },
    {
        "includes",
        1,
        TypedArray_prototype_includes
    },
    {
        "indexOf",
        1,
        TypedArray_prototype_indexOf
    },
    {
        "join",
        1,
        TypedArray_prototype_join
    },
    {
        "keys",
        0,
        TypedArray_prototype_keys
    },
    {
        "lastIndexOf",
        1,
        TypedArray_prototype_lastIndexOf
    },
    {
        "map",
        1,
        TypedArray_prototype_map
    },
    {
        "reduce",
        1,
        TypedArray_prototype_reduce
    },
    {
        "reduceRight",
        1,
        TypedArray_prototype_reduceRight
    },
    {
        "reverse",
        0,
        TypedArray_prototype_reverse
    },
    {
        "set",
        1,
        TypedArray_prototype_set
    },
    {
        "slice",
        2,
        TypedArray_prototype_slice
    },
    {
        "some",
        1,
        TypedArray_prototype_some
    },
    {
        "sort",
        1,
        TypedArray_prototype_sort
    },
    {
        "subarray",
        2,
        TypedArray_prototype_subarray
    },
    {
        "toLocaleString",
        0,
        TypedArray_prototype_toLocaleString
    },
    {
        "toReversed",
        0,
        TypedArray_prototype_toReversed
    },
    {
        "toSorted",
        1,
        TypedArray_prototype_toSorted
    },
    {
        "toString",
        0,
        NULL,
        "Array_prototype_toString"
    },
    {
        "values",
        0,
        TypedArray_prototype_values,
        "TypedArray_prototype_values"
    },
    {
        "with",
        2,
        TypedArray_prototype_with
    },
    {
        "@@iterator",
        0,
        NULL,
        "TypedArray_prototype_values"
    },
    {NULL}
};

/*get %TypedArray%.prototype.buffer*/
static RJS_NF(TypedArray_prototype_buffer_get)
{
    RJS_IntIndexedObject *iio;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_INT_INDEXED_OBJECT)
        return rjs_throw_type_error(rt, _("the value is not a TypedArray"));

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, rv, &iio->buffer);
    return RJS_OK;
}

/*get %TypedArray%.prototype.byteLength*/
static RJS_NF(TypedArray_prototype_byteLength_get)
{
    RJS_IntIndexedObject *iio;
    size_t                l;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_INT_INDEXED_OBJECT)
        return rjs_throw_type_error(rt, _("the value is not a TypedArray"));

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    if (rjs_is_detached_buffer(rt, &iio->buffer))
        l = 0;
    else
        l = iio->byte_length;

    rjs_value_set_number(rt, rv, l);
    return RJS_OK;
}

/*get %TypedArray%.prototype.byteOffset*/
static RJS_NF(TypedArray_prototype_byteOffset_get)
{
    RJS_IntIndexedObject *iio;
    size_t                l;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_INT_INDEXED_OBJECT)
        return rjs_throw_type_error(rt, _("the value is not a TypedArray"));

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    if (rjs_is_detached_buffer(rt, &iio->buffer))
        l = 0;
    else
        l = iio->byte_offset;

    rjs_value_set_number(rt, rv, l);
    return RJS_OK;
}

/*get %TypedArray%.prototype.length*/
static RJS_NF(TypedArray_prototype_length_get)
{
    RJS_IntIndexedObject *iio;
    size_t                l;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_INT_INDEXED_OBJECT)
        return rjs_throw_type_error(rt, _("the value is not a TypedArray"));

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    if (rjs_is_detached_buffer(rt, &iio->buffer))
        l = 0;
    else
        l = iio->array_length;

    rjs_value_set_number(rt, rv, l);
    return RJS_OK;
}

/*get %TypedArray%.prototype.toStringTag*/
static RJS_NF(TypedArray_prototype_toStringTag_get)
{
    RJS_IntIndexedObject *iio;
    char                 *name;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_INT_INDEXED_OBJECT) {
        rjs_value_set_undefined(rt, rv);
        return RJS_OK;
    }

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, thiz);

    switch (iio->type) {
    case RJS_ARRAY_ELEMENT_UINT8:
        name = "Uint8Array";
        break;
    case RJS_ARRAY_ELEMENT_INT8:
        name = "Int8Array";
        break;
    case RJS_ARRAY_ELEMENT_UINT8C:
        name = "Uint8ClampedArray";
        break;
    case RJS_ARRAY_ELEMENT_UINT16:
        name = "Uint16Array";
        break;
    case RJS_ARRAY_ELEMENT_INT16:
        name = "Int16Array";
        break;
    case RJS_ARRAY_ELEMENT_UINT32:
        name = "Uint32Array";
        break;
    case RJS_ARRAY_ELEMENT_INT32:
        name = "Int32Array";
        break;
    case RJS_ARRAY_ELEMENT_FLOAT32:
        name = "Float32Array";
        break;
    case RJS_ARRAY_ELEMENT_FLOAT64:
        name = "Float64Array";
        break;
#if ENABLE_BIG_INT
    case RJS_ARRAY_ELEMENT_BIGUINT64:
        name = "BigUint64Array";
        break;
    case RJS_ARRAY_ELEMENT_BIGINT64:
        name = "BigInt64Array";
        break;
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
        name = NULL;
    }

    return rjs_string_from_chars(rt, rv, name, -1);
}

static const RJS_BuiltinAccessorDesc
typed_array_prototype_accessor_descs[] = {
    {
        "buffer",
        TypedArray_prototype_buffer_get
    },
    {
        "byteLength",
        TypedArray_prototype_byteLength_get
    },
    {
        "byteOffset",
        TypedArray_prototype_byteOffset_get
    },
    {
        "length",
        TypedArray_prototype_length_get
    },
    {
        "@@toStringTag",
        TypedArray_prototype_toStringTag_get
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
typed_array_prototype_desc = {
    "TypedArray",
    NULL,
    NULL,
    NULL,
    NULL,
    typed_array_prototype_function_descs,
    typed_array_prototype_accessor_descs,
    NULL,
    "TypedArray_prototype"
};

/*Allocate a typed array buffer.*/
static RJS_Result
allocate_typed_array_buffer (RJS_Runtime *rt, RJS_Value *o, int64_t len)
{
    RJS_Realm            *realm = rjs_realm_current(rt);
    RJS_IntIndexedObject *iio;
    size_t                esize, dlen;
    RJS_Result            r;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);

    assert(rjs_value_is_undefined(rt, &iio->buffer));

    esize = rjs_typed_array_element_size(iio->type);
    dlen  = esize * len;

    if ((r = rjs_allocate_array_buffer(rt, rjs_o_ArrayBuffer(realm), dlen, &iio->buffer)) == RJS_ERR)
        return r;

    iio->byte_offset  = 0;
    iio->byte_length  = dlen;
    iio->array_length = len;

    return RJS_OK;
}

/*Allocate a new typed array.*/
static RJS_Result
allocate_typed_array (RJS_Runtime *rt, RJS_ArrayElementType etype, RJS_Value *nt,
        int dp_idx, int64_t len, RJS_Value *rv)
{
    size_t                top   = rjs_value_stack_save(rt);
    RJS_Value            *proto = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio;
    RJS_Result            r;

    if ((r = rjs_get_prototype_from_constructor(rt, nt, dp_idx, proto)) == RJS_ERR)
        goto end;

    if ((r = rjs_int_indexed_object_create(rt, proto, rv)) == RJS_ERR)
        goto end;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, rv);

    iio->type = etype;

    if (len != -1) {
        if ((r = allocate_typed_array_buffer(rt, rv, len)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Initialize a typed array from another one.*/
static RJS_Result
initialize_typed_array_from_typed_array (RJS_Runtime *rt, RJS_Value *o, RJS_Value *src)
{
    RJS_Realm            *realm  = rjs_realm_current(rt);
    size_t                top    = rjs_value_stack_save(rt);
    RJS_Value            *data   = rjs_value_stack_push(rt);
    RJS_Value            *item   = rjs_value_stack_push(rt);
    RJS_IntIndexedObject *iio, *siio;
    RJS_Value            *sdata;
    RJS_ArrayElementType  stype, etype;
    size_t                sesize, esize, sboff, elen, blen;
    RJS_Result            r;

    iio  = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);
    siio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, src);

    etype   = iio->type;
    esize   = rjs_typed_array_element_size(etype);

    sdata   = &siio->buffer;
    stype   = siio->type;
    sesize  = rjs_typed_array_element_size(stype);
    sboff   = siio->byte_offset;
    elen    = siio->array_length;
    blen    = esize * elen;

    if (etype == stype) {
        if ((r = rjs_clone_array_buffer(rt, sdata, sboff, blen, data)) == RJS_ERR)
            goto end;
    } else {
        size_t sbidx, tbidx, count;

        if ((r = rjs_allocate_array_buffer(rt, rjs_o_ArrayBuffer(realm), blen, data)) == RJS_ERR)
            goto end;

        if (rjs_is_detached_buffer(rt, sdata)) {
            r = rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        if (content_is_big_int(etype) != content_is_big_int(stype)) {
            r = rjs_throw_type_error(rt, _("array buffers' content type mismatch"));
            goto end;
        }

        sbidx = sboff;
        tbidx = 0;
        count = elen;

        while (count > 0) {
            rjs_get_value_from_buffer(rt, sdata, sbidx, stype, rjs_is_little_endian(), item);
            rjs_set_value_in_buffer(rt, data, tbidx, etype, item, rjs_is_little_endian());

            sbidx += sesize;
            tbidx += esize;
            count --;
        }
    }

    rjs_value_copy(rt, &iio->buffer, data);

    iio->byte_offset  = 0;
    iio->byte_length  = blen;
    iio->array_length = elen;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Initialize a typed array from an array buffer.*/
static RJS_Result
initialize_typed_array_from_array_buffer (RJS_Runtime *rt, RJS_Value *o, RJS_Value *buf, RJS_Value *boff, RJS_Value *length)
{
    size_t                top    = rjs_value_stack_save(rt);
    RJS_IntIndexedObject *iio;
    RJS_ArrayBuffer      *ab;
    size_t                esize;
    int64_t               off, nlen, buf_blen, new_blen;
    RJS_Result            r;

    iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);

    esize = rjs_typed_array_element_size(iio->type);

    if ((r = rjs_to_index(rt, boff, &off)) == RJS_ERR)
        goto end;

    if (off % esize) {
        r = rjs_throw_range_error(rt, _("offset is not aligned"));
        goto end;
    }

    if (!rjs_value_is_undefined(rt, length)) {
        if ((r = rjs_to_index(rt, length, &nlen)) == RJS_ERR)
            goto end;
    }

    if (rjs_is_detached_buffer(rt, buf)) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    ab = (RJS_ArrayBuffer*)rjs_value_get_object(rt, buf);

    buf_blen = ab->byte_length;

    if (rjs_value_is_undefined(rt, length)) {
        if (buf_blen % esize) {
            r = rjs_throw_range_error(rt, _("array buffer length is not aligned"));
            goto end;
        }

        new_blen = buf_blen - off;
        if (new_blen < 0) {
            r = rjs_throw_range_error(rt, _("array buffer length must >= 0"));
            goto end;
        }
    } else {
        new_blen = nlen * esize;

        if (off + new_blen > buf_blen) {
            r = rjs_throw_range_error(rt, _("array buffer length overflow"));
            goto end;
        }
    }

    rjs_value_copy(rt, &iio->buffer, buf);

    iio->byte_offset  = off;
    iio->byte_length  = new_blen;
    iio->array_length = new_blen / esize;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Initialize a typed array from an iterator.*/
static RJS_Result
initialize_typed_array_from_iterator (RJS_Runtime *rt, RJS_Value *o, RJS_Value *src, RJS_Value *method)
{
    size_t                top = rjs_value_stack_save(rt);
    RJS_Value            *vlv = rjs_value_stack_push(rt);
    RJS_Value            *kv  = rjs_value_stack_push(rt);
    RJS_Value            *ir  = rjs_value_stack_push(rt);
    RJS_ValueList        *vl;
    RJS_ValueListSegment *vls;
    RJS_Iterator          iter;
    int64_t               len, i, k;
    RJS_Result            r;

    rjs_iterator_init(rt, &iter);

    if ((r = rjs_get_iterator(rt, src, RJS_ITERATOR_SYNC, method, &iter)) == RJS_ERR)
        goto end;

    vl  = rjs_value_list_new(rt, vlv);
    len = 0;

    while (1) {
        if ((r = rjs_iterator_step(rt, &iter, ir)) == RJS_ERR)
            goto end;

        if (!r)
            break;

        if ((r = rjs_iterator_value(rt, ir, kv)) == RJS_ERR)
            goto end;

        rjs_value_list_append(rt, vl, kv);
        len ++;
    }

    if ((r = allocate_typed_array_buffer(rt, o, len)) == RJS_ERR)
        goto end;

    k = 0;
    rjs_list_foreach_c(&vl->seg_list, vls, RJS_ValueListSegment, ln) {
        for (i = 0; i < vls->num; i ++) {
            if ((r = rjs_set_index(rt, o, k, &vls->v[i], RJS_TRUE)) == RJS_ERR)
                goto end;

            k ++;
        }
    }

    r = RJS_OK;
end:
    rjs_iterator_deinit(rt, &iter);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Initialize a typed array from am array like object.*/
static RJS_Result
initialize_typed_array_from_arrray_like (RJS_Runtime *rt, RJS_Value *o, RJS_Value *src)
{
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *kv  = rjs_value_stack_push(rt);
    int64_t     len, k;
    RJS_Result  r;

    if ((r = rjs_length_of_array_like(rt, src, &len)) == RJS_ERR)
        goto end;

    if ((r = allocate_typed_array_buffer(rt, o, len)) == RJS_ERR)
        goto end;

    for (k = 0; k < len; k ++) {
        if ((r = rjs_get_index(rt, src, k, kv)) == RJS_ERR)
            goto end;

        if ((r = rjs_set_index(rt, o, k, kv, RJS_TRUE)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*The typed array constructor.*/
static RJS_Result
typedef_array_constructor (RJS_Runtime *rt, RJS_Value *args, size_t argc, RJS_Value *nt,
        RJS_ArrayElementType etype, int proto_idx, RJS_Value *rv)
{
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *use_iter = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!nt) {
        r = rjs_throw_type_error(rt, _("the function must be used as a constructor"));
        goto end;
    }

    if (argc == 0) {
        r = allocate_typed_array(rt, etype, nt, proto_idx, 0, rv);
    } else {
        RJS_Value *first_arg = rjs_value_buffer_item(rt, args, 0);

        if (rjs_value_is_object(rt, first_arg)) {
            RJS_GcThingType gtt = rjs_value_get_gc_thing_type(rt, first_arg);

            if ((r = allocate_typed_array(rt, etype, nt, proto_idx, -1, rv)) == RJS_ERR)
                goto end;

            if (gtt == RJS_GC_THING_INT_INDEXED_OBJECT) {
                r = initialize_typed_array_from_typed_array(rt, rv, first_arg);
            } else if (gtt == RJS_GC_THING_ARRAY_BUFFER) {
                RJS_Value *byte_off = rjs_argument_get(rt, args, argc, 1);
                RJS_Value *length   = rjs_argument_get(rt, args, argc, 2);

                r = initialize_typed_array_from_array_buffer(rt, rv, first_arg, byte_off, length);
            } else {
                if ((r = rjs_get_method(rt, first_arg, rjs_pn_s_iterator(rt), use_iter)) == RJS_ERR)
                    goto end;

                if (!rjs_value_is_undefined(rt, use_iter)) {
                    r = initialize_typed_array_from_iterator(rt, rv, first_arg, use_iter);
                } else {
                    r = initialize_typed_array_from_arrray_like(rt, rv, first_arg);
                }
            }
        } else {
            int64_t elen;

            if ((r = rjs_to_index(rt, first_arg, &elen)) == RJS_ERR)
                goto end;

            r = allocate_typed_array(rt, etype, nt, proto_idx, elen, rv);
        }
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#define TYPED_ARRAY_DECL(n, b, t) \
static RJS_NF(n##_constructor)\
{\
    return typedef_array_constructor(rt, args, argc, nt, RJS_ARRAY_ELEMENT_##t, RJS_O_##n##_prototype, rv);\
}\
static const RJS_BuiltinFuncDesc \
n##_constructor_desc = {\
    #n,\
    3,\
    n##_constructor\
};\
static const RJS_BuiltinFieldDesc \
n##_field_descs[] = {\
    {\
        "BYTES_PER_ELEMENT",\
        RJS_VALUE_NUMBER,\
        b\
    },\
    {NULL}\
};\
static const RJS_BuiltinObjectDesc \
n##_prototype_desc = {\
    #n,\
    "TypedArray_prototype",\
    NULL,\
    NULL,\
    n##_field_descs,\
    NULL,\
    NULL,\
    NULL,\
    #n"_prototype"\
};

TYPED_ARRAY_DECL(Int8Array, 1, INT8)
TYPED_ARRAY_DECL(Uint8Array, 1, UINT8)
TYPED_ARRAY_DECL(Uint8ClampedArray, 1, UINT8C)
TYPED_ARRAY_DECL(Int16Array, 2, INT16)
TYPED_ARRAY_DECL(Uint16Array, 2, UINT16)
TYPED_ARRAY_DECL(Int32Array, 4, INT32)
TYPED_ARRAY_DECL(Uint32Array, 4, UINT32)
TYPED_ARRAY_DECL(Float32Array, 4, FLOAT32)
TYPED_ARRAY_DECL(Float64Array, 8, FLOAT64)
#if ENABLE_BIG_INT
TYPED_ARRAY_DECL(BigInt64Array, 8, BIGINT64)
TYPED_ARRAY_DECL(BigUint64Array, 8, BIGUINT64)
#endif /*ENABLE_BIG_INT*/
