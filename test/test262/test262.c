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

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ratjs.h>
#include <yaml.h>

/*Feature entry.*/
typedef struct FeatureEntry_s FeatureEntry;
/*Feature entry.*/
struct FeatureEntry_s {
    FeatureEntry *next;
    char         *name;
};

/*Test entry.*/
typedef struct TestEntry_s TestEntry;
/*Test entry.*/
struct TestEntry_s {
    RJS_List  ln;
    char     *name;
};

/*Test files list.*/
static RJS_List test_list;
/*Base directory name.*/
static char *base_dir   = NULL;
/*Skip features.*/
static FeatureEntry *skip_features = NULL;
/*Test cases number.*/
static int   case_num   = 0;
/*Failed cases number.*/
static int   failed_num = 0;
/*Negative flag.*/
static int   meta_negative = 0;
/*Error phase.*/
static enum {
    PHASE_UNKNONW,
    PHASE_PARSE,
    PHASE_RESOLUTION,
    PHASE_RUNTIME
} meta_error_phase = PHASE_RUNTIME;
/*Error type.*/
static char *meta_error_type = NULL;
/*Includes.*/
static char *meta_includes[256];
/*Skip the cast.*/
static int   meta_skip = 0;

#define FLAG_ONLY_STRICT     1
#define FLAG_NO_STRICT       2
#define FLAG_MODULE          4
#define FLAG_RAW             8
#define FLAG_ASYNC           16
#define FLAG_CAN_BLOCK_TRUE  32
#define FLAG_CAN_BLOCK_FALSE 64
/*Flags.*/
static int   meta_flags = 0;

/*Run mode.*/
typedef enum {
    RUN_MODE_NO_STRICT,
    RUN_MODE_STRICT,
    RUN_MODE_MODULE
} RunMode;

/*Initialize the host resource.*/
extern void host_init (void);

/*Release the host resource.*/
extern void host_deinit (void);

/*Async wait.*/
extern int async_wait (RJS_Runtime *rt);

/*Load host defined functions.*/
extern int load_host_functions (RJS_Runtime *rt, RJS_Realm *realm);

/*Check if the directory is the test base directory.*/
static int
is_base_dir (char *dir)
{
    char        tmp[PATH_MAX];
    struct stat sb;

    snprintf(tmp, sizeof(tmp), "%s/test", dir);
    if ((stat(tmp, &sb) == -1) || ((sb.st_mode & S_IFMT) != S_IFDIR))
        return 0;

    snprintf(tmp, sizeof(tmp), "%s/harness", dir);
    if ((stat(tmp, &sb) == -1) || ((sb.st_mode & S_IFMT) != S_IFDIR))
        return 0;

    return 1;
}

/*Find the base directory.*/
static char*
find_base_dir (char *buf, size_t len, char *dir)
{
    char  tmp[PATH_MAX];
    char *pdir;

    if (is_base_dir(dir)) {
        strcpy(buf, dir);
        return buf;
    }

    strcpy(tmp, dir);

    pdir = dirname(tmp);

    if (!strcmp(dir, pdir)) {
        fprintf(stderr, "cannot find the test262 base directory\n");
        return NULL;
    }

    return find_base_dir(buf, len, pdir);
}

/*Clear the meta data.*/
static void
clear_meta (void)
{
    char **pi;

    meta_negative    = 0;
    meta_error_phase = PHASE_UNKNONW;
    meta_skip        = 0;
    meta_flags       = 0;

    if (meta_error_type) {
        free(meta_error_type);
        meta_error_type = NULL;
    }

    pi = meta_includes;
    while (*pi) {
        free(*pi);
        *pi = NULL;
        pi ++;
    }
}

/*Lookup the node.*/
static yaml_node_t*
meta_lookup (yaml_document_t *doc, yaml_node_t *node, char *tag)
{
    yaml_node_pair_t *pair;

    if (!node)
        node = yaml_document_get_root_node(doc);

    for (pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair ++) {
        yaml_node_t *key;

        key = yaml_document_get_node(doc, pair->key);
        if (!strcmp((char*)key->data.scalar.value, tag)) {
            return yaml_document_get_node(doc, pair->value);
        }
    }

    return NULL;
}

/*Load meta data from the document.*/
static void
load_meta_from_doc (yaml_document_t *doc)
{
    yaml_node_t *n;

    n = meta_lookup(doc, NULL, "negative");
    if (n) {
        yaml_node_t *cn;

        meta_negative = 1;

        cn = meta_lookup(doc, n, "phase");
        if (cn) {
            if (!strcmp((char*)cn->data.scalar.value, "parse"))
                meta_error_phase = PHASE_PARSE;
            else if (!strcmp((char*)cn->data.scalar.value, "resolution"))
                meta_error_phase = PHASE_RESOLUTION;
            else if (!strcmp((char*)cn->data.scalar.value, "runtime"))
                meta_error_phase = PHASE_RUNTIME;
        }

        cn = meta_lookup(doc, n, "type");
        if (cn) {
            meta_error_type = strdup((char*)cn->data.scalar.value);
        }
    }

    n = meta_lookup(doc, NULL, "includes");
    if (n) {
        yaml_node_item_t *item;
        char            **pi = meta_includes;

        for (item = n->data.sequence.items.start; item < n->data.sequence.items.top; item ++) {
            yaml_node_t *f;

            f = yaml_document_get_node(doc, *item);

            *pi = strdup((char*)f->data.scalar.value);
            pi ++;
        }
    }

    n = meta_lookup(doc, NULL, "features");
    if (n) {
        yaml_node_item_t *item;

        for (item = n->data.sequence.items.start; item < n->data.sequence.items.top; item ++) {
            yaml_node_t  *f;
            char         *str;
            FeatureEntry *fe;

            f = yaml_document_get_node(doc, *item);

            str = (char*)f->data.scalar.value;

            for (fe = skip_features; fe; fe = fe->next) {
                if (!strcmp(fe->name, str))
                    meta_skip = 1;
            }
        }
    }

    n = meta_lookup(doc, NULL, "flags");
    if (n) {
        yaml_node_item_t *item;

        for (item = n->data.sequence.items.start; item < n->data.sequence.items.top; item ++) {
            yaml_node_t *f;
            char        *str;

            f = yaml_document_get_node(doc, *item);

            str = (char*)f->data.scalar.value;

            if (!strcmp(str, "onlyStrict"))
                meta_flags |= FLAG_ONLY_STRICT;
            else if (!strcmp(str, "noStrict"))
                meta_flags |= FLAG_NO_STRICT;
            else if (!strcmp(str, "module"))
                meta_flags |= FLAG_MODULE;
            else if (!strcmp(str, "raw"))
                meta_flags |= FLAG_RAW;
            else if (!strcmp(str, "async"))
                meta_flags |= FLAG_ASYNC;
            else if (!strcmp(str, "CanBlockIsFalse"))
                meta_flags |= FLAG_CAN_BLOCK_FALSE;
            else if (!strcmp(str, "CanBlockIsTrue"))
                meta_flags |= FLAG_CAN_BLOCK_TRUE;
        }
    }
}

/*Load meta data.*/
static int
load_meta (char *test)
{
    FILE          *fp       = fopen(test, "r");
    int            r        = -1;
    unsigned char *yaml_buf = NULL;
    size_t         yaml_len = 0;
    size_t         yaml_cap = 0;
    char           buf[256];

    if (!fp) {
        fprintf(stderr, "cannot open \"%s\"\n", test);
        return -1;
    }

    while (1) {
        char *s;

        s = fgets(buf, sizeof(buf), fp);
        if (!s) {
            fprintf(stderr, "cannot find meta data in \"%s\"\n", test);
            break;
        }

        if (strstr(s, "/*---")) {
            while (1) {
                size_t slen;

                s = fgets(buf, sizeof(buf), fp);
                if (!s) {
                    fprintf(stderr, "unterminated meta block in \"%s\"\n", test);
                    goto end;
                }

                if (strstr(s, "---*/"))
                    goto end;

                slen = strlen(s);
                if (slen + yaml_len + 1 > yaml_cap) {
                    yaml_cap = RJS_MAX(yaml_cap * 2, slen + yaml_len + 1);
                    yaml_buf = realloc(yaml_buf, yaml_cap);
                }

                memcpy(yaml_buf + yaml_len, s, slen);
                yaml_len += slen;

            }
        }
    }

end:
    if (yaml_buf) {
        yaml_parser_t   parser;
        yaml_document_t doc;

        yaml_buf[yaml_len] = 0;

        yaml_parser_initialize(&parser);
        yaml_parser_set_input_string(&parser, yaml_buf, yaml_len);

        if (yaml_parser_load(&parser, &doc) == 0) {
            fprintf(stderr, "parse the YAML failed\n");
        } else {
            load_meta_from_doc(&doc);
            yaml_document_delete(&doc);
            r = 0;
        }

        yaml_parser_delete(&parser);

        free(yaml_buf);
    }

    fclose(fp);
    return r;
}

/*Load the include file.*/
static int
load_include (RJS_Runtime *rt, char *file)
{
    char       path[PATH_MAX];
    int        r;
    RJS_Realm *realm  = rjs_realm_current(rt);
    RJS_Value *script = rjs_value_stack_push(rt);

    snprintf(path, sizeof(path), "%s/harness/%s", base_dir, file);

    if ((r = rjs_script_from_file(rt, script, path, realm, RJS_FALSE)) == -1)
        goto end;
    if ((r = rjs_script_evaluation(rt, script, NULL)) == -1)
        goto end;

    r = 0;
end:
    if (r == -1)
        fprintf(stderr, "load \"%s\" failed\n", file);
    else
        RJS_LOGD("load \"%s\"", file);

    return r;
}

/*Failed on error.*/
static void
failed_on_error (RJS_Runtime *rt)
{
    RJS_Value *err = rjs_value_stack_push(rt);
    RJS_Value *str = rjs_value_stack_push(rt);

    rjs_catch(rt, err);
    rjs_to_string(rt, err, str);

    fprintf(stderr, "throw: %s\n", rjs_string_to_enc_chars(rt, str, NULL, NULL));
    rjs_dump_error_stack(rt, stderr);
}

/*Negative error check.*/
static int
negative_check (RJS_Runtime *rt, int r)
{
    RJS_Value       *err, *constr, *constr_str, *name, *name_str;
    const char      *nstr;
    RJS_PropertyName constr_pn, name_pn;

    if (r != -1) {
        fprintf(stderr, "expect an error\n");
        return -1;
    }

    if (meta_error_phase == PHASE_PARSE) {
        if (strcmp(meta_error_type, "SyntaxError")) {
            fprintf(stderr, "expect a SyntaxError\n");
            return -1;
        }

        return 0;
    }

    err = rjs_value_stack_push(rt);

    if (rjs_catch(rt, err) != RJS_OK) {
        fprintf(stderr, "cannot catch the error\n");
        return -1;
    }

    constr_str = rjs_value_stack_push(rt);
    rjs_string_from_chars(rt, constr_str, "constructor", -1);
    rjs_property_name_init(rt, &constr_pn, constr_str);

    constr = rjs_value_stack_push(rt);
    r = rjs_get_v(rt, err, &constr_pn, constr);
    rjs_property_name_deinit(rt, &constr_pn);
    if (r == RJS_ERR) {
        fprintf(stderr, "cannot get the error's constructor\n");
        return -1;
    }

    name_str = rjs_value_stack_push(rt);
    rjs_string_from_chars(rt, name_str, "name", -1);
    rjs_property_name_init(rt, &name_pn, name_str);

    name = rjs_value_stack_push(rt);
    if (rjs_value_is_undefined(rt, constr)) {
        rjs_get(rt, err, &name_pn, name);
    } else {
        rjs_get(rt, constr, &name_pn, name);
    }

    rjs_property_name_deinit(rt, &name_pn);

    nstr = rjs_string_to_enc_chars(rt, name, NULL, NULL);
    if (strcmp(nstr, meta_error_type)) {
        fprintf(stderr, "expect %s, but get %s\n", meta_error_type, nstr);
        return -1;
    }

    return 0;
}

#if ENABLE_MODULE

/*Check if the name is a relative name.*/
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

/*Module path lookup function.*/
static RJS_Result
module_path_func (RJS_Runtime *rt, const char *base,
        const char *name, char *path, size_t size)
{
    struct stat sb;
    int         r;

    if (base && is_rel_name(name)) {
        char  bpbuf[PATH_MAX];
        char *bpath;

        snprintf(bpbuf, sizeof(bpbuf), "%s", base);
        bpath = dirname(bpbuf);

        snprintf(path, size, "%s/%s", bpath, name);

        if ((r = stat(path, &sb)) != -1)
            return RJS_OK;
    } else {
        if ((r = stat(name, &sb)) != -1)
            return RJS_OK;
    }

    return RJS_FALSE;
}

#endif /*ENABLE_MODULE*/

/*Run the case once.*/
static int
run_case_once (char *test, RunMode mode)
{
    RJS_Runtime *rt;
    RJS_Realm   *realm;
    RJS_Value   *exec;
    char       **pi;
    int          r;

    rt    = rjs_runtime_new();
    realm = rjs_realm_current(rt);

#if ENABLE_MODULE
    rjs_set_module_path_func(rt, module_path_func);
#endif /*ENABLE_MODULE*/

    if (meta_flags & FLAG_CAN_BLOCK_TRUE)
        rjs_set_agent_can_block(rt, RJS_TRUE);
    else if (meta_flags & FLAG_CAN_BLOCK_FALSE)
        rjs_set_agent_can_block(rt, RJS_FALSE);

    host_init();
    load_host_functions(rt, realm);

    exec = rjs_value_stack_push(rt);

    case_num ++;

    if (!(meta_flags & FLAG_RAW)) {
        if ((r = load_include(rt, "assert.js")) == -1)
            goto end;
        if ((r = load_include(rt, "sta.js")) == -1)
            goto end;
    }

    if (meta_flags & FLAG_ASYNC) {
        if ((r = load_include(rt, "doneprintHandle.js")) == -1)
            goto end;
    }

    /*Load includes.*/
    pi = meta_includes;
    while (*pi) {
        if ((r = load_include(rt, *pi)) == -1)
            goto end;
        pi ++;
    }

    /*Run the test.*/
    RJS_LOGD("run \"%s\" %s",
            test,
            (mode == RUN_MODE_STRICT) ? "in strict mode"
            : ((mode == RUN_MODE_MODULE) ? "as module" : ""));

    switch (mode) {
    case RUN_MODE_NO_STRICT:
        r = rjs_script_from_file(rt, exec, test, realm, RJS_FALSE);
        if (meta_negative && (meta_error_phase == PHASE_PARSE)) {
            r = negative_check(rt, r);
            goto end;
        } else if (r == -1) {
            fprintf(stderr, "parse error\n");
            goto end;
        }

        r = rjs_script_evaluation(rt, exec, NULL);
        if (meta_negative && (meta_error_phase == PHASE_RUNTIME)) {
            r = negative_check(rt, r);
            goto end;
        } else if (r == -1) {
            failed_on_error(rt);
            goto end;
        }
        break;
    case RUN_MODE_STRICT:
        r = rjs_script_from_file(rt, exec, test, realm, RJS_TRUE);
        if (meta_negative && (meta_error_phase == PHASE_PARSE)) {
            r = negative_check(rt, r);
            goto end;
        } else if (r == -1) {
            fprintf(stderr, "parse error\n");
            goto end;
        }

        r = rjs_script_evaluation(rt, exec, NULL);
        if (meta_negative && (meta_error_phase == PHASE_RUNTIME)) {
            r = negative_check(rt, r);
            goto end;
        } else if (r == -1) {
            failed_on_error(rt);
            goto end;
        }
        break;
    case RUN_MODE_MODULE:
#if ENABLE_MODULE
        r = rjs_module_from_file(rt, exec, test, realm);
        if (meta_negative && (meta_error_phase == PHASE_PARSE)) {
            r = negative_check(rt, r);
            goto end;
        } else if (r == -1) {
            fprintf(stderr, "parse error\n");
            goto end;
        }

        r = rjs_module_link(rt, exec);
        if (meta_negative
                && (r == -1)
                && ((meta_error_phase == PHASE_RESOLUTION) || (meta_error_phase == PHASE_RUNTIME))) {
            r = negative_check(rt, r);
            goto end;
        } else if (r == -1) {
            failed_on_error(rt);
            goto end;
        }

        r = rjs_module_evaluate(rt, exec, NULL);
        if (meta_negative && (meta_error_phase == PHASE_RUNTIME)) {
            r = negative_check(rt, r);
            goto end;
        } else if (r == -1) {
            failed_on_error(rt);
            goto end;
        }
#endif /*ENABLE_MODULE*/
        break;
    }

    if (meta_flags & FLAG_ASYNC) {
        if ((r = async_wait(rt)) == -1)
            goto end;
    }

    r = 0;
end:
    if (r == -1)
        failed_num ++;

    fprintf(stderr, "\"%s\" test%s%s\n",
            test,
            (mode == RUN_MODE_STRICT) ? " in strict mode " : " ",
            (r == 0) ? "ok" : "failed");

    host_deinit();
    rjs_runtime_free(rt);
    return r;
}

/*Run a test case.*/
static int
run_case (char *test)
{
    int r = 0;

    printf("prepare \"%s\"\n", test);

    clear_meta();

    if (load_meta(test) == -1)
        return 0;

    if (meta_skip)
        return 0;

    if (!(meta_flags & (FLAG_ONLY_STRICT|FLAG_MODULE))) {
        r |= run_case_once(test, RUN_MODE_NO_STRICT);
    }

    if (meta_flags & FLAG_MODULE) {
        r |= run_case_once(test, RUN_MODE_MODULE);
    } else if (!(meta_flags & (FLAG_NO_STRICT|FLAG_RAW))) {
        r |= run_case_once(test, RUN_MODE_STRICT);
    }

    return r;
}

/*Run the test.*/
static int
run_test (char *test)
{
    struct stat sb;
    int         r;
    int         rc = 0;

    if (stat(test, &sb) == -1) {
        fprintf(stderr, "cannot find \"%s\"\n", test);
        return -1;
    }

    if ((sb.st_mode & S_IFMT) == S_IFDIR) {
        DIR           *dir;
        struct dirent *de;

        if (!(dir = opendir(test))) {
            fprintf(stderr, "cannot open directory \"%s\"\n", test);
            return -1;
        }

        while (1) {
            char   buf[PATH_MAX];
            size_t len;

            de = readdir(dir);
            if (!de)
                break;

            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            snprintf(buf, sizeof(buf), "%s/%s", test, de->d_name);

            r = stat(buf, &sb);
            if (r == 0) {
                if (S_ISDIR(sb.st_mode)) {
                    rc |= run_test(buf);
                } else if (S_ISREG(sb.st_mode)) {
                    len = strlen(buf);

                    if ((len > 3) && !(strcasecmp(buf + len - 3, ".js")))
                        rc |= run_test(buf);
                }
            }
        }

        closedir(dir);
    } else {
        rc |= run_case(test);
    }

    return rc;
}

/*Add skip feature.*/
static void
add_skip (char *name)
{
    FeatureEntry *fe;

    fe = malloc(sizeof(FeatureEntry));
    fe->name = strdup(name);
    fe->next = skip_features;
    skip_features = fe;
}

/*Add a test entry.*/
static void
add_test (char *name)
{
    TestEntry *te;

    te = malloc(sizeof(TestEntry));
    te->name = strdup(name);

    rjs_list_append(&test_list, &te->ln);
}

int
main (int argc, char **argv)
{
    char        base_dir_buf[PATH_MAX];
    char        test_file_buf[PATH_MAX];
    char        real_path_buf[PATH_MAX];
    char       *rpath;
    TestEntry  *test;
    struct stat sb;

    add_skip("array-grouping");
    add_skip("regexp-v-flag");
    add_skip("resizable-arraybuffer");
    add_skip("Array.fromAsync");
    add_skip("arraybuffer-transfer");
    add_skip("Atomics.waitAsync");
    add_skip("FinalizationRegistry.prototype.cleanupSome");
    add_skip("ShadowRealm");
    add_skip("String.prototype.toWellFormed");
    add_skip("String.prototype.isWellFormed");
    add_skip("Temporal");
    add_skip("import-assertions");
    add_skip("decorators");
    add_skip("align-detached-buffer-semantics-with-web-reality");
    add_skip("iterator-helpers");

    while (1) {
        int opt = getopt(argc, argv, "ds:");

        if (opt == -1)
            break;

        switch (opt) {
        case 'd':
            rjs_log_set_level(RJS_LOG_ALL);
            break;
        case 's':
            add_skip(optarg);
            break;
        default:
            return 1;
        }
    }

    rjs_list_init(&test_list);

    if (optind < argc) {
        int i;

        for (i = optind; i < argc; i ++)
            add_test(argv[i]);
    } else {
        add_test(getcwd(test_file_buf, sizeof(test_file_buf)));
    }

    rjs_list_foreach_c(&test_list, test, TestEntry, ln) {
        if (stat(test->name, &sb) == -1) {
            fprintf(stderr, "cannot find \"%s\"\n", test->name);
            return 1;
        }

#if defined WIN32 || defined WIN64
        rpath = _fullpath(real_path_buf, test->name, sizeof(real_path_buf));
#else
        rpath = realpath(test->name, real_path_buf);
#endif

        if (!(base_dir = find_base_dir(base_dir_buf, sizeof(base_dir_buf), rpath)))
            return 1;

        run_test(test->name);
    }

    printf("total cases: %d failed: %d\n", case_num, failed_num);

    return failed_num ? 1 : 0;
}
