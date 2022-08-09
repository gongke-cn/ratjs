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

/*Scan the reference things in the symbol.*/
static void
symbol_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
    RJS_Symbol *s = ptr;

    rjs_gc_scan_value(rt, &s->description);
}

/*Free the symbol.*/
static void
symbol_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Symbol *s = ptr;

    RJS_DEL(rt, s);
}

/*Symbol GC operation functions.*/
static const RJS_GcThingOps
symbol_gc_ops = {
    RJS_GC_THING_SYMBOL,
    symbol_op_gc_scan,
    symbol_op_gc_free
};

/**
 * Create a new symbol.
 * \param rt The current runtime.
 * \param[out] v Return the symbol value.
 * \param desc The description of the symbol.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_symbol_new (RJS_Runtime *rt, RJS_Value *v, RJS_Value *desc)
{
    RJS_Symbol *s;

    RJS_NEW(rt, s);

    if (desc)
        rjs_value_copy(rt, &s->description, desc);
    else
        rjs_value_set_undefined(rt, &s->description);

    rjs_value_set_symbol(rt, v, s);
    rjs_gc_add(rt, s, &symbol_gc_ops);

    return RJS_OK;
}

/**
 * Initialize the symbol registry.
 * \param rt The current runtime.
 */
void
rjs_runtime_symbol_registry_init (RJS_Runtime *rt)
{
    rjs_hash_init(&rt->sym_reg_key_hash);
    rjs_hash_init(&rt->sym_reg_sym_hash);
}

/**
 * Release the symbol registry.
 * \param rt The current runtime.
 */
void
rjs_runtime_symbol_registry_deinit (RJS_Runtime *rt)
{
    size_t              i;
    RJS_SymbolRegistry *sr, *nsr;

    rjs_hash_foreach_safe_c(&rt->sym_reg_key_hash, i, sr, nsr, RJS_SymbolRegistry, key_he) {
        RJS_DEL(rt, sr);
    }

    rjs_hash_deinit(&rt->sym_reg_key_hash, &rjs_hash_size_ops, rt);
    rjs_hash_deinit(&rt->sym_reg_sym_hash, &rjs_hash_size_ops, rt);
}

/**
 * Scan the referenced things in the symbol registry.
 * \param rt The current runtime.
 */
void
rjs_gc_scan_symbol_registry (RJS_Runtime *rt)
{
    RJS_SymbolRegistry *sr;
    size_t              i;

    rjs_hash_foreach_c(&rt->sym_reg_key_hash, i, sr, RJS_SymbolRegistry, key_he) {
        rjs_gc_scan_value(rt, &sr->key);
        rjs_gc_scan_value(rt, &sr->symbol);
    }
}
