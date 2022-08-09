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
 * Built-in function object internal header.
 */

#ifndef _RJS_BUILTIN_FUNC_OBJECT_INTERNAL_H_
#define _RJS_BUILTIN_FUNC_OBJECT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Built-in function object.*/
typedef struct {
    RJS_BaseFuncObject bfo;       /**< Base function object.*/
    RJS_Realm         *realm;     /**< The realm.*/
    int                flags;     /**< The function's flags.*/
#if ENABLE_FUNC_SOURCE
    RJS_Value          init_name; /**< The function's source.*/
#endif /*ENABLE_FUNC_SOURCE*/
    RJS_NativeFunc     func;      /**< Native function pointer.*/
} RJS_BuiltinFuncObject;

/**Built-in object's field description.*/
typedef struct {
    char          *name; /**< Name of the field.*/
    RJS_ValueType  type; /**< Field's value type.*/
    RJS_Number     n;    /**< The number value.*/
    void          *ptr;  /**< The pointer value.*/
    int            attrs;/**< The property's attributes.*/
} RJS_BuiltinFieldDesc;

/**Built-in function description.*/
typedef struct {
    char           *name;   /**< The name of the function.*/
    size_t          length; /**< The parameters length.*/
    RJS_NativeFunc  func;   /**< The native function.*/
    char           *native; /**< The native reference name.*/
} RJS_BuiltinFuncDesc;

/**Built-in accessor description.*/
typedef struct {
    char           *name;       /**< The name of the accessor.*/
    RJS_NativeFunc  get;        /**< Getter function.*/
    RJS_NativeFunc  set;        /**< Setter function.*/
    char           *native_get; /**< The getter's native reference name.*/
    char           *native_set; /**< The setter's native reference name.*/
} RJS_BuiltinAccessorDesc;

/**Built-in object description.*/
typedef struct RJS_BuiltinObjectDesc_s RJS_BuiltinObjectDesc;

/**Built-in object description.*/
struct RJS_BuiltinObjectDesc_s {
    char                          *name;       /**< The name of the object.*/
    char                          *parent;     /**< The parent object.*/
    const RJS_BuiltinFuncDesc     *constructor;/**< The constructor function.*/
    const RJS_BuiltinObjectDesc   *prototype;  /**< The prototype object.*/
    const RJS_BuiltinFieldDesc    *fields;     /**< Field methods description.*/
    const RJS_BuiltinFuncDesc     *functions;  /**< Function methods description.*/
    const RJS_BuiltinAccessorDesc *accessors;  /**< Accessor methods description.*/
    const RJS_BuiltinObjectDesc   *objects;    /**< Object methods description.*/
    char                          *native;     /**< The native reference name.*/
};

/**Built-in description.*/
typedef struct {
    const RJS_BuiltinFieldDesc    *fields;     /**< Field methods description.*/
    const RJS_BuiltinFuncDesc     *functions;  /**< Function methods description.*/
    const RJS_BuiltinObjectDesc   *objects;    /**< Object methods description.*/
} RJS_BuiltinDesc;

/**
 * Scan the referenced things in the built-in function object.
 * \param rt The current runtime.
 * \param ptr The built-in function object pointer.
 */
extern void
rjs_builtin_func_object_op_gc_scan (RJS_Runtime *rt, void *ptr);

/**
 * Free the built-in function object.
 * \param rt The current runtime.
 * \param ptr The built-in function object pointer.
 */
extern void
rjs_builtin_func_object_op_gc_free (RJS_Runtime *rt, void *ptr);

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
extern RJS_Result
rjs_builtin_func_object_op_call (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz,
        RJS_Value *args, size_t argc, RJS_Value *rv);

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
extern RJS_Result
rjs_builtin_func_object_op_construct (RJS_Runtime *rt, RJS_Value *o,
        RJS_Value *args, size_t argc, RJS_Value *target, RJS_Value *rv);

/**Built-in function object's operation functions.*/
#define RJS_BUILTIN_FUNCTION_OBJECT_OPS\
    RJS_ORDINARY_OBJECT_OPS,\
    rjs_builtin_func_object_op_call,\
    NULL

/**Built-in constructor object's operation functions.*/
#define RJS_BUILTIN_CONSTRUCTOR_OBJECT_OPS\
    RJS_ORDINARY_OBJECT_OPS,\
    rjs_builtin_func_object_op_call,\
    rjs_builtin_func_object_op_construct

/**
 * Create a new built-in function.
 * \param rt The current runtime.
 * \param[out] v Return the new built-in object.
 * \param realm The realm.
 * \param proto The prototype value.
 * \param script The script contains this function.
 * \param nf The native function's pointer.
 * \param flags The function's flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_builtin_func_object_new (RJS_Runtime *rt, RJS_Value *v, RJS_Realm *realm, RJS_Value *proto,
        RJS_Script *script, RJS_NativeFunc nf, int flags);

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
extern RJS_Result
rjs_builtin_func_object_init (RJS_Runtime *rt, RJS_Value *v, RJS_BuiltinFuncObject *bfo, RJS_Realm *realm,
        RJS_Value *proto, RJS_Script *script, RJS_NativeFunc nf, int flags, const RJS_ObjectOps *ops);

/**
 * Release the built-in function object.
 * \param rt The current runtime.
 * \param bfo The built-in function object to be released.
 */
extern void
rjs_builtin_func_object_deinit (RJS_Runtime *rt, RJS_BuiltinFuncObject *bfo);

/**
 * Make the built-in function object as constructor.
 * \param rt The current runtime.
 * \param f The script function object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_builtin_func_object_make_constructor (RJS_Runtime *rt, RJS_Value *f);

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
extern RJS_Result
rjs_init_builtin_function (RJS_Runtime *rt, RJS_BuiltinFuncObject *bfo, RJS_NativeFunc nf,
        int flags, const RJS_ObjectOps *ops, size_t len, RJS_Value *name, RJS_Realm *realm,
        RJS_Value *proto, RJS_Script *script, RJS_Value *prefix, RJS_Value *f);

/**
 * Load the built-in object description.
 * \param rt The current runtime.
 * \param realm The realm.
 * \param desc The built-in object description.
 * \param[out] o Return the new object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_load_builtin_object_desc (RJS_Runtime *rt, RJS_Realm *realm, const RJS_BuiltinObjectDesc *desc, RJS_Value *o);

/**
 * Load the built-in function description.
 * \param rt The current runtime.
 * \param realm The realm.
 * \param desc The built-in function description.
 * \param[out] f Return the new function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_load_builtin_func_desc (RJS_Runtime *rt, RJS_Realm *realm, const RJS_BuiltinFuncDesc *desc, RJS_Value *f);

/**
 * Load the built-in desciption to the global object.
 * \param rt The current runtime.
 * \param realm The realm.
 * \param desc The build-in desciption.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_load_builtin_desc (RJS_Runtime *rt, RJS_Realm *realm, const RJS_BuiltinDesc *desc);

#if ENABLE_MODULE

/**
 * Load the built-in desciption to the module.
 * \param rt The current runtime.
 * \param mod The module.
 * \param desc The build-in desciption.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_load_builtin_module_desc (RJS_Runtime *rt, RJS_Value *mod, const RJS_BuiltinDesc *desc);

#endif /*ENABLE_MODULE*/

#ifdef __cplusplus
}
#endif

#endif

