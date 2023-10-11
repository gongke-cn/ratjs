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
 * Private name.
 */

#ifndef _RJS_PRIVATE_NAME_H_
#define _RJS_PRIVATE_NAME_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Private name.*/
typedef struct {
    RJS_GcThing  gc_thing;     /**< Base GC thing data.*/
    RJS_Value    description;  /**< Description string.*/
} RJS_PrivateName;

/**Private name entry.*/
typedef struct {
    RJS_HashEntry he;        /**< Hash table entry.*/
    RJS_Value     priv_name; /**< Private name.*/
} RJS_PrivateNameEntry;

/**Private environment.*/
struct RJS_PrivateEnv_s {
    RJS_GcThing     gc_thing;       /**< Base GC thing data.*/
    RJS_PrivateEnv *outer;          /**< The outer private environment.*/
    RJS_Hash        priv_name_hash; /**< Private name hash table.*/
};

/**
 * Push a new private environment.
 * \param rt The current runtime.
 * \param script The script.
 * \param spe The script private environment record.
 * \return The new private environment.
 */
RJS_INTERNAL RJS_PrivateEnv*
rjs_private_env_push (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptPrivEnv *spe);

/**
 * Popup the top private environment.
 * \param rt The current runtime.
 * \param env The private environment to be popped.
 */
RJS_INTERNAL void
rjs_private_env_pop (RJS_Runtime *rt, RJS_PrivateEnv *env);

/**
 * Lookup the private name.
 * \param rt The current runtime.
 * \param id The identifier of the private name.
 * \param env The private environment.
 * \param[out] pn Return the private name.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot find the private name.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_private_name_lookup (RJS_Runtime *rt, RJS_Value *id, RJS_PrivateEnv *env, RJS_Value *pn);

/**
 * Get the object's private property's value.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The private property name.
 * \param[out] pv Return the private property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_private_get (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *pv);

/**
 * Set the object's private property's value.
 * \param rt The current runtime.
 * \param v The value.
 * \param pn The private property name.
 * \param pv The private property's new value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_private_set (RJS_Runtime *rt, RJS_Value *v, RJS_PropertyName *pn, RJS_Value *pv);

/**
 * Add a private field.
 * \param rt The current runtime.
 * \param o The object.
 * \param p The private name.
 * \param pv The field's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_private_field_add (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p, RJS_Value *pv);

/**
 * Add a private method.
 * \param rt The current runtime.
 * \param o The object.
 * \param p The private name.
 * \param pv The method's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_private_method_add (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p, RJS_Value *pv);

/**
 * Add a private accessor.
 * \param rt The current runtime.
 * \param o The object,
 * \param p The private name.
 * \param get The getter of the accessor.
 * \param set The setter of the accessor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_private_accessor_add (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p, RJS_Value *get, RJS_Value *set);

/**
 * Find a private element in an object.
 * \param rt The current runtime.
 * \param o The object.
 * \param p The private identifier.
 * \retval RJS_TRUE Find the private element.
 * \retval RJS_FALSE Cannot find the private element.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_private_element_find (RJS_Runtime *rt, RJS_Value *o, RJS_Value *p);

#ifdef __cplusplus
}
#endif

#endif

