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

/*AsyncGeneratorFunction.*/
static RJS_NF(AsyncGeneratorFunction_constructor)
{
    return rjs_create_dynamic_function(rt, f, nt, RJS_FUNC_FL_ASYNC|RJS_FUNC_FL_GENERATOR, args, argc, rv);
}

static const RJS_BuiltinFuncDesc
async_generator_function_constructor_desc = {
    "AsyncGeneratorFunction",
    1,
    AsyncGeneratorFunction_constructor
};

static const RJS_BuiltinFieldDesc
async_generator_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "AsyncGenerator",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
async_generator_function_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "AsyncGeneratorFunction",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*AsyncGenerator.prototype.next*/
static RJS_NF(AsyncGenerator_prototype_next)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);

    return rjs_async_generator_next(rt, thiz, v, rv);
}

/*AsyncGenerator.prototype.return*/
static RJS_NF(AsyncGenerator_prototype_return)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);

    return rjs_async_generator_return(rt, thiz, v, rv);
}

/*AsyncGenerator.prototype.throw*/
static RJS_NF(AsyncGenerator_prototype_throw)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);

    return rjs_async_generator_throw(rt, thiz, v, rv);
}

static const RJS_BuiltinFuncDesc
async_generator_prototype_function_descs[] = {
    {
        "next",
        1,
        AsyncGenerator_prototype_next
    },
    {
        "return",
        1,
        AsyncGenerator_prototype_return
    },
    {
        "throw",
        1,
        AsyncGenerator_prototype_throw
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
async_generator_prototype_desc = {
    "AsyncGenerator",
    "AsyncIteratorPrototype",
    NULL,
    NULL,
    async_generator_prototype_field_descs,
    async_generator_prototype_function_descs,
    NULL,
    NULL,
    "AsyncGenerator_prototype"
};

static const RJS_BuiltinObjectDesc
async_generator_function_prototype_desc = {
    "AsyncGeneratorFunction",
    "Function_prototype",
    NULL,
    &async_generator_prototype_desc,
    async_generator_function_prototype_field_descs,
    NULL,
    NULL,
    NULL,
    "AsyncGeneratorFunction_prototype"
};
