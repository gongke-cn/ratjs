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

/*Free the script.*/
static void
script_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Script *script = ptr;

    rjs_script_deinit(rt, script);

    RJS_DEL(rt, script);
}

/*Script GC operation functions.*/
static const RJS_GcThingOps
script_gc_ops = {
    RJS_GC_THING_SCRIPT,
    rjs_script_op_gc_scan,
    script_op_gc_free
};

/**
 * Initialize the script.
 * \param rt The current runtime.
 * \param script The script to be initialized.
 * \param realm The realm.
 */
void
rjs_script_init (RJS_Runtime *rt, RJS_Script *script, RJS_Realm *realm)
{
    script->realm                 = realm;
    script->base_script           = script;
    script->path                  = NULL;
    script->value_table           = NULL;
    script->func_table            = NULL;
    script->decl_table            = NULL;
    script->binding_table         = NULL;
    script->func_decl_table       = NULL;
    script->binding_ref_table     = NULL;
    script->prop_ref_table        = NULL;
    script->binding_group_table   = NULL;
    script->func_decl_group_table = NULL;
    script->value_num             = 0;
    script->func_num              = 0;
    script->decl_num              = 0;
    script->binding_num           = 0;
    script->func_decl_num         = 0;
    script->binding_ref_num       = 0;
    script->prop_ref_num          = 0;
    script->binding_group_num     = 0;
    script->func_decl_group_num   = 0;

    script->byte_code     = NULL;
    script->line_info     = NULL;
    script->byte_code_len = 0;
    script->line_info_num = 0;

#if ENABLE_MODULE
    script->mod_decl_idx     = -1;
    script->mod_var_grp_idx  = -1;
    script->mod_lex_grp_idx  = -1;
    script->mod_func_grp_idx = -1;
#endif /*ENABLE_MODULE*/

#if ENABLE_PRIV_NAME
    script->priv_id_num    = 0;
    script->priv_id_table  = NULL;
    script->priv_env_num   = 0;
    script->priv_env_table = NULL;
#endif /*ENABLE_PRIV_NAME*/
}

/**
 * Release the script.
 * \param rt The current runtime.
 * \param script The script to be released.
 */
void
rjs_script_deinit (RJS_Runtime *rt, RJS_Script *script)
{
    size_t i;

    /*Free the path.*/
    if (script->path)
        rjs_char_star_free(rt, script->path);

    /*Release all the binding reference.*/
    for (i = 0; i < script->binding_ref_num; i ++) {
        RJS_BindingName *bn;

        bn = &script->binding_ref_table[i].binding_name;

        rjs_binding_name_deinit(rt, bn);
    }

    /*Release all the property reference.*/
    for (i = 0; i < script->prop_ref_num; i ++) {
        RJS_PropertyName *pn;

        pn = &script->prop_ref_table[i].prop_name;

        rjs_property_name_deinit(rt, pn);
    }

    /*Free the buffers.*/
    if (script->value_table)
        RJS_DEL_N(rt, script->value_table, script->value_num);
    if (script->func_table)
        RJS_DEL_N(rt, script->func_table, script->func_num);
    if (script->decl_table)
        RJS_DEL_N(rt, script->decl_table, script->decl_num);
    if (script->binding_table)
        RJS_DEL_N(rt, script->binding_table, script->binding_num);
    if (script->func_decl_table)
        RJS_DEL_N(rt, script->func_decl_table, script->func_decl_num);
    if (script->binding_ref_table)
        RJS_DEL_N(rt, script->binding_ref_table, script->binding_ref_num);
    if (script->prop_ref_table)
        RJS_DEL_N(rt, script->prop_ref_table, script->prop_ref_num);
    if (script->binding_group_table)
        RJS_DEL_N(rt, script->binding_group_table, script->binding_group_num);
    if (script->func_decl_group_table)
        RJS_DEL_N(rt, script->func_decl_group_table, script->func_decl_group_num);
    if (script->byte_code)
        RJS_DEL_N(rt, script->byte_code, script->byte_code_len);
    if (script->line_info)
        RJS_DEL_N(rt, script->line_info, script->line_info_num);

#if ENABLE_PRIV_NAME
    if (script->priv_id_table)
        RJS_DEL_N(rt, script->priv_id_table, script->priv_id_num);
    if (script->priv_env_table)
        RJS_DEL_N(rt, script->priv_env_table, script->priv_env_num);
#endif /*ENABLE_PRIV_NAME*/
}

/**
 * Scan the referenced things in the script.
 * \param rt The current runtime.
 * \param ptr The script's pointer.
 */
void
rjs_script_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Script *script = ptr;

    if (script->realm)
        rjs_gc_mark(rt, script->realm);

    rjs_gc_scan_value_buffer(rt, script->value_table, script->value_num);
}

/**
 * Create a new script.
 * \param rt The current runtime.
 * \param[out] v Return the script value.
 * \param realm The realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Script*
rjs_script_new (RJS_Runtime *rt, RJS_Value *v, RJS_Realm *realm)
{
    RJS_Script *script;

    RJS_NEW(rt, script);

    rjs_script_init(rt, script, realm);

    rjs_value_set_gc_thing(rt, v, script);
    rjs_gc_add(rt, script, &script_gc_ops);

    return script;
}

#if ENABLE_SCRIPT

/**
 * Create a script from the file.
 * \param rt The current runtime.
 * \param[out] v Return the script value.
 * \param filename The script filename.
 * \param realm The realm.
 * \param force_strict Force the script as strict mode script.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_from_file (RJS_Runtime *rt, RJS_Value *v, const char *filename,
        RJS_Realm *realm, RJS_Bool force_strict)
{
    RJS_Input  fi;
    RJS_Result r;
    int        flags = 0;

    if ((r = rjs_file_input_init(rt, &fi, filename, NULL)) == RJS_ERR)
        return r;

    fi.flags |= RJS_INPUT_FL_CRLF_TO_LF;
    
    if (!realm)
        realm = rjs_realm_current(rt);

    if (force_strict)
        flags |= RJS_PARSE_FL_STRICT;

    r = rjs_parse_script(rt, &fi, realm, flags, v);

    rjs_input_deinit(rt, &fi);

    if (r == RJS_OK) {
        RJS_Script *script = rjs_value_get_gc_thing(rt, v);

        script->path = rjs_char_star_dup(rt, filename);
    }

    return r;
}

/**
 * Create a script from a string.
 * \param rt The current runtime.
 * \param[out] v Return the script value.
 * \param str The string.
 * \param realm The realm.
 * \param force_strict Force the script as strict mode script.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_from_string (RJS_Runtime *rt, RJS_Value *v, RJS_Value *str,
        RJS_Realm *realm, RJS_Bool force_strict)
{
    RJS_Input  si;
    RJS_Result r;
    int        flags = 0;

    assert(rjs_value_is_string(rt, str));

    if ((r = rjs_string_input_init(rt, &si, str)) == RJS_ERR)
        return r;

    si.flags |= RJS_INPUT_FL_CRLF_TO_LF;
    
    if (!realm)
        realm = rjs_realm_current(rt);

    if (force_strict)
        flags |= RJS_PARSE_FL_STRICT;

    r = rjs_parse_script(rt, &si, realm, flags, v);

    rjs_input_deinit(rt, &si);

    return r;
}

/**
 * Evaluate the script.
 * \param rt The current runtime.
 * \param v The script value.
 * \param rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_evaluation (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_Script      *script;
    RJS_ScriptFunc  *sf;
    RJS_Result       r;
    RJS_Context     *ctxt;
    RJS_Environment *global_env;
    RJS_ScriptDecl  *old_script_decl;

    assert(rjs_value_is_script(rt, v));

    script     = rjs_value_get_gc_thing(rt, v);
    sf         = script->func_table;
    global_env = rjs_global_env(script->realm);

    /*Save the old script declaration.*/
    old_script_decl = global_env->script_decl;

    ctxt = rjs_script_context_push(rt, NULL, script, sf,
            global_env, global_env, NULL, NULL, 0);

    ctxt->realm = script->realm;

    r = rjs_script_func_call(rt, RJS_SCRIPT_CALL_SYNC_START, NULL, rv);

    rjs_context_pop(rt);

    /*Restore the old script declaration.*/
    global_env->script_decl = old_script_decl;
    return r;
}

#endif /*ENABLE_SCRIPT*/

/**
 * Global declaration instantiation.
 * \param rt The current runtime.
 * \param script The script.
 * \param decl The declaration.
 * \param var_grp The variable declaration group.
 * \param lex_grp The lexical declaration group.
 * \param func_grp The functions to be initialized.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_global_declaration_instantiation (RJS_Runtime *rt,
        RJS_Script *script,
        RJS_ScriptDecl *decl,
        RJS_ScriptBindingGroup *var_grp,
        RJS_ScriptBindingGroup *lex_grp,
        RJS_ScriptFuncDeclGroup *func_grp)
{
    RJS_ScriptBinding    *sb;
    RJS_ScriptBindingRef *sbr;
    RJS_ScriptFuncDecl   *sfd;
    size_t                id;
    RJS_Result            r;
    RJS_Environment      *env = rjs_global_env(script->realm);
    size_t                top = rjs_value_stack_save(rt);
    RJS_Value            *tmp = rjs_value_stack_push(rt);

    env->script_decl = decl;

    if (lex_grp) {
        for (id = lex_grp->binding_start;
                id < lex_grp->binding_start + lex_grp->binding_num;
                id ++) {
            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if ((r = rjs_env_has_var_declaration(rt, env, &sbr->binding_name))) {
                r = rjs_throw_syntax_error(rt, _("\"%s\" is already declared"),
                        rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                goto end;
            }
            if ((r = rjs_env_has_lexical_declaration(rt, env, &sbr->binding_name))) {
                r = rjs_throw_syntax_error(rt, _("\"%s\" is already declared"),
                        rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                goto end;
            }
            if ((r = rjs_env_has_restricted_global_property(rt, env, &sbr->binding_name)) == RJS_ERR)
                goto end;
            if (r) {
                r = rjs_throw_syntax_error(rt, _("\"%s\" is already declared"),
                        rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                goto end;
            }
        }
    }

    if (var_grp) {
        for (id = var_grp->binding_start;
                id < var_grp->binding_start + var_grp->binding_num;
                id ++) {
            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if ((r = rjs_env_has_lexical_declaration(rt, env, &sbr->binding_name))) {
                r = rjs_throw_syntax_error(rt, _("\"%s\" is already declared"),
                        rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                goto end;
            }
        }
    }

    if (func_grp) {
        for (id = func_grp->func_decl_start;
                id < func_grp->func_decl_start + func_grp->func_decl_num;
                id ++) {
            sfd = &script->func_decl_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sfd->binding_ref_idx];

            if ((r = rjs_env_can_declare_global_function(rt, env, &sbr->binding_name)) == RJS_ERR)
                goto end;
            if (!r) {
                r = rjs_throw_type_error(rt, _("global function \"%s\" is already declared"),
                        rjs_string_to_enc_chars(rt, sbr->binding_name.name, NULL, NULL));
                goto end;
            }
        }
    }

    if (var_grp) {
        for (id = var_grp->binding_start;
                id < var_grp->binding_start + var_grp->binding_num;
                id ++) {
            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if ((r = rjs_env_can_declare_global_var(rt, env, &sbr->binding_name)) == RJS_ERR)
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
                r = rjs_env_create_immutable_binding(rt, env, &sbr->binding_name, RJS_TRUE);
            } else {
                r = rjs_env_create_mutable_binding(rt, env, &sbr->binding_name, RJS_FALSE);
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

            sfd = &script->func_decl_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sfd->binding_ref_idx];
            sf  = &script->func_table[sfd->func_idx];

            if ((r = rjs_create_function(rt, script, sf, env, NULL, RJS_TRUE, tmp)) == RJS_ERR)
                goto end;

            if ((r = rjs_env_create_global_function_binding(rt, env, &sbr->binding_name, tmp, RJS_FALSE)) == RJS_ERR)
                goto end;
        }
    }

    if (var_grp) {
        for (id = var_grp->binding_start;
                id < var_grp->binding_start + var_grp->binding_num;
                id ++) {
            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if ((r = rjs_env_create_global_var_binding(rt, env, &sbr->binding_name, RJS_FALSE)) == RJS_ERR)
                goto end;
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Initialize the binding group in the environment.
 * \param rt The current runtime.
 * \param script The script.
 * \param grp The binding group to be initialized.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_binding_group_init (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptBindingGroup *grp)
{
    size_t           id;
    RJS_Environment *env  = rjs_lex_env_running(rt);
    RJS_ScriptDecl  *decl = env->script_decl;
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *tmp  = rjs_value_stack_push(rt);

    for (id = grp->binding_start; id < grp->binding_start + grp->binding_num; id ++) {
        RJS_ScriptBinding    *sb;
        RJS_ScriptBindingRef *sbr;

        sb  = &script->binding_table[id];
        sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

        if (sb->flags & RJS_SCRIPT_BINDING_FL_CONST) {
            RJS_Bool strict = (sb->flags & RJS_SCRIPT_BINDING_FL_STRICT) ? RJS_TRUE : RJS_FALSE;

            rjs_env_create_immutable_binding(rt, env, &sbr->binding_name, strict);
        } else {
            rjs_env_create_mutable_binding(rt, env, &sbr->binding_name, RJS_FALSE);
        }

        if (sb->flags & RJS_SCRIPT_BINDING_FL_BOT) {
            RJS_Environment      *benv  = env->outer;
            RJS_ScriptDecl       *bdecl = benv->script_decl;
            RJS_ScriptBindingRef *bsbr;

            bsbr = &script->binding_ref_table[bdecl->binding_ref_start + sb->bot_ref_idx];

            rjs_env_get_binding_value(rt, benv, &bsbr->binding_name, RJS_FALSE, tmp);
            rjs_env_initialize_binding(rt, env, &sbr->binding_name, tmp);

        } else if (sb->flags & RJS_SCRIPT_BINDING_FL_UNDEF) {
            rjs_env_initialize_binding(rt, env, &sbr->binding_name, rjs_v_undefined(rt));
        }
    }

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/**
 * Duplicate the bindings from the source environemnt to current environment.
 * \param rt The current runtime.
 * \param scrip The script.
 * \param grp The binding group to be initialized.
 * \param env The current environemnt.
 * \param src The source environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_binding_group_dup (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptBindingGroup *grp,
        RJS_Environment *env, RJS_Environment *src)
{
    size_t          id;
    RJS_Result      r;
    RJS_ScriptDecl *decl = env->script_decl;
    size_t          top  = rjs_value_stack_save(rt);
    RJS_Value      *tmp  = rjs_value_stack_push(rt);

    if (grp) {
        for (id = grp->binding_start; id < grp->binding_start + grp->binding_num; id ++) {
            RJS_ScriptBinding    *sb;
            RJS_ScriptBindingRef *sbr;

            sb  = &script->binding_table[id];
            sbr = &script->binding_ref_table[decl->binding_ref_start + sb->ref_idx];

            if (sb->flags & RJS_SCRIPT_BINDING_FL_CONST) {
                RJS_Bool strict = (sb->flags & RJS_SCRIPT_BINDING_FL_STRICT) ? RJS_TRUE : RJS_FALSE;

                rjs_env_create_immutable_binding(rt, env, &sbr->binding_name, strict);
            } else {
                rjs_env_create_mutable_binding(rt, env, &sbr->binding_name, RJS_FALSE);
            }

            if ((r = rjs_env_get_binding_value(rt, src, &sbr->binding_name, RJS_TRUE, tmp)) == RJS_ERR)
                goto end;

            rjs_env_initialize_binding(rt, env, &sbr->binding_name, tmp);
        }
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Initialize the function group.
 * \param rt The current runtime.
 * \param script The script.
 * \param grp The function group.
 * \param is_top The function is in top level of the container function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_script_func_group_init (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptFuncDeclGroup *grp, RJS_Bool is_top)
{
    size_t           id;
    RJS_Result       r;
    RJS_ScriptDecl  *decl;
    RJS_Environment *decl_env;
    RJS_Environment *bot_env = rjs_lex_env_running(rt);
    size_t           top     = rjs_value_stack_save(rt);
    RJS_Value       *tmp     = rjs_value_stack_push(rt);
    RJS_PrivateEnv  *priv    = NULL;

    if (is_top) {
        RJS_ScriptContext *sc = (RJS_ScriptContext*)rjs_context_running(rt);
        RJS_Environment   *ve = sc->scb.var_env;

        decl_env = ve;
    } else {
        decl_env = bot_env;
    }

    decl = decl_env->script_decl;

#if ENABLE_PRIV_NAME
    priv = rjs_private_env_running(rt);
#endif /*ENABLE_PRIV_NAME*/

    for (id = grp->func_decl_start; id < grp->func_decl_start + grp->func_decl_num; id ++) {
        RJS_ScriptFuncDecl   *sfd;
        RJS_ScriptFunc       *sf;
        RJS_ScriptBindingRef *sbr;

        sfd = &script->func_decl_table[id];
        sf  = &script->func_table[sfd->func_idx];
        sbr = &script->binding_ref_table[decl->binding_ref_start + sfd->binding_ref_idx];

        r = rjs_create_function(rt, script, sf, bot_env, priv, RJS_TRUE, tmp);
        if (r == RJS_ERR)
            goto end;

        if (is_top)
            rjs_env_set_mutable_binding(rt, decl_env, &sbr->binding_name, tmp, RJS_FALSE);
        else
            rjs_env_initialize_binding(rt, decl_env, &sbr->binding_name, tmp);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Print the value in the script's value table.
 * \param rt The current runtime.
 * \param script The script.
 * \param fp The output file.
 * \param v The value pointer.
 */
void
rjs_script_print_value_pointer (RJS_Runtime *rt, RJS_Script *script, FILE *fp, RJS_Value *v)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);

    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_NULL:
        fprintf(fp, "null");
        break;
    case RJS_VALUE_UNDEFINED:
        fprintf(fp, "undefined");
        break;
    case RJS_VALUE_BOOLEAN:
        fprintf(fp, "%s", rjs_value_get_boolean(rt, v) ? "true" : "false");
        break;
    case RJS_VALUE_NUMBER:
        fprintf(fp, "%g", rjs_value_get_number(rt, v));
        break;
    case RJS_VALUE_STRING:
        fprintf(fp, "%s", rjs_string_to_enc_chars(rt, v, NULL, NULL));
        break;
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
        rjs_big_int_to_string(rt, v, 10, str);
        fprintf(fp, "%sn", rjs_string_to_enc_chars(rt, str, NULL, NULL));
        break;
#endif /*ENABLE_BIG_INT*/
    case RJS_VALUE_OBJECT:
        switch (rjs_value_get_gc_thing_type(rt, v)) {
        case RJS_GC_THING_ARRAY: {
            int64_t i, len;

            rjs_length_of_array_like(rt, v, &len);
            for (i = 0; i < len; i ++) {
                rjs_get_index(rt, v, i, str);
                fprintf(fp, "%s", rjs_string_to_enc_chars(rt, str, NULL, NULL));

                if (i != len - 1)
                    fprintf(fp, "${}");
            }

            break;
        }
        case RJS_GC_THING_REGEXP:
            rjs_to_string(rt, v, str);
            fprintf(fp, "%s", rjs_string_to_enc_chars(rt, str, NULL, NULL));
            break;
        default:
            break;
        }
        break;
    case RJS_VALUE_GC_THING:
        switch (rjs_value_get_gc_thing_type(rt, v)) {
#if ENABLE_PRIV_NAME
        case RJS_GC_THING_PRIVATE_NAME: {
            RJS_PrivateName *pn;

            pn = rjs_value_get_gc_thing(rt, v);

            fprintf(fp, "%s", rjs_string_to_enc_chars(rt, &pn->description, NULL, NULL));
            break;
        }
#endif /*ENABLE_PRIV_NAME*/
        default:
            break;
        }
        break;
    default:
        break;
    }

    rjs_value_stack_restore(rt, top);
}

/**
 * Print the value in the script's value table.
 * \param rt The current runtime.
 * \param script The script.
 * \param fp The output file.
 * \param id The value's index.
 */
void
rjs_script_print_value (RJS_Runtime *rt, RJS_Script *script, FILE *fp, int id)
{
    RJS_Value *v;

    v = &script->value_table[id];

    rjs_script_print_value_pointer(rt, script, fp, v);
}

/**
 * Disassemble the script.
 * \param rt The current runtime.
 * \param v The script value.
 * \param fp The output file.
 * \param flags The flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_script_disassemble (RJS_Runtime *rt, RJS_Value *v, FILE *fp, int flags)
{
    RJS_Script *script;
    int         i;

    assert(rjs_value_is_script(rt, v));
    assert(fp);

    script = rjs_value_get_gc_thing(rt, v);

    if ((flags & RJS_DISASSEMBLE_VALUE) && script->value_num) {
        fprintf(fp, "value:\n");

        for (i = 0; i < script->value_num; i ++) {
            fprintf(fp, "  %d: ", i);
            rjs_script_print_value(rt, script, fp, i);
            fprintf(fp, "\n");
        }
    }

    if ((flags & RJS_DISASSEMBLE_DECL) && script->decl_num) {
        fprintf(fp, "declaration:\n");

        for (i = 0; i < script->decl_num; i ++) {
            RJS_ScriptDecl *decl;
            int             j;

            decl = &script->decl_table[i];

            fprintf(fp, "  declaration %d:\n", i);

            for (j = 0; j < decl->binding_ref_num; j ++) {
                RJS_ScriptBindingRef *br;

                br = script->binding_ref_table + decl->binding_ref_start + j;

                fprintf(fp, "    %d: ", j);
                rjs_script_print_value_pointer(rt, script, fp, br->binding_name.name);
                fprintf(fp, "\n");
            }
        }
    }

    if ((flags & RJS_DISASSEMBLE_BINDING) && script->binding_group_num) {
        fprintf(fp, "binding group:\n");

        for (i = 0; i < script->binding_group_num; i ++) {
            RJS_ScriptBindingGroup *bg;
            int                     j;

            fprintf(fp, "  group %d:\n", i);

            bg = &script->binding_group_table[i];
            for (j = 0; j < bg->binding_num; j ++) {
                RJS_ScriptBinding    *b;
                RJS_ScriptDecl       *decl;
                RJS_ScriptBindingRef *br;

                b    = script->binding_table + bg->binding_start + j;
                decl = &script->decl_table[bg->decl_idx];
                br   = &script->binding_ref_table[decl->binding_ref_start + b->ref_idx];

                fprintf(fp, "    ");
                rjs_script_print_value_pointer(rt, script, fp, br->binding_name.name);

                if (b->flags & RJS_SCRIPT_BINDING_FL_CONST)
                    fprintf(fp, " const");
                if (b->flags & RJS_SCRIPT_BINDING_FL_UNDEF)
                    fprintf(fp, " var");
                if (b->flags & RJS_SCRIPT_BINDING_FL_BOT)
                    fprintf(fp, " bottom");

                fprintf(fp, "\n");
            }
        }
    }

    if ((flags & RJS_DISASSEMBLE_FUNC_DECL) && script->func_decl_group_num) {
        fprintf(fp, "function declaration:\n");

        for (i = 0; i < script->func_decl_group_num; i ++) {
            RJS_ScriptFuncDeclGroup *fdg;
            int                      j;

            fdg = &script->func_decl_group_table[i];

            fprintf(fp, "  group %d:\n", i);

            for (j = 0; j < fdg->func_decl_num; j ++) {
                RJS_ScriptFuncDecl   *fd;
                RJS_ScriptDecl       *decl;
                RJS_ScriptBindingRef *br;

                fd   = script->func_decl_table + fdg->func_decl_start + j;
                decl = &script->decl_table[fdg->decl_idx];
                br   = &script->binding_ref_table[decl->binding_ref_start + fd->binding_ref_idx];

                fprintf(fp, "    %d: ", fd->func_idx);
                rjs_script_print_value_pointer(rt, script, fp, br->binding_name.name);
                fprintf(fp, "\n");
            }
        }
    }

#if ENABLE_PRIV_NAME
    if ((flags & RJS_DISASSEMBLE_PRIV_ENV) && script->priv_env_num) {
        fprintf(fp, "private environment:\n");

        for (i = 0; i < script->priv_env_num; i ++) {
            RJS_ScriptPrivEnv *env = &script->priv_env_table[i];
            int                j;

            fprintf(fp, "  environment %d: ", i);
            for (j = env->priv_id_start; j < env->priv_id_start + env->priv_id_num; j ++) {
                RJS_ScriptPrivId *pid = &script->priv_id_table[j];

                rjs_script_print_value(rt, script, fp, pid->idx);
            }
            fprintf(fp, "\n");
        }
    }
#endif /*ENABLE_PRIV_NAME*/

    if (flags & RJS_DISASSEMBLE_FUNC) {
        for (i = 0; i < script->func_num; i ++) {
            RJS_ScriptFunc *func;

            func = &script->func_table[i];

            rjs_function_disassemble(rt, v, func, fp, flags);
        }
    }

    return RJS_OK;
}

/**
 * Call the script function.
 * \param rt The current runtime.
 * \param type The call type.
 * \param v The input value.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 * \retval RJS_FALSE The async function is waiting a promise.
 */
RJS_Result
rjs_script_func_call (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *v, RJS_Value *rv)
{
    RJS_Context       *ctxt = rjs_context_running(rt);
    RJS_ScriptContext *sc   = (RJS_ScriptContext*)ctxt;
#if ENABLE_GENERATOR
    RJS_Generator     *g;
#endif /*ENABLE_GENERATOR*/
    RJS_Result         r;

    if (!rv)
        rv = rjs_value_stack_push(rt);

    /*Call the script.*/
    r = rjs_bc_call(rt, type, v, rv);

    if (r != RJS_FALSE) {
        int flags = sc->script_func->flags;

#if ENABLE_GENERATOR
        /*Is generator?*/
        if ((flags & RJS_FUNC_FL_GENERATOR) && (type != RJS_SCRIPT_CALL_SYNC_START)) {
            g = (RJS_Generator*)rjs_value_get_object(rt, &ctxt->function);

            g->state = RJS_GENERATOR_STATE_COMPLETED;

#if ENABLE_ASYNC
            if (flags & RJS_FUNC_FL_ASYNC) {
                RJS_GeneratorRequestType req_type;

                if (r == RJS_OK) {
                    req_type = RJS_GENERATOR_REQUEST_NEXT;
                } else {
                    req_type = RJS_GENERATOR_REQUEST_THROW;
                    rjs_value_copy(rt, rv, &rt->error);
                }

                rjs_async_generator_complete_step(rt, &ctxt->function, req_type, rv, RJS_TRUE, NULL);
                rjs_async_generator_drain_queue(rt, &ctxt->function);

                rjs_value_set_undefined(rt, rv);
            } else
#endif /*ENABLE_ASYNC*/
            {
                if (r == RJS_OK) {
                    RJS_Value *tmp = rjs_value_stack_push(rt);

                    rjs_create_iter_result_object(rt, rv, RJS_TRUE, tmp);
                    rjs_value_copy(rt, rv, tmp);
                }
            }
        }
#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC
        /*Is async function?*/
        if (flags & RJS_FUNC_FL_ASYNC) {
            RJS_AsyncContext *ac = (RJS_AsyncContext*)ctxt;

            if (!rjs_value_is_undefined(rt, ac->capability.promise)) {
                if (r == RJS_OK)
                    rjs_call(rt, ac->capability.resolve, rjs_v_undefined(rt), rv, 1, NULL);
                else
                    rjs_call(rt, ac->capability.reject, rjs_v_undefined(rt), &rt->error, 1, NULL);
            }
        }
#endif /*ENABLE_ASYNC*/
    }

    return r;
}
