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

/*Object*/
static RJS_NF(Object_constructor)
{
    RJS_Value *value = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if (nt && !rjs_same_value(rt, nt, f)) {
        r = rjs_ordinary_create_from_constructor(rt, nt, RJS_O_Object_prototype, rv);
    } else if (rjs_value_is_undefined(rt, value) || rjs_value_is_null(rt, value)) {
        r = rjs_ordinary_object_create(rt, rjs_o_Object_prototype(realm), rv);
    } else {
        r = rjs_to_object(rt, value, rv);
    }

    return r;
}

static const RJS_BuiltinFuncDesc
object_constructor_desc = {
    "Object",
    1,
    Object_constructor
};

/*Object.assign*/
static RJS_NF(Object_assign)
{
    RJS_Value  *target = rjs_argument_get(rt, args, argc, 0);
    size_t      top    = rjs_value_stack_save(rt);
    RJS_Value  *to     = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    size_t      aid;
    RJS_Result  r;

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_to_object(rt, target, to)) == RJS_ERR)
        goto end;

    if (argc <= 1) {
        rjs_value_copy(rt, rv, to);
        r = RJS_OK;
        goto end;
    }

    for (aid = 1; aid < argc; aid ++) {
        RJS_Value *src;

        src = rjs_value_buffer_item(rt, args, aid);

        if ((r = rjs_object_assign(rt, to, src)) == RJS_ERR)
            goto end;
    }

    rjs_value_copy(rt, rv, to);
    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Define properties.*/
static RJS_Result
object_define_properties (RJS_Runtime *rt, RJS_Value *o, RJS_Value *props)
{
    size_t               top    = rjs_value_stack_save(rt);
    RJS_Value           *propso = rjs_value_stack_push(rt);
    RJS_Value           *keys   = rjs_value_stack_push(rt);
    RJS_Value           *desco  = rjs_value_stack_push(rt);
    RJS_PropertyKeyList *pkl;
    RJS_PropertyDesc     pd, npd;
    RJS_PropertyName     pn;
    size_t               i;
    RJS_Result           r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_desc_init(rt, &npd);

    if ((r = rjs_to_object(rt, props, propso)) == RJS_ERR)
        goto end;

    if ((r = rjs_object_own_property_keys(rt, propso, keys)) == RJS_ERR)
        goto end;

    pkl = rjs_value_get_gc_thing(rt, keys);
    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_Value *key = &pkl->keys.items[i];

        rjs_property_name_init(rt, &pn, key);

        r = rjs_object_get_own_property(rt, propso, &pn, &pd);
        if ((r = RJS_OK) && (pd.flags & RJS_PROP_FL_ENUMERABLE)) {
            if ((r = rjs_get(rt, propso, &pn, desco)) == RJS_OK) {
                r = rjs_to_property_descriptor(rt, desco, &npd);
                if (r == RJS_OK) {
                    r = rjs_define_property_or_throw(rt, o, &pn, &npd);
                }
            }
        }

        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_property_desc_deinit(rt, &npd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.create*/
static RJS_NF(Object_create)
{
    RJS_Value  *o     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *props = rjs_argument_get(rt, args, argc, 1);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, o) && !rjs_value_is_null(rt, o)) {
        r = rjs_throw_type_error(rt, _("the value is null or undefined"));
        goto end;
    }

    if ((r = rjs_ordinary_object_create(rt, o, rv)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_undefined(rt, props))
        r = object_define_properties(rt, rv, props);
    else
        r = RJS_OK;
end:
    return r;
}

/*Object.defineProperties*/
static RJS_NF(Object_defineProperties)
{
    RJS_Value  *o     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *props = rjs_argument_get(rt, args, argc, 1);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, o)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = object_define_properties(rt, o, props)) == RJS_OK)
        rjs_value_copy(rt, rv, o);
end:
    return r;
}

/*Object.defineProperty*/
static RJS_NF(Object_defineProperty)
{
    RJS_Value       *o     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *p     = rjs_argument_get(rt, args, argc, 1);
    RJS_Value       *attrs = rjs_argument_get(rt, args, argc, 2);
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *key   = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if (!rjs_value_is_object(rt, o)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_property_key(rt, p, key)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_property_descriptor(rt, attrs, &pd)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    r = rjs_define_property_or_throw(rt, o, &pn, &pd);
    rjs_property_name_deinit(rt, &pn);

    if (r == RJS_OK)
        rjs_value_copy(rt, rv, o);
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

#define ENUM_FL_KEY   1
#define ENUM_FL_VALUE 2

/*Create array from the own properties.*/
static RJS_Result
array_from_own_properties (RJS_Runtime *rt, RJS_Value *o, int flags, RJS_Value *a)
{
    size_t               top  = rjs_value_stack_save(rt);
    RJS_Value           *keys = rjs_value_stack_push(rt);
    RJS_Value           *item = rjs_value_stack_push(rt);
    RJS_Value           *pv   = rjs_value_stack_push(rt);
    RJS_PropertyKeyList *pkl;
    RJS_PropertyDesc     pd;
    RJS_PropertyName     pn;
    size_t               i, j;
    RJS_Result           r;

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_object_own_property_keys(rt, o, keys)) == RJS_ERR)
        goto end;

    rjs_array_new(rt, a, 0, NULL);
    j = 0;

    pkl = rjs_value_get_gc_thing(rt, keys);
    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_Value *key = &pkl->keys.items[i];

        if (!rjs_value_is_string(rt, key))
            continue;

        rjs_property_name_init(rt, &pn, key);

        r = rjs_object_get_own_property(rt, o, &pn, &pd);
        if ((r == RJS_OK) && (pd.flags & RJS_PROP_FL_ENUMERABLE)) {
            if (flags == ENUM_FL_KEY) {
                rjs_value_copy(rt, item, key);
            } else {
                if ((r = rjs_get(rt, o, &pn, pv)) == RJS_OK) {
                    if (flags == ENUM_FL_VALUE) {
                        rjs_value_copy(rt, item, pv);
                    } else {
                        r = rjs_create_array_from_elements(rt, item, key, pv, NULL);
                    }
                }
            }

            rjs_create_data_property_or_throw_index(rt, a, j, item);

            j ++;
        }

        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.entries*/
static RJS_NF(Object_entries)
{
    RJS_Value  *o   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *obj = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    r = array_from_own_properties(rt, obj, ENUM_FL_KEY|ENUM_FL_VALUE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.freeze*/
static RJS_NF(Object_freeze)
{
    RJS_Value  *o = rjs_argument_get(rt, args, argc, 0);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, o)) {
        rjs_value_copy(rt, rv, o);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_set_integrity_level(rt, o, RJS_INTEGRITY_FROZEN)) == RJS_ERR)
        goto end;

    if (!r) {
        r = rjs_throw_type_error(rt, _("cannot freeze the object"));
        goto end;
    }

    rjs_value_copy(rt, rv, o);
    r = RJS_OK;
end:
    return r;
}

/*Add entry to the object.*/
static RJS_Result
add_entry (RJS_Runtime *rt, RJS_Value *target, RJS_Value *args, size_t argc, void *data)
{
    RJS_Value       *key   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *value = rjs_argument_get(rt, args, argc, 1);
    RJS_Value       *o     = data;
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *pk    = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((r = rjs_to_property_key(rt, key, pk)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, pk);
    r = rjs_create_data_property_or_throw(rt, o, &pn, value);
    rjs_property_name_deinit(rt, &pn);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.fromEntries*/
static RJS_NF(Object_fromEntries)
{
    RJS_Value  *iter  = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm  *realm = rjs_realm_current(rt);
    RJS_Result  r;

    if ((r = rjs_require_object_coercible(rt, iter)) == RJS_ERR)
        goto end;

    if ((r = rjs_ordinary_object_create(rt, rjs_o_Object_prototype(realm), rv)) == RJS_ERR)
        goto end;

    r = rjs_add_entries_from_iterable(rt, rv, iter, add_entry, rv);
end:
    return r;
}

/*Object.getOwnPropertyDescriptor*/
static RJS_NF(Object_getOwnPropertyDescriptor)
{
    RJS_Value       *o   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *p   = rjs_argument_get(rt, args, argc, 1);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *obj = rjs_value_stack_push(rt);
    RJS_Value       *key = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_property_key(rt, p, key)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    r = rjs_object_get_own_property(rt, obj, &pn, &pd);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    if (!r)
        rjs_value_set_undefined(rt, rv);
    else
        r = rjs_from_property_descriptor(rt, &pd, rv);
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.getOwnPropertyDescriptors*/
static RJS_NF(Object_getOwnPropertyDescriptors)
{
    RJS_Value           *o     = rjs_argument_get(rt, args, argc, 0);
    RJS_Realm           *realm = rjs_realm_current(rt);
    size_t               top   = rjs_value_stack_save(rt);
    RJS_Value           *obj   = rjs_value_stack_push(rt);
    RJS_Value           *keys  = rjs_value_stack_push(rt);
    RJS_Value           *desc  = rjs_value_stack_push(rt);
    size_t               i;
    RJS_PropertyKeyList *pkl;
    RJS_PropertyDesc     pd;
    RJS_PropertyName     pn;
    RJS_Result           r;

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    if ((r = rjs_object_own_property_keys(rt, obj, keys)) == RJS_ERR)
        goto end;

    if ((r = rjs_ordinary_object_create(rt, rjs_o_Object_prototype(realm), rv)) == RJS_ERR)
        goto end;

    pkl = rjs_value_get_gc_thing(rt, keys);
    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_Value *key = &pkl->keys.items[i];

        rjs_property_name_init(rt, &pn, key);
        r = rjs_object_get_own_property(rt, obj, &pn, &pd);
        if (r == RJS_OK) {
            r = rjs_from_property_descriptor(rt, &pd, desc);
            if (r == RJS_OK)
                r = rjs_create_data_property_or_throw(rt, rv, &pn, desc);
        }
        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create array from own keys.*/
static RJS_Result
array_from_own_keys (RJS_Runtime *rt, RJS_Value *o, RJS_ValueType type, RJS_Value *a)
{
    size_t               top  = rjs_value_stack_save(rt);
    RJS_Value           *keys = rjs_value_stack_push(rt);
    RJS_PropertyKeyList *pkl;
    size_t               i, j;
    RJS_Result           r;

    if ((r = rjs_object_own_property_keys(rt, o, keys)) == RJS_ERR)
        goto end;

    rjs_array_new(rt, a, 0, NULL);
    j = 0;

    pkl = rjs_value_get_gc_thing(rt, keys);
    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_Value *key = &pkl->keys.items[i];

        if (!rjs_value_is_string(rt, key) && (type == RJS_VALUE_STRING))
            continue;
        if (!rjs_value_is_symbol(rt, key) && (type == RJS_VALUE_SYMBOL))
            continue;

        rjs_create_data_property_or_throw_index(rt, a, j, key);
        j ++;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.getOwnPropertyNames*/
static RJS_NF(Object_getOwnPropertyNames)
{
    RJS_Value  *o   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *obj = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    r = array_from_own_keys(rt, obj, RJS_VALUE_STRING, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.getOwnPropertySymbols*/
static RJS_NF(Object_getOwnPropertySymbols)
{
    RJS_Value  *o   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *obj = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    r = array_from_own_keys(rt, obj, RJS_VALUE_SYMBOL, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.getPrototypeOf*/
static RJS_NF(Object_getPrototypeOf)
{
    RJS_Value  *o   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *obj = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    r = rjs_object_get_prototype_of(rt, obj, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.hasOwn*/
static RJS_NF(Object_hasOwn)
{
    RJS_Value       *o   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *p   = rjs_argument_get(rt, args, argc, 1);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *obj = rjs_value_stack_push(rt);
    RJS_Value       *key = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_property_key(rt, p, key)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    r = rjs_has_own_property(rt, obj, &pn);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.is*/
static RJS_NF(Object_is)
{
    RJS_Value *v1 = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *v2 = rjs_argument_get(rt, args, argc, 1);
    RJS_Bool   b;

    b = rjs_same_value(rt, v1, v2);

    rjs_value_set_boolean(rt, rv, b);
    return RJS_OK;
}

/*Object.isExtensible*/
static RJS_NF(Object_isExtensible)
{
    RJS_Value *o = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if (!rjs_value_is_object(rt, o)) {
        r = RJS_FALSE;
    } else {
        if ((r = rjs_object_is_extensible(rt, o)) == RJS_ERR)
            return r;
    }

    rjs_value_set_boolean(rt, rv, r);
    return RJS_OK;
}

/*Object.isFrozen*/
static RJS_NF(Object_isFrozen)
{
    RJS_Value *o = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if (!rjs_value_is_object(rt, o)) {
        r = RJS_TRUE;
    } else {
        if ((r = rjs_test_integrity_level(rt, o, RJS_INTEGRITY_FROZEN)) == RJS_ERR)
            return r;
    }

    rjs_value_set_boolean(rt, rv, r);
    return RJS_OK;
}

/*Object.isSealed*/
static RJS_NF(Object_isSealed)
{
    RJS_Value *o = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if (!rjs_value_is_object(rt, o)) {
        r = RJS_TRUE;
    } else {
        if ((r = rjs_test_integrity_level(rt, o, RJS_INTEGRITY_SEALED)) == RJS_ERR)
            return r;
    }

    rjs_value_set_boolean(rt, rv, r);
    return RJS_OK;
}

/*Object.keys*/
static RJS_NF(Object_keys)
{
    RJS_Value  *o   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *obj = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    r = array_from_own_properties(rt, obj, ENUM_FL_KEY, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.preventExtensions*/
static RJS_NF(Object_preventExtensions)
{
    RJS_Value  *o = rjs_argument_get(rt, args, argc, 0);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, o)) {
        rjs_value_copy(rt, rv, o);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_object_prevent_extensions(rt, o)) == RJS_ERR)
        goto end;

    if (!r) {
        r = rjs_throw_type_error(rt, _("cannot prevent extensions of the object"));
        goto end;
    }

    rjs_value_copy(rt, rv, o);
    r = RJS_OK;
end:
    return r;
}

/*Object.seal*/
static RJS_NF(Object_seal)
{
    RJS_Value  *o = rjs_argument_get(rt, args, argc, 0);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, o)) {
        rjs_value_copy(rt, rv, o);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_set_integrity_level(rt, o, RJS_INTEGRITY_SEALED)) == RJS_ERR)
        goto end;

    if (!r) {
        r = rjs_throw_type_error(rt, _("cannot seel the object"));
        goto end;
    }

    rjs_value_copy(rt, rv, o);
    r = RJS_OK;
end:
    return r;
}

/*Object.setPrototypeOf*/
static RJS_NF(Object_setPrototypeOf)
{
    RJS_Value  *o     = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *proto = rjs_argument_get(rt, args, argc, 1);
    RJS_Result  r;

    if ((r = rjs_require_object_coercible(rt, o)) == RJS_ERR)
        return r;

    if (!rjs_value_is_object(rt, proto) && !rjs_value_is_null(rt, proto))
        return rjs_throw_type_error(rt, _("the value is null or undefined"));

    if (!rjs_value_is_object(rt, o)) {
        rjs_value_copy(rt, rv, o);
        return RJS_OK;
    }

    if ((r = rjs_object_set_prototype_of(rt, o, proto)) == RJS_ERR)
        return r;

    if (!r)
        return rjs_throw_type_error(rt, _("cannot set prototype of the object"));

    rjs_value_copy(rt, rv, o);
    return RJS_OK;
}

/*Object.values*/
static RJS_NF(Object_values)
{
    RJS_Value  *o   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *obj = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, o, obj)) == RJS_ERR)
        goto end;

    r = array_from_own_properties(rt, obj, ENUM_FL_VALUE, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
object_function_descs[] = {
    {
        "assign",
        2,
        Object_assign
    },
    {
        "create",
        2,
        Object_create
    },
    {
        "defineProperties",
        2,
        Object_defineProperties
    },
    {
        "defineProperty",
        3,
        Object_defineProperty
    },
    {
        "entries",
        1,
        Object_entries
    },
    {
        "freeze",
        1,
        Object_freeze
    },
    {
        "fromEntries",
        1,
        Object_fromEntries
    },
    {
        "getOwnPropertyDescriptor",
        2,
        Object_getOwnPropertyDescriptor
    },
    {
        "getOwnPropertyDescriptors",
        1,
        Object_getOwnPropertyDescriptors
    },
    {
        "getOwnPropertyNames",
        1,
        Object_getOwnPropertyNames
    },
    {
        "getOwnPropertySymbols",
        1,
        Object_getOwnPropertySymbols
    },
    {
        "getPrototypeOf",
        1,
        Object_getPrototypeOf
    },
    {
        "hasOwn",
        2,
        Object_hasOwn
    },
    {
        "is",
        2,
        Object_is
    },
    {
        "isExtensible",
        1,
        Object_isExtensible
    },
    {
        "isFrozen",
        1,
        Object_isFrozen
    },
    {
        "isSealed",
        1,
        Object_isSealed
    },
    {
        "keys",
        1,
        Object_keys
    },
    {
        "preventExtensions",
        1,
        Object_preventExtensions
    },
    {
        "seal",
        1,
        Object_seal
    },
    {
        "setPrototypeOf",
        2,
        Object_setPrototypeOf
    },
    {
        "values",
        1,
        Object_values
    },
    {NULL}
};

/*Object.prototype.hasOwnProperty*/
static RJS_NF(Object_prototype_hasOwnProperty)
{
    RJS_Value       *v   = rjs_argument_get(rt, args, argc, 0);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *p   = rjs_value_stack_push(rt);
    RJS_Value       *o   = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((r = rjs_to_property_key(rt, v, p)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, p);
    r = rjs_has_own_property(rt, o, &pn);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.prototype.isPrototypeOf*/
static RJS_NF(Object_prototype_isPrototypeOf)
{
    RJS_Value   *v     = rjs_argument_get(rt, args, argc, 0);
    size_t       top   = rjs_value_stack_save(rt);
    RJS_Value   *o     = rjs_value_stack_push(rt);
    RJS_Value   *t     = rjs_value_stack_push(rt);
    RJS_Value   *proto = rjs_value_stack_push(rt);
    RJS_Result   r;

    if (!rjs_value_is_object(rt, v)) {
        r = RJS_FALSE;
    } else {
        if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
            goto end;

        rjs_value_copy(rt, t, v);

        while (1) {
            if ((r = rjs_object_get_prototype_of(rt, t, proto)) == RJS_ERR)
                goto end;

            if (rjs_value_is_null(rt, proto)) {
                r = RJS_FALSE;
                break;
            } else if ((r = rjs_same_value(rt, proto, o))) {
                break;
            }

            rjs_value_copy(rt, t, proto);
        }
    }

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.prototype.propertyIsEnumerable*/
static RJS_NF(Object_prototype_propertyIsEnumerable)
{
    RJS_Value       *v   = rjs_argument_get(rt, args, argc, 0);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *p   = rjs_value_stack_push(rt);
    RJS_Value       *o   = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_to_property_key(rt, v, p)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, p);
    r = rjs_object_get_own_property(rt, o, &pn, &pd);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    if (r)
        r = (pd.flags & RJS_PROP_FL_ENUMERABLE) ? RJS_TRUE : RJS_FALSE;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.prototype.toLocaleString*/
static RJS_NF(Object_prototype_toLocaleString)
{
    return rjs_invoke(rt, thiz, rjs_pn_toString(rt), NULL, 0, rv);
}

/*Object.prototype.toString*/
static RJS_NF(Object_prototype_toString)
{
    size_t         top  = rjs_value_stack_save(rt);
    RJS_Value     *o    = rjs_value_stack_push(rt);
    RJS_Value     *tstr = rjs_value_stack_push(rt);
    const char    *tag  = "Object";
    RJS_CharBuffer cb;
    RJS_Result     r;

    if (rjs_value_is_undefined(rt, thiz)) {
        tag = "Undefined";
    } else if (rjs_value_is_null(rt, thiz)) {
        tag = "Null";
    } else {
        RJS_GcThingType gtt;

        if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
            goto end;

        gtt = rjs_value_get_gc_thing_type(rt, o);

        if ((r = rjs_is_array(rt, o)) == RJS_ERR)
            goto end;

        if (r) {
            tag = "Array";
        } else if (gtt == RJS_GC_THING_ARGUMENTS) {
            tag = "Arguments";
        } else if (rjs_is_callable(rt, o)) {
            tag = "Function";
        } else if (gtt == RJS_GC_THING_PRIMITIVE) {
            RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, o);

            if (rjs_value_is_boolean(rt, &po->value))
                tag = "Boolean";
            else if (rjs_value_is_number(rt, &po->value))
                tag = "Number";
            else if (rjs_value_is_string(rt, &po->value))
                tag = "String";
        } else if (gtt == RJS_GC_THING_ERROR) {
            tag = "Error";
        } else if (gtt == RJS_GC_THING_REGEXP) {
            tag = "RegExp";
        }
#if ENABLE_DATE
        else if (gtt == RJS_GC_THING_DATE) {
            tag = "Date";
        }
#endif /*ENABLE_DATE*/
        if ((r = rjs_get(rt, o, rjs_pn_s_toStringTag(rt), tstr)) == RJS_ERR)
            goto end;

        if (rjs_value_is_string(rt, tstr))
            tag = rjs_string_to_enc_chars(rt, tstr, NULL, 0);
    }

    rjs_char_buffer_init(rt, &cb);
    rjs_char_buffer_append_chars(rt, &cb, "[object ", -1);
    rjs_char_buffer_append_chars(rt, &cb, tag, -1);
    rjs_char_buffer_append_char(rt, &cb, ']');

    rjs_string_from_enc_chars(rt, rv, cb.items, cb.item_num, NULL);
    rjs_char_buffer_deinit(rt, &cb);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.prototype.valueOf*/
static RJS_NF(Object_prototype_valueOf)
{
    return rjs_to_object(rt, thiz, rv);
}

#if ENABLE_LEGACY_OPTIONAL

/*Object.prototype.__defineGetter__*/
static RJS_NF(Object_prototype___defineGetter__)
{
    RJS_Value       *p   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *fn  = rjs_argument_get(rt, args, argc, 1);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *o   = rjs_value_stack_push(rt);
    RJS_Value       *key = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, key);

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    pd.flags = RJS_PROP_FL_CONFIGURABLE
            |RJS_PROP_FL_ENUMERABLE
            |RJS_PROP_FL_HAS_GET
            |RJS_PROP_FL_HAS_CONFIGURABLE
            |RJS_PROP_FL_HAS_ENUMERABLE;
    rjs_value_copy(rt, pd.get, fn);

    if ((r = rjs_to_property_key(rt, p, key)) == RJS_ERR)
        goto end;

    if ((r = rjs_define_property_or_throw(rt, o, &pn, &pd)) == RJS_ERR)
        goto end;

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.prototype.__defineSetter__*/
static RJS_NF(Object_prototype___defineSetter__)
{
    RJS_Value       *p   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *fn  = rjs_argument_get(rt, args, argc, 1);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *o   = rjs_value_stack_push(rt);
    RJS_Value       *key = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, key);

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if (!rjs_is_callable(rt, fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    pd.flags = RJS_PROP_FL_CONFIGURABLE
            |RJS_PROP_FL_ENUMERABLE
            |RJS_PROP_FL_HAS_SET
            |RJS_PROP_FL_HAS_CONFIGURABLE
            |RJS_PROP_FL_HAS_ENUMERABLE;
    rjs_value_copy(rt, pd.set, fn);

    if ((r = rjs_to_property_key(rt, p, key)) == RJS_ERR)
        goto end;

    if ((r = rjs_define_property_or_throw(rt, o, &pn, &pd)) == RJS_ERR)
        goto end;

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.prototype.__lookupGetter__*/
static RJS_NF(Object_prototype___lookupGetter__)
{
    RJS_Value       *p     = rjs_argument_get(rt, args, argc, 0);
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *o     = rjs_value_stack_push(rt);
    RJS_Value       *key   = rjs_value_stack_push(rt);
    RJS_Value       *proto = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, key);

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_property_key(rt, p, key)) == RJS_ERR)
        goto end;

    while (1) {
        if ((r = rjs_object_get_own_property(rt, o, &pn, &pd)) == RJS_ERR)
            goto end;

        if (r) {
            if (rjs_is_accessor_descriptor(&pd)) {
                rjs_value_copy(rt, rv, pd.get);
            } else {
                rjs_value_set_undefined(rt, rv);
            }

            r = RJS_OK;
            break;
        }

        if ((r = rjs_object_get_prototype_of(rt, o, proto)) == RJS_ERR)
            goto end;

        if (rjs_value_is_null(rt, proto)) {
            rjs_value_set_undefined(rt, rv);
            break;
        }

        rjs_value_copy(rt, o, proto);
    }

    r = RJS_OK;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Object.prototype.__lookupSetter__*/
static RJS_NF(Object_prototype___lookupSetter__)
{
    RJS_Value       *p     = rjs_argument_get(rt, args, argc, 0);
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *o     = rjs_value_stack_push(rt);
    RJS_Value       *key   = rjs_value_stack_push(rt);
    RJS_Value       *proto = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, key);

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_property_key(rt, p, key)) == RJS_ERR)
        goto end;

    while (1) {
        if ((r = rjs_object_get_own_property(rt, o, &pn, &pd)) == RJS_ERR)
            goto end;

        if (r) {
            if (rjs_is_accessor_descriptor(&pd)) {
                rjs_value_copy(rt, rv, pd.set);
            } else {
                rjs_value_set_undefined(rt, rv);
            }

            r = RJS_OK;
            break;
        }

        if ((r = rjs_object_get_prototype_of(rt, o, proto)) == RJS_ERR)
            goto end;

        if (rjs_value_is_null(rt, proto)) {
            rjs_value_set_undefined(rt, rv);
            break;
        }

        rjs_value_copy(rt, o, proto);
    }

    r = RJS_OK;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*get Object.prototype.__proto__*/
static RJS_NF(Object_prototype___proto___get)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *o   = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_object(rt, thiz, o)) == RJS_ERR)
        goto end;

    r = rjs_object_get_prototype_of(rt, o, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*set Object.prototype.__proto__*/
static RJS_NF(Object_prototype___proto___set)
{
    RJS_Value *proto = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, thiz)) == RJS_ERR)
        goto end;

    if (!rjs_value_is_object(rt, proto) && !rjs_value_is_null(rt, proto)) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
        goto end;
    }

    if (!rjs_value_is_object(rt, thiz)) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_object_set_prototype_of(rt, thiz, proto)) == RJS_ERR)
        goto end;

    if (!r) {
        r = rjs_throw_type_error(rt, _("cannot set the prototype"));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    return r;
}

#endif /*ENABLE_LEGACY_OPTIONAL*/

static const RJS_BuiltinFuncDesc
object_prototype_function_descs[] = {
    {
        "hasOwnProperty",
        1,
        Object_prototype_hasOwnProperty
    },
    {
        "isPrototypeOf",
        1,
        Object_prototype_isPrototypeOf
    },
    {
        "propertyIsEnumerable",
        1,
        Object_prototype_propertyIsEnumerable
    },
    {
        "toLocaleString",
        0,
        Object_prototype_toLocaleString
    },
    {
        "toString",
        0,
        Object_prototype_toString,
        "Object_prototype_toString"
    },
    {
        "valueOf",
        0,
        Object_prototype_valueOf
    },
#if ENABLE_LEGACY_OPTIONAL
    {
        "__defineGetter__",
        2,
        Object_prototype___defineGetter__
    },
    {
        "__defineSetter__",
        2,
        Object_prototype___defineSetter__
    },
    {
        "__lookupGetter__",
        1,
        Object_prototype___lookupGetter__
    },
    {
        "__lookupSetter__",
        1,
        Object_prototype___lookupSetter__
    },
#endif /*ENABLE_LEGACY_OPTIONAL*/
    {NULL}
};

static const RJS_BuiltinAccessorDesc
object_prototype_accessot_descs[] = {
#if ENABLE_LEGACY_OPTIONAL
    {
        "__proto__",
        Object_prototype___proto___get,
        Object_prototype___proto___set
    },
#endif /*ENABLE_LEGACY_OPTIONAL*/
    {NULL}
};

static const RJS_BuiltinObjectDesc
object_prototype_desc = {
    "Object",
    NULL,
    NULL,
    NULL,
    NULL,
    object_prototype_function_descs,
    object_prototype_accessot_descs,
    NULL,
    "Object_prototype"
};
