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

/**
 * Check if the value is an array.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE v is an array.
 * \retval RJS_FALSE v is not an array.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_is_array (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_GcThingType gtt;

    if (!rjs_value_is_object(rt, v))
        return RJS_FALSE;

    gtt = rjs_value_get_gc_thing_type(rt, v);
    if (gtt == RJS_GC_THING_ARRAY)
        return RJS_TRUE;

#if ENABLE_PROXY
    if (gtt == RJS_GC_THING_PROXY_OBJECT) {
        RJS_ProxyObject *po = (RJS_ProxyObject*)rjs_value_get_object(rt, v);

        if (rjs_value_is_null(rt, &po->target))
            return rjs_throw_type_error(rt, _("target od proxy is null"));

        return rjs_is_array(rt, &po->target);
    }
#endif /*ENABLE_PROXY*/

    return RJS_FALSE;
}

/**
 * Convert the value which is not an object to object.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] o Return the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_to_object_non_object (RJS_Runtime *rt, RJS_Value *v, RJS_Value *o)
{
    RJS_Result r = RJS_OK;

    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_UNDEFINED:
    case RJS_VALUE_NULL:
        r = rjs_throw_type_error(rt, _("the value is null or undefined"));
        break;
    case RJS_VALUE_BOOLEAN:
        r = rjs_primitive_object_new(rt, o, NULL, RJS_O_Boolean_prototype, v);
        break;
    case RJS_VALUE_NUMBER:
        r = rjs_primitive_object_new(rt, o, NULL, RJS_O_Number_prototype, v);
        break;
    case RJS_VALUE_STRING:
        r = rjs_primitive_object_new(rt, o, NULL, RJS_O_String_prototype, v);
        break;
    case RJS_VALUE_SYMBOL:
        r = rjs_primitive_object_new(rt, o, NULL, RJS_O_Symbol_prototype, v);
        break;
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
        r = rjs_primitive_object_new(rt, o, NULL, RJS_O_BigInt_prototype, v);
        break;
#endif /*ENABLE_BIG_INT*/
    default:
        assert(0);
        break;
    }

    return r;
}

/**
 * Convert the value which is not a string to string.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] s Return the result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_to_string_non_string (RJS_Runtime *rt, RJS_Value *v, RJS_Value *s)
{
    RJS_Result r = RJS_OK;

    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_UNDEFINED:
        rjs_value_copy(rt, s, rjs_s_undefined(rt));
        break;
    case RJS_VALUE_NULL:
        rjs_value_copy(rt, s, rjs_s_null(rt));
        break;
    case RJS_VALUE_BOOLEAN:
        if (rjs_value_get_boolean(rt, v))
            rjs_value_copy(rt, s, rjs_s_true(rt));
        else
            rjs_value_copy(rt, s, rjs_s_false(rt));
        break;
    case RJS_VALUE_NUMBER: {
        RJS_Number n = rjs_value_get_number(rt, v);

        r = rjs_number_to_string(rt, n, s);
        break;
    }
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
        r = rjs_big_int_to_string(rt, v, 10, s);
        break;
#endif /*ENABLE_BIG_INT*/
    case RJS_VALUE_OBJECT:
        if ((r = rjs_to_primitive(rt, v, s, RJS_VALUE_STRING)) == RJS_OK)
            r = rjs_to_string(rt, s, s);
        break;
    case RJS_VALUE_SYMBOL:
        r = rjs_throw_type_error(rt, _("symbol cannot be converted to string directly"));
        break;
    default:
        assert(0);
    }

    return r;
}

/**
 * Convert the value to index.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the index.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_to_index (RJS_Runtime *rt, RJS_Value *v, int64_t *pi)
{
    RJS_Result r;
    int64_t    i;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);

    if (rjs_value_is_undefined(rt, v)) {
        i = 0;
    } else {
        RJS_Number n;
        int64_t    clamped = 0;

        if ((r = rjs_to_integer_or_infinity(rt, v, &n)) == RJS_ERR)
            goto end;

        rjs_value_set_number(rt, tmp, n);
        rjs_to_length(rt, tmp, &clamped);

        if (n != clamped) {
            r = rjs_throw_range_error(rt, _("the value cannot be converted to index"));
            goto end;
        }

        i = clamped;
    }

    *pi = i;
    r   = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Convert the value's description to encoded characters.
 * \param rt The current runtime.
 * \param v The value.
 * \param cb The characters buffer.
 * \param enc The encoding.
 * \return The pointer of the characters.
 */
extern const char*
rjs_to_desc_chars (RJS_Runtime *rt, RJS_Value *v, RJS_CharBuffer *cb, const char *enc)
{
    size_t      top   = rjs_value_stack_save(rt);
    RJS_Value  *str   = rjs_value_stack_push(rt);
    const char *chars = NULL;
    RJS_Result  r;

    if (rjs_value_is_symbol(rt, v)) {
        RJS_Symbol *s = rjs_value_get_gc_thing(rt, v);

        rjs_value_copy(rt, str, &s->description);

        if (rjs_value_is_undefined(rt, str))
            chars = "";
        else
            chars = rjs_string_to_enc_chars(rt, str, cb, enc);

        r = RJS_OK;
    }
#if ENABLE_PRIV_NAME
    else if (rjs_value_is_private_name(rt, v)) {
        RJS_PrivateName *pn = rjs_value_get_gc_thing(rt, v);

        chars = rjs_string_to_enc_chars(rt, &pn->description, cb, enc);
        r     = RJS_OK;
    }
#endif /*ENABLE_PRIV_NAME*/
    else {
        if ((r = rjs_to_string(rt, v, str)) == RJS_OK)
            chars = rjs_string_to_enc_chars(rt, str, cb, enc);
    }

    rjs_value_stack_restore(rt, top);
    return chars;
}

/**
 * Convert the object to property descriptor.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] pd Return the property descriptor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_to_property_descriptor (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyDesc *pd)
{
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);

    if (!rjs_value_is_object(rt, o)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    pd->flags = 0;

    /*Enumerable.*/
    if ((r = rjs_object_has_property(rt, o, rjs_pn_enumerable(rt))) == RJS_ERR)
        goto end;

    if (r) {
        if ((r = rjs_get(rt, o, rjs_pn_enumerable(rt), tmp)) == RJS_ERR)
            goto end;

        pd->flags |= RJS_PROP_FL_HAS_ENUMERABLE;

        if (rjs_to_boolean(rt, tmp))
            pd->flags |= RJS_PROP_FL_ENUMERABLE;
    }

    /*Configurable.*/
    if ((r = rjs_object_has_property(rt, o, rjs_pn_configurable(rt))) == RJS_ERR)
        goto end;

    if (r) {
        if ((r = rjs_get(rt, o, rjs_pn_configurable(rt), tmp)) == RJS_ERR)
            goto end;

        pd->flags |= RJS_PROP_FL_HAS_CONFIGURABLE;

        if (rjs_to_boolean(rt, tmp))
            pd->flags |= RJS_PROP_FL_CONFIGURABLE;
    }

    /*Value.*/
    if ((r = rjs_object_has_property(rt, o, rjs_pn_value(rt))) == RJS_ERR)
        goto end;

    if (r) {
        pd->flags |= RJS_PROP_FL_HAS_VALUE;

        if ((r = rjs_get(rt, o, rjs_pn_value(rt), pd->value)) == RJS_ERR)
            goto end;
    }

    /*Writable.*/
    if ((r = rjs_object_has_property(rt, o, rjs_pn_writable(rt))) == RJS_ERR)
        goto end;

    if (r) {
        if ((r = rjs_get(rt, o, rjs_pn_writable(rt), tmp)) == RJS_ERR)
            goto end;

        pd->flags |= RJS_PROP_FL_HAS_WRITABLE;

        if (rjs_to_boolean(rt, tmp))
            pd->flags |= RJS_PROP_FL_WRITABLE;
    }

    /*Get.*/
    if ((r = rjs_object_has_property(rt, o, rjs_pn_get(rt))) == RJS_ERR)
        goto end;

    if (r) {
        pd->flags |= RJS_PROP_FL_HAS_GET;

        if ((r = rjs_get(rt, o, rjs_pn_get(rt), pd->get)) == RJS_ERR)
            goto end;

        if (!rjs_is_callable(rt, pd->get) && !rjs_value_is_undefined(rt, pd->get)) {
            r = rjs_throw_type_error(rt, _("\"get\" is neither a function nor undefined"));
            goto end;
        }
    }

    /*Set.*/
    if ((r = rjs_object_has_property(rt, o, rjs_pn_set(rt))) == RJS_ERR)
        goto end;

    if (r) {
        pd->flags |= RJS_PROP_FL_HAS_SET;

        if ((r = rjs_get(rt, o, rjs_pn_set(rt), pd->set)) == RJS_ERR)
            goto end;

        if (!rjs_is_callable(rt, pd->set) && !rjs_value_is_undefined(rt, pd->set)) {
            r = rjs_throw_type_error(rt, _("\"set\" is neither a function nor undefined"));
            goto end;
        }
    }

    if ((pd->flags & (RJS_PROP_FL_HAS_GET|RJS_PROP_FL_HAS_SET))
            && (pd->flags & (RJS_PROP_FL_HAS_VALUE|RJS_PROP_FL_HAS_WRITABLE))) {
        r = rjs_throw_type_error(rt, _("the accessor descriptor cannot has \"value\" or \"writable\" property"));
        goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create an object from a property descritptor.
 * \param rt The current runtime.
 * \param pd The property descriptor.
 * \param[out] v Return the object value or undefined.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_from_property_descriptor (RJS_Runtime *rt, RJS_PropertyDesc *pd, RJS_Value *v)
{
    RJS_Result r;
    RJS_Realm *realm = rjs_realm_current(rt);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *tmp   = rjs_value_stack_push(rt);

    if (!pd) {
        rjs_value_set_undefined(rt, v);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_ordinary_object_create(rt, rjs_o_Object_prototype(realm), v)) == RJS_ERR)
        goto end;

    if (pd->flags & RJS_PROP_FL_HAS_VALUE) {
        if ((r = rjs_create_data_property_or_throw(rt, v, rjs_pn_value(rt), pd->value)) == RJS_ERR)
            goto end;
    }

    if (pd->flags & RJS_PROP_FL_HAS_WRITABLE) {
        rjs_value_set_boolean(rt, tmp, pd->flags & RJS_PROP_FL_WRITABLE);

        if ((r = rjs_create_data_property_or_throw(rt, v, rjs_pn_writable(rt), tmp)) == RJS_ERR)
            goto end;
    }

    if (pd->flags & RJS_PROP_FL_HAS_GET) {
        if ((r = rjs_create_data_property_or_throw(rt, v, rjs_pn_get(rt), pd->get)) == RJS_ERR)
            goto end;
    }

    if (pd->flags & RJS_PROP_FL_HAS_SET) {
        if ((r = rjs_create_data_property_or_throw(rt, v, rjs_pn_set(rt), pd->set)) == RJS_ERR)
            goto end;
    }

    if (pd->flags & RJS_PROP_FL_HAS_ENUMERABLE) {
        rjs_value_set_boolean(rt, tmp, pd->flags & RJS_PROP_FL_ENUMERABLE);

        if ((r = rjs_create_data_property_or_throw(rt, v, rjs_pn_enumerable(rt), tmp)) == RJS_ERR)
            goto end;
    }

    if (pd->flags & RJS_PROP_FL_HAS_CONFIGURABLE) {
        rjs_value_set_boolean(rt, tmp, pd->flags & RJS_PROP_FL_CONFIGURABLE);

        if ((r = rjs_create_data_property_or_throw(rt, v, rjs_pn_configurable(rt), tmp)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Complete the property descriptor.
 * \param rt The current runtime.
 * \param pd The property descriptor.
 */
void
rjs_complete_property_descriptor (RJS_Runtime *rt, RJS_PropertyDesc *pd)
{
    if (rjs_is_generic_descriptor(pd) || rjs_is_data_descriptor(pd)) {
        if (!(pd->flags & RJS_PROP_FL_HAS_VALUE)) {
            pd->flags |= RJS_PROP_FL_HAS_VALUE;
            rjs_value_set_undefined(rt, pd->value);
        }

        if (!(pd->flags & RJS_PROP_FL_HAS_WRITABLE)) {
            pd->flags |= RJS_PROP_FL_HAS_WRITABLE;
            pd->flags &= ~RJS_PROP_FL_WRITABLE;
        }
    } else {
        if (!(pd->flags & RJS_PROP_FL_HAS_GET)) {
            pd->flags |= RJS_PROP_FL_HAS_GET;
            rjs_value_set_undefined(rt, pd->get);
        }

        if (!(pd->flags & RJS_PROP_FL_HAS_SET)) {
            pd->flags |= RJS_PROP_FL_HAS_SET;
            rjs_value_set_undefined(rt, pd->set);
        }
    }

    if (!(pd->flags & RJS_PROP_FL_HAS_CONFIGURABLE)) {
        pd->flags |= RJS_PROP_FL_HAS_CONFIGURABLE;
        pd->flags &= ~RJS_PROP_FL_CONFIGURABLE;
    }

    if (!(pd->flags & RJS_PROP_FL_HAS_ENUMERABLE)) {
        pd->flags |= RJS_PROP_FL_HAS_ENUMERABLE;
        pd->flags &= ~RJS_PROP_FL_ENUMERABLE;
    }
}

/**
 * Get the length of an array like object.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] pl Return the length.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_length_of_array_like (RJS_Runtime *rt, RJS_Value *o, int64_t *pl)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_get(rt, o, rjs_pn_length(rt), tmp)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_length(rt, tmp, pl)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get the super property.
 * \param rt The current runtime.
 * \param thiz This argument.
 * \param pn The property name.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_super_get_v (RJS_Runtime *rt, RJS_Value *thiz, RJS_PropertyName *pn, RJS_Value *pv)
{
    size_t             top  = rjs_value_stack_save(rt);
    RJS_Value         *base = rjs_value_stack_push(rt);
    RJS_Value         *bo   = rjs_value_stack_push(rt);
    RJS_Environment   *env;
    RJS_Result         r;

    env = rjs_get_this_environment(rt);

    if ((r = rjs_env_get_super_base(rt, env, base)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_object(rt, base, bo)) == RJS_ERR)
        goto end;

    r = rjs_object_get(rt, bo, pn, thiz, pv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the super property.
 * \param rt The current runtime.
 * \param thiz This argument.
 * \param pn The property name.
 * \param pv The property value.
 * \param th Throw TypeError when failed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_super_set_v (RJS_Runtime *rt, RJS_Value *thiz, RJS_PropertyName *pn, RJS_Value *pv, RJS_Bool th)
{
    size_t             top  = rjs_value_stack_save(rt);
    RJS_Value         *base = rjs_value_stack_push(rt);
    RJS_Value         *bo   = rjs_value_stack_push(rt);
    RJS_Environment   *env;
    RJS_Result         r;

    env = rjs_get_this_environment(rt);

    if ((r = rjs_env_get_super_base(rt, env, base)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_object(rt, base, bo)) == RJS_ERR)
        goto end;

    if ((r = rjs_object_set(rt, bo, pn, pv, thiz)) == RJS_ERR)
        goto end;

    if (!r && th)
        r = rjs_throw_type_error(rt, _("the super property cannot be set"));
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if the object has the property.
 * \param rt The current runtime.
 * \param o The object value.
 * \param p The property value.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE The object has the property.
 * \retval RJS_FALSE The object has not the property. 
 */
RJS_Result
rjs_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p)
{
    RJS_PropertyName pn;
    RJS_Result       r;

    if (!rjs_value_is_object(rt, o))
        return rjs_throw_type_error(rt, _("the value is not an object"));

    rjs_property_name_init(rt, &pn, p);
    r = rjs_object_has_property(rt, o, &pn);
    rjs_property_name_deinit(rt, &pn);

    return r;
}

/**
 * Delete a property.
 * \param rt The current runtime.
 * \param o The object.
 * \param p The property to be deleted.
 * \param strict Is in strict mode or not.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE The property is deleted.
 * \retval RJS_FALSE The property cannot be deleted.
 */
RJS_Result
rjs_delete_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Bool strict)
{
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *bo  = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, o, bo)) == RJS_ERR)
        goto end;

    if ((r = rjs_object_delete(rt, bo, pn)) == RJS_ERR)
        goto end;

    if (!r && strict)
        r = rjs_throw_type_error(rt, _("property \"%s\" cannot be deleted"),
                rjs_to_desc_chars(rt, pn->name, NULL, NULL));
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a new data property.
 * \param rt The current runtime.
 * \param o The object value.
 * \param pn The property name.
 * \param v The data property value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The property cannot be created.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_data_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v)
{
    return rjs_create_data_property_attrs(rt, o, pn, v,
            RJS_PROP_FL_CONFIGURABLE|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_ENUMERABLE);
}

/**
 * Create a new data property with attributes.
 * \param rt The current runtime.
 * \param o The object value.
 * \param pn The property name.
 * \param v The data property value.
 * \param attrs The attributes.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The property cannot be created.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_data_property_attrs (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v, int attrs)
{
    RJS_PropertyDesc pd;
    RJS_Result       r;
    size_t           top = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, &pd);

    attrs &= RJS_PROP_FL_WRITABLE
            |RJS_PROP_FL_ENUMERABLE
            |RJS_PROP_FL_CONFIGURABLE;

    pd.flags = RJS_PROP_FL_DATA|attrs;

    rjs_value_copy(rt, pd.value, v);

    r = rjs_object_define_own_property(rt, o, pn, &pd);

    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Create a method property.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param v The function value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_method_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v)
{
    RJS_PropertyDesc pd;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;
    rjs_value_copy(rt, pd.value, v);
    r = rjs_object_define_own_property(rt, o, pn, &pd);

    rjs_property_desc_deinit(rt, &pd);

    return r;
}

/**
 * Create an array from the list.
 * \param rt The current relam.
 * \param[out] a Return the array value.
 * \param ... items end with NULL.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_array_from_elements (RJS_Runtime *rt, RJS_Value *a, ...)
{
    va_list          ap;
    RJS_Result       r;
    size_t           i;

    if ((r = rjs_array_new(rt, a, 0, NULL)) == RJS_ERR)
        goto end;

    va_start(ap, a);

    i = 0;

    while (1) {
        RJS_Value *item;

        item = va_arg(ap, RJS_Value*);
        if (!item)
            break;

        rjs_create_data_property_or_throw_index(rt, a, i, item);

        i ++;
    }

    va_end(ap);

    r = RJS_OK;
end:
    return r;
}

/**
 * Create an array from the value list.
 * \param rt The current runtime.
 * \param vl The value list.
 * \param[out] a Return the array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_array_from_list (RJS_Runtime *rt, RJS_ValueList *vl, RJS_Value *a)
{
    RJS_ValueListSegment *vls;
    size_t                i, k;
    RJS_Result            r;

    rjs_array_new(rt, a, vl->len, NULL);

    k = 0;

    rjs_list_foreach_c(&vl->seg_list, vls, RJS_ValueListSegment, ln) {
        for (i = 0; i < vls->num; i ++) {
            if ((r = rjs_set_index(rt, a, k, &vls->v[i], RJS_TRUE)) == RJS_ERR)
                return r;

            k ++;
        }
    }

    return RJS_OK;
}

/**
 * Create an array from the value buffer.
 * \param rt The current runtime.
 * \param items The items value buffer.
 * \param n Number of the items.
 * \param[out] a Return the array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_array_from_value_buffer (RJS_Runtime *rt, RJS_Value *items, size_t n, RJS_Value *a)
{
    RJS_Value  *item;
    RJS_Result  r;
    size_t      i;

    if ((r = rjs_array_new(rt, a, 0, NULL)) == RJS_ERR)
        goto end;

    for (i = 0; i < n; i ++) {
        item = rjs_value_buffer_item(rt, items, i);

        rjs_create_data_property_or_throw_index(rt, a, i, item);
    }

    r = RJS_OK;
end:
    return r;
}

/**
 * Create an array from the iterable object.
 * \param rt The current runtime.
 * \param iterable The iterable object.
 * \param[out] a Return the array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_array_from_iterable (RJS_Runtime *rt, RJS_Value *iterable, RJS_Value *a)
{
    RJS_Iterator  iter;
    RJS_Result    r;
    size_t        top = rjs_value_stack_save(rt);
    RJS_Value    *ir  = rjs_value_stack_push(rt);
    RJS_Value    *iv  = rjs_value_stack_push(rt);
    size_t        i   = 0;

    rjs_iterator_init(rt, &iter);

    if ((r = rjs_array_new(rt, a, 0, NULL)) == RJS_ERR)
        goto end;

    if ((r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iter)) == RJS_ERR)
        goto end;

    while (1) {
        if ((r = rjs_iterator_step(rt, &iter, ir)) == RJS_ERR)
            goto end;
        if (!r)
            break;

        if ((r = rjs_iterator_value(rt, ir, iv)) == RJS_ERR)
            goto end;

        rjs_create_data_property_or_throw_index(rt, a, i, iv);

        i ++;
    }

    r = RJS_OK;
end:
    rjs_iterator_deinit(rt, &iter);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Make the function as a consturctor.
 * \param rt The current runtime.
 * \param f The function.
 * \param writable The prototype is writable.
 * \param proto The prototype value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_make_constructor (RJS_Runtime *rt, RJS_Value *f, RJS_Bool writable, RJS_Value *proto)
{
    RJS_Realm       *realm  = rjs_realm_current(rt);
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *protov = rjs_value_stack_push(rt);
    RJS_GcThingType  gtt    = rjs_value_get_gc_thing_type(rt, f);
    RJS_PropertyDesc pd;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if (gtt == RJS_GC_THING_SCRIPT_FUNC) {
        rjs_script_func_object_make_constructor(rt, f);
    } else if (gtt == RJS_GC_THING_BUILTIN_FUNC) {
        rjs_builtin_func_object_make_constructor(rt, f);
    }

    if (!proto) {
        proto = protov;

        if ((r = rjs_ordinary_object_create(rt, rjs_o_Object_prototype(realm), proto)) == RJS_ERR)
            goto end;

        pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_CONFIGURABLE;
        if (writable)
            pd.flags |= RJS_PROP_FL_WRITABLE;

        rjs_value_copy(rt, pd.value, f);

        rjs_define_property_or_throw(rt, proto, rjs_pn_constructor(rt), &pd);
    }

    pd.flags = RJS_PROP_FL_DATA;
    if (writable)
        pd.flags |= RJS_PROP_FL_WRITABLE;

    rjs_value_copy(rt, pd.value, proto);

    rjs_define_property_or_throw(rt, f, rjs_pn_prototype(rt), &pd);

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Make the function as a method.
 * \param rt The current runtime.
 * \param f The function.
 * \param ho The home object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_make_method (RJS_Runtime *rt, RJS_Value *f, RJS_Value *ho)
{
    RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, f);

    rjs_value_copy(rt, &sfo->home_object, ho);

    return RJS_OK;
}

/**
 * Reset the object's integrity level.
 * \param rt The current runtime.
 * \param o The object.
 * \param level The new level.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_integrity_level (RJS_Runtime *rt, RJS_Value *o, RJS_IntegrityLevel level)
{
    RJS_Result           r;
    RJS_PropertyDesc     pd;
    RJS_PropertyKeyList *pkl;
    size_t               i;
    size_t               top  = rjs_value_stack_save(rt);
    RJS_Value           *keys = rjs_value_stack_push(rt);

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_object_prevent_extensions(rt, o)) != RJS_OK)
        goto end;

    if ((r = rjs_object_own_property_keys(rt, o, keys)) == RJS_ERR)
        goto end;

    pkl = (RJS_PropertyKeyList*)rjs_value_get_object(rt, keys);

    if (level == RJS_INTEGRITY_SEALED) {
        pd.flags = RJS_PROP_FL_HAS_CONFIGURABLE;

        for (i = 0; i < pkl->keys.item_num; i ++) {
            RJS_Value       *key = &pkl->keys.items[i];
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, key);
            r = rjs_define_property_or_throw(rt, o, &pn, &pd);
            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;
        }
    } else {
        for (i = 0; i < pkl->keys.item_num; i ++) {
            RJS_Value       *key = &pkl->keys.items[i];
            RJS_PropertyName pn;

            rjs_property_name_init(rt, &pn, key);

            r = rjs_object_get_own_property(rt, o, &pn, &pd);

            if (r == RJS_TRUE) {
                if (rjs_is_accessor_descriptor(&pd)) {
                    pd.flags = RJS_PROP_FL_HAS_CONFIGURABLE;
                } else {
                    pd.flags = RJS_PROP_FL_HAS_CONFIGURABLE|RJS_PROP_FL_HAS_WRITABLE;
                }

                r = rjs_define_property_or_throw(rt, o, &pn, &pd);
            }

            rjs_property_name_deinit(rt, &pn);

            if (r == RJS_ERR)
                goto end;
        }
    }
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Test the object's integrity level.
 * \param rt The current runtime.
 * \param o The object.
 * \param level The new level.
 * \retval RJS_TRUE The object has set the integrity level.
 * \retval RJS_FALSE The object has not set the integrity level.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_test_integrity_level (RJS_Runtime *rt, RJS_Value *o, RJS_IntegrityLevel level)
{
    size_t               top  = rjs_value_stack_save(rt);
    RJS_Value           *keys = rjs_value_stack_push(rt);
    RJS_PropertyKeyList *pkl;
    RJS_PropertyDesc     pd;
    size_t               i;
    RJS_Result           r;

    rjs_property_desc_init(rt, &pd);

    if (rjs_object_is_extensible(rt, o)) {
        r = RJS_FALSE;
        goto end;
    }

    if ((r = rjs_object_own_property_keys(rt, o, keys)) == RJS_ERR)
        goto end;

    pkl = rjs_value_get_gc_thing(rt, keys);
    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_PropertyName pn;
        RJS_Value       *key = &pkl->keys.items[i];

        rjs_property_name_init(rt, &pn, key);
        r = rjs_object_get_own_property(rt, o, &pn, &pd);
        if (r == RJS_OK) {
            if (pd.flags & RJS_PROP_FL_CONFIGURABLE) {
                r = RJS_FALSE;
            } else if ((level == RJS_INTEGRITY_FROZEN)
                    && rjs_is_data_descriptor(&pd)
                    && (pd.flags & RJS_PROP_FL_WRITABLE)) {
                r = RJS_FALSE;
            }
        } else if (r == RJS_FALSE) {
            r = RJS_TRUE;
        }
        rjs_property_name_deinit(rt, &pn);

        if (r != RJS_TRUE)
            goto end;
    }

    r = RJS_TRUE;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the name property of the function.
 * \param rt The current runtime.
 * \param f The function.
 * \param name The name value.
 * \param prefix Teh prefix.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_function_name (RJS_Runtime *rt, RJS_Value *f, RJS_Value *name, RJS_Value *prefix)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *n1  = rjs_value_stack_push(rt);
    RJS_Value       *n2  = rjs_value_stack_push(rt);
    RJS_Result       r;
    RJS_PropertyDesc pd;
    RJS_UCharBuffer  ucb;

    rjs_uchar_buffer_init(rt, &ucb);
    rjs_property_desc_init(rt, &pd);

    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_CONFIGURABLE;

    if (rjs_value_is_symbol(rt, name)) {
        RJS_Symbol *sym = rjs_value_get_symbol(rt, name);

        if (rjs_value_is_undefined(rt, &sym->description)) {
            rjs_value_copy(rt, n1, rjs_s_empty(rt));
        } else {
            rjs_uchar_buffer_append_uc(rt, &ucb, '[');
            rjs_uchar_buffer_append_string(rt, &ucb, &sym->description);
            rjs_uchar_buffer_append_uc(rt, &ucb, ']');

            rjs_string_from_uchars(rt, n1, ucb.items, ucb.item_num);
            rjs_uchar_buffer_clear(rt, &ucb);
        }
    } else
#if ENABLE_PRIV_NAME
    if (rjs_value_is_private_name(rt, name)) {
        RJS_PrivateName *pn = rjs_value_get_gc_thing(rt, name);

        rjs_value_copy(rt, n1, &pn->description);
    } else
#endif /*ENABLE_PRIV_NAME*/
    {
        rjs_value_copy(rt, n1, name);
    }

#if ENABLE_FUNC_SOURCE
    if (rjs_value_get_gc_thing_type(rt, f) == RJS_GC_THING_BUILTIN_FUNC) {
        RJS_BuiltinFuncObject *bfo = (RJS_BuiltinFuncObject*)rjs_value_get_object(rt, f);

        rjs_value_copy(rt, &bfo->init_name, n1);
    }
#endif /*ENABLE_FUNC_SOURCE*/

    if (prefix) {
        rjs_uchar_buffer_append_string(rt, &ucb, prefix);
        rjs_uchar_buffer_append_uc(rt, &ucb, ' ');
        rjs_uchar_buffer_append_string(rt, &ucb, n1);

        rjs_string_from_uchars(rt, n2, ucb.items, ucb.item_num);
    } else {
        rjs_value_copy(rt, n2, n1);
    }

    rjs_value_copy(rt, pd.value, n2);

    r = rjs_define_property_or_throw(rt, f, rjs_pn_name(rt), &pd);

    rjs_property_desc_deinit(rt, &pd);
    rjs_uchar_buffer_deinit(rt, &ucb);
    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Set the length property of the function.
 * \param rt The current runtime.
 * \param f The function.
 * \param len The length value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_function_length (RJS_Runtime *rt, RJS_Value *f, RJS_Number len)
{
    size_t           top = rjs_value_stack_save(rt);
    RJS_Result       r;
    RJS_PropertyDesc pd;

    rjs_property_desc_init(rt, &pd);

    pd.flags = RJS_PROP_FL_DATA|RJS_PROP_FL_CONFIGURABLE;
    rjs_value_set_number(rt, pd.value, len);

    r = rjs_define_property_or_throw(rt, f, rjs_pn_length(rt), &pd);

    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Typeof operation.
 * \param rt The current runtime.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_type_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_Value *pv;

    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_UNDEFINED:
        pv = rjs_s_undefined(rt);
        break;
    case RJS_VALUE_NULL:
        pv = rjs_s_object(rt);
        break;
    case RJS_VALUE_BOOLEAN:
        pv = rjs_s_boolean(rt);
        break;
    case RJS_VALUE_STRING:
        pv = rjs_s_string(rt);
        break;
    case RJS_VALUE_NUMBER:
        pv = rjs_s_number(rt);
        break;
    case RJS_VALUE_SYMBOL:
        pv = rjs_s_symbol(rt);
        break;
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
        pv = rjs_s_bigint(rt);
        break;
#endif /*ENABLE_BIG_INT*/
    case RJS_VALUE_OBJECT:
        if (rjs_is_callable(rt, v))
            pv = rjs_s_function(rt);
        else
            pv = rjs_s_object(rt);
        break;
    default:
        assert(0);
        pv = NULL;
    }

    rjs_value_copy(rt, rv, pv);
    return RJS_OK;
}

/**
 * Chck if the ordinary object has the instance.
 * \param rt The current runtime.
 * \param c The class object.
 * \param o The instance object.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE o is the instance of c.
 * \retval RJS_FALSE o is not the instance of c.
 */
RJS_Result
rjs_ordinary_has_instance (RJS_Runtime *rt, RJS_Value *c, RJS_Value *o)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *proto = rjs_value_stack_push(rt);
    RJS_Value *t     = rjs_value_stack_push(rt);
    RJS_Result r;

#if ENABLE_BOUND_FUNC
    if (rjs_value_get_gc_thing_type(rt, c) == RJS_GC_THING_BOUND_FUNC) {
        RJS_BoundFuncObject *bfo;

        bfo = (RJS_BoundFuncObject*)rjs_value_get_object(rt, c);

        r = rjs_instance_of(rt, o, &bfo->target_func);
        goto end;
    }
#endif /*ENABLE_BOUND_FUNC*/

    if (!rjs_value_is_object(rt, o)) {
        r = RJS_FALSE;
        goto end;
    }

    if ((r = rjs_object_get(rt, c, rjs_pn_prototype(rt), c, proto)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_object(rt, proto)) {
        r = rjs_throw_type_error(rt, _("\"prototype\" is not an object"));
        goto end;
    }

    while (1) {
        if ((r = rjs_object_get_prototype_of(rt, o, t)) == RJS_ERR)
            goto end;

        if (rjs_value_is_null(rt, t)) {
            r = RJS_FALSE;
            goto end;
        }

        if (rjs_same_value(rt, proto, t)) {
            r = RJS_TRUE;
            goto end;
        }

        rjs_value_copy(rt, o, t);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if the value is an instance of an object.
 * \param rt The current runtime.
 * \param v The value.
 * \param t The target object.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE v is the instance of t.
 * \retval RJS_FALSE v is not the instance of t.
 */
RJS_Result
rjs_instance_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *t)
{
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *fn  = rjs_value_stack_push(rt);
    RJS_Value  *rv  = rjs_value_stack_push(rt);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, t)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_get_method(rt, t, rjs_pn_s_hasInstance(rt), fn)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, fn)) {
        if ((r = rjs_call(rt, fn, t, v, 1, rv)) == RJS_ERR)
            goto end;

        r = rjs_to_boolean(rt, rv);
        goto end;
    }

    if (!rjs_is_callable(rt, t)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    r = rjs_ordinary_has_instance(rt, t, v);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if 2 values are loosely equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 == v2.
 * \retval RJS_FALSE v1 != v2.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_is_loosely_equal (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_Result r;
    size_t     top;
    RJS_Value *tmp;

    if (rjs_value_get_type(rt, v1) == rjs_value_get_type(rt, v2))
        return rjs_is_strictly_equal(rt, v1, v2);

    if (rjs_value_is_null(rt, v1) && rjs_value_is_undefined(rt, v2))
        return RJS_TRUE;
    if (rjs_value_is_null(rt, v2) && rjs_value_is_undefined(rt, v1))
        return RJS_TRUE;

    top = rjs_value_stack_save(rt);
    tmp = rjs_value_stack_push(rt);

    if (rjs_value_is_number(rt, v1) && rjs_value_is_string(rt, v2)) {
        RJS_Number n;

        rjs_to_number(rt, v2, &n);
        rjs_value_set_number(rt, tmp, n);

        r = rjs_is_loosely_equal(rt, v1, tmp);
        goto end;
    }

    if (rjs_value_is_string(rt, v1) && rjs_value_is_number(rt, v2)) {
        RJS_Number n;

        rjs_to_number(rt, v1, &n);
        rjs_value_set_number(rt, tmp, n);

        r = rjs_is_loosely_equal(rt, tmp, v2);
        goto end;
    }

#if ENABLE_BIG_INT
    if (rjs_value_is_big_int(rt, v1) && rjs_value_is_string(rt, v2)) {
        rjs_string_to_big_int(rt, v2, tmp);

        if (rjs_value_is_undefined(rt, tmp)) {
            r = RJS_FALSE;
            goto end;
        }

        r = rjs_is_loosely_equal(rt, v1, tmp);
        goto end;
    }

    if (rjs_value_is_string(rt, v1) && rjs_value_is_big_int(rt, v2)) {
        r = rjs_is_loosely_equal(rt, v2, v1);
        goto end;
    }
#endif /*ENABLE_BIG_INT*/

    if (rjs_value_is_boolean(rt, v1)) {
        RJS_Number n;

        rjs_to_number(rt, v1, &n);
        rjs_value_set_number(rt, tmp, n);

        r = rjs_is_loosely_equal(rt, tmp, v2);
        goto end;
    }

    if (rjs_value_is_boolean(rt, v2)) {
        RJS_Number n;

        rjs_to_number(rt, v2, &n);
        rjs_value_set_number(rt, tmp, n);

        r = rjs_is_loosely_equal(rt, v1, tmp);
        goto end;
    }

    switch (rjs_value_get_type(rt, v1)) {
    case RJS_VALUE_STRING:
    case RJS_VALUE_NUMBER:
    case RJS_VALUE_SYMBOL:
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
#endif
        if (rjs_value_is_object(rt, v2)) {
            if ((r = rjs_to_primitive(rt, v2, tmp, -1)) == RJS_ERR)
                goto end;

            r = rjs_is_loosely_equal(rt, v1, tmp);
            goto end;
        }
        break;
    default:
        break;
    }

    switch (rjs_value_get_type(rt, v2)) {
    case RJS_VALUE_STRING:
    case RJS_VALUE_NUMBER:
    case RJS_VALUE_SYMBOL:
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
#endif
        if (rjs_value_is_object(rt, v1)) {
            if ((r = rjs_to_primitive(rt, v1, tmp, -1)) == RJS_ERR)
                goto end;

            r = rjs_is_loosely_equal(rt, tmp, v2);
            goto end;
        }
        break;
    default:
        break;
    }

#if ENABLE_BIG_INT
    if (rjs_value_is_big_int(rt, v1) && rjs_value_is_number(rt, v2)) {
        RJS_Number n = rjs_value_get_number(rt, v2);

        r = (rjs_big_int_compare_number(rt, v1, n) == RJS_COMPARE_EQUAL);
        goto end;
    }

    if (rjs_value_is_big_int(rt, v2) && rjs_value_is_number(rt, v1)) {
        RJS_Number n = rjs_value_get_number(rt, v1);

        r = (rjs_big_int_compare_number(rt, v2, n) == RJS_COMPARE_EQUAL);
        goto end;
    }
#endif /*ENABLE_BIG_INT*/

    r = RJS_FALSE;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Check if 2 values are strictly equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 == v2.
 * \retval RJS_FALSE v1 != v2. 
 */
RJS_Bool
rjs_is_strictly_equal (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    if (rjs_value_get_type(rt, v1) != rjs_value_get_type(rt, v2))
        return RJS_FALSE;

    if (rjs_value_is_number(rt, v1))
        return (rjs_number_compare(rt, v1, v2) == RJS_COMPARE_EQUAL);

#if ENABLE_BIG_INT
    if (rjs_value_is_big_int(rt, v1))
        return (rjs_big_int_compare(rt, v1, v2) == RJS_COMPARE_EQUAL);
#endif /*ENABLE_BIG_INT*/

    return rjs_same_value_non_numeric(rt, v1, v2);
}

#if ENABLE_ASYNC

/**Await function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;   /**< Base built-in function object data.*/
    RJS_Context          *ctxt;  /**< The async context.*/
} RJS_AwaitFunc;

/*Scan referenced things in the await function.*/
static void
await_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_AwaitFunc *af = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, ptr);

    if (af->ctxt)
        rjs_gc_mark(rt, af->ctxt);
}

/*Free the await function.*/
static void
await_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_AwaitFunc *af = ptr;

    rjs_builtin_func_object_deinit(rt, &af->bfo);

    RJS_DEL(rt, af);
}

/*Built-in function operation functions.*/
static const RJS_ObjectOps
await_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        await_func_op_gc_scan,
        await_func_op_gc_free
    },
    RJS_BUILTIN_CONSTRUCTOR_OBJECT_OPS
};

/*Await fulfill.*/
static RJS_Result
await_fulfill_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_AwaitFunc *af  = (RJS_AwaitFunc*)rjs_value_get_object(rt, f);
    RJS_Value     *ivp = argc ? rjs_value_get_pointer(rt, args) : NULL;
    RJS_Value     *rvp = rjs_value_get_pointer(rt, rv);

    rjs_context_restore(rt, af->ctxt);

    rjs_script_func_call(rt, RJS_SCRIPT_CALL_ASYNC_FULFILL, ivp, rvp);

    rjs_context_pop(rt);

    return RJS_OK;
}

/*Await reject.*/
static RJS_Result
await_reject_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    RJS_AwaitFunc *af  = (RJS_AwaitFunc*)rjs_value_get_object(rt, f);
    RJS_Value     *ivp = argc ? rjs_value_get_pointer(rt, args) : NULL;
    RJS_Value     *rvp = rjs_value_get_pointer(rt, rv);

    rjs_context_restore(rt, af->ctxt);

    rjs_script_func_call(rt, RJS_SCRIPT_CALL_ASYNC_REJECT, ivp, rvp);

    rjs_context_pop(rt);

    return RJS_OK;
}

/**
 * Await operation.
 * \param rt The current runtime.
 * \param v The value.
 * \param type Await type.
 * \param op The operation function.
 * \param ip The integer parameter of the function.
 * \param vp The value parameter of the function.
 * \retval RJS_SUSPEND On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_await (RJS_Runtime *rt, RJS_Value *v, RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp)
{
    RJS_Context      *ctxt;
    RJS_Result        r;
    RJS_AwaitFunc    *af;
    RJS_Realm        *realm   = rjs_realm_current(rt);
    size_t            top     = rjs_value_stack_save(rt);
    RJS_Value        *promise = rjs_value_stack_push(rt);
    RJS_Value        *fulfill = rjs_value_stack_push(rt);
    RJS_Value        *reject  = rjs_value_stack_push(rt);
    RJS_Value        *rv      = rjs_value_stack_push(rt);

    ctxt = rjs_context_running(rt);

    if ((r = rjs_promise_resolve(rt, rjs_o_Promise(realm), v, promise)) == RJS_ERR)
        goto end;

    RJS_NEW(rt, af);
    af->ctxt = ctxt;
    rjs_init_builtin_function(rt, &af->bfo, await_fulfill_nf, 0, &await_func_ops, 1,
            rjs_s_empty(rt), realm, NULL, NULL, NULL, fulfill);

    RJS_NEW(rt, af);
    af->ctxt = ctxt;
    rjs_init_builtin_function(rt, &af->bfo, await_reject_nf, 0, &await_func_ops, 1,
            rjs_s_empty(rt), realm, NULL, NULL, NULL, reject);

    rjs_perform_proimise_then(rt, promise, fulfill, reject, NULL, rv);

    rjs_async_context_set_op(rt, op, ip, vp);

    r = RJS_SUSPEND;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_ASYNC*/

/**
 * Get the prototype from the constructor.
 * \param rt The current runtime.
 * \param c The constructor.
 * \param[out] p Return the prototype.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_constructor_prototype (RJS_Runtime *rt, RJS_Value *c, RJS_Value *p)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if (rjs_value_is_null(rt, c)) {
        rjs_value_set_null(rt, p);
        rjs_value_copy(rt, c, rjs_o_Function_prototype(realm));
        return RJS_OK;
    }

    if (!rjs_is_constructor(rt, c))
        return rjs_throw_type_error(rt, _("the value is not a constructor"));

    if ((r = rjs_get(rt, c, rjs_pn_prototype(rt), p)) == RJS_ERR)
        return r;

    if (!rjs_value_is_null(rt, p) && !rjs_value_is_object(rt, p))
        return rjs_throw_type_error(rt, _("the prototype is neither an object nor null"));

    return RJS_OK;
}

/**
 * Create a new constructor.
 * \param rt The current runtime.
 * \param proto The prototype of the constructor.
 * \param parent The parent of the constructor.
 * \param script The script.
 * \param func The script function of the constructor.
 * \param[out] c Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_constructor (RJS_Runtime *rt, RJS_Value *proto, RJS_Value *parent,
        RJS_Script *script, RJS_ScriptFunc *func, RJS_Value *c)
{
    RJS_Environment *env      = rjs_lex_env_running(rt);
    RJS_PrivateEnv  *priv_env = NULL;
    RJS_Result       r;

#if ENABLE_PRIV_NAME
    priv_env = rjs_private_env_running(rt);
#endif /*ENABLE_PRIV_NAME*/

    if ((r = rjs_script_func_object_new(rt, c, parent, script, func, env, priv_env)) == RJS_ERR)
        return r;

    rjs_make_method(rt, c, proto);
    rjs_make_constructor(rt, c, RJS_FALSE, proto);

    rjs_create_method_property(rt, proto, rjs_pn_constructor(rt), c);

    return RJS_OK;
}

/*Default constructor.*/
static RJS_NF(default_constructor)
{
    RJS_Result r;

    if (!nt) {
        r = rjs_throw_type_error(rt, _("new target is undefined"));
        goto end;
    }

    if ((r = rjs_ordinary_create_from_constructor(rt, nt, RJS_O_Object_prototype, rv)) == RJS_ERR)
        goto end;

    if ((r = rjs_initialize_instance_elements(rt, rv, f)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    return r;
}

/*Derived default constructor.*/
static RJS_NF(derived_default_constructor)
{
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *fn  = rjs_value_stack_push(rt);
    RJS_Result  r;

    if (!nt) {
        r = rjs_throw_type_error(rt, _("new target is undefined"));
        goto end;
    }

    if ((r = rjs_object_get_prototype_of(rt, f, fn)) == RJS_ERR)
        goto end;

    if (!rjs_is_constructor(rt, fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a constructor"));
        goto end;
    }

    if ((r = rjs_construct(rt, fn, args, argc, nt, rv)) == RJS_ERR)
        goto end;

    if ((r = rjs_initialize_instance_elements(rt, rv, f)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create the default constructor.
 * \param rt The current runtime.
 * \param proto The prototype of the constructor.
 * \param parent The parent of the constructor.
 * \param name The name of the class.
 * \param derived The class is derived.
 * \param[out] c Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_default_constructor (RJS_Runtime *rt, RJS_Value *proto, RJS_Value *parent,
        RJS_Value *name, RJS_Bool derived, RJS_Value *c)
{
    RJS_Realm  *realm = rjs_realm_current(rt);
    RJS_Result  r;

    if (!name)
        name = rjs_s_empty(rt);

    if ((r = rjs_create_builtin_function(rt, NULL,
            derived ? derived_default_constructor : default_constructor,
            0, name, realm, parent, NULL, c)) == RJS_ERR)
        return r;

    rjs_make_constructor(rt, c, RJS_FALSE, proto);
    rjs_create_method_property(rt, proto, rjs_pn_constructor(rt), c);

    return RJS_OK;
}

/**
 * Create a new function.
 * \param rt The current runtime.
 * \param script The script.
 * \param sf The script function.
 * \param env The lexical environment.
 * \param priv_env The private environment.
 * \param is_constr The function is a constructor or not.
 * \param[out] f Return the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_function (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptFunc *sf,
        RJS_Environment *env, RJS_PrivateEnv *priv_env, RJS_Bool is_constr, RJS_Value *f)
{
    RJS_Result r;

#if ENABLE_GENERATOR
    if (sf->flags & RJS_FUNC_FL_GENERATOR) {
        r = rjs_generator_function_new(rt, f, NULL, script, sf, env, priv_env);
    } else
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
    if (sf->flags & RJS_FUNC_FL_ASYNC) {
        r = rjs_async_function_new(rt, f, NULL, script, sf, env, priv_env);
    } else
#endif /*ENABLE_ASYNC*/
    {
        r = rjs_script_func_object_new(rt, f, NULL, script, sf, env, priv_env);
        if (r == RJS_OK) {
            if (is_constr)
                rjs_make_constructor(rt, f, RJS_TRUE, NULL);
        }
    }

    return r;
}

/**
 * Define a field to the object.
 * \param rt The current runtime.
 * \param o The object.
 * \param name The name of the field.
 * \param init The field initializer.
 * \param is_af The initializer is an anonymous function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_define_field (RJS_Runtime *rt, RJS_Value *o, RJS_Value *name, RJS_Value *init, RJS_Bool is_af)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;

    if (!rjs_value_is_undefined(rt, init)) {
        if ((r = rjs_call(rt, init, o, NULL, 0, tmp)) == RJS_ERR)
            goto end;

        if (is_af)
            rjs_set_function_name(rt, tmp, name, NULL);
    } else {
        rjs_value_set_undefined(rt, tmp);
    }

#if ENABLE_PRIV_NAME
    if (rjs_value_is_private_name(rt, name)) {
        r = rjs_private_field_add(rt, o, name, tmp);
    } else
#endif /*ENABLE_PRIV_NAME*/
    {
        RJS_PropertyName pn;

        rjs_property_name_init(rt, &pn, name);
        r = rjs_create_data_property_or_throw(rt, o, &pn, tmp);
        rjs_property_name_deinit(rt, &pn);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Define a method.
 * \param rt The current runtime.
 * \param o The object contains this method.
 * \param proto The prototype value.
 * \param script The script.
 * \param func The script function.
 * \param[out] f Return the method object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_define_method (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto, RJS_Script *script,
        RJS_ScriptFunc *func, RJS_Value *f)
{
    RJS_Environment *env      = rjs_lex_env_running(rt);
    RJS_PrivateEnv  *priv_env = NULL;
    RJS_Result       r;

#if ENABLE_PRIV_NAME
    priv_env = rjs_private_env_running(rt);
#endif /*ENABLE_PRIV_NAME*/

    if ((r = rjs_create_function(rt, script, func, env, priv_env, RJS_FALSE, f)) == RJS_ERR)
        return r;

    if ((r = rjs_make_method(rt, f, o)) == RJS_ERR)
        return r;

    return RJS_OK;
}

/**
 * Define a method property.
 * \param rt The current runtime.
 * \param o The object value.
 * \param n The name of the property.
 * \param f The method function value.
 * \param enumerable The property is enumerable.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_define_method_property (RJS_Runtime *rt, RJS_Value *o, RJS_Value *n, RJS_Value *f, RJS_Bool enumerable)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Result r;

#if ENABLE_PRIV_NAME
    if (rjs_value_is_private_name(rt, n)) {
        r = rjs_private_method_add(rt, o, n, f);
    } else
#endif /*ENABLE_PRIV_NAME*/
    {
        RJS_PropertyDesc pd;
        RJS_PropertyName pn;

        rjs_property_desc_init(rt, &pd);
        rjs_property_name_init(rt, &pn, n);

        pd.flags = RJS_PROP_FL_DATA
                | RJS_PROP_FL_WRITABLE
                | RJS_PROP_FL_CONFIGURABLE
                | (enumerable ? RJS_PROP_FL_ENUMERABLE : 0);

        rjs_value_copy(rt, pd.value, f);

        r = rjs_define_property_or_throw(rt, o, &pn, &pd);
        
        rjs_property_name_deinit(rt, &pn);
        rjs_property_desc_deinit(rt, &pd);
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a method or accessor.
 * \param rt The current runtime.
 * \param o The object contains this method.
 * \param type The element type of the method.
 * \param n The name of the property.
 * \param script The script.
 * \param sf The script function.
 * \param enumerable The property is enumerable.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_method (RJS_Runtime *rt, RJS_Value *o, RJS_ClassElementType type, RJS_Value *n,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Bool enumerable)
{
    size_t            top      = rjs_value_stack_save(rt);
    RJS_Value        *f        = rjs_value_stack_push(rt);
    RJS_Environment  *env      = rjs_lex_env_running(rt);
    RJS_PrivateEnv   *priv_env = NULL;
    RJS_PropertyDesc  pd;
    RJS_PropertyName  pn;
    RJS_Result        r;

#if ENABLE_PRIV_NAME
    priv_env = rjs_private_env_running(rt);
#endif /*ENABLE_PRIV_NAME*/

    switch (type) {
    case RJS_CLASS_ELEMENT_STATIC_GET:
    case RJS_CLASS_ELEMENT_GET:
#if ENABLE_PRIV_NAME
    case RJS_CLASS_ELEMENT_PRIV_GET:
    case RJS_CLASS_ELEMENT_STATIC_PRIV_GET:
#endif /*ENABLE_PRIV_NAME*/
        if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, f)) == RJS_ERR)
            goto end;

        rjs_make_method(rt, f, o);
        rjs_set_function_name(rt, f, n, rjs_s_get(rt));

        rjs_property_desc_init(rt, &pd);
        rjs_property_name_init(rt, &pn, n);

        pd.flags = RJS_PROP_FL_HAS_CONFIGURABLE
                | RJS_PROP_FL_HAS_ENUMERABLE
                | RJS_PROP_FL_HAS_GET
                | RJS_PROP_FL_CONFIGURABLE;

        if (enumerable)
            pd.flags |= RJS_PROP_FL_ENUMERABLE;

        rjs_value_copy(rt, pd.get, f);

        r = rjs_define_property_or_throw(rt, o, &pn, &pd);

        rjs_property_name_deinit(rt, &pn);
        rjs_property_desc_deinit(rt, &pd);

        if (r == RJS_ERR)
            goto end;
        break;
    case RJS_CLASS_ELEMENT_STATIC_SET:
    case RJS_CLASS_ELEMENT_SET:
#if ENABLE_PRIV_NAME
    case RJS_CLASS_ELEMENT_PRIV_SET:
    case RJS_CLASS_ELEMENT_STATIC_PRIV_SET:
#endif /*ENABLE_PRIV_NAME*/
        if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, f)) == RJS_ERR)
            goto end;

        rjs_make_method(rt, f, o);
        rjs_set_function_name(rt, f, n, rjs_s_set(rt));

        rjs_property_desc_init(rt, &pd);
        rjs_property_name_init(rt, &pn, n);

        pd.flags = RJS_PROP_FL_HAS_CONFIGURABLE
                | RJS_PROP_FL_HAS_ENUMERABLE
                | RJS_PROP_FL_HAS_SET
                | RJS_PROP_FL_CONFIGURABLE;

        if (enumerable)
            pd.flags |= RJS_PROP_FL_ENUMERABLE;
            
        rjs_value_copy(rt, pd.set, f);

        r = rjs_define_property_or_throw(rt, o, &pn, &pd);

        rjs_property_name_deinit(rt, &pn);
        rjs_property_desc_deinit(rt, &pd);

        if (r == RJS_ERR)
            goto end;
        break;
    case RJS_CLASS_ELEMENT_METHOD:
    case RJS_CLASS_ELEMENT_STATIC_METHOD:
#if ENABLE_PRIV_NAME
    case RJS_CLASS_ELEMENT_PRIV_METHOD:
    case RJS_CLASS_ELEMENT_STATIC_PRIV_METHOD:
#endif /*ENABLE_PRIV_NAME*/
        if ((r = rjs_define_method(rt, o, NULL, script, sf, f)) == RJS_ERR)
            goto end;

        rjs_set_function_name(rt, f, n, NULL);

        if ((r = rjs_define_method_property(rt, o, n, f, enumerable)) == RJS_ERR)
            goto end;
        break;
    default:
        assert(0);
        break;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Resolve the binding,
 * \param rt The current runtime.
 * \param bn The binding's name.
 * \param[out] pe Return the environment.
 * \retval RJS_TRUE found the binding.
 * \retval RJS_FALSE cannot find the binding.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_resolve_binding (RJS_Runtime *rt, RJS_BindingName *bn, RJS_Environment **pe)
{
    RJS_Environment *env = rjs_lex_env_running(rt);
    RJS_Result       r;

    while (env) {
        if ((r = rjs_env_has_binding(rt, env, bn)) == RJS_ERR)
            return r;
        if (r)
            break;

        env = env->outer;
    }

    if (pe)
        *pe = env;

    return env ? RJS_TRUE : RJS_FALSE;
}

/**
 * Get the binding's value.
 * \param rt The current runtime.
 * \param env The environment contains the binding.
 * \param bn The binding's name.
 * \param strict In strict mode or not.
 * \param[out] bv Return the binding value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Bool strict, RJS_Value *bv)
{
    if (env)
        return rjs_env_get_binding_value(rt, env, bn, strict, bv);

    return rjs_throw_reference_error(rt, _("cannot find binding \"%s\""),
            rjs_string_to_enc_chars(rt, bn->name, NULL, NULL));
}

/**
 * Set the binding's value.
 * \param rt The current runtime.
 * \param env The environment contains this binding.
 * \param bn The binding's name.
 * \param bv The new binding value.
 * \param strict In strict mode or not.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_set_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Value *bv, RJS_Bool strict)
{
    RJS_Realm       *realm = rjs_realm_current(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if (env)
        return rjs_env_set_mutable_binding(rt, env, bn, bv, strict);

    if (strict)
        return rjs_throw_reference_error(rt, _("cannot find binding \"%s\""),
                rjs_string_to_enc_chars(rt, bn->name, NULL, NULL));

    rjs_property_name_init(rt, &pn, bn->name);
    r = rjs_set(rt, rjs_global_object(realm), &pn, bv, RJS_FALSE);
    rjs_property_name_deinit(rt, &pn);

    return r;
}

/**
 * Delete a binding.
 * \param rt The current runtime.
 * \param env The environment contains the binding.
 * \param bn The binding name.
 * \param strict In strict mode or not.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The binding cannot be deleted.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Bool strict)
{
    if (env)
        return rjs_env_delete_binding(rt, env, bn);

    if (strict)
        return rjs_throw_reference_error(rt, _("cannot find binding \"%s\""),
                rjs_string_to_enc_chars(rt, bn->name, NULL, NULL));

    return RJS_TRUE;
}

/**
 * Add entries from the iterable object to an object.
 * \param rt The current runtime.
 * \param target The target object.
 * \param iterable The iterable object.
 * \param fn The add entry function.
 * \param data The parameter ot the add entry function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_add_entries_from_iterable (RJS_Runtime *rt, RJS_Value *target, RJS_Value *iterable,
        RJS_AddEntryFunc fn, void *data)
{
    size_t        top        = rjs_value_stack_save(rt);
    RJS_Value    *ir         = rjs_value_stack_push(rt);
    RJS_Value    *item       = rjs_value_stack_push(rt);
    RJS_Value    *key        = rjs_value_stack_push(rt);
    RJS_Value    *value      = rjs_value_stack_push(rt);
    RJS_Bool      need_close = RJS_FALSE;
    RJS_Iterator  iter;
    RJS_Result    r;

    rjs_iterator_init(rt, &iter);

    if ((r = rjs_get_iterator(rt, iterable, RJS_ITERATOR_SYNC, NULL, &iter)) == RJS_ERR)
        goto end;
    need_close = RJS_TRUE;

    while (1) {
        if ((r = rjs_iterator_step(rt, &iter, ir)) == RJS_ERR)
            goto end;

        if (!r)
            break;

        if ((r = rjs_iterator_value(rt, ir, item)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_object(rt, item)) {
            r = rjs_throw_type_error(rt, _("\"value\" of iterator result is not an object"));
            goto end;
        }

        if ((r = rjs_get_index(rt, item, 0, key)) == RJS_ERR)
            goto end;

        if ((r = rjs_get_index(rt, item, 1, value)) == RJS_ERR)
            goto end;

        if ((r = fn(rt, target, key, 2, data)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    if (need_close && (r == RJS_ERR))
        rjs_iterator_close(rt, &iter);

    rjs_iterator_deinit(rt, &iter);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get the object's species constructor.
 * \param rt The current runtime.
 * \param o The object.
 * \param def The default constructor.
 * \param[out] c Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_species_constructor (RJS_Runtime *rt, RJS_Value *o, RJS_Value *def, RJS_Value *c)
{
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *t   = rjs_value_stack_push(rt);

    if ((r = rjs_get(rt, o, rjs_pn_constructor(rt), t)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, t)) {
        rjs_value_copy(rt, c, def);
        r = RJS_OK;
        goto end;
    }

    if (!rjs_value_is_object(rt, t)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_get(rt, t, rjs_pn_s_species(rt), c)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, c) || rjs_value_is_null(rt, c)) {
        rjs_value_copy(rt, c, def);
        r = RJS_OK;
        goto end;
    }

    if (!rjs_is_constructor(rt, c)) {
        r = rjs_throw_type_error(rt, _("the value is not a constructor"));
        goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * The function return this argument.
 */
RJS_NF(rjs_return_this)
{
    rjs_value_copy(rt, rv, thiz);
    return RJS_OK;
}

/**
 * Get the realm of the function.
 * \param rt The current runtime.
 * \param obj The function object.
 * \return The realm of the function.
 */
RJS_Realm*
rjs_get_function_realm (RJS_Runtime *rt, RJS_Value *obj)
{
    RJS_GcThingType gtt = rjs_value_get_gc_thing_type(rt, obj);
    RJS_Realm      *realm;

    switch (gtt) {
    case RJS_GC_THING_SCRIPT_FUNC: {
        RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, obj);

        realm = sfo->realm;
        break;
    }
    case RJS_GC_THING_BUILTIN_FUNC: {
        RJS_BuiltinFuncObject *bfo = (RJS_BuiltinFuncObject*)rjs_value_get_object(rt, obj);

        realm = bfo->realm;
        break;
    }
#if ENABLE_BOUND_FUNC
    case RJS_GC_THING_BOUND_FUNC: {
        RJS_BoundFuncObject *bfo = (RJS_BoundFuncObject*)rjs_value_get_object(rt, obj);

        return rjs_get_function_realm(rt, &bfo->target_func);
    }
#endif /*ENABLE_BOUND_FUNC*/
#if ENABLE_PROXY
    case RJS_GC_THING_PROXY_OBJECT: {
        RJS_ProxyObject *po = (RJS_ProxyObject*)rjs_value_get_object(rt, obj);

        if (rjs_value_is_null(rt, &po->handler)) {
            rjs_throw_type_error(rt, _("proxy handler is null"));
            return NULL;
        }

        return rjs_get_function_realm(rt, &po->target);
    }
#endif /*ENABLE_PROXY*/
    default:
        realm = rjs_realm_current(rt);
        break;
    }

    return realm;
}

/**
 * Get the prototype from a constructor.
 * \param rt The current runtime.
 * \param constr The constructor.
 * \param dp_idx The default prototype object's index.
 * \param[out] o Return the prototype.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_prototype_from_constructor (RJS_Runtime *rt, RJS_Value *constr, int pd_idx, RJS_Value *proto)
{
    RJS_Result r;

    if (constr) {
        if ((r = rjs_get(rt, constr, rjs_pn_prototype(rt), proto)) == RJS_ERR)
            return r;
    }

    if (!constr || !rjs_value_is_object(rt, proto)) {
        RJS_Realm *realm;

        if (constr) {
            if (!(realm = rjs_get_function_realm(rt, constr)))
                return RJS_ERR;
        } else {
            realm = rjs_realm_current(rt);
        }

        rjs_value_copy(rt, proto, &realm->objects[pd_idx]);
    }

    return RJS_OK;
}

/**
 * Check if the value can be held weakly.
 * \param rt The current runtime.
 * \param v The value to be checked.
 * \retval RJS_TRUE The value can be held weakly.
 * \retval RJS_FALSE The value cannot be held weakly.
 */
RJS_Bool
rjs_can_be_held_weakly (RJS_Runtime *rt, RJS_Value *v)
{
    if (rjs_value_is_object(rt, v))
        return RJS_TRUE;

    if (rjs_value_is_symbol(rt, v)) {
        RJS_Symbol    *sym;
        RJS_HashEntry *he;
        RJS_Result     r;

        sym = rjs_value_get_symbol(rt, v);
        r   = rjs_hash_lookup(&rt->sym_reg_sym_hash, sym, &he, NULL, &rjs_hash_size_ops, rt);
        if (!r)
            return RJS_TRUE;
    }

    return RJS_FALSE;
}
