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
 * Native stack internal header.
 */

#ifndef _RJS_NATIVE_STACK_INTERNAL_H_
#define _RJS_NATIVE_STACK_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**State type.*/
typedef enum {
    RJS_STATE_LEX_ENV,    /**< Lexical environment.*/
    RJS_STATE_FOR_IN,     /**< For in iterator.*/
    RJS_STATE_FOR_OF,     /**< For or iterator.*/
    RJS_STATE_ARRAY_ASSI, /**< Array assignment pattern iterator.*/
    RJS_STATE_CLASS,      /**< Class.*/
    RJS_STATE_CALL,       /**< Call.*/
    RJS_STATE_ARRAY,      /**< Array.*/
    RJS_STATE_OBJECT,     /**< Object.*/
    RJS_STATE_OBJECT_ASSI,/**< Object pattern assignment.*/
    RJS_STATE_TRY         /**< Try...catch.*/
} RJS_StateType;

/**Try state.*/
typedef enum {
    RJS_TRY_STATE_TRY,     /**< In try block.*/
    RJS_TRY_STATE_CATCH,   /**< In catch block.*/
    RJS_TRY_STATE_FINALLY, /**< In finally block.*/
    RJS_TRY_STATE_END      /**< End of the full try catch fiannlly block.*/
} RJS_TryState;

/**Next operation of try.*/
typedef enum {
    RJS_TRY_NEXT_OP_NORMAL, /**< Run the next instruction.*/
    RJS_TRY_NEXT_OP_THROW,  /**< Throw an error.*/
    RJS_TRY_NEXT_OP_RETURN  /**< Return from the function.*/
} RJS_TryNextOp;

/**State class's element list entry.*/
typedef struct {
    RJS_List             ln;    /**< List node data.*/
    RJS_ClassElementType type;  /**< The type of the element.*/
    RJS_Value            name;  /**< The name of the field.*/
    RJS_Value            value; /**< The value of the element.*/
    RJS_Bool             is_af; /**< The field is an anonymous function.*/
} RJS_StateClassElement;

/**State.*/
struct RJS_State_s {
    RJS_StateType type;          /**< State type.*/
    size_t        sp;            /**< Stack pointer.*/
    union {
        struct {
            RJS_Context     *context;/**< The running context.*/
        } s_ctxt;                /**< The context data.*/
        struct {
            RJS_IteratorType type;       /**< Iterator type.*/
            RJS_Iterator    *iterator;   /**< Iterator record.*/
        } s_iter;                /**< Iterator state.*/
        struct {
            RJS_Value       *proto;      /**< The prototype value.*/
            RJS_Value       *constr;     /**< The constructor value.*/
            RJS_List        *elem_list;  /**< Element list.*/
#if ENABLE_PRIV_NAME
            RJS_PrivateEnv  *priv_env;   /**< The private environment.*/
#endif /*ENABLE_PRIV_NAME*/
            size_t     inst_priv_method_num; /**< The number of instance's private methods.*/
            size_t     inst_field_num;       /**< The number of instance's fields.*/
        } s_class;               /**< Class state.*/
        struct {
            RJS_Value    *func;        /**< The function.*/
            RJS_Value    *thiz;        /**< This argument.*/
            RJS_Value    *args;        /**< The arguments.*/
            size_t        argc;        /**< The arguments' count.*/
        } s_call;                  /**< Call state.*/
        struct {
            RJS_Value   *array;    /**< The array value.*/
            size_t       index;    /**< The current item index.*/
        } s_array;                 /**< Array state.*/
        struct {
            RJS_Value  *object;    /**< The object value.*/
        } s_object;                /**< Object state.*/
        struct {
            RJS_Value  *object;    /**< The object value.*/
            RJS_Hash    prop_hash; /**< The property hash table.*/
        } s_object_assi;           /**< Object assignment state.*/
        struct {
            RJS_TryState  state;      /**< Current state.*/
            RJS_TryNextOp next_op;    /**< The next operation.*/
            RJS_Value    *error;      /**< The error value.*/
            size_t        catch_ip;   /**< Catch instruction pointer.*/
            size_t        finally_ip; /**< Finally instruction pointer.*/
            size_t        next_ip;    /**< Jump destination instruction pointer.*/
        } s_try;                 /**< Try state.*/
    } s;
};

/**
 * Get the top state in the stack.
 * \param rt The current runtime.
 * \return The top state in the stack.
 */
static inline RJS_State*
rjs_state_top (RJS_Runtime *rt)
{
    assert(rt->rb.curr_native_stack->state.item_num);

    return &rt->rb.curr_native_stack->state.items[rt->rb.curr_native_stack->state.item_num - 1];
}

/**
 * Get the nth state in the stack.
 * \param rt The current runtime.
 * \param n The depth of the state.
 * \return The top state in the stack.
 */
static inline RJS_State*
rjs_state_top_n (RJS_Runtime *rt, size_t n)
{
    assert(rt->rb.curr_native_stack->state.item_num > n);

    return &rt->rb.curr_native_stack->state.items[rt->rb.curr_native_stack->state.item_num - n - 1];
}

/**
 * Release the state.
 * \param rt The current runtime.
 * \param s The state to be released.
 * \param op The await operation function.
 * \param ip The integer parameter of the function.
 * \param vp The value parameter of the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 * \retval RJS_FALSE Async wait a promise.
 */
extern RJS_Result
rjs_state_deinit (RJS_Runtime *rt, RJS_State *s, RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp);

/**
 * Popup the top state in the stack.
 * \param rt The current runtime.
 * \param op The await operation function.
 * \param ip The integer parameter of the function.
 * \param vp The value parameter of the function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 * \retval RJS_FALSE Async wait a promise.
 */
static inline RJS_Result
rjs_state_pop_await (RJS_Runtime *rt, RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp)
{
    RJS_State *s = rjs_state_top(rt);
    RJS_Result r;

    r = rjs_state_deinit(rt, s, op, ip, vp);

    rt->rb.curr_native_stack->state.item_num --;

    return r;
}

/**
 * Popup the top state in the stack.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_state_pop (RJS_Runtime *rt)
{
    RJS_State *s = rjs_state_top(rt);
    RJS_Result r;

    r = rjs_state_deinit(rt, s, NULL, 0, NULL);

    rt->rb.curr_native_stack->state.item_num --;

    return r;
}

/**
 * Push a new lexical environment state to the stack,
 * \param rt The current runtime.
 * \param env The lexical environment to be pushed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_lex_env_state_push (RJS_Runtime *rt, RJS_Environment *env);

/**
 * Push a new enumeration state to the stack,
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_enum_state_push (RJS_Runtime *rt, RJS_Value *v);

/**
 * Push a new iterator state to the stack,
 * \param rt The current runtime.
 * \param o The object.
 * \param type The iterator type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iter_state_push (RJS_Runtime *rt, RJS_Value *iter, RJS_IteratorType type);

/**
 * Push a new array assignment state in the stack,
 * \param rt The current runtime.
 * \param array The array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_array_assi_state_push (RJS_Runtime *rt, RJS_Value *array);

/**
 * Get the iterator's value and jump to the next position.
 * \param rt The current runtime.
 * \param[out] rv Return the current value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The iterator is done.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iter_state_step (RJS_Runtime *rt, RJS_Value *rv);

#if ENABLE_ASYNC
/**
 * Await for of step.
 * \param rt The current runtime.
 * \retval RJS_FALSE Await a promise.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iter_state_async_step (RJS_Runtime *rt);

/**
 * Resume await for step.
 * \param rt The current runtime.
 * \param ir The iterator result.
 * \param[out] rv The return value.
 * \retval RJS_ERR On error.
 * \retval RJS_FALSE The iterator is done.
 * \retval RJS_OK On success.
 */
extern RJS_Result
rjs_iter_state_async_step_resume (RJS_Runtime *rt, RJS_Value *ir, RJS_Value *rv);
#endif /*ENABLE_ASYNC*/

/**
 * Create an array from the rest items of the iterator.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_iter_state_rest (RJS_Runtime *rt, RJS_Value *rv);

/**
 * Push a new class state to the stack,
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_class_state_push (RJS_Runtime *rt);

#if ENABLE_PRIV_NAME

/**
 * Set the class state's private environment.
 * \param rt The current runtime.
 * \param script The script.
 * \param pe The private environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_class_state_set_priv_env (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptPrivEnv *pe);

#endif /*ENABLE_PRIV_NAME*/

/**
 * Create the constructor for the class state in the stack.
 * \param rt The current runtime.
 * \param cp The constructor's parent.
 * \param proto The prototype.
 * \param script The script.
 * \param sf The script function.
 * \param constr Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_class_state_create_constructor (RJS_Runtime *rt, RJS_Value *cp, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Value *constr);

/**
 * Create the default constructor for the class state in the stack.
 * \param rt The current runtime.
 * \param cp The constructor's parent.
 * \param proto The prototype.
 * \param name The class's name.
 * \param derived The class is derived.
 * \param constr Return the constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_class_state_create_default_constructor (RJS_Runtime *rt, RJS_Value *cp, RJS_Value *proto,
        RJS_Value *name, RJS_Bool derived, RJS_Value *constr);

/**
 * Add an element to the class state.
 * \param rt The current runtime.
 * \param type The element type.
 * \param name The name of the element.
 * \param script The script.
 * \param func The script function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_class_state_add_element (RJS_Runtime *rt, RJS_ClassElementType type, RJS_Value *name,
        RJS_Script *script, RJS_ScriptFunc *sf);

/**
 * Set the last class element as anonymous function field.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_class_state_set_anonymous_function_field (RJS_Runtime *rt);

/**
 * Initialize the class on the top of the state stack.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_class_state_init (RJS_Runtime *rt);

/**
 * Push a new call state to the stack.
 * \param rt The current runtime.
 * \param func The function to be called.
 * \param thiz This argument.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_call_state_push (RJS_Runtime *rt, RJS_Value *func, RJS_Value *thiz);

/**
 * Push a new call state to the stack for super call.
 * \param rt the current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_super_call_state_push (RJS_Runtime *rt);

/**
 * Push a new call state to the stack for construct.
 * \param rt The current runtime.
 * \param c The constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_new_state_push (RJS_Runtime *rt, RJS_Value *c);

/**
 * Push an argument to the stack.
 * \param rt The current runtime.
 * \param arg The argument to be pushed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_call_state_push_arg (RJS_Runtime *rt, RJS_Value *arg);

/**
 * Push spread arguments to the stack.
 * \param rt The current runtime.
 * \param args The arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_call_state_push_spread_args (RJS_Runtime *rt, RJS_Value *args);

/**Direct eval.*/
#define RJS_CALL_FL_EVAL 1
/**Enable tal call optimize.*/
#define RJS_CALL_FL_TCO  2

/**
 * Call the function use the top call state in the stack.
 * \param rt The current runtime.
 * \param sp The native stack pointer.
 * \param flags Call flags.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_call_state_call (RJS_Runtime *rt, size_t sp, int flags, RJS_Value *rv);

/**
 * Call the super constructor use the top call state in the stack.
 * \param rt The current runtime.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_call_state_super_call (RJS_Runtime *rt, RJS_Value *rv);

/**
 * Construct the new object use the top call state in the stack.
 * \param rt The current runtime.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_call_state_new (RJS_Runtime *rt, RJS_Value *rv);

/**
 * Push a new array state in the stack,
 * \param rt The current runtime.
 * \param array The array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_array_state_push (RJS_Runtime *rt, RJS_Value *array);

/**
 * Add an elistion element to the array state.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
static inline RJS_Result
rjs_array_state_elision (RJS_Runtime *rt)
{
    RJS_State *s = rjs_state_top(rt);

    assert(s->type == RJS_STATE_ARRAY);

    s->s.s_array.index ++;

    return RJS_OK;
}

/**
 * Add an element to the array state.
 * \param rt The current runtime.
 * \param v The element value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_array_state_add (RJS_Runtime *rt, RJS_Value *v);

/**
 * Add elements to the array state.
 * \param rt The current runtime.
 * \param v The value, all the elements in it will be added to the array.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_array_state_spread (RJS_Runtime *rt, RJS_Value *v);

/**
 * Push a new object state in the stack,
 * \param rt The current runtime.
 * \param o The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_state_push (RJS_Runtime *rt, RJS_Value *o);

/**
 * Add a property to the object state in the stack.
 * \param rt The current runtime.
 * \param name The property name.
 * \param value The property value.
 * \param is_af The property value is an anonymous function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_state_add (RJS_Runtime *rt, RJS_Value *name, RJS_Value *value, RJS_Bool is_af);

/**
 * Add properties to the object state in the stack.
 * \param rt The current runtime.
 * \param value The value. All the properties will be added to the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_state_spread (RJS_Runtime *rt, RJS_Value *value);

/**
 * Add a method or accessor to the object state in the stack.
 * \param rt The current runtime.
 * \param type The element type.
 * \param name The name of the element.
 * \param script The script.
 * \param sf The script function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_state_add_element (RJS_Runtime *rt, RJS_ClassElementType type, RJS_Value *name,
        RJS_Script *script, RJS_ScriptFunc *sf);

/**
 * Push a new object assignment state in the stack,
 * \param rt The current runtime.
 * \param o The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_assi_state_push (RJS_Runtime *rt, RJS_Value *o);

/**
 * Get a property value from the object assignment state in the stack.
 * \param rt The current runtime.
 * \param pn The property name.
 * \param[out] rv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_assi_state_step (RJS_Runtime *rt, RJS_PropertyName *pn, RJS_Value *rv);

/**
 * Get an object from the rest properties of the object assignment state in the stack.
 * \param rt The current runtime.
 * \param[out] rv Return the object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_assi_state_rest (RJS_Runtime *rt, RJS_Value *rv);

/**
 * Push a new try state in the stack,
 * \param rt The current runtime.
 * \param catch_ip The catch insrtruction pointer.
 * \param finally_ip The finally instruction pointer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_try_state_push (RJS_Runtime *rt, size_t catch_ip, size_t finally_ip);

/**
 * Initialize the native stack.
 * \param ns The native stack to be initialized.
 */
static inline void
rjs_native_stack_init (RJS_NativeStack *ns)
{
    rjs_vector_init(&ns->value);
    rjs_vector_init(&ns->state);
}

/**
 * Release the native stack.
 * \param rt The current runtime.
 * \param ns The native stack to be released.
 */
extern void
rjs_native_stack_deinit (RJS_Runtime *rt, RJS_NativeStack *ns);

/**
 * Clear the resouce in the native stack.
 * \param rt The current runtime.
 * \param ns The native stack to be cleared.
 */
extern void
rjs_native_stack_clear (RJS_Runtime *rt, RJS_NativeStack *ns);

/**
 * Scan the referenced things in the native stack.
 * \param rt The current runtime.
 * \param ns The native stack to be scanned.
 */
extern void
rjs_gc_scan_native_stack (RJS_Runtime *rt, RJS_NativeStack *ns);

#ifdef __cplusplus
}
#endif

#endif

