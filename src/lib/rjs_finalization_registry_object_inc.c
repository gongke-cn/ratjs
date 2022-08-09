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

/*FinalizationRegistry*/
static RJS_NF(FinalizationRegistry_constructor)
{
    RJS_Value *func = rjs_argument_get(rt, args, argc, 0);

    return rjs_finalization_registry_new(rt, rv, nt, func);
}

static const RJS_BuiltinFuncDesc
finalization_registry_constructor_desc = {
    "FinalizationRegistry",
    1,
    FinalizationRegistry_constructor
};

static const RJS_BuiltinFieldDesc
finalization_registry_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "FinalizationRegistry",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*FinalizationRegistry.prototype.register*/
static RJS_NF(FinalizationRegistry_prototype_register)
{
    RJS_Value *target = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *held   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value *token  = rjs_argument_get(rt, args, argc, 2);
    RJS_Result r;

    if ((r = rjs_finalization_register(rt, thiz, target, held, token)) == RJS_ERR)
        return r;

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/*FinalizationRegistry.prototype.unregister*/
static RJS_NF(FinalizationRegistry_prototype_unregister)
{
    RJS_Value *token = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if ((r = rjs_finalization_unregister(rt, thiz, token)) == RJS_ERR)
        return r;

    rjs_value_set_boolean(rt, rv, r);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
finalization_registry_prototype_function_descs[] = {
    {
        "register",
        2,
        FinalizationRegistry_prototype_register
    },
    {
        "unregister",
        1,
        FinalizationRegistry_prototype_unregister
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
finalization_registry_prototype_desc = {
    "FinalizationRegistry",
    NULL,
    NULL,
    NULL,
    finalization_registry_prototype_field_descs,
    finalization_registry_prototype_function_descs,
    NULL,
    NULL,
    "FinalizationRegistry_prototype"
};