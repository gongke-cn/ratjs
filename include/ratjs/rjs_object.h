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
 * Object.
 */

#ifndef _RJS_OBJECT_H_
#define _RJS_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup object Object
 * Generic object
 * @{
 */

/**The property is wriable.*/
#define RJS_PROP_ATTR_WRITABLE     1
/**The property is enumerable.*/
#define RJS_PROP_ATTR_ENUMERABLE   2
/**The property is configurable.*/
#define RJS_PROP_ATTR_CONFIGURABLE 4
/**The property is an accessor.*/
#define RJS_PROP_ATTR_ACCESSOR     8
/**The property is deleted.*/
#define RJS_PROP_ATTR_DELETED      16
/**The property is a method.*/
#define RJS_PROP_ATTR_METHOD       32

/**The property descriptor is wriable.*/
#define RJS_PROP_FL_WRITABLE     RJS_PROP_ATTR_WRITABLE
/**The property descriptor is enumerable.*/
#define RJS_PROP_FL_ENUMERABLE   RJS_PROP_ATTR_ENUMERABLE
/**The property descriptor is configurable.*/
#define RJS_PROP_FL_CONFIGURABLE RJS_PROP_ATTR_CONFIGURABLE
/**The property descriptor has writable attribute.*/
#define RJS_PROP_FL_HAS_WRITABLE     32
/**The property descriptor has enumerable attribute.*/
#define RJS_PROP_FL_HAS_ENUMERABLE   64
/**The property descriptor has configurable attribute.*/
#define RJS_PROP_FL_HAS_CONFIGURABLE 128
/**The property descriptor has value attribute.*/
#define RJS_PROP_FL_HAS_VALUE        256
/**The property descriptor has get attribute.*/
#define RJS_PROP_FL_HAS_GET          512
/**The proerty descriptor has set attribute.*/
#define RJS_PROP_FL_HAS_SET          1024

/**Data property's flags.*/
#define RJS_PROP_FL_DATA\
    (RJS_PROP_FL_HAS_VALUE\
            |RJS_PROP_FL_HAS_WRITABLE\
            |RJS_PROP_FL_HAS_ENUMERABLE\
            |RJS_PROP_FL_HAS_CONFIGURABLE)

/**Accessor property's flags.*/
#define RJS_PROP_FL_ACCESSOR\
    (RJS_PROP_FL_HAS_GET\
            |RJS_PROP_FL_HAS_SET\
            |RJS_PROP_FL_HAS_ENUMERABLE\
            |RJS_PROP_FL_HAS_CONFIGURABLE)

/**
 * Initialize the property descriptor.
 * \param rt The current runtime.
 * \param pd The property descriptor to be initialized.
 */
static inline void
rjs_property_desc_init (RJS_Runtime *rt, RJS_PropertyDesc *pd)
{
    RJS_Value *v;

    pd->flags = 0;

    v = rjs_value_stack_push_n(rt, 3);

    pd->value = v;
    pd->get   = rjs_value_buffer_item(rt, v, 1);
    pd->set   = rjs_value_buffer_item(rt, v, 2);
}

/**
 * Release the property descriptor.
 * \param rt The current runtime.
 * \param pd The property descriptor to be released.
 */
static inline void
rjs_property_desc_deinit (RJS_Runtime *rt, RJS_PropertyDesc *pd)
{
}

/**
 * Initialize the property name.
 * \param rt The current runtime.
 * \param pn The property name to be initialized.
 * \param v The name value.
 */
static inline void
rjs_property_name_init (RJS_Runtime *rt, RJS_PropertyName *pn, RJS_Value *v)
{
    pn->name = v;
}

/**
 * Release the property name.
 * \param rt The current runtime.
 * \param pn The property name to be released.
 */
static inline void
rjs_property_name_deinit (RJS_Runtime *rt, RJS_PropertyName *pn)
{
}

/**
 * Copy the property descriptor.
 * \param rt The current runtime.
 * \param d The destination descriptor.
 * \param s The source descriptor.
 */
static inline void
rjs_property_desc_copy (RJS_Runtime *rt, RJS_PropertyDesc *d, RJS_PropertyDesc *s)
{
    d->flags = s->flags;

    rjs_value_copy(rt, d->value, s->value);
    rjs_value_copy(rt, d->get, s->get);
    rjs_value_copy(rt, d->set, s->set);
}

/**
 * Allocate a new proprty key list.
 * \param rt The current runtime.
 * \param[out] v Return the key list value.
 * \param cap The buffer capacity of the key list.
 * \return The key list.
 */
extern RJS_PropertyKeyList*
rjs_property_key_list_new (RJS_Runtime *rt, RJS_Value *v, size_t cap);

/**
 * Add the object's own keys to the property key list.
 * \param rt The current runtime.
 * \param kv The property key list value.
 * \param ov The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_property_key_list_add_own_keys (RJS_Runtime *rt, RJS_Value *kv, RJS_Value *ov);

/**
 * Get the prototype of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] proto Return the prototype object or null.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_get_prototype_of (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->get_prototype_of(rt, o, proto);
}

/**
 * Set the prototype of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param proto The prototype object or null.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot reset the prototype.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_set_prototype_of (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->set_prototype_of(rt, o, proto);
}

/**
 * Check if the object is extensible.
 * \param rt The current runtime.
 * \param o The object.
 * \retval RJS_TRUE The object is extensible.
 * \retval RJS_FALSE The object is not extensible.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_is_extensible (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->is_extensible(rt, o);
}

/**
 * Prevent the extensions of the object.
 * \param rt The current runtime.
 * \param o The object.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot prevent extensions.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_prevent_extensions (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->prevent_extensions(rt, o);
}

/**
 * Get the own property's descriptor.
 * \param rt The current runtime.
 * \param o the object.
 * \param pn The property name.
 * \param[out] pd Return the property descriptor.
 * \retval RJS_OK On success.
 * \retval RJS_FALSE The property is not defined.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->get_own_property(rt, o, pn, pd);
}

/**
 * Define an own property.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param pd The property's descriptor.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot create or updete the property.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->define_own_property(rt, o, pn, pd);
}

/**
 * Check if the object has the property.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \retval RJS_TRUE The object has the property.
 * \retval RJS_FALSE The object has not the property.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->has_property(rt, o, pn);
}

/**
 * Get the property value of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param receiver The receiver object.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_get (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->get(rt, o, pn, receiver, pv);
}

/**
 * Set the property value of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param pv The new property value.
 * \param receiver The receiver object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_set (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->set(rt, o, pn, pv, receiver);
}

/**
 * Delete a property of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \retval RJS_TRUE The property is deleted.
 * \retval RJS_FALSE The property is not deleted.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_delete (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->delete(rt, o, pn);
}

/**
 * Get the own properties' keys of an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] keys Return the object's keys list object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    return ops->own_property_keys(rt, o, keys);
}

/**
 * Call an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param thiz This argument.
 * \param args Arguments.
 * \param argc Arguments' count.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;
    RJS_Result     r;
    size_t         top = rjs_value_stack_save(rt);

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;

    if (!rv)
        rv = rjs_value_stack_push(rt);

    r = ops->call(rt, o, thiz, args, argc, rv);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Construct a new object.
 * \param rt The current runtime.
 * \param o The object.
 * \param args Arguments.
 * \param argc Arguments' count.
 * \param target The new target object.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_object_construct (RJS_Runtime *rt, RJS_Value *o, RJS_Value *args, size_t argc, RJS_Value *target, RJS_Value *rv)
{
    RJS_GcThing   *gt;
    RJS_ObjectOps *ops;

    assert(rjs_value_is_object(rt, o));

    gt  = rjs_value_get_gc_thing(rt, o);
    ops = (RJS_ObjectOps*) gt->ops;
    
    return ops->construct(rt, o, args, argc, target, rv);
}

/**
 * Create a new object.
 * \param rt The current runtime.
 * \param[out] v Return the object value.
 * \param proto The prototype object.
 * If proto == NULL, use Object.prototype.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *proto);

/**
 * Convert the object to primitive type.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] prim Return the primitive type.
 * \param type Prefered value type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_to_primitive (RJS_Runtime *rt, RJS_Value *v, RJS_Value *prim, RJS_ValueType type);

/**
 * Convert the object to number.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] pn Return the number.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_to_number (RJS_Runtime *rt, RJS_Value *v, RJS_Number *pn);

/**
 * Assign the properies of destination object to the source.
 * \param rt The current runtime.
 * \param dst The destination object.
 * \param src The source object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_assign (RJS_Runtime *rt, RJS_Value *dst, RJS_Value *src);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

