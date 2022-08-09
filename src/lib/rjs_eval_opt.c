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

/**
 * Eval declaration instantiation.
 * \param rt The current runtime.
 * \param script The script.
 * \param decl The declaration.
 * \param var_grp The variable declaration group.
 * \param lex_grp The lexical declaration group.
 * \param func_grp The functions to be initialized.
 * \param strict In strict mode or not.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_eval_declaration_instantiation (RJS_Runtime *rt,
        RJS_Script *script,
        RJS_ScriptDecl *decl,
        RJS_ScriptBindingGroup *var_grp,
        RJS_ScriptBindingGroup *lex_grp,
        RJS_ScriptFuncDeclGroup *func_grp,
        RJS_Bool strict)
{
    RJS_ScriptContext    *sc         = (RJS_ScriptContext*)rjs_context_running(rt);
    RJS_Environment      *var_env    = sc->scb.var_env;
    RJS_Environment      *lex_env    = sc->scb.lex_env;
    RJS_Environment      *global_env = rjs_global_env(script->realm);
    size_t                top        = rjs_value_stack_save(rt);
    RJS_Value            *tmp        = rjs_value_stack_push(rt);
    RJS_Environment      *curr_env;
    size_t                id;
    RJS_ScriptBinding    *sb;
    RJS_ScriptBindingRef *sbr;
    RJS_ScriptFuncDecl   *sfd;
    RJS_Result            r;

    lex_env->script_decl = decl;

    if (!strict && var_grp) {
        if (var_env == global_env) {
            for (id = var_grp->binding_start;
                    id < var_grp->binding_start + var_grp->binding_num;
                    id ++) {
                sb  = &script->binding_table[id];
                sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

                if ((r = rjs_env_has_lexical_declaration(rt, var_env, &sbr->binding_name))) {
                    r = rjs_throw_syntax_error(rt, _("\"%s\" is already declared"),
                            rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                    goto end;
                }
            }
        }

        curr_env = lex_env;
        while (curr_env != var_env) {
            for (id = var_grp->binding_start;
                    id < var_grp->binding_start + var_grp->binding_num;
                    id ++) {
                sb  = &script->binding_table[id];
                sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

                if ((r = rjs_env_has_binding(rt, curr_env, &sbr->binding_name))) {
                    r = rjs_throw_syntax_error(rt, _("\"%s\" is already declared"),
                            rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                    goto end;
                }
            }

            curr_env = curr_env->outer;
        }
    }

    if (func_grp && (var_env == global_env)) {
        for (id = func_grp->func_decl_start;
                id < func_grp->func_decl_start + func_grp->func_decl_num;
                id ++) {
            sfd = &script->func_decl_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sfd->binding_ref_idx];

            if ((r = rjs_env_can_declare_global_function(rt, var_env, &sbr->binding_name)) == RJS_ERR)
                goto end;
            if (!r) {
                r = rjs_throw_type_error(rt, _("global function \"%s\" is already declared"),
                        rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                goto end;
            }
        }
    }

    if (var_grp && (var_env == global_env)) {
        for (id = var_grp->binding_start;
                id < var_grp->binding_start + var_grp->binding_num;
                id ++) {
            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if ((r = rjs_env_can_declare_global_var(rt, var_env, &sbr->binding_name)) == RJS_ERR)
                goto end;
            if (!r) {
                r = rjs_throw_type_error(rt, _("global variable \"%s\" is already declared"),
                        rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                goto end;
            }
        }
    }

    if (lex_grp) {
        for (id = lex_grp->binding_start;
                id < lex_grp->binding_start + lex_grp->binding_num;
                id ++) {
            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if (sb->flags & RJS_SCRIPT_BINDING_FL_CONST) {
                r = rjs_env_create_immutable_binding(rt, lex_env, &sbr->binding_name, RJS_TRUE);
            } else {
                r = rjs_env_create_mutable_binding(rt, lex_env, &sbr->binding_name, RJS_FALSE);
            }

            if (r == RJS_ERR)
                goto end;
        }
    }

    if (func_grp) {
        for (id = func_grp->func_decl_start;
                id < func_grp->func_decl_start + func_grp->func_decl_num;
                id ++) {
            RJS_ScriptFunc *sf;
            RJS_PrivateEnv *priv_env = NULL;

#if ENABLE_PRIV_NAME
            priv_env = rjs_private_env_running(rt);
#endif /*ENABLE_PRIV_NAME*/

            sfd = &script->func_decl_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sfd->binding_ref_idx];
            sf  = &script->func_table[sfd->func_idx];

            if ((r = rjs_create_function(rt, script, sf, lex_env, priv_env, RJS_TRUE, tmp)) == RJS_ERR)
                goto end;

            if (var_env == global_env) {
                if ((r = rjs_env_create_global_function_binding(rt, var_env, &sbr->binding_name, tmp, RJS_TRUE)) == RJS_ERR)
                    goto end;
            } else {
                r = rjs_env_has_binding(rt, var_env, &sbr->binding_name);
                if (!r) {
                    rjs_env_create_mutable_binding(rt, var_env, &sbr->binding_name, RJS_TRUE);
                    rjs_env_initialize_binding(rt, var_env, &sbr->binding_name, tmp);
                } else {
                    rjs_env_set_mutable_binding(rt, var_env, &sbr->binding_name, tmp, RJS_FALSE);
                }
            }
        }
    }

    if (var_grp) {
        for (id = var_grp->binding_start;
                id < var_grp->binding_start + var_grp->binding_num;
                id ++) {
            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if (var_env == global_env) {
                if ((r = rjs_env_create_global_var_binding(rt, var_env, &sbr->binding_name, RJS_TRUE)) == RJS_ERR)
                    goto end;
                if (!r) {
                    r = rjs_throw_type_error(rt, _("global variable \"%s\" is already declared"),
                            rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                    goto end;
                }
            } else {
                r = rjs_env_has_binding(rt, var_env, &sbr->binding_name);
                if (!r) {
                    rjs_env_create_mutable_binding(rt, var_env, &sbr->binding_name, RJS_TRUE);
                    rjs_env_initialize_binding(rt, var_env, &sbr->binding_name, rjs_v_undefined(rt));
                }
            }
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Compile and evalute script.
 * \param rt The current runtime.
 * \param[out] scriptv Return the script value.
 * \param x The input string.
 * \param realm The realm.
 * \param strict For the eval as strict mode code.
 * \param direct Direct call "eval".
 * \retval RJS_OK On success.
 * \retval RJS_FALSE The input is not a string.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_eval_from_string (RJS_Runtime *rt, RJS_Value *scriptv, RJS_Value *x, RJS_Realm *realm,
        RJS_Bool strict, RJS_Bool direct)
{
    int                flags       = RJS_PARSE_FL_ARGS;
    RJS_Bool           need_close  = RJS_FALSE;
    RJS_Environment   *env         = NULL;
    RJS_PrivateEnv    *priv_env    = NULL;
    RJS_Script        *base_script = NULL;
    RJS_Context       *ctxt;
    RJS_Input          si;
    RJS_Result         r;

    if (!rjs_value_is_string(rt, x)) {
        r = RJS_FALSE;
        goto end;
    }

    if (!realm)
        realm = rjs_realm_current(rt);

    if (strict)
        flags |= RJS_PARSE_FL_STRICT;

    if (direct) {
        /*Get the running script context.*/
        ctxt = rjs_context_running(rt);
        while (ctxt && (ctxt->gc_thing.ops->type == RJS_GC_THING_CONTEXT))
            ctxt = ctxt->bot;

        if (ctxt) {
            RJS_ScriptContext *sc = (RJS_ScriptContext*)ctxt;

            base_script = sc->script;

            if (sc->script_func->flags & RJS_FUNC_FL_STRICT)
                flags |= RJS_PARSE_FL_STRICT;

            for (env = sc->scb.lex_env; env; env = env->outer) {
                if (rjs_env_has_this_binding(rt, env))
                    break;
            }
        }

        /*Get the flags and the private environment.*/
        if (env && (env->gc_thing.ops->type == RJS_GC_THING_FUNCTION_ENV)) {
            RJS_ScriptContext    *sc  = (RJS_ScriptContext*)ctxt;
            RJS_FunctionEnv      *fe  = (RJS_FunctionEnv*)env;
            RJS_ScriptFuncObject *sfo = (RJS_ScriptFuncObject*)rjs_value_get_object(rt, &fe->function);

            /*Get the parser flags.*/
            flags |= RJS_PARSE_FL_NEW_TARGET;

            if (rjs_env_has_super_binding(rt, env))
                flags |= RJS_PARSE_FL_SUPER_PROP;

            if (sfo->script_func->flags & RJS_FUNC_FL_DERIVED)
                flags |= RJS_PARSE_FL_SUPER_CALL;

            if (sfo->script_func->flags & RJS_FUNC_FL_CLASS_FIELD_INIT)
                flags &= ~RJS_PARSE_FL_ARGS;

#if ENABLE_PRIV_NAME
            priv_env = sc->scb.priv_env;
#endif /*ENABLE_PRIV_NAME*/
        } else {
            ctxt = NULL;
        }
    }

    /*Create the input source.*/
    if ((r = rjs_string_input_init(rt, &si, x)) == RJS_ERR)
        goto end;

    si.flags |= RJS_INPUT_FL_CRLF_TO_LF;
    need_close = RJS_TRUE;

    /*Parse the input source.*/
    if ((r = rjs_parse_eval(rt, &si, realm, flags, priv_env, scriptv)) == RJS_ERR) {
        r = rjs_throw_syntax_error(rt, _("syntax error"));
        goto end;
    }

    /*Set the eval's base script.*/
    if (base_script) {
        RJS_Script *script = rjs_value_get_gc_thing(rt, scriptv);

        script->base_script = base_script;
    }

    r = RJS_OK;
end:
    /*Close the input.*/
    if (need_close)
        rjs_input_deinit(rt, &si);

    return r;
}

/**
 * Evaluate the "eval" script.
 * \param rt The current runtime.
 * \param scriptv The script.
 * \param direct Direct call "eval".
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_eval_evaluation (RJS_Runtime *rt, RJS_Value *scriptv, RJS_Bool direct, RJS_Value *rv)
{
    RJS_Script        *script;
    RJS_ScriptFunc    *sf;
    RJS_Environment   *lex_env, *var_env, *global_env;
    RJS_Result         r;
    RJS_Context       *ctxt;
    RJS_ScriptDecl    *old_script_decl;
    RJS_PrivateEnv    *priv_env = NULL;
    size_t             top      = rjs_value_stack_save(rt);

    script  = rjs_value_get_gc_thing(rt, scriptv);
    sf      = script->func_table;

    ctxt       = NULL;
    global_env = rjs_global_env(script->realm);
    lex_env    = global_env;
    var_env    = global_env;

    /*The global script declaration should be changed.*/
    old_script_decl = global_env->script_decl;

    if (direct) {
        /*Get the running script context.*/
        ctxt = rjs_context_running(rt);
        while (ctxt && (ctxt->gc_thing.ops->type == RJS_GC_THING_CONTEXT))
            ctxt = ctxt->bot;

        if (ctxt) {
            RJS_ScriptContext *sc = (RJS_ScriptContext*)ctxt;

            lex_env = sc->scb.lex_env;
            var_env = sc->scb.var_env;

#if ENABLE_PRIV_NAME
            priv_env = sc->scb.priv_env;
#endif /*ENABLE_PRIV_NAME*/
        }
    }

    /*Create the environment.*/
    rjs_decl_env_new(rt, &rt->env, NULL, lex_env);
    lex_env = rt->env;

    if (sf->flags & RJS_FUNC_FL_STRICT)
        var_env = lex_env;

    /*Run the script.*/
    ctxt = rjs_script_context_push(rt, NULL, script, sf,
            var_env, lex_env, priv_env, NULL, 0);

    ctxt->realm = script->realm;

    if (!rv)
        rv = rjs_value_stack_push(rt);

    r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_SYNC_START, NULL, rv);

    rjs_context_pop(rt);

    rjs_value_stack_restore(rt, top);

    /*Restore the old script declaration.*/
    global_env->script_decl = old_script_decl;
    return r;
}
