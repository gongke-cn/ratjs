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
 * Operation.
 */

#ifndef _RJS_OPERATION_H_
#define _RJS_OPERATION_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup operation Operation
 * Abstract operations
 * @{
 */

/**Integrity level.*/
typedef enum {
    RJS_INTEGRITY_SEALED, /**< Set configurable to false.*/
    RJS_INTEGRITY_FROZEN  /**< Set configurable and writable to false.*/
} RJS_IntegrityLevel;

/**
 * Check if the machine is little endian.
 * \retval RJS_TRUE Is little endian.
 * \retval RJS_FALSE Is big endian.
 */
static inline RJS_Bool
rjs_is_little_endian (void)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return RJS_TRUE;
#else
    return RJS_FALSE;
#endif
}

/**
 * Check if the value is not null nor undefined.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_OK The value is not null nor undefined.
 * \retval RJS_ERR The value is null or undefined.
 */
static inline RJS_Result
rjs_require_object_coercible (RJS_Runtime *rt, RJS_Value *v)
{
    if (rjs_value_is_undefined(rt, v) || rjs_value_is_null(rt, v))
        return rjs_throw_type_error(rt, _("the value is null or undefined"));

    return RJS_OK;
} 

/**
 * Check if the number is an integral number.
 * \param n The number.
 * \retval RJS_TRUE n is an integral number.
 * \retval RJS_FALSE n is not an integral number.
 */
static inline RJS_Bool
rjs_is_integral_number (RJS_Number n)
{
    RJS_Number a;

    if (isnan(n) || isinf(n))
        return RJS_FALSE;

    a = fabs(n);
    return floor(a) == a;
}

/**
 * Check if the value is a valid property key value.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a valid property key.
 * \retval RJS_FALSE The value is not a valid property key.
 */
static inline RJS_Bool
rjs_is_property_key (RJS_Runtime *rt, RJS_Value *v)
{
    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_STRING:
    case RJS_VALUE_SYMBOL:
        return RJS_TRUE;
    default:
        return RJS_FALSE;
    }
}

/**
 * Check if the value is an array.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE v is an array.
 * \retval RJS_FALSE v is not an array.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_is_array (RJS_Runtime *rt, RJS_Value *v);

/**
 * Check if the value is callable.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is callable.
 * \retval RJS_FALSE The value is not callback.
 */
static inline RJS_Bool
rjs_is_callable (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Object    *o;
    RJS_ObjectOps *ops;

    if (!rjs_value_is_object(rt, v))
        return RJS_FALSE;

    o   = rjs_value_get_object(rt, v);
    ops = (RJS_ObjectOps*)((RJS_GcThing*)o)->ops;

    return ops->call ? RJS_TRUE : RJS_FALSE;
}

/**
 * Check if the value is a constructor.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a constructor.
 * \retval RJS_FALSE The value is not a constructor.
 */
static inline RJS_Bool
rjs_is_constructor (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Object    *o;
    RJS_ObjectOps *ops;

    if (!rjs_value_is_object(rt, v))
        return RJS_FALSE;

    o   = rjs_value_get_object(rt, v);
    ops = (RJS_ObjectOps*)((RJS_GcThing*)o)->ops;

    return ops->construct ? RJS_TRUE : RJS_FALSE;
}

/**
 * Check if the property descriptor is an accessor descriptor.
 * \param d The property descriptor.
 * \retval RJS_TRUE The descriptor is an accessor descriptor.
 * \retval RJS_FALSE The descriptor is not an accessor descriptor.
 */
static inline RJS_Bool
rjs_is_accessor_descriptor (RJS_PropertyDesc *d)
{
    if (!d)
        return RJS_FALSE;

    if (d->flags & (RJS_PROP_FL_HAS_GET|RJS_PROP_FL_HAS_SET))
        return RJS_TRUE;

    return RJS_FALSE;
}

/**
 * Check if the property descriptor is a data descriptor.
 * \param d The property descriptor.
 * \retval RJS_TRUE The descriptor is a data descriptor.
 * \retval RJS_FALSE The descriptor is not a data descriptor.
 */
static inline RJS_Bool
rjs_is_data_descriptor (RJS_PropertyDesc *d)
{
    if (!d)
        return RJS_FALSE;

    if (d->flags & (RJS_PROP_FL_HAS_WRITABLE|RJS_PROP_FL_HAS_VALUE))
        return RJS_TRUE;

    return RJS_FALSE;
}

/**
 * Check if the property descriptor is a generic descriptor.
 * \param d The property descriptor.
 * \retval RJS_TRUE The descriptor is a generic descriptor.
 * \retval RJS_FALSE The descriptor is not a generic descriptor.
 */
static inline RJS_Bool
rjs_is_generic_descriptor (RJS_PropertyDesc *d)
{
    if (!d)
        return RJS_FALSE;

    if (rjs_is_accessor_descriptor(d))
        return RJS_FALSE;

    if (rjs_is_data_descriptor(d))
        return RJS_FALSE;

    return RJS_TRUE;
}

/**
 * Check if the value is a regular expression.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE The value is a regular expression.
 * \retval RJS_FALSE The value is not a regular expression.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_is_regexp (RJS_Runtime *rt, RJS_Value *v);

/**
 * Check if 2 property descriptors are compatible.
 * \param rt The current runtime.
 * \param ext The object is extensible.
 * \param desc The new descriptor.
 * \param curr The current descriptor.
 * \retval RJS_TRUE The descriptors are compatible.
 * \retval RJS_FALSE The descriptors are not compatible.
 */
extern RJS_Result
rjs_is_compatible_property_descriptor (RJS_Runtime *rt, RJS_Bool ext, RJS_PropertyDesc *desc, RJS_PropertyDesc *curr);

/**
 * \cond
 */

/**
 * Check if 2 non-numeric values are equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 and v2 are same.
 * \retval RJS_FALSE v1 and v2 are not same.
 */
static inline RJS_Bool
rjs_same_value_non_numeric (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_Bool r;

    switch (rjs_value_get_type(rt, v1)) {
    case RJS_VALUE_UNDEFINED:
    case RJS_VALUE_NULL:
        r = RJS_TRUE;
        break;
    case RJS_VALUE_STRING:
        r = rjs_string_equal(rt, v1, v2);
        break;
    case RJS_VALUE_BOOLEAN:
        r = rjs_value_get_boolean(rt, v1)
                == rjs_value_get_boolean(rt, v2);
        break;
    case RJS_VALUE_SYMBOL:
    case RJS_VALUE_OBJECT:
        r = rjs_value_get_gc_thing(rt, v1)
                == rjs_value_get_gc_thing(rt, v2);
        break;
    default:
        assert(0);
        r = RJS_FALSE;
    }

    return r;
}

/**
 * Check if 2 number values are equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 */
static inline RJS_Bool
rjs_number_same_value (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);

    if (isnan(n1) && isnan(n2))
        return RJS_TRUE;

    if (signbit(n1) != signbit(n2))
        return RJS_FALSE;

    return n1 == n2;
}

/**
 * Check if 2 number values are equal (+0 == -0).
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 */
static inline RJS_Bool
rjs_number_same_value_0 (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_Number n1 = rjs_value_get_number(rt, v1);
    RJS_Number n2 = rjs_value_get_number(rt, v2);

    if (isnan(n1) && isnan(n2))
        return RJS_TRUE;

    return n1 == n2;
}

/**
 * Check if 2 big integer values are equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 == v2.
 * \retval RJS_FALSE v1 != v2.
 */
extern RJS_Bool
rjs_big_int_same_value (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2);

/**
 * Check if 2 big integer values are equal. (_0 == -0)
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 == v2.
 * \retval RJS_FALSE v1 != v2.
 */
extern RJS_Bool
rjs_big_int_same_value_0 (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2);

/**
 * Check if the big integer is zero.
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_TRUE v is 0.
 * \retval RJS_FALSE v is not 0.
 */
extern RJS_Bool
rjs_big_int_is_0 (RJS_Runtime *rt, RJS_Value *v);

/**
 * \endcond
 */

/**
 * Check if 2 values are equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 and v2 are same.
 * \retval RJS_FALSE v1 and v2 are not same.
 */
static inline RJS_Bool
rjs_same_value (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_ValueType t1, t2;

    t1 = rjs_value_get_type(rt, v1);
    t2 = rjs_value_get_type(rt, v2);

    if (t1 != t2)
        return RJS_FALSE;

    if (t1 == RJS_VALUE_NUMBER)
        return rjs_number_same_value(rt, v1, v2);

#if ENABLE_BIG_INT
    if (t1 == RJS_VALUE_BIG_INT)
        return rjs_big_int_same_value(rt, v1, v2);
#endif /*ENABLE_BIG_INT*/

    return rjs_same_value_non_numeric(rt, v1, v2);
}

/**
 * Check if 2 values are equal. (+0 = -0)
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 and v2 are same.
 * \retval RJS_FALSE v1 and v2 are not same.
 */
static inline RJS_Bool
rjs_same_value_0 (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    RJS_ValueType t1, t2;

    t1 = rjs_value_get_type(rt, v1);
    t2 = rjs_value_get_type(rt, v2);

    if (t1 != t2)
        return RJS_FALSE;

    if (t1 == RJS_VALUE_NUMBER)
        return rjs_number_same_value_0(rt, v1, v2);

#if ENABLE_BIG_INT
    if (t1 == RJS_VALUE_BIG_INT)
        return rjs_big_int_same_value_0(rt, v1, v2);
#endif /*ENABLE_BIG_INT*/

    return rjs_same_value_non_numeric(rt, v1, v2);
}

/**
 * Convert the value to boolean.
 * \param rt The current runtime.
 * \param v The value.
 * \return The boolean value.
 */
static inline RJS_Bool
rjs_to_boolean (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Bool b;

    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_UNDEFINED:
    case RJS_VALUE_NULL:
        b = RJS_FALSE;
        break;
    case RJS_VALUE_BOOLEAN:
        b = rjs_value_get_boolean(rt, v);
        break;
    case RJS_VALUE_NUMBER: {
        RJS_Number n = rjs_value_get_number(rt, v);

        b = !((n == 0) || isnan(n));
        break;
    }
    case RJS_VALUE_STRING:
        if (rjs_value_is_index_string(rt, v))
            b = RJS_TRUE;
        else
            b = rjs_string_get_length(rt, v) ? RJS_TRUE : RJS_FALSE;
        break;
    case RJS_VALUE_SYMBOL:
        b = RJS_TRUE;
        break;
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
        b = !rjs_big_int_is_0(rt, v);
        break;
#endif /*ENABLE_BIG_INT*/
    case RJS_VALUE_OBJECT:
        b = RJS_TRUE;
        break;
    default:
        assert(0);
        b = RJS_TRUE;
        break;
    }

    return b;
}

/**
 * Convert the value to primitive type.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] prim Return the primitive type.
 * \param type Prefered value type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_primitive (RJS_Runtime *rt, RJS_Value *v, RJS_Value *prim, RJS_ValueType type)
{
    RJS_Result r;

    if (rjs_value_is_object(rt, v)) {
        r = rjs_object_to_primitive(rt, v, prim, type);
    } else {
        rjs_value_copy(rt, prim, v);
        r = RJS_OK;
    }

    return r;
}

/**
 * \cond
 */
/**
 * Convert the value which is not an object to object.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] o Return the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_to_object_non_object (RJS_Runtime *rt, RJS_Value *v, RJS_Value *o);
/**
 * \endcond
 */


/**
 * Convert the value to object.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] o Return the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_object (RJS_Runtime *rt, RJS_Value *v, RJS_Value *o)
{
    RJS_Result r;

    if (rjs_value_is_object(rt, v)) {
        rjs_value_copy(rt, o, v);
        r = RJS_OK;
    } else {
        r = rjs_to_object_non_object(rt, v, o);
    }

    return r;
}


/**
 * \cond
 */
/**
 * Convert the value which is not a string to string.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] s Return the result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_to_string_non_string (RJS_Runtime *rt, RJS_Value *v, RJS_Value *s);
/**
 * \endcond
 */

/**
 * Convert the value to string.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] s Return the result string.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_string (RJS_Runtime *rt, RJS_Value *v, RJS_Value *s)
{
    RJS_Result r;

    if (rjs_value_is_string(rt, v)) {
        rjs_value_copy(rt, s, v);
        r = RJS_OK;
    } else {
        r = rjs_to_string_non_string(rt, v, s);
    }

    return r;
}

/**
 * Convert the value's description to encoded characters.
 * \param rt The current runtime.
 * \param v The value.
 * \param cb The characters buffer.
 * \param enc The encoding.
 * \return The pointer of the characters.
 */
extern const char*
rjs_to_desc_chars (RJS_Runtime *rt, RJS_Value *v, RJS_CharBuffer *cb, const char *enc);

/**
 * Convert the value to property key.
 * \param rt The current runtime.
 * \param v The input value.
 * \param[out] key Return the property key.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_property_key (RJS_Runtime *rt, RJS_Value *v, RJS_Value *key)
{
    RJS_Result r;
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *prim = rjs_value_stack_push(rt);

    if ((r = rjs_to_primitive(rt, v, prim, RJS_VALUE_STRING)) == RJS_ERR)
        goto end;

    if (rjs_value_is_symbol(rt, prim))
        rjs_value_copy(rt, key, prim);
    else
        r = rjs_to_string(rt, prim, key);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Convert the value to a number.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pn Return the result number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_number (RJS_Runtime *rt, RJS_Value *v, RJS_Number *pn)
{
    RJS_Number n;

    switch (rjs_value_get_type(rt, v)) {
    case RJS_VALUE_UNDEFINED:
        n = NAN;
        break;
    case RJS_VALUE_NULL:
        n = 0;
        break;
    case RJS_VALUE_BOOLEAN:
        n = rjs_value_get_boolean(rt, v) ? 1 : 0;
        break;
    case RJS_VALUE_NUMBER:
        n = rjs_value_get_number(rt, v);
        break;
    case RJS_VALUE_STRING:
        if (rjs_value_is_index_string(rt, v))
            n = rjs_value_get_index_string(rt, v);
        else
            n = rjs_string_to_number(rt, v);
        break;
    case RJS_VALUE_SYMBOL:
        return rjs_throw_type_error(rt, _("symbol cannot be converted to number"));
#if ENABLE_BIG_INT
    case RJS_VALUE_BIG_INT:
        return rjs_throw_type_error(rt, _("big integer cannot be converted to number directly"));
#endif /*ENABLE_BIG_INT*/
    case RJS_VALUE_OBJECT:
        return rjs_object_to_number(rt, v, pn);
    default:
        assert(0);
        n = 0;
    }

    *pn = n;
    return RJS_OK;
}

/**
 * Convert the value to numeric value.
 * \param rt The current runtime.
 * \param v The input value.
 * \param[out] rv Return the numeric value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_numeric (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    RJS_Result r;
    RJS_Number n;

#if ENABLE_BIG_INT
    if ((r = rjs_to_primitive(rt, v, rv, RJS_VALUE_NUMBER)) == RJS_ERR)
        return r;

    if (rjs_value_is_big_int(rt, rv))
        return RJS_OK;

    if ((r = rjs_to_number(rt, rv, &n)) == RJS_ERR)
        return r;
#else /*!ENABLE_BIG_INT*/
    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;
#endif /*ENABLE_BIG_INT*/

    rjs_value_set_number(rt, rv, n);
    return RJS_OK;
}

/**
 * Convert the value to integer or infinity.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pn Return the result number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_integer_or_infinity (RJS_Runtime *rt, RJS_Value *v, RJS_Number *pn)
{
    RJS_Result r;

    if ((r = rjs_to_number(rt, v, pn)) == RJS_ERR)
        return r;

    if (isnan(*pn)) {
        *pn = 0;
    } else if (!isinf(*pn)) {
        if (*pn < 0)
            *pn = - floor(-*pn);
        else
            *pn = floor(*pn);

        if (*pn == 0)
            *pn = 0;
    }

    return RJS_OK;
}

/**
 * Convert the value to 8 bits signed integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_int8 (RJS_Runtime *rt, RJS_Value *v, int8_t *pi)
{
    RJS_Result r;
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = (int64_t)n;
    }

    return RJS_OK;
}

/**
 * Convert the value to 8 bits unsigned integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_uint8 (RJS_Runtime *rt, RJS_Value *v, uint8_t *pi)
{
    RJS_Result r;
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = (int64_t)n;
    }

    return RJS_OK;
}

/**
 * Convert the value to 8 bits unsigned integer (clamp conversion).
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_uint8_clamp (RJS_Runtime *rt, RJS_Value *v, uint8_t *pi)
{
    RJS_Result r;
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n))
        *pi = 0;
    else if (n < 0)
        *pi = 0;
    else if (n > 255)
        *pi = 0xff;
    else {
        RJS_Number f = floor(n);

        if (f + 0.5 < n)
            *pi = f + 1;
        else if (n < f + 0.5)
            *pi = f;
        else if (((int)f) & 1)
            *pi = f + 1;
        else
            *pi = f;
    }

    return RJS_OK;
}

/**
 * Convert the value to 16 bits signed integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_int16 (RJS_Runtime *rt, RJS_Value *v, int16_t *pi)
{
    RJS_Result r;
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = (int64_t)n;
    }

    return RJS_OK;
}

/**
 * Convert the value to 16 bits unsigned integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_uint16 (RJS_Runtime *rt, RJS_Value *v, uint16_t *pi)
{
    RJS_Result r;
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = (int64_t)n;
    }

    return RJS_OK;
}

/**
 * Convert the value to 32 bits signed integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_int32 (RJS_Runtime *rt, RJS_Value *v, int32_t *pi)
{
    RJS_Result r;
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = (int64_t)n;
    }

    return RJS_OK;
}

/**
 * Convert the value to 32 bits unsigned integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_uint32 (RJS_Runtime *rt, RJS_Value *v, uint32_t *pi)
{
    RJS_Result r;
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = (int64_t)n;
    }

    return RJS_OK;
}

/**
 * \cond
 */

/**
 * Convert the value to big intwger.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] bi The big integer.
 */
extern RJS_Result
rjs_to_big_int (RJS_Runtime *rt, RJS_Value *v, RJS_Value *bi);

/**
 * Convert the big integer to 64 bits signed integer number.
 * \param rt The current runtime.
 * \param v The big integer.
 * \param[out] pi Return the 64 bits signed integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_to_int64 (RJS_Runtime *rt, RJS_Value *v, int64_t *pi);

/**
 * Convert the big integer to 64 bits unsigned integer number.
 * \param rt The current runtime.
 * \param v The big integer.
 * \param[out] pi Return the 64 bits unsigned integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_big_int_to_uint64 (RJS_Runtime *rt, RJS_Value *v, uint64_t *pi);

/**
 * \endcond
 */

/**
 * Convert the value to 64 bits signed integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_big_int64 (RJS_Runtime *rt, RJS_Value *v, int64_t *pi)
{
    RJS_Result r;

#if ENABLE_BIG_INT
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *bi  = rjs_value_stack_push(rt);

    r = rjs_to_big_int(rt, v, bi);
    if (r == RJS_OK) {
        rjs_big_int_to_int64(rt, bi, pi);
    }

    rjs_value_stack_restore(rt, top);
#else
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = n;
    }

    r = RJS_OK;
#endif

    return r;
}

/**
 * Convert the value to 64 bits unsigned integer.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the integer number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_big_uint64 (RJS_Runtime *rt, RJS_Value *v, uint64_t *pi)
{
    RJS_Result r;

#if ENABLE_BIG_INT
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *bi  = rjs_value_stack_push(rt);

    r = rjs_to_big_int(rt, v, bi);
    if (r == RJS_OK) {
        rjs_big_int_to_uint64(rt, bi, pi);
    }

    rjs_value_stack_restore(rt, top);
#else
    RJS_Number n;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (isnan(n) || isinf(n) || (n == 0)) {
        *pi = 0;
    } else {
        *pi = n;
    }

    r = RJS_OK;
#endif

    return r;
}

/**
 * Convert the value to length.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pl Return the length.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_to_length (RJS_Runtime *rt, RJS_Value *v, int64_t *pl)
{
    RJS_Number n;
    RJS_Result r;
    int64_t    l;

    if ((r = rjs_to_integer_or_infinity(rt, v, &n)) == RJS_ERR)
        return r;

    if (n <= 0)
        l = 0;
    else
        l = RJS_MIN(n, RJS_MAX_INT);

    *pl = l;

    return RJS_OK;
}

/**
 * Convert the value to index.
 * \param rt The current runtime.
 * \param v The value.
 * \param[out] pi Return the index.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_to_index (RJS_Runtime *rt, RJS_Value *v, int64_t *pi);

/**
 * Convert the object to property descriptor.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] pd Return the property descriptor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_to_property_descriptor (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyDesc *pd);

/**
 * Create an object from a property descritptor.
 * \param rt The current runtime.
 * \param pd The property descriptor.
 * \param[out] v Return the object value or undefined.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_from_property_descriptor (RJS_Runtime *rt, RJS_PropertyDesc *pd, RJS_Value *v);

/**
 * Complete the property descriptor.
 * \param rt The current runtime.
 * \param pd The property descriptor.
 */
extern void
rjs_complete_property_descriptor (RJS_Runtime *rt, RJS_PropertyDesc *pd);

/**
 * Get the length of an array like object.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] pl Return the length.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_length_of_array_like (RJS_Runtime *rt, RJS_Value *o, int64_t *pl);

/**
 * Get the object's property.
 * \param rt The current runtime.
 * \param o The object value.
 * \param pn The property name.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_get (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv)
{
    return rjs_object_get(rt, o, pn, o, pv);
}

/**
 * Get the object's property by the array item index.
 * \param rt The current runtime.
 * \param o The object value.
 * \param idx The index.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_get_index (RJS_Runtime *rt, RJS_Value *o, int64_t idx, RJS_Value *pv)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *nv   = rjs_value_stack_push(rt);
    RJS_Value       *idxv = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((idx >= 0) && (idx < 0xffffffff)) {
        rjs_value_set_index_string(rt, idxv, idx);
    } else {
        rjs_value_set_number(rt, nv, idx);
        rjs_to_string(rt, nv, idxv);
    }

    rjs_property_name_init(rt, &pn, idxv);
    r = rjs_get(rt, o, &pn, pv);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get the value's property.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The property name.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_get_v (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *pv)
{
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *o   = rjs_value_stack_push(rt);
    RJS_Result  r;

    if ((r = rjs_to_object(rt, v, o)) == RJS_OK)
        r = rjs_object_get(rt, o, pn, v, pv);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Get the value's property by the array item index.
 * \param rt The current runtime.
 * \param v The value.
 * \param idx The index.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_get_index_v (RJS_Runtime *rt, RJS_Value *v, int64_t idx, RJS_Value *pv)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *nv   = rjs_value_stack_push(rt);
    RJS_Value       *idxv = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((idx >= 0) && (idx < 0xffffffff)) {
        rjs_value_set_index_string(rt, idxv, idx);
    } else {
        rjs_value_set_number(rt, nv, idx);
        rjs_to_string(rt, nv, idxv);
    }

    rjs_property_name_init(rt, &pn, idxv);
    r = rjs_get_v(rt, v, &pn, pv);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get the super property.
 * \param rt The current runtime.
 * \param thiz This argument.
 * \param pn The property name.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_super_get_v (RJS_Runtime *rt, RJS_Value *thiz, RJS_PropertyName *pn, RJS_Value *pv);

/**
 * Get the method function of the value.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The property name.
 * \param[out] f Return the method value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_get_method (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *f)
{
    RJS_Result r;

    if ((r = rjs_get_v(rt, v, pn, f)) == RJS_ERR)
        return r;

    if (rjs_value_is_undefined(rt, f))
        return RJS_OK;

    if (rjs_value_is_null(rt, f)) {
        rjs_value_set_undefined(rt, f);
        return RJS_OK;
    }

    if (!rjs_is_callable(rt, f))
        return rjs_throw_type_error(rt, _("property \"%s\" is not a function"),
                rjs_to_desc_chars(rt, pn->name, NULL, NULL));

    return RJS_OK;
}

/**
 * Set the property value of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param v The property value.
 * \param th Throw TypeError when failed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_set (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v, RJS_Bool th)
{
    RJS_Result r;

    if ((r = rjs_object_set(rt, o, pn, v, o)) == RJS_FALSE) {
        return th 
                ? rjs_throw_type_error(rt, _("property \"%s\" cannot be modified"),
                        rjs_to_desc_chars(rt, pn->name, NULL, NULL))
                : RJS_OK;
    }

    return r;
}

/**
 * Set the property value of an object by the array item index.
 * \param rt The current runtime.
 * \param o The object.
 * \param idx The array index.
 * \param v The property value.
 * \param th Throw TypeError when failed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_set_index (RJS_Runtime *rt, RJS_Value *o, int64_t idx, RJS_Value *v, RJS_Bool th)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *nv   = rjs_value_stack_push(rt);
    RJS_Value       *idxv = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((idx >= 0) && (idx < 0xffffffff)) {
        rjs_value_set_index_string(rt, idxv, idx);
    } else {
        rjs_value_set_number(rt, nv, idx);
        rjs_to_string(rt, nv, idxv);
    }

    rjs_property_name_init(rt, &pn, idxv);
    r = rjs_set(rt, o, &pn, v, th);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the property value of a value.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The property name.
 * \param pv The property value.
 * \param th Throw TypeError when failed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_set_v (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *pv, RJS_Bool th)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *o   = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_object(rt, v, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_object_set(rt, o, pn, pv, v)) == RJS_FALSE) {
        if (th) {
            r = rjs_throw_type_error(rt, _("property \"%s\" cannot be modified"),
                    rjs_to_desc_chars(rt, pn->name, NULL, NULL));
        } else {
            r = RJS_OK;
        }
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the super property.
 * \param rt The current runtime.
 * \param thiz This argument.
 * \param pn The property name.
 * \param pv The property value.
 * \param th Throw TypeError when failed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_super_set_v (RJS_Runtime *rt, RJS_Value *thiz, RJS_PropertyName *pn, RJS_Value *pv, RJS_Bool th);

/**
 * Set the number property value of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param n The number value.
 * \param th Throw TypeError when failed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_set_number (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Number n, RJS_Bool th)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;

    rjs_value_set_number(rt, tmp, n);

    r = rjs_set(rt, o, pn, tmp, th);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Call the function.
 * \param rt The current runtime.
 * \param f The function value.
 * \param v This argument.
 * \param args Arguments.
 * \param argc Arguments' count.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_call (RJS_Runtime *rt, RJS_Value *f, RJS_Value *v, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    if (!rjs_is_callable(rt, f))
        return rjs_throw_type_error(rt, _("the value is not a function"));

    return rjs_object_call(rt, f, v, args, argc, rv);
}

/**
 * Construct a new object.
 * \param rt The current runtime.
 * \param f The constructor function.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param nt The new target.
 * \param[out] rv Return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_construct (RJS_Runtime *rt, RJS_Value *f, RJS_Value *args, size_t argc, RJS_Value *nt, RJS_Value *rv)
{
    if (!nt)
        nt = f;

    return rjs_object_construct(rt, f, args, argc, nt, rv);
}

/**
 * Check if the object has the own property.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \retval RJS_TRUE The object has the own property.
 * \retval RJS_FALSE The object has not the own property.
 */
static inline RJS_Result
rjs_has_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_PropertyDesc pd;
    RJS_Result       r;
    size_t           top = rjs_value_stack_save(rt);

    rjs_property_desc_init(rt, &pd);

    r = rjs_object_get_own_property(rt, o, pn, &pd);

    rjs_property_desc_deinit(rt, &pd);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Check if the object has the property.
 * \param rt The current runtime.
 * \param o The object value.
 * \param p The property value.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE The object has the property.
 * \retval RJS_FALSE The object has not the property. 
 */
extern RJS_Result
rjs_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p);

/**
 * Delete a property.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The name of the property to be deleted.
 * \param strict Is in strict mode or not.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE The property is deleted.
 * \retval RJS_FALSE The property cannot be deleted.
 */
extern RJS_Result
rjs_delete_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Bool strict);

/**
 * Delete a property and throw error when failed.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The name of the property to be deleted.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_delete_property_or_throw (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_Result r;

    if ((r = rjs_object_delete(rt, o, pn)) == RJS_FALSE)
        r = rjs_throw_type_error(rt, _("cannot delete the property \"%s\""),
                rjs_string_to_enc_chars(rt, pn->name, NULL, NULL));

    return r;
}

/**
 * Delete a property and throw error when failed with index key.
 * \param rt The current runtime.
 * \param o The object.
 * \param idx The index key.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_delete_property_or_throw_index (RJS_Runtime *rt, RJS_Value *o, int64_t idx)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *nv   = rjs_value_stack_push(rt);
    RJS_Value       *idxv = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((idx >= 0) && (idx < 0xffffffff)) {
        rjs_value_set_index_string(rt, idxv, idx);
    } else {
        rjs_value_set_number(rt, nv, idx);
        rjs_to_string(rt, nv, idxv);
    }

    rjs_property_name_init(rt, &pn, idxv);
    r = rjs_delete_property_or_throw(rt, o, &pn);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Create a new data property.
 * \param rt The current runtime.
 * \param o The object value.
 * \param pn The property name.
 * \param v The data property value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The property cannot be created.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_data_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v);

/**
 * Create a new data property with attributes.
 * \param rt The current runtime.
 * \param o The object value.
 * \param pn The property name.
 * \param v The data property value.
 * \param attrs The attributes.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The property cannot be created.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_data_property_attrs (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v, int attrs);

/**
 * Create a new data property and throw TypeError on failed.
 * \param rt The current runtime.
 * \param o The object value.
 * \param pn The property name.
 * \param v The data property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_create_data_property_or_throw (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v)
{
    RJS_Result r;

    r = rjs_create_data_property(rt, o, pn, v);
    if (r == RJS_FALSE)
        return rjs_throw_type_error(rt, _("cannot create the property \"%s\""),
                rjs_to_desc_chars(rt, pn->name, NULL, NULL));

    return r;
}

/**
 * Create a new data property and throw TypeError on failed with array item index as its key.
 * \param rt The current runtime.
 * \param o The object value.
 * \param idx The array item index.
 * \param v The data property value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The property cannot be created.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_create_data_property_or_throw_index (RJS_Runtime *rt, RJS_Value *o, int64_t idx, RJS_Value *v)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *nv   = rjs_value_stack_push(rt);
    RJS_Value       *idxv = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((idx >= 0) && (idx < 0xffffffff)) {
        rjs_value_set_index_string(rt, idxv, idx);
    } else {
        rjs_value_set_number(rt, nv, idx);
        rjs_to_string(rt, nv, idxv);
    }

    rjs_property_name_init(rt, &pn, idxv);
    r = rjs_create_data_property_or_throw(rt, o, &pn, v);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Create a new data property with attributes and throw TypeError on failed.
 * \param rt The current runtime.
 * \param o The object value.
 * \param pn The property name.
 * \param v The data property value.
 * \param attrs The property's attributes.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_create_data_property_attrs_or_throw (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v, int attrs)
{
    RJS_Result r;

    r = rjs_create_data_property_attrs(rt, o, pn, v, attrs);
    if (r == RJS_FALSE)
        return rjs_throw_type_error(rt, _("cannot create the property \"%s\""),
                rjs_to_desc_chars(rt, pn->name, NULL, NULL));

    return r;
}

/**
 * Create a method property.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param v The function value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_method_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *v);

/**
 * Create an array from the elements.
 * \param rt The current runtime.
 * \param[out] a Return the array value.
 * \param ... items end with NULL.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_array_from_elements (RJS_Runtime *rt, RJS_Value *a, ...);

/**
 * Create an array from the value list.
 * \param rt The current runtime.
 * \param vl The value list.
 * \param[out] a Return the array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_array_from_list (RJS_Runtime *rt, RJS_ValueList *vl, RJS_Value *a);

/**
 * Create an array from the value buffer.
 * \param rt The current runtime.
 * \param items The items value buffer.
 * \param n Number of the items.
 * \param[out] a Return the array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_array_from_value_buffer (RJS_Runtime *rt, RJS_Value *items, size_t n, RJS_Value *a);

/**
 * Create an array from the iterable object.
 * \param rt The current runtime.
 * \param iterable The iterable object.
 * \param[out] a Return the array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_array_from_iterable (RJS_Runtime *rt, RJS_Value *iterable, RJS_Value *a);

/**
 * Invoke the method.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The method's property name.
 * \param args The arguments.
 * \param argc The arguments' count.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_invoke (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    size_t     top  = rjs_value_stack_save(rt);
    RJS_Value *func = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_get_v(rt, v, pn, func)) == RJS_ERR)
        goto end;

    r = rjs_call(rt, func, v, args, argc, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Define a property and throw TypeError when failed.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param pd The property descriptor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_define_property_or_throw (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_Result r;

    r = rjs_object_define_own_property(rt, o, pn, pd);
    if (r == RJS_FALSE)
        return rjs_throw_type_error(rt, _("cannot define the property \"%s\""),
                rjs_to_desc_chars(rt, pn->name, NULL, NULL));

    return r;
}

/**
 * Define a property and throw TypeError when failed with array item index as its key.
 * \param rt The current runtime.
 * \param o The object.
 * \param idx The array item index.
 * \param pd The property descriptor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_define_property_or_throw_index (RJS_Runtime *rt, RJS_Value *o, int64_t idx, RJS_PropertyDesc *pd)
{
    size_t           top  = rjs_value_stack_save(rt);
    RJS_Value       *nv   = rjs_value_stack_push(rt);
    RJS_Value       *idxv = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    if ((idx >= 0) && (idx < 0xffffffff)) {
        rjs_value_set_index_string(rt, idxv, idx);
    } else {
        rjs_value_set_number(rt, nv, idx);
        rjs_to_string(rt, nv, idxv);
    }

    rjs_property_name_init(rt, &pn, idxv);
    r = rjs_define_property_or_throw(rt, o, &pn, pd);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create an ordinary object.
 * \param rt The current runtime.
 * \param proto The prototype.
 * \param[out] o Return the new object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_ordinary_object_create (RJS_Runtime *rt, RJS_Value *proto, RJS_Value *o)
{
    return rjs_object_new(rt, o, proto);
}

/**
 * Get the realm of the function.
 * \param rt The current runtime.
 * \param obj The function object.
 * \return The realm of the function.
 */
extern RJS_Realm*
rjs_get_function_realm (RJS_Runtime *rt, RJS_Value *obj);

/**
 * Make the function as a consturctor.
 * \param rt The current runtime.
 * \param f The function.
 * \param writable The prototype is writable.
 * \param proto The prototype value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_make_constructor (RJS_Runtime *rt, RJS_Value *f, RJS_Bool writable, RJS_Value *proto);

/**
 * Make the function as a method.
 * \param rt The current runtime.
 * \param f The function.
 * \param ho The home object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_make_method (RJS_Runtime *rt, RJS_Value *f, RJS_Value *ho);

/**
 * Reset the object's integrity level.
 * \param rt The current runtime.
 * \param o The object.
 * \param level The new level.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_integrity_level (RJS_Runtime *rt, RJS_Value *o, RJS_IntegrityLevel level);

/**
 * Test the object's integrity level.
 * \param rt The current runtime.
 * \param o The object.
 * \param level The new level.
 * \retval RJS_TRUE The object has set the integrity level.
 * \retval RJS_FALSE The object has not set the integrity level.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_test_integrity_level (RJS_Runtime *rt, RJS_Value *o, RJS_IntegrityLevel level);

/**
 * Set the name property of the function.
 * \param rt The current runtime.
 * \param f The function.
 * \param name The name value.
 * \param prefix Teh prefix.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_function_name (RJS_Runtime *rt, RJS_Value *f, RJS_Value *name, RJS_Value *prefix);

/**
 * Set the length property of the function.
 * \param rt The current runtime.
 * \param f The function.
 * \param len The length value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_function_length (RJS_Runtime *rt, RJS_Value *f, RJS_Number len);

/**
 * Typeof operation.
 * \param rt The current runtime.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_type_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv);

/**
 * Chck if the ordinary object has the instance.
 * \param rt The current runtime.
 * \param c The class object.
 * \param o The instance object.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE o is the instance of c.
 * \retval RJS_FALSE o is not the instance of c.
 */
extern RJS_Result
rjs_ordinary_has_instance (RJS_Runtime *rt, RJS_Value *c, RJS_Value *o);

/**
 * Check if the value is an instance of an object.
 * \param rt The current runtime.
 * \param v The value.
 * \param t The target object.
 * \retval RJS_ERR On error.
 * \retval RJS_TRUE v is the instance of t.
 * \retval RJS_FALSE v is not the instance of t.
 */
extern RJS_Result
rjs_instance_of (RJS_Runtime *rt, RJS_Value *v, RJS_Value *t);

/**
 * Check if 2 values are loosely equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 == v2.
 * \retval RJS_FALSE v1 != v2.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_is_loosely_equal (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2);

/**
 * Check if 2 values are strictly equal.
 * \param rt The current runtime.
 * \param v1 Value 1.
 * \param v2 Value 2.
 * \retval RJS_TRUE v1 == v2.
 * \retval RJS_FALSE v1 != v2. 
 */
extern RJS_Bool
rjs_is_strictly_equal (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2);

/**
 * Get the prototype from the constructor.
 * \param rt The current runtime.
 * \param c The constructor.
 * \param[out] p Return the prototype.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_constructor_prototype (RJS_Runtime *rt, RJS_Value *c, RJS_Value *p);

/**
 * Resolve the binding,
 * \param rt The current runtime.
 * \param bn The binding's name.
 * \param[out] pe Return the environment.
 * \retval RJS_TRUE found the binding.
 * \retval RJS_FALSE cannot find the binding.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_resolve_binding (RJS_Runtime *rt, RJS_BindingName *bn, RJS_Environment **pe);

/**
 * Get the binding's value.
 * \param rt The current runtime.
 * \param env The environment contains the binding.
 * \param bn The binding's name.
 * \param strict In strict mode or not.
 * \param[out] bv Return the binding value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_get_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Bool strict, RJS_Value *bv);

/**
 * Set the binding's value.
 * \param rt The current runtime.
 * \param env The environment contains the binding.
 * \param bn The binding's name.
 * \param bv The new binding value.
 * \param strict In strict mode or not.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_set_binding_value (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Value *bv, RJS_Bool strict);

/**
 * Delete a binding.
 * \param rt The current runtime.
 * \param env The environment contains the binding.
 * \param bn The binding name.
 * \param strict In strict mode or not.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The binding cannot be deleted.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_delete_binding (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *bn, RJS_Bool strict);

/**Add entry to the object functin.*/
typedef RJS_Result (*RJS_AddEntryFunc) (RJS_Runtime *rt, RJS_Value *target, RJS_Value *args, size_t argc, void *data);

/**
 * Add entries from the iterable object to an object.
 * \param rt The current runtime.
 * \param target The target object.
 * \param iterable The iterable object.
 * \param fn The add entry function.
 * \param data The parameter ot the add entry function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_add_entries_from_iterable (RJS_Runtime *rt, RJS_Value *target, RJS_Value *iterable,
        RJS_AddEntryFunc fn, void *data);

/**
 * Get the object's species constructor.
 * \param rt The current runtime.
 * \param o The object.
 * \param def The default constructor.
 * \param[out] c Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_species_constructor (RJS_Runtime *rt, RJS_Value *o, RJS_Value *def, RJS_Value *c);

/**
 * Check if the value can be held weakly.
 * \param rt The current runtime.
 * \param v The value to be checked.
 * \retval RJS_TRUE The value can be held weakly.
 * \retval RJS_FALSE The value cannot be held weakly.
 */
extern RJS_Bool
rjs_can_be_held_weakly (RJS_Runtime *rt, RJS_Value *v);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

