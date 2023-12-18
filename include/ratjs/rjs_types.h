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
 * Basic data types.
 */

#ifndef _RJS_TYPES_H_
#define _RJS_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup misc
 * @{
 */

/**Boolean value.*/
typedef uint8_t RJS_Bool;

/**Boolean value true.*/
#define RJS_TRUE  1
/**Boolean value false.*/
#define RJS_FALSE 0

/**Function result.*/
typedef int RJS_Result;

/**Function result: OK.*/
#define RJS_OK        1
/**Function result: Throw an error.*/
#define RJS_ERR      -1

/**Function result: Throw an error.*/
#define RJS_THROW    -1
/**Function result: Function suspended.*/
#define RJS_SUSPEND   0
/**Function result: Run the next command.*/
#define RJS_NEXT      1
/**Function result: Return from the generator.*/
#define RJS_RETURN    2
/**Function result: ambiguous.*/
#define RJS_AMBIGUOUS 3

/**Compare result.*/
typedef enum {
    RJS_COMPARE_EQUAL,    /**< a == b.*/
    RJS_COMPARE_LESS,     /**< a < b.*/
    RJS_COMPARE_GREATER,  /**< a > b.*/
    RJS_COMPARE_UNDEFINED /**< Undefined.*/
} RJS_CompareResult;

/**Unicode character.*/
typedef uint16_t RJS_UChar;

/**Maximum integer value.*/
#define RJS_MAX_INT 0x1fffffffffffffull

/**Number.*/
typedef double RJS_Number;

/**Strict mode function.*/
#define RJS_FUNC_FL_STRICT       1
/**Derived constructor.*/
#define RJS_FUNC_FL_DERIVED      2
/**Is a class constructor.*/
#define RJS_FUNC_FL_CLASS_CONSTR 4
#if ENABLE_ARROW_FUNC
/**Is an arrow function.*/
#define RJS_FUNC_FL_ARROW        8
#endif /*ENABLE_ARROW_FUNC*/
#if ENABLE_GENERATOR
/**Generator function.*/
#define RJS_FUNC_FL_GENERATOR    16
#endif /*ENABLE_GENERATOR*/
#if ENABLE_ASYNC
/**Async function.*/
#define RJS_FUNC_FL_ASYNC        32
#endif /*ENABLE_ASYNC*/
/**The function is a class field initializer.*/
#define RJS_FUNC_FL_CLASS_FIELD_INIT 64
/**The function is getter of an accessor.*/
#define RJS_FUNC_FL_GET          128
/**The function is setter of an accessor.*/
#define RJS_FUNC_FL_SET          256

/**Disassemble the functions.*/
#define RJS_DISASSEMBLE_FUNC        1
/**Disassemble the code.*/
#define RJS_DISASSEMBLE_CODE        2
/**Disassemble the value table.*/
#define RJS_DISASSEMBLE_VALUE       4
/**Disassemble the declaration table.*/
#define RJS_DISASSEMBLE_DECL        8
/**Disassemble the binding group table.*/
#define RJS_DISASSEMBLE_BINDING     16
/**Disassemble the function declaration group table.*/
#define RJS_DISASSEMBLE_FUNC_DECL   32
/**Disassemble the property reference table.*/
#define RJS_DISASSEMBLE_PROP_REF    64
/**Disassemble the import entries.*/
#define RJS_DISASSEMBLE_IMPORT      128
/**Disassemble the export entries.*/
#define RJS_DISASSEMBLE_EXPORT      256
/**Disassemble the private environment.*/
#define RJS_DISASSEMBLE_PRIV_ENV    512
/**Output all disassemble data.*/
#define RJS_DISASSEMBLE_ALL\
    (RJS_DISASSEMBLE_FUNC\
            |RJS_DISASSEMBLE_CODE\
            |RJS_DISASSEMBLE_VALUE\
            |RJS_DISASSEMBLE_DECL\
            |RJS_DISASSEMBLE_BINDING\
            |RJS_DISASSEMBLE_FUNC_DECL\
            |RJS_DISASSEMBLE_PROP_REF\
            |RJS_DISASSEMBLE_IMPORT\
            |RJS_DISASSEMBLE_EXPORT\
            |RJS_DISASSEMBLE_PRIV_ENV)

/**
 * @}
 */

/**
 * \ingroup char_buf
 * Character buffer.
 */
typedef RJS_VECTOR_DECL(char) RJS_CharBuffer;

/**
 * \ingroup uchar_buf
 * Unicode character buffer.
 */
typedef RJS_VECTOR_DECL(RJS_UChar) RJS_UCharBuffer;

/**
 * \addtogroup list
 * @{
 */

/**
 * \ingroup list
 * \brief Double linked list.
 */
typedef struct RJS_List_s RJS_List;

/**Double linked list.*/
struct RJS_List_s {
    RJS_List *prev; /**< The previous node in the list.*/
    RJS_List *next; /**< The next node in the list.*/
};

/**
 * @}
 */

#if ENABLE_BIG_INT
/**
 * \ingroup big_int
 * \brief Big integer number.
 */
typedef struct RJS_BigInt_s RJS_BigInt;
#endif /*ENABLE_BIG_INT*/

/**
 * \ingroup context
 * \brief Private environment.
 */
typedef struct RJS_PrivateEnv_s RJS_PrivateEnv;

/**
 * \ingroup context
 * \brief Execution context.
 */
typedef struct RJS_Context_s RJS_Context;

/**
 * \ingroup realm
 * \brief Realm.
 */
typedef struct RJS_Realm_s RJS_Realm;

/**
 * \addtogroup value
 * @{
 */

/**Value type.*/
typedef enum {
    RJS_VALUE_UNDEFINED, /**< undefined.*/
    RJS_VALUE_NULL,      /**< null.*/
    RJS_VALUE_BOOLEAN,   /**< Boolean value.*/
    RJS_VALUE_STRING,    /**< String.*/
    RJS_VALUE_SYMBOL,    /**< Symbol.*/
    RJS_VALUE_NUMBER,    /**< Number.*/
    RJS_VALUE_OBJECT,    /**< Object.*/
    RJS_VALUE_GC_THING,  /**< Gc managed thing.*/
    RJS_VALUE_BIG_INT    /**< Big integer.*/
} RJS_ValueType;

/**Generic value.*/
typedef uint64_t RJS_Value;

/**
 * @}
 */

/**
 * \ingroup runtime
 * \brief Runtime.
 */
typedef struct RJS_Runtime_s RJS_Runtime;

/**
 * \addtogroup gc
 * @{
 */

/**GC mananged thing's type.*/
typedef enum {
    RJS_GC_THING_STRING,       /**< String.*/
    RJS_GC_THING_SYMBOL,       /**< Symbol.*/
    RJS_GC_THING_OBJECT,       /**< Object.*/
    RJS_GC_THING_ARRAY,        /**< Array.*/
    RJS_GC_THING_REGEXP,       /**< Regular expression.*/
    RJS_GC_THING_REGEXP_MODEL, /**< Regular expression model.*/
    RJS_GC_THING_SCRIPT_FUNC,  /**< Script function.*/
    RJS_GC_THING_BUILTIN_FUNC, /**< Built-in function.*/
    RJS_GC_THING_BOUND_FUNC,   /**< Bound function.*/
    RJS_GC_THING_PROMISE,      /**< Promise.*/
    RJS_GC_THING_PROMISE_STATUS,    /**< The promise's status.*/
    RJS_GC_THING_DECL_ENV,     /**< Declarative environment.*/
    RJS_GC_THING_OBJECT_ENV,   /**< Object environment.*/
    RJS_GC_THING_FUNCTION_ENV, /**< Function environment.*/
    RJS_GC_THING_MODULE_ENV,   /**< Module environment.*/
    RJS_GC_THING_GLOBAL_ENV,   /**< Global environment.*/
    RJS_GC_THING_PRIMITIVE,    /**< Primitive object.*/
    RJS_GC_THING_ARGUMENTS,    /**< Arguments object.*/
    RJS_GC_THING_ERROR,        /**< Error object.*/
    RJS_GC_THING_SCRIPT,       /**< Script.*/
    RJS_GC_THING_MODULE,       /**< Module.*/
    RJS_GC_THING_AST,          /**< Abstract syntax tree node.*/
    RJS_GC_THING_PROP_KEY_LIST,/**< Property keys list.*/
    RJS_GC_THING_STRING_ITERATOR,   /**< String iterator.*/
    RJS_GC_THING_ARRAY_ITERATOR,    /**< Array iterator.*/
    RJS_GC_THING_REGEXP_STRING_ITERATOR, /**< Regular expression string iterator.*/
    RJS_GC_THING_VALUE_BUFFER, /**< Value buffer.*/
    RJS_GC_THING_VALUE_LIST,   /**< Value list.*/
    RJS_GC_THING_INT,          /**< Integer value.*/
    RJS_GC_THING_REALM,        /**< Realm.*/
    RJS_GC_THING_RESOLVE_BINDING_LIST,   /**< Resolve binding list.*/
    RJS_GC_THING_BIG_INT,      /**< Big integer.*/
    RJS_GC_THING_PRIVATE_NAME, /**< Private name.*/
    RJS_GC_THING_PRIVATE_ENV,  /**< Private environment.*/
    RJS_GC_THING_CONTEXT,      /**< Running context.*/
    RJS_GC_THING_SCRIPT_CONTEXT,   /**< Script context.*/
    RJS_GC_THING_GENERATOR_CONTEXT,/**< Generator context.*/
    RJS_GC_THING_ASYNC_CONTEXT,/**< Async context.*/
    RJS_GC_THING_DATE,         /**< Date.*/
    RJS_GC_THING_ARRAY_BUFFER, /**< Array buffer.*/
    RJS_GC_THING_INT_INDEXED_OBJECT, /**< Integer indexed object.*/
    RJS_GC_THING_DATA_VIEW,    /**< Data view.*/
    RJS_GC_THING_PROXY_OBJECT, /**< Proxy object.*/
    RJS_GC_THING_HASH_ITERATOR,/**< Hash table iterator.*/
    RJS_GC_THING_MAP,          /**< Map.*/
    RJS_GC_THING_SET,          /**< Set.*/
    RJS_GC_THING_WEAK_MAP,     /**< Weak reference map.*/
    RJS_GC_THING_WEAK_SET,     /**< Weak reference set.*/
    RJS_GC_THING_WEAK_REF,     /**< Weak reference.*/
    RJS_GC_THING_FINALIZATION_REGISTRY, /**< Finalization registry.*/
    RJS_GC_THING_GENERATOR,    /**< Generaotr.*/
    RJS_GC_THING_ASYNC_GENERATOR,       /**< Async generator.*/
    RJS_GC_THING_ASYNC_FROM_SYNC_ITER,  /**< Async from sync iterator object.*/
    RJS_GC_THING_NATIVE_OBJECT,/**< Native object.*/
    RJS_GC_THING_CPTR,         /**< C pointer.*/
    RJS_GC_THING_MAX           /**< The maximum GC thing type value.*/
} RJS_GcThingType;

/**GC managed thing's operation functions.*/
typedef struct {
    RJS_GcThingType type; /**< Thing's type.*/
    void (*scan) (RJS_Runtime *rt, void *ptr); /**< Scan the referenced objects.*/
    void (*free) (RJS_Runtime *rt, void *ptr); /**< Free the thing.*/
} RJS_GcThingOps;

/**GC managed thing.*/
typedef struct {
    const RJS_GcThingOps *ops; /**< The operation functions' pointer.*/
    size_t next_flags; /**< The next GC thing's pointer and this thing's flags.*/
} RJS_GcThing;

/**
 * @}
 */

/**
 * \ingroup script
 * \brief Script.
 */
typedef struct RJS_Script_s RJS_Script;

/**
 * \addtogroup hash
 * @{
 */

/**Hash table entry.*/
typedef struct RJS_HashEntry_s RJS_HashEntry;

/**Hash table.*/
typedef struct RJS_Hash_s RJS_Hash;

/**Hash table operation functions.*/
typedef struct RJS_HashOps_s RJS_HashOps;

/**Hash table entry.*/
struct RJS_HashEntry_s {
    void          *key;  /**< The key of the entry.*/
    RJS_HashEntry *next; /**< The next entry in the list.*/
};

/**Hash table.*/
struct RJS_Hash_s {
    RJS_HashEntry **lists;     /**< The entries lists array.*/
    size_t          entry_num; /**< Entries number in the hash table.*/
    size_t          list_num;  /**< Lists number in the hash table.*/
};

/**Hash table operation functions.*/
struct RJS_HashOps_s {
    /**Memory buffer resize function.*/
    void*    (*realloc) (void *data, void *optr, size_t osize, size_t nsize);
    /**Key value calculate function.*/
    size_t   (*key) (void *data, void *key);
    /**Key euqal check function.*/
    RJS_Bool (*equal) (void *data, void *k1, void *k2);
};

/**
 * @}
 */

/**
 * \addtogroup string
 * @{
 */

/**String.*/
typedef struct {
    RJS_GcThing gc_thing;   /**< Base GC thing data.*/
    int         flags;      /**< Flags of the string.*/
    size_t      length;     /**< Length of the string.*/
    RJS_UChar  *uchars; /**< Unicode characters in the string.*/
} RJS_String;

/**
 * @}
 */

/**
 * \addtogroup symbol
 * @{
 */

/**Symbol.*/
typedef struct {
    RJS_GcThing gc_thing;    /**< Base GC thing data.*/
    RJS_Value   description; /**< Description of the symbol.*/
} RJS_Symbol;

/**
 * @}
 */

/**
 * \addtogroup object
 * @{
 */

/**Object.*/
typedef struct RJS_Object_s RJS_Object;

/**Property descriptor.*/
typedef struct {
    int        flags; /**< Flags.*/
    RJS_Value *get;   /**< The getter of the accessor.*/
    RJS_Value *set;   /**< The setter of the accessor.*/
    RJS_Value *value; /**< The value of the property.*/
} RJS_PropertyDesc;

/**Property name.*/
typedef struct {
    RJS_Value *name; /**< The name value.*/
} RJS_PropertyName;

/**Object's operation functions.*/
typedef struct {
    RJS_GcThingOps gc_thing_ops; /**< Base GC thing's operation functions.*/
    /**Get the prototype.*/
    RJS_Result (*get_prototype_of) (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto);
    /**Set the prototype.*/
    RJS_Result (*set_prototype_of) (RJS_Runtime *rt, RJS_Value *o, RJS_Value *proto);
    /**Check if the object is extensible.*/
    RJS_Result (*is_extensible) (RJS_Runtime *rt, RJS_Value *o);
    /**Prevent the extensions of the object.*/
    RJS_Result (*prevent_extensions) (RJS_Runtime *rt, RJS_Value *o);
    /**Get the own property's descriptor.*/
    RJS_Result (*get_own_property) (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd);
    /**Define an own property.*/
    RJS_Result (*define_own_property) (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_PropertyDesc *pd);
    /**Check if the object has the property.*/
    RJS_Result (*has_property) (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn);
    /**Get the property value.*/
    RJS_Result (*get) (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *receiver, RJS_Value *pv);
    /**Set the property value.*/
    RJS_Result (*set) (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn, RJS_Value *pv, RJS_Value *receiver);
    /**Delete a property.*/
    RJS_Result (*delete) (RJS_Runtime *rt, RJS_Value *o, RJS_PropertyName *pn);
    /**Get the own properties' keys.*/
    RJS_Result (*own_property_keys) (RJS_Runtime *rt, RJS_Value *o, RJS_Value *keys);
    /**Call a function.*/
    RJS_Result (*call) (RJS_Runtime *rt, RJS_Value *o, RJS_Value *thiz, RJS_Value *args, size_t argc, RJS_Value *rv);
    /**Construct a new object.*/
    RJS_Result (*construct) (RJS_Runtime *rt, RJS_Value *o, RJS_Value *args, size_t argc, RJS_Value *target, RJS_Value *rv);
} RJS_ObjectOps;

/**Property keys list.*/
typedef struct {
    RJS_GcThing                gc_thing; /**< Base GC thing*/
    RJS_VECTOR_DECL(RJS_Value) keys;     /**< Keys.*/
} RJS_PropertyKeyList;

/**
 * @}
 */

/**
 * \addtogroup misc
 * @{
 */

/**Native function.*/
typedef RJS_Result (*RJS_NativeFunc) (RJS_Runtime *rt,
        RJS_Value *f,
        RJS_Value *thiz,
        RJS_Value *args,
        size_t     argc,
        RJS_Value *new_target,
        RJS_Value *rv);

/**Declare a native function.*/
#define RJS_NF(name) RJS_Result name (RJS_Runtime *rt,\
        RJS_Value *f,\
        RJS_Value *thiz,\
        RJS_Value *args,\
        size_t     argc,\
        RJS_Value *nt,\
        RJS_Value *rv)

/**
 * @}
 */

/**
 * \addtogroup env
 * @{
 */

/**Binding name.*/
typedef struct RJS_BindingName_s RJS_BindingName;

/**Environment.*/
typedef struct RJS_Environment_s RJS_Environment;

/**Environment operation functions.*/
typedef struct {
    RJS_GcThingOps gc_thing_ops; /**< Base GC thing's operation functions.*/
    /**Check if the environment has the binding.*/
    RJS_Result (*has_binding) (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n);
    /**Create a mutable binding.*/
    RJS_Result (*create_mutable_binding) (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool del);
    /**Create an immutable binding.*/
    RJS_Result (*create_immutable_binding) (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict);
    /**Initialize the binding.*/
    RJS_Result (*initialize_binding) (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v);
    /**Set the mutable binding.*/
    RJS_Result (*set_mutable_binding) (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Value *v, RJS_Bool strict);
    /**Get the binding's value.*/
    RJS_Result (*get_binding_value) (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n, RJS_Bool strict, RJS_Value *v);
    /**Delete a binding.*/
    RJS_Result (*delete_binding) (RJS_Runtime *rt, RJS_Environment *env, RJS_BindingName *n);
    /**Check if the environment has this binding.*/
    RJS_Result (*has_this_binding) (RJS_Runtime *rt, RJS_Environment *env);
    /**Check if the environemnt has the super binding.*/
    RJS_Result (*has_super_binding) (RJS_Runtime *rt, RJS_Environment *env);
    /**Get base object of the with environment.*/
    RJS_Result (*with_base_object) (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *base);
    /**Get this binding.*/
    RJS_Result (*get_this_binding) (RJS_Runtime *rt, RJS_Environment *env, RJS_Value *v);
} RJS_EnvOps;

/**Binding name.*/
struct RJS_BindingName_s {
    RJS_Value *name; /**< The name of the binding.*/
};

/**
 * @}
 */

/**
 * \ingroup promise
 * \brief Promise capability.
 */
typedef struct {
    RJS_Value *promise;  /**< The promise.*/
    RJS_Value *resolve;  /**< The resolve function.*/
    RJS_Value *reject;   /**< The reject function.*/
} RJS_PromiseCapability;

/**
 * \addtogroup context
 * @{
 */

/**Running context.*/
typedef struct RJS_Context_s RJS_Context;

/**Context.*/
struct RJS_Context_s {
    RJS_GcThing   gc_thing;   /**< Base GC thing data.*/
    RJS_Context  *bot;        /**< The bottom context in the stack.*/
    RJS_Realm    *realm;      /**< The current realm.*/
    RJS_Value     function;   /**< The function.*/
};

/**Script context base data.*/
typedef struct {
    RJS_Context      context;     /**< Base context data.*/
    RJS_Environment *lex_env;     /**< The lexical environment.*/
    RJS_Environment *var_env;     /**< The variable environment.*/
#if ENABLE_PRIV_NAME
    RJS_PrivateEnv  *priv_env;    /**< The private environment.*/
#endif /*ENABLE_PRIV_NAME*/
} RJS_ScriptContextBase;

/**
 * @}
 */

/**
 * \addtogroup nstack
 * @{
 */

/**State.*/
typedef struct RJS_State_s RJS_State;

/**Native stack*/
typedef struct {
    RJS_VECTOR_DECL(RJS_Value) value; /**< The value stack.*/
    RJS_VECTOR_DECL(RJS_State) state; /**< The state stack.*/
} RJS_NativeStack;

/**
 * @}
 */

/**Job function.*/
typedef void (*RJS_JobFunc) (RJS_Runtime *rt, void *data);
/**Data scan function.*/
typedef void (*RJS_ScanFunc) (RJS_Runtime *rt, void *data);
/**Data free function.*/
typedef void (*RJS_FreeFunc) (RJS_Runtime *rt, void *data);

/**
 * \addtogroup runtime
 * @{
 */

/**
 * Solve event callback function.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
typedef RJS_Result (*RJS_EventFunc) (RJS_Runtime *rt);

/**
 * The callback function check the new loading module's pathname.
 * 
 * This function is invoked when the engine try to load a new module.
 * This function check if the module can be loaded and return the pathname of the module.
 * 
 * \param rt The current runtime.
 * \param base The base module's pathname which try to load the new module.
 * If the new module is loading by the native code, "base" is NULL.
 * \param name The new module's name.
 * \param path The new module's pathname output buffer.
 * \param size The size of the pathname output buffer.
 * \retval RJS_OK The new module can be loaded. And its pathname is stored in "path".
 * \retval RJS_ERR The new module cannot be loaded.
 */
typedef RJS_Result (*RJS_ModulePathFunc) (RJS_Runtime *rt, const char *base,
        const char *name, char *path, size_t size);

/**
 * @}
 */

/**
 * \ingroup realm
 * \brief Realm base data.
 */
typedef struct {
    RJS_GcThing      gc_thing;      /**< Base GC thing data.*/
    RJS_Environment *global_env;    /**< The global environment.*/
    RJS_Value        global_object; /**< The global object.*/
} RJS_RealmBase;

/**
 * \ingroup runtime
 * \brief Runtime base data.
 */
typedef struct {
    RJS_Value        v_undefined;          /**< Value undefined.*/
    RJS_Value        v_null;               /**< Value null.*/
    RJS_VECTOR_DECL(RJS_GcThing*) gc_mark_stack;   /**< The marked GC things' stack.*/
    RJS_Bool         gc_enable;            /**< GC is enabled.*/
    RJS_Bool         gc_running;           /**< GC is running.*/
    RJS_Bool         gc_mark_stack_full;   /**< The mark stack is full.*/
    RJS_NativeStack *curr_native_stack;    /**< The current native stack.*/
    RJS_Context     *ctxt_stack;           /**< The context stack.*/
    RJS_Realm       *bot_realm;            /**< The bottom realm.*/
} RJS_RuntimeBase;

/**
 * \addtogroup array_buffer
 * @{
 */

/**Array element type.*/
typedef enum {
    RJS_ARRAY_ELEMENT_UINT8,     /**< 8 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_INT8,      /**< 8 bits singed integer.*/
    RJS_ARRAY_ELEMENT_UINT8C,    /**< 8 bits unsigned integer (clamped conversion).*/
    RJS_ARRAY_ELEMENT_UINT16,    /**< 16 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_INT16,     /**< 16 bits signed integer.*/
    RJS_ARRAY_ELEMENT_UINT32,    /**< 32 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_INT32,     /**< 32 bits signed integer.*/
    RJS_ARRAY_ELEMENT_FLOAT32,   /**< 32 bits float point number.*/
    RJS_ARRAY_ELEMENT_FLOAT64,   /**< 64 bits float point number.*/
    RJS_ARRAY_ELEMENT_BIGUINT64, /**< 64 bits unsigned integer.*/
    RJS_ARRAY_ELEMENT_BIGINT64,  /**< 64 bits signed integer.*/
    RJS_ARRAY_ELEMENT_MAX
} RJS_ArrayElementType;

#if __SIZEOF_INT__ == 8
    #define RJS_ARRAY_ELEMENT_INT  RJS_ARRAY_ELEMENT_BIGINT64
    #define RJS_ARRAY_ELEMENT_UINT RJS_ARRAY_ELEMENT_BIGUINT64
#else
    #define RJS_ARRAY_ELEMENT_INT  RJS_ARRAY_ELEMENT_INT32
    #define RJS_ARRAY_ELEMENT_UINT RJS_ARRAY_ELEMENT_UINT32
#endif

#if __SIZEOF_LONG__ == 8
    #define RJS_ARRAY_ELEMENT_LONG  RJS_ARRAY_ELEMENT_BIGINT64
    #define RJS_ARRAY_ELEMENT_ULONG RJS_ARRAY_ELEMENT_BIGUINT64
#else
    #define RJS_ARRAY_ELEMENT_LONG  RJS_ARRAY_ELEMENT_INT32
    #define RJS_ARRAY_ELEMENT_ULONG RJS_ARRAY_ELEMENT_UINT32
#endif

#if __SIZEOF_SIZE_T__ == 8
    #define RJS_ARRAY_ELEMENT_SSIZE_T RJS_ARRAY_ELEMENT_BIGINT64
    #define RJS_ARRAY_ELEMENT_SIZE_T  RJS_ARRAY_ELEMENT_BIGUINT64
#else
    #define RJS_ARRAY_ELEMENT_SSIZE_T RJS_ARRAY_ELEMENT_INT32
    #define RJS_ARRAY_ELEMENT_SIZE_T  RJS_ARRAY_ELEMENT_UINT32
#endif

#define RJS_ARRAY_ELEMENT_CHAR   RJS_ARRAY_ELEMENT_INT8
#define RJS_ARRAY_ELEMENT_UCHAR  RJS_ARRAY_ELEMENT_UINT8
#define RJS_ARRAY_ELEMENT_SHORT  RJS_ARRAY_ELEMENT_INT16
#define RJS_ARRAY_ELEMENT_USHORT RJS_ARRAY_ELEMENT_UINT16
#define RJS_ARRAY_ELEMENT_LLONG  RJS_ARRAY_ELEMENT_BIGINT64
#define RJS_ARRAY_ELEMENT_ULLONG RJS_ARRAY_ELEMENT_BIGUINT64

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

