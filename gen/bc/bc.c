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
#include <unistd.h>
#include <assert.h>
#include <json-c/json.h>
#include <ratjs/rjs_config.h>

static json_object *json = NULL;

/*Check if the byte code is valid.*/
static int
is_valid_bc (const char *name)
{
#if !ENABLE_MODULE
    if (!strcmp(name, "module_init"))
        return 0;

    if (!strcmp(name, "load_import_meta"))
        return 0;

    if (!strcmp(name, "import"))
        return 0;
#endif

#if !ENABLE_SCRIPT
    if (!strcmp(name, "script_init"))
        return 0;
#endif

#if !ENABLE_EVAL
    if (!strcmp(name, "eval_init")
            || !strcmp(name, "eval"))
        return 0;
#endif

#if !ENABLE_GENERATOR
    if (!strcmp(name, "yield"))
        return 0;

    if (!strcmp(name, "yield_resume"))
        return 0;

    if (!strcmp(name, "yield_iter_start"))
        return 0;

    if (!strcmp(name, "yield_iter_next"))
        return 0;

    if (!strcmp(name, "generator_start"))
        return 0;
#endif

#if !ENABLE_ASYNC
    if (!strcmp(name, "await"))
        return 0;

    if (!strcmp(name, "await_resume"))
        return 0;

    if (!strcmp(name, "async_for_step"))
        return 0;

    if (!strcmp(name, "async_for_step_resume"))
        return 0;
#endif

#if !ENABLE_PRIV_NAME

    if (!strcmp(name, "priv_method_add")
            || !strcmp(name, "priv_getter_add")
            || !strcmp(name, "priv_setter_add")
            || !strcmp(name, "static_priv_method_add")
            || !strcmp(name, "static_priv_getter_add")
            || !strcmp(name, "static_priv_setter_add")
            || !strcmp(name, "priv_field_add")
            || !strcmp(name, "priv_inst_field_add")
            || !strcmp(name, "set_priv_env")
            || !strcmp(name, "has_priv")
            || !strcmp(name, "priv_get")
            || !strcmp(name, "priv_set"))
        return 0;
#endif

    return 1;
}

/*Convert the byte code parameter to C type.*/
static const char*
param_to_ctype (const char *p)
{
    if (!strcmp(p, "label") || !strcmp(p, "index"))
        return "int";
    if (!strcmp(p, "decl"))
        return "RJS_AstDecl*";
    if (!strcmp(p, "binding_table"))
        return "RJS_AstBindingTable*";
    if (!strcmp(p, "func_table"))
        return "RJS_AstFuncTable*";
    if (!strcmp(p, "rr") || !strcmp(p, "wr"))
        return "int";
    if (!strcmp(p, "binding"))
        return "RJS_AstBindingRef*";
    if (!strcmp(p, "value"))
        return "RJS_AstValueEntry*";
    if (!strcmp(p, "prop"))
        return "RJS_AstPropRef*";
    if (!strcmp(p, "func"))
        return "RJS_AstFunc*";
    if (!strcmp(p, "priv_env"))
        return "RJS_AstPrivEnv*";

    assert(0);
    return NULL;
}

/*Get the parameter's size.*/
static int
param_get_size (const char *p)
{
    if (!strcmp(p, "label")
            || !strcmp(p, "decl")
            || !strcmp(p, "binding_table")
            || !strcmp(p, "func_table")
            || !strcmp(p, "binding")
            || !strcmp(p, "value")
            || !strcmp(p, "prop")
            || !strcmp(p, "func")
            || !strcmp(p, "priv_env"))
        return 2;
    if (!strcmp(p, "index")
            || !strcmp(p, "rr")
            || !strcmp(p, "wr"))
        return 1;

    assert(0);
    return -1;
}

/*Generate byte code store code.*/
static int
bc_store (const char *cmd, const char *type, const char *name)
{
    if (!strcmp(type, "label")) {
        printf("        if ((r = bc_label_store(rt, bg, bc, cmd->%s.%s, off)) == RJS_ERR) return r;\n",
                cmd, name);
    } else if (!strcmp(type, "rr") || !strcmp(type, "wr")) {
        printf("        if ((r = bc_reg_store(rt, bg, bc, cmd->%s.%s)) == RJS_ERR) return r;\n",
                cmd, name);
    } else if (!strcmp(type, "index")) {
        printf("        if ((r = bc_arg_index_store(rt, bg, bc, cmd->%s.%s)) == RJS_ERR) return r;\n",
                cmd, name);
    } else if (!strcmp(type, "decl")) {
        printf("        id = rjs_code_gen_decl_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    } else if (!strcmp(type, "binding_table")) {
        printf("        id = rjs_code_gen_binding_table_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    } else if (!strcmp(type, "func_table")) {
        printf("        id = rjs_code_gen_func_table_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    } else if (!strcmp(type, "binding")) {
        printf("        id = rjs_code_gen_binding_ref_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    } else if (!strcmp(type, "value")) {
        printf("        id = rjs_code_gen_value_entry_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    } else if (!strcmp(type, "prop")) {
        printf("        id = rjs_code_gen_prop_ref_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    } else if (!strcmp(type, "func")) {
        printf("        id = rjs_code_gen_func_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    } else if (!strcmp(type, "priv_env")) {
        printf("        id = rjs_code_gen_priv_env_idx(rt, cmd->%s.%s);\n",
                cmd, name);
        printf("        if ((r = bc_index_store(rt, bg, bc, id, \"%s\")) == RJS_ERR) return r;\n",
                type);
    }
    printf("        bc += r;\n");

    return 0;
}

/*Generate byte code disassemble code.*/
static int
bc_disassemble (const char *cmd, const char *type, const char *name)
{
    if (!strcmp(type, "label")) {
        printf("        v = (int16_t)((bc[0] << 8) | bc[1]);\n");
        printf("        fprintf(fp, \"%%d \", v);\n");
        printf("        bc += 2;\n");
    } else if (!strcmp(type, "rr") || !strcmp(type, "wr")) {
        printf("        v = bc[0];\n");
        printf("        fprintf(fp, \"%%d \", v);\n");
        printf("        bc ++;\n");
    } else if (!strcmp(type, "index")) {
        printf("        v = bc[0];\n");
        printf("        fprintf(fp, \"%%d \", v);\n");
        printf("        bc ++;\n");
    } else {
        printf("        v = (bc[0] << 8) | bc[1];\n");
        printf("        fprintf(fp, \"%%d \", v);\n");
        printf("        bc += 2;\n");
    }

    return 0;
}

/*Generate the header file.*/
static void
gen_h (void)
{
    json_bool b;
    int       id;

    printf("typedef enum {\n");

    id = 0;
    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type;

            val = val;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    RJS_BC_%s, /*%d*/\n", tstr, id ++);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    RJS_BC_%s, /*%d*/\n", key, id ++);
            }
        }
    }

    printf("    RJS_BC_MAX\n");
    printf("} RJS_BcType;\n\n");

    printf("typedef struct {\n");
    printf("    RJS_BcType type;\n");
    printf("    int line;\n");
    printf("} RJS_BcCmd_gen;\n\n");

    {
        json_object_object_foreach(json, key, val) {
            json_object *params;

            if (!is_valid_bc(key))
                continue;

            printf("typedef struct {\n");
            printf("    RJS_BcType type;\n");
            printf("    int line;\n");

            b = json_object_object_get_ex(val, "params", &params);
            if (b) {
                json_object_object_foreach(params, pk, pv) {
                    const char *tstr = json_object_get_string(pv);

                    printf("    %s %s;\n", param_to_ctype(tstr), pk);
                }
            }

            printf("} RJS_BcCmd_%s;\n\n", key);
        }
    }

    printf("typedef union {\n");
    printf("    RJS_BcType type;\n");
    printf("    RJS_BcCmd_gen gen;\n");

    {
        json_object_object_foreach(json, key, val) {
            val = val;

            if (!is_valid_bc(key))
                continue;

            printf("    RJS_BcCmd_%s %s;\n", key, key);
        }
    }

    printf("} RJS_BcCommand;");
}

/*Generate the C file.*/
static void
gen_c (void)
{
    int model = 0;

    /*Generate byte code model table.*/
    printf("static uint8_t bc_model_table[] = {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type;
            json_bool    b;

            key = key;
            val = val;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    %d,\n", model);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    %d,\n", model);
            }

            model ++;
        }
    }
    printf("    -1\n");
    printf("};\n\n");

    /*Generate byte code size table.*/
    printf("static uint8_t bc_size_table[] = {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_object *params, *pseudo;
            json_bool    b;
            int          size = 0;

            key = key;

            if (!is_valid_bc(key))
                continue;

            b = json_object_object_get_ex(val, "pseudo", &pseudo);
            if (!b) {
                size = 1;
                b = json_object_object_get_ex(val, "params", &params);
                if (b) {
                    json_object_object_foreach(params, pk, pv) {
                        const char *tstr = json_object_get_string(pv);

                        pk = pk;

                        size += param_get_size(tstr);
                    }
                }
            }

            printf("    %d,\n", size);
        }
    }
    printf("    -1\n");
    printf("};\n\n");

    /*Generate byte code name table.*/
    printf("static const char* bc_name_table[] = {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_object *types;
            json_bool    b;

            key = key;
            val = val;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    json_object *type;
                    const char  *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    \"%s\",\n", tstr);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    \"%s\",\n", key);
            }

            model ++;
        }
    }
    printf("    NULL\n");
    printf("};\n\n");

    /*Generate set register's last access offset function.*/
    printf("static RJS_Result\n");
    printf("bc_cmd_set_regs_last_acc_off (RJS_Runtime *rt, RJS_BcGen *bg, RJS_BcCommand *cmd, int off)\n");
    printf("{\n");
    printf("    RJS_BcRegister *reg;\n");
    printf("    switch (cmd->type) {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type, *params;
            json_bool    b;

            val = val;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    case RJS_BC_%s:\n", tstr);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    case RJS_BC_%s:\n", key);
            }

            b = json_object_object_get_ex(val, "params", &params);
            if (b) {
                json_object_object_foreach(params, pk, pv) {
                    const char *tstr = json_object_get_string(pv);

                    if (!strcmp(tstr, "rr") || !strcmp(tstr, "wr")) {
                        printf("        reg = &bg->reg.items[cmd->%s.%s];\n",
                                key, pk);
                        printf("        reg->last_acc_off = off;\n");
                    }
                }
            }

            printf("        break;\n");
        }
    }
    printf("    default: assert(0);\n");
    printf("    }\n");
    printf("    return RJS_OK;");
    printf("}\n\n");

    /*Generate allocate register function.*/
    printf("static RJS_Result\n");
    printf("bc_cmd_alloc_regs (RJS_Runtime *rt, RJS_BcGen *bg, RJS_BcCommand *cmd, RJS_BcRegMap *rmap, int off)\n");
    printf("{\n");
    printf("    RJS_Result         r;\n");
    printf("    RJS_BcRegister    *reg;\n");
    printf("    switch (cmd->type) {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type, *params;
            json_bool    b;

            val = val;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    case RJS_BC_%s:\n", tstr);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    case RJS_BC_%s:\n", key);
            }

            b = json_object_object_get_ex(val, "params", &params);
            if (b) {
                json_object_object_foreach(params, pk, pv) {
                    const char *tstr = json_object_get_string(pv);

                    if (!strcmp(tstr, "rr") || !strcmp(tstr, "wr")) {
                        printf("        reg = &bg->reg.items[cmd->%s.%s];\n",
                                key, pk);
                        printf("        if ((r = bc_gen_alloc_reg(rt, rmap, reg, off)) == RJS_ERR)\n");
                        printf("            return r;\n");
                    }
                }
            }

            printf("        break;\n");
        }
    }
    printf("    default: assert(0);\n");
    printf("    }\n");
    printf("    return RJS_OK;\n");
    printf("}\n\n");

    /*Generate binding reference.*/
    printf("static RJS_Result\n");
    printf("bc_cmd_binding_ref (RJS_Runtime *rt, RJS_BcGen *bg, RJS_BcCommand *cmd)\n");
    printf("{\n");
    printf("    switch (cmd->type) {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type, *params;
            json_bool    b;

            val = val;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    case RJS_BC_%s:\n", tstr);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    case RJS_BC_%s:\n", key);
            }

            b = json_object_object_get_ex(val, "params", &params);
            if (b) {
                json_object_object_foreach(params, pk, pv) {
                    const char *tstr = json_object_get_string(pv);

                    if (!strcmp(tstr, "binding")) {
                        printf("        rjs_code_gen_binding_ref_idx(rt, cmd->%s.%s);\n", key, pk);
                    } else if (!strcmp(tstr, "binding_table")) {
                        printf("        rjs_code_gen_binding_table_idx(rt, cmd->%s.%s);\n", key, pk);
                    } else if (!strcmp(tstr, "func_table")) {
                        printf("        rjs_code_gen_func_table_idx(rt, cmd->%s.%s);\n", key, pk);
                    }
                }
            }

            printf("        break;\n");
        }
    }
    printf("    default: assert(0);\n");
    printf("    }\n");
    printf("    return RJS_OK;\n");
    printf("}\n\n");

    /*Generate byte code store function.*/
    printf("static RJS_Result\n");
    printf("bc_cmd_store_bc (RJS_Runtime *rt, RJS_BcGen *bg, RJS_BcCommand *cmd, uint8_t *bc, int off)\n");
    printf("{\n");
    printf("    RJS_Result r;\n");
    printf("    int        id;\n");
    printf("    switch (cmd->type) {\n");
    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type, *params, *pseudo;
            json_bool    b;

            val = val;

            b = json_object_object_get_ex(val, "pseudo", &pseudo);
            if (b)
                continue;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    case RJS_BC_%s:\n", tstr);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    case RJS_BC_%s:\n", key);
            }

            b = json_object_object_get_ex(val, "params", &params);
            if (b) {
                json_object_object_foreach(params, pk, pv) {
                    const char *tstr = json_object_get_string(pv);

                    bc_store(key, tstr, pk);
                }
            }

            printf("        break;\n");
        }
    }
    printf("    default: assert(0);\n");
    printf("    }\n");
    printf("    return RJS_OK;\n");
    printf("}\n\n");

    printf("static int\n");
    printf("bc_disassemble (RJS_Runtime *rt, FILE *fp, uint8_t *bc_start)\n");
    printf("{\n");
    printf("    uint8_t *bc = bc_start;\n");
    printf("    int      v;\n");
    printf("    fprintf(fp, \"%%-20s \", bc_name_table[*bc]);\n");
    printf("    switch (*bc ++) {\n");

    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type, *params, *pseudo;
            json_bool    b;

            val = val;

            b = json_object_object_get_ex(val, "pseudo", &pseudo);
            if (b)
                continue;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    printf("    case RJS_BC_%s:\n", tstr);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                printf("    case RJS_BC_%s:\n", key);
            }

            b = json_object_object_get_ex(val, "params", &params);
            if (b) {
                json_object_object_foreach(params, pk, pv) {
                    const char *tstr = json_object_get_string(pv);

                    bc_disassemble(key, tstr, pk);
                }
            }

            printf("        break;\n");
        }
    }

    printf("    }\n");
    printf("    return bc - bc_start;\n");
    printf("}\n\n");
}

/*Generate byte code running code.*/
static void
gen_bc_run (const char *name, json_object *o)
{
    json_object *params;
    json_bool    b;
    int          off;

    b = json_object_object_get_ex(o, "params", &params);

    printf("case RJS_BC_%s: {\n", name);
    
    off = 1;
    if (b) {
        json_object_object_foreach(params, key, val) {
            const char *type = json_object_get_string(val);
            int         size = param_get_size(type);

            if (!strcmp(type, "label")) {
                printf("    int16_t %s = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
            } else if (!strcmp(type, "decl")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_ScriptDecl *%s = (%s_id != RJS_INVALID_DECL_INDEX) ? &script->decl_table[%s_id] : NULL;\n",
                        key, key, key);
            } else if (!strcmp(type, "binding_table")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_ScriptBindingGroup *%s = (%s_id != RJS_INVALID_BINDING_GROUP_INDEX) ? &script->binding_group_table[%s_id] : NULL;\n",
                        key, key, key);
            } else if (!strcmp(type, "func_table")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_ScriptFuncDeclGroup *%s = (%s_id != RJS_INVALID_FUNC_GROUP_INDEX) ? &script->func_decl_group_table[%s_id] : NULL;\n",
                        key, key, key);
            } else if (!strcmp(type, "binding")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_ScriptBindingRef *%s = &script->binding_ref_table[sc->scb.lex_env->script_decl->binding_ref_start + %s_id];\n",
                        key, key);
            } else if (!strcmp(type, "value")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_Value *%s = (%s_id != RJS_INVALID_VALUE_INDEX) ? &script->value_table[%s_id] : NULL;\n", key, key, key);
            } else if (!strcmp(type, "prop")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_ScriptPropRef *%s = &script->prop_ref_table[sf->prop_ref_start + %s_id];\n", key, key);
            } else if (!strcmp(type, "func")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_ScriptFunc *%s = (%s_id != RJS_INVALID_FUNC_INDEX) ? &script->func_table[%s_id] : NULL;\n", key, key, key);
            } else if (!strcmp(type, "index")) {
                printf("    uint8_t %s = bc[%d];\n", key, off);
            } else if (!strcmp(type, "rr") || !strcmp(type, "wr")) {
                printf("    RJS_Value *%s = rjs_value_buffer_item(rt, sc->regs, bc[%d]);\n", key, off);
            } else if (!strcmp(type, "priv_env")) {
                printf("    uint16_t %s_id = (bc[%d] << 8) | bc[%d];\n", key, off, off + 1);
                printf("    RJS_ScriptPrivEnv *%s = &script->priv_env_table[%s_id];\n", key, key);
            }

            off += size;
        }
    }
    printf("    ip_size = %d;\n", off);
    printf("\n    bc_%s(", name);
    if (b) {
        int first = 1;

        json_object_object_foreach(params, key, val) {
            val = val;

            if (first)
                first = 0;
            else
                printf(", ");
            printf("%s", key);
        }
    }
    printf(");\n\n");
    printf("    sc->ip += ip_size;\n");
    printf("    break;\n");
    printf("}\n");
}

/*Generate byte code template code.*/
static void
gen_bc_t (const char *name, json_object *o)
{
    json_object *params;
    json_bool    b;

    b = json_object_object_get_ex(o, "params", &params);

    printf("#define bc_%s(", name);
    
    if (b) {
        int first = 1;

        json_object_object_foreach(params, key, val) {
            val = val;
            
            if (first)
                first = 0;
            else
                printf(", ");
                
            printf("%s", key);
        }
    }

    printf(")\n\n");
}

/*Generate byte code running source file.*/
static void
gen_r (void)
{
    json_bool b;

    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type, *pseudo;

            val = val;

            b = json_object_object_get_ex(val, "pseudo", &pseudo);
            if (b && json_object_get_boolean(pseudo))
                continue;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    gen_bc_run(tstr, val);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;

                gen_bc_run(key, val);
            }
        }
    }
}

/*Generate byte code template file.*/
static void
gen_t (void)
{
    json_bool b;

    {
        json_object_object_foreach(json, key, val) {
            json_object *types, *type, *pseudo;

            val = val;

            b = json_object_object_get_ex(val, "pseudo", &pseudo);
            if (b && json_object_get_boolean(pseudo))
                continue;

            b = json_object_object_get_ex(val, "types", &types);

            if (b) {
                int i, len;

                len = json_object_array_length(types);

                for (i = 0; i < len; i ++) {
                    const char *tstr;

                    type = json_object_array_get_idx(types, i);
                    tstr = json_object_get_string(type);

                    if (!is_valid_bc(tstr))
                        continue;

                    gen_bc_t(tstr, val);
                }
            } else {
                if (!is_valid_bc(key))
                    continue;
                    
                gen_bc_t(key, val);
            }
        }
    }
}

int
main (int argc, char **argv)
{
    json = json_object_from_file("gen/bc/bc.json");
    if (!json) {
        fprintf(stderr, "cannot open byte code definition json\n");
        return 1;
    }

    if ((argc > 1) && !strcmp(argv[1], "-h")) {
        gen_h();
    } else if ((argc > 1) && !strcmp(argv[1], "-r")) {
        gen_r();
    } else if ((argc > 1) && !strcmp(argv[1], "-t")) {
        gen_t();
    } else {
        gen_c();
    }

    json_object_put(json);
    return 0;
}
