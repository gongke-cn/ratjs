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
 * Native stack.
 */

#ifndef _RJS_NATIVE_STACK_H_
#define _RJS_NATIVE_STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup nstack Native stack
 * Native value stack and native state stack
 * @{
 */

/**
 * Save the value stack's top.
 * \param rt The current runtime.
 * \return The value stack's top.
 */
static inline size_t
rjs_value_stack_save (RJS_Runtime *rt)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;

    return rb->curr_native_stack->value.item_num;
}

/**
 * Allocate values buffer in the value stack.
 * \param rt The current runtime.
 * \param n Number of values in the buffer.
 * \return The first value's reference in the stack.
 */
extern RJS_Value*
rjs_value_stack_append (RJS_Runtime *rt, size_t n);

/**
 * Allocate values buffer in the value stack.
 * \param rt The current runtime.
 * \param n Number of values in the buffer.
 * \return The first value's reference in the stack.
 */
static inline RJS_Value*
rjs_value_stack_push_n (RJS_Runtime *rt, size_t n)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;

    if (rb->curr_native_stack->value.item_cap >= rb->curr_native_stack->value.item_num + n) {
        RJS_Value *v = rjs_stack_pointer_to_value(rb->curr_native_stack->value.item_num);

        rb->curr_native_stack->value.item_num += n;

        rjs_value_buffer_fill_undefined(rt, v, n);

        return v;
    }

    return rjs_value_stack_append(rt, n);
}

/**
 * Allocate a value in the value stack.>
 * \param rt The current runtime.
 * \return Return the value reference in the stack.
 */
static inline RJS_Value*
rjs_value_stack_push (RJS_Runtime *rt)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;

    if (rb->curr_native_stack->value.item_cap > rb->curr_native_stack->value.item_num) {
        RJS_Value *v = rjs_stack_pointer_to_value(rb->curr_native_stack->value.item_num);

        rb->curr_native_stack->value.item_num ++;

        rjs_value_set_undefined(rt, v);

        return v;
    }

    return rjs_value_stack_append(rt, 1);
}

/**
 * Restore the old value stack's top.
 * \param rt The current runtime.
 * \param top The top value to be restored.
 */
static inline void
rjs_value_stack_restore (RJS_Runtime *rt, size_t top)
{
    RJS_RuntimeBase *rb = (RJS_RuntimeBase*)rt;

    rb->curr_native_stack->value.item_num = top;
}

/**
 * Restore the old value stack's top from a value pointer.
 * \param rt The current runtime.
 * \param v The top value pointer to be restored.
 */
static inline void
rjs_value_stack_restore_pointer (RJS_Runtime *rt, RJS_Value *v)
{
    RJS_RuntimeBase *rb  = (RJS_RuntimeBase*)rt;
    size_t           top = rjs_value_to_stack_pointer(v);

    rb->curr_native_stack->value.item_num = top;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

