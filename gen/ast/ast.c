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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <json-c/json.h>

static json_object *json = NULL;

static void
gen_h (void)
{
    int id;

    printf("typedef enum {\n");
    id = 0;
    {
        json_object_object_foreach(json, key, val) {
            val = val;
            printf("    RJS_AST_MODEL_%s, /*%d*/\n", key, id ++);
        }
    }
    printf("    RJS_AST_MODEL_MAX\n");
    printf("} RJS_AstModelType;\n\n");

    printf("typedef enum {\n");
    id = 0;
    {
        json_object_object_foreach(json, key, val) {
            json_bool    b;
            json_object *types;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int len = json_object_array_length(types);
                int i;

                for (i = 0; i < len; i ++) {
                    json_object *type = json_object_array_get_idx(types, i);
                    const char  *tstr = json_object_get_string(type);

                    printf("    RJS_AST_%s, /*%d*/\n", tstr, id ++);
                }
            } else {
                printf("    RJS_AST_%s, /*%d*/\n", key, id ++);
            }
        }
    }
    printf("    RJS_AST_MAX\n");
    printf("} RJS_AstType;\n\n");

    {
        json_object_object_foreach(json, key, val) {
            val = val;

            printf("typedef struct RJS_Ast%s_s RJS_Ast%s;\n\n", key, key);
        }
    }

    {
        json_object_object_foreach(json, key, val) {
            json_bool    b;
            json_object *members;

            b = json_object_object_get_ex(val, "members", &members);
            if (!b)
                continue;

            printf("struct RJS_Ast%s_s {\n", key);
            printf("    RJS_Ast ast;\n");
            {
                json_object_object_foreach(members, mkey, mval) {
                    const char *m = json_object_get_string(mval);
                    if (!strcmp(m, "RJS_Value")) {
                        printf("    RJS_Value %s;\n", mkey);
                    }
                }
            }
            {
                json_object_object_foreach(members, mkey, mval) {
                    const char *m = json_object_get_string(mval);
                    if (!strcmp(m, "RJS_List")) {
                        printf("    RJS_List %s;\n", mkey);
                    }
                }
            }
            {
                json_object_object_foreach(members, mkey, mval) {
                    const char *m = json_object_get_string(mval);
                    if (!strcmp(m, "RJS_Hash")) {
                        printf("    RJS_Hash %s;\n", mkey);
                    }
                }
            }
            {
                json_object_object_foreach(members, mkey, mval) {
                    const char *m = json_object_get_string(mval);
                    if (strcmp(m, "RJS_Value")
                            && strcmp(m, "RJS_List")
                            && strcmp(m, "RJS_Hash")) {
                        if (!strcmp(m, "RJS_PrivateEnv*")) {
                            printf("#if ENABLE_PRIV_NAME\n");
                            printf("    %s %s;\n", m, mkey);
                            printf("#endif /*ENABLE_PRIV_NAME*/\n");
                        } else {
                            printf("    %s %s;\n", m, mkey);
                        }
                    }
                }
            }
            printf("};\n\n");
        }
    }
}

static void
gen_c (void)
{
    printf("static const RJS_AstModelType\n");
    printf("ast_type_model_tab[] = {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_bool    b;
            json_object *types;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int len = json_object_array_length(types);
                int i;

                for (i = 0; i < len; i ++) {
                    json_object *type;
                    const char  *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    printf("    RJS_AST_MODEL_%s, /*%s*/\n", key, tstr);
                }
            } else {
                printf("    RJS_AST_MODEL_%s, /*%s*/\n", key, key);
            }
        }
    }
    printf("    RJS_AST_MODEL_MAX\n");
    printf("};\n\n");

    printf("static const RJS_AstOps\n");
    printf("ast_ops_tab[] = {\n");
    {
        json_object_object_foreach(json, key, val) {
            int          vn = 0, ln = 0, hn = 0;
            char        *vf = NULL, *lf = NULL, *hf = NULL;
            json_bool    b;
            json_object *members;

            b = json_object_object_get_ex(val, "members", &members);
            if (b) {
                json_object_object_foreach(members, mkey, mval) {
                    mkey = mkey;
                    const char *m = json_object_get_string(mval);
                    if (!strcmp(m, "RJS_Value")) {
                        if (!vf)
                            vf = mkey;
                        vn ++;
                    } else if (!strcmp(m, "RJS_List")) {
                        if (!lf)
                            lf = mkey;
                        ln ++;
                    } else if (!strcmp(m, "RJS_Hash")) {
                        if (!hf)
                            hf = mkey;
                        hn ++;
                    }
                }
            }
            printf("    /*%s*/\n", key);
            printf("    {\n");
            printf("        {\n");
            printf("            RJS_GC_THING_AST,\n");
            printf("            ast_op_gc_scan,\n");
            printf("            ast_op_gc_free\n");
            printf("        },\n");
            printf("        sizeof(RJS_Ast%s),\n", key);
            if (vf)
                printf("        RJS_OFFSET_OF(RJS_Ast%s, %s),\n", key, vf);
            else
                printf("        0,\n");
            printf("        %d,\n", vn);
            if (lf)
                printf("        RJS_OFFSET_OF(RJS_Ast%s, %s),\n", key, lf);
            else
                printf("        0,\n");
            printf("        %d,\n", ln);
            if (hf)
                printf("        RJS_OFFSET_OF(RJS_Ast%s, %s),\n", key, hf);
            else
                printf("        0,\n");
            printf("        %d\n", hn);
            printf("    },\n");
        }
    }
    printf("    {{RJS_GC_THING_AST, NULL, NULL}, -1, 0, 0}\n");
    printf("};\n\n");
}

int
main (int argc, char **argv)
{
    json = json_object_from_file("gen/ast/ast.json");
    if (!json) {
        fprintf(stderr, "load AST json failed\n");
        return 1;
    }

    if ((argc > 1) && !strcmp(argv[1], "-h")) {
        gen_h();
    } else {
        gen_c();
    }

    return 0;
}
