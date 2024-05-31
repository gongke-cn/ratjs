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
 * Module internal header.
 */

#ifndef _RJS_MODULE_INTERNAL_H_
#define _RJS_MODULE_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**Module request index.*/
typedef uint16_t RJS_ModuleRequestIndex;

#define RJS_INVALID_MODULE_REQUEST_INDEX 0xffff

/**Native module initialize function.*/
typedef RJS_Result (*RJS_ModuleInitFunc) (RJS_Runtime *rt, RJS_Value *m);
/**Native module exectue funciton.*/
typedef RJS_Result (*RJS_ModuleExecFunc) (RJS_Runtime *rt, RJS_Value *m);

/**Module request.*/
typedef struct {
    RJS_ValueIndex module_name_idx; /**< The module name's index.*/
    RJS_Value      module;          /**< The module.*/
} RJS_ModuleRequest;

/**Import entry.*/
typedef struct {
    RJS_ModuleRequestIndex module_request_idx; /**< The module request's index.*/
    RJS_ValueIndex         import_name_idx;    /**< The import name's index.*/
    RJS_ValueIndex         local_name_idx;     /**< The local name's index.*/
} RJS_ImportEntry;

/**Export entry.*/
typedef struct {
    RJS_HashEntry          he;                 /**< Hash table entry.*/
    RJS_ModuleRequestIndex module_request_idx; /**< The module request's index.*/
    RJS_ValueIndex         import_name_idx;    /**< The import name's index.*/
    RJS_ValueIndex         local_name_idx;     /**< The local name's index.*/
    RJS_ValueIndex         export_name_idx;    /**< The export name's index.*/
} RJS_ExportEntry;

/**Module status.*/
typedef enum {
    RJS_MODULE_STATUS_ALLOCATED,         /**< The module is allocated.*/
    RJS_MODULE_STATUS_LOADED,            /**< The module is loaded.*/
    RJS_MODULE_STATUS_LOADING_FAILED,    /**< Loading failed.*/
    RJS_MODULE_STATUS_LOADING_REQUESTED, /**< Loading the requested modules.*/
    RJS_MODULE_STATUS_UNLINKED,          /**< The module is unlinked.*/
    RJS_MODULE_STATUS_LINKING,           /**< In linking process.*/
    RJS_MODULE_STATUS_LINKED,            /**< The module is linked.*/
    RJS_MODULE_STATUS_EVALUATING,        /**< The module is evaluating.*/
    RJS_MODULE_STATUS_EVALUATING_ASYNC,  /**< The module is evaluating in async mod.*/
    RJS_MODULE_STATUS_EVALUATED          /**< The module is evaluated.*/
} RJS_ModuleStatus;

/**The async parent module.*/
typedef struct {
    RJS_List  ln;     /**< List node data.*/
    RJS_Value module; /**< The parent module.*/
} RJS_ModuleAsyncParent;

/**Module.*/
typedef struct {
    RJS_Script            script;                  /**< Base script data.*/
    RJS_ModuleStatus      status;                  /**< The module's status.*/
    RJS_Value             eval_error;              /**< Evaluate error.*/
    int                   eval_result;             /**< Evaluate result.*/
    int                   dfs_index;               /**< DFS index value.*/
    int                   dfs_ancestor_index;      /**< DFA ancestor index value.*/
#if ENABLE_ASYNC
    int                   async_eval;              /**< Evaluate in async mode.*/
    int                   pending_async;           /**< Pending on async counter.*/
    RJS_Value             cycle_root;              /**< The cycle root module.*/
    RJS_PromiseCapability capability;              /**< The promise capability for async evaluation.*/
    RJS_Value             promise;                 /**< Promise of the capability.*/
    RJS_Value             resolve;                 /**< Resovle function of the capability.*/
    RJS_Value             reject;                  /**< Reject function of the capability.*/
    RJS_List              async_parent_list;       /**< Async parent module list.*/
#endif /*ENABLE_ASYNC*/
    RJS_PromiseCapability top_level_capability;    /**< The top level promise capability.*/
    RJS_Value             top_promise;             /**< Promise of the top capability.*/
    RJS_Value             top_resolve;             /**< Resovle function of the top capability.*/
    RJS_Value             top_reject;              /**< Reject function of the top capability.*/
    RJS_List              ln;                      /**< List node data.*/
    RJS_List              star_ln;                 /**< List node data used for star export.*/
    RJS_HashEntry         he;                      /**< Hash table entries.*/
    RJS_ModuleRequest    *module_requests;         /**< Module request entries.*/
    size_t                module_request_num;      /**< Number of module request entries.*/
    RJS_ImportEntry      *import_entries;          /**< Import entries.*/
    size_t                import_entry_num;        /**< Number of import entries.*/
    RJS_ExportEntry      *export_entries;          /**< Export entries.*/
    RJS_Hash              export_hash;             /**< Export hasn table.*/
    size_t                local_export_entry_num;  /**< Number of local export entries.*/
    size_t                indir_export_entry_num;  /**< Number of indirect export entries.*/
    size_t                star_export_entry_num;   /**< Number of star export entries.*/
    RJS_Value             namespace;               /**< The module namespace.*/
    RJS_Environment      *env;                     /**< The module environment.*/
    RJS_Value             import_meta;             /**< Import meta value.*/
    RJS_NativeData        native_data;             /**< The native data.*/
#if ENABLE_NATIVE_MODULE
    void                 *native_handle;           /**< The native module's handle.*/
#endif /*ENABLE_NATIVE_MODULE*/
} RJS_Module;

/**
 * Create a new module.
 * \param rt The current runtime.
 * \param[out] v Return the module value.
 * \param id The module's indentifier.
 * \param realm The realm.
 * \return The new module.
 */
RJS_INTERNAL RJS_Module*
rjs_module_new (RJS_Runtime *rt, RJS_Value *v, const char *id, RJS_Realm *realm);

/**
 * Initialize the module data in the rt.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_runtime_module_init (RJS_Runtime *rt);

/**
 * Release the module data in the rt.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_runtime_module_deinit (RJS_Runtime *rt);

/**
 * Scan the module data in the rt.
 * \param rt The current runtime.
 */
RJS_INTERNAL void
rjs_gc_scan_module (RJS_Runtime *rt);

#ifdef __cplusplus
}
#endif

#endif

