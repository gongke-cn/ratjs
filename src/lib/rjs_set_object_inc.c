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

/*Free the set.*/
static void
set_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    hash_op_gc_free(rt, ptr, sizeof(RJS_SetEntry));
}

/*Set object operation functions.*/
static const RJS_ObjectOps
set_ops = {
    {
        RJS_GC_THING_SET,
        set_op_gc_scan,
        set_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Set*/
static RJS_NF(Set_constructor)
{
    RJS_Value *iterable = rjs_argument_get(rt, args, argc, 0);

    return set_new(rt, rv, nt, RJS_O_Set_prototype, &set_ops, iterable);
}

static const RJS_BuiltinFuncDesc
set_constructor_desc = {
    "Set",
    0,
    Set_constructor
};

static const RJS_BuiltinAccessorDesc
set_accessor_descs[] = {
    {
        "@@species",
        rjs_return_this
    },
    {NULL}
};

static const RJS_BuiltinFieldDesc
set_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Set",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

/*Set.prototype.add*/
static RJS_NF(Set_prototype_add)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    hash_add(rt, thiz, v, sizeof(RJS_SetEntry));

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

/*Set.prototype.clear*/
static RJS_NF(Set_prototype_clear)
{
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    if ((r = hash_clear(rt, thiz, sizeof(RJS_SetEntry))) == RJS_ERR)
        goto end;

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    return r;
}

/*Set.prototype.delete*/
static RJS_NF(Set_prototype_delete)
{
    RJS_Value    *key = rjs_argument_get(rt, args, argc, 0);
    RJS_SetEntry *se;
    RJS_Bool      b;
    RJS_Result    r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    se = hash_delete(rt, thiz, key);
    if (se) {
        RJS_DEL(rt, se);

        b = RJS_TRUE;
    } else {
        b = RJS_FALSE;
    }

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*Set.prototype.entries*/
static RJS_NF(Set_prototype_entries)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    r = hash_iter_new(rt, rv, thiz, rjs_o_SetIteratorPrototype(realm), RJS_HASH_ITER_KEY_VALUE);
end:
    return r;
}

/*Set.prototype.forEach*/
static RJS_NF(Set_prototype_forEach)
{
    RJS_Value      *cb_fn    = rjs_argument_get(rt, args, argc, 0);
    RJS_Value      *this_arg = rjs_argument_get(rt, args, argc, 1);
    size_t          top      = rjs_value_stack_save(rt);
    RJS_Value      *k        = rjs_value_stack_push(rt);
    RJS_Value      *v        = rjs_value_stack_push(rt);
    RJS_Value      *s        = rjs_value_stack_push(rt);
    RJS_Value      *res      = rjs_value_stack_push(rt);
    RJS_HashObject *ho;
    RJS_HashIter    hi;
    RJS_Result      r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    if (!rjs_is_callable(rt, cb_fn)) {
        r = rjs_throw_type_error(rt, _("the value is not a funciton"));
        goto end;
    }

    rjs_value_copy(rt, s, thiz);

    ho = (RJS_HashObject*)rjs_value_get_object(rt, thiz);
    hi.curr = ho->list.next;
    hi.done = RJS_FALSE;
    rjs_list_append(&ho->iters, &hi.ln);

    r = RJS_OK;

    while (hi.curr != &ho->list) {
        RJS_SetEntry *se;

        se = RJS_CONTAINER_OF(hi.curr, RJS_SetEntry, ln);

        rjs_value_copy(rt, k, &se->key);
        rjs_value_copy(rt, v, &se->key);

        hi.curr = hi.curr->next;

        if ((r = rjs_call(rt, cb_fn, this_arg, k, 3, res)) == RJS_ERR)
            break;
    }

    rjs_list_remove(&hi.ln);
    rjs_value_set_undefined(rt, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Set.prototype.has*/
static RJS_NF(Set_prototype_has)
{
    RJS_Value    *k = rjs_argument_get(rt, args, argc, 0);
    RJS_Result    r;
    RJS_SetEntry *se;
    RJS_Bool      b;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    se = hash_get(rt, thiz, k);
    b  = se ? RJS_TRUE : RJS_FALSE;

    rjs_value_set_boolean(rt, rv, b);
    r = RJS_OK;
end:
    return r;
}

/*Set.prototype.values*/
static RJS_NF(Set_prototype_values)
{
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_Result r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    r = hash_iter_new(rt, rv, thiz, rjs_o_SetIteratorPrototype(realm), RJS_HASH_ITER_VALUE);
end:
    return r;
}

static const RJS_BuiltinFuncDesc
set_prototype_function_descs[] = {
    {
        "add",
        1,
        Set_prototype_add
    },
    {
        "clear",
        0,
        Set_prototype_clear
    },
    {
        "delete",
        1,
        Set_prototype_delete
    },
    {
        "entries",
        0,
        Set_prototype_entries
    },
    {
        "forEach",
        1,
        Set_prototype_forEach
    },
    {
        "has",
        1,
        Set_prototype_has
    },
    {
        "values",
        0,
        Set_prototype_values,
        "Set_prototype_values"
    },
    {
        "keys",
        0,
        NULL,
        "Set_prototype_values"
    },
    {
        "@@iterator",
        0,
        NULL,
        "Set_prototype_values"
    },
    {NULL}
};

/*get Set.prototype.size*/
static RJS_NF(Set_prototype_size_get)
{
    RJS_HashObject *ho;
    RJS_Result      r;

    if (rjs_value_get_gc_thing_type(rt, thiz) != RJS_GC_THING_SET) {
        r = rjs_throw_type_error(rt, _("the value is not a set"));
        goto end;
    }

    ho = (RJS_HashObject*)rjs_value_get_object(rt, thiz);

    rjs_value_set_number(rt, rv, ho->hash.entry_num);
    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinAccessorDesc
set_prototype_accessor_descs[] = {
    {
        "size",
        Set_prototype_size_get
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
set_prototype_desc = {
    "Set",
    NULL,
    NULL,
    NULL,
    set_prototype_field_descs,
    set_prototype_function_descs,
    set_prototype_accessor_descs,
    NULL,
    "Set_prototype"
};

static const RJS_BuiltinFieldDesc
set_iterator_prototype_field_descs[] = {
    {
        "@@toStringTag",
        RJS_VALUE_STRING,
        0,
        "Set Iterator",
        RJS_PROP_ATTR_CONFIGURABLE
    },
    {NULL}
};

static const RJS_BuiltinFuncDesc
set_iterator_prototype_function_descs[] = {
    {
        "next",
        0,
        hash_iter_next
    },
    {NULL}
};
