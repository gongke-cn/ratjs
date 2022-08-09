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

/*Add built-in fields to the object.*/
static RJS_Result
object_add_builtin_fields (RJS_Runtime *rt, RJS_Value *o, const RJS_BuiltinFieldDesc *bfd);
/*Add built-in function methods to the object.*/
static RJS_Result
object_add_builtin_functions (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, const RJS_BuiltinFuncDesc *bfd);
/*Add built-in accessor methods to the object.*/
static RJS_Result
object_add_builtin_accessors (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, const RJS_BuiltinAccessorDesc *bad);
/*Add built-in object methods to the object.*/
static RJS_Result
object_add_builtin_objects (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, const RJS_BuiltinObjectDesc *bod);

/**
 * Scan the referenced things in the built-in function object.
 * \param rt The current runtime.
 * \param ptr The built-in function object pointer.
 */
void
rjs_builtin_func_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_BuiltinFuncObject *bfo = ptr;

    rjs_base_func_object_op_gc_scan(rt, &bfo->bfo);

    if (bfo->realm)
        rjs_gc_mark(rt, bfo->realm);

#if ENABLE_FUNC_SOURCE
    rjs_gc_scan_value(rt, &bfo->init_name);
#endif /*ENABLE_FUNC_SOURCE*/
}

/**
 * Free the built-in function object.
 * \param rt The current runtime.
 * \param ptr The built-in function object pointer.
 */
void
rjs_builtin_func_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_BuiltinFuncObject *bfo = ptr;

    rjs_builtin_func_object_deinit(rt, bfo);

    RJS_DEL(rt, bfo);
}

/**
 * Call the built-in function object.
 * \param rt The current runtime.
 * \param o The built-in function object.
 * \param thiz This argument.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_builtin_func_object_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_BuiltinFuncObject *bfo;
    RJS_Context           *ctxt;
    RJS_Result             r;

    ctxt = rjs_context_push(rt, o);
    bfo  = (RJS_BuiltinFuncObject*)rjs_value_get_object(rt, o);

    ctxt->realm = bfo->realm;

    r = bfo->func(rt, o, thiz, args, argc, NULL, rv);

    rjs_context_pop(rt);

    return r;
}

/**
 * Construct a new object from a built-in function.
 * \param rt The current runtime.
 * \param o The built-in function object.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param target The new target.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_builtin_func_object_op_construct (RJS_Runtime *rt, RJS_Value *o, RJS_Value *args, size_t argc, RJS_Value *target, RJS_Value *rv)
{
    RJS_BuiltinFuncObject *bfo;
    RJS_Context           *ctxt;
    RJS_Result             r;

    ctxt = rjs_context_push(rt, o);
    bfo  = (RJS_BuiltinFuncObject*)rjs_value_get_object(rt, o);

    ctxt->realm = bfo->realm;

    r = bfo->func(rt, o, NULL, args, argc, target, rv);

    rjs_context_pop(rt);

    return r;
}

/*Built-in function operation functions.*/
static const RJS_ObjectOps
builtin_func_object_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        rjs_builtin_func_object_op_gc_scan,
        rjs_builtin_func_object_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/*Built-in constructor operation functions.*/
static const RJS_ObjectOps
builtin_constructor_object_ops = {
    {
        RJS_GC_THING_BUILTIN_FUNC,
        rjs_builtin_func_object_op_gc_scan,
        rjs_builtin_func_object_op_gc_free
    },
    RJS_BUILTIN_CONSTRUCTOR_OBJECT_OPS
};

/**
 * Create a new built-in function.
 * \param rt The current runtime.
 * \param[out] v Return the new built-in object.
 * \param realm The relam.
 * \param proto The prototype value.
 * \param script The script contains this function.
 * \param nf The native function's pointer.
 * \param flags The function's flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_builtin_func_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Realm *realm,
        RJS_Value *proto, RJS_Script *script, RJS_NativeFunc nf, int flags)
{
    RJS_BuiltinFuncObject *bfo;

    RJS_NEW(rt, bfo);

    return rjs_builtin_func_object_init(rt, v, bfo, realm, proto, script, nf, flags, NULL);
}

/**
 * Initialize the built-in function.
 * \param rt The current runtime.
 * \param[out] v Return the new built-in function object.
 * \param bfo The build-in function object to be initialized.
 * \param realm The realm.
 * \param proto The prototype value.
 * \param script The script contains this function.
 * \param nf The native function's pointer.
 * \param flags The function's flags.
 * \param ops The object's operation functions.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_builtin_func_object_init (RJS_Runtime *rt, RJS_Value *v, RJS_BuiltinFuncObject *bfo, RJS_Realm *realm,
        RJS_Value *proto, RJS_Script *script, RJS_NativeFunc nf, int flags, const RJS_ObjectOps *ops)
{
    if (!realm)
        realm = rjs_realm_current(rt);

    bfo->func  = nf;
    bfo->realm = realm;
    bfo->flags = flags;

#if ENABLE_FUNC_SOURCE
    rjs_value_set_undefined(rt, &bfo->init_name);
#endif /*ENABLE_FUNC_SOURCE*/

    if (!proto)
        proto = rjs_o_Function_prototype(realm);

    if (!ops)
        ops = &builtin_func_object_ops;

    rjs_base_func_object_init(rt, v, &bfo->bfo, proto, ops, script);

    return RJS_OK;
}

/**
 * Release the built-in function object.
 * \param rt The current runtime.
 * \param bfo The built-in function object to be released.
 */
void
rjs_builtin_func_object_deinit (RJS_Runtime *rt, RJS_BuiltinFuncObject *bfo)
{
    rjs_base_func_object_deinit(rt, &bfo->bfo);
}

/**
 * Make the built-in function object as constructor.
 * \param rt The current runtime.
 * \param f The script function object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_builtin_func_object_make_constructor (RJS_Runtime *rt, RJS_Value *f)
{
    RJS_BuiltinFuncObject *bfo = (RJS_BuiltinFuncObject*)rjs_value_get_object(rt, f);

    if (bfo->bfo.object.gc_thing.ops == (RJS_GcThingOps*)&builtin_func_object_ops)
        bfo->bfo.object.gc_thing.ops = (RJS_GcThingOps*)&builtin_constructor_object_ops;

    return RJS_OK;
}

/**
 * Create a new built-in function.
 * \param rt The current runtime.
 * \param mod The module contains this function.
 * \param nf The native function.
 * \param len The parameters length.
 * \param name The function's name.
 * \param realm The realm.
 * \param proto The prototype value.
 * \param prefix The name prefix.
 * \param[out] f Return the new built-in function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_create_builtin_function (RJS_Runtime *rt, RJS_Value *mod, RJS_NativeFunc nf, size_t len,
        RJS_Value *name, RJS_Realm *realm, RJS_Value *proto, RJS_Value *prefix, RJS_Value *f)
{
    RJS_Result  r;
    RJS_Script *script = NULL;
    int         flags  = 0;

    if (mod)
        script = rjs_value_get_gc_thing(rt, mod);

    if (prefix) {
        if (rjs_same_value(rt, prefix, rjs_s_get(rt)))
            flags = RJS_FUNC_FL_GET;
        else if (rjs_same_value(rt, prefix, rjs_s_set(rt)))
            flags = RJS_FUNC_FL_SET;
    }

    if ((r = rjs_builtin_func_object_new(rt, f, realm, proto, script, nf, flags)) == RJS_ERR)
        return r;

    if ((r = rjs_set_function_length(rt, f, len)) == RJS_ERR)
        return r;

    if (name) {
        if ((r = rjs_set_function_name(rt, f, name, prefix)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

/**
 * Initialize a new built-in function.
 * \param rt The current runtime.
 * \param bfo The built-in function object to be initialized.
 * \param nf The native function.
 * \param flags The function's flags.
 * \param ops The object's operation functions.
 * \param len The parameters length.
 * \param name The function's name.
 * \param realm The realm.
 * \param proto The prototype value.
 * \param script The script contains this function.
 * \param prefix The name prefix.
 * \param[out] f Return the new built-in function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_init_builtin_function (RJS_Runtime *rt, RJS_BuiltinFuncObject *bfo, RJS_NativeFunc nf,
        int flags, const RJS_ObjectOps *ops, size_t len, RJS_Value *name,
        RJS_Realm *realm, RJS_Value *proto, RJS_Script *script, RJS_Value *prefix, RJS_Value *f)
{
    RJS_Result r;

    if ((r = rjs_builtin_func_object_init(rt, f, bfo, realm, proto, script, nf, flags, ops)) == RJS_ERR)
        return r;

    if ((r = rjs_set_function_length(rt, f, len)) == RJS_ERR)
        return r;

    if ((r = rjs_set_function_name(rt, f, name, prefix)) == RJS_ERR)
        return r;

    return RJS_OK;
}

/*Get the property name.*/
static RJS_Result
get_prop_name (RJS_Runtime *rt, const char *name, RJS_Value *v)
{
    if (name[0] == '@') {
        if (name[1] == '@') {
            RJS_Value *sym = rjs_internal_symbol_lookup(rt, name + 2);

            if (!sym) {
                RJS_LOGE("illegal internal symbol \"%s\"", name);
                return RJS_ERR;
            }

            rjs_value_copy(rt, v, sym);
        } else {
            size_t     top  = rjs_value_stack_save(rt);
            RJS_Value *desc = rjs_value_stack_push(rt);

            rjs_string_from_chars(rt, desc, name + 1, -1);
            rjs_symbol_new(rt, v, desc);

            rjs_value_stack_restore(rt, top);
        }
    } else {
        rjs_string_from_chars(rt, v, name, -1);
        rjs_string_to_property_key(rt, v);
    }

    return RJS_OK;
}

#include <rjs_object_table_inc.c>

static RJS_Value*
get_internal_object_pointer (RJS_Realm *realm, const char *name)
{
    const char **pn  = internal_object_name_table;
    int          idx = 0;

    while (*pn) {
        if (!strcmp(*pn, name))
            return &realm->objects[idx];

        pn  ++;
        idx ++;
    }

    return NULL;
}

/*Load the built-in object.*/
static RJS_Result
object_desc_load (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, RJS_Value *name, const RJS_BuiltinObjectDesc *bod)
{
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *proto  = rjs_value_stack_push(rt);
    RJS_Value *parent = rjs_value_stack_push(rt);
    RJS_Value *tmp    = rjs_value_stack_push(rt);
    RJS_Value *fname  = rjs_value_stack_push(rt);
    RJS_Result r;

    /*Get the parent.*/
    if (bod->parent) {
        RJS_Value *n = get_internal_object_pointer(realm, bod->parent);

        if (n)
            rjs_value_copy(rt, parent, n);
    }

    if (rjs_value_is_undefined(rt, parent)) {
        if (bod->constructor) {
            rjs_value_copy(rt, parent, rjs_o_Function_prototype(realm));
        } else {
            rjs_value_copy(rt, parent, rjs_o_Object_prototype(realm));
        }
    }

    /*Get the prototype.*/
    if (bod->prototype) {
        if ((r = object_desc_load(rt, realm, proto, NULL, bod->prototype)) == RJS_ERR)
            goto end;
    } else {
        rjs_value_copy(rt, proto, rjs_o_Object_prototype(realm));
    }

    /*Create the object.*/
    if (bod->constructor) {
        const RJS_BuiltinFuncDesc *bfd = bod->constructor;
        
        /*If no name use native name as constructor's name.*/
        if (!name) {
            if (bod->native)
                rjs_string_from_enc_chars(rt, fname, bod->native, -1, NULL);
        } else {
            rjs_value_copy(rt, fname, name);
        }

        if ((r = rjs_create_builtin_function(rt, NULL, bfd->func, bfd->length, fname,
                realm, parent, NULL, o)) == RJS_ERR)
            goto end;

        rjs_builtin_func_object_make_constructor(rt, o);
    } else if (bod->native && !strcmp(bod->native, "Object_prototype")) {
        rjs_value_copy(rt, o, rjs_o_Object_prototype(realm));
    } else if (bod->native && !strcmp(bod->native, "Function_prototype")) {
        rjs_value_copy(rt, o, rjs_o_Function_prototype(realm));
    } else if (bod->native && !strcmp(bod->native, "Boolean_prototype")) {
        rjs_value_set_boolean(rt, tmp, RJS_FALSE);
        rjs_primitive_object_new(rt, o, NULL, RJS_O_Object_prototype, tmp);
    } else if (bod->native && !strcmp(bod->native, "Number_prototype")) {
        rjs_value_set_number(rt, tmp, 0);
        rjs_primitive_object_new(rt, o, NULL, RJS_O_Object_prototype, tmp);
    } else if (bod->native && !strcmp(bod->native, "String_prototype")) {
        rjs_primitive_object_new(rt, o, NULL, RJS_O_Object_prototype, rjs_s_empty(rt));
    } else if (bod->native && !strcmp(bod->native, "Array_prototype")) {
        rjs_array_new(rt, o, 0, rjs_o_Object_prototype(realm));
    } else {
        if ((r = rjs_object_new(rt, o, parent)) == RJS_ERR)
            goto end;
    }

    /*Store the internal native object.*/
    if (bod->native
            && strcmp(bod->native, "Object_prototype")
            && strcmp(bod->native, "Function_prototype")) {
        RJS_Value *n = get_internal_object_pointer(realm, bod->native);

        if (n)
            rjs_value_copy(rt, n, o);
    }

    /*Create the "constructor" and "prototype" properties.*/
    if (bod->prototype) {
        int pflags = RJS_PROP_FL_DATA;
        int cflags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;

        if (bod->native) {
            if (!strcmp(bod->native, "GeneratorFunction_prototype")
                    || !strcmp(bod->native, "AsyncGeneratorFunction_prototype")) {
                pflags |= RJS_PROP_FL_CONFIGURABLE;
                cflags &= ~RJS_PROP_FL_WRITABLE;
            }

            if (!strcmp(bod->native, "GeneratorFunction")
                    || !strcmp(bod->native, "AsyncGeneratorFunction"))
                cflags &= ~RJS_PROP_FL_WRITABLE;
        }

        if ((r = rjs_create_data_property_attrs_or_throw(rt, o, rjs_pn_prototype(rt), proto,
                pflags)) == RJS_ERR)
            goto end;

        if ((r = rjs_create_data_property_attrs_or_throw(rt, proto, rjs_pn_constructor(rt), o,
                cflags)) == RJS_ERR)
            goto end;
    }

    /*Load fields.*/
    if (bod->fields) {
        if ((r = object_add_builtin_fields(rt, o, bod->fields)) == RJS_ERR)
            goto end;
    }

    /*Load methods.*/
    if (bod->functions) {
        if ((r = object_add_builtin_functions(rt, realm, o, bod->functions)) == RJS_ERR)
            goto end;
    }

    /*Load accessors.*/
    if (bod->accessors) {
        if ((r = object_add_builtin_accessors(rt, realm, o, bod->accessors)) == RJS_ERR)
            goto end;
    }

    /*Load the objects.*/
    if (bod->objects) {
        if ((r = object_add_builtin_objects(rt, realm, o, bod->objects)) == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Load a field description.*/
static RJS_Result
field_desc_load (RJS_Runtime *rt, RJS_Value *v, const RJS_BuiltinFieldDesc *bfd)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);

    switch (bfd->type) {
    case RJS_VALUE_UNDEFINED:
        rjs_value_set_undefined(rt, v);
        break;
    case RJS_VALUE_NUMBER:
        rjs_value_set_number(rt, v, bfd->n);
        break;
    case RJS_VALUE_STRING:
        rjs_string_from_chars(rt, v, bfd->ptr, -1);
        break;
    case RJS_VALUE_SYMBOL: {
        const char *str = bfd->ptr;

        if ((str[0] == '@') && (str[1] == '@')) {
            RJS_Value *sym = rjs_internal_symbol_lookup(rt, str + 2);

            if (!sym) {
                RJS_LOGE("illegal internal symbol \"%s\"", str);
                return RJS_ERR;
            }

            rjs_value_copy(rt, v, sym);
        } else {
            rjs_string_from_chars(rt, tmp, bfd->ptr, -1);
            rjs_symbol_new(rt, v, tmp);
        }
        break;
    }
    default:
        assert(0);
    }

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/*Add built-in fields to the object.*/
static RJS_Result
object_add_builtin_fields (RJS_Runtime *rt, RJS_Value *o, const RJS_BuiltinFieldDesc *bfd)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *name = rjs_value_stack_push(rt);
    RJS_Value       *v    = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    while (bfd->name) {
        if ((r = get_prop_name(rt, bfd->name, name)) == RJS_ERR)
            goto end;

        if ((r = field_desc_load(rt, v, bfd)) == RJS_ERR)
            goto end;

        rjs_property_name_init(rt, &pn, name);
        r = rjs_create_data_property_attrs_or_throw(rt, o, &pn, v,
                RJS_PROP_FL_DATA|bfd->attrs);
        rjs_property_name_deinit(rt, &pn);
        
        if (r == RJS_ERR)
            goto end;

        bfd ++;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Load a function description.*/
static RJS_Result
function_desc_load (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *v, RJS_Value *name, const RJS_BuiltinFuncDesc *bfd)
{
    RJS_Result r;

    if (bfd->func) {
        if (!name && bfd->native && !strcmp(bfd->native, "ThrowTypeError"))
            name = rjs_s_empty(rt);
            
        /*Create the function.*/
        if ((r = rjs_create_builtin_function(rt, NULL, bfd->func, bfd->length, name, realm,
                NULL, NULL, v)) == RJS_ERR)
            return r;

        /*Store the internal native object.*/
        if (bfd->native) {
            RJS_Value *n = get_internal_object_pointer(realm, bfd->native);

            rjs_value_copy(rt, n, v);

            if (!strcmp(bfd->native, "ThrowTypeError")) {
                RJS_PropertyDesc pd;

                rjs_property_desc_init(rt, &pd);
                pd.flags = RJS_PROP_FL_HAS_WRITABLE|RJS_PROP_FL_HAS_CONFIGURABLE|RJS_PROP_FL_HAS_ENUMERABLE;
                rjs_object_define_own_property(rt, v, rjs_pn_name(rt), &pd);
                rjs_object_define_own_property(rt, v, rjs_pn_length(rt), &pd);
                rjs_property_desc_deinit(rt, &pd);

                rjs_object_prevent_extensions(rt, v);
            }
        }
    } else {
        RJS_Value *n;

        assert(bfd->native);

        n = get_internal_object_pointer(realm, bfd->native);

        rjs_value_copy(rt, v, n);
    }

    return RJS_OK;
}

/*Add built-in function methods to the object.*/
static RJS_Result
object_add_builtin_functions (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, const RJS_BuiltinFuncDesc *bfd)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *name = rjs_value_stack_push(rt);
    RJS_Value       *v    = rjs_value_stack_push(rt);
    RJS_Value       *n;
    RJS_PropertyName pn;
    RJS_Result       r;

    while (bfd->name) {
        if (bfd->name[0] == '%') {
            n = NULL;
        } else {
            if ((r = get_prop_name(rt, bfd->name, name)) == RJS_ERR)
                goto end;
            n = name;
        }

        if ((r = function_desc_load(rt, realm, v, n, bfd)) == RJS_ERR)
            goto end;

        if (n) {
            int flags;

            if (!strcmp(bfd->name, "@@toPrimitive"))
                flags = RJS_PROP_FL_DATA|RJS_PROP_FL_CONFIGURABLE;
            else if (!strcmp(bfd->name, "@@hasInstance"))
                flags = RJS_PROP_FL_DATA;
            else
                flags = RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE;

            rjs_property_name_init(rt, &pn, n);
            r = rjs_create_data_property_attrs_or_throw(rt, o, &pn, v, flags);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;
        }

        bfd ++;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Add built-in accessor methods to the object.*/
static RJS_Result
object_add_builtin_accessors (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, const RJS_BuiltinAccessorDesc *bad)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *name = rjs_value_stack_push(rt);
    RJS_PropertyDesc pd;
    RJS_PropertyName pn;
    RJS_Result       r;

    rjs_property_desc_init(rt, &pd);

    pd.flags = RJS_PROP_FL_ACCESSOR|RJS_PROP_FL_CONFIGURABLE;

    while (bad->name) {
        if ((r = get_prop_name(rt, bad->name, name)) == RJS_ERR)
            goto end;

        if (bad->get) {
            if ((r = rjs_create_builtin_function(rt, NULL, bad->get, 0, name, realm, NULL,
                    rjs_s_get(rt), pd.get)) == RJS_ERR)
                goto end;
        } else if (bad->native_get) {
            RJS_Value *n = get_internal_object_pointer(realm, bad->native_get);

            rjs_value_copy(rt, pd.get, n);
        } else {
            rjs_value_set_undefined(rt, pd.get);
        }

        if (bad->set) {
            if ((r = rjs_create_builtin_function(rt, NULL, bad->set, 1, name, realm, NULL,
                    rjs_s_set(rt), pd.set)) == RJS_ERR)
                goto end;
        } else if (bad->native_set) {
            RJS_Value *n = get_internal_object_pointer(realm, bad->native_set);

            rjs_value_copy(rt, pd.set, n);
        } else {
            rjs_value_set_undefined(rt, pd.set);
        }

        rjs_property_name_init(rt, &pn, name);
        r = rjs_define_property_or_throw(rt, o, &pn, &pd);
        rjs_property_name_deinit(rt, &pn);
        if (r == RJS_ERR)
            goto end;

        bad ++;
    }

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Add built-in object methods to the object.*/
static RJS_Result
object_add_builtin_objects (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, const RJS_BuiltinObjectDesc *bod)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *name = rjs_value_stack_push(rt);
    RJS_Value       *v    = rjs_value_stack_push(rt);
    RJS_Value       *n;
    RJS_PropertyName pn;
    RJS_Result       r;

    while (bod->name) {
        if (bod->name[0] == '%') {
            n = NULL;
        } else {
            if ((r = get_prop_name(rt, bod->name, name)) == RJS_ERR)
                goto end;
            n = name;
        }

        if ((r = object_desc_load(rt, realm, v, n, bod)) == RJS_ERR)
            goto end;

        if (n) {
            rjs_property_name_init(rt, &pn, n);
            r = rjs_create_data_property_attrs_or_throw(rt, o, &pn, v,
                    RJS_PROP_FL_DATA|RJS_PROP_FL_WRITABLE|RJS_PROP_FL_CONFIGURABLE);
            rjs_property_name_deinit(rt, &pn);
            if (r == RJS_ERR)
                goto end;
        }

        bod ++;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Load the built-in object description.
 * \param rt The current runtime.
 * \param realm The realm.
 * \param desc The built-in object description.
 * \param[out] o Return the new object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_load_builtin_object_desc (RJS_Runtime *rt, RJS_Realm *realm, const RJS_BuiltinObjectDesc *desc, RJS_Value *o)
{
    return object_desc_load(rt, realm, o, NULL, desc);
}

/**
 * Load the built-in function description.
 * \param rt The current runtime.
 * \param realm The realm.
 * \param desc The built-in function description.
 * \param[out] f Return the new function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_load_builtin_func_desc (RJS_Runtime *rt, RJS_Realm *realm, const RJS_BuiltinFuncDesc *desc, RJS_Value *f)
{
    return function_desc_load(rt, realm, f, rjs_v_undefined(rt), desc);
}

/**
 * Load the built-in desciption to the global object.
 * \param rt The current runtime.
 * \param realm The realm.
 * \param desc The built-in desciption.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_load_builtin_desc (RJS_Runtime *rt, RJS_Realm *realm, const RJS_BuiltinDesc *desc)
{
    RJS_Value  *go;
    RJS_Result  r;

    go = rjs_global_object(realm);

    if (desc->fields) {
        if ((r = object_add_builtin_fields(rt, go, desc->fields)) == RJS_ERR)
            return r;
    }

    if (desc->functions) {
        if ((r = object_add_builtin_functions(rt, realm, go, desc->functions)) == RJS_ERR)
            return r;
    }

    if (desc->objects) {
        if ((r = object_add_builtin_objects(rt, realm, go, desc->objects)) == RJS_ERR)
            return r;
    }

    return RJS_OK;
}

#if ENABLE_MODULE

/*Add an export name.*/
static RJS_Value*
export_name_add (RJS_Runtime *rt, RJS_Module *mod, size_t id, const char *str)
{
    RJS_Script      *script = &mod->script;
    RJS_ExportEntry *ee     = &mod->export_entries[id];
    RJS_Value       *name   = &script->value_table[id];
    RJS_String      *key;

    rjs_string_from_chars(rt, name, str, -1);
    rjs_string_to_property_key(rt, name);

    key = rjs_value_get_string(rt, name);

    /*Add the export entries.*/
    ee->module_request_idx = RJS_INVALID_MODULE_REQUEST_INDEX;
    ee->import_name_idx    = RJS_INVALID_VALUE_INDEX;
    ee->local_name_idx     = id;
    ee->export_name_idx    = id;

    rjs_hash_insert(&mod->export_hash, key, &ee->he, NULL, &rjs_hash_size_ops, rt);

    return name;
}

/*Add an export binding.*/
static RJS_Result
binding_add (RJS_Runtime *rt, RJS_Module *mod, RJS_Value *name, RJS_Value *v)
{
    RJS_BindingName bn;
    RJS_Result      r;

    rjs_binding_name_init(rt, &bn, name);

    r = rjs_env_create_immutable_binding(rt, mod->env, &bn, RJS_TRUE);
    if (r == RJS_OK)
        r = rjs_env_initialize_binding(rt, mod->env, &bn, v);

    rjs_binding_name_deinit(rt, &bn);

    return r;
}

/**
 * Load the built-in desciption to the module.
 * \param rt The current runtime.
 * \param mod The module.
 * \param desc The built-in desciption.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_load_builtin_module_desc (RJS_Runtime *rt, RJS_Value *mod, const RJS_BuiltinDesc *desc)
{
    RJS_Module      *m    = (RJS_Module*)rjs_value_get_gc_thing(rt, mod);
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *v    = rjs_value_stack_push(rt);
    RJS_Value       *name;
    RJS_Script      *script;
    RJS_Result       r;
    size_t           en, eid;
    const RJS_BuiltinFieldDesc  *field;
    const RJS_BuiltinFuncDesc   *func;
    const RJS_BuiltinObjectDesc *obj;

    script = &m->script;

    assert(!m->export_entries);
    assert(!script->value_num);

    /*Get the export entries' number.*/
    en = 0;

    field = desc->fields;
    while (field->name) {
        en    ++;
        field ++;
    }

    func = desc->functions;
    while (func->name) {
        en   ++;
        func ++;
    }

    obj = desc->objects;
    while (obj->name) {
        en  ++;
        obj ++;
    }

    /*Allocate value buffer.*/
    script->value_num = en;
    RJS_NEW_N(rt, script->value_table, en);
    rjs_value_buffer_fill_undefined(rt, script->value_table, en);

    /*Allocate export entries buffer.*/
    m->local_export_entry_num = en;
    RJS_NEW_N(rt, m->export_entries, en);

    /*Process the export entries.*/
    eid = 0;
    field = desc->fields;
    while (field->name) {
        name = export_name_add(rt, m, eid, field->name);

        if ((r = field_desc_load(rt, v, field)) == RJS_ERR)
            goto end;

        if ((r = binding_add(rt, m, name, v)) == RJS_ERR)
            goto end;

        eid   ++;
        field ++;
    }

    func = desc->functions;
    while (func->name) {
        name = export_name_add(rt, m, eid, func->name);

        if ((r = function_desc_load(rt, script->realm, v, name, func)) == RJS_ERR)
            goto end;

        if ((r = binding_add(rt, m, name, v)) == RJS_ERR)
            goto end;

        eid ++;
        func ++;
    }

    obj = desc->objects;
    while (obj->name) {
        name = export_name_add(rt, m, eid, obj->name);

        if ((r = object_desc_load(rt, script->realm, v, name, obj)) == RJS_ERR)
            goto end;

        if ((r = binding_add(rt, m, name, v)) == RJS_ERR)
            goto end;

        eid ++;
        obj ++;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
    
}

#endif /*ENABLE_MODULE*/

/**
 * Get the module contains this function.
 * \param rt The current runtime.
 * \param func The function.
 * \param[out] mod Return the module contains this function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_get_function_module (RJS_Runtime *rt, RJS_Value *func, RJS_Value *mod)
{
    RJS_GcThingType     gtt = rjs_value_get_gc_thing_type(rt, func);
    RJS_BaseFuncObject *bfo;

    assert((gtt == RJS_GC_THING_SCRIPT_FUNC) || (gtt == RJS_GC_THING_BUILTIN_FUNC));

    bfo = (RJS_BaseFuncObject*)rjs_value_get_object(rt, func);

    assert(bfo->script && (bfo->script->gc_thing.ops->type == RJS_GC_THING_MODULE));

    rjs_value_set_gc_thing(rt, mod, bfo->script);
    return RJS_OK;
}
