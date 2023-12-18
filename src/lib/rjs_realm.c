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

/*Scan the referenced things in the realm.*/
static void
realm_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Realm *realm = ptr;

    if (realm->rb.global_env)
        rjs_gc_mark(rt, realm->rb.global_env);

    rjs_gc_scan_value(rt, &realm->rb.global_object);

    rjs_gc_scan_value_buffer(rt, realm->objects, RJS_O_MAX);
}

/*Free the realm.*/
static void
realm_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Realm *realm = ptr;

    RJS_DEL(rt, realm);
}

/*Realm operation functions.*/
static const RJS_GcThingOps
realm_ops = {
    RJS_GC_THING_REALM,
    realm_op_gc_scan,
    realm_op_gc_free
};

/*Function prototype native function.*/
static RJS_NF(function_prototype_func)
{
    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/**
 * Create a new realm.
 * \param rt The current runtime.
 * \param[out] v The value buffer store the realm.
 * \return The new realm.
 */
RJS_Realm*
rjs_realm_new (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Realm *realm;
    RJS_Realm *old_realm;

    RJS_NEW(rt, realm);

    realm->rb.global_env = NULL;
    rjs_value_set_undefined(rt, &realm->rb.global_object);
    rjs_value_buffer_fill_undefined(rt, realm->objects, RJS_O_MAX);

    rjs_value_set_gc_thing(rt, v, realm);
    rjs_gc_add(rt, realm, &realm_ops);

    old_realm        = rt->rb.bot_realm;
    rt->rb.bot_realm = realm;

    /*Create global object.*/
    rjs_ordinary_object_create(rt, rjs_v_null(rt), rjs_o_Object_prototype(realm));
    rjs_create_builtin_function(rt, NULL, function_prototype_func, 0, rjs_s_empty(rt), realm,
            rjs_o_Object_prototype(realm), NULL, rjs_o_Function_prototype(realm));
    rjs_ordinary_object_create(rt, NULL, &realm->rb.global_object);

    /*Create global environment.*/
    rjs_global_env_new(rt, &realm->rb.global_env, &realm->rb.global_object, &realm->rb.global_object);

    /*Initialize the global object.*/
    rjs_realm_global_object_init(rt, realm);
    rjs_realm_for_in_iterator_init(rt, realm);
    rjs_realm_error_init(rt, realm);

    rt->rb.bot_realm = old_realm;

    return realm;
}

/**
 * Get the "%IteratorPrototype%" object of the realm.
 * \param realm The realm.
 * \return "%IteratorPrototype%" value's pointer.
 */
RJS_Value*
rjs_realm_iterator_prototype (RJS_Realm *realm)
{
    return rjs_o_IteratorPrototype(realm);
}

/**
 * Get the "Function.prototype" object of the realm.
 * \param realm The realm.
 * \return "Function.prototype" value's pointer.
 */
RJS_Value*
rjs_realm_function_prototype (RJS_Realm *realm)
{
    return rjs_o_Function_prototype(realm);
}
