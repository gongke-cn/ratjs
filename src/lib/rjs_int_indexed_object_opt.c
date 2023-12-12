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

/*Scan the referenced things in the integer indexed object.*/
static void
int_indexed_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_IntIndexedObject *iio = ptr;

    rjs_object_op_gc_scan(rt, &iio->object);
    rjs_gc_scan_value(rt, &iio->buffer);
}

/*Free the integer indexed object.*/
static void
int_indexed_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_IntIndexedObject *iio = ptr;

#if ENABLE_CTYPE
    if (iio->cptr_he.key) {
        rjs_cptr_remove(rt, &iio->cptr_info);
    }
#endif /*ENABLE_CTYPE*/

    rjs_object_deinit(rt, &iio->object);

    RJS_DEL(rt, iio);
}

/**
 * Check if the number is a valid integer index.
 * \param rt The current runtime.
 * \param o The integer index object.
 * \param n The number value.
 * \param[out] pidx Return the index value.
 * \retval RJS_TRUE The value is a valid index.
 * \retval RJS_FALSE The value is not a valid index.
 */
RJS_Bool
rjs_is_valid_int_index (RJS_Runtime *rt, RJS_Value *o, RJS_Number n, size_t *pidx)
{
    RJS_IntIndexedObject *iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);

    if (rjs_is_detached_buffer(rt, &iio->buffer))
        return RJS_FALSE;

    if (!rjs_is_integral_number(n))
        return RJS_FALSE;

    if (signbit(n))
        return RJS_FALSE;

    if ((n < 0) || (n >= iio->array_length))
        return RJS_FALSE;

    *pidx = n;

    return RJS_TRUE;
}

/**
 * Get the integer indexed element.
 * \param rt The current runtime.
 * \param o The integer indexed object.
 * \param n The index number.
 * \param[out] v Return the element value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_int_indexed_element_get (RJS_Runtime *rt, RJS_Value *o, RJS_Number n, RJS_Value *v)
{
    RJS_IntIndexedObject *iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);
    size_t                idx;
    size_t                offset, esize, pos;

    if (!rjs_is_valid_int_index(rt, o, n, &idx)) {
        rjs_value_set_undefined(rt, v);
        return RJS_OK;
    }

    offset = iio->byte_offset;
    esize  = rjs_typed_array_element_size(iio->type);
    pos    = offset + esize * idx;

    rjs_get_value_from_buffer(rt, &iio->buffer, pos, iio->type, rjs_is_little_endian(), v);

    return RJS_OK;
}

/**
 * Set the integer indexed element.
 * \param rt The current runtime.
 * \param o The integer indexed object.
 * \param n The index number.
 * \param v The new element value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_int_indexed_element_set (RJS_Runtime *rt, RJS_Value *o, RJS_Number n, RJS_Value *v)
{
    RJS_IntIndexedObject *iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);
    size_t                top = rjs_value_stack_save(rt);
    RJS_Value            *ev  = rjs_value_stack_push(rt);
    size_t                idx;
    RJS_Result            r;

#if ENABLE_BIG_INT
    if ((iio->type == RJS_ARRAY_ELEMENT_BIGINT64) || (iio->type == RJS_ARRAY_ELEMENT_BIGUINT64)) {
        if ((r = rjs_to_big_int(rt, v, ev)) == RJS_ERR)
            goto end;
    } else
#endif /*ENABLE_BIG_INT*/
    {
        RJS_Number en;

        if ((r = rjs_to_number(rt, v, &en)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, ev, en);
    }

    if (rjs_is_valid_int_index(rt, o, n, &idx)) {
        size_t offset, esize, pos;

        offset = iio->byte_offset;
        esize  = rjs_typed_array_element_size(iio->type);
        pos    = offset + esize * idx;

        rjs_set_value_in_buffer(rt, &iio->buffer, pos, iio->type, ev, rjs_is_little_endian());
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the integer indexed object's own property.*/
static RJS_Result
int_indexed_object_op_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Number n;
    RJS_Result r;

    if (rjs_value_is_string(rt, pn->name)
            && rjs_canonical_numeric_index_string(rt, pn->name, &n)) {
        pd->flags = RJS_PROP_FL_DATA
                |RJS_PROP_FL_WRITABLE
                |RJS_PROP_FL_CONFIGURABLE
                |RJS_PROP_FL_ENUMERABLE;

        if ((r = rjs_int_indexed_element_get(rt, o, n, pd->value)) == RJS_ERR)
            return r;

        if (rjs_value_is_undefined(rt, pd->value))
            return RJS_FALSE;
    } else {
        r = rjs_ordinary_object_op_get_own_property(rt, o, pn, pd);
    }

    return r;
}

/*Define a property to the integer indexed object.*/
static RJS_Result
int_indexed_object_op_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Number n;
    RJS_Result r;

    if (rjs_value_is_string(rt, pn->name)
            && rjs_canonical_numeric_index_string(rt, pn->name, &n)) {
        size_t idx;

        if (!rjs_is_valid_int_index(rt, o, n, &idx))
            return RJS_FALSE;

        if ((pd->flags & RJS_PROP_FL_HAS_CONFIGURABLE) && !(pd->flags & RJS_PROP_FL_CONFIGURABLE))
            return RJS_FALSE;
        if ((pd->flags & RJS_PROP_FL_HAS_ENUMERABLE) && !(pd->flags & RJS_PROP_FL_ENUMERABLE))
            return RJS_FALSE;
        if (rjs_is_accessor_descriptor(pd))
            return RJS_FALSE;
        if ((pd->flags & RJS_PROP_FL_HAS_WRITABLE) && !(pd->flags & RJS_PROP_FL_WRITABLE))
            return RJS_FALSE;
        if (pd->flags & RJS_PROP_FL_HAS_VALUE) {
            if ((r = rjs_int_indexed_element_set(rt, o, n, pd->value)) == RJS_ERR)
                return r;
        }

        return RJS_TRUE;
    }

    return rjs_ordinary_object_op_define_own_property(rt, o, pn, pd);
}

/*Check if the integer indexed object has the porperty.*/
static RJS_Result
int_indexed_object_op_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_Number n;

    if (rjs_value_is_string(rt, pn->name)
            && rjs_canonical_numeric_index_string(rt, pn->name, &n)) {
        size_t idx;

        return rjs_is_valid_int_index(rt, o, n, &idx);
    }

    return rjs_ordinary_object_op_has_property(rt, o, pn);
}

/*Get the integer indexed object's property.*/
static RJS_Result
int_indexed_object_op_get (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv)
{
    RJS_Number n;
    RJS_Result r;

    if (rjs_value_is_string(rt, pn->name)
            && rjs_canonical_numeric_index_string(rt, pn->name, &n)) {
        r = rjs_int_indexed_element_get(rt, o, n, pv);
    } else {
        r = rjs_ordinary_object_op_get(rt, o, pn, receiver, pv);
    }

    return r;
}

/*Set the integer indexed object's property.*/
static RJS_Result
int_indexed_object_op_set (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver)
{
    RJS_Number n;
    RJS_Result r;

    if (rjs_value_is_string(rt, pn->name)
            && rjs_canonical_numeric_index_string(rt, pn->name, &n)) {
        r = rjs_int_indexed_element_set(rt, o, n, pv);
    } else {
        r = rjs_ordinary_object_op_set(rt, o, pn, pv, receiver);
    }

    return r;
}

/*Delete a property of the integer indexed object.*/
static RJS_Result
int_indexed_object_op_delete (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_Number n;

    if (rjs_value_is_string(rt, pn->name)
            && rjs_canonical_numeric_index_string(rt, pn->name, &n)) {
        size_t idx;

        if (!rjs_is_valid_int_index(rt, o, n, &idx))
            return RJS_TRUE;

        return RJS_FALSE;
    }

    return rjs_ordinary_object_op_delete(rt, o, pn);
}

/*Get the integer indexed object's keys.*/
static RJS_Result
int_indexed_object_op_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys)
{
    RJS_IntIndexedObject *iio = (RJS_IntIndexedObject*)rjs_value_get_object(rt, o);
    size_t                top = rjs_value_stack_save(rt);
    RJS_Value            *tmp = rjs_value_stack_push(rt);
    RJS_PropertyKeyList  *pkl;
    size_t                i, cap;
    RJS_Result            r;

    cap = iio->array_length + iio->object.array_item_num + iio->object.prop_hash.entry_num;

    pkl = rjs_property_key_list_new(rt, keys, cap);

    for (i = 0; i < iio->array_length; i ++) {
        RJS_Value *kv = pkl->keys.items + pkl->keys.item_num ++;

        rjs_value_set_number(rt, tmp, i);
        rjs_to_string(rt, tmp, kv);
    }

    r = rjs_property_key_list_add_own_keys(rt, keys, o);

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Integer indexed object operation functions.*/
static const RJS_ObjectOps
int_indexed_object_ops = {
    {
        RJS_GC_THING_INT_INDEXED_OBJECT,
        int_indexed_object_op_gc_scan,
        int_indexed_object_op_gc_free
    },
    rjs_ordinary_object_op_get_prototype_of,
    rjs_ordinary_object_op_set_prototype_of,
    rjs_ordinary_object_op_is_extensible,
    rjs_ordinary_object_op_prevent_extensions,
    int_indexed_object_op_get_own_property,
    int_indexed_object_op_define_own_property,
    int_indexed_object_op_has_property,
    int_indexed_object_op_get,
    int_indexed_object_op_set,
    int_indexed_object_op_delete,
    int_indexed_object_op_own_property_keys,
    NULL,
    NULL
};

/**
 * Create a new integer indexed object.
 * \param rt The current runtime.
 * \param proto The prototype.
 * \param[out] v Return the new object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_int_indexed_object_create (RJS_Runtime *rt, RJS_Value *proto, RJS_Value *v)
{
    RJS_IntIndexedObject *iio;

    RJS_NEW(rt, iio);

    iio->type         = RJS_ARRAY_ELEMENT_UINT8;
    iio->array_length = 0;
    iio->byte_offset  = 0;
    iio->byte_length  = 0;

#if ENABLE_CTYPE
    iio->cptr_he.key = NULL;
#endif /*ENABLE_CTYPE*/

    rjs_value_set_undefined(rt, &iio->buffer);

    rjs_object_init(rt, v, &iio->object, proto, &int_indexed_object_ops);

    return RJS_OK;
}
