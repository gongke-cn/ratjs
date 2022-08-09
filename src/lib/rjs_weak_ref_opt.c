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

/**
 * Add a weak reference.
 * \param rt The current runtime.
 * \param base The base object.
 * \param ref The referenced object.
 * \param on_final On final function.
 * \param data The parameter of the onfinal function.
 * \return The weak reference.
 * \retval The value is not an GC thing.
 */
extern RJS_WeakRef*
rjs_weak_ref_add (RJS_Runtime *rt, RJS_Value *base, RJS_Value *ref,
        RJS_WeakRefOnFinalFunc on_final)
{
    RJS_WeakRef *wr;

    RJS_NEW(rt, wr);

    rjs_value_copy(rt, &wr->base, base);
    rjs_value_copy(rt, &wr->ref, ref);

    wr->on_final = on_final;

    rjs_list_append(&rt->weak_ref_list, &wr->ln);

    return wr;
}

/**
 * Free a weak reference.
 * \param rt The current runtime.
 * \param wr The weak reference to be freed.
 */
void
rjs_weak_ref_free (RJS_Runtime *rt, RJS_WeakRef *wr)
{
    rjs_list_remove(&wr->ln);

    RJS_DEL(rt, wr);
}

/**
 * Solve the weak references in the rt.
 * \param rt The current runtime.
 */
void
rjs_solve_weak_refs (RJS_Runtime *rt)
{
    RJS_WeakRef *r, *nr;

    rjs_list_foreach_safe_c(&rt->weak_ref_list, r, nr, RJS_WeakRef, ln) {
        RJS_GcThing *base_gt, *ref_gt;

        base_gt = rjs_value_get_gc_thing(rt, &r->base);
        ref_gt  = rjs_value_get_gc_thing(rt, &r->ref);

        if (!(base_gt->next_flags & RJS_GC_THING_FL_MARKED)) {
            rjs_list_remove(&r->ln);

            RJS_DEL(rt, r);
        } else if (!(ref_gt->next_flags & RJS_GC_THING_FL_MARKED)) {
            rjs_list_remove(&r->ln);

            r->on_final(rt, r);

            RJS_DEL(rt, r);
        }
    }
}

/**
 * Initialize the weak reference data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_weak_ref_init (RJS_Runtime *rt)
{
    rjs_list_init(&rt->weak_ref_list);
}

/**
 * Release the weak reference data in the rt.
 * \param rt The current runtime.
 */
void
rjs_runtime_weak_ref_deinit (RJS_Runtime *rt)
{
    RJS_WeakRef *r, *nr;

    rjs_list_foreach_safe_c(&rt->weak_ref_list, r, nr, RJS_WeakRef, ln) {
        RJS_DEL(rt, r);
    }
}
