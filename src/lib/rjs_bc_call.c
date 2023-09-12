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

#if ENABLE_ASYNC

/*Resume from "await" command.*/
static RJS_Result
await_command (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    ASYNC_OP_DEBUG();

    if (type == RJS_SCRIPT_CALL_ASYNC_REJECT)
        return rjs_throw(rt, iv);

    return RJS_OK;
}

/*Resume from pop async iterator state.*/
static RJS_Result
await_pop_async_iter_state (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    RJS_AsyncContext *ac = (RJS_AsyncContext*)rjs_context_running(rt);
    RJS_Result        r;

    ASYNC_OP_DEBUG();

    if ((r = rjs_await_async_iterator_close(rt, type, iv, rv)) == RJS_ERR)
        return r;

    r = ac->i0;
    if (r == RJS_ERR)
        rjs_throw(rt, &ac->v0);

    return r;
}

#else /*!define ENABLE_ASYNC*/

#define await_command NULL
#define await_pop_async_iter_state NULL
#define rjs_await_async_iterator_close NULL

#endif /*ENABLE_ASYNC*/

#define bc_script_init(decl, var_table, lex_table, func_table)\
    if ((r = rjs_global_declaration_instantiation(rt, script, decl, var_table, lex_table, func_table)) == RJS_ERR)\
        goto error;

#define bc_eval_init(decl, var_table, lex_table, func_table)\
    if ((r = rjs_eval_declaration_instantiation(rt, script, decl, var_table, lex_table, func_table, strict)) == RJS_ERR)\
        goto error;

#define bc_set_decl(decl)\
    RJS_Environment *env = sc->scb.lex_env;\
    env->script_decl = decl;

#define bc_set_var_env(decl)\
    rjs_decl_env_new(rt, &rt->env, decl, sc->scb.lex_env);\
    sc->scb.var_env = rt->env;\
    sc->scb.lex_env = rt->env;

#define bc_push_lex_env(decl)\
    rjs_decl_env_new(rt, &rt->env, decl, sc->scb.lex_env);\
    if ((r = rjs_lex_env_state_push(rt, rt->env)) == RJS_ERR)\
        goto error;

#define bc_save_lex_env(dest)\
    RJS_Environment *env = sc->scb.lex_env;\
    rjs_value_set_gc_thing(rt, dest, env);\
    rjs_state_pop(rt);

#define bc_top_lex_env(dest)\
    RJS_Environment *env = sc->scb.lex_env;\
    rjs_value_set_gc_thing(rt, dest, env);

#define bc_restore_lex_env(value)\
    RJS_Environment *env;\
    env = rjs_value_get_gc_thing(rt, value);\
    if ((r = rjs_lex_env_state_push(rt, env)) == RJS_ERR)\
        goto error;

#define bc_next_lex_env(lex_table)\
    RJS_Environment *env = sc->scb.lex_env;\
    rjs_decl_env_new(rt, &rt->env, env->script_decl, env->outer);\
    if ((r = rjs_script_binding_group_dup(rt, script, lex_table, rt->env, env)) == RJS_ERR)\
        goto error;\
    sc->scb.lex_env = rt->env;

#define bc_binding_table_init(table)\
    if ((r = rjs_script_binding_group_init(rt, script, table)) == RJS_ERR)\
        goto error;

#define bc_top_func_table_init(table)\
    if ((r = rjs_script_func_group_init(rt, script, table, RJS_TRUE)) == RJS_ERR)\
        goto error;

#define bc_func_table_init(table)\
    if ((r = rjs_script_func_group_init(rt, script, table, RJS_FALSE)) == RJS_ERR)\
        goto error;

#define bc_push_enum(value)\
    if ((r = rjs_enum_state_push(rt, value)) == RJS_ERR)\
        goto error;

#define bc_push_iter(value)\
    if ((r = rjs_iter_state_push(rt, value, RJS_ITERATOR_SYNC)) == RJS_ERR)\
        goto error;

#define bc_push_async_iter(value)\
    if ((r = rjs_iter_state_push(rt, value, RJS_ITERATOR_ASYNC)) == RJS_ERR)\
        goto error;

#define bc_for_step(value, done)\
    if ((r = rjs_iter_state_step(rt, value)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, done, r);

#define bc_async_for_step()\
    if ((r = rjs_iter_state_async_step(rt)) == RJS_ERR)\
        goto error;\
    r = RJS_SUSPEND;\
    sc->ip += ip_size;\
    goto end;

#define bc_async_for_step_resume(value, done)\
    if ((r = rjs_iter_state_async_step_resume(rt, iv, value)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, done, r);

#define bc_unmapped_args()\
    size_t           top  = rjs_value_stack_save(rt);\
    RJS_Value       *argv = rjs_value_stack_push(rt);\
    RJS_Environment *env  = sc->scb.lex_env;\
    rjs_unmapped_arguments_object_new(rt, argv, sc->args, sc->argc);\
    rjs_env_add_arguments_object(rt, env, argv, strict);\
    rjs_value_stack_restore(rt, top);

#define bc_mapped_args(map)\
    size_t           top  = rjs_value_stack_save(rt);\
    RJS_Value       *argv = rjs_value_stack_push(rt);\
    RJS_Environment *env  = sc->scb.lex_env;\
    rjs_mapped_arguments_object_new(rt, argv, &ctxt->function, map, sc->args, sc->argc, env);\
    rjs_env_add_arguments_object(rt, env, argv, strict);\
    rjs_value_stack_restore(rt, top);

#define bc_load_undefined(dest)\
    rjs_value_set_undefined(rt, dest);

#define bc_load_null(dest)\
    rjs_value_set_null(rt, dest);

#define bc_load_true(dest)\
    rjs_value_set_boolean(rt, dest, RJS_TRUE);

#define bc_load_false(dest)\
    rjs_value_set_boolean(rt, dest, RJS_FALSE);

#define bc_load_this(dest)\
    if ((r = rjs_resolve_this_binding(rt, dest)) == RJS_ERR)\
        goto error;

#define bc_load_with_base(env, dest)\
    RJS_Environment *e = rjs_value_get_gc_thing(rt, env);\
    if ((r = rjs_env_with_base_object(rt, e, dest)) == RJS_ERR)\
        goto error;

#define bc_load_object_proto(dest)\
    RJS_Realm *realm = rjs_realm_current(rt);\
    rjs_value_copy(rt, dest, rjs_o_Object_prototype(realm));

#define bc_load_func_proto(dest)\
    RJS_Realm *realm = rjs_realm_current(rt);\
    rjs_value_copy(rt, dest, rjs_o_Function_prototype(realm));

#define bc_load_import_meta(dest)\
    size_t     top = rjs_value_stack_save(rt);\
    RJS_Value *mod = rjs_value_stack_push(rt);\
    rjs_value_set_gc_thing(rt, mod, script);\
    rjs_module_import_meta(rt, mod, dest);\
    rjs_value_stack_restore(rt, top);

#define bc_load_new_target(dest)\
    rjs_get_new_target(rt, dest);

#define bc_load_super_constr(dest)\
    rjs_get_super_constructor(rt, dest);

#define bc_load_1(dest)\
    rjs_value_set_number(rt, dest, 1);

#define bc_load_0(dest)\
    rjs_value_set_number(rt, dest, 0);

#define bc_load_arg(id, dest)\
    if (id < sc->argc) {\
        RJS_Value *arg = rjs_value_buffer_item(rt, sc->args, id);\
        rjs_value_copy(rt, dest, arg);\
    } else {\
        rjs_value_set_undefined(rt, dest);\
    }

#define bc_load_rest_args(id, dest)\
    RJS_Value *al;\
    size_t     n;\
    if (id >= sc->argc) {\
        al = NULL;\
        n  = 0;\
    } else {\
        al = rjs_value_buffer_item(rt, sc->args, id);\
        n  = sc->argc - id;\
    }\
    if ((r = rjs_create_array_from_value_buffer(rt, al, n, dest)) == RJS_ERR)\
        goto error;

#define bc_load_value(value, dest)\
    rjs_value_copy(rt, dest, value);

#define bc_load_regexp(value, dest)\
    rjs_regexp_clone(rt, dest, value);

#define bc_is_undefined(operand, result)\
    RJS_Bool b;\
    b = rjs_value_is_undefined(rt, operand);\
    rjs_value_set_boolean(rt, result, b);

#define bc_is_undefined_null(operand, result)\
    RJS_Bool b;\
    b = rjs_value_is_undefined(rt, operand) || rjs_value_is_null(rt, operand);\
    rjs_value_set_boolean(rt, result, b);

#define bc_to_number(operand, result)\
    RJS_Number n;\
    if ((r = rjs_to_number(rt, operand, &n)) == RJS_ERR)\
        goto error;\
    rjs_value_set_number(rt, result, n);

#define bc_to_numeric(operand, result)\
    if ((r = rjs_to_numeric(rt, operand, result)) == RJS_ERR)\
        goto error;

#define bc_to_prop(operand, result)\
    if ((r = rjs_to_property_key(rt, operand, result)) == RJS_ERR)\
        goto error;

#define bc_require_object(value)\
    if ((r = rjs_require_object_coercible(rt, value)) == RJS_ERR)\
        goto error;

/*Unary minus.*/
static inline RJS_Result
do_bc_negative (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_numeric(rt, v, tmp)) == RJS_ERR)
        goto end;

#if ENABLE_BIG_INT
    if (rjs_value_get_type(rt, tmp) == RJS_VALUE_BIG_INT)
        rjs_big_int_unary_minus(rt, tmp, rv);
    else
#endif /*ENABLE_BIG_INT*/
        rjs_number_unary_minus(rt, tmp, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#define bc_negative(operand, result)\
    if ((r = do_bc_negative(rt, operand, result)) == RJS_ERR)\
        goto error;

/*Bitwise not.*/
static inline RJS_Result
do_bc_reverse (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *tmp = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_numeric(rt, v, tmp)) == RJS_ERR)
        goto end;

#if ENABLE_BIG_INT
    if (rjs_value_get_type(rt, tmp) == RJS_VALUE_BIG_INT)
        rjs_big_int_bitwise_not(rt, tmp, rv);
    else
#endif /*ENABLE_BIG_INT*/
        rjs_number_bitwise_not(rt, tmp, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#define bc_reverse(operand, result)\
    if ((r = do_bc_reverse(rt, operand, result)) == RJS_ERR)\
        goto error;

#define bc_typeof(operand, result)\
    rjs_type_of(rt, operand, result);

#define bc_typeof_binding(binding, dest)\
    RJS_Environment *e;\
    if ((r = rjs_resolve_binding(rt, &binding->binding_name, &e)) == RJS_ERR)\
        goto error;\
    if (!r) {\
        rjs_value_copy(rt, dest, rjs_s_undefined(rt));\
    } else {\
        size_t     top = rjs_value_stack_save(rt);\
        RJS_Value *tmp = rjs_value_stack_push(rt);\
        r = rjs_get_binding_value(rt, e, &binding->binding_name, strict, tmp);\
        if (r == RJS_OK)\
            rjs_type_of(rt, tmp, dest);\
        rjs_value_stack_restore(rt, top);\
        if (r == RJS_ERR)\
            goto error;\
    }

#define bc_not(operand, result)\
    RJS_Bool b;\
    b = rjs_to_boolean(rt, operand);\
    rjs_value_set_boolean(rt, result, !b);

#define bc_yield(value)\
    if ((r = rjs_yield(rt, value, rv)) == RJS_ERR)\
        goto error;\
    sc->ip += ip_size;\
    goto end;

#define bc_yield_resume(result)\
    r = rjs_yield_resume(rt, result, rv);\
    if (r == RJS_THROW)\
        goto error;\
    if (r == RJS_RETURN) {\
        rjs_value_copy(rt, &sc->retv, rv);\
        goto ret;\
    }\
    if (r == RJS_SUSPEND) {\
        sc->ip += ip_size;\
        goto end;\
    }

#define bc_yield_iter_start(value)\
    if ((r = rjs_iterator_yield_start(rt, value)) == RJS_ERR)\
        goto error;

#define bc_yield_iter_next(result)\
    if ((r = rjs_iterator_yield_next(rt, result)) == RJS_THROW)\
        goto error;\
    if (r == RJS_SUSPEND) {\
        rjs_value_copy(rt, rv, result);\
        goto end;\
    }\
    if (r == RJS_RETURN) {\
        rjs_value_copy(rt, &sc->retv, result);\
        goto ret;\
    }

#define bc_await(value)\
    if ((r = rjs_await(rt, value, await_command, 0, NULL)) == RJS_ERR)\
        goto error;\
    sc->ip += ip_size;\
    goto end;

#define bc_await_resume(result)\
    rjs_value_copy(rt, result, iv);

#define bc_import(operand, result)\
    size_t     top = rjs_value_stack_save(rt);\
    RJS_Value *sv  = rjs_value_stack_push(rt);\
    rjs_value_set_gc_thing(rt, sv, script);\
    r = rjs_module_import_dynamically(rt, sv, operand, result);\
    rjs_value_stack_restore(rt, top);\
    if (r == RJS_ERR)\
        goto error;

#define bc_dup(operand, result)\
    rjs_value_copy(rt, result, operand);

#define bc_set_priv_env(priv_env)\
    if ((r = rjs_class_state_set_priv_env(rt, script, priv_env)) == RJS_ERR)\
        goto error;

/* ++ */
static inline RJS_Result
do_bc_inc (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
#if ENABLE_BIG_INT
    if (rjs_value_is_big_int(rt, v)) {
        if (rjs_big_int_inc(rt, v, rv) == RJS_ERR)
            return RJS_ERR;
    }
    else
#endif /*ENABLE_BIG_INT*/
        rjs_number_inc(rt, v, rv);

    return RJS_OK;
}

/* -- */
static inline RJS_Result
do_bc_dec (RJS_Runtime *rt, RJS_Value *v, RJS_Value *rv)
{
#if ENABLE_BIG_INT
    if (rjs_value_is_big_int(rt, v)) {
        if (rjs_big_int_dec(rt, v, rv) == RJS_ERR)
            return RJS_ERR;
    }
    else
#endif /*ENABLE_BIG_INT*/
        rjs_number_dec(rt, v, rv);

    return RJS_OK;
}

#define bc_inc(operand, result)\
    if ((r = do_bc_inc(rt, operand, result)) == RJS_ERR)\
        goto error;

#define bc_dec(operand, result)\
    if ((r = do_bc_dec(rt, operand, result)) == RJS_ERR)\
        goto error;

/*Add.*/
static inline RJS_Result
do_bc_add (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *lp  = rjs_value_stack_push(rt);
    RJS_Value *rp  = rjs_value_stack_push(rt);
    RJS_Value *ls  = rjs_value_stack_push(rt);
    RJS_Value *rs  = rjs_value_stack_push(rt);
    RJS_Value *ln  = rjs_value_stack_push(rt);
    RJS_Value *rn  = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_primitive(rt, v1, lp, -1)) == RJS_ERR)
        goto end;
    if ((r = rjs_to_primitive(rt, v2, rp, -1)) == RJS_ERR)
        goto end;

    if (rjs_value_is_string(rt, lp) || rjs_value_is_string(rt, rp)) {
        if ((r = rjs_to_string(rt, lp, ls)) == RJS_ERR)
            goto end;
        if ((r = rjs_to_string(rt, rp, rs)) == RJS_ERR)
            goto end;
        r = rjs_string_concat(rt, ls, rs, rv);
        goto end;
    }

    if ((r = rjs_to_numeric(rt, lp, ln)) == RJS_ERR)
        goto end;
    if ((r = rjs_to_numeric(rt, rp, rn)) == RJS_ERR)
        goto end;
    if (rjs_value_get_type(rt, ln) != rjs_value_get_type(rt, rn)) {
        r = rjs_throw_type_error(rt, _("operands are not in same type"));
        goto end;
    }

#if ENABLE_BIG_INT
    if (rjs_value_is_big_int(rt, ln))
        rjs_big_int_add(rt, ln, rn, rv);
    else
#endif /*ENABLE_BIG_INT*/
        rjs_number_add(rt, ln, rn, rv);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#define bc_add(operand1, operand2, result)\
    if ((r = do_bc_add(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#if ENABLE_BIG_INT
#define BIG_INT_BINARY_OP(name)\
    if (rjs_value_is_big_int(rt, ln)) {\
        if ((r = rjs_big_int_##name(rt, ln, rn, rv)) == RJS_ERR)\
            goto end;\
    } else
#else
#define BIG_INT_BINARY_OP(name)
#endif

#define NUMERIC_BINARY_OP(name) \
static inline RJS_Result \
do_bc_##name (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2, RJS_Value *rv) \
{\
    size_t     top = rjs_value_stack_save(rt);\
    RJS_Value *ln  = rjs_value_stack_push(rt);\
    RJS_Value *rn  = rjs_value_stack_push(rt);\
    RJS_Result r;\
    if ((r = rjs_to_numeric(rt, v1, ln)) == RJS_ERR)\
        goto end;\
    if ((r = rjs_to_numeric(rt, v2, rn)) == RJS_ERR)\
        goto end;\
    if (rjs_value_get_type(rt, ln) != rjs_value_get_type(rt, rn)) {\
        r = rjs_throw_type_error(rt, _("operands are not in same type"));\
        goto end;\
    }\
    BIG_INT_BINARY_OP(name)\
        rjs_number_##name(rt, ln, rn, rv);\
    r = RJS_OK;\
end:\
    rjs_value_stack_restore(rt, top);\
    return r;\
}

NUMERIC_BINARY_OP(subtract)
NUMERIC_BINARY_OP(multiply)
NUMERIC_BINARY_OP(divide)
NUMERIC_BINARY_OP(remainder)
NUMERIC_BINARY_OP(exponentiate)
NUMERIC_BINARY_OP(left_shift)
NUMERIC_BINARY_OP(signed_right_shift)
NUMERIC_BINARY_OP(unsigned_right_shift)
NUMERIC_BINARY_OP(bitwise_and)
NUMERIC_BINARY_OP(bitwise_xor)
NUMERIC_BINARY_OP(bitwise_or)

#define bc_sub(operand1, operand2, result)\
    if ((r = do_bc_subtract(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_mul(operand1, operand2, result)\
    if ((r = do_bc_multiply(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_div(operand1, operand2, result)\
    if ((r = do_bc_divide(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_mod(operand1, operand2, result)\
    if ((r = do_bc_remainder(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_exp(operand1, operand2, result)\
    if ((r = do_bc_exponentiate(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_shl(operand1, operand2, result)\
    if ((r = do_bc_left_shift(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_shr(operand1, operand2, result)\
    if ((r = do_bc_signed_right_shift(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_ushr(operand1, operand2, result)\
    if ((r = do_bc_unsigned_right_shift(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_and(operand1, operand2, result)\
    if ((r = do_bc_bitwise_and(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_xor(operand1, operand2, result)\
    if ((r = do_bc_bitwise_xor(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

#define bc_or(operand1, operand2, result)\
    if ((r = do_bc_bitwise_or(rt, operand1, operand2, result)) == RJS_ERR)\
        goto error;

/*Compare 2 values.*/
static RJS_Result
do_bc_compare (RJS_Runtime *rt, RJS_Value *v1, RJS_Value *v2)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *p1  = rjs_value_stack_push(rt);
    RJS_Value *p2  = rjs_value_stack_push(rt);
    RJS_Value *n1  = rjs_value_stack_push(rt);
    RJS_Value *n2  = rjs_value_stack_push(rt);
    RJS_Result r;

    if ((r = rjs_to_primitive(rt, v1, p1, -1)) == RJS_ERR)
        goto end;
    if ((r = rjs_to_primitive(rt, v2, p2, -1)) == RJS_ERR)
        goto end;

    if (rjs_value_is_string(rt, p1) && rjs_value_is_string(rt, p2)) {
        r = rjs_string_compare(rt, p1, p2);
        goto end;
    }

#if ENABLE_BIG_INT
    if (rjs_value_is_big_int(rt, p2) && rjs_value_is_string(rt, p1)) {
        if ((r = rjs_string_to_big_int(rt, p1, n1)) == RJS_ERR)
            goto end;
        if (rjs_value_is_undefined(rt, n1)) {
            r = RJS_COMPARE_UNDEFINED;
            goto end;
        }

        r = rjs_big_int_compare(rt, n1, p2);
        goto end;
    }

    if (rjs_value_is_big_int(rt, p1) && rjs_value_is_string(rt, p2)) {
        if ((r = rjs_string_to_big_int(rt, p2, n2)) == RJS_ERR)
            goto end;
        if (rjs_value_is_undefined(rt, n2)) {
            r = RJS_COMPARE_UNDEFINED;
            goto end;
        }

        r = rjs_big_int_compare(rt, p1, n2);
        goto end;
    }
#endif /*ENABLE_BIG_INT*/

    if ((r = rjs_to_numeric(rt, p1, n1)) == RJS_ERR)
        goto end;
    if ((r = rjs_to_numeric(rt, p2, n2)) == RJS_ERR)
        goto end;
#if ENABLE_BIG_INT
    if (rjs_value_get_type(rt, n1) == rjs_value_get_type(rt, n2)) {

        if (rjs_value_is_big_int(rt, n1))
            r = rjs_big_int_compare(rt, n1, n2);
        else
#endif /*ENABLE_BIG_INT*/
            r = rjs_number_compare(rt, n1, n2);
#if ENABLE_BIG_INT
        goto end;
    }
#endif /*ENABLE_BIG_INT*/

#if ENABLE_BIG_INT
    if (rjs_value_is_number(rt, n1)) {
        RJS_Number n = rjs_value_get_number(rt, n1);

        r = rjs_big_int_compare_number (rt, n2, n);
        if (r == RJS_COMPARE_GREATER)
            r = RJS_COMPARE_LESS;
        else if (r == RJS_COMPARE_LESS)
            r = RJS_COMPARE_GREATER;
    } else {
        RJS_Number n = rjs_value_get_number(rt, n2);

        r = rjs_big_int_compare_number (rt, n1, n);
    }

#endif /*ENABLE_BIG_INT*/
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#define bc_lt(operand1, operand2, result)\
    if ((r = do_bc_compare(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, (r == RJS_COMPARE_LESS));

#define bc_le(operand1, operand2, result)\
    if ((r = do_bc_compare(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, (r == RJS_COMPARE_LESS) || (r == RJS_COMPARE_EQUAL));

#define bc_gt(operand1, operand2, result)\
    if ((r = do_bc_compare(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, (r == RJS_COMPARE_GREATER));

#define bc_ge(operand1, operand2, result)\
    if ((r = do_bc_compare(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, (r == RJS_COMPARE_GREATER) || (r == RJS_COMPARE_EQUAL));

#define bc_eq(operand1, operand2, result)\
    if ((r = rjs_is_loosely_equal(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_ne(operand1, operand2, result)\
    if ((r = rjs_is_loosely_equal(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, !r);

#define bc_strict_eq(operand1, operand2, result)\
    if ((r = rjs_is_strictly_equal(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_strict_ne(operand1, operand2, result)\
    if ((r = rjs_is_strictly_equal(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, !r);

#define bc_has_prop(operand1, operand2, result)\
    if ((r = rjs_has_property(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_has_priv(operand1, operand2, result)\
    if ((r = rjs_private_element_find(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_instanceof(operand1, operand2, result)\
    if ((r = rjs_instance_of(rt, operand1, operand2)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_del_binding(env, binding, result)\
    RJS_Environment *e = rjs_value_is_undefined(rt, env) ? NULL : rjs_value_get_gc_thing(rt, env); \
    if ((r = rjs_delete_binding(rt, e, &binding->binding_name, strict)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_del_prop(base, prop, result)\
    if ((r = rjs_delete_property(rt, base, &prop->prop_name, strict)) == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_del_prop_expr(base, prop, result)\
    RJS_PropertyName pn;\
    rjs_property_name_init(rt, &pn, prop);\
    r = rjs_delete_property(rt, base, &pn, strict);\
    rjs_property_name_deinit(rt, &pn);\
    if (r == RJS_ERR)\
        goto error;\
    rjs_value_set_boolean(rt, result, r);

#define bc_get_proto(operand, result)\
    if ((r = rjs_constructor_prototype(rt, operand, result)) == RJS_ERR)\
        goto error;

#define bc_object_create(operand, result)\
    if ((r = rjs_ordinary_object_create(rt, operand, result)) == RJS_ERR)\
        goto error;

#define bc_constr_create(constr_parent, proto, func, obj)\
    if ((r = rjs_class_state_create_constructor(rt, constr_parent, proto, script, func, obj)) == RJS_ERR)\
        goto error;

#define bc_default_constr(constr_parent, proto, name, obj)\
    if ((r = rjs_class_state_create_default_constructor(rt, constr_parent, proto, name, RJS_FALSE, obj)) == RJS_ERR)\
        goto error;

#define bc_derived_default_constr(constr_parent, proto, name, obj)\
    if ((r = rjs_class_state_create_default_constructor(rt, constr_parent, proto, name, RJS_TRUE, obj)) == RJS_ERR)\
        goto error;

#if ENABLE_PRIV_NAME
    #define RUNNING_PRIV_ENV rjs_private_env_running(rt)
#else
    #define RUNNING_PRIV_ENV NULL
#endif

#if ENABLE_ARROW_FUNC
    #define IS_CONSTRUCTOR(_f) !((_f)->flags & RJS_FUNC_FL_ARROW)
#else
    #define IS_CONSTRUCTOR(_f) RJS_TRUE
#endif /*ENABLE_ARROW_FUNC*/

#define bc_func_create(func, dest)\
    RJS_Environment *env       = rjs_lex_env_running(rt);\
    RJS_PrivateEnv  *priv      = RUNNING_PRIV_ENV;\
    RJS_Bool         is_constr = IS_CONSTRUCTOR(func);\
    if ((r = rjs_create_function(rt, script, func, env, priv, is_constr, dest)) == RJS_ERR)\
        goto error;

#define bc_class_init()\
    if ((r = rjs_class_state_init(rt)) == RJS_ERR)\
        goto error;

#define bc_object_method_add(name, func)\
    if ((r = rjs_object_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_METHOD, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_object_getter_add(name, func)\
    if ((r = rjs_object_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_GET, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_object_setter_add(name, func)\
    if ((r = rjs_object_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_SET, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_method_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_METHOD, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_getter_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_GET, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_setter_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_SET, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_static_method_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_METHOD, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_static_getter_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_GET, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_static_setter_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_SET, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_field_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_FIELD, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_inst_field_add(name, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_INST_FIELD, name, script, func)) == RJS_ERR)\
        goto error;

#define bc_set_af_field()\
    if ((r = rjs_class_state_set_anonymous_function_field(rt)) == RJS_ERR)\
        goto error;

#define bc_priv_method_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_PRIV_METHOD, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_priv_getter_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_PRIV_GET, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_priv_setter_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_PRIV_SET, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_static_priv_method_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_PRIV_METHOD, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_static_priv_getter_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_PRIV_GET, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_static_priv_setter_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_PRIV_SET, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_priv_field_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_PRIV_FIELD, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_priv_inst_field_add(priv, func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_PRIV_INST_FIELD, priv, script, func)) == RJS_ERR)\
        goto error;

#define bc_static_block_add(func)\
    if ((r = rjs_class_state_add_element(rt, RJS_CLASS_ELEMENT_STATIC_INIT, rjs_v_undefined(rt), script, func)) == RJS_ERR)\
        goto error;

#define bc_push_class()\
    if ((r = rjs_class_state_push(rt)) == RJS_ERR)\
        goto error;

#define bc_binding_resolve(binding, env)\
    RJS_Environment *e;\
    if ((r = rjs_resolve_binding(rt, &binding->binding_name, &e)) == RJS_ERR)\
        goto error;\
    if (e)\
        rjs_value_set_gc_thing(rt, env, e);\
    else\
        rjs_value_set_undefined(rt, env);

#define bc_binding_init(env, binding, value)\
    RJS_Environment *e = rjs_value_is_undefined(rt, env) ? NULL : rjs_value_get_gc_thing(rt, env);\
    if ((r = rjs_env_initialize_binding(rt, e, &binding->binding_name, value)) == RJS_ERR)\
        goto error;

#define bc_binding_set(env, binding, value)\
    RJS_Environment *e = rjs_value_is_undefined(rt, env) ? NULL : rjs_value_get_gc_thing(rt, env);\
    if ((r = rjs_set_binding_value(rt, e, &binding->binding_name, value, strict)) == RJS_ERR)\
        goto error;

#define bc_binding_get(env, binding, dest)\
    RJS_Environment *e = rjs_value_is_undefined(rt, env) ? NULL : rjs_value_get_gc_thing(rt, env);\
    if ((r = rjs_get_binding_value(rt, e, &binding->binding_name, strict, dest)) == RJS_ERR)\
        goto error;

#define bc_prop_get(base, prop, dest)\
    if ((r = rjs_get_v(rt, base, &prop->prop_name, dest)) == RJS_ERR)\
        goto error;

#define bc_prop_get_expr(base, prop, dest)\
    RJS_PropertyName pn;\
    rjs_property_name_init(rt, &pn, prop);\
    r = rjs_get_v(rt, base, &pn, dest);\
    rjs_property_name_deinit(rt, &pn);\
    if (r == RJS_ERR)\
        goto error;

#define bc_super_prop_get(thiz, prop, dest)\
    if ((r = rjs_super_get_v(rt, thiz, &prop->prop_name, dest)) == RJS_ERR)\
        goto error;

#define bc_super_prop_get_expr(thiz, prop, dest)\
    RJS_PropertyName pn;\
    rjs_property_name_init(rt, &pn, prop);\
    r = rjs_super_get_v(rt, thiz, &pn, dest);\
    rjs_property_name_deinit(rt, &pn);\
    if (r == RJS_ERR)\
        goto error;

#define bc_priv_get(base, priv, dest)\
    if ((r = rjs_private_get(rt, base, &priv->prop_name, dest)) == RJS_ERR)\
        goto error;

#define bc_prop_set(base, prop, value)\
    if ((r = rjs_set_v(rt, base, &prop->prop_name, value, strict)) == RJS_ERR)\
        goto error;

#define bc_prop_set_expr(base, prop, value)\
    RJS_PropertyName pn;\
    rjs_property_name_init(rt, &pn, prop);\
    r = rjs_set_v(rt, base, &pn, value, strict);\
    rjs_property_name_deinit(rt, &pn);\
    if (r == RJS_ERR)\
        goto error;

#define bc_super_prop_set(thiz, prop, value)\
    if ((r = rjs_super_set_v(rt, thiz, &prop->prop_name, value, strict)) == RJS_ERR)\
        goto error;

#define bc_super_prop_set_expr(thiz, prop, value)\
    RJS_PropertyName pn;\
    rjs_property_name_init(rt, &pn, prop);\
    r = rjs_super_set_v(rt, thiz, &pn, value, strict);\
    rjs_property_name_deinit(rt, &pn);\
    if (r == RJS_ERR)\
        goto error;

#define bc_priv_set(base, priv, value)\
    if ((r = rjs_private_set(rt, base, &priv->prop_name, value)) == RJS_ERR)\
        goto error;

#define bc_arg_add(arg)\
    if ((r = rjs_call_state_push_arg(rt, arg)) == RJS_ERR)\
        goto error;

#define bc_spread_args_add(arg)\
    if ((r = rjs_call_state_push_spread_args(rt, arg)) == RJS_ERR)\
        goto error;

#define bc_push_call(func, thiz)\
    if ((r = rjs_call_state_push(rt, func, thiz)) == RJS_ERR)\
        goto error;

#define bc_call(result)\
    if ((r = rjs_call_state_call(rt, sp, 0, result)) == RJS_ERR)\
        goto error;

#define bc_tail_call(result)\
    if ((r = rjs_call_state_call(rt, sp, RJS_CALL_FL_TCO, result)) == RJS_ERR)\
        goto error;\
    if (r == RJS_FALSE) {\
        sc->ip = sf->byte_code_start;\
        continue;\
    }

#define bc_eval(result)\
    if ((r = rjs_call_state_call(rt, sp, RJS_CALL_FL_EVAL, result)) == RJS_ERR)\
        goto error;

#define bc_tail_eval(result)\
    if ((r = rjs_call_state_call(rt, sp, RJS_CALL_FL_EVAL|RJS_CALL_FL_TCO, result)) == RJS_ERR)\
        goto error;\
    if (r == RJS_FALSE) {\
        sc->ip = sf->byte_code_start;\
        continue;\
    }

#define bc_push_super_call()\
    if ((r = rjs_super_call_state_push(rt)) == RJS_ERR)\
        goto error;\
    if (r == RJS_FALSE)\
        continue;

#define bc_super_call(result)\
    if ((r = rjs_call_state_super_call(rt, result)) == RJS_ERR)\
        goto error;

#define bc_push_new(constr)\
    if ((r = rjs_new_state_push(rt, constr)) == RJS_ERR)\
        goto error;

#define bc_new(result)\
    if ((r = rjs_call_state_new(rt, result)) == RJS_ERR)\
        goto error;

#define bc_push_concat()\
    RJS_Realm *realm = rjs_realm_current(rt);\
    if ((r = rjs_call_state_push(rt, rjs_o_Concat(realm), rjs_v_undefined(rt))) == RJS_ERR)\
        goto error;

#define bc_push_array_assi(value)\
    if ((r = rjs_array_assi_state_push(rt, value)) == RJS_ERR)\
        goto error;

#define bc_next_array_item()\
    if ((r = rjs_iter_state_step(rt, NULL)) == RJS_ERR)\
        goto error;

#define bc_get_array_item(dest)\
    if ((r = rjs_iter_state_step(rt, dest)) == RJS_ERR)\
        goto error;

#define bc_rest_array_items(dest)\
    if ((r = rjs_iter_state_rest(rt, dest)) == RJS_ERR)\
        goto error;

#define bc_push_object_assi(value)\
    if ((r = rjs_object_assi_state_push(rt, value)) == RJS_ERR)\
        goto error;

#define bc_get_object_prop(prop, dest)\
    if ((r = rjs_object_assi_state_step(rt, &prop->prop_name, dest)) == RJS_ERR)\
        goto error;

#define bc_get_object_prop_expr(prop, dest)\
    RJS_PropertyName pn;\
    rjs_property_name_init(rt, &pn, prop);\
    r = rjs_object_assi_state_step(rt, &pn, dest);\
    rjs_property_name_deinit(rt, &pn);\
    if (r == RJS_ERR)\
        goto error;

#define bc_rest_object_props(dest)\
    if ((r = rjs_object_assi_state_rest(rt, dest)) == RJS_ERR)\
        goto error;

#define bc_push_new_array(dest)\
    if ((r = rjs_array_state_push(rt, dest)) == RJS_ERR)\
        goto error;

#define bc_array_elision_item()\
    if ((r = rjs_array_state_elision(rt)) == RJS_ERR)\
        goto error;

#define bc_array_add_item(value)\
    if ((r = rjs_array_state_add(rt, value)) == RJS_ERR)\
        goto error;

#define bc_array_spread_items(value)\
    if ((r = rjs_array_state_spread(rt, value)) == RJS_ERR)\
        goto error;

#define bc_push_new_object(dest)\
    if ((r = rjs_object_state_push(rt, dest)) == RJS_ERR)\
        goto error;

#define bc_object_add_prop(prop, value)\
    if ((r = rjs_object_state_add(rt, prop, value, RJS_FALSE)) == RJS_ERR)\
        goto error;

#define bc_object_add_func(prop, value)\
    if ((r = rjs_object_state_add(rt, prop, value, RJS_TRUE)) == RJS_ERR)\
        goto error;

#define bc_object_spread_props(value)\
    if ((r = rjs_object_state_spread(rt, value)) == RJS_ERR)\
        goto error;

#define bc_pop_state()\
    RJS_State *s;\
    s = rjs_state_top(rt);\
    if (s->type == RJS_STATE_TRY) {\
        switch (s->s.s_try.state) {\
        case RJS_TRY_STATE_TRY:\
        case RJS_TRY_STATE_CATCH:\
            s->s.s_try.next_ip = sc->ip + 1;\
            sc->ip = s->s.s_try.finally_ip;\
            goto restart;\
        case RJS_TRY_STATE_FINALLY:\
            switch (s->s.s_try.next_op) {\
            case RJS_TRY_NEXT_OP_NORMAL:\
            case RJS_TRY_NEXT_OP_THROW:\
                break;\
            case RJS_TRY_NEXT_OP_RETURN:\
                rjs_state_pop(rt);\
                r = RJS_RETURN;\
                goto ret;\
            }\
            break;\
        case RJS_TRY_STATE_END:\
            switch (s->s.s_try.next_op) {\
            case RJS_TRY_NEXT_OP_NORMAL:\
                if (s->s.s_try.next_ip != (size_t)-1) {\
                    rjs_state_pop(rt);\
                    sc->ip = s->s.s_try.next_ip;\
                    goto restart;\
                }\
                break;\
            case RJS_TRY_NEXT_OP_THROW:\
                rjs_value_copy(rt, &rt->error, s->s.s_try.error);\
                rt->error_flag = RJS_TRUE;\
                rjs_state_pop(rt);\
                r = RJS_ERR;\
                goto error;\
            case RJS_TRY_NEXT_OP_RETURN:\
                rjs_state_pop(rt);\
                r = RJS_RETURN;\
                goto ret;\
            }\
            break;\
        }\
    }\
    if ((r = rjs_state_pop_await(rt, rjs_await_async_iterator_close, 0, NULL)) == RJS_ERR)\
        goto error;\
    if (r == RJS_FALSE) {\
        sc->ip += ip_size;\
        goto end;\
    }

#define bc_set_proto(obj, proto)\
    if (rjs_value_is_null(rt, proto) || rjs_value_is_object(rt, proto))\
        rjs_object_set_prototype_of(rt, obj, proto);

#define bc_push_with(value, decl)\
    RJS_Environment *env = sc->scb.lex_env;\
    size_t           top = rjs_value_stack_save(rt);\
    RJS_Value       *o   = rjs_value_stack_push(rt);\
    if ((r = rjs_to_object(rt, value, o)) == RJS_ERR)\
        goto error;\
    rjs_object_env_new(rt, &rt->env, o, RJS_TRUE, decl, env);\
    rjs_value_stack_restore(rt, top);\
    rjs_lex_env_state_push(rt, rt->env);

#define bc_push_try(catch_label, final_label)\
    if ((r = rjs_try_state_push(rt, sc->ip + catch_label, sc->ip + final_label)) == RJS_ERR)\
        goto error;

#define bc_catch_error(dest)\
    RJS_State *s = rjs_state_top(rt);\
    assert(s->type == RJS_STATE_TRY);\
    s->s.s_try.state   = RJS_TRY_STATE_CATCH;\
    s->s.s_try.next_op = RJS_TRY_NEXT_OP_NORMAL;\
    if (!rjs_catch(rt, dest))\
        rjs_value_set_undefined(rt, dest);

#define bc_finally()\
    RJS_State *s = rjs_state_top(rt);\
    assert(s->type == RJS_STATE_TRY);\
    s->s.s_try.state = RJS_TRY_STATE_FINALLY;

#define bc_try_end()\
    RJS_State *s = rjs_state_top(rt);\
    assert(s->type == RJS_STATE_TRY);\
    s->s.s_try.state = RJS_TRY_STATE_END;

#define bc_throw_error(value)\
    r = rjs_throw(rt, value);\
    goto error;

#define bc_throw_ref_error()\
    r = rjs_throw_reference_error(rt, _("super binding cannot be deleted"));\
    goto error;

#define bc_generator_start()\
    rjs_generator_start(rt, rv);\
    sc->ip += ip_size;\
    r = RJS_SUSPEND;\
    goto end;

#define bc_return_value(value)\
    rjs_value_copy(rt, &sc->retv, value);\
    r = RJS_RETURN;\
    goto ret;

#define bc_jump(label)\
    sc->ip = ((ssize_t)sc->ip) + label;\
    continue;

#define bc_jump_true(value, label)\
    if (rjs_to_boolean(rt, value)) {\
        sc->ip = ((ssize_t)sc->ip) + label;\
        continue;\
    }

#define bc_jump_false(value, label)\
    if (!rjs_to_boolean(rt, value)) {\
        sc->ip = ((ssize_t)sc->ip) + label;\
        continue;\
    }

#define bc_debugger()\
    RJS_LOGD("debugger");

/**
 * Call the byte code function.
 * \param relam The current runtime.
 * \param type Call type.
 * \param v The input value.
 * \param[out] rv The return value.
 * \retval RJS_OK On sucess.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_bc_call (RJS_Runtime *rt, RJS_ScriptCallType type, RJS_Value *iv, RJS_Value *rv)
{
    RJS_Context          *ctxt;
    RJS_ScriptContext    *sc;
    RJS_Script           *script;
    RJS_ScriptFunc       *sf;
    size_t                ip_end, ip_size;
    uint8_t              *bc;
    RJS_Bool              strict;
    size_t                sp;
    RJS_Result            r = RJS_OK;

    ctxt   = rjs_context_running(rt);
    sc     = (RJS_ScriptContext*)ctxt;
    script = sc->script;
    sf     = sc->script_func;
    strict = sf->flags & RJS_FUNC_FL_STRICT;
    ip_end = sf->byte_code_start + sf->byte_code_len;

    if ((type == RJS_SCRIPT_CALL_CONSTRUCT) || (type == RJS_SCRIPT_CALL_SYNC_START)) {
        sp = rt->rb.curr_native_stack->state.item_num;
    } else {
        sp = 0;
    }

#if ENABLE_GENERATOR
    /*Set the generator's resume value.*/
    switch (type) {
    case RJS_SCRIPT_CALL_GENERATOR_RESUME: {
        RJS_Generator *g;

        g = (RJS_Generator*)rjs_value_get_object(rt, &ctxt->function);
        g->received_type = RJS_GENERATOR_REQUEST_NEXT;
        rjs_value_copy(rt, &g->receivedv, iv);
        break;
    }
    case RJS_SCRIPT_CALL_GENERATOR_THROW: {
        RJS_Generator *g;

        g = (RJS_Generator*)rjs_value_get_object(rt, &ctxt->function);
        g->received_type = RJS_GENERATOR_REQUEST_THROW;
        rjs_value_copy(rt, &g->receivedv, iv);
        break;
    }
    case RJS_SCRIPT_CALL_GENERATOR_RETURN: {
        RJS_Generator *g;

        g = (RJS_Generator*)rjs_value_get_object(rt, &ctxt->function);
        g->received_type = RJS_GENERATOR_REQUEST_RETURN;
        rjs_value_copy(rt, &g->receivedv, iv);
        break;
    }
    default:
        break;
    }
#endif /*ENABLE_GENERATOR*/

#if ENABLE_ASYNC
    /*Resume from await operation.*/
    if ((type == RJS_SCRIPT_CALL_ASYNC_FULFILL) || (type == RJS_SCRIPT_CALL_ASYNC_REJECT)) {
        RJS_AsyncContext *ac = (RJS_AsyncContext*)rjs_context_running(rt);
        RJS_AsyncOpFunc   op = ac->op;

        if (op) {
            ac->op = NULL;
            r = op(rt, type, iv, rv);

            if (r == RJS_ERR)
                goto error;
            if (r == RJS_RETURN) {
                rjs_value_copy(rt, &sc->retv, rv);
                goto ret;
            }
            if (r == RJS_SUSPEND)
                goto end;
        }
    }
#endif /*ENABLE_ASYNC*/

restart:
    while (sc->ip < ip_end) {
        bc = sc->script->byte_code + sc->ip;

#if 0
        fprintf(stdout, "%05"PRIdPTR": ", sc->ip - sc->script_func->byte_code_start);
        rjs_bc_disassemble(rt, stdout, bc);
        fprintf(stdout, "\n");
#endif

        switch (bc[0]) {
#include <rjs_bc_run_inc.c>
        default:
            assert(0);
        }
    }

    rjs_value_set_undefined(rt, rv);

    r = RJS_OK;
ret:
error:
    while (sp < rt->rb.curr_native_stack->state.item_num) {
        RJS_State *s;
        RJS_Result r1;

        s = rjs_state_top(rt);
        if (s->type == RJS_STATE_TRY) {
            if (r == RJS_ERR)
                s->s.s_try.next_op = RJS_TRY_NEXT_OP_THROW;
            else if (r == RJS_RETURN)
                s->s.s_try.next_op = RJS_TRY_NEXT_OP_RETURN;

            switch (s->s.s_try.state) {
            case RJS_TRY_STATE_TRY:
                if (r == RJS_ERR)
                    rjs_value_copy(rt, s->s.s_try.error, &rt->error);
                sc->ip = (r == RJS_ERR) ? s->s.s_try.catch_ip : s->s.s_try.finally_ip;
                goto restart;
            case RJS_TRY_STATE_CATCH:
                if (r == RJS_ERR)
                    rjs_value_copy(rt, s->s.s_try.error, &rt->error);
                sc->ip = s->s.s_try.finally_ip;
                goto restart;
            case RJS_TRY_STATE_FINALLY:
                rjs_state_pop(rt);
                break;
            default:
                assert(0);
            }
        } else {
            if ((r1 = rjs_state_pop_await(rt, await_pop_async_iter_state, r, &rt->error)) == RJS_FALSE) {
                r = RJS_SUSPEND;
                goto end;
            } else if (r1 == RJS_ERR) {
                r = r1;
            }
        }
    }

    if (r == RJS_RETURN) r = RJS_OK;
    if (r == RJS_OK)
        rjs_value_copy(rt, rv, &sc->retv);
end:
    if (r == RJS_SUSPEND) rt->error_flag = 0;
    return r;
}
