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
 * Initialize the finalization registry data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_finalization_registry_init (RJS_Runtime *rt)
{
    rjs_list_init(&rt->final_cb_list);
}

/**
 * Release tje finalization registry data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_finalization_registry_deinint (RJS_Runtime *rt)
{
    RJS_FinalizationCallback *fcb, *nfcb;

    rjs_list_foreach_safe_c(&rt->final_cb_list, fcb, nfcb, RJS_FinalizationCallback, ln) {
        RJS_DEL(rt, fcb);
    }
}

/**
 * Scan the referenced things in the finalization registry data.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_finalization_registry (RJS_Runtime *rt)
{
    RJS_FinalizationCallback *fcb;

    rjs_list_foreach_c(&rt->final_cb_list, fcb, RJS_FinalizationCallback, ln) {
        rjs_gc_scan_value(rt, &fcb->registry);
        rjs_gc_scan_value(rt, &fcb->held);

        if (!rjs_same_value(rt, &fcb->target, &fcb->token))
            rjs_gc_scan_value(rt, &fcb->token);
    }
}

/**Finalization.*/
typedef struct {
    RJS_Value func; /**< The function.*/
    RJS_Value held; /**< The held parameter.*/
} RJS_Finalization;

/**Finalization job function.*/
static void
finalization_func (RJS_Runtime *rt, void *data)
{
    RJS_Finalization *f = data;

    rjs_call(rt, &f->func, rjs_v_undefined(rt), &f->held, 1, NULL);
}

/**Finalization scan function.*/
static void
finalization_scan (RJS_Runtime *rt, void *data)
{
    RJS_Finalization *f = data;

    rjs_gc_scan_value(rt, &f->func);
    rjs_gc_scan_value(rt, &f->held);
}

/**Finalization free function.*/
static void
finalization_free (RJS_Runtime *rt, void *data)
{
    RJS_Finalization *f = data;

    RJS_DEL(rt, f);
}

/**
 * Solve the callback functions in the finalization registry.
 * \param rt The current runtime.
 */
void
rjs_solve_finalization_registry (RJS_Runtime *rt)
{
    RJS_FinalizationCallback *fcb, *nfcb;

    rjs_list_foreach_safe_c(&rt->final_cb_list, fcb, nfcb, RJS_FinalizationCallback, ln) {
        RJS_GcThing *gt;

        gt = rjs_value_get_gc_thing(rt, &fcb->target);

        if (!(gt->next_flags & RJS_GC_THING_FL_MARKED)) {
            RJS_FinalizationRegistry *fr;
            RJS_Finalization         *f;
            RJS_Realm                *realm = rjs_realm_current(rt);

            /*Enqueue the finalization function.*/
            fr = (RJS_FinalizationRegistry*)rjs_value_get_object(rt, &fcb->registry);

            RJS_NEW(rt, f);

            rjs_value_copy(rt, &f->func, &fr->func);
            rjs_value_copy(rt, &f->held, &fcb->held);

            rjs_job_enqueue(rt, finalization_func, realm, finalization_scan, finalization_free, f);

            /*Remove the finalization entry.*/
            if (!rjs_value_is_undefined(rt, &fcb->token)) {
                RJS_HashEntry *he, **phe;
                RJS_Result     r;

                r = rjs_hash_lookup(&fr->cb_hash, &fcb->token, &he, &phe, &rjs_hash_value_ops, rt);
                if (r) {
                    rjs_hash_remove(&fr->cb_hash, phe, rt);
                }
            }

            rjs_list_remove(&fcb->ln);

            RJS_DEL(rt, fcb);
        }
    }
}

/*Scan the reference things in the finalization registry.*/
static void
finalization_registry_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_FinalizationRegistry *fr = ptr;

    rjs_object_op_gc_scan(rt, &fr->object);

    rjs_gc_scan_value(rt, &fr->func);
}

/*Free the finalization registry.*/
static void
finalization_registry_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_FinalizationRegistry *fr = ptr;

    rjs_object_deinit(rt, &fr->object);

    rjs_hash_deinit(&fr->cb_hash, &rjs_hash_value_ops, rt);

    RJS_DEL(rt, fr);
}

/*Finalization registry operation functions.*/
static const RJS_ObjectOps
finalization_registry_ops = {
    {
        RJS_GC_THING_FINALIZATION_REGISTRY,
        finalization_registry_op_gc_scan,
        finalization_registry_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/**
 * Create a new finalization registry.
 * \param rt The current runtime.
 * \param[out] registry Return the new registry object.
 * \param nt The new target.
 * \param func The callback function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_finalization_registry_new (RJS_Runtime *rt, RJS_Value *registry, RJS_Value *nt, RJS_Value *func)
{
    RJS_FinalizationRegistry *fr;
    RJS_Result                r;
    
    if (!nt)
        return rjs_throw_type_error(rt, _("\"FinalizationRegistry\" must be used as a constructor"));

    if (!rjs_is_callable(rt, func))
        return rjs_throw_type_error(rt, _("the value is not a function"));

    RJS_NEW(rt, fr);

    rjs_value_copy(rt, &fr->func, func);
    rjs_hash_init(&fr->cb_hash);

    r = rjs_ordinary_init_from_constructor(rt, &fr->object, nt,
            RJS_O_FinalizationRegistry_prototype, &finalization_registry_ops, registry);
    if (r == RJS_ERR) {
        rjs_hash_deinit(&fr->cb_hash, &rjs_hash_value_ops, rt);
        RJS_DEL(rt, fr);
        return r;
    }

    return RJS_OK;
}

/**
 * Register a finalization callback.
 * \param rt The current runtime.
 * \param registry The registry.
 * \param target The target value.
 * \param held The held value.
 * \param token Unregister token.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_finalization_register (RJS_Runtime *rt, RJS_Value *registry, RJS_Value *target,
        RJS_Value *held, RJS_Value *token)
{
    RJS_FinalizationRegistry *fr;
    RJS_HashEntry            *he, **phe = NULL;
    RJS_FinalizationCallback *fcb = NULL;

    if (rjs_value_get_gc_thing_type(rt, registry) != RJS_GC_THING_FINALIZATION_REGISTRY)
        return rjs_throw_type_error(rt, _("the value is not a finalization registry"));

    if (!rjs_can_be_held_weakly(rt, target))
        return rjs_throw_type_error(rt, _("the value cannot be held weakly"));

    if (rjs_same_value(rt, target, held))
        return rjs_throw_type_error(rt, _("target and held cannot be the same value"));

    if (!rjs_can_be_held_weakly(rt, token)) {
        if (!rjs_value_is_undefined(rt, token))
            return rjs_throw_type_error(rt, _("unregister token cannot be held weakly"));
    }

    fr = (RJS_FinalizationRegistry*)rjs_value_get_object(rt, registry);

    if (!rjs_value_is_undefined(rt, token)) {
        RJS_Result r;

        r = rjs_hash_lookup(&fr->cb_hash, token, &he, &phe, &rjs_hash_value_ops, rt);

        if (r)
            fcb = RJS_CONTAINER_OF(he, RJS_FinalizationCallback, he);
    }

    if (!fcb) {
        RJS_NEW(rt, fcb);

        rjs_value_copy(rt, &fcb->token, token);

        rjs_list_append(&rt->final_cb_list, &fcb->ln);

        if (!rjs_value_is_undefined(rt, token))
            rjs_hash_insert(&fr->cb_hash, &fcb->token, &fcb->he, phe, &rjs_hash_value_ops, rt);
    }

    rjs_value_copy(rt, &fcb->registry, registry);
    rjs_value_copy(rt, &fcb->target, target);
    rjs_value_copy(rt, &fcb->held, held);
    rjs_value_copy(rt, &fcb->token, token);

    return RJS_OK;
}

/**
 * Unregister a finalization callback.
 * \param rt The current runtime.
 * \param token Unregister token.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot find the callback.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_finalization_unregister (RJS_Runtime *rt, RJS_Value *registry, RJS_Value *token)
{
    RJS_FinalizationRegistry *fr;
    RJS_FinalizationCallback *fcb;
    RJS_Result                r;
    RJS_HashEntry            *he, **phe;

    if (rjs_value_get_gc_thing_type(rt, registry) != RJS_GC_THING_FINALIZATION_REGISTRY)
        return rjs_throw_type_error(rt, _("the value is not a finalization registry"));

    if (!rjs_can_be_held_weakly(rt, token))
        return rjs_throw_type_error(rt, _("the unregister token cannot be held weakly"));

    fr = (RJS_FinalizationRegistry*)rjs_value_get_object(rt, registry);
    r  = rjs_hash_lookup(&fr->cb_hash, token, &he, &phe, &rjs_hash_value_ops, rt);
    if (!r)
        return RJS_FALSE;

    fcb = RJS_CONTAINER_OF(he, RJS_FinalizationCallback, he);

    rjs_hash_remove(&fr->cb_hash, phe, rt);
    rjs_list_remove(&fcb->ln);

    RJS_DEL(rt, fcb);
    return RJS_TRUE;
}
