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

/*Set the length property.*/
static RJS_Result
array_set_length (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyDesc *pd)
{
    RJS_Number       n;
    uint32_t         new_len, old_len, i;
    RJS_Result       r;
    RJS_PropertyDesc new_desc, old_desc;
    RJS_Bool         new_wr;
    RJS_Object      *o;
    RJS_ObjectOps   *ops;
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *idx = rjs_value_stack_push(rt);
    RJS_Value       *pk  = rjs_value_stack_push(rt);

    rjs_property_desc_init(rt, &new_desc);
    rjs_property_desc_init(rt, &old_desc);

    if (!(pd->flags & RJS_PROP_FL_HAS_VALUE)) {
        r = rjs_ordinary_object_op_define_own_property(rt, v, rjs_pn_length(rt), pd);
        goto end;
    }

    if ((r = rjs_to_uint32(rt, pd->value, &new_len)) == RJS_ERR)
        goto end;
    if ((r = rjs_to_number(rt, pd->value, &n)) == RJS_ERR)
        goto end;
    if (new_len != n) {
        r = rjs_throw_range_error(rt, _("\"length\" must be a valid array index value"));
        goto end;
    }

    new_desc.flags = pd->flags;
    rjs_value_set_number(rt, new_desc.value, new_len);

    rjs_ordinary_object_op_get_own_property(rt, v, rjs_pn_length(rt), &old_desc);
    old_len = rjs_value_get_number(rt, old_desc.value);

    if (new_len >= old_len) {
        r = rjs_ordinary_object_op_define_own_property(rt, v, rjs_pn_length(rt), &new_desc);
        goto end;
    }

    if (!(old_desc.flags & RJS_PROP_FL_WRITABLE)) {
        r = RJS_FALSE;
        goto end;
    }

    if (!(new_desc.flags & RJS_PROP_FL_HAS_WRITABLE) || (new_desc.flags & RJS_PROP_FL_WRITABLE)) {
        new_wr = RJS_TRUE;
    } else {
        new_wr = RJS_FALSE;
        new_desc.flags |= RJS_PROP_FL_WRITABLE;
    }

    if (!(r = rjs_ordinary_object_op_define_own_property(rt, v, rjs_pn_length(rt), &new_desc)))
        goto end;

    /*Delete the elements >= new_len.*/
    o   = rjs_value_get_object(rt, v);
    ops = (RJS_ObjectOps*)o->gc_thing.ops;
    r   = RJS_TRUE;

    if (ops->delete == rjs_ordinary_object_op_delete) {
        r = rjs_ordinary_object_delete_from_index(rt, v, old_len, new_len, &i);
    } else {
        for (i = old_len - 1; i >= new_len; i --) {
            RJS_PropertyName idx_pn;

            rjs_value_set_number(rt, idx, i);
            rjs_to_string(rt, idx, pk);

            rjs_property_name_init(rt, &idx_pn, pk);
            r = rjs_object_delete(rt, v, &idx_pn);
            rjs_property_name_deinit(rt, &idx_pn);

            if (!r)
                break;
        }
    }

    if (!r) {
        new_desc.flags = RJS_PROP_FL_HAS_VALUE;
        rjs_value_set_number(rt, new_desc.value, i + 1);

        if (!new_wr)
            new_desc.flags |= RJS_PROP_FL_HAS_WRITABLE;

        rjs_ordinary_object_op_define_own_property(rt, v, rjs_pn_length(rt), &new_desc);
        goto end;
    }

    if (!new_wr) {
        new_desc.flags = RJS_PROP_FL_HAS_WRITABLE;
        rjs_ordinary_object_op_define_own_property(rt, v, rjs_pn_length(rt), &new_desc);
    }

    r = RJS_TRUE;
end:
    rjs_property_desc_deinit(rt, &new_desc);
    rjs_property_desc_deinit(rt, &old_desc);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Set an array's item.*/
static RJS_Result
array_set_item (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_PropertyKey *pk, RJS_PropertyDesc *pd)
{
    RJS_PropertyDesc old;
    RJS_Result       r;
    uint32_t         olen;
    size_t           top = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, &old);

    rjs_ordinary_object_op_get_own_property(rt, v, rjs_pn_length(rt), &old);

    olen = rjs_value_get_number(rt, old.value);

    if ((pk->index >= olen) && !(old.flags & RJS_PROP_FL_WRITABLE)) {
        r = RJS_FALSE;
        goto end;
    }

    r = rjs_ordinary_object_op_define_own_property(rt, v, pn, pd);
    if (!r)
        goto end;

    if (pk->index >= olen) {
        RJS_Number n = ((RJS_Number)pk->index) + 1;

        rjs_value_set_number(rt, old.value, n);
        rjs_ordinary_object_op_define_own_property(rt, v, rjs_pn_length(rt), &old);
    }

    r = RJS_TRUE;
end:
    rjs_property_desc_deinit(rt, &old);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**Define an own property of an array.*/
extern RJS_Result
array_op_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_PropertyKey pk;
    RJS_Result      r;

    rjs_property_key_get(rt, pn->name, &pk);

    if (pk.is_index) {
        r = array_set_item(rt, o, pn, &pk, pd);
    } else if (rjs_value_is_string(rt, pn->name)
            && rjs_string_equal(rt, pn->name, rjs_s_length(rt))) {
        r = array_set_length(rt, o, pd);
    } else {
        r = rjs_ordinary_object_op_define_own_property(rt, o, pn, pd);
    }

    return r;
}

/*Array operation functions.*/
static const RJS_ObjectOps
array_ops = {
    {
        RJS_GC_THING_ARRAY,
        rjs_object_op_gc_scan,
        rjs_object_op_gc_free
    },
    rjs_ordinary_object_op_get_prototype_of,
    rjs_ordinary_object_op_set_prototype_of,
    rjs_ordinary_object_op_is_extensible,
    rjs_ordinary_object_op_prevent_extensions,
    rjs_ordinary_object_op_get_own_property,
    array_op_define_own_property,
    rjs_ordinary_object_op_has_property,
    rjs_ordinary_object_op_get,
    rjs_ordinary_object_op_set,
    rjs_ordinary_object_op_delete,
    rjs_ordinary_object_op_own_property_keys,
    NULL,
    NULL
};

/**
 * Create a new array.
 * \param rt The current runtime.
 * \param[out] v Return the array value.
 * \param len The length of the array.
 * \param proto The prototype of the array.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_array_new (RJS_Runtime *rt, RJS_Value *v, RJS_Number len, RJS_Value *proto)
{
    RJS_Realm        *realm = rjs_realm_current(rt);
    RJS_Object       *o;
    RJS_PropertyDesc  pd;
    size_t            top;

    if (len > 0xffffffff)
        return rjs_throw_range_error(rt, _("\"length\" is too big"));

    if (!proto)
        proto = rjs_o_Array_prototype(realm);

    RJS_NEW(rt, o);

    rjs_object_init(rt, v, o, proto, &array_ops);

    top = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, &pd);
    rjs_value_set_number(rt, pd.value, len);
    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE;

    rjs_ordinary_object_op_define_own_property(rt, v, rjs_pn_length(rt), &pd);

    rjs_property_desc_deinit(rt, &pd);

    rjs_value_stack_restore(rt, top);

    return RJS_OK;
}
