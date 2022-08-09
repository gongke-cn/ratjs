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

/**Weak reference object.*/
typedef struct {
    RJS_Object   object;   /**< Base object.*/
    RJS_WeakRef *weak_ref; /**< The weak reference.*/
    RJS_Value    target;   /**< The target object.*/
} RJS_WeakRefObject;

/*Scan the reference things in the weak reference object.*/
static void
weak_ref_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_WeakRefObject *wro = ptr;

    rjs_object_op_gc_scan(rt, &wro->object);
}

/*Free the weak reference object.*/
static void
weak_ref_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_WeakRefObject *wro = ptr;

    RJS_DEL(rt, wro);
}

/*Weak reference object operation functions.*/
static const RJS_ObjectOps
weak_ref_ops = {
    {
        RJS_GC_THING_WEAK_REF,
        weak_ref_op_gc_scan,
        weak_ref_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Weak reference on finalize function.*/
static void
weak_ref_on_final (RJS_Runtime *rt, RJS_WeakRef *wr)
{
    RJS_WeakRefObject *wro = (RJS_WeakRefObject*)rjs_value_get_object(rt, &wr->base);

    rjs_value_set_undefined(rt, &wro->target);

    wro->weak_ref = NULL;
}

/*WeakRef*/
static RJS_NF(WeakRef_constructor)
{
    RJS_Value         *target = rjs_argument_get(rt, args, argc, 0);
    RJS_WeakRefObject *wro;
    RJS_Result         r;

    if (!nt)
        return rjs_throw_type_error(rt, _("\"WeakRef\" must be used as a constructor"));

    if (!rjs_can_be_held_weakly(rt, target))
        return rjs_throw_type_error(rt, _("the value cannot be held weakly"));

    RJS_NEW(rt, wro);

    rjs_value_copy(rt, &wro->target, target);
    wro->weak_ref = NULL;

    r = rjs_ordinary_init_from_constructor(rt, &wro->object, nt, RJS_O_WeakRef_prototype, &weak_ref_ops, rv);
    if (r == RJS_ERR) {
        RJS_DEL(rt, wro);
        return r;
    }

    wro->weak_ref = rjs_weak_ref_add(rt, rv, target, weak_ref_on_final);
    return RJS_OK;
}

static const RJS_BuiltinFuncDesc
weak_ref_constructor_desc = {
    "WeakRef",
    1,
    WeakRef_constructor
};

static const RJS_BuiltinFieldDesc
weak_ref_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "WeakRef",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*WeakRef.prototype.deref*/
static RJS_NF(WeakRef_prototype_deref)
{
    RJS_Result         r;
    RJS_WeakRefObject *wro;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_REF) {
        r = rjs_throw_type_error(rt, _("the value is not a weak reference"));
        goto end;
    }

    wro = (RJS_WeakRefObject*)rjs_value_get_object(rt, thiz);

    rjs_value_copy(rt, rv, &wro->target);

    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinFuncDesc
weak_ref_prototype_function_descs[] = {
    {
        "deref",
        0,
        WeakRef_prototype_deref
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
weak_ref_prototype_desc = {
    "WeakRef",
    NULL,
    NULL,
    NULL,
    weak_ref_prototype_field_descs,
    weak_ref_prototype_function_descs,
    NULL,
    NULL,
    "WeakRef_prototype"
};