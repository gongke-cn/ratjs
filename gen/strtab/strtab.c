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

/**String table entry.*/
typedef struct {
    char *str;  /**< The string.*/
    char *name; /**< The name of the string.*/
} STR_Entry;

/**String property entry.*/
typedef struct {
    char *str; /**< The string.*/
} STR_PropEntry;

/**Symbol property entry.*/
typedef struct {
    char *name;  /**< The name.*/
    char *descr; /**< The description.*/
} SYM_PropEntry;

#include "internal_string.h"

/*Generate header file.*/
static void
gen_h (void)
{
    const STR_Entry     *se   = strings;
    const STR_PropEntry *strp = str_props;
    const SYM_PropEntry *symp = sym_props;

    printf("enum {\n");
    while (se->str) {
        char *name = se->name;

        if (!se->name)
            name = se->str;

        printf("\tRJS_S_%s,\n", name);
        se ++;
    }
    printf("\tRJS_S_MAX\n");
    printf("};\n\n");

    printf("enum {\n");
    while (strp->str) {
        printf("\tRJS_PN_STR_%s,\n", strp->str);
        strp ++;
    }
    printf("\tRJS_PN_STR_MAX\n");
    printf("};\n\n");

    printf("enum {\n");
    while (symp->name) {
        printf("\tRJS_PN_SYM_%s", symp->name);
        if (symp == sym_props)
            printf(" = RJS_PN_STR_MAX");
        printf(",\n");
        symp ++;
    }
    printf("\tRJS_PN_SYM_MAX\n");
    printf("};\n\n");
    printf("#define RJS_PN_MAX RJS_PN_SYM_MAX\n\n");
}

/*Generate function file.*/
static void
gen_f (void)
{
    const STR_Entry     *se = strings;
    const STR_PropEntry *strp = str_props;
    const SYM_PropEntry *symp = sym_props;

    while (se->str) {
        char *name = se->name;

        if (!se->name)
            name = se->str;

        printf("static inline RJS_Value* rjs_s_%s (RJS_Runtime *rt)\n",
                name);
        printf("{\n");
        printf("\treturn &rt->strings[RJS_S_%s];\n",
                name);
        printf("}\n\n");
        se ++;
    }

    while (strp->str) {
        printf("static inline RJS_PropertyName* rjs_pn_%s (RJS_Runtime *rt)\n",
                strp->str);
        printf("{\n");
        printf("\treturn &rt->prop_names[RJS_PN_STR_%s];\n",
                strp->str);
        printf("}\n\n");
        strp ++;
    }

    while (symp->name) {
        printf("static inline RJS_PropertyName* rjs_pn_s_%s (RJS_Runtime *rt)\n",
                symp->name);
        printf("{\n");
        printf("\treturn &rt->prop_names[RJS_PN_SYM_%s];\n",
                symp->name);
        printf("}\n\n");
        symp ++;
    }
}

/*Generate C file.*/
static void
gen_c (void)
{
    const STR_Entry     *se   = strings;
    const STR_PropEntry *strp = str_props;
    const SYM_PropEntry *symp = sym_props;

    printf("static const char* string_table[] = {\n");
    while (se->str) {
        printf("\t\"%s\",\n", se->str);
        se ++;
    }
    printf("\tNULL\n");
    printf("};\n\n");

    printf("static const char* str_prop_table[] = {\n");
    while (strp->str) {
        printf("\t\"%s\",\n", strp->str);
        strp ++;
    }
    printf("\tNULL\n");
    printf("};\n\n");

    printf("static const char* sym_prop_table[] = {\n");
    while (symp->name) {
        printf("\t\"%s\",\n", symp->descr);
        symp ++;
    }
    printf("\tNULL\n");
    printf("};\n\n");

    printf("static const char* sym_name_table[] = {\n");
    symp = sym_props;
    while (symp->name) {
        printf("\t\"%s\",\n", symp->name);
        symp ++;
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
    } else {
        gen_c();
    }

    return 0;
}
