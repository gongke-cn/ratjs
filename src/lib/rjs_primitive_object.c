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

/*Scan the reference things in the primitive object.*/
static void
primitive_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PrimitiveObject *po = ptr;

    rjs_object_op_gc_scan(rt, &po->object);

    rjs_gc_scan_value(rt, &po->value);
}

/*Free the primitive object.*/
static void
primitive_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PrimitiveObject *po = ptr;

    rjs_object_deinit(rt, &po->object);

    RJS_DEL(rt, po);
}

/*Primitive value object operation functions.*/
static const RJS_ObjectOps
primitive_object_ops = {
    {
        RJS_GC_THING_PRIMITIVE,
        primitive_object_op_gc_scan,
        primitive_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS,
    NULL,
    NULL
};

/*Get the string's own property.*/
static RJS_Result
string_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p, RJS_PropertyDesc *pd)
{
    RJS_PrimitiveObject *po;
    RJS_Result           r;
    RJS_Number           n;
    uint64_t             idx;
    size_t               len;

    if (rjs_value_is_string(rt, p)) {
        if ((r = rjs_to_number(rt, p, &n)) == RJS_ERR)
            return r;
    } else {
        return RJS_FALSE;
    }

    if (isinf(n) || isnan(n))
        return RJS_FALSE;

    if (signbit(n))
        return RJS_FALSE;

    if (floor(n) != n)
        return RJS_FALSE;

    po  = (RJS_PrimitiveObject*)rjs_value_get_object(rt, o);
    len = rjs_string_get_length(rt, &po->value);

    idx = n;
    if (idx >= len)
        return RJS_FALSE;

    pd->flags = RJS_PROP_FL_DATA|RJS_PROP_FL_ENUMERABLE;

    rjs_string_substr(rt, &po->value, idx, idx + 1, pd->value);

    return RJS_OK;
}

/*Get the string object's own property.*/
static RJS_Result
string_object_op_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Result r;

    r = rjs_ordinary_object_op_get_own_property(rt, o, pn, pd);
    if (r)
        return r;

    return string_get_own_property(rt, o, pn->name, pd);
}

/*Define the string object's own property.*/
static RJS_Result
string_object_op_define_own_property (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_PropertyDesc old;
    RJS_Result       r;
    size_t           top = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, &old);

    if ((r = string_get_own_property(rt, v, pn->name, &old)) == RJS_ERR)
        goto end;

    if (r) {
        RJS_Object *o   = rjs_value_get_object(rt, v);
        RJS_Bool    ext = (o->flags & RJS_OBJECT_FL_EXTENSIBLE) ? RJS_TRUE : RJS_FALSE;

        r = rjs_is_compatible_property_descriptor(rt, ext, pd, &old);
    } else {
        r = rjs_ordinary_object_op_define_own_property(rt, v, pn, pd);
    }

end:
    rjs_property_desc_deinit(rt, &old);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the string object's own property keys.*/
static RJS_Result
string_object_op_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys)
{
    RJS_PrimitiveObject *po;
    RJS_PropertyKeyList *pkl;
    size_t               cap, len, i;
    RJS_Value           *kv;
    size_t               top = rjs_value_stack_save(rt);
    RJS_Value           *idx = rjs_value_stack_push(rt);

    po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, o);

    len = rjs_string_get_length(rt, &po->value);
    cap = len + po->object.prop_hash.entry_num + po->object.array_item_num;

    pkl = rjs_property_key_list_new(rt, keys, cap);

    for (i = 0; i < len; i ++) {
        kv = pkl->keys.items + pkl->keys.item_num ++;

        rjs_value_set_number(rt, idx, i);
        rjs_to_string(rt, idx, kv);
    }

    rjs_property_key_list_add_own_keys(rt, keys, o);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*String object operation functions.*/
static const RJS_ObjectOps
string_object_ops = {
    {
        RJS_GC_THING_PRIMITIVE,
        primitive_object_op_gc_scan,
        primitive_object_op_gc_free
    },
    rjs_ordinary_object_op_get_prototype_of,
    rjs_ordinary_object_op_set_prototype_of,
    rjs_ordinary_object_op_is_extensible,
    rjs_ordinary_object_op_prevent_extensions,
    string_object_op_get_own_property,
    string_object_op_define_own_property,
    rjs_ordinary_object_op_has_property,
    rjs_ordinary_object_op_get,
    rjs_ordinary_object_op_set,
    rjs_ordinary_object_op_delete,
    string_object_op_own_property_keys,
    NULL,
    NULL
};

/**
 * Create a new primitive value object.
 * \param rt The current runtime.
 * \param[out] v Return the new primitive object value.
 * \param nt The new target.
 * \param dp_idx The default prototype object's index.
 * \param prim The primitive value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_primitive_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *nt, int dp_idx, RJS_Value *prim)
{
    RJS_Result           r;
    const RJS_ObjectOps *ops;
    RJS_Realm           *realm = rjs_realm_current(rt);
    RJS_PrimitiveObject *po    = NULL;
    size_t               top   = rjs_value_stack_save(rt);
    RJS_Value           *proto = rjs_value_stack_push(rt);

    if (nt) {
        if ((r = rjs_get_prototype_from_constructor(rt, nt, dp_idx, proto)) == RJS_ERR)
            goto end;
    } else {
        rjs_value_copy(rt, proto, &realm->objects[dp_idx]);
    }

    RJS_NEW(rt, po);

    rjs_value_copy(rt, &po->value, prim);

    if (rjs_value_is_string(rt, prim))
        ops = &string_object_ops;
    else
        ops = &primitive_object_ops;

    if ((r = rjs_object_init(rt, v, &po->object, proto, ops)) == RJS_ERR)
        goto end;

    if (ops == &string_object_ops) {
        RJS_PropertyDesc pd;

        rjs_property_desc_init(rt, &pd);

        pd.flags = RJS_PROP_FL_DATA;
        rjs_value_set_number(rt, pd.value, rjs_string_get_length(rt, prim));
        rjs_define_property_or_throw(rt, v, rjs_pn_length(rt), &pd);

        rjs_property_desc_deinit(rt, &pd);
    }

    r = RJS_OK;
end:
    if (r == RJS_ERR) {
        if (po)
            RJS_DEL(rt, po);
    }

    rjs_value_stack_restore(rt, top);
    return r;
}
