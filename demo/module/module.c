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
 * This program demonstrates how to use module.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>

/*Include "ratjs.h".*/
#include <ratjs.h>

/*
 * Module path check function.
 * When the engine try to load a module, this function will be invoked to check if the module can be loaded.
 * Parameter "base" is the path name of the base module which try to import the new module.
 * If "base" == NULL, it means the new module is loaded by the native code, not another module.
 * Parameter "name" is the new module's name.
 * Parameter "path" is the output buffer to store the new module's pathname.
 * Parameter "size" is the size of the buffer "path".
 * If the module can be loaded, this function return RJS_OK.
 * If the module cannot be loaded, this function return RJS_ERR.
 */
static RJS_Result
module_path_check (RJS_Runtime *rt, const char *base,
        const char *name, char *path, size_t size)
{
    char        base_buf[PATH_MAX];
    char       *dir;
    struct stat sb;
    int         r;

    printf("try to load \"%s\"\n", name);

    if (base) {
        /*Import from another module.*/

        /*Get the base module's directory.*/
        snprintf(base_buf, sizeof(base_buf), "%s", base);

        dir = dirname(base_buf);

        /*Relative pathname from the base directory.*/
        snprintf(path, size, "%s/%s", dir, name);
    } else {
        /*Use name as the pathname.*/
        snprintf(path, size, "%s", name);
    }

    /*Check if the module exist.*/
    r = stat(path, &sb);
    if (r == -1)
        return RJS_ERR;

    return RJS_OK;
}

int
main (int argc, char **argv)
{
    RJS_Runtime *rt;
    size_t       top;
    RJS_Value   *modv;
    RJS_Result   r;
    char        *mod_name;
    int          rc = 1;

    /*First create a ratjs runtime.*/
    rt = rjs_runtime_new();

    /*Load extension functions.*/
    rjs_realm_load_extension(rt, NULL);

    /*
     * Save the native value stack's top pointer.
     * Then you can use "rjs_value_stack_push" to allocate value buffer in the native value stack.
     * These allocated values are on top of this stored top pointer.
     */
    top = rjs_value_stack_save(rt);

    /*Set the module path check function.*/
    rjs_set_module_path_func(rt, module_path_check);

    /*Allocate a value from native value stack to store the module value.*/
    modv = rjs_value_stack_push(rt);

    mod_name = "entry.js";

    /*Load the module*/
    r = rjs_module_from_file(rt, modv, mod_name, NULL);
    if (r == RJS_ERR) {
        fprintf(stderr, "load module failed\n");
        goto end;
    }

    /*Link the module.*/
    r = rjs_module_link(rt, modv);
    if (r == RJS_ERR) {
        fprintf(stderr, "module link failed\n");
        goto end;
    }

    /*Execute the module.*/
    r = rjs_module_evaluate(rt, modv, NULL);
    if (r == RJS_ERR) {
        fprintf(stderr, "module evalute failed\n");
        goto end;
    }

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

