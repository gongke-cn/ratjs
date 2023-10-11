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
 * Function object internal header.
 */

#ifndef _RJS_FUNCTION_INTERNAL_H_
#define _RJS_FUNCTION_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Script class element type.*/
typedef enum {
    RJS_SCRIPT_CLASS_ELEMENT_METHOD, /**< Method.*/
    RJS_SCRIPT_CLASS_ELEMENT_GET,    /**< Accessor's getter.*/
    RJS_SCRIPT_CLASS_ELEMENT_SET     /**< Accessor's setter.*/
} RJS_ScriptClassElementType;

/**Script method.*/
typedef struct {
    RJS_ScriptClassElementType type;   /**< Method type.*/
    RJS_Value                  name;   /**< Name.*/
    RJS_Value                  value;  /**< Value.*/
} RJS_ScriptMethod;

/**Script field.*/
typedef struct {
    RJS_Value name;  /**< Name.*/
    RJS_Value init;  /**< Initializer.*/
    RJS_Bool  is_af; /**< The initializer is an anonymous function.*/
} RJS_ScriptField;

/*Class information.*/
typedef struct {
#if ENABLE_PRIV_NAME
    RJS_ScriptMethod *priv_methods;    /**< Private methods.*/
    size_t            priv_method_num; /**< Number of private methods.*/
#endif /*ENABLE_PRIV_NAME*/
    RJS_ScriptField  *fields;          /**< Fields.*/
    size_t            field_num;       /**< Number of fields.*/
} RJS_ScriptClass;

/**Base function object.*/
typedef struct {
    RJS_Object       object; /**< Base object data.*/
    RJS_Script      *script; /**< The script contains this function.*/
    RJS_ScriptClass *clazz;  /**< Class information.*/
} RJS_BaseFuncObject;

/**
 * Initialize the base function object.
 * \param rt The current runtime.
 * \param[out] v Return the function value.
 * \param bfo The base function object.
 * \param proto The prototype.
 * \param ops The object's operation functions.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_base_func_object_init (RJS_Runtime *rt, RJS_Value *v, RJS_BaseFuncObject *bfo,
        RJS_Value *proto, const RJS_ObjectOps *ops, RJS_Script *script)
{
    bfo->clazz  = NULL;
    bfo->script = script;

    return rjs_object_init(rt, v, &bfo->object, proto, ops);
}

/**
 * Release the base function object.
 * \param rt The current runtime.
 * \param bfo The base function object to be released.
 */
RJS_INTERNAL void
rjs_base_func_object_deinit (RJS_Runtime *rt, RJS_BaseFuncObject *bfo);

/**
 * Scan referenced things in the base function object.
 * \param rt The current runtime.
 * \param bfo The base function object.
 */
RJS_INTERNAL void
rjs_base_func_object_op_gc_scan (RJS_Runtime *rt, RJS_BaseFuncObject *bfo);

/**
 * Initialize the instance's elements.
 * \param rt The current runtime.
 * \param o The instance object.
 * \param f The constructor function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_INTERNAL RJS_Result
rjs_initialize_instance_elements (RJS_Runtime *rt, RJS_Value *o, RJS_Value *f);

#ifdef __cplusplus
}
#endif

#endif

