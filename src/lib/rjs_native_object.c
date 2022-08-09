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

    r = rjs_object_init(rt, o, &no->o, proto, &native_object_ops);
    if (r == RJS_ERR) {
        RJS_DEL(rt, no);
        return RJS_ERR;
    }

    rjs_native_data_init(&no->native_data);

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
    RJS_NativeObject *no;

    assert(rjs_value_get_gc_thing_type(rt, o) == RJS_GC_THING_NATIVE_OBJECT);

    no = (RJS_NativeObject*)rjs_value_get_object(rt, o);

    no->tag = tag;

    rjs_native_data_set(&no->native_data, data, scan, free);

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
    RJS_NativeObject *no;

    if (rjs_value_get_gc_thing_type(rt, o) != RJS_GC_THING_NATIVE_OBJECT)
        return NULL;

    no = (RJS_NativeObject*)rjs_value_get_object(rt, o);

    return no->tag;
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
    RJS_NativeObject *no;

    if (rjs_value_get_gc_thing_type(rt, o) != RJS_GC_THING_NATIVE_OBJECT)
        return NULL;

    no = (RJS_NativeObject*)rjs_value_get_object(rt, o);

    return no->native_data.data;
}
