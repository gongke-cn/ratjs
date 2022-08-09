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
 * Operation internal header.
 */

#ifndef _RJS_OPERATION_INTERNAL_H_
#define _RJS_OPERATION_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Class's element type.*/
typedef enum {
    RJS_CLASS_ELEMENT_GET,               /**< Getter.*/
    RJS_CLASS_ELEMENT_SET,               /**< Setter.*/
    RJS_CLASS_ELEMENT_METHOD,            /**< Method.*/
    RJS_CLASS_ELEMENT_FIELD,             /**< Field.*/
    RJS_CLASS_ELEMENT_INST_FIELD,        /**< Instance's field.*/
    RJS_CLASS_ELEMENT_STATIC_GET,        /**< Static getter.*/
    RJS_CLASS_ELEMENT_STATIC_SET,        /**< Static setter.*/
    RJS_CLASS_ELEMENT_STATIC_METHOD,     /**< Static method.*/
    RJS_CLASS_ELEMENT_STATIC_INIT,       /**< Static initializer.*/
#if ENABLE_PRIV_NAME
    RJS_CLASS_ELEMENT_PRIV_GET,          /**< Instance's private getter.*/
    RJS_CLASS_ELEMENT_PRIV_SET,          /**< Instance's private setter.*/
    RJS_CLASS_ELEMENT_PRIV_METHOD,       /**< Instance's private method.*/
    RJS_CLASS_ELEMENT_PRIV_FIELD,        /**< Private field.*/
    RJS_CLASS_ELEMENT_PRIV_INST_FIELD,   /**< Instance's private field.*/
    RJS_CLASS_ELEMENT_STATIC_PRIV_GET,   /**< Instance's private getter.*/
    RJS_CLASS_ELEMENT_STATIC_PRIV_SET,   /**< Instance's private setter.*/
    RJS_CLASS_ELEMENT_STATIC_PRIV_METHOD /**< Instance's private method.*/
#endif /*ENABLE_PRIV_NAME*/
} RJS_ClassElementType;

/**
 * Get the prototype from a constructor.
 * \param rt The current runtime.
 * \param constr The constructor.
 * \param dp_idx The default prototype object's index.
 * \param[out] o Return the prototype.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_get_prototype_from_constructor (RJS_Runtime *rt, RJS_Value *constr, int pd_idx, RJS_Value *proto);

/**
 * Create an ordinary object from a constructor.
 * \param rt The current runtime.
 * \param constr The constructor.
 * \param dp_idx The default prototype object's index.
 * \param[out] o Return the new object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_ordinary_create_from_constructor (RJS_Runtime *rt, RJS_Value *constr, int dp_idx, RJS_Value *o)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *proto = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_get_prototype_from_constructor(rt, constr, dp_idx, proto)) == RJS_ERR)
        return r;

    r = rjs_ordinary_object_create(rt, proto, o);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Initialize an ordinary object from a constructor.
 * \param rt The current runtime.
 * \param o The object.
 * \param constr The constructor.
 * \param dp_idx The default prototype object's index.
 * \param ops The object operation functions.
 * \param[out] ov Return the new object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_ordinary_init_from_constructor (RJS_Runtime *rt, RJS_Object *o, RJS_Value *constr,
        int dp_idx, const RJS_ObjectOps *ops, RJS_Value *ov)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *proto = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_get_prototype_from_constructor(rt, constr, dp_idx, proto)) == RJS_ERR)
        return r;

    r = rjs_object_init(rt, ov, o, proto, ops);

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Create a new constructor.
 * \param rt The current runtime.
 * \param proto The prototype of the constructor.
 * \param parent The parent of the constructor.
 * \param script The script.
 * \param func The script function of the constructor.
 * \param[out] c Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_constructor (RJS_Runtime *rt, RJS_Value *proto, RJS_Value *parent,
        RJS_Script *script, RJS_ScriptFunc *func, RJS_Value *c);

/**
 * Create the default constructor.
 * \param rt The current runtime.
 * \param proto The prototype of the constructor.
 * \param parent The parent of the constructor.
 * \param name The name of the class.
 * \param derived The class is derived.
 * \param[out] c Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_default_constructor (RJS_Runtime *rt, RJS_Value *proto, RJS_Value *parent,
        RJS_Value *name, RJS_Bool derived, RJS_Value *c);

/**
 * Create a new function.
 * \param rt The current runtime.
 * \param script The script.
 * \param sf The script function.
 * \param env The lexical environment.
 * \param priv_env The private environment.
 * \param is_constr The function is a constructor or not.
 * \param[out] f Return the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_function (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptFunc *sf,
        RJS_Environment *env, RJS_PrivateEnv *priv_env, RJS_Bool is_constr, RJS_Value *f);

/**
 * Define a field to the object.
 * \param rt The current runtime.
 * \param o The object.
 * \param name The name of the field.
 * \param init The field initializer.
 * \param is_af The initializer is an anonymous function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_define_field (RJS_Runtime *rt, RJS_Value *o, RJS_Value *name, RJS_Value *init, RJS_Bool is_af);

/**
 * Define a method.
 * \param rt The current runtime.
 * \param o The object contains this method.
 * \param proto The prototype value.
 * \param script The script.
 * \param func The script function.
 * \param[out] f Return the method object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_define_method (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto, RJS_Script *script,
        RJS_ScriptFunc *func, RJS_Value *f);

/**
 * Define a method property.
 * \param rt The current runtime.
 * \param o The object value.
 * \param n The name of the property.
 * \param f The method function value.
 * \param enumerable The property is enumerable.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_define_method_property (RJS_Runtime *rt, RJS_Value *o, RJS_Value *n, RJS_Value *f, RJS_Bool enumerable);

/**
 * Create a method or accessor.
 * \param rt The current runtime.
 * \param o The object contains this method.
 * \param type The element type of the method.
 * \param n The name of the property.
 * \param script The script.
 * \param sf The script function.
 * \param enumerable The property is enumerable.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_create_method (RJS_Runtime *rt, RJS_Value *o, RJS_ClassElementType type, RJS_Value *n,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Bool enumerable);

#if ENABLE_ASYNC
/**
 * Await operation.
 * \param rt The current runtime.
 * \param v The value.
 * \param type Await type.
 * \param op The operation function.
 * \param ip The integer parameter of the function.
 * \param vp The value parameter of the function.
 * \retval RJS_SUSPEND On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_await (RJS_Runtime *rt, RJS_Value *v, RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp);
#endif /*ENABLE_ASYNC*/

/**
 * The function return this argument.
 */
extern RJS_NF(rjs_return_this);

#ifdef __cplusplus
}
#endif

#endif

