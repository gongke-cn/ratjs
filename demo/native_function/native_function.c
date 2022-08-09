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

/*This program demonstrates how to extend scripts with native functions.*/

#include <ratjs.h>

/*Script source.*/
static const char*
source =
"native_function(\"*\", 1, 10);"
"native_function(\"^\", 10, 1);"
"native_function(\"$\", 5, 8);"
"native_function(\"#\", 7, 5);"
;

/*The native function.*/
static RJS_NF(native_function)
{
    /*Get argument 1.*/
    RJS_Value     *arg1 = rjs_argument_get(rt, args, argc, 0);
    /*Get argument 2.*/
    RJS_Value     *arg2 = rjs_argument_get(rt, args, argc, 1);
    /*Get argument 3.*/
    RJS_Value     *arg3 = rjs_argument_get(rt, args, argc, 2);
    /*Store the native value stack's pointer.*/
    size_t         top  = rjs_value_stack_save(rt);
    /*Allocate temporary value.*/
    RJS_Value     *str  = rjs_value_stack_push(rt);
    /*Character buffer to store the NULL terminated C string.*/
    RJS_CharBuffer cb;
    /*NULL terminated C string's pointer.*/
    const char    *cstr;
    int64_t        from, to, step, i;
    RJS_Result     r;

    /*Initialize the character buffer.*/
    rjs_char_buffer_init(rt, &cb);

    /*Convert argument 1 to string.*/
    if ((r = rjs_to_string(rt, arg1, str)) == RJS_ERR)
        goto end;

    /*Convert the string to NULL terminated C string.*/
    cstr = rjs_string_to_enc_chars(rt, str, &cb, NULL);

    if (argc > 1) {
        /*Convert the argument 2 to integer >= 0.*/
        if ((r = rjs_to_length(rt, arg2, &from)) == RJS_ERR)
            goto end;
    } else {
        from = 1;
    }

    if (argc > 2) {
        /*Convert the argument 3 to integer >= 0.*/
        if ((r = rjs_to_length(rt, arg3, &to)) == RJS_ERR)
            goto end;
    } else {
        to = from;
    }

    if (to >= from)
        step = 1;
    else
        step = -1;

    i = from;
    while (1) {
        int64_t j;

        for (j = 0; j < i; j ++) {
            printf("%s", cstr);
        }
        printf("\n");

        if (i == to)
            break;

        i += step;
    }

    r = RJS_OK;
end:
    /*Release the character buffer.*/
    rjs_char_buffer_deinit(rt, &cb);
    /*Restore the old native value stack top to release all temporary values.*/
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Add native functions.*/
static void
add_native (RJS_Runtime *rt)
{
    RJS_Realm *realm;
    RJS_Value *global;
    RJS_PropertyName pn;
    /*Store the native value stack's pointer.*/
    size_t     top  = rjs_value_stack_save(rt);
    /*Allocate temporary value to store the function name.*/
    RJS_Value *name = rjs_value_stack_push(rt);
    /*Allocate temporary value to store the function.*/
    RJS_Value *func = rjs_value_stack_push(rt);

    /*Get the current realm.*/
    realm = rjs_realm_current(rt);

    /*Get the global object of the current realm.*/
    global = rjs_global_object(realm);

    /*Create the function's name.*/
    rjs_string_from_enc_chars(rt, name, "native_function", -1, NULL);

    /*Create the function(argument count = 3, name = "native_function").*/
    rjs_create_builtin_function(rt, NULL, native_function, 3, name, NULL, NULL, NULL, func);

    /*Initialize the property name "native_function".*/
    rjs_property_name_init(rt, &pn, name);

    /*Add the function to the global object with property name.*/
    rjs_create_data_property(rt, global, &pn, func);

    /*Release the property name.*/
    rjs_property_name_deinit(rt, &pn);

    /*Restore the old native value stack top to release all temporary values.*/
    rjs_value_stack_restore(rt, top);
}

int
main (int argc, char **argv)
{
    RJS_Runtime *rt;
    RJS_Value   *str, *script;
    RJS_Result   r;
    int          rc = 1;

    /*Create the JS runtime.*/
    rt = rjs_runtime_new();

    /*Add native functions.*/
    add_native(rt);

    /*Allocate the string value to store the source of the script.*/
    str = rjs_value_stack_push(rt);

    /*Create the source string.*/
    rjs_string_from_enc_chars(rt, str, source, -1, NULL);

    /*Allocate the string value to store the script.*/
    script = rjs_value_stack_push(rt);

    /*Load the script.*/
    r = rjs_script_from_string(rt, script, str, NULL, RJS_FALSE);
    if (r == RJS_ERR) {
        fprintf(stderr, "parse script faled\n");
        goto end;
    }

    /*Run the script.*/
    r = rjs_script_evaluation(rt, script, NULL);
    if (r == RJS_ERR) {
        fprintf(stderr, "execure the script failed\n");
        goto end;
    }

    rc = 0;

end:
    /*Free the JS runtime.*/
    rjs_runtime_free(rt);
    return rc;
}
