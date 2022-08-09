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

/*
 * This program demonstrates how to call the libratjs to execute a script.
 */

#include <stdlib.h>
#include <assert.h>

/*Include "ratjs.h".*/
#include <ratjs.h>

/*The source pattern of the script.*/
static const char
source_pattern[] = "let sum = 0; for (let i = 0; i <= %d; i ++) sum += i; sum;";

int
main (int argc, char **argv)
{
    RJS_Runtime *rt;
    size_t       top;
    RJS_Value   *strv, *scriptv, *retv;
    RJS_Result   r;
    char         source[256];
    int          loop = 100;
    int          rc   = 1;

    /*Load the loop count from the argument.*/
    if (argc > 1) {
        loop = strtol(argv[1], NULL, 0);
    }

    /*First create a ratjs runtime.*/
    rt = rjs_runtime_new();

    /*
     * Save the native value stack's top pointer.
     * Then you can use "rjs_value_stack_push" to allocate value buffer in the native value stack.
     * These allocated values are on top of this stored top pointer.
     */
    top = rjs_value_stack_save(rt);

    /*Allocate a value in the native stack to store the string value.*/
    strv = rjs_value_stack_push(rt);

    /*Generate the source from the pattern and loop count.*/
    snprintf(source, sizeof(source), source_pattern, loop);

    /*Convert the source to ratjs string.*/
    rjs_string_from_chars(rt, strv, source, -1);

    /*Allocate a value in the native stack to store the script.*/
    scriptv = rjs_value_stack_push(rt);

    /*Parse the string to script.*/
    r = rjs_script_from_string(rt, scriptv, strv, NULL, RJS_FALSE);
    if (r == RJS_ERR) {
        /*Return RJS_ERR, means the source string has syntax error.*/
        fprintf(stderr, "parse the source failed\n");
        goto end; 
    }

    /*Allocate a value in the native stack to store the return value of the script.*/
    retv = rjs_value_stack_push(rt);

    /*Run the script.*/
    r = rjs_script_evaluation(rt, scriptv, retv);
    if (r == RJS_ERR) {
        /*Return RJS_ERR, means some runtime error generated.*/
        fprintf(stderr, "runtime error\n");
        goto end; 
    }

    /*The result must be a number.*/
    assert(rjs_value_is_number(rt, retv));

    /*Output the result value.*/
    printf("result: %f\n", rjs_value_get_number(rt, retv));

    /*
     * Restore the native stack's top pointer.
     * Then the values (allocated by "rjs_value_stack_push") on top of this top pointer are released.
     */
    rjs_value_stack_restore(rt, top);

    rc = 0;
end:
    /*Finally free the runtime.*/
    rjs_runtime_free(rt);
    return rc;
}

