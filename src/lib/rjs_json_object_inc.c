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
json_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "JSON",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Internalize the JSON property.*/
static RJS_Result
internalize_json_property (RJS_Runtime *rt, RJS_Value *holder, RJS_PropertyName *pn,
        RJS_Value *reviver, RJS_Value *rv)
{
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *name  = rjs_value_stack_push(rt);
    RJS_Value       *val   = rjs_value_stack_push(rt);
    RJS_Value       *idx   = rjs_value_stack_push(rt);
    RJS_Value       *key   = rjs_value_stack_push(rt);
    RJS_Value       *nelem = rjs_value_stack_push(rt);
    RJS_Value       *keys  = rjs_value_stack_push(rt);
    RJS_PropertyName cpn;
    RJS_PropertyDesc pd;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    if ((r = rjs_get(rt, holder, pn, val)) == RJS_ERR)
        goto end;

    if (rjs_value_is_object(rt, val)) {
        if (rjs_is_array(rt, val)) {
            int64_t i, len;

            if ((r = rjs_length_of_array_like(rt, val, &len)) == RJS_ERR)
                goto end;

            for (i = 0; i < len; i ++) {
                rjs_value_set_number(rt, idx, i);
                rjs_to_string(rt, idx, key);

                rjs_property_name_init(rt, &cpn, key);
                r = internalize_json_property(rt, val, &cpn, reviver, nelem);
                if (r == RJS_OK) {
                    if (rjs_value_is_undefined(rt, nelem)) {
                        r = rjs_object_delete(rt, val, &cpn);
                    } else {
                        r = rjs_create_data_property(rt, val, &cpn, nelem);
                    }
                }
                rjs_property_name_deinit(rt, &cpn);
                if (r == RJS_ERR)
                    goto end;
            }
        } else {
            RJS_PropertyKeyList *pkl;
            size_t               i;

            if ((r = rjs_object_own_property_keys(rt, val, keys)) == RJS_ERR)
                goto end;

            pkl = (RJS_PropertyKeyList*)rjs_value_get_gc_thing(rt, keys);
            for (i = 0; i < pkl->keys.item_num; i ++) {
                RJS_Value *k = &pkl->keys.items[i];

                if (!rjs_value_is_string(rt, k))
                    continue;

                rjs_property_name_init(rt, &cpn, k);
                r = rjs_object_get_own_property(rt, val, &cpn, &pd);
                if ((r != RJS_OK) || !(pd.flags & RJS_PROP_FL_ENUMERABLE))
                    rjs_value_set_undefined(rt, k);
                rjs_property_name_deinit(rt, &cpn);
                if (r == RJS_ERR)
                    goto end;
            }

            for (i = 0; i < pkl->keys.item_num; i ++) {
                RJS_Value *k = &pkl->keys.items[i];

                if (!rjs_value_is_string(rt, k))
                    continue;

                rjs_property_name_init(rt, &cpn, k);

                r = internalize_json_property(rt, val, &cpn, reviver, nelem);
                if (r == RJS_OK) {
                    if (rjs_value_is_undefined(rt, nelem)) {
                        r = rjs_object_delete(rt, val, &cpn);
                    } else {
                        r = rjs_create_data_property(rt, val, &cpn, nelem);
                    }
                }
                rjs_property_name_deinit(rt, &cpn);
                if (r == RJS_ERR)
                    goto end;
            }
        }
    }

    rjs_value_copy(rt, name, pn->name);

    r = rjs_call(rt, reviver, holder, name, 2, rv);
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*JSON.parse*/
static RJS_NF(JSON_parse)
{
    RJS_Value  *text    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *reviver = rjs_argument_get(rt, args, argc, 1);
    size_t      top     = rjs_value_stack_save(rt);
    RJS_Value  *str     = rjs_value_stack_push(rt);
    RJS_Value  *json    = rjs_value_stack_push(rt);
    RJS_Value  *root    = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_string(rt, text, str)) == RJS_ERR)
        goto end;

    if ((r = rjs_json_from_string(rt, json, str)) == RJS_ERR) {
        r = rjs_throw_syntax_error(rt, _("JSON parse error"));
        goto end;
    }

    if (rjs_is_callable(rt, reviver)) {
        RJS_PropertyName pn;

        rjs_ordinary_object_create(rt, NULL, root);
        rjs_property_name_init(rt, &pn, rjs_s_empty(rt));
        rjs_create_data_property_or_throw(rt, root, &pn, json);
        r = internalize_json_property(rt, root, &pn, reviver, rv);
        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    } else {
        rjs_value_copy(rt, rv, json);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*JSON.stringify*/
static RJS_NF(JSON_stringify)
{
    RJS_Value *value    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *replacer = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *space    = rjs_argument_get(rt, args, argc, 2);

    return rjs_json_stringify(rt, value, replacer, space, rv);
}

static const RJS_BuiltinFuncDesc
json_function_descs[] = {
    {
        "parse",
        2,
        JSON_parse
    },
    {
        "stringify",
        3,
        JSON_stringify
    },
    {NULL}
};
