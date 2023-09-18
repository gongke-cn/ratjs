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

/*Scan the referneced things in the module.*/
static void
mod_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Module *mod = ptr;

    rjs_script_op_gc_scan(rt, &mod->script);

    rjs_gc_scan_value(rt, &mod->eval_error);
    rjs_gc_scan_value(rt, &mod->namespace);
    rjs_gc_scan_value(rt, &mod->import_meta);

    if (mod->env)
        rjs_gc_mark(rt, mod->env);

#if ENABLE_ASYNC
    rjs_gc_scan_value(rt, &mod->cycle_root);
    rjs_gc_scan_value(rt, &mod->promise);
    rjs_gc_scan_value(rt, &mod->resolve);
    rjs_gc_scan_value(rt, &mod->reject);
#endif /*ENABLE_ASYNC*/

    rjs_gc_scan_value(rt, &mod->top_promise);
    rjs_gc_scan_value(rt, &mod->top_resolve);
    rjs_gc_scan_value(rt, &mod->top_reject);

    /*Scan module request.*/
    if (mod->module_requests) {
        size_t i;

        for (i = 0; i < mod->module_request_num; i ++) {
            RJS_ModuleRequest *mr = &mod->module_requests[i];

            rjs_gc_scan_value(rt, &mr->module);
        }
    }

    /*Scan the native data.*/
    rjs_native_data_scan(rt, &mod->native_data);
}

#if ENABLE_ASYNC
static void
mod_clear_async_parent_list (RJS_Runtime *rt, RJS_Module *mod)
{
    RJS_ModuleAsyncParent *p, *np;

    rjs_list_foreach_safe_c(&mod->async_parent_list, p, np, RJS_ModuleAsyncParent, ln) {
        RJS_DEL(rt, p);
    }

    rjs_list_init(&mod->async_parent_list);
}
#endif /*ENABLE_ASYNC*/

/*Free the module.*/
static void
mod_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Module *mod = ptr;
    size_t      en;

    rjs_script_deinit(rt, &mod->script);

#if ENABLE_ASYNC
    mod_clear_async_parent_list(rt, mod);
    rjs_promise_capability_deinit(rt, &mod->capability);
#endif /*ENABLE_ASYNC*/

    en = mod->local_export_entry_num
            + mod->indir_export_entry_num
            + mod->star_export_entry_num;

    if (mod->module_requests)
        RJS_DEL_N(rt, mod->module_requests, mod->module_request_num);
    if (mod->import_entries)
        RJS_DEL_N(rt, mod->import_entries, mod->import_entry_num);
    if (mod->export_entries)
        RJS_DEL_N(rt, mod->export_entries, en);

    rjs_hash_deinit(&mod->export_hash, &rjs_hash_size_ops, rt);

    rjs_promise_capability_deinit(rt, &mod->top_level_capability);

    /*Free the native data.*/
    rjs_native_data_free(rt, &mod->native_data);

    /*Unload the native module.*/
    if (mod->native_handle)
        dlclose(mod->native_handle);

    RJS_DEL(rt, mod);
}

/*Module GC operation functions.*/
static const RJS_GcThingOps
mod_gc_ops = {
    RJS_GC_THING_MODULE,
    mod_op_gc_scan,
    mod_op_gc_free
};

/*Check if 2 module values are same.*/
static RJS_Bool
same_module (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_Module *m1, *m2;

    if (!rjs_value_is_module(rt, v1))
        return RJS_FALSE;

    if (!rjs_value_is_module(rt, v2))
        return RJS_FALSE;

    m1 = rjs_value_get_gc_thing(rt, v1);
    m2 = rjs_value_get_gc_thing(rt, v2);

    return (m1 == m2) ? RJS_TRUE : RJS_FALSE;
}

#if ENABLE_ASYNC

/**Async module reaction function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;    /**< Base built-in function object data.*/
    RJS_Value             module; /**< The module.*/
} RJS_AsyncModuleFunc;

/*Scan the referenced things in the async module function.*/
static void
async_module_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncModuleFunc *amf = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, ptr);

    rjs_gc_scan_value(rt, &amf->module);
}

/*Free the async module function.*/
static void
async_module_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_AsyncModuleFunc *amf = ptr;

    rjs_builtin_func_object_deinit(rt, &amf->bfo);

    RJS_DEL(rt, amf);
}

/**Async module reaction function operation functions.*/
static const RJS_ObjectOps
async_module_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        async_module_func_op_gc_scan,
        async_module_func_op_gc_free
    },
    RJS_BUILTIN_CONSTRUCTOR_OBJECT_OPS
};

#endif /*ENABLE_ASYNC*/

/**
 * Create a new module.
 * \param rt The current runtime.
 * \param[out] v Return the module value.
 * \param realm The realm.
 * \return The new module.
 */
RJS_Module*
rjs_module_new (RJS_Runtime *rt, RJS_Value *v, RJS_Realm *realm)
{
    RJS_Module *mod;

    RJS_NEW(rt, mod);

    rjs_script_init(rt, &mod->script, realm);

    mod->status                 = RJS_MODULE_STATUS_UNLINKED;
    mod->dfs_index              = 0;
    mod->dfs_ancestor_index     = 0;
    mod->eval_result            = RJS_OK;
    mod->env                    = NULL;
    mod->module_requests        = NULL;
    mod->import_entries         = NULL;
    mod->export_entries         = NULL;
    mod->native_handle          = NULL;
    mod->module_request_num     = 0;
    mod->import_entry_num       = 0;
    mod->local_export_entry_num = 0;
    mod->indir_export_entry_num = 0;
    mod->star_export_entry_num  = 0;

    rjs_native_data_init(&mod->native_data);

    rjs_value_set_undefined(rt, &mod->top_promise);
    rjs_value_set_undefined(rt, &mod->top_resolve);
    rjs_value_set_undefined(rt, &mod->top_reject);
    rjs_promise_capability_init_vp(rt, &mod->top_level_capability,
            &mod->top_promise, &mod->top_resolve, &mod->top_reject);

#if ENABLE_ASYNC
    mod->pending_async = 0;
    mod->async_eval    = RJS_FALSE;

    rjs_value_set_undefined(rt, &mod->promise);
    rjs_value_set_undefined(rt, &mod->resolve);
    rjs_value_set_undefined(rt, &mod->reject);
    rjs_promise_capability_init_vp(rt, &mod->capability,
            &mod->promise, &mod->resolve, &mod->reject);

    rjs_value_set_undefined(rt, &mod->cycle_root);
    rjs_list_init(&mod->async_parent_list);
#endif /*ENABLE_ASYNC*/

    rjs_hash_init(&mod->export_hash);

    rjs_value_set_undefined(rt, &mod->eval_error);
    rjs_value_set_undefined(rt, &mod->namespace);
    rjs_value_set_undefined(rt, &mod->import_meta);

    rjs_value_set_gc_thing(rt, v, mod);
    rjs_gc_add(rt, mod, &mod_gc_ops);

    return mod;
}

/*Module declaration instantiation.*/
static RJS_Result
module_declaration_instantiation (RJS_Runtime *rt,
        RJS_Value *modv,
        RJS_Environment *env,
        RJS_ScriptDecl *decl,
        RJS_ScriptBindingGroup *var_grp,
        RJS_ScriptBindingGroup *lex_grp,
        RJS_ScriptFuncDeclGroup *func_grp)
{
    RJS_Module      *mod    = rjs_value_get_gc_thing(rt, modv);
    RJS_Script      *script = &mod->script;
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *func   = rjs_value_stack_push(rt);
    size_t           i;

    env->script_decl = decl;

    if (var_grp) {
        for (i = 0; i < var_grp->binding_num; i ++) {
            RJS_ScriptBinding    *sb  = &script->binding_table[var_grp->binding_start + i];
            RJS_ScriptBindingRef *sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            rjs_env_create_mutable_binding(rt, env, &sbr->binding_name, RJS_FALSE);
            rjs_env_initialize_binding(rt, env, &sbr->binding_name, rjs_v_undefined(rt));
        }
    }

    if (lex_grp) {
        for (i = 0; i < lex_grp->binding_num; i ++) {
            RJS_ScriptBinding    *sb  = &script->binding_table[lex_grp->binding_start + i];
            RJS_ScriptBindingRef *sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if (sb->flags & RJS_SCRIPT_BINDING_FL_CONST)
                rjs_env_create_immutable_binding(rt, env, &sbr->binding_name, RJS_TRUE);
            else
                rjs_env_create_mutable_binding(rt, env, &sbr->binding_name, RJS_FALSE);
        }
    }

    if (func_grp) {
        for (i = 0; i < func_grp->func_decl_num; i ++) {
            RJS_ScriptFuncDecl   *sfd = &script->func_decl_table[func_grp->func_decl_start + i];
            RJS_ScriptFunc       *sf  = &script->func_table[sfd->func_idx];
            RJS_ScriptBindingRef *sbr = &script->binding_ref_table[decl->binding_ref_start + sfd->binding_ref_idx];

            rjs_create_function(rt, script, sf, env, NULL, RJS_TRUE, func);
            rjs_env_initialize_binding(rt, env, &sbr->binding_name, func);
        }
    }

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*Initialize the module environment.*/
static RJS_Result
module_init_env (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module        *mod    = rjs_value_get_gc_thing(rt, modv);
    RJS_Script        *script = &mod->script;
    size_t             top    = rjs_value_stack_save(rt);
    RJS_Value         *v      = rjs_value_stack_push(rt);
    RJS_ResolveBinding rb;
    size_t             i, end;
    RJS_Result         r;
    RJS_ScriptDecl    *decl;
    RJS_ScriptBindingGroup  *var_grp, *lex_grp;
    RJS_ScriptFuncDeclGroup *func_grp;

    /*Resolve indirect export entries.*/
    rjs_resolve_binding_init(rt, &rb);

    i   = mod->local_export_entry_num;
    end = i + mod->indir_export_entry_num;
    while (i < end) {
        RJS_ExportEntry *ee;
        RJS_Value       *name;

        ee   = &mod->export_entries[i];
        name = &script->value_table[ee->export_name_idx];

        if ((r = rjs_module_resolve_export(rt, modv, name, &rb)) == RJS_ERR)
            goto end;
        if (r == RJS_AMBIGUOUS) {
            r = rjs_throw_syntax_error(rt, _("ambiguous export name \"%s\""),
                    rjs_string_to_enc_chars(rt, name, NULL, NULL));
            goto end;
        }
        if (!r) {
            r = rjs_throw_syntax_error(rt, _("cannot resolve export name \"%s\""),
                    rjs_string_to_enc_chars(rt, name, NULL, NULL));
            goto end;
        }

        i ++;
    }

    /*Create module environment.*/
    rjs_module_env_new(rt, &mod->env, rjs_global_env(script->realm));

    /*Resolve import entries.*/
    for (i = 0; i < mod->import_entry_num; i ++) {
        RJS_ImportEntry   *ie;
        RJS_ModuleRequest *mr;
        RJS_Value         *ln;
        RJS_BindingName    bn;

        ie = &mod->import_entries[i];
        mr = &mod->module_requests[ie->module_request_idx];

        if (rjs_value_is_undefined(rt, &mr->module)) {
            RJS_Value *name = &script->value_table[mr->module_name_idx];

            if ((r = rjs_resolve_imported_module(rt, modv, name, &mr->module)) == RJS_ERR)
                goto end;
        }

        if (ie->import_name_idx == RJS_INVALID_VALUE_INDEX) {
            if ((r = rjs_module_get_namespace(rt, &mr->module, v)) == RJS_ERR)
                goto end;

            ln = &script->value_table[ie->local_name_idx];

            rjs_binding_name_init(rt, &bn, ln);

            rjs_env_create_immutable_binding(rt, mod->env, &bn, RJS_TRUE);
            rjs_env_initialize_binding(rt, mod->env, &bn, v);

            rjs_binding_name_deinit(rt, &bn);
        } else {
            RJS_Value *iname = &script->value_table[ie->import_name_idx];

            if ((r = rjs_module_resolve_export(rt, &mr->module, iname, &rb)) == RJS_ERR)
                goto end;
            if (r == RJS_AMBIGUOUS) {
                r = rjs_throw_syntax_error(rt, _("ambiguous export name \"%s\""),
                        rjs_string_to_enc_chars(rt, iname, NULL, NULL));
                goto end;
            }
            if (!r) {
                r = rjs_throw_syntax_error(rt, _("cannot resolve import name \"%s\""),
                        rjs_string_to_enc_chars(rt, iname, NULL, NULL));
                goto end;
            }

            if (rjs_value_is_undefined(rt, rb.name)) {
                if ((r = rjs_module_get_namespace(rt, rb.module, v)) == RJS_ERR)
                    goto end;

                ln = &script->value_table[ie->local_name_idx];
                rjs_binding_name_init(rt, &bn, ln);

                rjs_env_create_immutable_binding(rt, mod->env, &bn, RJS_TRUE);
                rjs_env_initialize_binding(rt, mod->env, &bn, v);

                rjs_binding_name_deinit(rt, &bn);
            } else {
                ln = &script->value_table[ie->local_name_idx];

                rjs_env_create_import_binding(rt, mod->env, ln, rb.module, rb.name);
            }
        }
    }

    /*Initialize the module's declaration.*/
    decl     = (script->mod_decl_idx == -1) ? NULL : &script->decl_table[script->mod_decl_idx];
    var_grp  = (script->mod_var_grp_idx == -1) ? NULL : &script->binding_group_table[script->mod_var_grp_idx];
    lex_grp  = (script->mod_lex_grp_idx == -1) ? NULL : &script->binding_group_table[script->mod_lex_grp_idx];
    func_grp = (script->mod_func_grp_idx == -1) ? NULL : &script->func_decl_group_table[script->mod_func_grp_idx];

    module_declaration_instantiation(rt, modv, mod->env, decl, var_grp, lex_grp, func_grp);

    r = RJS_OK;
end:
    rjs_resolve_binding_deinit(rt, &rb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Link the module.*/
static RJS_Result
inner_module_link (RJS_Runtime *rt, RJS_Value *modv, RJS_List *stack, int index)
{
    RJS_Module *mod    = rjs_value_get_gc_thing(rt, modv);
    RJS_Script *script = &mod->script;
    size_t      i;
    RJS_Result  r;

    if ((mod->status == RJS_MODULE_STATUS_LINKING)
            || (mod->status == RJS_MODULE_STATUS_LINKED)
            || (mod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC)
            || (mod->status == RJS_MODULE_STATUS_EVALUATED))
        return index;

    assert(mod->status == RJS_MODULE_STATUS_UNLINKED);

    mod->status             = RJS_MODULE_STATUS_LINKING;
    mod->dfs_index          = index;
    mod->dfs_ancestor_index = index;

    index ++;

    rjs_list_prepend(stack, &mod->ln);

    for (i = 0; i < mod->module_request_num; i ++) {
        RJS_ModuleRequest *mr = &mod->module_requests[i];

        if (rjs_value_is_undefined(rt, &mr->module)) {
            RJS_Value  *name = &script->value_table[mr->module_name_idx];
            RJS_Module *rmod;

            if ((r = rjs_resolve_imported_module(rt, modv, name, &mr->module)) == RJS_ERR)
                return r;

            if ((r = inner_module_link(rt, &mr->module, stack, index)) == RJS_ERR)
                return r;

            index = r;

            rmod = rjs_value_get_gc_thing(rt, &mr->module);
            if (rmod->status == RJS_MODULE_STATUS_LINKING)
                mod->dfs_ancestor_index = RJS_MIN(rmod->dfs_ancestor_index, mod->dfs_ancestor_index);
        }
    }

    if ((r = module_init_env(rt, modv)) == RJS_ERR)
        return r;

    if (mod->dfs_index == mod->dfs_ancestor_index) {
        RJS_Module *tmod, *nmod;

        rjs_list_foreach_safe_c(stack, tmod, nmod, RJS_Module, ln) {
            rjs_list_remove(&tmod->ln);

            tmod->status = RJS_MODULE_STATUS_LINKED;
            if(tmod == mod)
                break;
        }
    }

    return index;
}

/**
 * Link the module.
 * \param rt The current runtime.
 * \param modv The module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_link (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module *mod;
    RJS_Result  r;
    RJS_List    stack;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    assert((mod->status != RJS_MODULE_STATUS_LINKING)
            && (mod->status != RJS_MODULE_STATUS_EVALUATING));

    rjs_list_init(&stack);
    r = inner_module_link(rt, modv, &stack, 0);
    if (r == RJS_ERR) {
        RJS_Module *m;

        rjs_list_foreach_c(&stack, m, RJS_Module, ln) {
            assert(m->status == RJS_MODULE_STATUS_LINKING);
            m->status = RJS_MODULE_STATUS_UNLINKED;
        }

        assert(mod->status == RJS_MODULE_STATUS_UNLINKED);
        return r;
    }

    assert((mod->status == RJS_MODULE_STATUS_LINKED)
            || (mod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC)
            || (mod->status == RJS_MODULE_STATUS_EVALUATED));

    assert(rjs_list_is_empty(&stack));

    return RJS_OK;
}

#if ENABLE_ASYNC

/*Check if the module has top level await.*/
static RJS_Bool
module_has_tla (RJS_Runtime *rt, RJS_Module *mod)
{
    RJS_Script     *script = &mod->script;
    RJS_ScriptFunc *sf;

    if (script->func_num == 0)
        return RJS_FALSE;

    sf = &script->func_table[0];

    return (sf->flags & RJS_FUNC_FL_ASYNC) ? RJS_TRUE : RJS_FALSE;
}

#endif /*ENABLE_ASYNC*/

/*Execute the module.*/
static RJS_Result
execute_module (RJS_Runtime *rt, RJS_Value *modv, RJS_PromiseCapability *pc)
{
    RJS_Module     *mod;
    RJS_Script     *script;
    RJS_ScriptFunc *sf;
    RJS_Context    *ctxt;
    size_t          top = rjs_value_stack_save(rt);
    RJS_Value      *rv  = rjs_value_stack_push(rt);
    RJS_Result      r   = RJS_OK;

    mod    = rjs_value_get_gc_thing(rt, modv);
    script = &mod->script;

    if (script->func_num) {
        sf = &script->func_table[0];

        if (!pc) {
            /*Sync mode.*/
            ctxt = rjs_script_context_push(rt, NULL, script, sf, mod->env, mod->env, NULL, NULL, 0);

            ctxt->realm = script->realm;

            r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_SYNC_START, NULL, rv);

            rjs_context_pop(rt);
        }
#if ENABLE_ASYNC
        else {
            /*Async mode.*/
            RJS_Value *rvp = rjs_value_get_pointer(rt, rv);

            ctxt = rjs_async_context_push(rt, NULL, script, sf, mod->env, mod->env, NULL, NULL, 0, pc);

            ctxt->realm = script->realm;

            r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_ASYNC_START, NULL, rvp);

            rjs_context_pop(rt);
        }
#endif /*ENABLE_ASYNC*/
    } else if (mod->native_handle) {
        RJS_ModuleExecFunc ef;

        ef = dlsym(mod->native_handle, "ratjs_module_exec");
        if (ef) {
            if ((r = ef(rt, modv)) == RJS_ERR)
                RJS_LOGE("native module execute failed");
        }
    }

    rjs_value_stack_restore(rt, top);
    return (r == RJS_ERR) ? RJS_ERR : RJS_OK;
}

#if ENABLE_ASYNC

/*Execute the module in async mode.*/
static RJS_Result
execute_async_module (RJS_Runtime *rt, RJS_Value *modv);
/*Async module rejected.*/
static RJS_Result
async_module_execution_rejected (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *err);

/*Create a new async module function.*/
static RJS_Result
async_module_func_new (RJS_Runtime *rt, RJS_Value *f, RJS_NativeFunc nf, RJS_Value *modv)
{
    RJS_AsyncModuleFunc *amf;
    RJS_Module          *mod    = rjs_value_get_gc_thing(rt, modv);
    RJS_Script          *script = &mod->script;

    RJS_NEW(rt, amf);

    rjs_value_copy(rt, &amf->module, modv);

    return rjs_init_builtin_function(rt, &amf->bfo, nf, 0, &async_module_func_ops,
            0, rjs_s_empty(rt), script->realm, NULL, NULL, NULL, f);
}

/*Execute the valid modules.*/
static void
execute_valid_modules (RJS_Runtime *rt, RJS_Value *modv, RJS_Hash *hash)
{
    RJS_Module            *mod;
    RJS_ModuleAsyncParent *parent;
    size_t                 top = rjs_value_stack_save(rt);
    RJS_Value             *err = rjs_value_stack_push(rt);

    mod = rjs_value_get_gc_thing(rt, modv);

    rjs_list_foreach_c(&mod->async_parent_list, parent, RJS_ModuleAsyncParent, ln) {
        RJS_Module    *rmod;
        RJS_HashEntry *he;
        RJS_Result     r;

        rmod = rjs_value_get_gc_thing(rt, &parent->module);
        r    = rjs_hash_lookup(hash, rmod, &he, NULL, &rjs_hash_size_ops, rt);
        if (r)
            continue;

        assert(rmod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC);
        assert(rmod->eval_result == RJS_OK);
        assert(rmod->async_eval);
        assert(rmod->pending_async > 0);

        rmod->pending_async --;
        if (rmod->pending_async == 0) {

            if (rmod->status == RJS_MODULE_STATUS_EVALUATED) {
                assert(rmod->eval_result == RJS_ERR);
            } else if (module_has_tla(rt, rmod)) {
                execute_async_module(rt, &parent->module);
            } else {
                RJS_Result r;

                r = execute_module(rt, &parent->module, NULL);
                if (r == RJS_ERR) {
                    rjs_catch(rt, err);
                    async_module_execution_rejected(rt, &parent->module, err);
                } else {
                    rmod->status = RJS_MODULE_STATUS_EVALUATED;

                    if (!rjs_value_is_undefined(rt, rmod->top_level_capability.promise)) {
                        assert(same_module(rt, &rmod->cycle_root, &parent->module));

                        rjs_call(rt, rmod->top_level_capability.resolve, rjs_v_undefined(rt), NULL, 0, NULL);
                    }
                }
            }
            
            if (!module_has_tla(rt, rmod))
                execute_valid_modules(rt, &parent->module, hash);
        }
    }

    rjs_value_stack_restore(rt, top);
}

/*Async module fulfilled.*/
static RJS_Result
async_module_execution_fulfilled (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module    *mod;
    RJS_Hash       hash;
    RJS_HashEntry *he, *nhe;
    size_t         i;

    mod = rjs_value_get_gc_thing(rt, modv);

    if (mod->status == RJS_MODULE_STATUS_EVALUATED) {
        assert(mod->eval_result == RJS_ERR);
        return RJS_OK;
    }

    mod->async_eval = RJS_FALSE;
    mod->status     = RJS_MODULE_STATUS_EVALUATED;

    if (!rjs_value_is_undefined(rt, mod->top_level_capability.promise)) {
        rjs_call(rt, mod->top_level_capability.resolve, rjs_v_undefined(rt), NULL, 0, NULL);
    }

    rjs_hash_init(&hash);

    execute_valid_modules(rt, modv, &hash);

    rjs_hash_foreach_safe(&hash, i, he, nhe) {
        RJS_DEL(rt, he);
    }

    rjs_hash_deinit(&hash, &rjs_hash_size_ops, rt);

    return RJS_OK;
}

/*Async module fulfilled callback.*/
static RJS_Result
async_module_execution_fulfilled_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc,
        RJS_Value *nt, RJS_Value *rv)
{
    RJS_AsyncModuleFunc *amf;

    amf = (RJS_AsyncModuleFunc*)rjs_value_get_object(rt, f);

    rjs_value_set_undefined(rt, rv);

    return async_module_execution_fulfilled(rt, &amf->module);
}

/*Async module rejected.*/
static RJS_Result
async_module_execution_rejected (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *err)
{
    RJS_Module            *mod;
    RJS_ModuleAsyncParent *parent;

    mod = rjs_value_get_gc_thing(rt, modv);

    if (mod->status == RJS_MODULE_STATUS_EVALUATED) {
        assert(mod->eval_result == RJS_ERR);
        return RJS_OK;
    }

    assert(mod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC);

    mod->status      = RJS_MODULE_STATUS_EVALUATED;
    mod->eval_result = RJS_ERR;
    rjs_value_copy(rt, &mod->eval_error, err);

    rjs_list_foreach_c(&mod->async_parent_list, parent, RJS_ModuleAsyncParent, ln) {
        async_module_execution_rejected(rt, &parent->module, err);
    }

    if (!rjs_value_is_undefined(rt, mod->top_level_capability.promise)) {
        assert(same_module(rt, &mod->cycle_root, modv));

        rjs_call(rt, mod->top_level_capability.reject, rjs_v_undefined(rt), err, 1, NULL);
    }

    return RJS_OK;
}

/*Async module rejected callback.*/
static RJS_Result
async_module_execution_rejected_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc,
        RJS_Value *nt, RJS_Value *rv)
{
    RJS_AsyncModuleFunc *amf;
    RJS_Value           *err = rjs_argument_get(rt, args, argc, 0);

    amf = (RJS_AsyncModuleFunc*)rjs_value_get_object(rt, f);

    rjs_value_set_undefined(rt, rv);

    return async_module_execution_rejected(rt, &amf->module, err);
}

/*Execute the module in async mode.*/
static RJS_Result
execute_async_module (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module  *mod;
    RJS_Script  *script;
    RJS_Result   r;
    size_t       top     = rjs_value_stack_save(rt);
    RJS_Value   *fulfill = rjs_value_stack_push(rt);
    RJS_Value   *reject  = rjs_value_stack_push(rt);
    RJS_Value   *promise = rjs_value_stack_push(rt);

    mod    = rjs_value_get_gc_thing(rt, modv);
    script = &mod->script;

    rjs_new_promise_capability(rt, rjs_o_Promise(script->realm), &mod->capability);

    async_module_func_new(rt, fulfill, async_module_execution_fulfilled_nf, modv);
    async_module_func_new(rt, reject, async_module_execution_rejected_nf, modv);

    rjs_perform_proimise_then(rt, mod->capability.promise, fulfill, reject, NULL, promise);

    r = execute_module(rt, modv, &mod->capability);

    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_ASYNC*/

/*Inner module evaluation funciton.*/
static RJS_Result
inner_module_evaluation (RJS_Runtime *rt, RJS_Value *modv, RJS_List *stack, int index)
{
    RJS_Module *mod;
    RJS_Script *script;
    RJS_Result  r;
    size_t      i;

    mod    = rjs_value_get_gc_thing(rt, modv);
    script = &mod->script;

    if ((mod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC)
            || (mod->status == RJS_MODULE_STATUS_EVALUATED)) {
        if (mod->eval_result == RJS_OK)
            return index;

        rjs_throw(rt, &mod->eval_error);
        return mod->eval_result;
    }

    if (mod->status == RJS_MODULE_STATUS_EVALUATING)
        return index;

    assert(mod->status == RJS_MODULE_STATUS_LINKED);

    mod->status             = RJS_MODULE_STATUS_EVALUATING;
    mod->dfs_index          = index;
    mod->dfs_ancestor_index = index;

    index ++;

    rjs_list_prepend(stack, &mod->ln);

    for (i = 0; i < mod->module_request_num; i ++) {
        RJS_ModuleRequest *mr = &mod->module_requests[i];
        RJS_Module        *rmod;

        if (rjs_value_is_undefined(rt, &mr->module)) {
            RJS_Value *name = &script->value_table[mr->module_name_idx];

            if ((r = rjs_resolve_imported_module(rt, modv, name, &mr->module)) == RJS_ERR)
                return r;
        }

        if ((r = inner_module_evaluation(rt, &mr->module, stack, index)) == RJS_ERR)
            return r;

        index = r;

        rmod = rjs_value_get_gc_thing(rt, &mr->module);

        assert((rmod->status == RJS_MODULE_STATUS_EVALUATING)
                || (rmod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC)
                || (rmod->status == RJS_MODULE_STATUS_EVALUATED));

        if (rmod->status == RJS_MODULE_STATUS_EVALUATING) {
            mod->dfs_ancestor_index = RJS_MIN(mod->dfs_ancestor_index, rmod->dfs_ancestor_index);
        }
#if ENABLE_ASYNC
        else {
            rmod = rjs_value_get_gc_thing(rt, &rmod->cycle_root);

            assert((rmod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC)
                    || (rmod->status == RJS_MODULE_STATUS_EVALUATED));

            if (rmod->eval_result == RJS_ERR) {
                rjs_throw(rt, &rmod->eval_error);
                return rmod->eval_result;
            }
        }

        if (rmod->async_eval) {
            RJS_ModuleAsyncParent *parent;

            RJS_NEW(rt, parent);

            rjs_value_copy(rt, &parent->module, modv);
            rjs_list_append(&rmod->async_parent_list, &parent->ln);

            mod->pending_async ++;
        }
#endif /*ENABLE_ASYNC*/
    }

#if ENABLE_ASYNC
    if (mod->pending_async || module_has_tla(rt, mod)) {
        assert(!mod->async_eval);

        mod->async_eval = RJS_TRUE;

        if (mod->pending_async == 0)
            execute_async_module(rt, modv);
    } else
#endif /*ENABLE_ASYNC*/
    {
        if ((r = execute_module(rt, modv, NULL)) == RJS_ERR)
            return r;
    }

    if (mod->dfs_index == mod->dfs_ancestor_index) {
        RJS_Module *rmod, *nmod;

        rjs_list_foreach_safe_c(stack, rmod, nmod, RJS_Module, ln) {
            rjs_list_remove(&rmod->ln);

#if ENABLE_ASYNC
            rjs_value_copy(rt, &rmod->cycle_root, modv);

            if (rmod->async_eval)
                rmod->status = RJS_MODULE_STATUS_EVALUATING_ASYNC;
            else
#endif /*ENABLE_ASYNC*/
                rmod->status = RJS_MODULE_STATUS_EVALUATED;

            if (rmod == mod)
                break;
        }
    }

    return index;
}

/**
 * Evaluate the module.
 * \param rt The current runtime.
 * \param modv The module.
 * \param[out] promise Return the evaluation promise.
 * If promise is NULL, the function will waiting until the module evaluated.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_evaluate (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *promise)
{
    RJS_Module *mod;
    RJS_Script *script;
    RJS_List    stack;
    RJS_Result  r;
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *err = rjs_value_stack_push(rt);

    assert(rjs_value_is_module(rt, modv));

    mod    = rjs_value_get_gc_thing(rt, modv);
    script = &mod->script;

    assert((mod->status == RJS_MODULE_STATUS_LINKED)
            || (mod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC)
            || (mod->status == RJS_MODULE_STATUS_EVALUATED));

#if ENABLE_ASYNC
    if ((mod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC)
            || (mod->status == RJS_MODULE_STATUS_EVALUATED)) {
        mod  = rjs_value_get_gc_thing(rt, &mod->cycle_root);
        modv = &mod->cycle_root;
    }
#endif /*ENABLE_ASYNC*/

    if (rjs_value_is_undefined(rt, mod->top_level_capability.promise)) {
        rjs_list_init(&stack);

        rjs_new_promise_capability(rt, rjs_o_Promise(script->realm), &mod->top_level_capability);

        r = inner_module_evaluation(rt, modv, &stack, 0);
        if (r == RJS_ERR) {
            RJS_Module *tmod;

            rjs_catch(rt, err);

            rjs_list_foreach_c(&stack, tmod, RJS_Module, ln) {
                assert(tmod->status == RJS_MODULE_STATUS_EVALUATING);

                tmod->status      = RJS_MODULE_STATUS_EVALUATED;
                tmod->eval_result = r;

                rjs_value_copy(rt, &tmod->eval_error, err);
#if ENABLE_ASYNC
                rjs_value_set_gc_thing(rt, &tmod->cycle_root, tmod);
#endif /*ENABLE_ASYNC*/
            }

            rjs_call(rt, mod->top_level_capability.reject, rjs_v_undefined(rt), err, 1, NULL);
        } else {
#if ENABLE_ASYNC
            if (!mod->async_eval)
#endif /*ENABLE_ASYNC*/
            {
                mod->status = RJS_MODULE_STATUS_EVALUATED;

                rjs_call(rt, mod->top_level_capability.resolve, rjs_v_undefined(rt), rjs_v_undefined(rt), 1, NULL);
            }

            assert(rjs_list_is_empty(&stack));
        }
    }

    if (promise)
        rjs_value_copy(rt, promise, mod->top_level_capability.promise);

    r = RJS_OK;
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Disassemble an export entry.*/
static void
export_disassemble (RJS_Runtime *rt, RJS_Module *mod, RJS_ExportEntry *ee, FILE *fp)
{
    RJS_Script *script = &mod->script;

    fprintf(fp, "  ");

    if (ee->import_name_idx != RJS_INVALID_VALUE_INDEX) {
        rjs_script_print_value(rt, script, fp, ee->import_name_idx);
    } else if (ee->local_name_idx != RJS_INVALID_VALUE_INDEX) {
        rjs_script_print_value(rt, script, fp, ee->local_name_idx);
    }

    if (ee->export_name_idx != RJS_INVALID_VALUE_INDEX) {
        fprintf(fp, " as ");
        rjs_script_print_value(rt, script, fp, ee->export_name_idx);
    }

    if (ee->module_request_idx != RJS_INVALID_MODULE_REQUEST_INDEX) {
        RJS_ModuleRequest *mr = &mod->module_requests[ee->module_request_idx];

        fprintf(fp, " from ");
        rjs_script_print_value(rt, script, fp, mr->module_name_idx);
    }

    fprintf(fp, "\n");
}

/**
 * Disassemble the module.
 * \param rt The current runtime.
 * \param v The module value.
 * \param fp The output file.
 * \param flags The disassemble flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_disassemble (RJS_Runtime *rt, RJS_Value *v, FILE *fp, int flags)
{
    RJS_Module *mod;
    RJS_Script *script;
    size_t      i;

    assert(rjs_value_is_module(rt, v));
    assert(fp);

    mod    = rjs_value_get_gc_thing(rt, v);
    script = &mod->script;

    if ((flags & RJS_DISASSEMBLE_IMPORT) && mod->import_entry_num) {
        fprintf(fp, "import entries:\n");

        for (i = 0; i < mod->import_entry_num; i ++) {
            RJS_ImportEntry  *ie;
            RJS_ModuleRequest *mr;

            ie = &mod->import_entries[i];

            fprintf(fp, "  ");

            if (ie->import_name_idx != RJS_INVALID_VALUE_INDEX) {
                rjs_script_print_value(rt, script, fp, ie->import_name_idx);
            }

            if (ie->local_name_idx != RJS_INVALID_VALUE_INDEX) {
                fprintf(fp, " as ");
                rjs_script_print_value(rt, script, fp, ie->local_name_idx);
            }

            if ((ie->import_name_idx != RJS_INVALID_VALUE_INDEX)
                    || (ie->local_name_idx != RJS_INVALID_VALUE_INDEX))
                fprintf(fp, " from ");

            mr = &mod->module_requests[ie->module_request_idx];
            rjs_script_print_value(rt, script, fp, mr->module_name_idx);

            fprintf(fp, "\n");
        }
    }

    if (flags & RJS_DISASSEMBLE_EXPORT) {
        RJS_ExportEntry *ee;

        i = 0;

        if (mod->local_export_entry_num) {
            fprintf(fp, "local export entries:\n");
            for (; i < mod->local_export_entry_num; i ++) {
                ee = &mod->export_entries[i];

                export_disassemble(rt, mod, ee, fp);
            }
        }

        if (mod->indir_export_entry_num) {
            fprintf(fp, "indirect export entries:\n");
            for (; i < mod->indir_export_entry_num; i ++) {
                ee = &mod->export_entries[i];

                export_disassemble(rt, mod, ee, fp);
            }
        }

        if (mod->star_export_entry_num) {
            fprintf(fp, "star export entries:\n");
            for (; i < mod->star_export_entry_num; i ++) {
                ee = &mod->export_entries[i];

                export_disassemble(rt, mod, ee, fp);
            }
        }
    }

    fprintf(fp, "module declaration: %d var group: %d lex group: %d function group: %d\n",
            script->mod_decl_idx,
            script->mod_var_grp_idx,
            script->mod_lex_grp_idx,
            script->mod_func_grp_idx);

    return rjs_script_disassemble(rt, v, fp, flags);
}

/*Load the script module.*/
static RJS_Result
load_script_module (RJS_Runtime *rt, RJS_Value *mod, const char *path, RJS_Realm *realm)
{
    RJS_Input  fi;
    RJS_Result r;

    /*Load the module.*/
    if ((r = rjs_file_input_init(rt, &fi, path, NULL)) == RJS_ERR)
        return r;

    fi.flags |= RJS_INPUT_FL_CRLF_TO_LF;

    r = rjs_parse_module(rt, &fi, realm, mod);

    rjs_input_deinit(rt, &fi);

    return r;
}

#if ENABLE_JSON

/*Load the JSON module.*/
static RJS_Result
load_json_module (RJS_Runtime *rt, RJS_Value *mod, const char *path, RJS_Realm *realm)
{
    RJS_Result  r;
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *json = rjs_value_stack_push(rt);

    /*Load the module.*/
    if ((r = rjs_json_from_file(rt, json, path, NULL)) == RJS_OK) {
        RJS_Module      *m;
        RJS_Script      *s;
        RJS_Value       *v;
        RJS_ExportEntry *ee;
        RJS_BindingName  bn;
        RJS_String      *key;

        rjs_module_new(rt, mod, realm);

        m = rjs_value_get_gc_thing(rt, mod);
        s = &m->script;

        /*Add export entries.*/
        s->value_num              = 2;
        m->local_export_entry_num = 1;

        RJS_NEW_N(rt, s->value_table, s->value_num);
        RJS_NEW_N(rt, m->export_entries, m->local_export_entry_num);

        v = s->value_table;
        rjs_value_copy(rt, v, rjs_s_default(rt));

        v ++;
        rjs_value_copy(rt, v, rjs_s_star_default_star(rt));

        ee = m->export_entries;

        ee->export_name_idx    = 0;
        ee->local_name_idx     = 1;
        ee->module_request_idx = RJS_INVALID_MODULE_REQUEST_INDEX;
        ee->import_name_idx    = RJS_INVALID_VALUE_INDEX;

        key = rjs_value_get_string(rt, s->value_table);

        rjs_hash_insert(&m->export_hash, key, &ee->he, NULL, &rjs_hash_size_ops, rt);

        /*Create the module environment.*/
        rjs_module_link(rt, mod);

        /*Store JSON as *default* binding.*/
        rjs_binding_name_init(rt, &bn, rjs_s_star_default_star(rt));
        rjs_env_create_immutable_binding(rt, m->env, &bn, RJS_TRUE);
        rjs_env_initialize_binding(rt, m->env, &bn, json);

        rjs_binding_name_deinit(rt, &bn);
    }

    rjs_value_stack_restore(rt, top);

    return r;
}

#endif /*ENABLE_JSON*/

/*Load the native module.*/
static RJS_Result
load_native_module (RJS_Runtime *rt, RJS_Value *mod, const char *path, RJS_Realm *realm)
{
    RJS_Module        *m;
    void              *handle;
    RJS_ModuleInitFunc init;
    RJS_Result         r;

    if (!(handle = dlopen(path, RTLD_LAZY))) {
        RJS_LOGE("cannot open native module \"%s\"", path);
        return RJS_ERR;
    }

    init = dlsym(handle, "ratjs_module_init");
    if (!init) {
        RJS_LOGE("cannot find symbol \"ratjs_module_init\" in the \"%s\"", path);
        dlclose(handle);
        return RJS_ERR;
    }

    rjs_module_new(rt, mod, realm);

    m = rjs_value_get_gc_thing(rt, mod);

    if ((r = init(rt, mod)) == RJS_ERR) {
        RJS_LOGE("initialize native module \"%s\" failed", path);
        dlclose(handle);
        return RJS_ERR;
    }

    m->native_handle = handle;
    return RJS_OK;
}

/*Create the module from the filename.*/
static RJS_Result
module_from_file (RJS_Runtime *rt, RJS_Value *mod, const char *path, RJS_Realm *realm)
{
    char           rpath_buf[PATH_MAX];
    char          *rpath, *sub;
    RJS_HashEntry *he, **phe;
    RJS_Module    *m;
    RJS_Script    *script;
    RJS_Result     r;

    /*Get the full path.*/
    rpath = realpath(path, rpath_buf);
    if (!rpath)
        return RJS_ERR;

    /*Check if the module is already loaded.*/
    r = rjs_hash_lookup(&rt->mod_hash, rpath, &he, &phe, &rjs_hash_char_star_ops, rt);
    if (r) {
        m = RJS_CONTAINER_OF(he, RJS_Module, he);
        rjs_value_set_gc_thing(rt, mod, m);
        return RJS_OK;
    }

    sub = strrchr(rpath, '.');

    /*Load the module.*/
#if ENABLE_JSON
    if (sub && !strcasecmp(sub, ".json")) {
        r = load_json_module(rt, mod, rpath, realm);
    } else
#endif /*ENABLE_JSON*/
    if (sub && !strcasecmp(sub, ".njs")) {
        r = load_native_module(rt, mod, rpath, realm);
    } else {
        r = load_script_module(rt, mod, rpath, realm);
    }

    if (r == RJS_ERR)
        return r;

    /*Add the module to the hash table.*/
    m      = rjs_value_get_gc_thing(rt, mod);
    script = &m->script;

    script->path = rjs_char_star_dup(rt, rpath);

    rjs_hash_insert(&rt->mod_hash, script->path, &m->he, phe, &rjs_hash_char_star_ops, rt);

    return RJS_OK;
}

/*Resolve the imported module.*/
static RJS_Result
resolve_imported_module (RJS_Runtime *rt, RJS_Value *script, RJS_Value *name, RJS_Value *rmod)
{
    const char *nstr, *bstr;
    char        path[PATH_MAX];
    RJS_Result  r = RJS_FALSE;

    if (script) {
        RJS_Script *base;

        base = rjs_value_get_gc_thing(rt, script);
        base = base->base_script;
        bstr = base->path;
    } else {
        bstr = NULL;
    }

    nstr = rjs_string_to_enc_chars(rt, name, NULL, NULL);

    if (rt->mod_path_func) {
        if ((r = rt->mod_path_func(rt, bstr, nstr, path, sizeof(path))) == RJS_OK) {
            RJS_Realm  *realm = rjs_realm_current(rt);

            if ((r = module_from_file(rt, rmod, path, realm)) == RJS_ERR)
                rjs_throw_syntax_error(rt, _("load module \"%s\" failed"), path);
        }
    }

    return r;
}

/**Resolve binding entry.*/
typedef struct {
    RJS_List  ln;     /**< List node data.*/
    RJS_Value module; /**< The module.*/
    RJS_Value name;   /**< The export name.*/
} ResolveBindingEntry;

/**Resolve binding list.*/
typedef struct {
    RJS_GcThing gc_thing; /**< Base GC thing's data.*/
    RJS_List    entries;  /**< Entries list.*/
} ResolveBindingList;

/*Scan referenced things in the resolve binding list.*/
static void
resolve_binding_list_op_gc_scan (RJS_Runtime *rt, void *p)
{
    ResolveBindingList  *rbl = p;
    ResolveBindingEntry *rbe;

    rjs_list_foreach_c(&rbl->entries, rbe, ResolveBindingEntry, ln) {
        rjs_gc_scan_value(rt, &rbe->module);
        rjs_gc_scan_value(rt, &rbe->name);
    }
}

/*Free the resolve binding list.*/
static void
resolve_binding_list_op_gc_free (RJS_Runtime *rt, void *p)
{
    ResolveBindingList  *rbl = p;
    ResolveBindingEntry *rbe, *nrbe;

    rjs_list_foreach_safe_c(&rbl->entries, rbe, nrbe, ResolveBindingEntry, ln) {
        RJS_DEL(rt, rbe);
    }

    RJS_DEL(rt, rbl);
}

/**Resolve binding list's operation functions.*/
static const RJS_GcThingOps
resolve_binding_list_ops = {
    RJS_GC_THING_RESOLVE_BINDING_LIST,
    resolve_binding_list_op_gc_scan,
    resolve_binding_list_op_gc_free
};

/*Resovle the export.*/
static RJS_Result
resolve_export (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *name, ResolveBindingList *rb_set, RJS_ResolveBinding *rb)
{
    ResolveBindingEntry *rbe;
    RJS_ResolveBinding   star_rb;
    RJS_HashEntry       *he;
    RJS_Result           r;
    RJS_Module          *m;
    RJS_Script          *script;
    RJS_String          *str;
    RJS_ExportEntry     *ee;
    size_t               i;
    RJS_Bool             resolved;

    rjs_list_foreach_c(&rb_set->entries, rbe, ResolveBindingEntry, ln) {
        if (same_module(rt, mod, &rbe->module) && rjs_same_value(rt, name, &rbe->name))
            return RJS_FALSE;
    }

    rjs_resolve_binding_init(rt, &star_rb);

    RJS_NEW(rt, rbe);
    rjs_value_copy(rt, &rbe->module, mod);
    rjs_value_copy(rt, &rbe->name, name);
    rjs_list_append(&rb_set->entries, &rbe->ln);

    m      = rjs_value_get_gc_thing(rt, mod);
    script = &m->script;

    rjs_string_to_property_key(rt, name);
    str = rjs_value_get_string(rt, name);

    /*Lookup local and indirect export entry.*/
    r = rjs_hash_lookup(&m->export_hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    if (r) {
        ee = RJS_CONTAINER_OF(he, RJS_ExportEntry, he);

        if (ee->module_request_idx == RJS_INVALID_MODULE_REQUEST_INDEX) {
            rjs_value_copy(rt, rb->module, mod);
            rjs_value_copy(rt, rb->name, &script->value_table[ee->local_name_idx]);

            r = RJS_TRUE;
        } else {
            RJS_ModuleRequest *mr = &m->module_requests[ee->module_request_idx];

            if (rjs_value_is_undefined(rt, &mr->module)) {
                RJS_Value *name = &script->value_table[mr->module_name_idx];

                if ((r = rjs_resolve_imported_module(rt, mod, name, &mr->module)) == RJS_ERR)
                    goto end;
            }

            if (ee->import_name_idx == RJS_INVALID_VALUE_INDEX) {
                rjs_value_copy(rt, rb->module, &mr->module);
                rjs_value_set_undefined(rt, rb->name);

                r = RJS_TRUE;
            } else {
                r = resolve_export(rt, &mr->module, &script->value_table[ee->import_name_idx], rb_set, rb);
            }
        }

        goto end;
    }

    /*"default" is not defined.*/
    if (rjs_same_value(rt, name, rjs_s_default(rt))) {
        r = RJS_FALSE;
        goto end;
    }

    /*star export entries.*/
    ee       = m->export_entries + m->local_export_entry_num + m->indir_export_entry_num;
    resolved = RJS_FALSE;

    for (i = 0; i < m->star_export_entry_num; i ++, ee ++) {
        RJS_ModuleRequest *mr = &m->module_requests[ee->module_request_idx];

        if (rjs_value_is_undefined(rt, &mr->module)) {
            RJS_Value *name = &script->value_table[mr->module_name_idx];

            if ((r = rjs_resolve_imported_module(rt, mod, name, &mr->module)) == RJS_ERR)
                goto end;
        }

        if ((r = resolve_export(rt, &mr->module, name, rb_set, &star_rb)) == RJS_ERR)
            goto end;

        if (r) {
            if (resolved) {
                if (!same_module(rt, star_rb.module, rb->module)
                        || !rjs_same_value(rt, star_rb.name, rb->name)) {
                    r = RJS_AMBIGUOUS;
                    goto end;
                }
            } else {
                rjs_value_copy(rt, rb->module, star_rb.module);
                rjs_value_copy(rt, rb->name, star_rb.name);
                resolved = RJS_TRUE;
            }
        }
    }

    r = resolved;
end:
    rjs_resolve_binding_deinit(rt, &star_rb);
    return r;
}

/**
 * Resolve the export of the module.
 * \param rt The current runtime.
 * \param mod The module value.
 * \param name The export name.
 * \param[out] rb Return the resolve binding record.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot find the export.
 * \retval RJS_AMBIGUOUS Find ambiguous export entries.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_resolve_export (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *name, RJS_ResolveBinding *rb)
{
    ResolveBindingList *rbl;
    RJS_Result          r;
    size_t              top = rjs_value_stack_save(rt);
    RJS_Value          *tmp = rjs_value_stack_push(rt);

    assert(rjs_value_is_module(rt, mod));

    RJS_NEW(rt, rbl);
    rjs_list_init(&rbl->entries);
    rjs_value_set_gc_thing(rt, tmp, rbl);
    rjs_gc_add(rt, rbl, &resolve_binding_list_ops);

    r = resolve_export(rt, mod, name, rbl, rb);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Get the module's namespace object.
 * \param rt The current runtime.
 * \param mod The module value.
 * \param[out] ns Return the namespace object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_get_namespace (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *ns)
{
    RJS_Module *m;
    RJS_Result  r;

    assert(rjs_value_is_module(rt, mod));
    
    m = rjs_value_get_gc_thing(rt, mod);

    if (rjs_value_is_undefined(rt, &m->namespace)) {
        if ((r = rjs_module_ns_object_new(rt, &m->namespace, mod)) == RJS_ERR)
            return r;
    }

    rjs_value_copy(rt, ns, &m->namespace);
    return RJS_OK;
}

/**
 * Get the module environment.
 * \param rt The current runtime.
 * \param modv The module value.
 * \return The module's environment.
 */
RJS_Environment*
rjs_module_get_env (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module *mod = rjs_value_get_gc_thing(rt, modv);

    return mod->env;
}

/**
 * Resolve the imported module.
 * \param rt The current runtime.
 * \param script The base script.
 * \param name The name of the imported module.
 * \param[out] imod Return the imported module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_resolve_imported_module (RJS_Runtime *rt, RJS_Value *script, RJS_Value *name, RJS_Value *imod)
{
    RJS_Result r;

    r = resolve_imported_module(rt, script, name, imod);
    if (r == RJS_FALSE) {
        const char *nstr = rjs_string_to_enc_chars(rt, name, NULL, NULL);

        RJS_LOGE("cannot find module \"%s\"", nstr);
        r = rjs_throw_reference_error(rt, _("cannot find module \"%s\""), nstr);
    }

    return r;
}

/**
 * Create a module from the file.
 * \param rt The current runtime.
 * \param[out] mod Return the new module.
 * \param filename The module's filename.
 * \param realm The realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_from_file (RJS_Runtime *rt, RJS_Value *mod, const char *filename, RJS_Realm *realm)
{
    if (!realm)
        realm = rjs_realm_current(rt);
        
    return module_from_file(rt, mod, filename, realm);
}

/*Module dynamic import function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;       /**< Base built-in function object.*/
    RJS_PromiseCapability pc;        /**< Dynamic import promise capability.*/
    RJS_Value             mod;       /**< The module.*/
    RJS_Value             promisev;  /**< Promise value of the pc.*/
    RJS_Value             resolvev;  /**< Resovle value of the pc.*/
    RJS_Value             rejectv;   /**< Reject value of the pc.*/
} RJS_ModuleDynamicFunc;

/*Scan reference things in the module dynamic import function.*/
static void
module_dynamic_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleDynamicFunc *mdf = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, &mdf->bfo);

    rjs_gc_scan_value(rt, &mdf->mod);
    rjs_gc_scan_value(rt, &mdf->promisev);
    rjs_gc_scan_value(rt, &mdf->resolvev);
    rjs_gc_scan_value(rt, &mdf->rejectv);
}

/*Free the module dynamic import function.*/
static void
module_dynamic_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleDynamicFunc *mdf = ptr;

    rjs_builtin_func_object_deinit(rt, &mdf->bfo);

    RJS_DEL(rt, mdf);
}

/*Module dynamic import function operation functions.*/
static const RJS_ObjectOps
module_dynamic_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        module_dynamic_func_op_gc_scan,
        module_dynamic_func_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/*Module dynamic imported resolve function.*/
static RJS_NF(module_dynamic_resolve)
{
    RJS_ModuleDynamicFunc *mdf = (RJS_ModuleDynamicFunc*)rjs_value_get_object(rt, f);
    size_t                 top = rjs_value_stack_save(rt);
    RJS_Value             *ns  = rjs_value_stack_push(rt);
    RJS_Value             *err = rjs_value_stack_push(rt);
    RJS_Result             r;

    if ((r = rjs_module_get_namespace(rt, &mdf->mod, ns)) == RJS_ERR) {
        rjs_catch(rt, err);
        rjs_call(rt, mdf->pc.reject, rjs_v_undefined(rt), err, 1, NULL);
    } else {
        rjs_call(rt, mdf->pc.resolve, rjs_v_undefined(rt), ns, 1, NULL);

        r = RJS_OK;
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Module dynamic imported reject function.*/
static RJS_NF(module_dynamic_reject)
{
    RJS_ModuleDynamicFunc *mdf = (RJS_ModuleDynamicFunc*)rjs_value_get_object(rt, f);
    RJS_Value             *v   = rjs_argument_get(rt, args, argc, 0);

    rjs_call(rt, mdf->pc.reject, rjs_v_undefined(rt), v, 1, NULL);

    return RJS_OK;
}

/*Import the module dynamically.*/
static RJS_Result
module_import_dynamically (RJS_Runtime *rt, RJS_Value *ref, RJS_Value *spec, RJS_PromiseCapability *pc)
{
    RJS_Result             r;
    RJS_ModuleDynamicFunc *resolvef;
    RJS_ModuleDynamicFunc *rejectf;
    RJS_Module            *m;
    RJS_Script            *script1, *script2;
    size_t                 top     = rjs_value_stack_save(rt);
    RJS_Value             *str     = rjs_value_stack_push(rt);
    RJS_Value             *mod     = rjs_value_stack_push(rt);
    RJS_Value             *err     = rjs_value_stack_push(rt);
    RJS_Value             *p       = rjs_value_stack_push(rt);
    RJS_Value             *resolve = rjs_value_stack_push(rt);
    RJS_Value             *reject  = rjs_value_stack_push(rt);
    RJS_Value             *ns      = rjs_value_stack_push(rt);

    if ((r = rjs_to_string(rt, spec, str)) == RJS_ERR)
        goto end;

    if ((r = resolve_imported_module(rt, ref, str, mod)) == RJS_ERR)
        goto end;
    if (!r) {
        r = rjs_throw_type_error(rt, _("cannot resolve the module \"%s\""),
                rjs_string_to_enc_chars(rt, str, NULL, NULL));
        goto end;
    }

    m       = rjs_value_get_gc_thing(rt, mod);
    script1 = rjs_value_get_gc_thing(rt, ref);
    script2 = &m->script;

    if (script1 != script2) {
        /*Link and evaluate the module.*/
        if ((r = rjs_module_link(rt, mod)) == RJS_ERR)
            goto end;

        if ((r = rjs_module_evaluate(rt, mod, p)) == RJS_ERR)
            goto end;

        RJS_NEW(rt, resolvef);
        rjs_value_copy(rt, &resolvef->mod, mod);
        rjs_value_set_undefined(rt, &resolvef->promisev);
        rjs_value_set_undefined(rt, &resolvef->resolvev);
        rjs_value_set_undefined(rt, &resolvef->rejectv);
        rjs_promise_capability_init_vp(rt, &resolvef->pc, &resolvef->promisev,
                &resolvef->resolvev, &resolvef->rejectv);
        rjs_promise_capability_copy(rt, &resolvef->pc, pc);
        rjs_builtin_func_object_init(rt, resolve, &resolvef->bfo, NULL, NULL,
                NULL, module_dynamic_resolve, 0, &module_dynamic_func_ops);

        RJS_NEW(rt, rejectf);
        rjs_value_copy(rt, &rejectf->mod, mod);
        rjs_value_set_undefined(rt, &rejectf->promisev);
        rjs_value_set_undefined(rt, &rejectf->resolvev);
        rjs_value_set_undefined(rt, &rejectf->rejectv);
        rjs_promise_capability_init_vp(rt, &rejectf->pc, &rejectf->promisev,
                &rejectf->resolvev, &rejectf->rejectv);
        rjs_promise_capability_copy(rt, &rejectf->pc, pc);
        rjs_builtin_func_object_init(rt, reject, &rejectf->bfo, NULL, NULL,
                NULL, module_dynamic_reject, 0, &module_dynamic_func_ops);

        if ((r = rjs_invoke(rt, p, rjs_pn_then(rt), resolve, 2, NULL)) == RJS_ERR)
            goto end;
    } else {
        /*Imported module is just the referenced module.*/
        if ((r = rjs_module_get_namespace(rt, mod, ns)) == RJS_ERR)
            goto end;

        rjs_call(rt, pc->resolve, rjs_v_undefined(rt), ns, 1, NULL);
    }

    r = RJS_OK;
end:
    if (r == RJS_ERR) {
        rjs_catch(rt, err);
        rjs_call(rt, pc->reject, rjs_v_undefined(rt), err, 1, NULL);
        r = RJS_OK;
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Import the module dynamically.
 * \param rt The current runtime.
 * \param scriptv The base script.
 * \param name The name of the module.
 * \param[out] Return the promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_import_dynamically (RJS_Runtime *rt, RJS_Value *scriptv, RJS_Value *name, RJS_Value *promise)
{
    RJS_PromiseCapability  pc;
    RJS_Result             r;
    RJS_Script            *script = rjs_value_get_gc_thing(rt, scriptv);
    size_t                 top    = rjs_value_stack_save(rt);

    rjs_promise_capability_init(rt, &pc);

    rjs_new_promise_capability(rt, rjs_o_Promise(script->realm), &pc);

    if ((r = module_import_dynamically(rt, scriptv, name, &pc)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, promise, pc.promise);
    r = RJS_OK;
end:
    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Load the module's import meta object.
 * \param rt The current runtime.
 * \param modv The module value.
 * \param[out] v Return the import meta object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_import_meta (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *v)
{
    RJS_Module *mod;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    if (rjs_value_is_undefined(rt, &mod->import_meta)) {
        rjs_ordinary_object_create(rt, NULL, &mod->import_meta);
    }

    rjs_value_copy(rt, v, &mod->import_meta);
    return RJS_OK;
}

/**
 * Load all the export values of the module to the object.
 * \param rt The current runtime.
 * \param modv The module value.
 * \param o The object to store the export values.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_load_exports (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *o)
{
    RJS_Module      *mod;
    RJS_ExportEntry *ee;
    size_t           i;
    RJS_Result       r;
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *key = rjs_value_stack_push(rt);
    RJS_Value       *ev  = rjs_value_stack_push(rt);

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    assert(mod->status == RJS_MODULE_STATUS_EVALUATED);

    rjs_hash_foreach_c(&mod->export_hash, i, ee, RJS_ExportEntry, he) {
        RJS_BindingName  bn;
        RJS_PropertyName pn;

        rjs_value_set_string(rt, key, ee->he.key);

        rjs_binding_name_init(rt, &bn, key);
        r = rjs_env_get_binding_value(rt, mod->env, &bn, RJS_TRUE, ev);
        rjs_binding_name_deinit(rt, &bn);
        if (r == RJS_ERR)
            goto end;

        rjs_property_name_init(rt, &pn, key);
        r = rjs_create_data_property_or_throw(rt, o, &pn, ev);
        rjs_property_name_deinit(rt, &pn);
        if (r == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the module's native data.
 * \param rt The current runtime.
 * \param modv The module value.
 * \param data The data's pointer.
 * \param scan The function scan the referenced things in the data.
 * \param free The function free the data.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_set_data (RJS_Runtime *rt, RJS_Value *modv, void *data,
        RJS_ScanFunc scan, RJS_FreeFunc free)
{
    RJS_Module *mod;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    rjs_native_data_free(rt, &mod->native_data);
    rjs_native_data_set(&mod->native_data, data, scan, free);

    return RJS_OK;
}

/**
 * Get the native data's pointer.
 * \param rt The current runtime.
 * \param modv The module value.
 * \retval The native data's pointer.
 */
void*
rjs_module_get_data (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module *mod;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    return mod->native_data.data;
}

/**Module value entry.*/
typedef struct {
    RJS_HashEntry he; /**< Hash entry data.*/
    RJS_Value    *v;  /**< The value.*/
    size_t        id; /**< The value's index.*/
} ModuleValueEntry;

/*Add a value to the module value hash table.*/
static void
module_value_add (RJS_Runtime *rt, RJS_Hash *hash, const char *name)
{
    RJS_Value     *v = rjs_value_stack_push(rt);
    RJS_String    *str;
    RJS_HashEntry *he, **phe;
    RJS_Result     r;

    rjs_string_from_chars(rt, v, name, -1);
    rjs_string_to_property_key(rt, v);

    str = rjs_value_get_string(rt, v);
    r   = rjs_hash_lookup(hash, str, &he, &phe, &rjs_hash_size_ops, rt);
    if (!r) {
        ModuleValueEntry *mve;

        RJS_NEW(rt, mve);

        mve->id = hash->entry_num;
        mve->v  = v;

        rjs_hash_insert(hash, str, &mve->he, phe, &rjs_hash_size_ops, rt);
    }
}

/*Get the module value's index.*/
static int
module_value_get_v (RJS_Runtime *rt, RJS_Hash *hash, RJS_Value *v)
{
    RJS_String       *str;
    RJS_HashEntry    *he;
    RJS_Result        r;
    ModuleValueEntry *mve;

    str = rjs_value_get_string(rt, v);
    r   = rjs_hash_lookup(hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    assert(r == RJS_OK);

    mve = RJS_CONTAINER_OF(he, ModuleValueEntry, he);

    return mve->id;
}

/*Get the module value's index.*/
static int
module_value_get (RJS_Runtime *rt, RJS_Hash *hash, const char *name)
{
    size_t            top;
    RJS_Value        *v;
    RJS_String       *str;
    RJS_HashEntry    *he;
    RJS_Result        r;
    ModuleValueEntry *mve;

    if (!name)
        return RJS_INVALID_VALUE_INDEX;

    top = rjs_value_stack_save(rt);
    v   = rjs_value_stack_push(rt);

    rjs_string_from_chars(rt, v, name, -1);
    rjs_string_to_property_key(rt, v);

    str = rjs_value_get_string(rt, v);
    r   = rjs_hash_lookup(hash, str, &he, NULL, &rjs_hash_size_ops, rt);
    assert(r == RJS_OK);

    mve = RJS_CONTAINER_OF(he, ModuleValueEntry, he);

    rjs_value_stack_restore(rt, top);
    return mve->id;
}

/*Release the module value hash table.*/
static void
module_value_hash_deinit (RJS_Runtime *rt, RJS_Hash *hash)
{
    size_t            i;
    ModuleValueEntry *mve, *nmve;

    rjs_hash_foreach_safe_c(hash, i, mve, nmve, ModuleValueEntry, he) {
        RJS_DEL(rt, mve);
    }
    rjs_hash_deinit(hash, &rjs_hash_size_ops, rt);
}

/**
 * Set the module's import and export entries.
 * This function must be invoked in native module's "ratjs_module_init" function.
 * \param rt The current runtime.
 * \param modv The module.
 * \param imports The import entries.
 * \param local_exports The local export entries.
 * \param indir_exports The indirect export entries.
 * \param star_exports The star export entries.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_set_import_export (RJS_Runtime *rt, RJS_Value *modv,
        const RJS_ModuleImportDesc *imports,
        const RJS_ModuleExportDesc *local_exports,
        const RJS_ModuleExportDesc *indir_exports,
        const RJS_ModuleExportDesc *star_exports)
{
    size_t            import_num       = 0;
    size_t            local_export_num = 0;
    size_t            indir_export_num = 0;
    size_t            star_export_num  = 0;
    size_t            export_num       = 0;
    size_t            top              = rjs_value_stack_save(rt);
    RJS_Hash          value_hash, mod_hash;
    size_t            i;
    const RJS_ModuleImportDesc *ie;
    const RJS_ModuleExportDesc *ee;
    ModuleValueEntry *mve;
    RJS_Module       *mod;
    RJS_Script       *script;

    assert(rjs_value_is_module(rt, modv));

    rjs_hash_init(&value_hash);
    rjs_hash_init(&mod_hash);

    mod    = rjs_value_get_gc_thing(rt, modv);
    script = &mod->script;

    if (imports) {
        for (ie = imports; ie->import_name; ie ++) {
            import_num ++;
            module_value_add(rt, &mod_hash, ie->module_name);
            module_value_add(rt, &value_hash, ie->module_name);
            module_value_add(rt, &value_hash, ie->import_name);
            module_value_add(rt, &value_hash, ie->local_name);
        }
    }

    if (local_exports) {
        for (ee = local_exports; ee->export_name; ee ++) {
            local_export_num ++;
            module_value_add(rt, &value_hash, ee->local_name);
            module_value_add(rt, &value_hash, ee->export_name);
        }
    }

    if (indir_exports) {
        for (ee = indir_exports; ee->export_name; ee ++) {
            indir_export_num ++;
            module_value_add(rt, &mod_hash, ee->module_name);
            module_value_add(rt, &value_hash, ee->module_name);
            module_value_add(rt, &value_hash, ee->import_name);
            module_value_add(rt, &value_hash, ee->export_name);
        }
    }

    if (star_exports) {
        for (ee = star_exports; ee->module_name; ee ++) {
            star_export_num ++;
            module_value_add(rt, &mod_hash, ee->module_name);
            module_value_add(rt, &value_hash, ee->module_name);
        }
    }

    /*Create the value table.*/
    if (value_hash.entry_num) {
        script->value_num = value_hash.entry_num;
        RJS_NEW_N(rt, script->value_table, script->value_num);

        rjs_hash_foreach_c(&value_hash, i, mve, ModuleValueEntry, he) {
            rjs_value_copy(rt, &script->value_table[mve->id], mve->v);
        }
    }

    /*Create the module request table.*/
    if (mod_hash.entry_num) {
        mod->module_request_num = mod_hash.entry_num;
        RJS_NEW_N(rt, mod->module_requests, mod->module_request_num);

        rjs_hash_foreach_c(&mod_hash, i, mve, ModuleValueEntry, he) {
            RJS_ModuleRequest *mr = &mod->module_requests[mve->id];

            rjs_value_set_undefined(rt, &mr->module);
            mr->module_name_idx = module_value_get_v(rt, &value_hash, mve->v);
        }
    }

    /*Create the import entry table.*/
    if (import_num) {
        RJS_ImportEntry *mie;

        mod->import_entry_num = import_num;
        RJS_NEW_N(rt, mod->import_entries, import_num);

        for (mie = mod->import_entries, ie = imports;
                ie->import_name;
                mie ++, ie ++) {
            mie->module_request_idx = module_value_get(rt, &value_hash, ie->module_name);
            mie->import_name_idx    = module_value_get(rt, &value_hash, ie->import_name);
            mie->local_name_idx     = module_value_get(rt, &value_hash, ie->local_name);
        }
    }

    /*Create the export entry table.*/
    export_num = local_export_num + indir_export_num + star_export_num;
    if (export_num) {
        RJS_ExportEntry *mee;
        RJS_String      *key;

        mod->local_export_entry_num = local_export_num;
        mod->indir_export_entry_num = indir_export_num;
        mod->star_export_entry_num  = star_export_num;
        RJS_NEW_N(rt, mod->export_entries, export_num);

        mee = mod->export_entries;

        if (local_exports) {
            for (ee = local_exports; ee->export_name; ee ++) {
                mee->module_request_idx = RJS_INVALID_MODULE_REQUEST_INDEX;
                mee->import_name_idx    = RJS_INVALID_VALUE_INDEX;
                mee->local_name_idx     = module_value_get(rt, &value_hash, ee->local_name);
                mee->export_name_idx    = module_value_get(rt, &value_hash, ee->export_name);

                key = rjs_value_get_string(rt, &script->value_table[mee->export_name_idx]);
                rjs_hash_insert(&mod->export_hash, key, &mee->he, NULL, &rjs_hash_size_ops, rt);

                mee ++;
            }
        }

        if (indir_exports) {
            for (ee = indir_exports; ee->export_name; ee ++) {
                mee->module_request_idx = module_value_get(rt, &value_hash, ee->module_name);
                mee->import_name_idx    = module_value_get(rt, &value_hash, ee->import_name);
                mee->local_name_idx     = RJS_INVALID_VALUE_INDEX;
                mee->export_name_idx    = module_value_get(rt, &value_hash, ee->export_name);

                key = rjs_value_get_string(rt, &script->value_table[mee->export_name_idx]);
                rjs_hash_insert(&mod->export_hash, key, &mee->he, NULL, &rjs_hash_size_ops, rt);

                mee ++;
            }
        }

        if (star_exports) {
            for (ee = star_exports; ee->module_name; ee ++) {
                mee->module_request_idx = module_value_get(rt, &value_hash, ee->module_name);
                mee->import_name_idx    = RJS_INVALID_VALUE_INDEX;
                mee->local_name_idx     = RJS_INVALID_VALUE_INDEX;
                mee->export_name_idx    = RJS_INVALID_VALUE_INDEX;
                mee ++;
            }
        }
    }

    /*Free the hash tables.*/
    module_value_hash_deinit(rt, &value_hash);
    module_value_hash_deinit(rt, &mod_hash);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/**
 * Get the binding's value from the module's environment.
 * \param rt The current runtime.
 * \param modv The module.
 * \param name The name of the binding.
 * \param[out] v Return the binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_get_binding (RJS_Runtime *rt, RJS_Value *modv, const char *name, RJS_Value *v)
{
    size_t          top = rjs_value_stack_save(rt);
    RJS_Value      *nv  = rjs_value_stack_push(rt);
    RJS_BindingName bn;
    RJS_Module     *mod;
    RJS_Result      r;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    rjs_string_from_chars(rt, nv, name, -1);
    rjs_string_to_property_key(rt, nv);

    rjs_binding_name_init(rt, &bn, nv);
    r = rjs_env_get_binding_value(rt, mod->env, &bn, RJS_TRUE, v);
    rjs_binding_name_deinit(rt, &bn);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Add a module binding.
 * This function must be invoked in native module's "ratjs_module_exec" function.
 * \param rt The current runtime.
 * \param modv The module value.
 * \param name The name of the binding.
 * \param v The binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_add_binding (RJS_Runtime *rt, RJS_Value *modv, const char *name, RJS_Value *v)
{
    size_t          top = rjs_value_stack_save(rt);
    RJS_Value      *nv  = rjs_value_stack_push(rt);
    RJS_BindingName bn;
    RJS_Module     *mod;
    RJS_Result      r;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    rjs_string_from_chars(rt, nv, name, -1);
    rjs_string_to_property_key(rt, nv);

    rjs_binding_name_init(rt, &bn, nv);
    r = rjs_env_create_immutable_binding(rt, mod->env, &bn, RJS_TRUE);
    if (r == RJS_OK)
        r = rjs_env_initialize_binding(rt, mod->env, &bn, v);
    rjs_binding_name_deinit(rt, &bn);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Initialize the module data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_module_init (RJS_Runtime *rt)
{
    rjs_hash_init(&rt->mod_hash);
}

/**
 * Release the module data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_module_deinit (RJS_Runtime *rt)
{
    /*Clear the module hash table.*/
    rjs_hash_deinit(&rt->mod_hash, &rjs_hash_char_star_ops, rt);
}

/**
 * Scan the module data in the rt.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_module (RJS_Runtime *rt)
{
    RJS_Module *mod;
    size_t      i;

    rjs_hash_foreach_c(&rt->mod_hash, i, mod, RJS_Module, he) {
        rjs_gc_mark(rt, mod);
    }
}
