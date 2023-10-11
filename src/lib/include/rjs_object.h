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
 * Object internal header.
 */

#ifndef _RJS_OBJECT_INTERNAL_H_
#define _RJS_OBJECT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**The object is extensible.*/
#define RJS_OBJECT_FL_EXTENSIBLE  1
/**The array is stored in red/back tree.*/
#define RJS_OBJECT_FL_RBT         2

/**Property.*/
typedef struct {
    int           attrs;    /**< The attributes of the property.*/
    union {
        RJS_Value value;    /**< The value of the property.*/
        struct {
            RJS_Value get;  /**< The getter of the accessor.*/
            RJS_Value set;  /**< The setter of the accessor.*/
        } a;                /**< The accessor's data.*/
    } p;                    /**< The property's data.*/
} RJS_Property;

/**Property node.*/
typedef struct {
    RJS_List      ln;   /**< List node data.*/
    RJS_HashEntry he;   /**< Hash table entry.*/
    RJS_Property  prop; /**< Property.*/
} RJS_PropertyNode;

/**Property red/black tree node.*/
typedef struct {
    RJS_Rbt      rbt;   /**< Red/black tree node.*/
    uint32_t     index; /**< The index.*/
    RJS_Property prop;  /**< Property.*/
} RJS_PropertyRbt;

/**Property key.*/
typedef struct {
    RJS_Bool  is_index; /**< Is array index.*/
    void     *key;      /**< The key value.*/
    uint32_t  index;    /**< The index value.*/
} RJS_PropertyKey;

/**Object.*/
struct RJS_Object_s {
    RJS_GcThing  gc_thing;  /**< Base Gc thing data.*/
    int          flags;     /**< Flags.*/
    RJS_Value    prototype; /**< The prototype.*/
    RJS_List     prop_list; /**< The properties' list.*/
    RJS_Hash     prop_hash; /**< The properties' hash table.*/
    union {
        RJS_Rbt      *rbt;       /**< The red/black tree root.*/  
        RJS_Property *vec;       /**< Vector.*/
    } prop_array;                /**< Property array.*/
    uint32_t     array_item_num; /**< Properties' number in the array.*/
    uint32_t     array_item_max; /**< The maximum array item index.*/
    uint32_t     array_item_cap; /**< The capacity of the array vector.*/
};

/**String property entry.*/
typedef struct {
    RJS_HashEntry he;    /**< Hash table entry.*/
    RJS_Value     value; /**< Value.*/
} RJS_StringPropEntry;

/**
 * Convert the value to property key.
 * \param rt The current runtime.
 * \param p The property key value.
 * \param[out] pk Return the property key.
 */
RJS_INTERNAL void
rjs_property_key_get (RJS_Runtime *rt, RJS_Value *p, RJS_PropertyKey *pk);

/**
 * Scan the reference things in the ordinary object.
 * \param rt The current runtime.
 * \param ptr The object pointer.
 */
RJS_INTERNAL void
rjs_object_op_gc_scan (RJS_Runtime *rt, void *ptr);

/**
 * Free the ordinary object.
 * \param rt The current runtime.
 * \param ptr The object pointer.
 */
RJS_INTERNAL void
rjs_object_op_gc_free (RJS_Runtime *rt, void *ptr);

/**
 * Get the prototype of an ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] proto Return the prototype object or null.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_get_prototype_of (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto);

/**
 * Set the prototype of an ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \param proto The prototype object or null.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot reset the prototype.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_set_prototype_of (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto);

/**
 * Check if the ordinary object is extensible.
 * \param rt The current runtime.
 * \param o The object.
 * \retval RJS_TRUE The object is extensible.
 * \retval RJS_FALSE The object is not extensible.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_is_extensible (RJS_Runtime *rt, RJS_Value *o);

/**
 * Prevent the extensions of the ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot prevent extensions.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_prevent_extensions (RJS_Runtime *rt, RJS_Value *o);

/**
 * Get the own property's descriptor of the ordinary object.
 * \param rt The current runtime.
 * \param o the object.
 * \param pn The property name.
 * \param[out] pd Return the property descriptor.
 * \retval RJS_OK On success.
 * \retval RJS_FALSE The property is not defined.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_get_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd);

/**
 * Define an own property of an ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param pd The property's descriptor.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot create or updete the property.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_define_own_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd);

/**
 * Check if the ordinary object has the property.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \retval RJS_TRUE The object has the property.
 * \retval RJS_FALSE The object has not the property.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_has_property (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn);

/**
 * Get the property value of an ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param receiver The receiver object.
 * \param[out] pv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_get (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv);

/**
 * Set the property value of an ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \param pv The new property value.
 * \param receiver The receiver object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_set (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver);

/**
 * Delete a property of an ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \param pn The property name.
 * \retval RJS_TRUE The property is deleted.
 * \retval RJS_FALSE The property is not deleted.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_delete (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn);

/**
 * Get the own properties' keys of an ordinary object.
 * \param rt The current runtime.
 * \param o The object.
 * \param[out] keys Return the object's keys list object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_op_own_property_keys (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys);

/**Ordinary object operation functions.*/
#define RJS_ORDINARY_OBJECT_OPS\
    rjs_ordinary_object_op_get_prototype_of,\
    rjs_ordinary_object_op_set_prototype_of,\
    rjs_ordinary_object_op_is_extensible,\
    rjs_ordinary_object_op_prevent_extensions,\
    rjs_ordinary_object_op_get_own_property,\
    rjs_ordinary_object_op_define_own_property,\
    rjs_ordinary_object_op_has_property,\
    rjs_ordinary_object_op_get,\
    rjs_ordinary_object_op_set,\
    rjs_ordinary_object_op_delete,\
    rjs_ordinary_object_op_own_property_keys

/**
 * Delete the array index key type properties from index.
 * \param rt The current runtime.
 * \param o The object.
 * \param old_len The old length.
 * \param new_len The new length.
 * \param[out] last_idx Return the last index after deleted.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Some properties cannot be deleted.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_object_delete_from_index (RJS_Runtime *rt, RJS_Value *o,
        uint32_t old_len, uint32_t new_len, uint32_t *last_idx);

/**
 * Initialize the object.
 * \param rt The current runtime.
 * \param v The value to store the object.
 * \param o The object to be initialized.
 * \param proto The prototype value.
 * \param ops The object's operation functions.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_object_init (RJS_Runtime *rt, RJS_Value *v, RJS_Object *o, RJS_Value *proto, const RJS_ObjectOps *ops);

/**
 * Release the object.
 * \param rt The current runtime.
 * \param o The object to be released.
 */
RJS_INTERNAL void
rjs_object_deinit (RJS_Runtime *rt, RJS_Object *o);

/**
 * Convert the ordinary object to primitive type.
 * \param rt The current runtime.
 * \param v The object value.
 * \param[out] prim Return the primitive type.
 * \param type Prefered value type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_ordinary_to_primitive (RJS_Runtime *rt, RJS_Value *v, RJS_Value *prim, RJS_ValueType type);

#ifdef __cplusplus
}
#endif

#endif

