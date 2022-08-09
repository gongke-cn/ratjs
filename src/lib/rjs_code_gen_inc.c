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

#define BINDING_INIT_FL_VAR   1
#define BINDING_INIT_FL_LET   2
#define BINDING_INIT_FL_CONST 4
#define BINDING_INIT_FL_CLASS 8
#define BINDING_INIT_FL_FUNC  16
#define BINDING_INIT_FL_PARAM 32
#define BINDING_INIT_FL_UNDEF 64
#define BINDING_INIT_FL_LEX \
    (BINDING_INIT_FL_LET|BINDING_INIT_FL_CONST|BINDING_INIT_FL_CLASS)

/*Create the binding initialize table with parameter check.*/
static void
binding_init_table_param_check (RJS_Runtime *rt, RJS_Value *tab,
        RJS_AstDecl *decl, RJS_AstDecl *param_decl, int flags)
{
    RJS_AstDeclItem     *di;
    RJS_AstBindingInit  *bi;
    RJS_AstBindingTable *bt  = NULL;
    size_t               top = rjs_value_stack_save(rt);
    RJS_Value           *tmp = rjs_value_stack_push(rt);

    if (rjs_list_is_empty(&decl->item_list))
        return;

    rjs_list_foreach_c(&decl->item_list, di, RJS_AstDeclItem, ast.ln) {
        switch (di->type) {
        case RJS_AST_DECL_PARAMETER:
            if (!(flags & BINDING_INIT_FL_PARAM))
                continue;
            break;
        case RJS_AST_DECL_VAR:
            if (!(flags & BINDING_INIT_FL_VAR))
                continue;
            break;
        case RJS_AST_DECL_LET:
            if (!(flags & BINDING_INIT_FL_LET))
                continue;
            break;
        case RJS_AST_DECL_CONST:
        case RJS_AST_DECL_STRICT:
            if (!(flags & BINDING_INIT_FL_CONST))
                continue;
            break;
        case RJS_AST_DECL_CLASS:
            if (!(flags & BINDING_INIT_FL_CLASS))
                continue;
            break;
        case RJS_AST_DECL_FUNCTION:
            if (!(flags & BINDING_INIT_FL_FUNC))
                continue;
            break;
        }

        bi = ast_new(rt, tmp, RJS_AST_BindingInit, &di->ast.location);

        bi->binding_ref = di->binding_ref;
        bi->flags       = 0;
        bi->param_index = -1;

        if (di->type == RJS_AST_DECL_CONST)
            bi->flags |= RJS_AST_BINDING_INIT_IMMUT;
        else if (di->type == RJS_AST_DECL_STRICT)
            bi->flags |= RJS_AST_BINDING_INIT_IMMUT|RJS_AST_BINDING_INIT_STRICT;

        if (param_decl) {
            RJS_HashEntry *he;

            he = hash_lookup(rt, &param_decl->item_hash, &di->binding_ref->name->value, NULL);
            if (he) {
                RJS_AstDeclItem *bdi;

                bdi = RJS_CONTAINER_OF(he, RJS_AstDeclItem, he);

                if (bdi->type == RJS_AST_DECL_PARAMETER) {
                    bi->flags |= RJS_AST_BINDING_INIT_BOT;
                    bi->bot_binding_ref = bdi->binding_ref;
                }
            }
        }

        if (!(bi->flags & RJS_AST_BINDING_INIT_BOT)) {
            if (flags & BINDING_INIT_FL_UNDEF)
                bi->flags |= RJS_AST_BINDING_INIT_UNDEF;
        }

        if (!bt)
            bt = binding_table_new(rt, tab, decl);

        ast_list_append(rt, &bt->binding_init_list, tmp);
        bt->num ++;
    }

    rjs_value_stack_restore(rt, top);
}

/*Create the binding initialize table.*/
static void
binding_init_table (RJS_Runtime *rt, RJS_Value *tab, RJS_AstDecl *decl, int flags)
{
    binding_init_table_param_check(rt, tab, decl, NULL, flags);
}

/*Generate the function.*/
static RJS_Result
gen_func (RJS_Runtime *rt, RJS_AstFunc *func)
{
    RJS_Parser *parser = rt->parser;
    RJS_BcGen  *bg     = parser->code_gen;
    RJS_Result  r;

    if ((func->flags & RJS_AST_FUNC_FL_EXPR) && !func->name)
        func->name = value_entry_add(rt, &func->ast.location, rjs_s_empty(rt));

#if ENABLE_SCRIPT || ENABLE_EVAL
    if (func->flags & (RJS_AST_FUNC_FL_SCRIPT|RJS_AST_FUNC_FL_EVAL)) {
        binding_init_table(rt, &func->var_table, func->var_decl,
            BINDING_INIT_FL_VAR|BINDING_INIT_FL_FUNC|BINDING_INIT_FL_UNDEF);
        binding_init_table(rt, &func->lex_table, func->lex_decl,
            BINDING_INIT_FL_LEX);
    } else
#endif /*ENABLE_SCRIPT*/
#if ENABLE_MODULE
    if (func->flags & RJS_AST_FUNC_FL_MODULE) {
        binding_init_table(rt, &func->var_table, func->var_decl,
            BINDING_INIT_FL_VAR|BINDING_INIT_FL_UNDEF);
        binding_init_table(rt, &func->lex_table, func->lex_decl,
            BINDING_INIT_FL_LEX|BINDING_INIT_FL_FUNC);
    } else
#endif /*ENABLE_MODULE*/
    {
        int flags = BINDING_INIT_FL_PARAM;

        if (func->flags & RJS_AST_FUNC_FL_DUP_PARAM)
            flags |= BINDING_INIT_FL_UNDEF;

        /*Create parameters table.*/
        binding_init_table(rt, &func->param_table, func->param_decl, flags);

        if ((func->flags & RJS_AST_FUNC_FL_NEED_ARGS) && !(func->flags & RJS_AST_FUNC_FL_UNMAP_ARGS)) {
            /*For mapped arguments, we need get the parameters index.*/
            RJS_AstBindingTable *bt;
            RJS_AstBindingInit  *bi;
            RJS_AstBindingElem  *be;

            bt = rjs_value_get_gc_thing(rt, &func->param_table);
            if (bt) {
                rjs_list_foreach_c(&bt->binding_init_list, bi, RJS_AstBindingInit, ast.ln) {
                    int idx = -1, curr = 0;

                    rjs_list_foreach_c(&func->param_list, be, RJS_AstBindingElem, ast.ln) {
                        RJS_AstId *id = rjs_value_get_gc_thing(rt, &be->binding);

                        if (rjs_string_equal(rt, &id->identifier->value, &bi->binding_ref->name->value))
                            idx = curr;

                        curr ++;
                    }

                    bi->param_index = idx;
                }
            }
        }

        /*Create variables table.*/
        binding_init_table_param_check(rt, &func->var_table,
            func->var_decl, func->param_decl,
            BINDING_INIT_FL_VAR|BINDING_INIT_FL_FUNC|BINDING_INIT_FL_UNDEF);

        /*Create lexical declarations table.*/
        binding_init_table(rt, &func->lex_table, func->lex_decl,
            BINDING_INIT_FL_LEX);
    }

    parser->decl_stack   = NULL;
    func->prop_ref_start = parser->prop_ref_num;

    r = rjs_bc_gen_func(rt, bg, func);

    return r;
}

/*Generate script.*/
static RJS_Result
gen_script_internal (RJS_Runtime *rt, RJS_Script *script)
{
    RJS_Parser           *parser      = rt->parser;
    RJS_BcGen            *bg          = parser->code_gen;
    size_t                src_val_cnt = 0;
    RJS_AstFunc          *func;
    RJS_AstValueEntry    *ve;
    RJS_AstDecl          *decl;
    RJS_AstFuncTable     *ft;
    RJS_AstBindingTable  *bt;
    RJS_AstPropRef       *pr;
    size_t                off;
    RJS_Result            r;

    func = RJS_CONTAINER_OF(parser->func_list.next, RJS_AstFunc, ast.ln);
    rjs_code_gen_func_idx(rt, func);

    /*Generate byte code of every functions.*/
    rjs_list_foreach_c(&parser->func_list, func, RJS_AstFunc, ast.ln) {
        if ((r = gen_func(rt, func)) == RJS_ERR)
            return r;
    }

    /*Store the functions.*/
    script->func_num      = parser->func_num;
    script->byte_code_len = bg->bc.item_num;
    script->line_info_num = bg->li.item_num;

    RJS_NEW_N(rt, script->func_table, script->func_num);
    RJS_NEW_N(rt, script->byte_code, script->byte_code_len);
    RJS_NEW_N(rt, script->line_info, script->line_info_num);
    RJS_ELEM_CPY(script->byte_code, bg->bc.items, bg->bc.item_num);
    RJS_ELEM_CPY(script->line_info, bg->li.items, bg->li.item_num);

    rjs_list_foreach_c(&parser->func_list, func, RJS_AstFunc, ast.ln) {
        RJS_ScriptFunc *s_func;
        RJS_BcFunc     *bc_func;

        if (func->id == -1)
            continue;

        s_func  = &script->func_table[func->id];
        bc_func = func->data;

        s_func->flags = 0;

        if (func->flags & RJS_AST_FUNC_FL_STRICT)
            s_func->flags |= RJS_FUNC_FL_STRICT;
        if (func->flags & RJS_AST_FUNC_FL_CLASS_CONSTR)
            s_func->flags |= RJS_FUNC_FL_CLASS_CONSTR;
        if (func->flags & RJS_AST_FUNC_FL_DERIVED)
            s_func->flags |= RJS_FUNC_FL_DERIVED;
        if (func->flags & RJS_AST_FUNC_FL_CLASS_FIELD_INIT)
            s_func->flags |= RJS_FUNC_FL_CLASS_FIELD_INIT;
        if (func->flags & RJS_AST_FUNC_FL_GET)
            s_func->flags |= RJS_FUNC_FL_GET;
        if (func->flags & RJS_AST_FUNC_FL_SET)
            s_func->flags |= RJS_FUNC_FL_SET;

#if ENABLE_ARROW_FUNC
        if (func->flags & RJS_AST_FUNC_FL_ARROW)
            s_func->flags |= RJS_FUNC_FL_ARROW;
#endif /*ENABLE_ARROW_FUNC*/

#if ENABLE_GENERATOR
        if (func->flags & RJS_AST_FUNC_FL_GENERATOR)
            s_func->flags |= RJS_FUNC_FL_GENERATOR;
#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC
        if (func->flags & RJS_AST_FUNC_FL_ASYNC)
            s_func->flags |= RJS_FUNC_FL_ASYNC;
#endif /*ENABLE_ASYNC*/

        s_func->name_idx        = rjs_code_gen_id_entry_idx(rt, func->name);
        s_func->reg_num         = bc_func->reg_num;
        s_func->param_len       = func->param_len;
        s_func->byte_code_start = bc_func->bc_start;
        s_func->byte_code_len   = bc_func->bc_size;
        s_func->line_info_start = bc_func->li_start;
        s_func->line_info_len   = bc_func->li_size;
        s_func->prop_ref_start  = bc_func->pr_start;
        s_func->prop_ref_len    = bc_func->pr_size;

#if ENABLE_FUNC_SOURCE
        s_func->source_idx = RJS_INVALID_VALUE_INDEX;

        if (!(func->flags & (RJS_AST_FUNC_FL_SCRIPT|RJS_AST_FUNC_FL_MODULE|RJS_AST_FUNC_FL_EVAL)))
            src_val_cnt ++;
#endif /*ENABLE_FUNC_SOURCE*/
    }

   /*Store module's binding groups.*/
#if ENABLE_MODULE
    script->mod_var_grp_idx  = rjs_code_gen_binding_table_idx(rt, bg->mod_var_table);
    script->mod_lex_grp_idx  = rjs_code_gen_binding_table_idx(rt, bg->mod_lex_table);
#endif /*ENABLE_MODULE*/

    /*Store binding groups.*/
    script->binding_group_num = parser->binding_table_num;
    RJS_NEW_N(rt, script->binding_group_table, script->binding_group_num);
    rjs_list_foreach_c(&parser->binding_table_list, bt, RJS_AstBindingTable, ast.ln) {
        if (bt->id != -1) {
            RJS_ScriptBindingGroup *sbg;

            sbg = &script->binding_group_table[bt->id];

            sbg->binding_start = script->binding_num;
            sbg->binding_num   = bt->num;
            sbg->decl_idx      = rjs_code_gen_decl_idx(rt, bt->decl);

            script->binding_num += bt->num;
        }
    }

   /*Store module's function group'.*/
#if ENABLE_MODULE
    script->mod_func_grp_idx = rjs_code_gen_func_table_idx(rt, bg->mod_func_table);
#endif /*ENABLE_MODULE*/

    /*Store function declaration groups.*/
    script->func_decl_group_num = parser->func_table_num;
    RJS_NEW_N(rt, script->func_decl_group_table, script->func_decl_group_num);
    rjs_list_foreach_c(&parser->func_table_list, ft, RJS_AstFuncTable, ast.ln) {
        if (ft->id != -1) {
            RJS_ScriptFuncDeclGroup *sfdp;

            sfdp = &script->func_decl_group_table[ft->id];

            sfdp->func_decl_start = script->func_decl_num;
            sfdp->func_decl_num   = ft->num;
            sfdp->decl_idx        = rjs_code_gen_decl_idx(rt, ft->decl);

            script->func_decl_num += ft->num;
        }
    }

    /*Store function declarations.*/
    RJS_NEW_N(rt, script->func_decl_table, script->func_decl_num);
    off = 0;
    rjs_list_foreach_c(&parser->func_table_list, ft, RJS_AstFuncTable, ast.ln) {
        RJS_AstFuncDeclRef      *fdr;

        rjs_list_foreach_c(&ft->func_decl_ref_list, fdr, RJS_AstFuncDeclRef, ast.ln) {
            if (fdr->func->id != -1) {
                RJS_ScriptFuncDecl *sfd;

                sfd = &script->func_decl_table[off];

                sfd->binding_ref_idx = rjs_code_gen_binding_ref_idx(rt, fdr->decl_item->binding_ref);
                sfd->func_idx        = rjs_code_gen_func_idx(rt, fdr->func);

                off ++;
            }
        }
    }

   /*Store module's declaration.*/
#if ENABLE_MODULE
    script->mod_decl_idx = rjs_code_gen_decl_idx(rt, bg->mod_decl);
#endif /*ENABLE_MODULE*/

    /*Store the declarations.*/
    script->decl_num = parser->decl_num;
    
    RJS_NEW_N(rt, script->decl_table, script->decl_num);

    rjs_list_foreach_c(&parser->decl_list, decl, RJS_AstDecl, ast.ln) {
        RJS_ScriptDecl    *s_decl;
        RJS_AstBindingRef *br;

        if (decl->id == -1)
            continue;

        s_decl = &script->decl_table[decl->id];

        s_decl->binding_ref_start = script->binding_ref_num;
        s_decl->binding_ref_num   = decl->binding_ref_num;

        rjs_list_foreach_c(&decl->binding_ref_list, br, RJS_AstBindingRef, ast.ln) {
            if (br->id != -1)
                rjs_code_gen_id_entry_idx(rt, br->name);
        }

        script->binding_ref_num += decl->binding_ref_num;
    }

    RJS_NEW_N(rt, script->binding_ref_table, script->binding_ref_num);

    /*Store the bindings.*/
    RJS_NEW_N(rt, script->binding_table, script->binding_num);
    off = 0;
    rjs_list_foreach_c(&parser->binding_table_list, bt, RJS_AstBindingTable, ast.ln) {
        RJS_AstBindingInit *bi;

        if (bt->id == -1)
            continue;

        rjs_list_foreach_c(&bt->binding_init_list, bi, RJS_AstBindingInit, ast.ln) {
            RJS_ScriptBinding *b;

            b = &script->binding_table[off];

            b->flags       = 0;
            b->ref_idx     = rjs_code_gen_binding_ref_idx(rt, bi->binding_ref);
            b->bot_ref_idx = -1;

            if (bi->flags & RJS_AST_BINDING_INIT_IMMUT)
                b->flags |= RJS_SCRIPT_BINDING_FL_CONST;
            if (bi->flags & RJS_AST_BINDING_INIT_UNDEF)
                b->flags |= RJS_SCRIPT_BINDING_FL_UNDEF;
            if (bi->flags & RJS_AST_BINDING_INIT_BOT) {
                b->flags |= RJS_SCRIPT_BINDING_FL_BOT;
                b->bot_ref_idx = rjs_code_gen_binding_ref_idx(rt, bi->bot_binding_ref);
            } else {
                b->bot_ref_idx = (bi->param_index == -1) ? RJS_INVALID_BINDING_REF_INDEX : bi->param_index;
            }
            if (bi->flags & RJS_AST_BINDING_INIT_STRICT)
                b->flags |= RJS_SCRIPT_BINDING_FL_STRICT;

            off ++;
        }
    }

#if ENABLE_PRIV_NAME
    /*Create the private environments.*/
    if (parser->priv_id_num) {
        RJS_AstPrivEnv    *ast_env;
        RJS_AstPrivId     *ast_pid;
        RJS_ScriptPrivEnv *env;
        RJS_ScriptPrivId  *pid;
        size_t             idx;

        script->priv_id_num  = parser->priv_id_num;
        script->priv_env_num = parser->priv_env_num;

        RJS_NEW_N(rt, script->priv_id_table, script->priv_id_num);
        RJS_NEW_N(rt, script->priv_env_table, script->priv_env_num);

        env = script->priv_env_table;
        pid = script->priv_id_table;
        idx = 0;

        rjs_list_foreach_c(&parser->priv_env_list, ast_env, RJS_AstPrivEnv, ast.ln) {
            if (ast_env->id != -1) {
                env->priv_id_start = idx;
                env->priv_id_num   = ast_env->priv_id_hash.entry_num;

                rjs_list_foreach_c(&ast_env->priv_id_list, ast_pid, RJS_AstPrivId, ast.ln) {
                    pid->idx = rjs_code_gen_value_entry_idx(rt, ast_pid->ve);
                    pid ++;
                }

                idx += env->priv_id_num;
                env ++;
            }
        }
    }
#endif /*ENABLE_PRIV_NAME*/

    /*Store the values.*/
    script->value_num = parser->value_entry_num + src_val_cnt;
    RJS_NEW_N(rt, script->value_table, script->value_num);
    rjs_value_buffer_fill_undefined(rt, script->value_table, script->value_num);
    
    rjs_list_foreach_c(&parser->value_entry_list, ve, RJS_AstValueEntry, ast.ln) {
        if (ve->id != -1) {
            RJS_Value *pv;

            pv = &script->value_table[ve->id];
            rjs_value_copy(rt, pv, &ve->value);
        }
    }

#if ENABLE_FUNC_SOURCE
    /*Store the functions' source text.*/
    off = parser->value_entry_num;
    rjs_list_foreach_c(&parser->func_list, func, RJS_AstFunc, ast.ln) {
        RJS_ScriptFunc *s_func;
        RJS_Value      *v;
        RJS_Input      *input;
        RJS_Location   *loc;

        if (func->id == -1)
            continue;

        if (func->flags & (RJS_AST_FUNC_FL_SCRIPT|RJS_AST_FUNC_FL_MODULE|RJS_AST_FUNC_FL_EVAL))
            continue;

        s_func = &script->func_table[func->id];
        input  = parser->lex.input;
        loc    = &func->ast.location;

        s_func->source_idx = off;
        v = &script->value_table[off];

        rjs_string_substr(rt, input->str, loc->first_pos, loc->last_pos, v);

        off ++;
    }
#endif /*ENABLE_FUNC_SOURCE*/

    /*Create the binding references.*/
    off = 0;
    rjs_list_foreach_c(&parser->decl_list, decl, RJS_AstDecl, ast.ln) {
        RJS_AstBindingRef *br;

        if (decl->id == -1)
            continue;

        rjs_list_foreach_c(&decl->binding_ref_list, br, RJS_AstBindingRef, ast.ln) {
            if (br->id != -1) {
                int                   idx = rjs_code_gen_id_entry_idx(rt, br->name);
                RJS_Value            *v   = &script->value_table[idx];
                RJS_ScriptBindingRef *sbr = &script->binding_ref_table[off + br->id];

                rjs_binding_name_init(rt, &sbr->binding_name, v);
            }
        }

        off += decl->binding_ref_num;
    }

    /*Store property references.*/
    script->prop_ref_num = parser->prop_ref_num;
    RJS_NEW_N(rt, script->prop_ref_table, script->prop_ref_num);

    rjs_list_foreach_c(&parser->prop_ref_list, pr, RJS_AstPropRef, ast.ln) {
        if (pr->id != -1) {
            int                idx = rjs_code_gen_id_entry_idx(rt, pr->prop);
            RJS_Value         *v   = &script->value_table[idx];
            RJS_ScriptPropRef *spr;

            spr = &script->prop_ref_table[pr->id + pr->func->prop_ref_start];

            rjs_property_name_init(rt, &spr->prop_name, v);
        }
    }

    return RJS_OK;
}

#if ENABLE_MODULE

/*Generate an export entry.*/
static void
gen_export (RJS_Runtime *rt, RJS_ExportEntry *ee, RJS_AstExport *ast)
{
    if (ast->module)
        ee->module_request_idx = ast->module->id;
    else
        ee->module_request_idx = RJS_INVALID_MODULE_REQUEST_INDEX;

    if (ast->import_name)
        ee->import_name_idx = rjs_code_gen_id_entry_idx(rt, ast->import_name);
    else
        ee->import_name_idx = RJS_INVALID_VALUE_INDEX;

    if (ast->local_name)
        ee->local_name_idx = rjs_code_gen_id_entry_idx(rt, ast->local_name);
    else
        ee->local_name_idx = RJS_INVALID_VALUE_INDEX;

    if (ast->export_name)
        ee->export_name_idx = rjs_code_gen_id_entry_idx(rt, ast->export_name);
    else
        ee->export_name_idx = RJS_INVALID_VALUE_INDEX;
}

/*Generate module.*/
static RJS_Result
gen_module_internal (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_Result  r;
    RJS_Module *mod;
    RJS_Script *script;
    size_t      en, i;
    RJS_Parser *parser = rt->parser;

    /*Allocate the module.*/
    mod = rjs_module_new(rt, rv, realm);

    /*Store the module request.*/
    mod->module_request_num = parser->module_request_hash.entry_num;
    if (mod->module_request_num) {
        RJS_AstModuleRequest *ast_mr;

        RJS_NEW_N(rt, mod->module_requests, mod->module_request_num);

        rjs_list_foreach_c(&parser->module_request_list, ast_mr, RJS_AstModuleRequest, ast.ln) {
            RJS_ModuleRequest *mr = mod->module_requests + ast_mr->id;

            rjs_value_set_undefined(rt, &mr->module);
            mr->module_name_idx = rjs_code_gen_value_entry_idx(rt, ast_mr->name);
        }
    }

    /*Store import entries.*/
    if (parser->import_num) {
        RJS_AstImport   *iast;
        RJS_ImportEntry *ie;

        mod->import_entry_num = parser->import_num;
        RJS_NEW_N(rt, mod->import_entries, mod->import_entry_num);

        ie = mod->import_entries;
        rjs_list_foreach_c(&parser->import_list, iast, RJS_AstImport, ast.ln) {
            if (iast->module)
                ie->module_request_idx = iast->module->id;
            else
                ie->module_request_idx = RJS_INVALID_MODULE_REQUEST_INDEX;

            if (iast->import_name)
                ie->import_name_idx = rjs_code_gen_id_entry_idx(rt, iast->import_name);
            else
                ie->import_name_idx = RJS_INVALID_VALUE_INDEX;

            if (iast->local_name)
                ie->local_name_idx = rjs_code_gen_id_entry_idx(rt, iast->local_name);
            else
                ie->local_name_idx = RJS_INVALID_VALUE_INDEX;

            ie ++;
        }
    }

    /*Store export entries.*/
    en = parser->local_export_num + parser->indir_export_num + parser->star_export_num;
    if (en) {
        RJS_AstExport   *east;
        RJS_ExportEntry *ee;

        mod->local_export_entry_num = parser->local_export_num;
        mod->indir_export_entry_num = parser->indir_export_num;
        mod->star_export_entry_num  = parser->star_export_num;

        RJS_NEW_N(rt, mod->export_entries, en);

        ee = mod->export_entries;
        
        rjs_list_foreach_c(&parser->local_export_list, east, RJS_AstExport, ast.ln) {
            gen_export(rt, ee, east);
            ee ++;
        }

        rjs_list_foreach_c(&parser->indir_export_list, east, RJS_AstExport, ast.ln) {
            gen_export(rt, ee, east);
            ee ++;
        }

        rjs_list_foreach_c(&parser->star_export_list, east, RJS_AstExport, ast.ln) {
            gen_export(rt, ee, east);
            ee ++;
        }
    }

    /*Generate base script data.*/
    script = &mod->script;

    if ((r = gen_script_internal(rt, script)) == RJS_ERR)
        return r;

    /*Build the export hash table.*/
    for (i = 0; i < en; i ++) {
        RJS_ExportEntry *ee = &mod->export_entries[i];

        if (ee->export_name_idx != RJS_INVALID_VALUE_INDEX) {
            RJS_Value  *name;
            RJS_String *str;

            name = &script->value_table[ee->export_name_idx];
            str  = rjs_value_get_string(rt, name);

            rjs_hash_insert(&mod->export_hash, str, &ee->he, NULL, &rjs_hash_size_ops, rt);
        }
    }

    return RJS_OK;
}

#endif /*ENABLE_MODULE*/

#if ENABLE_SCRIPT || ENABLE_EVAL

/*Generate script.*/
static RJS_Result
gen_script (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_Parser *parser = rt->parser;
    RJS_Script *script;
    RJS_BcGen   bg;
    RJS_Result  r;

    rjs_bc_gen_init(rt, &bg);
    parser->code_gen   = &bg;
    parser->decl_stack = NULL;

    rjs_script_new(rt, rv, realm);

    script = rjs_value_get_gc_thing(rt, rv);

    r = gen_script_internal(rt, script);

    rjs_bc_gen_deinit(rt, &bg);

    return r;
}

#endif /*ENABLE_SCRIPT || ENABLE_EVAL*/

#if ENABLE_MODULE

/*Generate module.*/
static RJS_Result
gen_module (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *rv)
{
    RJS_Parser *parser = rt->parser;
    RJS_BcGen   bg;
    RJS_Result  r;

    rjs_bc_gen_init(rt, &bg);
    parser->code_gen   = &bg;
    parser->decl_stack = NULL;

    r = gen_module_internal(rt, realm, rv);

    rjs_bc_gen_deinit(rt, &bg);

    return r;
}

#endif /*ENABLE_MODULE*/

/**
 * Push a declaration to the stack.
 * \param rt The current runtime.
 * \param decl The declaration to be pushed.
 */
void
rjs_code_gen_push_decl (RJS_Runtime *rt, RJS_AstDecl *decl)
{
    RJS_Parser *parser = rt->parser;

    decl->bot = parser->decl_stack;
    parser->decl_stack = decl;
}

/**
 * Popup the top declaration from the stack.
 * \param rt The current runtime.
 */
void
rjs_code_gen_pop_decl (RJS_Runtime *rt)
{
    RJS_Parser  *parser = rt->parser;
    RJS_AstDecl *decl   = parser->decl_stack;

    assert(decl);

    parser->decl_stack = decl->bot;
}

/**
 * Create the binding initialize table.
 * \param rt The current runtime.
 * \param tab The binding initlaize table.
 * \param decl The declaration.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error. 
 */
RJS_Result
rjs_code_gen_binding_init_table (RJS_Runtime *rt, RJS_Value *tab, RJS_AstDecl *decl)
{
    binding_init_table(rt, tab, decl, BINDING_INIT_FL_LEX|BINDING_INIT_FL_FUNC);

    return RJS_OK;
}

/**
 * Get the binding reference from the identifier.
 * \param rt The current runtime.
 * \param loc The location.
 * \param id The identifier.
 * \return The binding reference.
 */
RJS_AstBindingRef*
rjs_code_gen_binding_ref (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *id)
{
    RJS_Parser  *parser = rt->parser;
    RJS_AstDecl *decl   = parser->decl_stack;

    return binding_ref_new(rt, decl, loc, id);
}

/**
 * Get the property reference from the identifier.
 * \param rt The current runtime.
 * \param v The value to store the reference.
 * \param loc The location.
 * \param func The function contains this property reference.
 * \param id The identifier.
 * \return The binding reference.
 */
RJS_AstPropRef*
rjs_code_gen_prop_ref (RJS_Runtime *rt, RJS_Value *v, RJS_Location *loc, RJS_AstFunc *func, RJS_Value *id)
{
    return prop_ref_new(rt, v, loc, func, id);
}

/**
 * Get the value entry.
 * \param rt The current runtime.
 * \param loc The location.
 * \param id The identifier.
 * \return The binding reference.
 */
RJS_AstValueEntry*
rjs_code_gen_value_entry (RJS_Runtime *rt, RJS_Location *loc, RJS_Value *v)
{
    return value_entry_add(rt, loc, v);
}

/**
 * Get the binding table's index.
 * \param rt The current runtime.
 * \param bt The binding table.
 * \return The binding table's index.
 */
int
rjs_code_gen_binding_table_idx (RJS_Runtime *rt, RJS_AstBindingTable *bt)
{
    RJS_Parser *parser = rt->parser;

    if (!bt)
        return -1;

    if (bt->id == -1) {
        RJS_AstBindingInit *bi;

        bt->id = parser->binding_table_num ++;

        rjs_list_foreach_c(&bt->binding_init_list, bi, RJS_AstBindingInit, ast.ln) {
            rjs_code_gen_binding_ref_idx(rt, bi->binding_ref);
        }
    }

    return bt->id;
}

/**
 * Get the function table's index.
 * \param rt The current runtime.
 * \param ft The function table.
 * \return The function table's index.
 */
int
rjs_code_gen_func_table_idx (RJS_Runtime *rt, RJS_AstFuncTable *ft)
{
    RJS_Parser *parser = rt->parser;

    if (!ft)
        return -1;

    if (ft->id == -1) {
        RJS_AstFuncDeclRef *fdr;

        rjs_list_foreach_c(&ft->func_decl_ref_list, fdr, RJS_AstFuncDeclRef, ast.ln) {
            rjs_code_gen_func_idx(rt, fdr->func);
            rjs_code_gen_binding_ref_idx(rt, fdr->decl_item->binding_ref);
        }

        rjs_code_gen_decl_idx(rt, ft->decl);

        ft->id = parser->func_table_num ++;
    }

    return ft->id;
}

#if ENABLE_PRIV_NAME

/**
 * Get the private environment's index.
 * \param rt The current runtime.
 * \param pe The private environment AST node.
 * \return The private environment's index.
 */
int
rjs_code_gen_priv_env_idx (RJS_Runtime *rt, RJS_AstPrivEnv *pe)
{
    RJS_Parser *parser = rt->parser;

    if (pe->id == -1) {
        if (pe->priv_id_hash.entry_num) {
            pe->id = parser->priv_env_num ++;
            parser->priv_id_num += pe->priv_id_hash.entry_num;
        }
    }

    return pe->id;
}

#endif /*ENABLE_PRIV_NAME*/
