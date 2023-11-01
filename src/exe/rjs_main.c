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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>
#include <ratjs.h>

/*JS rt.*/
static RJS_Runtime *rt           = NULL;
/*JS filename.*/
static const char  *js_filename  = NULL;

#if ENABLE_SCRIPT && ENABLE_MODULE
/*Load the script as module.*/
static RJS_Bool     module_mode  = RJS_TRUE;
#endif /*ENABLE_SCRIPT && ENABLE_MODULE*/

#if ENABLE_EVAL
/*Eval source string.*/
static char        *eval_code    = NULL;
#endif /*ENABLE_EVAL*/

/*Compile only*/
static RJS_Bool     compile_only = RJS_FALSE;

/*Run the code in strict mode.*/
static RJS_Bool     strict_mode  = RJS_FALSE;

/*Disassemble.*/
static int          disassemble  = 0;

#if ENABLE_MODULE

/**Module lookup directory.*/
typedef struct {
    RJS_List  ln;  /**< List node data.*/
    char     *dir; /**< The directory.*/
} ModuleDirectory;

/*Module lookup directory list.*/
static RJS_List     module_dir_list;
/*Module evaluation result.*/
static RJS_Result   module_eval_result;
/*Module evaluation return value.*/
static RJS_Value   *module_eval_rv;
#endif /*ENABLE_MODULE*/

/*Main function's result.*/
static RJS_Result   main_result;
/*Main function's retuen value.*/
static RJS_Value   *main_rv;

/*Show version information.*/
static void
show_version (void)
{
    fprintf(stdout, "%d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, MICRO_VERSION);
}

/*Show help message.*/
static void
show_help (char *cmd)
{
    fprintf(stdout, _(
            "Usage: %s [options] FILE [js_options]\n"
            "Options:\n"
#if ENABLE_SCRIPT && ENABLE_MODULE
            "  -s               load the file as script\n"
#endif /*ENABLE_SCRIPT && ENABLE_MODULE*/
#if ENABLE_MODULE
            "  -m DIR           add module lookup directory\n"
            "  -l MODULE        load a module and add its exports to global object\n"
#endif /*ENABLE_MODULE*/
#if ENABLE_SCRIPT
            "  -i FILE          include a script file\n"
#endif /*ENABLE_SCRIPT*/
            "  -c               compile only\n"
#if ENABLE_EVAL
            "  -e STRING        eval the string\n"
#endif /*ENABLE_EVAL*/
            "  --strict         run in strict mode\n"
            "  --dump-stack     dump stack when throwing an error\n"
            "  -d all|func|code|value|decl|binding|fdecl|prop|import|export\n"
            "                   disassemble\n"
            "        all:           output all information\n"
            "        func:          output function information\n"
            "        code:          output the byte code of the functions\n"
            "        value:         output the value table\n"
            "        decl:          output the declaration table\n"
            "        binding:       output the binding table\n"
            "        fdecl:         output the function declarations table\n"
            "        prop:          output the property table\n"
            "        import:        output the module's import entries\n"
            "        export:        output the module's export entries\n"
            "        penv:          output the private environments\n"
            "  --version        show version information\n"
            "  --help           show this help message\n"
            ), cmd);
}

#if ENABLE_MODULE

/*Check if the pathname is a relative pathname.*/
static RJS_Bool
is_rel_name (const char *name)
{
    if (name[0] == '.') {
        if ((name[1] == '.') && (name[2] == '/'))
            return RJS_TRUE;
        if (name[1] == '/')
            return RJS_TRUE;
    }

    return RJS_FALSE;
}

/*Check if the name is a absolute pathname.*/
static RJS_Bool
is_abs_name (const char *name)
{
    if (name[0] == '/')
        return RJS_TRUE;
    if (isalpha(name[0]) && (name[1] == ':'))
        return RJS_TRUE;

    return RJS_FALSE;
}

/*Check if the module exist.*/
static RJS_Result
module_exist (char *path, size_t size)
{
    struct stat sb;
    int         r;
    size_t      len, space;

    if ((r = stat(path, &sb)) != -1) {
        if ((sb.st_mode & S_IFMT) == S_IFREG)
            return RJS_OK;
    }

    len = strlen(path);
    if ((len >= 3) && !strcasecmp(path + len - 3, ".js"))
        return RJS_FALSE;
    if ((len >= 4) && !strcasecmp(path + len - 4, ".njs"))
        return RJS_FALSE;
    if ((len >= 5) && !strcasecmp(path + len - 5, ".json"))
        return RJS_FALSE;

    space = size - len;

    if (space > 4) {
        snprintf(path + len, size - len, ".njs");

        if ((r = stat(path, &sb)) != -1)
            return RJS_OK;
    }

    if (space > 3) {
        snprintf(path + len, size - len, ".js");

        if ((r = stat(path, &sb)) != -1)
            return RJS_OK;
    }
    
    if (space > 5) {
        snprintf(path + len, size - len, ".json");

        if ((r = stat(path, &sb)) != -1)
            return RJS_OK;
    }

    return RJS_FALSE;
}

/*Module pathname lookup function.*/
static RJS_Result
module_path_func (RJS_Runtime *rt, const char *base, const char *name,
        char *path, size_t size)
{
    if (base && is_rel_name(name)) {
        char  bpbuf[PATH_MAX];
        char *bpath;

        snprintf(bpbuf, sizeof(bpbuf), "%s", base);
        bpath = dirname(bpbuf);

        snprintf(path, size, "%s/%s", bpath, name);

        if (module_exist(path, size) == RJS_OK)
            return RJS_OK;
    } else if (is_abs_name(name)) {
        snprintf(path, size, "%s", name);

        if (module_exist(path, size) == RJS_OK)
            return RJS_OK;
    } else {
        ModuleDirectory *md;

        rjs_list_foreach_c(&module_dir_list, md, ModuleDirectory, ln) {
            snprintf(path, size, "%s/%s", md->dir, name);

            if (module_exist(path, size) == RJS_OK)
                return RJS_OK;
        }
    }

    return RJS_FALSE;
}

/*Initialize the module lookup directory.*/
static void
module_dir_list_init (void)
{
    rjs_list_init(&module_dir_list);

    rjs_set_module_path_func(rt, module_path_func);
}

/*Release the module lookup directory.*/
static void
module_dir_list_deinit (void)
{
    ModuleDirectory *md, *nmd;

    rjs_list_foreach_safe_c(&module_dir_list, md, nmd, ModuleDirectory, ln) {
        if (md->dir)
            free(md->dir);
        free(md);
    }
}

/*Add a module lookup directory.*/
static void
module_dir_add (const char *dir)
{
    ModuleDirectory *md;

    md = malloc(sizeof(ModuleDirectory));
    assert(md);

    md->dir = strdup(dir);

    rjs_list_append(&module_dir_list, &md->ln);
}

/*Function "addModuleDirectory"*/
static RJS_NF(addModuleDirectory)
{
    RJS_Value  *dir = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *str = rjs_value_stack_push(rt);
    const char *cstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, dir, str)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, str, NULL, NULL);

    module_dir_add(cstr);

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Add "addModuleDirectory" function.*/
static void
module_dir_function (void)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *name  = rjs_value_stack_push(rt);
    RJS_Value *func  = rjs_value_stack_push(rt);
    RJS_Realm *realm = rjs_realm_current(rt);
    RJS_PropertyName pn;
    RJS_Value *global;

    rjs_string_from_chars(rt, name, "addModuleDirectory", -1);
    rjs_create_builtin_function(rt, NULL, addModuleDirectory, 1, name, realm, NULL, NULL, func);

    global = rjs_global_object(realm);

    rjs_property_name_init(rt, &pn, name);
    rjs_create_data_property_or_throw(rt, global, &pn, func);
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);
}

/*Module evaluation ok callback.*/
static RJS_NF(on_module_eval_ok)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);

    rjs_value_copy(rt, module_eval_rv, arg);
    rjs_value_set_undefined(rt, rv);
    module_eval_result = RJS_OK;
    return RJS_OK;
}

/*Module evaluation error callback.*/
static RJS_NF(on_module_eval_error)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);

    rjs_value_copy(rt, module_eval_rv, arg);
    rjs_value_set_undefined(rt, rv);
    module_eval_result = RJS_ERR;
    return RJS_OK;
}

/*Load a module's exports to the global object.*/
static RJS_Result
module_load (const char *name)
{
    size_t     top     = rjs_value_stack_save(rt);
    RJS_Value *str     = rjs_value_stack_push(rt);
    RJS_Value *mod     = rjs_value_stack_push(rt);
    RJS_Value *promise = rjs_value_stack_push(rt);
    RJS_Value *res     = rjs_value_stack_push(rt);
    RJS_Realm *realm   = rjs_realm_current(rt);
    RJS_Result r;

    rjs_string_from_enc_chars(rt, str, name, -1, NULL);

    if ((r = rjs_resolve_imported_module(rt, NULL, str, mod)) == RJS_ERR)
        goto end;

    if ((r = rjs_module_link(rt, mod)) == RJS_ERR)
        goto end;

    module_eval_result = 0;
    module_eval_rv     = res;

    if ((r = rjs_module_evaluate(rt, mod, promise)) == RJS_ERR)
        goto end;

    if ((r = rjs_promise_then_native(rt, promise, on_module_eval_ok, on_module_eval_error, NULL)) == RJS_ERR)
        goto end;

    while (module_eval_result == 0)
        rjs_solve_jobs(rt);

    if (module_eval_result == RJS_ERR) {
        rjs_throw(rt, res);
        r = RJS_ERR;
        goto end;
    }

    r = rjs_module_load_exports(rt, mod, rjs_global_object(realm));
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_MODULE*/

/*Dump the error message.*/
static void
dump_error (void)
{
    RJS_Value *err = rjs_value_stack_push(rt);
    RJS_Value *str = rjs_value_stack_push(rt);
    RJS_Result r;

    if (rjs_catch(rt, err)) {
        rjs_dump_error_stack(rt, stderr);

        if ((r = rjs_to_string(rt, err, str)) == RJS_OK) {
            fprintf(stderr, "throw: %s\n", rjs_string_to_enc_chars(rt, str, NULL, NULL));
        }
    }
}

/*Parse options.*/
static RJS_Result
parse_options (int argc, char **argv)
{
    while (1) {
        int        c;
        int        opt_idx;
        RJS_Result r;

        enum {
            OPT_HELP = 256,
            OPT_STRICT,
            OPT_VERSION,
            OPT_DUMP_STACK
        };

        static const char *short_opts =
                "+"
#if ENABLE_SCRIPT && ENABLE_MODULE
                "s"
#endif /*ENABLE_SCRIPT && ENABLE_MODULE*/
#if ENABLE_MODULE
                "l:m:"
#endif /*ENABLE_MODULE*/
#if ENABLE_SCRIPT
                "i:"
#endif /*ENABLE_SCRIPT*/
#if ENABLE_EVAL
                "e:"
#endif /*ENABLE_EVAL*/
                "cd:"
                ;
        static const struct option long_opts[] = {
            {"strict",  no_argument,   0,  OPT_STRICT},
            {"version", no_argument,   0,  OPT_VERSION},
            {"help",    no_argument,   0,  OPT_HELP},
            {"dump-stack", no_argument,0,  OPT_DUMP_STACK},
            {0,         0,             0,  0}
        };

        c = getopt_long(argc, argv, short_opts, long_opts, &opt_idx);
        if (c == -1)
            break;

        switch (c) {
#if ENABLE_SCRIPT && ENABLE_MODULE
        case 's':
            module_mode = RJS_FALSE;
            break;
#endif /*ENABLE_SCRIPT && ENABLE_MODULE*/
#if ENABLE_MODULE
        case 'm':
            module_dir_add(optarg);
            break;
        case 'l':
            if ((r = module_load(optarg)) == RJS_ERR) {
                dump_error();
                return r;
            }
            break;
#endif /*ENABLE_MODULE*/
#if ENABLE_SCRIPT
        case 'i': {
            size_t      top    = rjs_value_stack_save(rt);
            RJS_Value  *script = rjs_value_stack_push(rt);
            RJS_Realm  *realm  = rjs_realm_current(rt);

            if ((r = rjs_script_from_file(rt, script, optarg, realm, RJS_FALSE)) == RJS_ERR)
                return r;

            if ((r = rjs_script_evaluation(rt, script, NULL)) == RJS_ERR) {
                dump_error();
                return r;
            }

            rjs_value_stack_restore(rt, top);
            break;
        }
#endif /*ENABLE_SCRIPT*/
        case 'c':
            compile_only = RJS_TRUE;
            break;
#if ENABLE_EVAL
        case 'e':
            eval_code = optarg;
            break;
#endif /*ENABLE_EVAL*/
        case 'd':
            if (!strcmp(optarg, "all"))
                disassemble |= RJS_DISASSEMBLE_ALL;
            else if (!strcmp(optarg, "func"))
                disassemble |= RJS_DISASSEMBLE_FUNC;
            else if (!strcmp(optarg, "code"))
                disassemble |= RJS_DISASSEMBLE_FUNC|RJS_DISASSEMBLE_CODE;
            else if (!strcmp(optarg, "value"))
                disassemble |= RJS_DISASSEMBLE_VALUE;
            else if (!strcmp(optarg, "decl"))
                disassemble |= RJS_DISASSEMBLE_DECL;
            else if (!strcmp(optarg, "binding"))
                disassemble |= RJS_DISASSEMBLE_BINDING;
            else if (!strcmp(optarg, "fdecl"))
                disassemble |= RJS_DISASSEMBLE_FUNC_DECL;
            else if (!strcmp(optarg, "prop"))
                disassemble |= RJS_DISASSEMBLE_FUNC|RJS_DISASSEMBLE_PROP_REF;
            else if (!strcmp(optarg, "import"))
                disassemble |= RJS_DISASSEMBLE_IMPORT;
            else if (!strcmp(optarg, "export"))
                disassemble |= RJS_DISASSEMBLE_EXPORT;
            else if (!strcmp(optarg, "penv"))
                disassemble |= RJS_DISASSEMBLE_PRIV_ENV;
            break;
        case OPT_DUMP_STACK:
            rjs_set_throw_dump(rt, RJS_TRUE);
            break;
        case OPT_STRICT:
            strict_mode = RJS_TRUE;
            break;
        case OPT_VERSION:
            show_version();
            break;
        case OPT_HELP:
            show_help(argv[0]);
            break;
        case '?':
        default:
            return RJS_ERR;
        }
    }

    if (optind < argc)
        js_filename = argv[optind];

    return RJS_OK;
}

/*Main function ok callback.*/
static RJS_NF(on_main_ok)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);

    rjs_value_copy(rt, main_rv, arg);
    rjs_value_set_undefined(rt, rv);
    main_result = RJS_OK;
    return RJS_OK;
}

/*Main function error callback.*/
static RJS_NF(on_main_error)
{
    RJS_Value *arg = rjs_argument_get(rt, args, argc, 0);

    rjs_value_copy(rt, main_rv, arg);
    rjs_value_set_undefined(rt, rv);
    main_result = RJS_ERR;
    return RJS_OK;
}

/**
 * Main function of ratjs.
 */
int
main (int argc, char **argv)
{
    RJS_Result       r;
    char            *ev;
    int              ec = 1;
    RJS_Realm       *realm;
    RJS_Value       *exec, *main_str, *main_fn, *rv;
    RJS_PropertyName main_pn;

    /*Set log output level.*/
    if ((ev = getenv("RJS_LOG_LEVEL"))) {
        RJS_LogLevel level = RJS_LOG_ALL;

        if (!strcasecmp(ev, "debug"))
            level = RJS_LOG_DEBUG;
        else if (!strcasecmp(ev, "info"))
            level = RJS_LOG_INFO;
        else if (!strcasecmp(ev, "warning"))
            level = RJS_LOG_WARNING;
        else if (!strcasecmp(ev, "error"))
            level = RJS_LOG_ERROR;
        else if (!strcasecmp(ev, "fatal"))
            level = RJS_LOG_FATAL;
        else if (!strcasecmp(ev, "none"))
            level = RJS_LOG_NONE;

        rjs_log_set_level(level);
    }

    /*Create the runtime.*/
    rt = rjs_runtime_new();

#if ENABLE_EXTENSION
    /*Load extension functions.*/
    rjs_realm_load_extension(rt, NULL);
#endif /*ENABLE_EXTENSION*/

    exec     = rjs_value_stack_push(rt);
    main_fn  = rjs_value_stack_push(rt);
    main_str = rjs_value_stack_push(rt);
    rv       = rjs_value_stack_push(rt);

    rjs_string_from_chars(rt, main_str, "main", -1);
    rjs_property_name_init(rt, &main_pn, main_str);

#if ENABLE_MODULE
    module_dir_list_init();

    module_dir_function();
#endif /*ENABLE_MODULE*/

    /*Parse options.*/
    if ((r = parse_options(argc, argv)) == RJS_ERR)
        goto end;

    realm = rjs_realm_current(rt);

#if ENABLE_EVAL
    if (eval_code) {
        RJS_Value *eval_str = rjs_value_stack_push(rt);

        rjs_string_from_enc_chars(rt, eval_str, eval_code, -1, NULL);

        r = rjs_eval_from_string(rt, exec, eval_str, realm, strict_mode, RJS_FALSE);
        if (r == RJS_ERR) {
            dump_error();
            goto end;
        }

        if (r == RJS_OK) {
            /*Disassemble.*/
            if (disassemble)
                rjs_script_disassemble(rt, exec, stdout, disassemble);

            /*Run the script.*/
            if (!compile_only) {
                if ((r = rjs_eval_evaluation(rt, exec, RJS_FALSE, rv)) == RJS_ERR) {
                    dump_error();
                    goto end;
                }

                if ((r = rjs_get(rt, rjs_global_object(realm), &main_pn, main_fn)) == RJS_ERR) {
                    dump_error();
                    goto end;
                }
            }
        }
    } else
#endif /*ENABLE_EVAL*/

    /*Run the script.*/
    if (js_filename) {
#if ENABLE_SCRIPT && ENABLE_MODULE
        if (module_mode)
#endif /*ENABLE_SCRIPT && ENABLE_MODULE*/
#if ENABLE_MODULE
        {
            /*Load the module.*/
            if ((r = rjs_module_from_file(rt, exec, js_filename, realm)) == RJS_ERR)
                goto end;

            /*Disassemble.*/
            if (disassemble)
                rjs_module_disassemble(rt, exec, stdout, disassemble);

            /*Run the module.*/
            if (!compile_only) {
                RJS_Environment *env;
                RJS_BindingName  bn;
                RJS_Value       *promise;

                /*Link the module.*/
                if ((r = rjs_module_link(rt, exec)) == RJS_ERR) {
                    dump_error();
                    goto end;
                }

                /*Evaluate the module.*/
                promise = rjs_value_stack_push(rt);

                module_eval_result = 0;
                module_eval_rv     = rv;

                if ((r = rjs_module_evaluate(rt, exec, promise)) == RJS_ERR) {
                    dump_error();
                    goto end;
                }

                /*Wait the evaluation end.*/
                if ((r = rjs_promise_then_native(rt, promise, on_module_eval_ok,
                        on_module_eval_error, NULL)) == RJS_ERR) {
                    dump_error();
                    goto end;
                }

                while (module_eval_result == 0)
                    rjs_solve_jobs(rt);

                if (module_eval_result == RJS_ERR) {
                    rjs_throw(rt, rv);
                    dump_error();
                    goto end;
                }

                /*Get "main" binding.*/
                env = rjs_module_get_env(rt, exec);

                rjs_binding_name_init(rt, &bn, main_str);
                r = rjs_env_has_binding(rt, env, &bn);
                if (r == RJS_OK) {
                    r = rjs_env_get_binding_value(rt, env, &bn, RJS_TRUE, main_fn);
                }
                rjs_binding_name_deinit(rt, &bn);

                if (r == RJS_ERR) {
                    dump_error();
                    goto end;
                }
            }
        }
#endif /*ENABLE_MODULE*/
#if ENABLE_SCRIPT && ENABLE_MODULE
        else
#endif /*ENABLE_SCRIPT && ENABLE_MODULE*/
#if ENABLE_SCRIPT
        {
            /*Load the script.*/
            if ((r = rjs_script_from_file(rt, exec, js_filename, realm, strict_mode)) == RJS_ERR)
                goto end;

            /*Disassemble.*/
            if (disassemble)
                rjs_script_disassemble(rt, exec, stdout, disassemble);

            /*Run the script.*/
            if (!compile_only) {
                if ((r = rjs_script_evaluation(rt, exec, NULL)) == RJS_ERR) {
                    dump_error();
                    goto end;
                }

                if ((r = rjs_get(rt, rjs_global_object(realm), &main_pn, main_fn)) == RJS_ERR) {
                    dump_error();
                    goto end;
                }
            }
        }
#endif /*ENABLE_SCRIPT*/
    } else {
        ec = 0;
    }

    if (rjs_is_callable(rt, main_fn)) {
        RJS_Value *js_args = NULL;
        size_t     js_argc = 0;
        RJS_Value *str     = rjs_value_stack_push(rt);
        size_t     i, aid;

        js_argc = argc - optind;
        js_args = rjs_value_stack_push_n(rt, js_argc);

        for (i = 0, aid = optind; i < js_argc; i ++, aid ++) {
            RJS_Value *arg = rjs_value_buffer_item(rt, js_args, i);

            rjs_string_from_enc_chars(rt, str, argv[aid], -1, NULL);
            rjs_value_copy(rt, arg, str);
        }

        if ((r = rjs_call(rt, main_fn, rjs_v_undefined(rt), js_args, js_argc, rv)) == RJS_ERR) {
            dump_error();
            goto end;
        }

        /*Wait main function.*/
        if (rjs_value_is_promise(rt, rv)) {
            RJS_Value *promise = rjs_value_stack_push(rt);

            rjs_value_copy(rt, promise, rv);

            main_result = 0;
            main_rv     = rv;

            if ((r = rjs_promise_then_native(rt, promise, on_main_ok, on_main_error, NULL)) == RJS_ERR) {
                dump_error();
                goto end;
            }

            while (main_result == 0)
                rjs_solve_jobs(rt);

            if (main_result == RJS_ERR) {
                rjs_throw(rt, rv);
                dump_error();
                goto end;
            }
        }
    }

    if (rjs_value_is_number(rt, rv))
        ec = rjs_value_get_number(rt, rv);
    else
        ec = 0;

    rjs_solve_jobs(rt);
    dump_error();
end:
    /*Release property name "main".*/
    rjs_property_name_deinit(rt, &main_pn);

    /*Free the rt.*/
    rjs_runtime_free(rt);

    /*Free the module lookup directories list.*/
#if ENABLE_MODULE
    module_dir_list_deinit();
#endif /*ENABLE_MODULE*/
    
    return ec;
}
