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

/*Scan the reference things in the private name.*/
static void
private_name_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PrivateName *pn = ptr;

    rjs_gc_scan_value(rt, &pn->description);
}

/*Free the private name.*/
static void
private_name_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PrivateName *pn = ptr;

    RJS_DEL(rt, pn);
}

/*Private name GC operation functions.*/
static const RJS_GcThingOps
private_name_gc_ops = {
    RJS_GC_THING_PRIVATE_NAME,
    private_name_op_gc_scan,
    private_name_op_gc_free
};

/*Scan the reference things in the private environment.*/
static void
private_env_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_PrivateEnv       *pe = ptr;
    RJS_PrivateNameEntry *pne;
    size_t                i;

    if (pe->outer)
        rjs_gc_mark(rt, pe->outer);

    rjs_hash_foreach_c(&pe->priv_name_hash, i, pne, RJS_PrivateNameEntry, he) {
        rjs_gc_scan_value(rt, &pne->priv_name);
    }
}

/*Free the private environment.*/
static void
private_env_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_PrivateEnv       *pe = ptr;
    RJS_PrivateNameEntry *pne, *npne;
    size_t                i;

    rjs_hash_foreach_safe_c(&pe->priv_name_hash, i, pne, npne, RJS_PrivateNameEntry, he) {
        RJS_DEL(rt, pne);
    }

    rjs_hash_deinit(&pe->priv_name_hash, &rjs_hash_string_ops, rt);

    RJS_DEL(rt, pe);
}

/*Private environment GC operation functions.*/
static const RJS_GcThingOps
private_env_gc_ops = {
    RJS_GC_THING_PRIVATE_ENV,
    private_env_op_gc_scan,
    private_env_op_gc_free
};

/*Convert the private name to NULL terminated characters.*/
static const char*
private_name_to_chars (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_PrivateName *pn = rjs_value_get_gc_thing(rt, v);

    return rjs_string_to_enc_chars(rt, &pn->description, NULL, NULL);
}

/*Add a new private name to the environment.*/
static RJS_Result
private_name_add (RJS_Runtime *rt, RJS_PrivateEnv *env, RJS_Value *desc)
{
    RJS_String           *str;
    RJS_HashEntry        *he, **phe;
    RJS_PrivateNameEntry *pne;
    RJS_PrivateName      *pn;
    RJS_Result            r;

    str = rjs_value_get_string(rt, desc);

    r = rjs_hash_lookup(&env->priv_name_hash, str, &he, &phe, &rjs_hash_string_ops, rt);
    if (r)
        return RJS_OK;

    RJS_NEW(rt, pne);

    rjs_value_set_undefined(rt, &pne->priv_name);

    rjs_hash_insert(&env->priv_name_hash, str, &pne->he, phe, &rjs_hash_string_ops, rt);

    RJS_NEW(rt, pn);

    rjs_value_copy(rt, &pn->description, desc);

    rjs_value_set_gc_thing(rt, &pne->priv_name, pn);
    rjs_gc_add(rt, pn, &private_name_gc_ops);

    return RJS_OK;
}

/**
 * Push a new private environment.
 * \param rt The current runtime.
 * \param script The script.
 * \param spe The script private environment record.
 * \return The new private environment.
 */
RJS_PrivateEnv*
rjs_private_env_push (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptPrivEnv *spe)
{
    RJS_ScriptContext *sc = (RJS_ScriptContext*)rjs_context_running(rt);
    RJS_PrivateEnv    *env;

    /*Push a new environment.*/
    RJS_NEW(rt, env);

    env->outer = sc->scb.priv_env;

    rjs_hash_init(&env->priv_name_hash);

    sc->scb.priv_env = env;

    rjs_gc_add(rt, env, &private_env_gc_ops);

    /*Add the private names.*/
    if (spe) {
        RJS_ScriptPrivId *pid, *pid_end;

        pid     = script->priv_id_table + spe->priv_id_start;
        pid_end = pid + spe->priv_id_num;

        while (pid < pid_end) {
            RJS_Value *desc = &script->value_table[pid->idx];

            private_name_add(rt, env, desc);

            pid ++;
        }
    }

    return env;
}

/**
 * Popup the top private environment.
 * \param rt The current runtime.
 * \param env The private environment to be popped.
 */
void
rjs_private_env_pop (RJS_Runtime *rt, RJS_PrivateEnv *env)
{
    RJS_ScriptContext *sc = (RJS_ScriptContext*)rjs_context_running(rt);

    if (env == sc->scb.priv_env)
        sc->scb.priv_env = env->outer;
}

/**
 * Lookup the private name.
 * \param rt The current runtime.
 * \param id The identifier of the private name.
 * \param env The private environment.
 * \param[out] pn Return the private name.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot find the private name.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_private_name_lookup (RJS_Runtime *rt, RJS_Value *id, RJS_PrivateEnv *env, RJS_Value *pn)
{
    RJS_String           *str;
    RJS_HashEntry        *he;
    RJS_PrivateNameEntry *pne;
    RJS_Result            r;

    str = rjs_value_get_string(rt, id);

    for (; env; env = env->outer) {
        r = rjs_hash_lookup(&env->priv_name_hash, str, &he, NULL, &rjs_hash_string_ops, rt);
        if (r) {

            if (pn) {
                pne = RJS_CONTAINER_OF(he, RJS_PrivateNameEntry, he);

                rjs_value_copy(rt, pn, &pne->priv_name);
            }

            return RJS_TRUE;
        }
    }

    return RJS_FALSE;
}

/*Find the private name entry.*/
static RJS_PropertyNode*
private_element_find (RJS_Runtime *rt, RJS_Value *v, RJS_Value *p, RJS_HashEntry ***phe)
{
    RJS_Object       *o;
    RJS_PrivateName  *pn;
    RJS_HashEntry    *he;
    RJS_Result        r;

    assert(rjs_value_is_object(rt, v));
    assert(rjs_value_is_private_name(rt, p));

    o  = rjs_value_get_object(rt, v);
    pn = (RJS_PrivateName*)rjs_value_get_gc_thing(rt, p);

    r = rjs_hash_lookup(&o->prop_hash, pn, &he, phe, &rjs_hash_size_ops, rt);
    if (!r)
        return NULL;

    return RJS_CONTAINER_OF(he, RJS_PropertyNode, he);
}

/*Add a private element.*/
static void
private_element_add (RJS_Runtime *rt, RJS_Value *v, RJS_Value *p, RJS_PropertyNode *prop, RJS_HashEntry **phe)
{
    RJS_Object       *o;
    RJS_PrivateName  *pn;

    o  = rjs_value_get_object(rt, v);
    pn = (RJS_PrivateName*)rjs_value_get_gc_thing(rt, p);

    rjs_hash_insert(&o->prop_hash, pn, &prop->he, phe, &rjs_hash_size_ops, rt);
    rjs_list_append(&o->prop_list, &prop->ln);
}

/**
 * Get the object's private property's value.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The private property name.
 * \param[out] pv Return the private property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_private_get (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *pv)
{
    RJS_PropertyNode *prop;
    RJS_Result        r;
    RJS_PrivateEnv   *pe   = rjs_private_env_running(rt);
    size_t            top  = rjs_value_stack_save(rt);
    RJS_Value        *name = rjs_value_stack_push(rt);
    RJS_Value        *o    = rjs_value_stack_push(rt);

    if ((r = rjs_to_object(rt, v, o)) == RJS_ERR)
        goto end;

    rjs_private_name_lookup(rt, pn->name, pe, name);

    prop = private_element_find(rt, o, name, NULL);
    if (!prop) {
        r = rjs_throw_type_error(rt, _("cannot find the private element \"%s\""),
                private_name_to_chars(rt, name));
        goto end;
    }

    if (!(prop->prop.attrs & RJS_PROP_ATTR_ACCESSOR)) {
        rjs_value_copy(rt, pv, &prop->prop.p.value);
        r = RJS_OK;
        goto end;
    }

    if (rjs_value_is_undefined(rt, &prop->prop.p.a.get)) {
        r = rjs_throw_type_error(rt, _("get function of \"%s\" is not defined"),
                private_name_to_chars(rt, name));
        goto end;
    }

    r = rjs_call(rt, &prop->prop.p.a.get, o, NULL, 0, pv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the object's private property's value.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The private property name.
 * \param pv The private property's new value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_private_set (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *pv)
{
    RJS_PropertyNode *prop;
    RJS_Result        r;
    RJS_PrivateEnv   *pe   = rjs_private_env_running(rt);
    size_t            top  = rjs_value_stack_save(rt);
    RJS_Value        *name = rjs_value_stack_push(rt);
    RJS_Value        *o    = rjs_value_stack_push(rt);

    if ((r = rjs_to_object(rt, v, o)) == RJS_ERR)
        goto end;

    rjs_private_name_lookup(rt, pn->name, pe, name);

    prop = private_element_find(rt, o, name, NULL);
    if (!prop) {
        r = rjs_throw_type_error(rt, _("cannot find the private element \"%s\""),
                private_name_to_chars(rt, name));
        goto end;
    }

    if (!(prop->prop.attrs & (RJS_PROP_ATTR_ACCESSOR|RJS_PROP_ATTR_METHOD))) {
        rjs_value_copy(rt, &prop->prop.p.value, pv);
        r = RJS_OK;
    } else if (prop->prop.attrs & RJS_PROP_ATTR_METHOD) {
        r = rjs_throw_type_error(rt, _("private method \"%s\" cannot be reset"),
                private_name_to_chars(rt, name));
    } else {
        if (rjs_value_is_undefined(rt, &prop->prop.p.a.set)) {
            r = rjs_throw_type_error(rt, _("set function of \"%s\" is not defined"),
                    private_name_to_chars(rt, name));
            goto end;
        }

        r = rjs_call(rt, &prop->prop.p.a.set, o, pv, 1, NULL);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Add a private field.
 * \param rt The current runtime.
 * \param v The object value.
 * \param p The private name.
 * \param pv The field's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_private_field_add (RJS_Runtime *rt, RJS_Value *v, RJS_Value *p, RJS_Value *pv)
{
    RJS_PropertyNode  *prop;
    RJS_HashEntry    **phe;

    prop = private_element_find(rt, v, p, &phe);
    if (prop)
        return rjs_throw_type_error(rt, _("cannot find the private element \"%s\""),
                private_name_to_chars(rt, p));

    RJS_NEW(rt, prop);

    prop->prop.attrs = RJS_PROP_ATTR_WRITABLE;
    rjs_value_copy(rt, &prop->prop.p.value, pv);

    private_element_add(rt, v, p, prop, phe);

    return RJS_OK;
}

/**
 * Add a private method.
 * \param rt The current runtime.
 * \param v The object value.
 * \param p The private name.
 * \param pv The method's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_private_method_add (RJS_Runtime *rt, RJS_Value *v, RJS_Value *p, RJS_Value *pv)
{
    RJS_PropertyNode  *prop;
    RJS_HashEntry    **phe;

    prop = private_element_find(rt, v, p, &phe);
    if (prop)
        return rjs_throw_type_error(rt, _("cannot find the private element \"%s\""),
                private_name_to_chars(rt, p));

    RJS_NEW(rt, prop);

    prop->prop.attrs = RJS_PROP_ATTR_METHOD;
    rjs_value_copy(rt, &prop->prop.p.value, pv);

    private_element_add(rt, v, p, prop, phe);

    return RJS_OK;
}

/**
 * Add a private accessor.
 * \param rt The current runtime.
 * \param v The object value.
 * \param p The private name.
 * \param get The getter of the accessor.
 * \param set The setter of the accessor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_private_accessor_add (RJS_Runtime *rt, RJS_Value *v, RJS_Value *p, RJS_Value *get, RJS_Value *set)
{
    RJS_PropertyNode  *prop;
    RJS_HashEntry    **phe;

    prop = private_element_find(rt, v, p, &phe);
    if (prop) {
        if (!(prop->prop.attrs & RJS_PROP_ATTR_ACCESSOR))
            return rjs_throw_type_error(rt, _("private element \"%s\" is not an accessor"),
                    private_name_to_chars(rt, p));

        if (!rjs_value_is_undefined(rt, &prop->prop.p.a.get)
                && get
                && !rjs_value_is_undefined(rt, get))
            return rjs_throw_type_error(rt, _("get function of \"%s\" is already declared"),
                    private_name_to_chars(rt, p));

        if (!rjs_value_is_undefined(rt, &prop->prop.p.a.set)
                && set
                && !rjs_value_is_undefined(rt, set))
            return rjs_throw_type_error(rt, _("set function of \"%s\" is already declared"),
                    private_name_to_chars(rt, p));

        if (get && !rjs_value_is_undefined(rt, get))
            rjs_value_copy(rt, &prop->prop.p.a.get, get);

        if (set && !rjs_value_is_undefined(rt, set))
            rjs_value_copy(rt, &prop->prop.p.a.set, set);

        return RJS_OK;
    }

    RJS_NEW(rt, prop);

    prop->prop.attrs = RJS_PROP_ATTR_ACCESSOR;
    rjs_value_set_undefined(rt, &prop->prop.p.a.get);
    rjs_value_set_undefined(rt, &prop->prop.p.a.set);

    if (get && !rjs_value_is_undefined(rt, get))
        rjs_value_copy(rt, &prop->prop.p.a.get, get);
    if (set && !rjs_value_is_undefined(rt, set))
        rjs_value_copy(rt, &prop->prop.p.a.set, set);

    private_element_add(rt, v, p, prop, phe);

    return RJS_OK;
}

/**
 * Find a private element in an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param p The private identifier.
 * \retval RJS_TRUE Find the private element.
 * \retval RJS_FALSE Cannot find the private element.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_private_element_find (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p)
{
    RJS_PrivateEnv   *pe   = rjs_private_env_running(rt);
    size_t            top  = rjs_value_stack_save(rt);
    RJS_Value        *name = rjs_value_stack_push(rt);
    RJS_PropertyNode *prop;
    RJS_Result        r;

    if (!rjs_value_is_object(rt, o)) {
        r = rjs_throw_type_error(rt, _("the value is not an object"));
        goto end;
    }

    rjs_private_name_lookup(rt, p, pe, name);

    prop = private_element_find(rt, o, name, NULL);
    r    = prop ? RJS_TRUE : RJS_FALSE;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}
