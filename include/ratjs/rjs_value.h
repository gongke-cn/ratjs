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

/**
 * \file
 * Generic value.
 */

#ifndef _RJS_VALUE_H_
#define _RJS_VALUE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup value Value
 * Generic value
 * @{
 */

/**Value tag of undefined.*/
#define RJS_VALUE_TAG_UNDEFINED    0x7ff9
/**Value tag of null.*/
#define RJS_VALUE_TAG_NULL         0x7ffa
/**Value tag of boolean.*/
#define RJS_VALUE_TAG_BOOLEAN      0x7ffb
/**Value tag of string.*/
#define RJS_VALUE_TAG_STRING       0x7ffc
/**Value tag of symbol.*/
#define RJS_VALUE_TAG_SYMBOL       0x7ffd
/**Value tag of big integer.*/
#define RJS_VALUE_TAG_BIG_INT      0x7ffe
/**Value tag of object.*/
#define RJS_VALUE_TAG_OBJECT       0xfff9
/**Value tag of GC thing.*/
#define RJS_VALUE_TAG_GC_THING     0xfffa
/**Index string.*/
#define RJS_VALUE_TAG_INDEX_STRING 0xfffb

/**
 * Check if the value is in the native stack.
 * \param v The value.
 * \retval RJS_TRUE The value is in the native stack.
 * \retval RJS_FALSE The value is not in the native stack.
 */
static inline RJS_Bool
rjs_value_is_stack_pointer (RJS_Value *v)
{
    return RJS_PTR2SIZE(v) & 1;
}

/**
 * Convert the value to native stack poiner.
 * \param v The value.
 * \return The native stack pointer.
 */
static inline size_t
rjs_value_to_stack_pointer (RJS_Value *v)
{
    return RJS_PTR2SIZE(v) >> 1;
}

/**
 * Convert the native stack pointer to value.
 * \param sp The native stack pointer.
 * \return The value.
 */
static inline RJS_Value*
rjs_stack_pointer_to_value (size_t sp)
{
    return RJS_SIZE2PTR((sp << 1) | 1);
}

/**
 * Get the real value pointer.
 * \param rt The current runtime.
 * \param v The value.
 * \return The real value pointer.
 */
static inline RJS_Value*
rjs_value_get_pointer (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;

    if (rjs_value_is_stack_pointer(v)) {
        size_t sp = rjs_value_to_stack_pointer(v);

        assert(sp < rb->curr_native_stack->value.item_num);

        return rb->curr_native_stack->value.items + sp;
    } else {
        return v;
    }
}

/**
 * Get the value buffer's item.
 * \param rt The current runtime.
 * \param v The value buffer's first item.
 * \param id The item's index.
 * \return The value item.
 */
static inline RJS_Value*
rjs_value_buffer_item (RJS_Runtime *rt, RJS_Value *v, size_t id)
{
    if (rjs_value_is_stack_pointer(v)) {
        size_t sp = rjs_value_to_stack_pointer(v);

        sp += id;

        return rjs_stack_pointer_to_value(sp);
    } else {
        return v + id;
    }
}

/**
 * Get the value tag.
 * \param v The value's pointer.
 * \return The tag of the value.
 */
static inline int
rjs_value_get_tag (RJS_Value *v)
{
    return ((*v) >> 48) & 0xffff;
}

/**Create a value from tag.*/
static inline RJS_Value
rjs_value_from_tag (int tag)
{
    return ((RJS_Value)tag) << 48;
}

/**
 * Get the value type.
 * \param rt The current runtime.
 * \param v The value's pointer.
 * \return The value's type.
 */
static inline RJS_ValueType
rjs_value_get_type (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_ValueType type;

    v = rjs_value_get_pointer(rt, v);

    switch (rjs_value_get_tag(v)) {
    case RJS_VALUE_TAG_UNDEFINED:
        type = RJS_VALUE_UNDEFINED;
        break;
    case RJS_VALUE_TAG_NULL:
        type = RJS_VALUE_NULL;
        break;
    case RJS_VALUE_TAG_BOOLEAN:
        type = RJS_VALUE_BOOLEAN;
        break;
    case RJS_VALUE_TAG_STRING:
    case RJS_VALUE_TAG_INDEX_STRING:
        type = RJS_VALUE_STRING;
        break;
    case RJS_VALUE_TAG_SYMBOL:
        type = RJS_VALUE_SYMBOL;
        break;
    case RJS_VALUE_TAG_BIG_INT:
        type = RJS_VALUE_BIG_INT;
        break;
    case RJS_VALUE_TAG_OBJECT:
        type = RJS_VALUE_OBJECT;
        break;
    case RJS_VALUE_TAG_GC_THING:
        type = RJS_VALUE_GC_THING;
        break;
    default:
        type = RJS_VALUE_NUMBER;
        break;
    }

    return type;
}

/**
 * Set the value as undefined.
 * \param rt The current runtime.
 * \param v The to be set.
 */
static inline void
rjs_value_pointer_set_undefined (RJS_Runtime *rt, RJS_Value *v)
{
    *v = rjs_value_from_tag(RJS_VALUE_TAG_UNDEFINED);
}

/**
 * Set the value as undefined.
 * \param rt The current runtime.
 * \param v The to be set.
 */
static inline void
rjs_value_set_undefined (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    rjs_value_pointer_set_undefined(rt, v);
}

/**
 * Fill the value buffer with undefined.
 * \param rt The current runtime.
 * \param v The value buffer's pointer.
 * \param n Number of values in the buffer.
 */
extern void
rjs_value_buffer_fill_undefined (RJS_Runtime *rt, RJS_Value *v, size_t n);

/**
 * Set the value as null.
 * \param rt The current runtime.
 * \param v The value to be set.
 */
static inline void
rjs_value_set_null (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    *v = rjs_value_from_tag(RJS_VALUE_TAG_NULL);
}

/**
 * Set the value as boolean value.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param b The boolean value.
 */
static inline void
rjs_value_set_boolean (RJS_Runtime *rt, RJS_Value *v, RJS_Bool b)
{
    v = rjs_value_get_pointer(rt, v);

    *v = rjs_value_from_tag(RJS_VALUE_TAG_BOOLEAN) | (b ? RJS_TRUE : RJS_FALSE);
}

/**
 * Set the value as number.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param n The number value.
 */
static inline void
rjs_value_set_number (RJS_Runtime *rt, RJS_Value *v, RJS_Number n)
{
    v = rjs_value_get_pointer(rt, v);

    *(RJS_Number*)v = n;
}

/**
 * Set the value as GC managed thing with tag.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param p The GC thing's pointer.
 * \param tag The value's tag.
 */
static inline void
rjs_value_set_gc_thing_tag (RJS_Runtime *rt, RJS_Value *v, void *p, int tag)
{
    v = rjs_value_get_pointer(rt, v);

    *v = rjs_value_from_tag(tag) | (RJS_PTR2SIZE(p) & 0xffffffffffff);
}

/**
 * Set the value as a string.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param s The string.
 */
static inline void
rjs_value_set_string (RJS_Runtime *rt, RJS_Value *v, RJS_String *s)
{
    rjs_value_set_gc_thing_tag(rt, v, s, RJS_VALUE_TAG_STRING);
}

/**
 * Set the value as an index string.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param idx The index value.
 */
static inline void
rjs_value_set_index_string (RJS_Runtime *rt, RJS_Value *v, uint32_t idx)
{
    v = rjs_value_get_pointer(rt, v);

    *v = rjs_value_from_tag(RJS_VALUE_TAG_INDEX_STRING) | idx;
}

/**
 * Set the value as a symbol.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param s The symbol.
 */
static inline void
rjs_value_set_symbol (RJS_Runtime *rt, RJS_Value *v, RJS_Symbol *s)
{
    rjs_value_set_gc_thing_tag(rt, v, s, RJS_VALUE_TAG_SYMBOL);
}

#if ENABLE_BIG_INT
/**
 * Set the value as a big integer.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param bi The big integer.
 */
static inline void
rjs_value_set_big_int (RJS_Runtime *rt, RJS_Value *v, RJS_BigInt *bi)
{
    rjs_value_set_gc_thing_tag(rt, v, bi, RJS_VALUE_TAG_BIG_INT);
}
#endif /*ENABLE_BIG_INT*/

/**
 * Set the value as an object.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param o The object.
 */
static inline void
rjs_value_set_object (RJS_Runtime *rt, RJS_Value *v, RJS_Object *o)
{
    rjs_value_set_gc_thing_tag(rt, v, o, RJS_VALUE_TAG_OBJECT);
}

/**
 * Set the value as GC managed thing.
 * \param rt The current runtime.
 * \param v The value to be set.
 * \param p The GC thing's pointer.
 */
static inline void
rjs_value_set_gc_thing (RJS_Runtime *rt, RJS_Value *v, void *p)
{
    rjs_value_set_gc_thing_tag(rt, v, p, RJS_VALUE_TAG_GC_THING);
}

/**
 * Get the boolean value from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The boolean value.
 */
static inline RJS_Bool
rjs_value_get_boolean (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    return (*v) & 1 ? RJS_TRUE : RJS_FALSE;
}

/**
 * Get the number from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The number value.
 */
static inline RJS_Number
rjs_value_get_number (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    return *(RJS_Number*)v;
}

/**
 * Get the GC thing from the generic value's pointer.
 * \param rt The current runtime.
 * \param v The generic value pointer.
 * \return The GC thing's pointer.
 */
static inline void*
rjs_value_pointer_get_gc_thing (RJS_Runtime *rt, RJS_Value *v)
{
#if __SIZEOF_POINTER__ == 8
    size_t p = *v & 0xffffffffffffull;

    if (p & 0x800000000000ull) {
        p |= 0xffff000000000000ull;
    }
#else
    size_t p = *v & 0xffffffff;
#endif

    return RJS_SIZE2PTR(p);
}

/**
 * Get the GC thing from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The GC thing's pointer.
 */
static inline void*
rjs_value_get_gc_thing (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    return rjs_value_pointer_get_gc_thing(rt, v);
}

/**
 * \cond 
 */

/**
 * Convert an index string to normal string.
 * \param rt The current runtime.
 * \param v The index string value, and return the normal string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_index_string_normalize (RJS_Runtime *rt, RJS_Value *v);

/**
 * \endcond
 */

/**
 * Get the string from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The string's pointer.
 */
static inline RJS_String*
rjs_value_get_string (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    if (rjs_value_get_tag(v) == RJS_VALUE_TAG_INDEX_STRING)
        rjs_index_string_normalize(rt, v);

    return rjs_value_pointer_get_gc_thing(rt, v);
}

/**
 * Get the index string from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The index value.
 */
static inline uint32_t
rjs_value_get_index_string (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    return *v & 0xffffffff;
}

/**
 * Get the symbol from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The symbol's pointer.
 */
static inline RJS_Symbol*
rjs_value_get_symbol (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing(rt, v);
}

#if ENABLE_BIG_INT
/**
 * Get the big integer from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The big integer pointer.
 */
static inline RJS_BigInt*
rjs_value_get_big_int (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing(rt, v);
}
#endif /*ENABLE_BIG_INT*/

/**
 * Get the object from the generic value.
 * \param rt The current runtime.
 * \param v The generic value.
 * \return The big integer pointer.
 */
static inline RJS_Object*
rjs_value_get_object (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing(rt, v);
}

/**
 * Get the GC thing's type.
 * \param rt The current runtime.
 * \param v The value.
 * \return The GC thing's type.
 * \retval -1 The value is not a GC thing.
 */
static inline RJS_GcThingType
rjs_value_get_gc_thing_type (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_GcThing *gt;
    int          tag;

    v   = rjs_value_get_pointer(rt, v);
    tag = rjs_value_get_tag(v);

    if ((tag != RJS_VALUE_TAG_GC_THING)
            && (tag != RJS_VALUE_TAG_OBJECT)
            && (tag != RJS_VALUE_TAG_STRING)
            && (tag != RJS_VALUE_TAG_SYMBOL)
            && (tag != RJS_VALUE_TAG_BIG_INT))
        return -1;

    gt = rjs_value_pointer_get_gc_thing(rt, v);

    return gt->ops->type;
}

/**
 * Check if the value is undefined.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is undefined.
 * \retval RJS_FALSE The value is not undefined.
 */
static inline RJS_Bool
rjs_value_is_undefined (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_UNDEFINED;
}

/**
 * Check if the value is null.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is null.
 * \retval RJS_FALSE The value is not null.
 */
static inline RJS_Bool
rjs_value_is_null (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_NULL;
}

/**
 * Check if the value is a boolean value.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a boolean value.
 * \retval RJS_FALSE The value is not a boolean value.
 */
static inline RJS_Bool
rjs_value_is_boolean (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_BOOLEAN;
}

/**
 * Check if the value is a number.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a number.
 * \retval RJS_FALSE The value is not a number.
 */
static inline RJS_Bool
rjs_value_is_number (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_NUMBER;
}

/**
 * Check if the value is a string.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a string.
 * \retval RJS_FALSE The value is not a string.
 */
static inline RJS_Bool
rjs_value_is_string (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_STRING;
}

/**
 * Check if the value is an index string.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is an index string.
 * \retval RJS_FALSE The value is not an index string.
 */
static inline RJS_Bool
rjs_value_is_index_string (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    return rjs_value_get_tag(v) == RJS_VALUE_TAG_INDEX_STRING;
}

/**
 * Check if the value is a symbol.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a symbol.
 * \retval RJS_FALSE The value is not a symbol.
 */
static inline RJS_Bool
rjs_value_is_symbol (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_SYMBOL;
}

#if ENABLE_BIG_INT
/**
 * Check if the value is a big integer.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a big integer.
 * \retval RJS_FALSE The value is not a big integer.
 */
static inline RJS_Bool
rjs_value_is_big_int (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_BIG_INT;
}
#endif /*ENABLE_BIG_INT*/

/**
 * Check if the value is an object.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is an object.
 * \retval RJS_FALSE The value is not an object.
 */
static inline RJS_Bool
rjs_value_is_object (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_type(rt, v) == RJS_VALUE_OBJECT;
}

#if ENABLE_SCRIPT || ENABLE_MODULE

/**
 * Check if the value is a script.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a script.
 * \retval RJS_FALSE The value is not a script.
 */
static inline RJS_Bool
rjs_value_is_script (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_GcThingType gtt = rjs_value_get_gc_thing_type(rt, v);

    return (gtt == RJS_GC_THING_SCRIPT) || (gtt == RJS_GC_THING_MODULE);
}

#endif /*ENABLE_SCRIPT || ENABLE_MODULE*/

#if ENABLE_MODULE

/**
 * Check if the value is a module.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a module.
 * \retval RJS_FALSE The value is not a module.
 */
static inline RJS_Bool
rjs_value_is_module (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_MODULE;
}

#endif /*ENABLE_MODULE*/

#if ENABLE_PRIV_NAME

/**
 * Check if the value is a private name.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a private name.
 * \retval RJS_FALSE The value is not a private name.
 */
static inline RJS_Bool
rjs_value_is_private_name (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_PRIVATE_NAME;
}

#endif /*ENABLE_PRIV_NAME*/

/**
 * Check if the value is a promise.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a promise.
 * \retval RJS_FALSE The value is not a promise.
 */
static inline RJS_Bool
rjs_value_is_promise (RJS_Runtime *rt, RJS_Value *v)
{
    return rjs_value_get_gc_thing_type(rt, v) == RJS_GC_THING_PROMISE;
}

/**
 * Scan the referenced things in a value.
 * \param rt The current runtime.
 * \param v The value to be scanned.
 */
static inline void
rjs_gc_scan_value (RJS_Runtime *rt, RJS_Value *v)
{
    v = rjs_value_get_pointer(rt, v);

    switch (rjs_value_get_tag(v)) {
    case RJS_VALUE_TAG_STRING:
    case RJS_VALUE_TAG_SYMBOL:
#if ENABLE_BIG_INT
    case RJS_VALUE_TAG_BIG_INT:
#endif /*ENABLE_BIG_INT*/
    case RJS_VALUE_TAG_OBJECT:
    case RJS_VALUE_TAG_GC_THING:
        rjs_gc_mark(rt, rjs_value_get_gc_thing(rt, v));
        break;
    default:
        break;
    }
}

/**
 * Copy the value.
 * \param rt The current runtime.
 * \param d The destination value.
 * \param s The source value.
 */
static inline void
rjs_value_copy (RJS_Runtime *rt, RJS_Value *d, RJS_Value *s)
{
    d = rjs_value_get_pointer(rt, d);
    s = rjs_value_get_pointer(rt, s);

    *d = *s;
}

/**
 * Copy the values in the buffer.
 * \param rt The current runtime.
 * \param d The destination value buffer.
 * \param s The source value buffer.
 * \param n Number of values to be copyed.
 */
static inline void
rjs_value_buffer_copy (RJS_Runtime *rt, RJS_Value *d, RJS_Value *s, size_t n)
{
    d = rjs_value_get_pointer(rt, d);
    s = rjs_value_get_pointer(rt, s);

    RJS_ELEM_CPY(d, s, n);
}

/**
 * Scan the referenced things in a value buffer.
 * \param rt The current runtime.
 * \param v The value buffer.
 * \param n Number of values in the buffer.
 */
extern void
rjs_gc_scan_value_buffer (RJS_Runtime *rt, RJS_Value *v, size_t n);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

