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

/*Free the weak set.*/
static void
weak_set_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    hash_op_gc_free(rt, ptr, sizeof(RJS_WeakSetEntry));
}

/*Weak set object operation functions.*/
static const RJS_ObjectOps
weak_set_ops = {
    {
        RJS_GC_THING_WEAK_SET,
        weak_hash_op_gc_scan,
        weak_set_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*WeakSet*/
static RJS_NF(WeakSet_constructor)
{
    RJS_Value *iterable = rjs_argument_get(rt, args, argc, 0);

    return set_new(rt, rv, nt, RJS_O_WeakSet_prototype, &weak_set_ops, iterable);
}

static const RJS_BuiltinFuncDesc
weak_set_constructor_desc = {
    "WeakSet",
    0,
    WeakSet_constructor
};

static const RJS_BuiltinFieldDesc
weak_set_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "WeakSet",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Weak set in finalize function.*/
static void
weak_set_on_final (RJS_Runtime *rt, RJS_WeakRef *wr)
{
    RJS_WeakSetEntry *wse;

    wse = (RJS_WeakSetEntry*)hash_delete(rt, &wr->base, &wr->ref);

    RJS_DEL(rt, wse);
}

/*WeakSet.prototype.add*/
static RJS_NF(WeakSet_prototype_add)
{
    RJS_Value        *v = rjs_argument_get(rt, args, argc, 0);
    RJS_WeakSetEntry *wse;
    RJS_Result        r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a weak set"));
        goto end;
    }

    if (!rjs_can_be_held_weakly(rt, v)) {
        r = rjs_throw_type_error(rt, _("the value cannot be held weakly"));
        goto end;
    }

    wse = (RJS_WeakSetEntry*)hash_add(rt, thiz, v, sizeof(RJS_WeakSetEntry));

    if (!wse->weak_ref)
        wse->weak_ref = rjs_weak_ref_add(rt, thiz, v, weak_set_on_final);

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

/*WeakSet.prototype.delete*/
static RJS_NF(WeakSet_prototype_delete)
{
    RJS_Value        *key = rjs_argument_get(rt, args, argc, 0);
    RJS_WeakSetEntry *wse;
    RJS_Bool          b;
    RJS_Result        r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a weak set"));
        goto end;
    }

    wse = (RJS_WeakSetEntry*)hash_delete(rt, thiz, key);
    if (wse) {
        if (wse->weak_ref)
            rjs_weak_ref_free(rt, wse->weak_ref);

        RJS_DEL(rt, wse);

        b = RJS_TRUE;
    } else {
        b = RJS_FALSE;
    }

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*WeakSet.prototype.has*/
static RJS_NF(WeakSet_prototype_has)
{
    RJS_Value        *k = rjs_argument_get(rt, args, argc, 0);
    RJS_Result        r;
    RJS_WeakSetEntry *wse;
    RJS_Bool          b;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a weak set"));
        goto end;
    }

    wse = (RJS_WeakSetEntry*)hash_get(rt, thiz, k);
    b  = wse ? RJS_TRUE : RJS_FALSE;

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinFuncDesc
weak_set_prototype_function_descs[] = {
    {
        "add",
        1,
        WeakSet_prototype_add
    },
    {
        "delete",
        1,
        WeakSet_prototype_delete
    },
    {
        "has",
        1,
        WeakSet_prototype_has
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
weak_set_prototype_desc = {
    "WeakSet",
    NULL,
    NULL,
    NULL,
    weak_set_prototype_field_descs,
    weak_set_prototype_function_descs,
    NULL,
    NULL,
    "WeakSet_prototype"
};