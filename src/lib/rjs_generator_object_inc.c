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

/*GeneratorFunction.*/
static RJS_NF(GeneratorFunction_constructor)
{
    return rjs_create_dynamic_function(rt, f, nt, RJS_FUNC_FL_GENERATOR, args, argc, rv);
}

static const RJS_BuiltinFuncDesc
generator_function_constructor_desc = {
    "GeneratorFunction",
    1,
    GeneratorFunction_constructor
};

static const RJS_BuiltinFieldDesc
generator_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Generator",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
generator_function_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "GeneratorFunction",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Generator.prototype.next*/
static RJS_NF(Generator_prototype_next)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);

    return rjs_generator_resume(rt, thiz, v, rjs_s_empty(rt), rv);
}

/*Generator.prototype.return*/
static RJS_NF(Generator_prototype_return)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);

    return rjs_generator_resume_abrupt(rt, thiz, RJS_GENERATOR_ABRUPT_RETURN, v,
            rjs_s_empty(rt), rv);
}

/*Generator.prototype.throw*/
static RJS_NF(Generator_prototype_throw)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);

    return rjs_generator_resume_abrupt(rt, thiz, RJS_GENERATOR_ABRUPT_THROW, v,
            rjs_s_empty(rt), rv);
}

static const RJS_BuiltinFuncDesc
generator_prototype_function_descs[] = {
    {
        "next",
        1,
        Generator_prototype_next
    },
    {
        "return",
        1,
        Generator_prototype_return
    },
    {
        "throw",
        1,
        Generator_prototype_throw
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
generator_prototype_desc = {
    "Generator",
    "IteratorPrototype",
    NULL,
    NULL,
    generator_prototype_field_descs,
    generator_prototype_function_descs,
    NULL,
    NULL,
    "Generator_prototype"
};

static const RJS_BuiltinObjectDesc
generator_function_prototype_desc = {
    "GeneratorFunction",
    "Function_prototype",
    NULL,
    &generator_prototype_desc,
    generator_function_prototype_field_descs,
    NULL,
    NULL,
    NULL,
    "GeneratorFunction_prototype"
};
