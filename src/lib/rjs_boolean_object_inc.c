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

/*Boolean*/
static RJS_NF(Boolean_constructor)
{
    RJS_Value *v     = rjs_argument_get(rt, args, argc, 0);
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *prim  = rjs_value_stack_push(rt);
    RJS_Bool   b;
    RJS_Result r;

    b = rjs_to_boolean(rt, v);

    if (!nt) {
        rjs_value_set_boolean(rt, rv, b);
    } else {
        rjs_value_set_boolean(rt, prim, b);

        if ((r = rjs_primitive_object_new(rt, rv, nt, RJS_O_Boolean_prototype, prim)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
boolean_constructor_desc = {
    "Boolean",
    1,
    Boolean_constructor
};

/*Get this boolean value.*/
static RJS_Result
this_boolean_value (RJS_Runtime *rt, RJS_Value *thiz)
{
    RJS_Result r;

    if (rjs_value_is_boolean(rt, thiz)) {
        r = rjs_value_get_boolean(rt, thiz);
    } else if (rjs_value_is_object(rt, thiz)
            && (rjs_value_get_gc_thing_type(rt, thiz) == RJS_GC_THING_PRIMITIVE)) {
        RJS_PrimitiveObject *po = (RJS_PrimitiveObject*)rjs_value_get_object(rt, thiz);

        if (!rjs_value_is_boolean(rt, &po->value))
            return rjs_throw_type_error(rt, _("this is not a boolean value"));

        r = rjs_value_get_boolean(rt, &po->value);
    } else {
        return rjs_throw_type_error(rt, _("this is not a boolean value"));
    }

    return r;
}

/*Boolean.prototype.toString*/
static RJS_NF(Boolean_prototype_toString)
{
    RJS_Result r;

    if ((r = this_boolean_value(rt, thiz)) == RJS_ERR)
        return r;

    if (r)
        rjs_value_copy(rt, rv, rjs_s_true(rt));
    else
        rjs_value_copy(rt, rv, rjs_s_false(rt));

    return RJS_OK;
}

/*Boolean.prototype.valueOf*/
static RJS_NF(Boolean_prototype_valueOf)
{
    RJS_Result r;

    if ((r = this_boolean_value(rt, thiz)) == RJS_ERR)
        return r;

    rjs_value_set_boolean(rt, rv, r);

    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
boolean_prototype_function_descs[] = {
    {
        "toString",
        0,
        Boolean_prototype_toString
    },
    {
        "valueOf",
        0,
        Boolean_prototype_valueOf
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
boolean_prototype_desc = {
    "Boolean",
    NULL,
    NULL,
    NULL,
    NULL,
    boolean_prototype_function_descs,
    NULL,
    NULL,
    "Boolean_prototype"
};
