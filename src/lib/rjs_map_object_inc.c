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

/*Free the map.*/
static void
map_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    hash_op_gc_free(rt, ptr, sizeof(RJS_MapEntry));
}

/*Map object operation functions.*/
static const RJS_ObjectOps
map_ops = {
    {
        RJS_GC_THING_MAP,
        map_op_gc_scan,
        map_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Map*/
static RJS_NF(Map_constructor)
{
    RJS_Value *iterable = rjs_argument_get(rt, args, argc, 0);

    return map_new(rt, rv, nt, RJS_O_Map_prototype, &map_ops, iterable);
}

static const RJS_BuiltinFuncDesc
map_constructor_desc = {
    "Map",
    0,
    Map_constructor
};

static const RJS_BuiltinAccessorDesc
map_accessor_descs[] = {
    {
        "@@species",
        rjs_return_this
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
map_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Map",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Map.prototype.clear*/
static RJS_NF(Map_prototype_clear)
{
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    r = hash_clear(rt, thiz, sizeof(RJS_MapEntry));

    rjs_value_set_undefined(rt, rv);
end:
    return r;
}

/*Map.prototype.delete*/
static RJS_NF(Map_prototype_delete)
{
    RJS_Value    *key = rjs_argument_get(rt, args, argc, 0);
    RJS_MapEntry *me;
    RJS_Bool      b;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    me = (RJS_MapEntry*)hash_delete(rt, thiz, key);
    if (me) {
        RJS_DEL(rt, me);

        b = RJS_TRUE;
    } else {
        b = RJS_FALSE;
    }

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*Map.prototype.entries*/
static RJS_NF(Map_prototype_entries)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    r = hash_iter_new(rt, rv, thiz, rjs_o_MapIteratorPrototype(realm), RJS_HASH_ITER_KEY_VALUE);
end:
    return r;
}

/*Map.prototype.forEach*/
static RJS_NF(Map_prototype_forEach)
{
    RJS_Value      *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value      *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t          top      = rjs_value_stack_save(rt);
    RJS_Value      *v        = rjs_value_stack_push(rt);
    RJS_Value      *k        = rjs_value_stack_push(rt);
    RJS_Value      *m        = rjs_value_stack_push(rt);
    RJS_Value      *res      = rjs_value_stack_push(rt);
    RJS_HashObject *ho;
    RJS_HashIter    hi;
    RJS_Result      r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a funciton"));
        goto end;
    }

    rjs_value_copy(rt, m, thiz);

    ho = (RJS_HashObject*)rjs_value_get_object(rt, thiz);
    hi.curr = ho->list.next;
    hi.done = RJS_FALSE;
    rjs_list_append(&ho->iters, &hi.ln);

    r = RJS_OK;

    while (hi.curr != &ho->list) {
        RJS_MapEntry *me;

        me = RJS_CONTAINER_OF(hi.curr, RJS_MapEntry, se.ln);

        rjs_value_copy(rt, k, &me->se.key);
        rjs_value_copy(rt, v, &me->value);

        hi.curr = hi.curr->next;

        if ((r = rjs_call(rt, cb_fn, this_arg, v, 3, res)) == RJS_ERR)
            break;
    }

    rjs_list_remove(&hi.ln);
    rjs_value_set_undefined(rt, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Map.prototype.get*/
static RJS_NF(Map_prototype_get)
{
    RJS_Value    *key = rjs_argument_get(rt, args, argc, 0);
    RJS_MapEntry *me;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    me = (RJS_MapEntry*)hash_get(rt, thiz, key);
    if (me) {
        rjs_value_copy(rt, rv, &me->value);
    } else {
        rjs_value_set_undefined(rt, rv);
    }

    r = RJS_OK;
end:
    return r;
}

/*Map.prototype.has*/
static RJS_NF(Map_prototype_has)
{
    RJS_Value    *key = rjs_argument_get(rt, args, argc, 0);
    RJS_MapEntry *me;
    RJS_Bool      b;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    me = (RJS_MapEntry*)hash_get(rt, thiz, key);
    b  = me ? RJS_TRUE : RJS_FALSE;

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*Map.prototype.keys*/
static RJS_NF(Map_prototype_keys)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    r = hash_iter_new(rt, rv, thiz, rjs_o_MapIteratorPrototype(realm), RJS_HASH_ITER_KEY);
end:
    return r;
}

/*Map.prototype.set*/
static RJS_NF(Map_prototype_set)
{
    RJS_Value    *k = rjs_argument_get(rt, args, argc, 0);
    RJS_Value    *v = rjs_argument_get(rt, args, argc, 1);
    RJS_MapEntry *me;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    me = (RJS_MapEntry*)hash_add(rt, thiz, k, sizeof(RJS_MapEntry));

    rjs_value_copy(rt, &me->value, v);

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

/*Map.prototype.values*/
static RJS_NF(Map_prototype_values)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    r = hash_iter_new(rt, rv, thiz, rjs_o_MapIteratorPrototype(realm), RJS_HASH_ITER_VALUE);
end:
    return r;
}

static const RJS_BuiltinFuncDesc
map_prototype_function_descs[] = {
    {
        "clear",
        0,
        Map_prototype_clear
    },
    {
        "delete",
        1,
        Map_prototype_delete
    },
    {
        "entries",
        0,
        Map_prototype_entries,
        "Map_prototype_entries"
    },
    {
        "forEach",
        1,
        Map_prototype_forEach
    },
    {
        "get",
        1,
        Map_prototype_get
    },
    {
        "has",
        1,
        Map_prototype_has
    },
    {
        "keys",
        0,
        Map_prototype_keys
    },
    {
        "set",
        2,
        Map_prototype_set
    },
    {
        "values",
        0,
        Map_prototype_values
    },
    {
        "@@iterator",
        0,
        NULL,
        "Map_prototype_entries"
    },
    {NULL}
};

/*get Map.prototype.size*/
static RJS_NF(Map_prototype_size_get)
{
    RJS_HashObject *ho;
    RJS_Result      r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_MAP) {
        r = rjs_throw_type_error(rt, _("the value is not a map"));
        goto end;
    }

    ho = (RJS_HashObject*)rjs_value_get_object(rt, thiz);

    rjs_value_set_number(rt, rv, ho->hash.entry_num);
    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinAccessorDesc
map_prototype_accessor_descs[] = {
    {
        "size",
        Map_prototype_size_get
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
map_prototype_desc = {
    "Map",
    NULL,
    NULL,
    NULL,
    map_prototype_field_descs,
    map_prototype_function_descs,
    map_prototype_accessor_descs,
    NULL,
    "Map_prototype"
};

static const RJS_BuiltinFieldDesc
map_iterator_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Map Iterator",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

static const RJS_BuiltinFuncDesc
map_iterator_prototype_function_descs[] = {
    {
        "next",
        0,
        hash_iter_next
    },
    {NULL}
};
