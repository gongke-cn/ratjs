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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ratjs/rjs_config.h>

/**Object table entry.*/
typedef struct {
    char *name;  /**< The name of the symbol.*/
} OBJ_Entry;

#include "internal_object.h"

/*Generate header file.*/
static void
gen_h (void)
{
    const OBJ_Entry *oe = objects;

    printf("enum {\n");
    while (oe->name) {
        printf("\tRJS_O_%s,\n", oe->name);
        oe ++;
    }
    printf("\tRJS_O_MAX\n");
    printf("};\n\n");
}

/*Generate function file.*/
static void
gen_f (void)
{
    const OBJ_Entry *oe = objects;

    while (oe->name) {
        printf("static inline RJS_Value* rjs_o_%s (RJS_Realm *realm)\n",
                oe->name);
        printf("{\n");
        printf("\treturn &realm->objects[RJS_O_%s];\n",
                oe->name);
        printf("}\n\n");
        oe ++;
    }
}

/*Generate the name table file.*/
static void
gen_c (void)
{
    const OBJ_Entry *oe = objects;

    printf("static const char* internal_object_name_table[] = {\n");
    while (oe->name) {
        printf("\t\"%s\",\n", oe->name);
        oe ++;
    }
    printf("\tNULL\n");
    printf("};\n\n");
}

int
main (int argc, char **argv)
{
    if ((argc > 1) && !strcmp(argv[1], "-h")) {
        gen_h();
    } else if ((argc > 1) && !strcmp(argv[1], "-f")) {
        gen_f();
    } else if ((argc > 1) && !strcmp(argv[1], "-c")) {
        gen_c();
    }

    return 0;
}
