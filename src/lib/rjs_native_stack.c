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

#include "ratjs_internal.h"

/**String property entry.*/
typedef struct {
    RJS_HashEntry he;    /**< Hash table entry.*/
    RJS_Value     value; /**< Value.*/
} RJS_StringPropEntry;

/*Push a new state to the stack.*/
static RJS_State*
state_push (RJS_Runtime *rt, RJS_StateType type)
{
    RJS_State *s;

    rjs_vector_set_capacity(&rt->rb.curr_native_stack->state, rt->rb.curr_native_stack->state.item_num + 1, rt);

    s = &rt->rb.curr_native_stack->state.items[rt->rb.curr_native_stack->state.item_num ++];

    s->sp   = rjs_value_stack_save(rt);
    s->type = type;

    return s;
}

/*Scan the referenced things in the state.*/
static void
state_scan (RJS_Runtime *rt, RJS_State *s)
{
    switch (s->type) {
    case RJS_STATE_LEX_ENV:
        if (s->s.s_ctxt.context)
            rjs_gc_mark(rt, s->s.s_ctxt.context);
        break;
    case RJS_STATE_CLASS: {
        RJS_StateClassElement *ce;

        rjs_list_foreach_c(s->s.s_class.elem_list, ce, RJS_StateClassElement, ln) {
            rjs_gc_scan_value(rt, &ce->name);
            rjs_gc_scan_value(rt, &ce->value);
        }
        break;
    }
    case RJS_STATE_OBJECT_ASSI: {
        size_t               i;
        RJS_StringPropEntry *e;

        rjs_hash_foreach_c(&s->s.s_object_assi.prop_hash, i, e, RJS_StringPropEntry, he) {
            rjs_gc_scan_value(rt, &e->value);
        }
        break;
    }
    case RJS_STATE_FOR_IN:
    case RJS_STATE_FOR_OF:
    case RJS_STATE_ARRAY_ASSI:
    case RJS_STATE_CALL:
    case RJS_STATE_ARRAY:
    case RJS_STATE_OBJECT:
    case RJS_STATE_TRY:
        break;
    }
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
RJS_Result
rjs_state_deinit (RJS_Runtime *rt, RJS_State *s, RJS_AsyncOpFunc op, size_t ip, RJS_Value *vp)
{
    RJS_Result r   = RJS_OK;
    size_t     top = s->sp;

    switch (s->type) {
    case RJS_STATE_LEX_ENV: {
        RJS_ScriptContext *sc  = (RJS_ScriptContext*)s->s.s_ctxt.context;
        RJS_Environment   *env = sc->scb.lex_env;

        if (env)
            sc->scb.lex_env = env->outer;
        break;
    }
    case RJS_STATE_FOR_IN:
    case RJS_STATE_FOR_OF:
    case RJS_STATE_ARRAY_ASSI:
        if (s->s.s_iter.iterator) {
            RJS_Iterator *iter = s->s.s_iter.iterator;

            if (((s->type == RJS_STATE_FOR_OF) || (s->type == RJS_STATE_ARRAY_ASSI))
                    && !iter->done) {
#if ENABLE_ASYNC
                if (s->s.s_iter.type == RJS_ITERATOR_ASYNC)
                    r = rjs_async_iterator_close(rt, iter, op, ip, vp);
                else
#endif /*ENABLE_ASYNC*/
                    r = rjs_iterator_close(rt, iter);
            }

            rjs_iterator_deinit(rt, iter);
            RJS_DEL(rt, iter);
        }
        break;
    case RJS_STATE_CLASS: {
        RJS_StateClassElement *ce, *nce;

        rjs_list_foreach_safe_c(s->s.s_class.elem_list, ce, nce, RJS_StateClassElement, ln) {
            RJS_DEL(rt, ce);
        }

        RJS_DEL(rt, s->s.s_class.elem_list);

#if ENABLE_PRIV_NAME
        /*Restore the private environment.*/
        if (s->s.s_class.priv_env)
            rjs_private_env_pop(rt, s->s.s_class.priv_env);
#endif /*ENABLE_PRIV_NAME*/
        break;
    }
    case RJS_STATE_ARRAY: {
        size_t     top = rjs_value_stack_save(rt);
        RJS_Value *len = rjs_value_stack_push(rt);

        rjs_value_set_number(rt, len, s->s.s_array.index);
        r = rjs_set(rt, s->s.s_array.array, rjs_pn_length(rt), len, RJS_TRUE);

        rjs_value_stack_restore(rt, top);
        break;
    }
    case RJS_STATE_OBJECT_ASSI: {
        size_t               i;
        RJS_StringPropEntry *e, *ne;

        rjs_hash_foreach_safe_c(&s->s.s_object_assi.prop_hash, i, e, ne, RJS_StringPropEntry, he) {
            RJS_DEL(rt, e);
        }

        rjs_hash_deinit(&s->s.s_object_assi.prop_hash, &rjs_hash_value_ops, rt);
        break;
    }
    case RJS_STATE_CALL:
    case RJS_STATE_OBJECT:
    case RJS_STATE_TRY:
        break;
    }

    if (r == RJS_OK)
        rjs_value_stack_restore(rt, top);

    return r;
}

/**
 * Push a new lexical environment state to the stack,
 * \param rt The current runtime.
 * \param env The lexical environment to be pushed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_lex_env_state_push (RJS_Runtime *rt, RJS_Environment *env)
{
    RJS_ScriptContext *sc = (RJS_ScriptContext*)rjs_context_running(rt);
    RJS_State         *s;

    s = state_push(rt, RJS_STATE_LEX_ENV);

    sc->scb.lex_env = env;
    s->s.s_ctxt.context = &sc->scb.context;

    return RJS_OK;
}

/**
 * Push a new enumeration state to the stack,
 * \param rt The current runtime.
 * \param v The value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_enum_state_push (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_Result   r;
    size_t       top;
    RJS_Value   *iterv;
    RJS_State   *s;

    s = state_push(rt, RJS_STATE_FOR_IN);

    RJS_NEW(rt, s->s.s_iter.iterator);

    s->s.s_iter.type = RJS_ITERATOR_SYNC;
    rjs_iterator_init(rt, s->s.s_iter.iterator);

    top   = rjs_value_stack_save(rt);
    iterv = rjs_value_stack_push(rt);

    if ((r = rjs_for_in_iterator_new(rt, iterv, v)) == RJS_OK) {
        if ((r = rjs_get_iterator(rt, iterv, RJS_ITERATOR_SYNC, NULL, s->s.s_iter.iterator)) == RJS_ERR) {
            s = rjs_state_top(rt);
            s->s.s_iter.iterator->done = RJS_TRUE;
            rjs_state_pop(rt);
            return r;
        }
    }

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

/**
 * Push a new iterator state to the stack,
 * \param rt The current runtime.
 * \param o The object.
 * \param type The iterator type.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iter_state_push (RJS_Runtime *rt, RJS_Value *o, RJS_IteratorType type)
{
    RJS_Result  r;
    RJS_State  *s;

    s = state_push(rt, RJS_STATE_FOR_OF);

    RJS_NEW(rt, s->s.s_iter.iterator);

    s->s.s_iter.type = type;
    rjs_iterator_init(rt, s->s.s_iter.iterator);

    if ((r = rjs_get_iterator(rt, o, type, NULL, s->s.s_iter.iterator)) == RJS_ERR) {
        s = rjs_state_top(rt);
        s->s.s_iter.iterator->done = RJS_TRUE;
        rjs_state_pop(rt);
        return r;
    }

    return RJS_OK;
}

/**
 * Push a new array assignment state in the stack,
 * \param rt The current runtime.
 * \param array The array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_array_assi_state_push (RJS_Runtime *rt, RJS_Value *array)
{
    RJS_Result r;

    if ((r = rjs_iter_state_push(rt, array, RJS_ITERATOR_SYNC)) == RJS_OK) {
        RJS_State *s = rjs_state_top(rt);

        s->type = RJS_STATE_ARRAY_ASSI;
    }

    return r;
}

/**
 * Get the iterator's value and jump to the next position.
 * \param rt The current runtime.
 * \param[out] rv Return the current value.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE The iterator is done.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iter_state_step (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_State    *s      = rjs_state_top(rt);
    RJS_Result    r      = RJS_FALSE;
    size_t        top    = rjs_value_stack_save(rt);
    RJS_Value    *result = rjs_value_stack_push(rt);
    RJS_Iterator *iter   = s->s.s_iter.iterator;

    assert((s->type == RJS_STATE_FOR_IN)
            || (s->type == RJS_STATE_FOR_OF)
            || (s->type == RJS_STATE_ARRAY_ASSI));

    if (!iter->done) {
        if ((r = rjs_iterator_step(rt, iter, result)) == RJS_ERR)
            goto end;
    }

    if (rv) {
        if (r) {
            s = rjs_state_top(rt);
            r = rjs_iterator_value(rt, result, rv);
        } else {
            rjs_value_set_undefined(rt, rv);
        }
    }
end:
    if (r != RJS_TRUE) {
        s = rjs_state_top(rt);
        iter->done = RJS_TRUE;
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_ASYNC

/*Await async iterator step.*/
static RJS_Result
await_async_iter_step (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    RJS_Result r;

    ASYNC_OP_DEBUG();

    r = (type == RJS_SCRIPT_CALL_ASYNC_FULFILL) ? RJS_OK : RJS_ERR;
    if (r == RJS_ERR) {
        RJS_State *s = rjs_state_top(rt);

        assert(s->type == RJS_STATE_FOR_OF);

        s->s.s_iter.iterator->done = RJS_TRUE;
        rjs_throw(rt, iv);
    }

    return r;
}

/**
 * Await for of step.
 * \param rt The current runtime.
 * \retval RJS_FALSE Await a promise.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iter_state_async_step (RJS_Runtime *rt)
{
    RJS_State    *s      = rjs_state_top(rt);
    size_t        top    = rjs_value_stack_save(rt);
    RJS_Value    *result = rjs_value_stack_push(rt);
    RJS_Iterator *iter   = s->s.s_iter.iterator;
    RJS_Result    r;

    assert(s->type == RJS_STATE_FOR_OF);

    if ((r = rjs_call(rt, iter->next_method, iter->iterator, NULL, 0, result)) == RJS_ERR)
        goto end;

    r = rjs_await(rt, result, await_async_iter_step, 0, NULL);
end:
    if (r == RJS_ERR) {
        s = rjs_state_top(rt);
        s->s.s_iter.iterator->done = RJS_TRUE;
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Resume await for step.
 * \param rt The current runtime.
 * \param ir The iterator result.
 * \param[out] rv The return value.
 * \retval RJS_ERR On error.
 * \retval RJS_FALSE The iterator is done.
 * \retval RJS_OK On success.
 */
RJS_Result
rjs_iter_state_async_step_resume (RJS_Runtime *rt, RJS_Value *ir, RJS_Value *rv)
{
    RJS_State  *s = rjs_state_top(rt);
    RJS_Result  r;

    assert(s->type == RJS_STATE_FOR_OF);

    if (!rjs_value_is_object(rt, ir)) {
        r = rjs_throw_type_error(rt, _("the result is not an object"));
        goto end;
    }

    if ((r = rjs_iterator_complete(rt, ir)) == RJS_ERR)
        goto end;

    if (r) {
        rjs_value_set_undefined(rt, rv);

        r = RJS_FALSE;
    } else {
        if ((r = rjs_iterator_value(rt, ir, rv)) == RJS_ERR)
            goto end;

        r = RJS_OK;
    }
end:
    if (r != RJS_OK) {
        s = rjs_state_top(rt);
        s->s.s_iter.iterator->done = RJS_TRUE;
    }
    return r;
}

#endif /*ENABLE_ASYNC*/

/**
 * Create an array from the rest items of the iterator.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_iter_state_rest (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_State    *s      = rjs_state_top(rt);
    size_t        top    = rjs_value_stack_save(rt);
    RJS_Value    *result = rjs_value_stack_push(rt);
    RJS_Value    *item   = rjs_value_stack_push(rt);
    size_t        i      = 0;
    RJS_Iterator *iter   = s->s.s_iter.iterator;
    RJS_Result    r;

    assert(s->type == RJS_STATE_ARRAY_ASSI);

    if ((r = rjs_array_new(rt, rv, 0, NULL)) == RJS_ERR)
        goto end;

    while (!iter->done) {
        r = rjs_iterator_step(rt, iter, result);
        s = rjs_state_top(rt);
        if (r == RJS_ERR) {
            iter->done = RJS_TRUE;
            goto end;
        }

        if (!r) {
            iter->done = RJS_TRUE;
            break;
        }

        if ((r = rjs_iterator_value(rt, result, item)) == RJS_ERR)
            goto end;

        if ((r = rjs_create_data_property_or_throw_index(rt, rv, i, item)) == RJS_ERR)
            goto end;

        i ++;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Push a new class state to the stack,
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_class_state_push (RJS_Runtime *rt)
{
    RJS_State *s;

    s = state_push(rt, RJS_STATE_CLASS);

    s->s.s_class.proto  = rjs_value_stack_push(rt);
    s->s.s_class.constr = rjs_value_stack_push(rt);

    rjs_value_set_undefined(rt, s->s.s_class.proto);
    rjs_value_set_undefined(rt, s->s.s_class.constr);

#if ENABLE_PRIV_NAME
    s->s.s_class.priv_env = NULL;
#endif /*ENABLE_PRIV_NAME*/

    s->s.s_class.inst_field_num       = 0;
    s->s.s_class.inst_priv_method_num = 0;

    RJS_NEW(rt, s->s.s_class.elem_list);
    rjs_list_init(s->s.s_class.elem_list);

    return RJS_OK;
}

#if ENABLE_PRIV_NAME

/**
 * Set the class state's private environment.
 * \param rt The current runtime.
 * \param script The script.
 * \param pe The private environment.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_class_state_set_priv_env (RJS_Runtime *rt, RJS_Script *script, RJS_ScriptPrivEnv *pe)
{
    RJS_State *s = rjs_state_top(rt);

    assert(s->type == RJS_STATE_CLASS);

    s->s.s_class.priv_env = rjs_private_env_push(rt, script, pe);

    return RJS_OK;
}

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
RJS_Result
rjs_class_state_create_constructor (RJS_Runtime *rt, RJS_Value *cp, RJS_Value *proto,
        RJS_Script *script, RJS_ScriptFunc *sf, RJS_Value *constr)
{
    RJS_State *s = rjs_state_top_n(rt, 1);
    RJS_Result r;

    assert(s->type == RJS_STATE_CLASS);

    if ((r = rjs_create_constructor(rt, proto, cp, script, sf, constr)) == RJS_ERR)
        return r;

    rjs_value_copy(rt, s->s.s_class.proto, proto);
    rjs_value_copy(rt, s->s.s_class.constr, constr);

    return RJS_OK;
}

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
RJS_Result
rjs_class_state_create_default_constructor (RJS_Runtime *rt, RJS_Value *cp, RJS_Value *proto,
        RJS_Value *name, RJS_Bool derived, RJS_Value *constr)
{
    RJS_State *s = rjs_state_top_n(rt, 1);
    RJS_Result r;

    assert(s->type == RJS_STATE_CLASS);

    if ((r = rjs_create_default_constructor(rt, proto, cp, name, derived, constr)) == RJS_ERR)
        return r;

    rjs_value_copy(rt, s->s.s_class.proto, proto);
    rjs_value_copy(rt, s->s.s_class.constr, constr);

    return RJS_OK;
}

/**
 * Initialize the class on the top of the state stack.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_class_state_init (RJS_Runtime *rt)
{
    RJS_State             *s = rjs_state_top(rt);
    RJS_BaseFuncObject    *bfo;
    RJS_ScriptClass       *clazz;
    RJS_StateClassElement *se;
#if ENABLE_PRIV_NAME
    RJS_ScriptMethod      *sm;
#endif /*ENABLE_PRIV_NAME*/
    RJS_ScriptField       *sf;
    RJS_Result             r;

    assert(s->type == RJS_STATE_CLASS);

    bfo = (RJS_BaseFuncObject*)rjs_value_get_object(rt, s->s.s_class.constr);
    assert(!bfo->clazz);

    RJS_NEW(rt, clazz);

#if ENABLE_PRIV_NAME
    clazz->priv_method_num = s->s.s_class.inst_priv_method_num;
    if (clazz->priv_method_num)
        RJS_NEW_N(rt, clazz->priv_methods, clazz->priv_method_num);
    else
        clazz->priv_methods = NULL;
#endif /*ENABLE_PRIV_NAME*/

    clazz->field_num = s->s.s_class.inst_field_num;
    if (clazz->field_num)
        RJS_NEW_N(rt, clazz->fields, clazz->field_num);
    else
        clazz->fields = NULL;

#if ENABLE_PRIV_NAME
    sm = clazz->priv_methods;
#endif /*ENABLE_PRIV_NAME*/

    sf = clazz->fields;

    rjs_list_foreach_c(s->s.s_class.elem_list, se, RJS_StateClassElement, ln) {
        switch (se->type) {
#if ENABLE_PRIV_NAME
        case RJS_CLASS_ELEMENT_PRIV_GET:
            sm->type = RJS_SCRIPT_CLASS_ELEMENT_GET;
            rjs_value_copy(rt, &sm->name, &se->name);
            rjs_value_copy(rt, &sm->value, &se->value);
            sm ++;
            break;
        case RJS_CLASS_ELEMENT_PRIV_SET:
            sm->type = RJS_SCRIPT_CLASS_ELEMENT_SET;
            rjs_value_copy(rt, &sm->name, &se->name);
            rjs_value_copy(rt, &sm->value, &se->value);
            sm ++;
            break;
        case RJS_CLASS_ELEMENT_PRIV_METHOD:
            sm->type = RJS_SCRIPT_CLASS_ELEMENT_METHOD;
            rjs_value_copy(rt, &sm->name, &se->name);
            rjs_value_copy(rt, &sm->value, &se->value);
            sm ++;
            break;
        case RJS_CLASS_ELEMENT_STATIC_PRIV_GET:
            if ((r = rjs_private_accessor_add(rt, s->s.s_class.constr, &se->name, &se->value, NULL)) == RJS_ERR)
                goto end;
            break;
        case RJS_CLASS_ELEMENT_STATIC_PRIV_SET:
            if ((r = rjs_private_accessor_add(rt, s->s.s_class.constr, &se->name, NULL, &se->value)) == RJS_ERR)
                goto end;
            break;
        case RJS_CLASS_ELEMENT_STATIC_PRIV_METHOD:
            if ((r = rjs_private_method_add(rt, s->s.s_class.constr, &se->name, &se->value)) == RJS_ERR)
                goto end;
            break;
#endif /*ENABLE_PRIV_NAME*/
        case RJS_CLASS_ELEMENT_INST_FIELD:
            rjs_value_copy(rt, &sf->name, &se->name);
            rjs_value_copy(rt, &sf->init, &se->value);
            sf->is_af = se->is_af;
            sf ++;
            break;
        case RJS_CLASS_ELEMENT_STATIC_INIT:
            if ((r = rjs_call(rt, &se->value, s->s.s_class.constr, NULL, 0, NULL)) == RJS_ERR)
                goto end;
            break;
        case RJS_CLASS_ELEMENT_FIELD:
            if ((r = rjs_define_field(rt, s->s.s_class.constr, &se->name, &se->value, se->is_af)) == RJS_ERR)
                goto end;
            break;
        default:
            assert(0);
            break;
        }

        s = rjs_state_top(rt);
    }

    bfo->clazz = clazz;
    r = RJS_OK;
end:
    if (r == RJS_ERR) {
#if ENABLE_PRIV_NAME
        if (clazz->priv_methods)
            RJS_DEL_N(rt, clazz->priv_methods, clazz->priv_method_num);
#endif /*ENABLE_PRIV_NAME*/
        if (clazz->fields)
            RJS_DEL_N(rt, clazz->fields, clazz->field_num);
        RJS_DEL(rt, clazz);
    }

    return r;
}

/*Add an entry node to the class element list.*/
static RJS_Result
class_state_add_element (RJS_Runtime *rt, RJS_State *s, RJS_ClassElementType type, RJS_Value *name, RJS_Value *value)
{
    RJS_StateClassElement *ce;

    RJS_NEW(rt, ce);

    ce->type = type;

    rjs_value_copy(rt, &ce->name, name);
    rjs_value_copy(rt, &ce->value, value);
    ce->is_af = RJS_FALSE;

    rjs_list_append(s->s.s_class.elem_list, &ce->ln);

    return RJS_OK;
}

#if ENABLE_PRIV_NAME
/*Lookup the private identifier.*/
static RJS_Result
priv_name_lookup (RJS_Runtime *rt, RJS_PrivateEnv *env, RJS_Value *id, RJS_Value *name)
{
    RJS_Result r;

    r = rjs_private_name_lookup(rt, id, env, name);
    assert(r == RJS_OK);

    return r;
}
#endif /*ENABLE_PRIV_NAME*/

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
RJS_Result
rjs_class_state_add_element (RJS_Runtime *rt, RJS_ClassElementType type, RJS_Value *name,
        RJS_Script *script, RJS_ScriptFunc *sf)
{
    RJS_State       *s        = rjs_state_top_n(rt, 1);
    size_t           top      = rjs_value_stack_save(rt);
    RJS_Value       *value    = rjs_value_stack_push(rt);
    RJS_Value       *ename    = rjs_value_stack_push(rt);
    RJS_Environment *env      = rjs_lex_env_running(rt);
    RJS_PrivateEnv  *priv_env = NULL;
    RJS_Result       r;

    assert(s->type == RJS_STATE_CLASS);

#if ENABLE_PRIV_NAME
    priv_env = rjs_private_env_running(rt);
#endif /*ENABLE_PRIV_NAME*/

    switch (type) {
    case RJS_CLASS_ELEMENT_GET:
    case RJS_CLASS_ELEMENT_SET:
    case RJS_CLASS_ELEMENT_METHOD:
        r = rjs_create_method(rt, s->s.s_class.proto, type, name, script, sf, RJS_FALSE);
        break;
    case RJS_CLASS_ELEMENT_STATIC_GET:
    case RJS_CLASS_ELEMENT_STATIC_SET:
    case RJS_CLASS_ELEMENT_STATIC_METHOD:
        r = rjs_create_method(rt, s->s.s_class.constr, type, name, script, sf, RJS_FALSE);
        break;
    case RJS_CLASS_ELEMENT_FIELD:
        if (sf) {
            if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
                goto end;
            rjs_make_method(rt, value, s->s.s_class.constr);
        } else {
            rjs_value_set_undefined(rt, value);
        }
        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_FIELD, name, value);
        break;
    case RJS_CLASS_ELEMENT_INST_FIELD:
        if (sf) {
            if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
                goto end;
            rjs_make_method(rt, value, s->s.s_class.proto);
        } else {
            rjs_value_set_undefined(rt, value);
        }
        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_INST_FIELD, name, value);
        s->s.s_class.inst_field_num ++;
        break;
    case RJS_CLASS_ELEMENT_STATIC_INIT:
        if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
            goto end;
        rjs_make_method(rt, value, s->s.s_class.constr);
        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_STATIC_INIT, name, value);
        break;
#if ENABLE_PRIV_NAME
    case RJS_CLASS_ELEMENT_PRIV_GET:
        priv_name_lookup(rt, priv_env, name, ename);

        if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
            goto end;
        rjs_make_method(rt, value, s->s.s_class.proto);
        rjs_set_function_name(rt, value, ename, rjs_s_get(rt));

        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_PRIV_GET, ename, value);
        s->s.s_class.inst_priv_method_num ++;
        break;
    case RJS_CLASS_ELEMENT_PRIV_SET:
        priv_name_lookup(rt, priv_env, name, ename);

        if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
            goto end;
        rjs_make_method(rt, value, s->s.s_class.proto);
        rjs_set_function_name(rt, value, ename, rjs_s_set(rt));

        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_PRIV_SET, ename, value);
        s->s.s_class.inst_priv_method_num ++;
        break;
    case RJS_CLASS_ELEMENT_PRIV_METHOD:
        priv_name_lookup(rt, priv_env, name, ename);

        if ((r = rjs_define_method(rt, s->s.s_class.proto, NULL, script, sf, value)) == RJS_ERR)
            goto end;

        rjs_set_function_name(rt, value, ename, NULL);

        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_PRIV_METHOD, ename, value);

        s->s.s_class.inst_priv_method_num ++;
        break;
    case RJS_CLASS_ELEMENT_PRIV_FIELD:
        priv_name_lookup(rt, priv_env, name, ename);

        if (sf) {
            if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
                goto end;
            rjs_make_method(rt, value, s->s.s_class.constr);
        } else {
            rjs_value_set_undefined(rt, value);
        }
        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_FIELD, ename, value);
        break;
    case RJS_CLASS_ELEMENT_PRIV_INST_FIELD:
        priv_name_lookup(rt, priv_env, name, ename);

        if (sf) {
            if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
                goto end;
            rjs_make_method(rt, value, s->s.s_class.proto);
        } else {
            rjs_value_set_undefined(rt, value);
        }
        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_INST_FIELD, ename, value);
        s->s.s_class.inst_field_num ++;
        break;
    case RJS_CLASS_ELEMENT_STATIC_PRIV_GET:
        priv_name_lookup(rt, priv_env, name, ename);

        if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
            goto end;
        rjs_make_method(rt, value, s->s.s_class.constr);
        rjs_set_function_name(rt, value, ename, rjs_s_get(rt));

        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_STATIC_PRIV_GET, ename, value);
        break;
    case RJS_CLASS_ELEMENT_STATIC_PRIV_SET:
        priv_name_lookup(rt, priv_env, name, ename);

        if ((r = rjs_create_function(rt, script, sf, env, priv_env, RJS_FALSE, value)) == RJS_ERR)
            goto end;
        rjs_make_method(rt, value, s->s.s_class.constr);
        rjs_set_function_name(rt, value, ename, rjs_s_set(rt));

        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_STATIC_PRIV_SET, ename, value);
        break;
    case RJS_CLASS_ELEMENT_STATIC_PRIV_METHOD:
        priv_name_lookup(rt, priv_env, name, ename);

        if ((r = rjs_define_method(rt, s->s.s_class.constr, NULL, script, sf, value)) == RJS_ERR)
            goto end;

        rjs_set_function_name(rt, value, name, NULL);

        r = class_state_add_element(rt, s, RJS_CLASS_ELEMENT_STATIC_PRIV_METHOD, ename, value);
        break;
#endif /*ENABLE_PRIV_NAME*/
    default:
        assert(0);
        r = RJS_ERR;
        break;
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Set the last class element as anonymous function field.
 * \param rt The current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_class_state_set_anonymous_function_field (RJS_Runtime *rt)
{
    RJS_State             *s = rjs_state_top_n(rt, 1);
    RJS_StateClassElement *ce;

    assert(s->type == RJS_STATE_CLASS);
    assert(!rjs_list_is_empty(s->s.s_class.elem_list));

    ce = RJS_CONTAINER_OF(s->s.s_class.elem_list->prev, RJS_StateClassElement, ln);

    assert((ce->type == RJS_CLASS_ELEMENT_FIELD) || (ce->type == RJS_CLASS_ELEMENT_INST_FIELD));

    ce->is_af = RJS_TRUE;

    return RJS_OK;
}

/**
 * Push a new call state to the stack.
 * \param rt The current runtime.
 * \param func The function to be called.
 * \param thiz This argument.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_call_state_push (RJS_Runtime *rt, RJS_Value *func, RJS_Value *thiz)
{
    RJS_State *s;

    s = state_push(rt, RJS_STATE_CALL);

    s->s.s_call.func = rjs_value_stack_push(rt);
    s->s.s_call.thiz = rjs_value_stack_push(rt);
    s->s.s_call.args = rjs_stack_pointer_to_value(rt->rb.curr_native_stack->value.item_num);
    s->s.s_call.argc = 0;

    rjs_value_copy(rt, s->s.s_call.func, func);
    rjs_value_copy(rt, s->s.s_call.thiz, thiz);

    return RJS_OK;
}

/**
 * Push a new call state to the stack for super call.
 * \param rt the current runtime.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_super_call_state_push (RJS_Runtime *rt)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_State *s;
    RJS_Result r;

    s = state_push(rt, RJS_STATE_CALL);

    s->s.s_call.func = rjs_value_stack_push(rt);
    s->s.s_call.thiz = rjs_value_stack_push(rt);
    s->s.s_call.args = rjs_stack_pointer_to_value(rt->rb.curr_native_stack->value.item_num);
    s->s.s_call.argc = 0;

    top = rjs_value_stack_save(rt);

    if ((r = rjs_get_new_target(rt, s->s.s_call.thiz)) == RJS_ERR)
        goto end;

    if ((r = rjs_get_super_constructor(rt, s->s.s_call.func)) == RJS_ERR)
        goto end;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    if (r == RJS_ERR)
        rjs_state_pop(rt);

    return r;
}

/**
 * Push a new call state to the stack for construct.
 * \param rt The current runtime.
 * \param c The constructor.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_new_state_push (RJS_Runtime *rt, RJS_Value *c)
{
    return rjs_call_state_push(rt, c, rjs_v_undefined(rt));
}

/**
 * Push an argument to the stack.
 * \param rt The current runtime.
 * \param arg The argument to be pushed.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_call_state_push_arg (RJS_Runtime *rt, RJS_Value *arg)
{
    RJS_State *s = rjs_state_top(rt);
    size_t     sp;
    RJS_Value *v;

    assert(s->type == RJS_STATE_CALL);

    sp = rjs_value_to_stack_pointer(s->s.s_call.args);
    sp += s->s.s_call.argc;

    rt->rb.curr_native_stack->value.item_num = sp;

    v = rjs_value_stack_push(rt);

    rjs_value_copy(rt, v, arg);

    s->s.s_call.argc ++;

    return RJS_OK;
}

/**
 * Push spread arguments to the stack.
 * \param rt The current runtime.
 * \param args The arguments.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_call_state_push_spread_args (RJS_Runtime *rt, RJS_Value *args)
{
    RJS_State   *s   = rjs_state_top(rt);
    size_t       add = 0;
    size_t       last_sp, base_sp, sp;
    RJS_Iterator iter;
    RJS_Value   *result;
    RJS_Value   *v;
    RJS_Result   r;

    assert(s->type == RJS_STATE_CALL);

    rjs_iterator_init(rt, &iter);
    result = rjs_value_stack_push(rt);

    if ((r = rjs_get_iterator(rt, args, RJS_ITERATOR_SYNC, NULL, &iter)) == RJS_ERR)
        goto end;

    base_sp = rt->rb.curr_native_stack->value.item_num;
    sp      = base_sp;

    while (1) {
        if ((r = rjs_iterator_step(rt, &iter, result)) == RJS_ERR)
            goto end;
        if (!r)
            break;

        rt->rb.curr_native_stack->value.item_num = sp;

        v = rjs_value_stack_push(rt);

        if ((r = rjs_iterator_value(rt, result, v)) == RJS_ERR)
            goto end;

        sp ++;
        add ++;
    }

    s = rjs_state_top(rt);

    last_sp = rjs_value_to_stack_pointer(s->s.s_call.args) + s->s.s_call.argc;

    if (add) {
        RJS_ELEM_MOVE(rt->rb.curr_native_stack->value.items + last_sp,
                rt->rb.curr_native_stack->value.items + base_sp,
                add);

        s->s.s_call.argc += add;
    }

    rt->rb.curr_native_stack->value.item_num = last_sp + add;

    r = RJS_OK;
end:
    return r;
}

#if ENABLE_EVAL

/*Direct eval call.*/
static RJS_Result
direct_eval (RJS_Runtime *rt, RJS_Value *args, size_t argc, RJS_Value *rv)
{
    RJS_Value *arg    = rjs_argument_get(rt, args, argc, 0);
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *script = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_eval_from_string(rt, script, arg, NULL, RJS_FALSE, RJS_TRUE)) == RJS_OK) {
        r = rjs_eval_evaluation(rt, script, RJS_TRUE, rv);
    } else if (r == RJS_FALSE) {
        r = RJS_OK;
        rjs_value_copy(rt, rv, arg);
    }

    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_EVAL*/

/**
 * Call the function use the top call state in the stack.
 * \param rt The current runtime.
 * \param sp The native stack pointer.
 * \param flags Call flags.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_call_state_call (RJS_Runtime *rt, size_t sp, int flags, RJS_Value *rv)
{
    RJS_State   *s     = rjs_state_top(rt);
    RJS_Context *ctxt  = rjs_context_running(rt);
    RJS_Realm   *realm = rjs_realm_current(rt);
    RJS_Result   r;

    assert(s->type == RJS_STATE_CALL);

    if ((flags & RJS_CALL_FL_TCO)
            && rjs_same_value(rt, &ctxt->function, s->s.s_call.func)) {
        /*Tail call optimize.*/
        RJS_ScriptContext *sc = (RJS_ScriptContext*)ctxt;
        RJS_Value         *args;
        size_t             argc;

        /*Replace the arguments.*/
        if (s->s.s_call.argc) {
            argc = s->s.s_call.argc;
            RJS_NEW_N(rt, args, argc);

            rjs_value_buffer_copy(rt, args, s->s.s_call.args, argc);
        } else {
            args = NULL;
            argc = 0;
        }

        if (sc->args)
            RJS_DEL_N(rt, sc->args, sc->argc);

        sc->args = args;
        sc->argc = argc;

        /*Bind this argument.*/
        rt->env         = sc->scb.lex_env;
        sc->scb.lex_env = sc->scb.var_env;
        rjs_decl_env_clear(rt, sc->scb.var_env);
        rjs_ordinary_call_bind_this(rt, &ctxt->function, s->s.s_call.thiz);
        sc->scb.lex_env = rt->env;

        /*Pop up the states.*/
        while (sp < rt->rb.curr_native_stack->state.item_num) {
            if ((r = rjs_state_pop(rt)) == RJS_ERR)
                return r;
        }

        r = RJS_FALSE;
#if ENABLE_EVAL
    } else if ((flags & RJS_CALL_FL_EVAL)
            && rjs_same_value(rt, s->s.s_call.func, rjs_o_eval(realm))) {
        r = direct_eval(rt, s->s.s_call.args, s->s.s_call.argc, rv);
        
        rjs_state_pop(rt);
#endif /*ENABLE_EVAL*/
    } else {
        r = rjs_call(rt, s->s.s_call.func, s->s.s_call.thiz, s->s.s_call.args, s->s.s_call.argc, rv);
        
        rjs_state_pop(rt);
    }

    return r;
}

/**
 * Call the super constructor use the top call state in the stack.
 * \param rt The current runtime.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_call_state_super_call (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_State       *s = rjs_state_top(rt);
    RJS_Environment *env;
    RJS_FunctionEnv *fe;
    RJS_Result       r;

    assert(s->type == RJS_STATE_CALL);

    if (!rjs_is_constructor(rt, s->s.s_call.func))
        return rjs_throw_type_error(rt, _("the value is not a constructor"));

    if ((r = rjs_construct(rt, s->s.s_call.func, s->s.s_call.args, s->s.s_call.argc, s->s.s_call.thiz, rv)) == RJS_ERR)
        return r;

    rjs_state_pop(rt);

    env = rjs_get_this_environment(rt);

    if ((r = rjs_env_bind_this_value(rt, env, rv)) == RJS_ERR)
        return r;

    fe = (RJS_FunctionEnv*)env;

    r = rjs_initialize_instance_elements(rt, rv, &fe->function);

    return r;
}

/**
 * Construct the new object use the top call state in the stack.
 * \param rt The current runtime.
 * \param[out] rv The return value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_call_state_new (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_State *s = rjs_state_top(rt);
    RJS_Result r;

    assert(s->type == RJS_STATE_CALL);

    if (!rjs_is_constructor(rt, s->s.s_call.func))
        return rjs_throw_type_error(rt, _("the value is not a constructor"));

    r = rjs_construct(rt, s->s.s_call.func, s->s.s_call.args, s->s.s_call.argc, NULL, rv);

    rjs_state_pop(rt);

    return r;
}

/**
 * Push a new array state in the stack,
 * \param rt The current runtime.
 * \param array The array value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_array_state_push (RJS_Runtime *rt, RJS_Value *array)
{
    RJS_State *s;
    RJS_Result r;

    s = state_push(rt, RJS_STATE_ARRAY);

    s->s.s_array.index = 0;
    s->s.s_array.array = rjs_value_stack_push(rt);

    if ((r = rjs_array_new(rt, s->s.s_array.array, 0, NULL)) == RJS_ERR) {
        rjs_state_pop(rt);
        return r;
    }

    rjs_value_copy(rt, array, s->s.s_array.array);
    return RJS_OK;
}

/**
 * Add an element to the array state.
 * \param rt The current runtime.
 * \param v The element value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_array_state_add (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_State  *s = rjs_state_top(rt);

    assert(s->type == RJS_STATE_ARRAY);

    rjs_create_data_property_or_throw_index(rt, s->s.s_array.array,
            s->s.s_array.index, v);

    s->s.s_array.index ++;

    return RJS_OK;
}

/**
 * Add elements to the array state.
 * \param rt The current runtime.
 * \param v The value, all the elements in it will be added to the array.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_array_state_spread (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_State       *s      = rjs_state_top(rt);
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *result = rjs_value_stack_push(rt);
    RJS_Value       *iv     = rjs_value_stack_push(rt);
    RJS_Iterator     iter;
    RJS_Result       r;

    assert(s->type == RJS_STATE_ARRAY);

    rjs_iterator_init(rt, &iter);

    if ((r = rjs_get_iterator(rt, v, RJS_ITERATOR_SYNC, NULL, &iter)) == RJS_ERR)
        goto end;

    while (1) {
        if ((r = rjs_iterator_step(rt, &iter, result)) == RJS_ERR)
            goto end;

        if (!r)
            break;

        if ((r = rjs_iterator_value(rt, result, iv)) == RJS_ERR)
            goto end;

        s = rjs_state_top(rt);

        rjs_create_data_property_or_throw_index(rt, s->s.s_array.array,
                s->s.s_array.index, iv);

        s->s.s_array.index ++;
    }

    r = RJS_OK;
end:
    rjs_iterator_deinit(rt, &iter);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Push a new object state in the stack,
 * \param rt The current runtime.
 * \param o The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_state_push (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_State *s;
    RJS_Result r;

    s = state_push(rt, RJS_STATE_OBJECT);

    s->s.s_object.object = rjs_value_stack_push(rt);

    if ((r = rjs_ordinary_object_create(rt, NULL, s->s.s_object.object)) == RJS_ERR) {
        rjs_state_pop(rt);
        return r;
    }

    rjs_value_copy(rt, o, s->s.s_object.object);
    return RJS_OK;
}

/**
 * Add a property to the object state in the stack.
 * \param rt The current runtime.
 * \param name The property name.
 * \param value The property value.
 * \param is_af The property value is an anonymous function.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_state_add (RJS_Runtime *rt, RJS_Value *name, RJS_Value *value, RJS_Bool is_af)
{
    RJS_State       *s   = rjs_state_top(rt);
    size_t           top = rjs_value_stack_save(rt);
    RJS_Value       *key = rjs_value_stack_push(rt);
    RJS_PropertyName pn;
    RJS_Result       r;

    assert(s->type == RJS_STATE_OBJECT);

    if (is_af)
        rjs_set_function_name(rt, value, name, NULL);

    if ((r = rjs_to_property_key(rt, name, key)) == RJS_ERR)
        goto end;

    rjs_property_name_init(rt, &pn, key);
    rjs_create_data_property_or_throw(rt, s->s.s_object.object, &pn, value);
    rjs_property_name_deinit(rt, &pn);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create a object and copy the properties from the original object.*/
static RJS_Result
copy_data_properties (RJS_Runtime *rt, RJS_Value *target, RJS_Value *src, RJS_State *s)
{
    size_t               top  = rjs_value_stack_save(rt);
    RJS_Value           *o    = rjs_value_stack_push(rt);
    RJS_Value           *keys = rjs_value_stack_push(rt);
    RJS_Value           *pv   = rjs_value_stack_push(rt);
    RJS_PropertyDesc     pd;
    RJS_PropertyKeyList *pkl;
    size_t               i;
    RJS_Result           r;

    rjs_property_desc_init(rt, &pd);

    if (rjs_value_is_undefined(rt, src) || rjs_value_is_null(rt, src)) {
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_to_object(rt, src, o)) == RJS_ERR)
        goto end;

    if ((r = rjs_object_own_property_keys(rt, o, keys)) == RJS_ERR)
        goto end;

    pkl = rjs_value_get_gc_thing(rt, keys);

    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_HashEntry    *he;
        RJS_PropertyName  pn;
        RJS_Value        *k = &pkl->keys.items[i];

        if (s) {
            RJS_Hash *hash;

            s    = rjs_state_top(rt);
            hash = &s->s.s_object_assi.prop_hash;

            r = rjs_hash_lookup(hash, k, &he, NULL, &rjs_hash_value_ops, rt);
            if (r)
                continue;
        }

        rjs_property_name_init(rt, &pn, k);
        r = rjs_object_get_own_property(rt, o, &pn, &pd);
        if (r == RJS_OK) {
            if (pd.flags & RJS_PROP_FL_ENUMERABLE) {
                r = rjs_get(rt, o, &pn, pv);
                if (r == RJS_OK)
                    rjs_create_data_property_or_throw(rt, target, &pn, pv);
            }
        }
        rjs_property_name_deinit(rt, &pn);

        if (r == RJS_ERR)
            goto end;
    }

    r = RJS_OK;
end:
    rjs_property_desc_deinit(rt, &pd);
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Add properties to the object state in the stack.
 * \param rt The current runtime.
 * \param value The value. All the properties will be added to the object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_state_spread (RJS_Runtime *rt, RJS_Value *value)
{
    RJS_State *s = rjs_state_top(rt);

    assert(s->type == RJS_STATE_OBJECT);

    return copy_data_properties(rt, s->s.s_object.object, value, NULL);
}

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
RJS_Result
rjs_object_state_add_element (RJS_Runtime *rt, RJS_ClassElementType type, RJS_Value *name,
        RJS_Script *script, RJS_ScriptFunc *sf)
{
    RJS_State *s = rjs_state_top(rt);

    assert(s->type == RJS_STATE_OBJECT);

    return rjs_create_method(rt, s->s.s_object.object, type, name, script, sf, RJS_TRUE);
}

/**
 * Push a new object assignment state in the stack,
 * \param rt The current runtime.
 * \param o The object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_assi_state_push (RJS_Runtime *rt, RJS_Value *o)
{
    RJS_State *s;
    RJS_Result r;

    if ((r = rjs_require_object_coercible(rt, o)) == RJS_ERR)
        return r;

    s = state_push(rt, RJS_STATE_OBJECT_ASSI);

    s->s.s_object_assi.object = rjs_value_stack_push(rt);

    rjs_value_copy(rt, s->s.s_object_assi.object, o);
    rjs_hash_init(&s->s.s_object_assi.prop_hash);

    return RJS_OK;
}

/**
 * Get a property value from the object assignment state in the stack.
 * \param rt The current runtime.
 * \param pn The property name.
 * \param[out] rv Return the property value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_object_assi_state_step (RJS_Runtime *rt, RJS_PropertyName *pn, RJS_Value *rv)
{
    RJS_State           *s   = rjs_state_top(rt);
    size_t               top = rjs_value_stack_save(rt);
    RJS_Value           *n   = rjs_value_stack_push(rt);
    RJS_HashEntry       *he, **phe;
    RJS_StringPropEntry *spe;
    RJS_Result           r;

    assert(s->type == RJS_STATE_OBJECT_ASSI);

    if ((r = rjs_get_v(rt, s->s.s_object_assi.object, pn, rv)) == RJS_ERR)
        goto end;

    rjs_value_copy(rt, n, pn->name);

    s = rjs_state_top(rt);

    r = rjs_hash_lookup(&s->s.s_object_assi.prop_hash, n, &he, &phe, &rjs_hash_value_ops, rt);
    if (!r) {
        RJS_NEW(rt, spe);

        rjs_value_copy(rt, &spe->value, n);
        rjs_hash_insert(&s->s.s_object_assi.prop_hash, &spe->value, &spe->he, phe, &rjs_hash_value_ops, rt);
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/**
 * Get an object from the rest properties of the object assignment state in the stack.
 * \param rt The current runtime.
 * \param[out] rv Return the object value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_object_assi_state_rest (RJS_Runtime *rt, RJS_Value *rv)
{
    RJS_State *s = rjs_state_top(rt);
    RJS_Result r;

    assert(s->type == RJS_STATE_OBJECT_ASSI);

    if ((r = rjs_ordinary_object_create(rt, NULL, rv)) == RJS_ERR)
        return r;

    return copy_data_properties(rt, rv, s->s.s_object_assi.object, s);
}

/**
 * Push a new try state in the stack,
 * \param rt The current runtime.
 * \param catch_ip The catch insrtruction pointer.
 * \param finally_ip The finally instruction pointer.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_try_state_push (RJS_Runtime *rt, size_t catch_ip, size_t finally_ip)
{
    RJS_State *s;

    s = state_push(rt, RJS_STATE_TRY);

    s->s.s_try.next_op    = RJS_TRY_NEXT_OP_NORMAL;
    s->s.s_try.catch_ip   = catch_ip;
    s->s.s_try.finally_ip = finally_ip;
    s->s.s_try.next_ip    = (size_t)-1;
    s->s.s_try.state      = RJS_TRY_STATE_TRY;
    s->s.s_try.error      = rjs_value_stack_push(rt);

    rjs_value_set_undefined(rt, s->s.s_try.error);

    return RJS_OK;
}

/**
 * Allocate values buffer in the value stack.
 * \param rt The current runtime.
 * \param n Number of values in the buffer.
 * \return The first value's reference in the stack.
 */
RJS_Value*
rjs_value_stack_append (RJS_Runtime *rt, size_t n)
{
    size_t id = rt->rb.curr_native_stack->value.item_num;

    rjs_vector_resize_init(&rt->rb.curr_native_stack->value,
            rt->rb.curr_native_stack->value.item_num + n,
            rt,
            rjs_value_buffer_fill_undefined);

    return RJS_SIZE2PTR((id << 1) | 1);
}

/**
 * Scan the referenced things in the native stack.
 * \param rt The current runtime.
 * \param ns The native stack to be scanned.
 */
void
rjs_gc_scan_native_stack (RJS_Runtime *rt, RJS_NativeStack *ns)
{
    RJS_State *s;
    size_t     i;

    rjs_gc_scan_value_buffer(rt, ns->value.items, ns->value.item_num);

    rjs_vector_foreach(&ns->state, i, s) {
        state_scan(rt, s);
    }
}

/**
 * Release the native stack.
 * \param rt The current runtime.
 * \param ns The native stack to be released.
 */
void
rjs_native_stack_deinit (RJS_Runtime *rt, RJS_NativeStack *ns)
{
    rjs_vector_deinit(&ns->value, rt);
    rjs_vector_deinit(&ns->state, rt);
}

/**
 * Clear the resouce in the native stack.
 * \param rt The current runtime.
 * \param ns The native stack to be cleared.
 */
void
rjs_native_stack_clear (RJS_Runtime *rt, RJS_NativeStack *ns)
{
    size_t           i;
    RJS_State       *s;
    RJS_NativeStack *curr_ns = rt->rb.curr_native_stack;

    rt->rb.curr_native_stack = ns;

    rjs_vector_foreach(&ns->state, i, s) {
        /*Set the iterator done flag to avoid "close" be called*/
        if ((s->type == RJS_STATE_FOR_IN)
                || (s->type == RJS_STATE_FOR_OF)
                || (s->type == RJS_STATE_ARRAY_ASSI)) {
            if (s->s.s_iter.iterator)
                s->s.s_iter.iterator->done = RJS_TRUE;
        }

        rjs_state_deinit(rt, s, NULL, 0, NULL);
    }

    rt->rb.curr_native_stack = curr_ns;
}
