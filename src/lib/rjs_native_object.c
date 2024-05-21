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

/**The native object.*/
typedef struct {
    RJS_Object     o;           /**< Base object data.*/
    const void    *tag;         /**< Data's tag.*/
    RJS_NativeData native_data; /**< The native data.*/
} RJS_NativeObject;

/**The native function object.*/
typedef struct {
    RJS_BuiltinFuncObject bfo;         /**< Base built-in function object.*/
    const void           *tag;         /**< Data's tag.*/
    RJS_NativeData        native_data; /**< The native data.*/
} RJS_NativeFuncObject;

/*Scan the referenced things in the native object.*/
static void
native_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_NativeObject *no = ptr;

    rjs_object_op_gc_scan(rt, &no->o);
    rjs_native_data_scan(rt, &no->native_data);
}

/*Free the native object.*/
static void
native_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_NativeObject *no = ptr;

    rjs_object_deinit(rt, &no->o);
    rjs_native_data_free(rt, &no->native_data);

    RJS_DEL(rt, no);
}

/**Native object's operation functions.*/
static const RJS_ObjectOps
native_object_ops = {
    {
        RJS_GC_THING_NATIVE_OBJECT,
        native_object_op_gc_scan,
        native_object_op_gc_free
    },
    RJS_ORDINARY_OBJECT_OPS
};

/*Scan the referenced things in the native function object.*/
static void
native_func_object_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_NativeFuncObject *nfo = ptr;

    rjs_builtin_func_object_op_gc_scan(rt, ptr);
    rjs_native_data_scan(rt, &nfo->native_data);
}

/*Free the native function object.*/
static void
native_func_object_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_NativeFuncObject *nfo = ptr;

    rjs_builtin_func_object_deinit(rt, &nfo->bfo);
    rjs_native_data_free(rt, &nfo->native_data);

    RJS_DEL(rt, nfo);
}

/**Native function object's operation functions.*/
static const RJS_ObjectOps
native_func_object_ops = {
    {
        RJS_GC_THING_NATIVE_FUNC,
        native_func_object_op_gc_scan,
        native_func_object_op_gc_free
    },
    RJS_BUILTIN_FUNCTION_OBJECT_OPS
};

/**Native constructor operation functions.*/
static const RJS_ObjectOps
native_constructor_ops = {
    {
        RJS_GC_THING_NATIVE_FUNC,
        native_func_object_op_gc_scan,
        native_func_object_op_gc_free
    },
    RJS_BUILTIN_CONSTRUCTOR_OBJECT_OPS
};

/**
 * Create a new native object.
 * \param rt The current runtime.
 * \param[out] o Return the new native object.
 * \param proto The prototype of the object.
 * If proto == NULL, use Object.prototype.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_native_object_new (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto)
{
    RJS_NativeObject *no;
    RJS_Result        r;

    RJS_NEW(rt, no);

    rjs_native_data_init(&no->native_data);
    no->tag = NULL;

    r = rjs_object_init(rt, o, &no->o, proto, &native_object_ops);
    if (r == RJS_ERR) {
        RJS_DEL(rt, no);
        return RJS_ERR;
    }

    return RJS_OK;
}

/**
 * Create a new native object from the constructor.
 * \param rt The current runtime.
 * \param c The constructor.
 * \param proto The default prototype object.
 * \param[out] o REturn the new native object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_native_object_from_constructor (RJS_Runtime *rt, RJS_Value *c, RJS_Value *proto, RJS_Value *o)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *p   = rjs_value_stack_push(rt);
    RJS_Result r;

    if (c) {
        if ((r = rjs_get(rt, c, rjs_pn_prototype(rt), p)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_object(rt, p))
            rjs_value_copy(rt, p, proto);
    } else {
        rjs_value_copy(rt, p, proto);
    }

    r = rjs_native_object_new(rt, o, p);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a new native function object.
 * \param rt The current runtime.
 * \param[out] v Return the new native function object value.
 * \param realm The function's realm.
 * \param proto The prototype of the function.
 * \param script The script contains the function.
 * \param nf The native function pointer.
 * \param flags The flags of the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_native_func_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Realm *realm,
        RJS_Value *proto, RJS_Script *script, RJS_NativeFunc nf, int flags)
{
    RJS_NativeFuncObject *nfo;
    RJS_Result            r;

    RJS_NEW(rt, nfo);

    rjs_native_data_init(&nfo->native_data);
    nfo->tag = NULL;

    r = rjs_builtin_func_object_init(rt, v, &nfo->bfo, realm, proto, script, nf, flags,
            &native_func_object_ops);
    if (r == RJS_ERR) {
        RJS_DEL(rt, nfo);
        return RJS_ERR;
    }

    return RJS_OK;
}

/**
 * Create a new native function.
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
rjs_create_native_function (RJS_Runtime *rt, RJS_Value *mod, RJS_NativeFunc nf, size_t len,
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

    if ((r = rjs_native_func_object_new(rt, f, realm, proto, script, nf, flags)) == RJS_ERR)
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
 * Make the native function object as constructor.
 * \param rt The current runtime.
 * \param f The native function object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_native_func_object_make_constructor (RJS_Runtime *rt, RJS_Value *f)
{
    RJS_GcThing *gt = (RJS_GcThing*)rjs_value_get_object(rt, f);

    if (gt->ops == (RJS_GcThingOps*)&native_func_object_ops)
        gt->ops = (RJS_GcThingOps*)&native_constructor_ops;

    return RJS_OK;
}

/**
 * Set the native data of the object.
 * \param rt The current runtime.
 * \param o The native object.
 * \param tag The tag of the data.
 * \param data The data's pointer.
 * \param scan The function scan the referenced things in the data.
 * \param free The function free the data.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_native_object_set_data (RJS_Runtime *rt, RJS_Value *o, const void *tag,
        void *data, RJS_ScanFunc scan, RJS_FreeFunc free)
{
    RJS_GcThingType gtt;

    gtt = rjs_value_get_gc_thing_type(rt, o);

    if (gtt == RJS_GC_THING_NATIVE_OBJECT) {
        RJS_NativeObject *no;

        no = (RJS_NativeObject*)rjs_value_get_object(rt, o);
        no->tag = tag;

        rjs_native_data_set(&no->native_data, data, scan, free);
    } else if (gtt == RJS_GC_THING_NATIVE_FUNC) {
        RJS_NativeFuncObject *nfo;

        nfo = (RJS_NativeFuncObject*)rjs_value_get_object(rt, o);
        nfo->tag = tag;

        rjs_native_data_set(&nfo->native_data, data, scan, free);
    } else {
        assert(0);
    }

    return RJS_OK;
}

/**
 * Get the native data's tag.
 * \param rt The current runtime.
 * \param o The native object.
 * \return The native object's tag.
 */
const void*
rjs_native_object_get_tag (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_GcThingType gtt;
    const void     *tag = NULL;

    gtt = rjs_value_get_gc_thing_type(rt, o);

    if (gtt == RJS_GC_THING_NATIVE_OBJECT) {
        RJS_NativeObject *no;

        no  = (RJS_NativeObject*)rjs_value_get_object(rt, o);
        tag = no->tag;
    } else if (gtt == RJS_GC_THING_NATIVE_FUNC) {
        RJS_NativeFuncObject *nfo;

        nfo = (RJS_NativeFuncObject*)rjs_value_get_object(rt, o);
        tag = nfo->tag;
    }

    return tag;
}

/**
 * Get the native data's pointer of the object.
 * \param rt The current runtime.
 * \param o The native object.
 * \retval The native data's pointer.
 */
void*
rjs_native_object_get_data (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_GcThingType gtt;
    void           *data = NULL;

    gtt = rjs_value_get_gc_thing_type(rt, o);

    if (gtt == RJS_GC_THING_NATIVE_OBJECT) {
        RJS_NativeObject *no;

        no = (RJS_NativeObject*)rjs_value_get_object(rt, o);
        data = no->native_data.data;
    } else {
        RJS_NativeFuncObject *nfo;

        nfo = (RJS_NativeFuncObject*)rjs_value_get_object(rt, o);
        data = nfo->native_data.data;
    }

    return data;
}
