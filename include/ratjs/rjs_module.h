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
 * Module.
 */

#ifndef _RJS_MODULE_H_
#define _RJS_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup module Module
 * Module
 * @{
 */

/**Resolve binding.*/
typedef struct {
    RJS_Value *module; /**< The target module.*/
    RJS_Value *name;   /**< The binding name.*/
} RJS_ResolveBinding;

/**Module import entry description.*/
typedef struct {
    char *module_name; /**< The module name.*/
    char *import_name; /**< The import name.*/
    char *local_name;  /**< The local name.*/
} RJS_ModuleImportDesc;

/**Module export entry description.*/
typedef struct {
    char *module_name; /**< The module name.*/
    char *import_name; /**< The import name.*/
    char *local_name;  /**< The local name.*/
    char *export_name; /**< The export name.*/
} RJS_ModuleExportDesc;

/**
 * Initialize the resolve binding record.
 * \param rt The current runtime.
 * \param rb The resolve binding to be initialized.
 */
static inline void
rjs_resolve_binding_init (RJS_Runtime *rt, RJS_ResolveBinding *rb)
{
    rb->module = rjs_value_stack_push(rt);
    rb->name   = rjs_value_stack_push(rt);
}

/**
 * Release the resolve binding record.
 * \param rt The current runtime.
 * \param rb The resolve binding to be released.
 */
static inline void
rjs_resolve_binding_deinit (RJS_Runtime *rt, RJS_ResolveBinding *rb)
{
}

/**
 * Create a module from the file.
 * \param rt The current runtime.
 * \param[out] mod Return the new module.
 * \param filename The module's filename.
 * \param realm The realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_from_file (RJS_Runtime *rt, RJS_Value *mod, const char *filename, RJS_Realm *realm);

/**
 * Import the module dynamically.
 * \param rt The current runtime.
 * \param script The base script.
 * \param name The name of the module.
 * \param[out] promise Return the promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_import_dynamically (RJS_Runtime *rt, RJS_Value *script, RJS_Value *name, RJS_Value *promise);

/**
 * Disassemble the module.
 * \param rt The current runtime.
 * \param v The module value.
 * \param fp The output file.
 * \param flags The disassemble flags.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_disassemble (RJS_Runtime *rt, RJS_Value *v, FILE *fp, int flags);

/**
 * Resolve the export of the module.
 * \param rt The current runtime.
 * \param mod The module value.
 * \param name The export name.
 * \param[out] rb Return the resolve binding record.
 * \retval RJS_TRUE On success.
 * \retval RJS_FALSE Cannot find the export.
 * \retval RJS_AMBIGUOUS Find ambiguous export entries.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_resolve_export (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *name, RJS_ResolveBinding *rb);

/**
 * Get the module's namespace object.
 * \param rt The current runtime.
 * \param mod The module value.
 * \param[out] ns Return the namespace object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_get_namespace (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *ns);

/**
 * Get the module environment.
 * \param rt The current runtime.
 * \param modv The module value.
 * \return The module's environment.
 */
extern RJS_Environment*
rjs_module_get_env (RJS_Runtime *rt, RJS_Value *modv);

/**
 * Load the module's import meta object.
 * \param rt The current runtime.
 * \param modv The module value.
 * \param[out] v Return the import meta object.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_import_meta (RJS_Runtime *rt, RJS_Value *modv, RJS_Value *v);

/**
 * Resolve the imported module.
 * \param rt The current runtime.
 * \param script The base script.
 * \param name The name of the imported module.
 * \param[out] imod Return the imported module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_resolve_imported_module (RJS_Runtime *rt, RJS_Value *script, RJS_Value *name, RJS_Value *imod);

/**
 * Link the module.
 * \param rt The current runtime.
 * \param mod The module.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_link (RJS_Runtime *rt, RJS_Value *mod);

/**
 * Evaluate the module.
 * \param rt The current runtime.
 * \param mod The module.
 * \param[out] promise Return the evaluation promise.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_evaluate (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *promise);

/**
 * Load all the export values of the module to the object.
 * \param rt The current runtime.
 * \param mod The module.
 * \param o The object to store the export values.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_load_exports (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *o);

/**
 * Set the module's native data.
 * \param rt The current runtime.
 * \param mod The module.
 * \param data The data's pointer.
 * \param scan The function scan the referenced things in the data.
 * \param free The function free the data.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_set_data (RJS_Runtime *rt, RJS_Value *mod, void *data,
        RJS_ScanFunc scan, RJS_FreeFunc free);

/**
 * Get the native data's pointer of the module.
 * \param rt The current runtime.
 * \param mod The module.
 * \retval The native data's pointer.
 */
extern void*
rjs_module_get_data (RJS_Runtime *rt, RJS_Value *mod);

/**
 * Set the module's import and export entries.
 * This function must be invoked in native module's "ratjs_module_init" function.
 * \param rt The current runtime.
 * \param mod The module.
 * \param imports The import entries.
 * \param local_exports The local export entries.
 * \param indir_exports The indirect export entries.
 * \param star_exports The star export entries.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_set_import_export (RJS_Runtime *rt,
        RJS_Value *mod,
        const RJS_ModuleImportDesc *imports,
        const RJS_ModuleExportDesc *local_exports,
        const RJS_ModuleExportDesc *indir_exports,
        const RJS_ModuleExportDesc *star_exports);

/**
 * Get the binding's value from the module's environment.
 * \param rt The current runtime.
 * \param mod The module.
 * \param name The name of the binding.
 * \param[out] v Return the binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_get_binding (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *name, RJS_Value *v);

/**
 * Add a module binding.
 * This function must be invoked in native module's "ratjs_module_exec" function.
 * \param rt The current runtime.
 * \param mod The module.
 * \param name The name of the binding.
 * \param v The binding's value.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
extern RJS_Result
rjs_module_add_binding (RJS_Runtime *rt, RJS_Value *mod, RJS_Value *name, RJS_Value *v);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

