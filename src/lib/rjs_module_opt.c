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

/*Lookup the imported module.*/
static RJS_Result
lookup_module (RJS_Runtime *rt, RJS_Value *script, RJS_Value *name,
        RJS_Value *promise, RJS_Value *modv, RJS_Bool dynamic);

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

#if ENABLE_NATIVE_MODULE
    /*Unload the native module.*/
    if (mod->native_handle)
        dlclose(mod->native_handle);
#endif /*ENABLE_NATIVE_MODULE*/

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

/*Scan the referenced things in the async module function.*/
static void
async_module_scan (RJS_Runtime *rt, void *ptr)
{
    if (ptr)
        rjs_gc_mark(rt, ptr);
}

#endif /*ENABLE_ASYNC*/

/**
 * Create a new module.
 * \param rt The current runtime.
 * \param[out] v Return the module value.
 * \param id The identifier of the module.
 * \param realm The realm.
 * \return The new module.
 */
RJS_Module*
rjs_module_new (RJS_Runtime *rt, RJS_Value *v, const char *id, RJS_Realm *realm)
{
    RJS_Module    *mod;
    RJS_HashEntry *e, **pe = NULL;
    RJS_Result     r;

    if (id) {
        r = rjs_hash_lookup(&rt->mod_hash, (void*)id, &e, &pe, &rjs_hash_char_star_ops, rt);
        if (r) {
            mod = RJS_CONTAINER_OF(e, RJS_Module, he);
            rjs_value_set_gc_thing(rt, v, mod);
            return mod;
        }
    }

    RJS_NEW(rt, mod);

    rjs_script_init(rt, &mod->script, realm);

    mod->status                 = RJS_MODULE_STATUS_ALLOCATED;
    mod->dfs_index              = 0;
    mod->dfs_ancestor_index     = 0;
    mod->eval_result            = RJS_OK;
    mod->env                    = NULL;
    mod->module_requests        = NULL;
    mod->import_entries         = NULL;
    mod->export_entries         = NULL;
    mod->module_request_num     = 0;
    mod->import_entry_num       = 0;
    mod->local_export_entry_num = 0;
    mod->indir_export_entry_num = 0;
    mod->star_export_entry_num  = 0;

#if ENABLE_NATIVE_MODULE
    mod->native_handle          = NULL;
#endif /*ENABLE_NATIVE_MODULE*/

    rjs_native_data_init(&mod->native_data);

    rjs_value_set_undefined(rt, &mod->top_promise);
    rjs_value_set_undefined(rt, &mod->top_resolve);
    rjs_value_set_undefined(rt, &mod->top_reject);
    rjs_promise_capability_init_vp(rt, &mod->top_level_capability,
            &mod->top_promise, &mod->top_resolve, &mod->top_reject);

#if ENABLE_ASYNC
    mod->pending_async = 0;
    mod->async_eval    = 0;

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

    if (id) {
        RJS_Script *script = &mod->script;

        script->path = rjs_char_star_dup(rt, id);

        rjs_hash_insert(&rt->mod_hash, script->path, &mod->he, pe, &rjs_hash_char_star_ops, rt);
    }

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

        assert(!rjs_value_is_undefined(rt, &mr->module));

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

/**Module load requested module data.*/
typedef struct {
    int       ref;      /**< Reference counter.*/
    int       wait_cnt; /**< Waiting module's counter.*/
    RJS_Hash  mod_hash; /**< Module hash table.*/
    RJS_List  mod_list; /**< The module list.*/
    RJS_PromiseCapability pc; /**< Promise capability.*/
    RJS_Value promise;  /**< The proimise.*/
    RJS_Value resolve;  /**< The resolve function.*/
    RJS_Value reject;   /**< The reject function.*/
    RJS_Value module;   /**< The module.*/
    RJS_Value error;    /**< The error value.*/
} RJS_ModuleLoadRequestedData;

/*Scan the reference data in the module load requested data.*/
static void
module_load_req_data_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleLoadRequestedData *mlrd = ptr;

    rjs_gc_scan_value(rt, &mlrd->promise);
    rjs_gc_scan_value(rt, &mlrd->resolve);
    rjs_gc_scan_value(rt, &mlrd->reject);
    rjs_gc_scan_value(rt, &mlrd->module);
    rjs_gc_scan_value(rt, &mlrd->error);
}

/*Free the module load requested data.*/
static void
module_load_req_data_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleLoadRequestedData *mlrd = ptr;

    mlrd->ref --;

    if (mlrd->ref == 0) {
        RJS_HashEntry *e, *ne;
        size_t         i;

        rjs_hash_foreach_safe(&mlrd->mod_hash, i, e, ne) {
            RJS_DEL(rt, e);
        }

        rjs_hash_deinit(&mlrd->mod_hash, &rjs_hash_size_ops, rt);
        RJS_DEL(rt, mlrd);
    }
}

/*Check the module load requested result.*/
static RJS_Result
module_load_req_result (RJS_Runtime *rt, RJS_ModuleLoadRequestedData *mlrd)
{
    if (mlrd->wait_cnt == 0) {
        RJS_Bool failed = RJS_FALSE;

        while (!rjs_list_is_empty(&mlrd->mod_list)) {
            RJS_Module *m, *nm;
            RJS_Bool action = RJS_FALSE;

            rjs_list_foreach_safe_c(&mlrd->mod_list, m, nm, RJS_Module, ln) {
                RJS_Bool mod_failed  = RJS_FALSE;
                RJS_Bool mod_not_end = RJS_FALSE;
                size_t   i;

                for (i = 0; i < m->module_request_num; i ++) {
                    RJS_ModuleRequest *mr = &m->module_requests[i];

                    if (rjs_value_is_undefined(rt, &mr->module)) {
                        mod_failed = RJS_TRUE;
                    } else {
                        RJS_Module *rm = rjs_value_get_gc_thing(rt, &mr->module);

                        if (rm->status == RJS_MODULE_STATUS_LOADING_FAILED)
                            mod_failed = RJS_TRUE;
                        else if (rm->status == RJS_MODULE_STATUS_LOADING_REQUESTED)
                            mod_not_end = RJS_TRUE;
                    }

                    if (mod_failed)
                        break;
                }

                if (mod_failed) {
                    m->status = RJS_MODULE_STATUS_LOADING_FAILED;
                    failed = RJS_TRUE;
                    mod_not_end = RJS_FALSE;
                }

                if (!mod_not_end) {
                    m->status = RJS_MODULE_STATUS_UNLINKED;
                    rjs_value_set_undefined(rt, m->top_level_capability.promise);
                    rjs_list_remove(&m->ln);
                    action = RJS_TRUE;
                }
            }

            if (!action) {
                rjs_list_foreach_safe_c(&mlrd->mod_list, m, nm, RJS_Module, ln) {
                    m->status = RJS_MODULE_STATUS_UNLINKED;
                    rjs_value_set_undefined(rt, m->top_level_capability.promise);
                    rjs_list_remove(&m->ln);
                }
            }
        }

        if (failed) {
            rjs_call(rt, mlrd->pc.reject, rjs_v_undefined(rt), &mlrd->error, 1, NULL);
        } else {
            rjs_call(rt, mlrd->pc.resolve, rjs_v_undefined(rt), NULL, 0, NULL);
        }
    }

    return RJS_OK;
}

/*Add a module to the load requested data's hash table.*/
static RJS_Result
module_load_req_data_add (RJS_Runtime *rt, RJS_ModuleLoadRequestedData *mlrd, RJS_Value *modv);

/*Load all the requested moudles.*/
static RJS_Result
module_load_req_modules (RJS_Runtime *rt, RJS_ModuleLoadRequestedData *mlrd, RJS_Value *modv)
{
    RJS_Module *mod    = rjs_value_get_gc_thing(rt, modv);
    size_t      top    = rjs_value_stack_save(rt);
    RJS_Value  *p      = rjs_value_stack_push(rt);
    RJS_Script *script = &mod->script;
    size_t      i;
    RJS_Result  r;

    for (i = 0; i < mod->module_request_num; i ++) {
        RJS_ModuleRequest *mr   = &mod->module_requests[i];
        RJS_Value         *name = &script->value_table[mr->module_name_idx];

        if (rjs_value_is_undefined(rt, &mr->module)) {
            if ((r = lookup_module(rt, modv, name, p, &mr->module, RJS_FALSE)) == RJS_ERR) {
                rjs_catch(rt, &mlrd->error);
                goto end;
            }
        }

        if ((r = module_load_req_data_add(rt, mlrd, &mr->module)) == RJS_ERR) {
            rjs_catch(rt, &mlrd->error);
            goto end;
        }
    }
end:
    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*Load module ok.*/
static RJS_NF(module_load_ok)
{
    RJS_ModuleLoadRequestedData *mlrd = rjs_native_object_get_data(rt, f);
    RJS_Value                   *modv = rjs_argument_get(rt, args, argc, 0);
    RJS_Module                  *mod  = rjs_value_get_gc_thing(rt, modv);

    mlrd->wait_cnt --;

    if (mod->status == RJS_MODULE_STATUS_LOADED) {
        mod->status = RJS_MODULE_STATUS_LOADING_REQUESTED;
        module_load_req_modules(rt, mlrd, modv);
    }

    return module_load_req_result(rt, mlrd);
}

/*Load module error.*/
static RJS_NF(module_load_error)
{
    RJS_ModuleLoadRequestedData *mlrd = rjs_native_object_get_data(rt, f);
    RJS_Value                   *err  = rjs_argument_get(rt, args, argc, 0);

    mlrd->wait_cnt --;
    rjs_value_copy(rt, &mlrd->error, err);

    return module_load_req_result(rt, mlrd);
}

/*Add a module to the load requested data's hash table.*/
static RJS_Result
module_load_req_data_add (RJS_Runtime *rt, RJS_ModuleLoadRequestedData *mlrd, RJS_Value *modv)
{
    RJS_Realm     *realm   = rjs_realm_current(rt);
    RJS_Module    *mod     = rjs_value_get_gc_thing(rt, modv);
    size_t         top     = rjs_value_stack_save(rt);
    RJS_Value     *fulfill = rjs_value_stack_push(rt);
    RJS_Value     *reject  = rjs_value_stack_push(rt);
    RJS_Value     *rv      = rjs_value_stack_push(rt);
    RJS_HashEntry *e, **pe;
    RJS_Result     r;

    r = rjs_hash_lookup(&mlrd->mod_hash, mod, &e, &pe, &rjs_hash_size_ops, rt);
    if (r)
        goto end;

    RJS_NEW(rt, e);
    rjs_hash_insert(&mlrd->mod_hash, mod, e, pe, &rjs_hash_size_ops, rt);

    switch (mod->status) {
    case RJS_MODULE_STATUS_ALLOCATED:
        rjs_native_func_object_new(rt, fulfill, realm, NULL, NULL, module_load_ok, 0);
        rjs_native_object_set_data(rt, fulfill, NULL, mlrd, module_load_req_data_scan, module_load_req_data_free);
        mlrd->ref ++;
        rjs_native_func_object_new(rt, reject, realm, NULL, NULL, module_load_error, 0);
        rjs_native_object_set_data(rt, reject, NULL, mlrd, module_load_req_data_scan, module_load_req_data_free);
        mlrd->ref ++;
        rjs_promise_then(rt, mod->top_level_capability.promise, fulfill, reject, rv);
        mlrd->wait_cnt ++;
        break;
    case RJS_MODULE_STATUS_LOADED:
        mod->status = RJS_MODULE_STATUS_LOADING_REQUESTED;
    case RJS_MODULE_STATUS_LOADING_REQUESTED:
        module_load_req_modules(rt, mlrd, modv);
        rjs_list_append(&mlrd->mod_list, &mod->ln);
        break;
    case RJS_MODULE_STATUS_LOADING_FAILED:
        rjs_value_copy(rt, &mlrd->error, &mod->eval_error);
        break;
    default:
        break;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Allocate a new module load requested modules data.*/
static RJS_ModuleLoadRequestedData*
module_load_req_data_new (RJS_Runtime *rt, RJS_Value *modv, RJS_PromiseCapability *pc)
{
    RJS_ModuleLoadRequestedData *mlrd;

    RJS_NEW(rt, mlrd);

    mlrd->ref      = 1;
    mlrd->wait_cnt = 0;

    rjs_value_set_undefined(rt, &mlrd->promise);
    rjs_value_set_undefined(rt, &mlrd->resolve);
    rjs_value_set_undefined(rt, &mlrd->reject);
    rjs_value_set_undefined(rt, &mlrd->error);

    rjs_value_copy(rt, &mlrd->module, modv);

    rjs_hash_init(&mlrd->mod_hash);
    rjs_list_init(&mlrd->mod_list);

    rjs_promise_capability_init_vp(rt, &mlrd->pc, &mlrd->promise, &mlrd->resolve, &mlrd->reject);
    rjs_promise_capability_copy(rt, &mlrd->pc, pc);

    module_load_req_data_add(rt, mlrd, modv);

    return mlrd;
}

/**
 * Load the requested modules.
 * \param rt The current runtime.
 * \param modv The module.
 * \param[out] promise Return the promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_module_load_requested (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *promise)
{
    RJS_Module *mod;
    RJS_Result  r;
    RJS_ModuleLoadRequestedData *mlrd = NULL;
    size_t      top   = rjs_value_stack_save(rt);
    RJS_Realm  *realm = rjs_realm_current(rt);
    RJS_PromiseCapability pc;

    rjs_promise_capability_init(rt, &pc);

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    assert(mod->status == RJS_MODULE_STATUS_LOADED);

    rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc);

    mlrd = module_load_req_data_new(rt, modv, &pc);

    r = module_load_req_result(rt, mlrd);

    if (promise)
        rjs_value_copy(rt, promise, mlrd->pc.promise);

    if (mlrd)
        module_load_req_data_free(rt, mlrd);

    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Link the module.*/
static RJS_Result
inner_module_link (RJS_Runtime *rt, RJS_Value *modv, RJS_List *stack, int index)
{
    RJS_Module *mod = rjs_value_get_gc_thing(rt, modv);
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
        RJS_Module        *rmod;

        assert(!rjs_value_is_undefined(rt, &mr->module));

        if ((r = inner_module_link(rt, &mr->module, stack, index)) == RJS_ERR)
            return r;

        index = r;

        rmod = rjs_value_get_gc_thing(rt, &mr->module);
        if (rmod->status == RJS_MODULE_STATUS_LINKING)
            mod->dfs_ancestor_index = RJS_MIN(rmod->dfs_ancestor_index, mod->dfs_ancestor_index);
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
    }
#if ENABLE_NATIVE_MODULE
    else if (mod->native_handle) {
        RJS_ModuleExecFunc ef;

        ef = dlsym(mod->native_handle, "ratjs_module_exec");
        if (ef) {
            if ((r = ef(rt, modv)) == RJS_ERR)
                RJS_LOGE("native module execute failed");
        }
    }
#endif /*ENABLE_NATIVE_MODULE*/

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
    RJS_Module *mod    = rjs_value_get_gc_thing(rt, modv);
    RJS_Script *script = &mod->script;
    RJS_Result  r;

    r = rjs_create_native_function(rt, NULL, nf, 0, rjs_s_empty(rt), script->realm, NULL, NULL, f);
    if (r == RJS_ERR)
        return r;

    rjs_native_object_set_data(rt, f, NULL, mod, async_module_scan, NULL);
    return RJS_OK;
}

/*Sort the async module.*/
static RJS_CompareResult
async_module_sort (const void *p1, const void *p2, void *data)
{
    RJS_Module *m1 = *(RJS_Module**)p1;
    RJS_Module *m2 = *(RJS_Module**)p2;

    if (m1->async_eval > m2->async_eval)
        return RJS_COMPARE_GREATER;

    return RJS_COMPARE_LESS;
}

/*Module pointer vector.*/
typedef RJS_VECTOR_DECL(RJS_Module*) RJS_ModuleVector;

/*Gather the pending ancestor modules.*/
static RJS_Result
gather_available_ancestors (RJS_Runtime *rt, RJS_Module *mod, RJS_ModuleVector *vec, RJS_Hash *hash)
{
    RJS_ModuleAsyncParent *parent;

    rjs_list_foreach_c(&mod->async_parent_list, parent, RJS_ModuleAsyncParent, ln) {
        RJS_Module    *pmod;
        RJS_HashEntry *e, **pe;
        RJS_Result     r;

        pmod = rjs_value_get_gc_thing(rt, &parent->module);

        if (pmod->status == RJS_MODULE_STATUS_EVALUATED)
            continue;

        if (!rjs_value_is_undefined(rt, &pmod->cycle_root)) {
            RJS_Module *rmod = rjs_value_get_gc_thing(rt, &pmod->cycle_root);

            if (rmod->eval_result == RJS_ERR)
                continue;
        }

        r = rjs_hash_lookup(hash, pmod, &e, &pe, &rjs_hash_size_ops, rt);
        if (r)
            continue;

        RJS_NEW(rt, e);
        rjs_hash_insert(hash, pmod, e, pe, &rjs_hash_size_ops, rt);

        assert(pmod->status == RJS_MODULE_STATUS_EVALUATING_ASYNC);
        assert(pmod->eval_result != RJS_ERR);
        assert(pmod->async_eval);
        assert(pmod->pending_async > 0);

        pmod->pending_async --;

        if (pmod->pending_async == 0) {
            rjs_vector_append(vec, pmod, rt);

            if (!module_has_tla(rt, pmod))
                gather_available_ancestors(rt, pmod, vec, hash);
        }
    }

    return RJS_OK;
}

/*Execute the valid modules.*/
static void
execute_valid_modules (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module       *mod, **mod_ptr;
    RJS_ModuleVector  mod_vec;
    size_t            i;
    RJS_Hash          mod_hash;
    RJS_HashEntry    *e, *ne;
    size_t            top   = rjs_value_stack_save(rt);
    RJS_Value        *err   = rjs_value_stack_push(rt);
    RJS_Value        *pmodv = rjs_value_stack_push(rt);
    RJS_List          mod_list;

    rjs_vector_init(&mod_vec);
    rjs_list_init(&mod_list);

    mod = rjs_value_get_gc_thing(rt, modv);

    /*Gather the pending ancestor modules.*/
    rjs_hash_init(&mod_hash);
    gather_available_ancestors(rt, mod, &mod_vec, &mod_hash);
    rjs_hash_foreach_safe(&mod_hash, i, e, ne) {
        RJS_DEL(rt, e);
    }
    rjs_hash_deinit(&mod_hash, &rjs_hash_size_ops, rt);

    /*Sort the pending module list.*/
    rjs_sort(mod_vec.items, mod_vec.item_num, sizeof(RJS_Module*), async_module_sort, NULL);

    /*Check the pending modules.*/
    rjs_vector_foreach(&mod_vec, i, mod_ptr) {
        RJS_Module  *pmod = *mod_ptr;

        rjs_value_set_gc_thing(rt, pmodv, pmod);

        if (pmod->status == RJS_MODULE_STATUS_EVALUATED) {
            assert(!rjs_value_is_undefined(rt, &pmod->eval_error));
        } else if (module_has_tla(rt, pmod)) {
            execute_async_module(rt, pmodv);
        } else {
            RJS_Result r;

            r = execute_module(rt, pmodv, NULL);
            if (r == RJS_ERR) {
                rjs_catch(rt, err);
                async_module_execution_rejected(rt, pmodv, err);
            } else {
                pmod->status = RJS_MODULE_STATUS_EVALUATED;

                if (!rjs_value_is_undefined(rt, pmod->top_level_capability.promise)) {
                    assert(same_module(rt, &pmod->cycle_root, pmodv));

                    rjs_call(rt, pmod->top_level_capability.resolve, rjs_v_undefined(rt), NULL, 0, NULL);
                }

                rjs_list_append(&mod_list, &pmod->ln);
            }
        }
    }

    rjs_list_foreach_c(&mod_list, mod, RJS_Module, ln) {
        rjs_value_set_gc_thing(rt, pmodv, mod);
        execute_valid_modules(rt, pmodv);
    }

    rjs_vector_deinit(&mod_vec, rt);
    rjs_value_stack_restore(rt, top);
}

/*Async module fulfilled.*/
static RJS_Result
async_module_execution_fulfilled (RJS_Runtime *rt, RJS_Value *modv)
{
    RJS_Module *mod;

    mod = rjs_value_get_gc_thing(rt, modv);

    if (mod->status == RJS_MODULE_STATUS_EVALUATED) {
        assert(mod->eval_result == RJS_ERR);
        return RJS_OK;
    }

    mod->async_eval = 0;
    mod->status     = RJS_MODULE_STATUS_EVALUATED;

    if (!rjs_value_is_undefined(rt, mod->top_level_capability.promise)) {
        rjs_call(rt, mod->top_level_capability.resolve, rjs_v_undefined(rt), NULL, 0, NULL);
    }

    execute_valid_modules(rt, modv);
    return RJS_OK;
}

/*Async module fulfilled callback.*/
static RJS_Result
async_module_execution_fulfilled_nf (RJS_Runtime *rt, RJS_Value *f, RJS_Value *thiz, RJS_Value *args, size_t argc,
        RJS_Value *nt, RJS_Value *rv)
{
    RJS_Module *mod  = rjs_native_object_get_data(rt, f);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *modv = rjs_value_stack_push(rt);
    RJS_Result  r;

    rjs_value_set_undefined(rt, rv);
    rjs_value_set_gc_thing(rt, modv, mod);

    r = async_module_execution_fulfilled(rt, modv);

    rjs_value_stack_restore(rt, top);
    return r;
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
    RJS_Value  *err  = rjs_argument_get(rt, args, argc, 0);
    RJS_Module *mod  = rjs_native_object_get_data(rt, f);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *modv = rjs_value_stack_push(rt);
    RJS_Result  r;

    rjs_value_set_undefined(rt, rv);
    rjs_value_set_gc_thing(rt, modv, mod);

    r = async_module_execution_rejected(rt, modv, err);

    rjs_value_stack_restore(rt, top);
    return r;
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
    RJS_Result  r;
    size_t      i;

    mod = rjs_value_get_gc_thing(rt, modv);

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

        assert(!rjs_value_is_undefined(rt, &mr->module));

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

        mod->async_eval = rt->async_eval_cnt ++;

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
load_script_module (RJS_Runtime *rt, RJS_Value *mod, RJS_Input *input, const char *id, RJS_Realm *realm)
{
    RJS_Result r;

    input->flags |= RJS_INPUT_FL_CRLF_TO_LF;

    r = rjs_parse_module(rt, input, id, realm, mod);
    if (r == RJS_ERR)
        rjs_throw_syntax_error(rt, _("illegal module"));

    return r;
}

#if ENABLE_JSON

/*Load the JSON module.*/
static RJS_Result
load_json_module (RJS_Runtime *rt, RJS_Value *mod, RJS_Input *input, const char *id, RJS_Realm *realm)
{
    RJS_Result  r;
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *json = rjs_value_stack_push(rt);

    /*Load the module.*/
    if ((r = rjs_json_from_input(rt, json, input)) == RJS_OK) {
        RJS_Module      *m;
        RJS_Script      *s;
        RJS_Value       *v;
        RJS_ExportEntry *ee;
        RJS_BindingName  bn;
        RJS_String      *key;

        rjs_module_new(rt, mod, id, realm);

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

#if ENABLE_NATIVE_MODULE

/*Load the native module.*/
static RJS_Result
load_native_module (RJS_Runtime *rt, RJS_Value *mod, const char *path, RJS_Realm *realm)
{
    RJS_Module        *m;
    void              *handle = NULL;
    RJS_ModuleInitFunc init;
    RJS_Result         r;

    if (!(handle = dlopen(path, RTLD_LAZY))) {
        RJS_LOGE("cannot open native module \"%s\"", path);
        r = RJS_ERR;
        goto end;
    }

    init = dlsym(handle, "ratjs_module_init");
    if (!init) {
        RJS_LOGE("cannot find symbol \"ratjs_module_init\" in the \"%s\"", path);
        r = RJS_ERR;
        goto end;
    }

    rjs_module_new(rt, mod, path, realm);

    m = rjs_value_get_gc_thing(rt, mod);

    if ((r = init(rt, mod)) == RJS_ERR) {
        RJS_LOGE("initialize native module \"%s\" failed", path);
        r = RJS_ERR;
        goto end;
    }

    m->native_handle = handle;
    r = RJS_OK;
end:
    if (r == RJS_ERR) {
        if (handle)
            dlclose(handle);
        rjs_throw_syntax_error(rt, _("illegal native module \"%s\""), path);
    }
    return r;
}

#endif /*ENABLE_NATIVE_MODULE*/

/*Load a module.*/
static RJS_Result
load_module (RJS_Runtime *rt, RJS_ModuleType type, RJS_Input *input, const char *id,
        RJS_Realm *realm, RJS_Value *mod)
{
    char          *sub;
    RJS_HashEntry *he, **phe;
    RJS_Module    *m;
    RJS_Result     r;

    /*Check if the module is already loaded.*/
    r = rjs_hash_lookup(&rt->mod_hash, (void*)id, &he, &phe, &rjs_hash_char_star_ops, rt);
    if (r) {
        m = RJS_CONTAINER_OF(he, RJS_Module, he);
        if (m->status == RJS_MODULE_STATUS_LOADING_FAILED) {
            r = rjs_throw_syntax_error(rt, _("illegal module"));
            return r;
        }

        if (m->status != RJS_MODULE_STATUS_ALLOCATED) {
            rjs_value_set_gc_thing(rt, mod, m);
            return RJS_OK;
        }
    }

    sub = strrchr(id, '.');

    /*Load the module.*/
#if ENABLE_JSON
    if (sub && !strcasecmp(sub, ".json")) {
        r = load_json_module(rt, mod, input, id, realm);
    } else
#endif /*ENABLE_JSON*/
#if ENABLE_NATIVE_MODULE
    if (sub && !strcasecmp(sub, ".njs")) {
        r = load_native_module(rt, mod, id, realm);
    } else
#endif /*ENABLE_NATIVE_MODULE*/
    {
        r = load_script_module(rt, mod, input, id, realm);
    }

    if (r == RJS_ERR) {
        if (rjs_hash_lookup(&rt->mod_hash, (void*)id, &he, &phe, &rjs_hash_char_star_ops, rt)) {
            m = RJS_CONTAINER_OF(he, RJS_Module, he);
            m->status = RJS_MODULE_STATUS_LOADING_FAILED;
            rjs_value_copy(rt, &m->eval_error, &rt->error);
        }
    } else {
        m = rjs_value_get_gc_thing(rt, mod);
        m->status = RJS_MODULE_STATUS_LOADED;
    }

    return r;
}

/*Lookup the module.*/
static RJS_Result
lookup_module (RJS_Runtime *rt, RJS_Value *script, RJS_Value *name, RJS_Value *promise,
        RJS_Value *mod, RJS_Bool dynamic)
{
    const char    *nstr, *bstr;
    char           id[PATH_MAX];
    RJS_Result     r;
    RJS_Module    *modp  = NULL;
    RJS_Realm     *realm = rjs_realm_current(rt);
    size_t         top   = rjs_value_stack_save(rt);
    RJS_Value     *err   = rjs_value_stack_push(rt);
    RJS_Bool       done  = RJS_FALSE;
    RJS_PromiseCapability pc;

    if (!mod)
        mod = rjs_value_stack_push(rt);

    rjs_promise_capability_init(rt, &pc);

    if (script) {
        RJS_Script *base;

        base = rjs_value_get_gc_thing(rt, script);
        base = base->base_script;
        bstr = base->path;
    } else {
        bstr = NULL;
    }

    nstr = rjs_string_to_enc_chars(rt, name, NULL, NULL);

    if (!rt->mod_lookup_func
            || (r = rt->mod_lookup_func(rt, bstr, nstr, id)) == RJS_ERR) {
        if (dynamic)
            rjs_throw_type_error(rt, _("cannot resolve the module \"%s\""), nstr);
        else
            rjs_throw_reference_error(rt, _("cannot resolve the module \"%s\""), nstr);

        r = RJS_ERR;
        goto end;
    }

    if ((r = rjs_new_promise_capability(rt, rjs_o_Promise(realm), &pc)) == RJS_ERR)
        goto end;

    /*Check if the module is already loaded.*/
    if (!(modp = rjs_module_new(rt, mod, id, realm))) {
        r = RJS_ERR;
        goto end;
    }

    if (modp->status == RJS_MODULE_STATUS_LOADING_FAILED) {
        if (dynamic)
            rjs_throw_type_error(rt, _("cannot resolve the module \"%s\""), nstr);
        else
            rjs_throw_reference_error(rt, _("cannot resolve the module \"%s\""), nstr);

        rjs_catch(rt, err);
        r = rjs_call(rt, pc.reject, rjs_v_undefined(rt), err, 1, NULL);
        goto end;
    } else if (modp->status != RJS_MODULE_STATUS_ALLOCATED) {
        r = rjs_call(rt, pc.resolve, rjs_v_undefined(rt), mod, 1, NULL);
        goto end;
    }

    rjs_value_copy(rt, modp->top_level_capability.promise, pc.promise);

    /*Try to load the module.*/
    if (!rt->mod_load_func) {
        if (dynamic)
            rjs_throw_type_error(rt, _("cannot load the module \"%s\""), id);
        else
            rjs_throw_reference_error(rt, _("cannot load the module \"%s\""), id);

        r = RJS_ERR;
        goto end;
    }

    done = RJS_TRUE;
    if ((r = rt->mod_load_func(rt, id, &pc)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    if (modp) {
        if ((modp->status == RJS_MODULE_STATUS_ALLOCATED) && (r == RJS_ERR)) {
            modp->status = RJS_MODULE_STATUS_LOADING_FAILED;
            rjs_catch(rt, &modp->eval_error);

            if (!done)
                rjs_call(rt, pc.reject, rjs_v_undefined(rt), &modp->eval_error, 1, NULL);
        }

        if (promise && (r == RJS_OK))
            rjs_value_copy(rt, promise, pc.promise);
    }

    rjs_promise_capability_deinit(rt, &pc);
    rjs_value_stack_restore(rt, top);
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

            assert(!rjs_value_is_undefined(rt, &mr->module));

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

        assert(!rjs_value_is_undefined(rt, &mr->module));

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
 * Lookup the module.
 * \param rt The current runtime.
 * \param script The base script.
 * \param name The name of the module.
 * \param[out] promise Return the promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_lookup_module (RJS_Runtime *rt, RJS_Value *script, RJS_Value *name, RJS_Value *promise)
{
    return lookup_module(rt, script, name, promise, NULL, RJS_FALSE);
}

/**
 * Load the module.
 * \param rt The current runtime.
 * \param type The module's type.
 * \param input The module source input.
 * \param id The module's idneitifer.
 * \param realm The realm of the module.
 * \param[out] mod Return the new module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_load_module (RJS_Runtime *rt, RJS_ModuleType type, RJS_Input *input,
        const char *id, RJS_Realm *realm, RJS_Value *mod)
{
    if (!realm)
        realm = rjs_realm_current(rt);
        
    return load_module(rt, type, input, id, realm, mod);
}

/*Module dynamic import data.*/
typedef struct {
    int                   ref;       /**< Reference counter.*/
    RJS_PromiseCapability pc;        /**< Dynamic import promise capability.*/
    RJS_Value             mod;       /**< The module.*/
    RJS_Value             promisev;  /**< Promise value of the pc.*/
    RJS_Value             resolvev;  /**< Resovle value of the pc.*/
    RJS_Value             rejectv;   /**< Reject value of the pc.*/
} RJS_ModuleDynamicData;

/*Scan reference things in the module dynamic import data.*/
static void
module_dynamic_data_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleDynamicData *mdd = ptr;

    rjs_gc_scan_value(rt, &mdd->mod);
    rjs_gc_scan_value(rt, &mdd->promisev);
    rjs_gc_scan_value(rt, &mdd->resolvev);
    rjs_gc_scan_value(rt, &mdd->rejectv);
}

/*Free the module dynamic import data.*/
static void
module_dynamic_data_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ModuleDynamicData *mdd = ptr;

    mdd->ref --;

    if (mdd->ref == 0) {
        rjs_promise_capability_deinit(rt, &mdd->pc);
        RJS_DEL(rt, mdd);
    }
}

/*Allocate a new module dynamic data.*/
static RJS_ModuleDynamicData*
module_dynamic_data_new (RJS_Runtime *rt, RJS_PromiseCapability *pc)
{
    RJS_ModuleDynamicData *mdd;

    RJS_NEW(rt, mdd);

    mdd->ref = 1;

    rjs_value_set_undefined(rt, &mdd->promisev);
    rjs_value_set_undefined(rt, &mdd->resolvev);
    rjs_value_set_undefined(rt, &mdd->rejectv);
    rjs_value_set_undefined(rt, &mdd->mod);

    rjs_promise_capability_init_vp(rt, &mdd->pc, &mdd->promisev, &mdd->resolvev, &mdd->rejectv);
    rjs_promise_capability_copy(rt, &mdd->pc, pc);

    return mdd;
}

/*Module evaluate resolve function.*/
static RJS_NF(module_eval_resolve)
{
    RJS_ModuleDynamicData *mdd = rjs_native_object_get_data(rt, f);
    size_t                 top = rjs_value_stack_save(rt);
    RJS_Value             *ns  = rjs_value_stack_push(rt);
    RJS_Value             *err = rjs_value_stack_push(rt);
    RJS_Result             r;

    if ((r = rjs_module_get_namespace(rt, &mdd->mod, ns)) == RJS_ERR) {
        rjs_catch(rt, err);
        rjs_call(rt, mdd->pc.reject, rjs_v_undefined(rt), err, 1, NULL);
    } else {
        rjs_call(rt, mdd->pc.resolve, rjs_v_undefined(rt), ns, 1, NULL);

        r = RJS_OK;
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/*Module dynamic load reject function.*/
static RJS_NF(module_load_reject)
{
    RJS_ModuleDynamicData *mdd = rjs_native_object_get_data(rt, f);
    RJS_Value             *v   = rjs_argument_get(rt, args, argc, 0);

    rjs_call(rt, mdd->pc.reject, rjs_v_undefined(rt), v, 1, NULL);

    return RJS_OK;
}

/*Module dynamic load resolve function.*/
static RJS_Result
module_load_requested (RJS_Runtime *rt, RJS_ModuleDynamicData *mdd)
{
    size_t      top     = rjs_value_stack_save(rt);
    RJS_Value  *p       = rjs_value_stack_push(rt);
    RJS_Value  *resolve = rjs_value_stack_push(rt);
    RJS_Value  *reject  = rjs_value_stack_push(rt);
    RJS_Value  *err     = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_module_link(rt, &mdd->mod)) == RJS_ERR)
        goto end;

    if ((r = rjs_module_evaluate(rt, &mdd->mod, p)) == RJS_ERR)
        goto end;

    r = rjs_native_func_object_new(rt, resolve, NULL, NULL, NULL, module_eval_resolve, 0);
    if (r == RJS_ERR)
        goto end;
    rjs_native_object_set_data(rt, resolve, NULL, mdd, module_dynamic_data_scan, module_dynamic_data_free);
    mdd->ref ++;

    r = rjs_native_func_object_new(rt, reject, NULL, NULL, NULL, module_load_reject, 0);
    if (r == RJS_ERR)
        goto end;
    rjs_native_object_set_data(rt, reject, NULL, mdd, module_dynamic_data_scan, module_dynamic_data_free);
    mdd->ref ++;

    if ((r = rjs_invoke(rt, p, rjs_pn_then(rt), resolve, 2, NULL)) == RJS_ERR)
        goto end;
end:
    if (r == RJS_ERR) {
        rjs_catch(rt, err);
        rjs_call(rt, mdd->pc.reject, rjs_v_undefined(rt), err, 1, NULL);
        r = RJS_OK;
    }
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Module dynamic load resolve function.*/
static RJS_NF(module_load_requested_resolve)
{
    RJS_ModuleDynamicData *mdd = rjs_native_object_get_data(rt, f);

    return module_load_requested(rt, mdd);
}

/*Module dynamic load resolve function.*/
static RJS_NF(module_load_resolve)
{
    RJS_ModuleDynamicData *mdd     = rjs_native_object_get_data(rt, f);
    RJS_Value             *mod     = rjs_argument_get(rt, args, argc, 0);
    size_t                 top     = rjs_value_stack_save(rt);
    RJS_Value             *p       = rjs_value_stack_push(rt);
    RJS_Value             *resolve = rjs_value_stack_push(rt);
    RJS_Value             *reject  = rjs_value_stack_push(rt);
    RJS_Value             *err     = rjs_value_stack_push(rt);
    RJS_Module            *modp    = rjs_value_get_gc_thing(rt, mod);
    RJS_Result             r;

    rjs_value_copy(rt, &mdd->mod, mod);

    if (modp->status != RJS_MODULE_STATUS_LOADED) {
        r = module_load_requested(rt, mdd);
        goto end;
    }

    if ((r = rjs_module_load_requested(rt, mod, p)) == RJS_ERR)
        goto end;

    r = rjs_native_func_object_new(rt, resolve, NULL, NULL, NULL, module_load_requested_resolve, 0);
    if (r == RJS_ERR)
        goto end;
    rjs_native_object_set_data(rt, resolve, NULL, mdd, module_dynamic_data_scan, module_dynamic_data_free);
    mdd->ref ++;

    r = rjs_native_func_object_new(rt, reject, NULL, NULL, NULL, module_load_reject, 0);
    if (r == RJS_ERR)
        goto end;
    rjs_native_object_set_data(rt, reject, NULL, mdd, module_dynamic_data_scan, module_dynamic_data_free);
    mdd->ref ++;

    if ((r = rjs_invoke(rt, p, rjs_pn_then(rt), resolve, 2, NULL)) == RJS_ERR)
        goto end;
end:
    if (r == RJS_ERR) {
        rjs_catch(rt, err);
        rjs_call(rt, mdd->pc.reject, rjs_v_undefined(rt), err, 1, NULL);
        r = RJS_OK;
    }
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Import the module dynamically.*/
static RJS_Result
module_import_dynamically (RJS_Runtime *rt, RJS_Value *ref, RJS_Value *spec, RJS_PromiseCapability *pc)
{
    RJS_Result             r;
    RJS_ModuleDynamicData *mdd     = NULL;
    size_t                 top     = rjs_value_stack_save(rt);
    RJS_Value             *str     = rjs_value_stack_push(rt);
    RJS_Value             *err     = rjs_value_stack_push(rt);
    RJS_Value             *p       = rjs_value_stack_push(rt);
    RJS_Value             *resolve = rjs_value_stack_push(rt);
    RJS_Value             *reject  = rjs_value_stack_push(rt);

    if ((r = rjs_to_string(rt, spec, str)) == RJS_ERR)
        goto end;

    if ((r = lookup_module(rt, ref, str, p, NULL, RJS_TRUE)) == RJS_ERR)
        goto end;

    mdd = module_dynamic_data_new(rt, pc);

    r = rjs_native_func_object_new(rt, resolve, NULL, NULL, NULL, module_load_resolve, 0);
    if (r == RJS_ERR)
        goto end;
    rjs_native_object_set_data(rt, resolve, NULL, mdd, module_dynamic_data_scan, module_dynamic_data_free);
    mdd->ref ++;

    r = rjs_native_func_object_new(rt, reject, NULL, NULL, NULL, module_load_reject, 0);
    if (r == RJS_ERR)
        goto end;
    rjs_native_object_set_data(rt, reject, NULL, mdd, module_dynamic_data_scan, module_dynamic_data_free);
    mdd->ref ++;

    if ((r = rjs_invoke(rt, p, rjs_pn_then(rt), resolve, 2, NULL)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    if (mdd)
        module_dynamic_data_free(rt, mdd);

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
rjs_module_get_binding (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *name, RJS_Value *v)
{
    RJS_BindingName bn;
    RJS_Module     *mod;
    RJS_Result      r;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    rjs_string_to_property_key(rt, name);

    rjs_binding_name_init(rt, &bn, name);
    r = rjs_env_get_binding_value(rt, mod->env, &bn, RJS_TRUE, v);
    rjs_binding_name_deinit(rt, &bn);

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
rjs_module_add_binding (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *name, RJS_Value *v)
{
    RJS_BindingName bn;
    RJS_Module     *mod;
    RJS_Result      r;

    assert(rjs_value_is_module(rt, modv));

    mod = rjs_value_get_gc_thing(rt, modv);

    rjs_string_to_property_key(rt, name);

    rjs_binding_name_init(rt, &bn, name);
    r = rjs_env_create_immutable_binding(rt, mod->env, &bn, RJS_TRUE);
    if (r == RJS_OK)
        r = rjs_env_initialize_binding(rt, mod->env, &bn, v);
    rjs_binding_name_deinit(rt, &bn);

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

#if ENABLE_ASYNC
    rt->async_eval_cnt = 1;
#endif /*ENABLE_ASYNC*/
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
