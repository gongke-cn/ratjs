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

/*GC start only when the memory allocated size > this value.*/
#define RJS_GC_START_SIZE      (64*1024)
/*The default mark stack size.*/
#define RJS_GC_MARK_STACK_SIZE (256)

/*Get the next pointer in the thing.*/
static inline RJS_GcThing*
gc_thing_get_next (RJS_GcThing *gt)
{
    return RJS_SIZE2PTR(gt->next_flags & ~3);
}

/*Scan a thing.*/
static inline void
gc_scan (RJS_Runtime *rt, RJS_GcThing *gt)
{
    gt->next_flags |= RJS_GC_THING_FL_SCANNED;

    if (gt->ops->scan)
        gt->ops->scan(rt, gt);
}

/*Scan the root things.*/
static void
gc_scan_root (RJS_Runtime *rt)
{
    rjs_gc_scan_internal_strings(rt);

    /*Error.*/
    rjs_gc_scan_value(rt, &rt->error);
    if (rt->error_context)
        rjs_gc_mark(rt, rt->error_context);

    /*Realm.*/
    if (rt->main_realm)
        rjs_gc_mark(rt, rt->main_realm);
    if (rt->rb.bot_realm)
        rjs_gc_mark(rt, rt->rb.bot_realm);

    /*Parser.*/
    if (rt->parser)
        rjs_gc_scan_parser(rt, rt->parser);

    /*Temporary environment.*/
    if (rt->env)
        rjs_gc_mark(rt, rt->env);

    /*Context stack.*/
    rjs_gc_scan_context_stack(rt);

    /*State the native stack.*/
    rjs_gc_scan_native_stack(rt, &rt->native_stack);

    /*Job queue.*/
    rjs_gc_scan_job(rt);

    /*Symbol registry.*/
    rjs_gc_scan_symbol_registry(rt);

#if ENABLE_MODULE
    rjs_gc_scan_module(rt);
#endif /*ENABLE_MODULE*/

#if ENABLE_FINALIZATION_REGISTRY
    rjs_gc_scan_finalization_registry(rt);
#endif /*ENABLE_FINALIZATION_REGISTRY*/

#if ENABLE_CTYPE
    rjs_gc_scan_ctype(rt);
#endif /*ENABLE_CTYPE*/

    /*Scan the runtime's native data.*/
    rjs_native_data_scan(rt, &rt->native_data);
}

/*Scan the used things.*/
static void
gc_scan_things (RJS_Runtime *rt)
{
    RJS_GcThing *gt;

    while (1) {
        while (rt->rb.gc_mark_stack.item_num) {
            rt->rb.gc_mark_stack.item_num --;

            gt = rt->rb.gc_mark_stack.items[rt->rb.gc_mark_stack.item_num];

            gc_scan(rt, gt);
        }

        if (!rt->rb.gc_mark_stack_full)
            break;

        if (rt->gc_scan_count > 5) {
            rjs_vector_set_capacity(&rt->rb.gc_mark_stack,
                    rt->rb.gc_mark_stack.item_cap * 2,
                    rt);

            RJS_LOGD("set gc mark stack's capacity to %"PRIdPTR"",
                    rt->rb.gc_mark_stack.item_cap);
        }

        rt->rb.gc_mark_stack_full = RJS_FALSE;

        for (gt = rt->gc_thing_list; gt; gt = gc_thing_get_next(gt)) {
            if ((gt->next_flags & 3) == RJS_GC_THING_FL_MARKED) {
                gc_scan(rt, gt);
            }
        }
    }
}

/*Sweep the unused things.*/
static void
gc_sweep (RJS_Runtime *rt)
{
    RJS_GcThing *gt, **pgt;

    pgt = &rt->gc_thing_list;
    while ((gt = *pgt)) {
        if (gt->next_flags & RJS_GC_THING_FL_MARKED) {
            gt->next_flags &= ~3;
            pgt = (RJS_GcThing**)&gt->next_flags;
        } else {
            *pgt = (RJS_GcThing*)gt->next_flags;

            if (gt->ops->free)
                gt->ops->free(rt, gt);
        }
    }
}

/*Run the GC process.*/
static void
gc_run (RJS_Runtime *rt)
{
    size_t old = rt->mem_size;

    RJS_LOGD("gc start");

    rt->rb.gc_running         = RJS_TRUE;
    rt->rb.gc_mark_stack_full = RJS_FALSE;
    rt->gc_scan_count         = 0;
    rjs_vector_set_capacity(&rt->rb.gc_mark_stack, RJS_GC_MARK_STACK_SIZE, rt);

    /*Scan the root objects.*/
    gc_scan_root(rt);

    /*Scan the used things.*/
    gc_scan_things(rt);

#if ENABLE_WEAK_REF
    /*Solve the weak references.*/
    rjs_solve_weak_refs(rt);
#endif /*ENABLE_WEAK_REF*/

#if ENABLE_FINALIZATION_REGISTRY
    /*Solve the finalization callbacks.*/
    rjs_solve_finalization_registry(rt);
#endif /*ENABLE_FINALIZATION_REGISTRY*/

#if ENABLE_GENERATOR || ENABLE_ASYNC
    /*Solve generator contexts.*/
    rjs_solve_generator_contexts(rt);
#endif /*ENABLE_GENERATOR || ENABLE_ASYNC*/

    /*Sweep the unused things.*/
    gc_sweep(rt);

    rt->gc_last_mem_size = rt->mem_size;

    rt->rb.gc_running = RJS_FALSE;

    RJS_LOGD("gc end, collected %"PRIdPTR"B", old - rt->mem_size);
}

/**
 * Add a thing to the rt. Then the thing is managed by the GC.
 * \param rt The current runtime.
 * \param thing The thing to be added.
 * \param ops The thing's operation functions.
 */
void
rjs_gc_add (RJS_Runtime *rt, void *thing, const RJS_GcThingOps *ops)
{
    RJS_GcThing *gt = thing;

    gt->ops        = ops;
    gt->next_flags = RJS_PTR2SIZE(rt->gc_thing_list);

    rt->gc_thing_list = gt;

    if (rt->rb.gc_running) {
        rjs_gc_mark(rt, thing);
    } else {
        if (rt->rb.gc_enable && (rt->mem_size > RJS_GC_START_SIZE)) {
            if (rt->gc_last_mem_size * 4 < rt->mem_size * 3) {
                gc_run(rt);
            }
        }
    }
}

/**
 * Run the GC process.
 * \param rt The current runtime.
 */
void
rjs_gc_run (RJS_Runtime *rt)
{
    gc_run(rt);
}

/**
 * Initialize the GC resource in the rt.
 */
void
rjs_runtime_gc_init (RJS_Runtime *rt)
{
    rt->rb.gc_enable          = RJS_FALSE;
    rt->rb.gc_running         = RJS_FALSE;
    rt->rb.gc_mark_stack_full = RJS_FALSE;
    rt->gc_thing_list         = NULL;
    rt->gc_scan_count         = 0;
    rt->gc_last_mem_size      = 0;

    rjs_vector_init(&rt->rb.gc_mark_stack);
}

/**
 * Release the GC resource in the rt.
 */
void
rjs_runtime_gc_deinit (RJS_Runtime *rt)
{
    RJS_GcThing *gt, *ngt;

    /*Free all the things.*/
    for (gt = rt->gc_thing_list; gt; gt = ngt) {
        ngt = gc_thing_get_next(gt);

        if (gt->ops->free)
            gt->ops->free(rt, gt);
    }

    rjs_vector_deinit(&rt->rb.gc_mark_stack, rt);
}
