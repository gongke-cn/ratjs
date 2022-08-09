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

/*Free the weak map.*/
static void
weak_map_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    hash_op_gc_free(rt, ptr, sizeof(RJS_WeakMapEntry));
}

/*Weak map object operation functions.*/
static const RJS_ObjectOps
weak_map_ops = {
    {
        RJS_GC_THING_WEAK_MAP,
        weak_hash_op_gc_scan,
        weak_map_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*WeakMap*/
static RJS_NF(WeakMap_constructor)
{
    RJS_Value *iterable = rjs_argument_get(rt, args, argc, 0);

    return map_new(rt, rv, nt, RJS_O_WeakMap_prototype, &weak_map_ops, iterable);
}

static const RJS_BuiltinFuncDesc
weak_map_constructor_desc = {
    "WeakMap",
    0,
    WeakMap_constructor
};

static const RJS_BuiltinFieldDesc
weak_map_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "WeakMap",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*WeakMap.prototype.delete*/
static RJS_NF(WeakMap_prototype_delete)
{
    RJS_Value        *key = rjs_argument_get(rt, args, argc, 0);
    RJS_WeakMapEntry *wme;
    RJS_Bool          b;
    RJS_Result        r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a weak map"));
        goto end;
    }

    wme = (RJS_WeakMapEntry*)hash_delete(rt, thiz, key);
    if (wme) {
        if (wme->weak_ref)
            rjs_weak_ref_free(rt, wme->weak_ref);

        RJS_DEL(rt, wme);

        b = RJS_TRUE;
    } else {
        b = RJS_FALSE;
    }

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*WeakMap.prototype.get*/
static RJS_NF(WeakMap_prototype_get)
{
    RJS_Value        *key = rjs_argument_get(rt, args, argc, 0);
    RJS_WeakMapEntry *wme;
    RJS_Result        r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a weak map"));
        goto end;
    }

    wme = (RJS_WeakMapEntry*)hash_get(rt, thiz, key);
    if (wme) {
        rjs_value_copy(rt, rv, &wme->me.value);
    } else {
        rjs_value_set_undefined(rt, rv);
    }

    r = RJS_OK;
end:
    return r;
}

/*WeakMap.prototype.has*/
static RJS_NF(WeakMap_prototype_has)
{
    RJS_Value        *key = rjs_argument_get(rt, args, argc, 0);
    RJS_WeakMapEntry *wme;
    RJS_Bool          b;
    RJS_Result        r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a weak map"));
        goto end;
    }

    wme = (RJS_WeakMapEntry*)hash_get(rt, thiz, key);
    b   = wme ? RJS_TRUE : RJS_FALSE;

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*Weak map in finalize function.*/
static void
weak_map_on_final (RJS_Runtime *rt, RJS_WeakRef *wr)
{
    RJS_WeakMapEntry *wme;

    wme = (RJS_WeakMapEntry*)hash_delete(rt, &wr->base, &wr->ref);

    RJS_DEL(rt, wme);
}

/*WeakMap.prototype.set*/
static RJS_NF(WeakMap_prototype_set)
{
    RJS_Value        *k = rjs_argument_get(rt, args, argc, 0);
    RJS_Value        *v = rjs_argument_get(rt, args, argc, 1);
    RJS_WeakMapEntry *wme;
    RJS_Result        r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_WEAK_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a weak map"));
        goto end;
    }

    if (!rjs_can_be_held_weakly(rt, k)) {
        r = rjs_throw_type_error(rt, _("the key cannot be held weakly"));
        goto end;
    }

    wme = (RJS_WeakMapEntry*)hash_add(rt, thiz, k, sizeof(RJS_WeakMapEntry));

    rjs_value_copy(rt, &wme->me.value, v);

    if (wme->weak_ref)
        rjs_weak_ref_free(rt, wme->weak_ref);

    wme->weak_ref = rjs_weak_ref_add(rt, thiz, k, weak_map_on_final);

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinFuncDesc
weak_map_prototype_function_descs[] = {
    {
        "delete",
        1,
        WeakMap_prototype_delete
    },
    {
        "get",
        1,
        WeakMap_prototype_get
    },
    {
        "has",
        1,
        WeakMap_prototype_has
    },
    {
        "set",
        2,
        WeakMap_prototype_set
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
weak_map_prototype_desc = {
    "WeakMap",
    NULL,
    NULL,
    NULL,
    weak_map_prototype_field_descs,
    weak_map_prototype_function_descs,
    NULL,
    NULL,
    "WeakMap_prototype"
};