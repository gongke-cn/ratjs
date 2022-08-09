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

static const RJS_BuiltinFieldDesc
reflect_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Reflect",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Convert the array to arguments.*/
static RJS_Result
array_to_args (RJS_Runtime *rt, RJS_Value *arg_list, RJS_Value **pargs, size_t *pargc)
{
    RJS_Value *args = NULL;
    RJS_Result r;
    int64_t   len, i;

    if (!rjs_value_is_object(rt, arg_list)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_length_of_array_like(rt, arg_list, &len)) == RJS_ERR)
        goto end;

    if (len) {
        args = rjs_value_stack_push_n(rt, len);

        for (i = 0; i < len; i ++) {
            RJS_Value *arg = rjs_value_buffer_item(rt, args, i);

            if ((r = rjs_get_index(rt, arg_list, i, arg)) == RJS_ERR)
                goto end;
        }
    } else {
        args = NULL;
    }

    *pargs = args;
    *pargc = len;

    r = RJS_OK;
end:
    return r;
}

/*Reflect.apply*/
static RJS_NF(Reflect_apply)
{
    RJS_Value *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *this_arg = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *arg_list = rjs_argument_get(rt, args, argc, 2);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *rargs    = NULL;
    size_t     rargc    = 0;
    RJS_Result r;

    if (!rjs_is_callable(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not a function"));
        goto end;
    }

    if ((r = array_to_args(rt, arg_list, &rargs, &rargc)) == RJS_ERR)
        goto end;

    r = rjs_call(rt, target, this_arg, rargs, rargc, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.construct*/
static RJS_NF(Reflect_construct)
{
    RJS_Value *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *arg_list = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *rnt      = rjs_argument_get(rt, args, argc, 2);
    size_t     top      = rjs_value_stack_save(rt);
    RJS_Value *rargs    = NULL;
    size_t     rargc    = 0;
    RJS_Result r;

    if (!rjs_is_constructor(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not a constructor"));
        goto end;
    }

    if (argc < 3) {
        rnt = target;
    } else if (!rjs_is_constructor(rt, rnt)) {
        r = rjs_throw_type_error(rt, _("the value is not a constructor"));
        goto end;
    }

    if ((r = array_to_args(rt, arg_list, &rargs, &rargc)) == RJS_ERR)
        goto end;

    if (rjs_value_is_undefined(rt, rnt))
        rnt = NULL;
        
    r = rjs_construct(rt, target, rargs, rargc, rnt, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.defineProperty*/
static RJS_NF(Reflect_defineProperty)
{
    RJS_Value       *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *prop_key = rjs_argument_get(rt, args, argc, 1);
    RJS_Value       *attrs    = rjs_argument_get(rt, args, argc, 2);
    size_t           top      = rjs_value_stack_save(rt);
    RJS_Value       *key      = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_property_key(rt, prop_key, key)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_property_descriptor(rt, attrs, &pd)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    r = rjs_object_define_own_property(rt, target, &pn, &pd);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.deleteProperty*/
static RJS_NF(Reflect_deleteProperty)
{
    RJS_Value       *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *prop_key = rjs_argument_get(rt, args, argc, 1);
    size_t           top      = rjs_value_stack_save(rt);
    RJS_Value       *key      = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_property_key(rt, prop_key, key)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    r = rjs_object_delete(rt, target, &pn);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.get*/
static RJS_NF(Reflect_get)
{
    RJS_Value       *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *prop_key = rjs_argument_get(rt, args, argc, 1);
    size_t           top      = rjs_value_stack_save(rt);
    RJS_Value       *key      = rjs_value_stack_push(rt);
    RJS_Value       *receiver;
    RJS_PropertyName pn;
    RJS_Result       r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_property_key(rt, prop_key, key)) == RJS_ERR)
        goto end;

    if (argc < 3) {
        receiver = target;
    } else {
        receiver = rjs_value_buffer_item(rt, args, 2);
    }

    rjs_property_name_init(rt, &pn, key);
    r = rjs_object_get(rt, target, &pn, receiver, rv);
    rjs_property_name_deinit(rt, &pn);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.getOwnPropertyDescriptor*/
static RJS_NF(Reflect_getOwnPropertyDescriptor)
{
    RJS_Value       *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *prop_key = rjs_argument_get(rt, args, argc, 1);
    size_t           top      = rjs_value_stack_save(rt);
    RJS_Value       *key      = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_property_key(rt, prop_key, key)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    r = rjs_object_get_own_property(rt, target, &pn, &pd);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    if (!r)
        rjs_value_set_undefined(rt, rv);
    else
        rjs_from_property_descriptor(rt, &pd, rv);
        
    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.getPrototypeOf*/
static RJS_NF(Reflect_getPrototypeOf)
{
    RJS_Value  *target = rjs_argument_get(rt, args, argc, 0);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    r = rjs_object_get_prototype_of(rt, target, rv);
end:
    return r;
}

/*Reflect.has*/
static RJS_NF(Reflect_has)
{
    RJS_Value       *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *prop_key = rjs_argument_get(rt, args, argc, 1);
    size_t           top      = rjs_value_stack_save(rt);
    RJS_Value       *key      = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_property_key(rt, prop_key, key)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    r = rjs_object_has_property(rt, target, &pn);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.isExtensible*/
static RJS_NF(Reflect_isExtensible)
{
    RJS_Value  *target = rjs_argument_get(rt, args, argc, 0);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_object_is_extensible(rt, target)) == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    return r;
}

/*Reflect.ownKeys*/
static RJS_NF(Reflect_ownKeys)
{
    RJS_Value           *target = rjs_argument_get(rt, args, argc, 0);
    size_t               top    = rjs_value_stack_save(rt);
    RJS_Value           *keys   = rjs_value_stack_push(rt);
    RJS_PropertyKeyList *pkl;
    size_t               i;
    RJS_Result           r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_object_own_property_keys(rt, target, keys)) == RJS_ERR)
        goto end;

    pkl = rjs_value_get_gc_thing(rt, keys);

    rjs_array_new(rt, rv, 0, NULL);

    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_Value *key = &pkl->keys.items[i];

        rjs_create_data_property_or_throw_index(rt, rv, i, key);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.preventExtensions*/
static RJS_NF(Reflect_preventExtensions)
{
    RJS_Value  *target = rjs_argument_get(rt, args, argc, 0);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_object_prevent_extensions(rt, target)) == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    return r;
}

/*Reflect.set*/
static RJS_NF(Reflect_set)
{
    RJS_Value       *target   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *prop_key = rjs_argument_get(rt, args, argc, 1);
    RJS_Value       *v        = rjs_argument_get(rt, args, argc, 2);
    size_t           top      = rjs_value_stack_save(rt);
    RJS_Value       *key      = rjs_value_stack_push(rt);
    RJS_Value       *receiver;
    RJS_PropertyName pn;
    RJS_Result       r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if ((r = rjs_to_property_key(rt, prop_key, key)) == RJS_ERR)
        goto end;

    if (argc < 4) {
        receiver = target;
    } else {
        receiver = rjs_value_buffer_item(rt, args, 3);
    }

    rjs_property_name_init(rt, &pn, key);
    r = rjs_object_set(rt, target, &pn, v, receiver);
    rjs_property_name_deinit(rt, &pn);
    if (r == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Reflect.setPrototypeOf*/
static RJS_NF(Reflect_setPrototypeOf)
{
    RJS_Value  *target = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *proto  = rjs_argument_get(rt, args, argc, 1);
    RJS_Result  r;

    if (!rjs_value_is_object(rt, target)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    if (!rjs_value_is_object(rt, proto) && !rjs_value_is_null(rt, proto)) {
        r = rjs_throw_type_error(rt, _("the prototype must be an object or null"));
        goto end;
    }

    r = rjs_object_set_prototype_of(rt, target, proto);
    if (r == RJS_ERR)
        goto end;

    rjs_value_set_boolean(rt, rv, r);
    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinFuncDesc
reflect_function_descs[] = {
    {
        "apply",
        3,
        Reflect_apply
    },
    {
        "construct",
        2,
        Reflect_construct
    },
    {
        "defineProperty",
        3,
        Reflect_defineProperty
    },
    {
        "deleteProperty",
        2,
        Reflect_deleteProperty
    },
    {
        "get",
        2,
        Reflect_get
    },
    {
        "getOwnPropertyDescriptor",
        2,
        Reflect_getOwnPropertyDescriptor
    },
    {
        "getPrototypeOf",
        1,
        Reflect_getPrototypeOf
    },
    {
        "has",
        2,
        Reflect_has
    },
    {
        "isExtensible",
        1,
        Reflect_isExtensible
    },
    {
        "ownKeys",
        1,
        Reflect_ownKeys
    },
    {
        "preventExtensions",
        1,
        Reflect_preventExtensions
    },
    {
        "set",
        3,
        Reflect_set
    },
    {
        "setPrototypeOf",
        2,
        Reflect_setPrototypeOf
    },
    {NULL}
};
