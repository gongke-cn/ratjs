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

/*Scan the reference things in the object environment.*/
static void
object_env_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_ObjectEnv *oe = ptr;

    if (oe->env.outer)
        rjs_gc_mark(rt, oe->env.outer);

    rjs_gc_scan_value(rt, &oe->object);
}

/*Free the object environment.*/
static void
object_env_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_ObjectEnv *oe = ptr;

    RJS_DEL(rt, oe);
}

/*Check if the object environment has the binding.*/
static RJS_Result
object_env_op_has_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_ObjectEnv   *oe          = (RJS_ObjectEnv*)env;
    size_t           top         = rjs_value_stack_save(rt);
    RJS_Value       *unscopables = rjs_value_stack_push(rt);
    RJS_Value       *blocked     = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_name_init(rt, &pn, n->name);

    if ((r = rjs_object_has_property(rt, &oe->object, &pn)) == RJS_ERR)
        goto end;
    if (!r)
        goto end;

    if (!oe->is_with)
        goto end;

    if ((r = rjs_get(rt, &oe->object, rjs_pn_s_unscopables(rt), unscopables)) == RJS_ERR)
        goto end;

    if (rjs_value_is_object(rt, unscopables)) {
        if ((r = rjs_get(rt, unscopables, &pn, blocked)) == RJS_ERR)
            goto end;

        if (rjs_to_boolean(rt, blocked)) {
            r = RJS_FALSE;
            goto end;
        }
    }

    r = RJS_TRUE;
end:
    rjs_property_name_deinit(rt, &pn);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create a mutable binding in an object environment.*/
static RJS_Result
object_env_op_create_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool del)
{
    RJS_ObjectEnv   *oe  = (RJS_ObjectEnv*)env;
    size_t           top = rjs_value_stack_save(rt);
    RJS_PropertyDesc pd;
    RJS_Result       r;
    RJS_PropertyName pn;

    rjs_property_desc_init(rt, &pd);
    rjs_property_name_init(rt, &pn, n->name);

    pd.flags = RJS_PROP_FL_DATA\
            |RJS_PROP_FL_WRITABLE
            |RJS_PROP_FL_ENUMERABLE;

    if (del)
        pd.flags |= RJS_PROP_FL_CONFIGURABLE;

    r = rjs_define_property_or_throw(rt, &oe->object, &pn, &pd);

    rjs_property_name_deinit(rt, &pn);
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create an immutable binding in an object environment.*/
static RJS_Result
object_env_op_create_immutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict)
{
    assert(0);
    return RJS_OK;
}

/*Initialize the binding in an object environment.*/
static RJS_Result
object_env_op_initialize_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v)
{
    return rjs_env_set_mutable_binding(rt, env, n, v, RJS_FALSE);
}

/*Set the mutable binding in an object environment.*/
static RJS_Result
object_env_op_set_mutable_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v, RJS_Bool strict)
{
    RJS_ObjectEnv   *oe = (RJS_ObjectEnv*)env;
    RJS_Result       r;
    RJS_PropertyName pn;

    rjs_property_name_init(rt, &pn, n->name);

    if ((r = rjs_object_has_property(rt, &oe->object, &pn)) == RJS_ERR)
        goto end;

    if (!r && strict) {
        r = rjs_throw_reference_error(rt, _("property \"%s\" is not defined"),
                rjs_string_to_enc_chars(rt, n->name, NULL, NULL));
        goto end;
    }

    r = rjs_set(rt, &oe->object, &pn, v, strict);
end:
    rjs_property_name_deinit(rt, &pn);
    return r;
}

/*Get the binding's value in an object environment.*/
static RJS_Result
object_env_op_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict, RJS_Value *v)
{
    RJS_ObjectEnv   *oe = (RJS_ObjectEnv*)env;
    RJS_Result       r;
    RJS_PropertyName pn;

    rjs_property_name_init(rt, &pn, n->name);

    if ((r = rjs_object_has_property(rt, &oe->object, &pn)) == RJS_ERR)
        goto end;

    if (!r) {
        if (!strict) {
            rjs_value_set_undefined(rt, v);
            r = RJS_OK;
            goto end;
        }

        r = rjs_throw_reference_error(rt, _("property \"%s\" is not defined"),
                rjs_string_to_enc_chars(rt, n->name, NULL, NULL));
        goto end;
    }

    r = rjs_get(rt, &oe->object, &pn, v);
end:
    rjs_property_name_deinit(rt, &pn);
    return r;
}

/*Delete a binding in an object environment.*/
static RJS_Result
object_env_op_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n)
{
    RJS_ObjectEnv   *oe = (RJS_ObjectEnv*)env;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_name_init(rt, &pn, n->name);
    r = rjs_object_delete(rt, &oe->object, &pn);
    rjs_property_name_deinit(rt, &pn);

    return r;
}

/*Check if the object environment has this binding.*/
static RJS_Result
object_env_op_has_this_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    return RJS_FALSE;
}

/*Check if the object environemnt has the super binding.*/
static RJS_Result
object_env_op_has_super_binding (RJS_Runtime *rt, RJS_Environment *env)
{
    return RJS_FALSE;
}

/*Get base object of the object environment.*/
static RJS_Result
object_env_op_with_base_object (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *base)
{
    RJS_ObjectEnv *oe = (RJS_ObjectEnv*)env;

    if (oe->is_with) {
        rjs_value_copy(rt, base, &oe->object);
    } else {
        rjs_value_set_undefined(rt, base);
    }

    return RJS_OK;
}


/*Object environment operation functions.*/
static const RJS_EnvOps
object_env_ops = {
    {
        RJS_GC_THING_OBJECT_ENV,
        object_env_op_gc_scan,
        object_env_op_gc_free
    },
    object_env_op_has_binding,
    object_env_op_create_mutable_binding,
    object_env_op_create_immutable_binding,
    object_env_op_initialize_binding,
    object_env_op_set_mutable_binding,
    object_env_op_get_binding_value,
    object_env_op_delete_binding,
    object_env_op_has_this_binding,
    object_env_op_has_super_binding,
    object_env_op_with_base_object,
    NULL
};

/**
 * Create a new object environment.
 * \param rt The current runtime.
 * \param[out] pe Return the new environment.
 * \param o The object.
 * \param is_with Is with environment.
 * \param decl The script declaration.
 * \param outer The outer environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_env_new (RJS_Runtime *rt, RJS_Environment **pe, RJS_Value *o, RJS_Bool is_with,
        RJS_ScriptDecl *decl, RJS_Environment *outer)
{
    RJS_ObjectEnv *oe;

    RJS_NEW(rt, oe);

    oe->env.outer       = outer;
    oe->env.script_decl = decl;
    oe->is_with         = is_with;
    rjs_value_copy(rt, &oe->object, o);

    *pe = &oe->env;

    rjs_gc_add(rt, oe, &object_env_ops.gc_thing_ops);

    return RJS_OK;
}
