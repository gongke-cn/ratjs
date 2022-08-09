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

/*Proxy*/
static RJS_NF(Proxy_constructor)
{
    RJS_Value *target  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *handler = rjs_argument_get(rt, args, argc, 1);

    if (!nt)
        return rjs_throw_type_error(rt, _("\"Proxy\" must be used as a constructor"));

    return rjs_proxy_object_new(rt, rv, target, handler);
}

static const RJS_BuiltinFuncDesc
proxy_constructor_desc = {
    "Proxy",
    2,
    Proxy_constructor
};

/**Revoke function.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;   /**< Base built-in function object data.*/
    RJS_Value             proxy; /**< Proxy object.*/
} RJS_RevokeFunc;

/*Scan referenced things in the revoke function.*/
static void
revoke_func_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_RevokeFunc *rf = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, &rf->bfo);
    rjs_gc_scan_value(rt, &rf->proxy);
}

/*Free the revoke function.*/
static void
revoke_func_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_RevokeFunc *rf = ptr;

    rjs_builtin_func_object_deinit(rt, &rf->bfo);

    RJS_DEL(rt, rf);
}

/*Revoke function operation functions.*/
static const RJS_ObjectOps
revoke_func_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        revoke_func_op_gc_scan,
        revoke_func_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/*Revoke the proxy.*/
static RJS_NF(revoke_proxy)
{
    RJS_RevokeFunc  *rf = (RJS_RevokeFunc*)rjs_value_get_object(rt, f);
    RJS_ProxyObject *po;

    if (rjs_value_is_null(rt, &rf->proxy)) {
        rjs_value_set_undefined(rt, rv);
        return RJS_OK;
    }

    po = (RJS_ProxyObject*)rjs_value_get_object(rt, &rf->proxy);

    rjs_value_set_null(rt, &po->target);
    rjs_value_set_null(rt, &po->handler);
    rjs_value_set_null(rt, &rf->proxy);

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/*Proxy.revocable*/
static RJS_NF(Proxy_revocable)
{
    RJS_Value      *target  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value      *handler = rjs_argument_get(rt, args, argc, 1);
    RJS_Realm      *realm   = rjs_realm_current(rt);
    size_t          top     = rjs_value_stack_save(rt);
    RJS_Value      *p       = rjs_value_stack_push(rt);
    RJS_Value      *func    = rjs_value_stack_push(rt);
    RJS_RevokeFunc *rf;
    RJS_Result      r;

    if ((r = rjs_proxy_object_new(rt, p, target, handler)) == RJS_ERR)
        goto end;

    RJS_NEW(rt, rf);

    rjs_value_copy(rt, &rf->proxy, p);

    if ((r = rjs_init_builtin_function(rt, &rf->bfo, revoke_proxy, 0, &revoke_func_ops, 0,
            rjs_s_empty(rt), realm, NULL, NULL, NULL, func)) == RJS_ERR) {
        RJS_DEL(rt, rf);
        goto end;
    }

    rjs_ordinary_object_create(rt, NULL, rv);
    rjs_create_data_property_or_throw(rt, rv, rjs_pn_proxy(rt), p);
    rjs_create_data_property_or_throw(rt, rv, rjs_pn_revoke(rt), func);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
proxy_function_descs[] = {
    {
        "revocable",
        2,
        Proxy_revocable
    },
    {NULL}
};
