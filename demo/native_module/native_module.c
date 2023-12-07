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

/*This program demonstrates how to write a native module.*/

/*
 * Native module is a dynamic library implement 2 functions:
 * "ratjs_module_init" and "ratjs_module_exec".
 */

#include <ratjs.h>

/*Local export entries.*/
static const RJS_ModuleExportDesc
local_exports[] = {
    /*"test"*/
    {NULL, NULL, "test", "test"},
    /*Default export*/
    {NULL, NULL, "*default*", "default"},
    {NULL, NULL, NULL, NULL}
};

/*
 * "ratjs_module_init" initialize the native module.
 * Create import and export entries here.
 */
RJS_Result
ratjs_module_init (RJS_Runtime *rt, RJS_Value *mod)
{
    printf("native module initialize\n");

    /*Add export entries.*/
    rjs_module_set_import_export(rt, mod, NULL, local_exports, NULL, NULL);

    return RJS_OK;
}

static RJS_NF(test_func)
{
    printf("test invoked!\n");

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

static RJS_NF(default_func)
{
    printf("default invoked!\n");

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/*
 * "ratjs_module_exec" execute the native module.
 * Initialize the local binding values here.
 */
RJS_Result
ratjs_module_exec (RJS_Runtime *rt, RJS_Value *mod)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *func = rjs_value_stack_push(rt);
    RJS_Value *name = rjs_value_stack_push(rt);

    printf("native module execute\n");

    /*Add "test" binding.*/
    rjs_string_from_enc_chars(rt, name, "test", -1, NULL);
    rjs_create_builtin_function(rt, mod, test_func, 0, name, NULL, NULL, NULL, func);
    rjs_module_add_binding(rt, mod, name, func);

    /*Add default export binding.*/
    rjs_create_builtin_function(rt, mod, default_func, 0, NULL, NULL, NULL, NULL, func);
    rjs_string_from_enc_chars(rt, name, "*default*", -1, NULL);
    rjs_module_add_binding(rt, mod, name, func);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}
