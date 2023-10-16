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

/*Print to file.*/
static RJS_Result
print_to_file (RJS_Runtime *rt, RJS_Value *args, size_t argc, FILE *fp)
{
    size_t     i;
    RJS_Result r;
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *str = rjs_value_stack_push(rt);

    for (i = 0; i < argc; i ++) {
        RJS_Value *arg = rjs_argument_get(rt, args, argc, i);

        if ((r = rjs_to_string(rt, arg, str)) == RJS_ERR)
            goto end;

        fprintf(fp, "%s", rjs_string_to_enc_chars(rt, str, NULL, 0));
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Print to stdout.*/
static RJS_NF(ext_print)
{
    rjs_value_set_undefined(rt, rv);
    return print_to_file(rt, args, argc, stdout);
}

/*Print to stderr.*/
static RJS_NF(ext_prerr)
{
    rjs_value_set_undefined(rt, rv);
    return print_to_file(rt, args, argc, stderr);
}

/*Get the directory name.*/
static RJS_NF(ext_dirname)
{
    RJS_Value  *path = rjs_argument_get(rt, args, argc, 0);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *pstr = rjs_value_stack_push(rt);
    const char *cstr;
    char        pbuf[PATH_MAX];
    char       *dcstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, pstr, NULL, NULL);

    snprintf(pbuf, sizeof(pbuf), "%s", cstr);

    dcstr = dirname(pbuf);

    r = rjs_string_from_enc_chars(rt, rv, dcstr, -1, NULL);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the base filename.*/
static RJS_NF(ext_basename)
{
    RJS_Value  *path = rjs_argument_get(rt, args, argc, 0);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *pstr = rjs_value_stack_push(rt);
    const char *cstr;
    char        pbuf[PATH_MAX];
    char       *bcstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, pstr, NULL, NULL);

    snprintf(pbuf, sizeof(pbuf), "%s", cstr);

    bcstr = basename(pbuf);

    r = rjs_string_from_enc_chars(rt, rv, bcstr, -1, NULL);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the real pathname.*/
static RJS_NF(ext_realpath)
{
    RJS_Value  *path = rjs_argument_get(rt, args, argc, 0);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *pstr = rjs_value_stack_push(rt);
    const char *cstr;
    char        rbuf[PATH_MAX];
    char       *rstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, pstr, NULL, NULL);

    rstr = realpath(cstr, rbuf);
    if (!rstr) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
    } else {
        r = rjs_string_from_enc_chars(rt, rv, rstr, -1, NULL);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Rename a file.*/
static RJS_NF(ext_rename)
{
    RJS_Value     *opath = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *npath = rjs_argument_get(rt, args, argc, 1);
    size_t         top   = rjs_value_stack_save(rt);
    RJS_Value     *ostr  = rjs_value_stack_push(rt);
    RJS_Value     *nstr  = rjs_value_stack_push(rt);
    const char    *ocstr, *ncstr;
    RJS_CharBuffer ocb, ncb;
    RJS_Result     r;

    rjs_char_buffer_init(rt, &ocb);
    rjs_char_buffer_init(rt, &ncb);

    if ((r = rjs_to_string(rt, opath, ostr)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, npath, nstr)) == RJS_ERR)
        goto end;

    ocstr = rjs_string_to_enc_chars(rt, ostr, &ocb, NULL);
    ncstr = rjs_string_to_enc_chars(rt, nstr, &ncb, NULL);

    if (rename(ocstr, ncstr) == -1) {
        r = rjs_throw_type_error(rt, _("rename failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_char_buffer_deinit(rt, &ocb);
    rjs_char_buffer_deinit(rt, &ncb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Unlink a file.*/
static RJS_NF(ext_unlink)
{
    RJS_Value  *path = rjs_argument_get(rt, args, argc, 0);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *pstr = rjs_value_stack_push(rt);
    const char *cstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, pstr, NULL, NULL);

    if (unlink(cstr) == -1) {
        r = rjs_throw_type_error(rt, _("unlink failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Remove a directory.*/
static RJS_NF(ext_rmdir)
{
    RJS_Value  *path = rjs_argument_get(rt, args, argc, 0);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *pstr = rjs_value_stack_push(rt);
    const char *cstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, pstr, NULL, NULL);

    if (rmdir(cstr) == -1) {
        r = rjs_throw_type_error(rt, _("rmdir failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Create a new directory.*/
static RJS_NF(ext_mkdir)
{
    RJS_Value  *path = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *mode = rjs_argument_get(rt, args, argc, 1);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *pstr = rjs_value_stack_push(rt);
    int32_t     modei;
    const char *cstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    if (argc > 1) {
        if ((r = rjs_to_int32(rt, mode, &modei)) == RJS_ERR)
            goto end;
    } else {
        modei = 0777;
    }

    cstr = rjs_string_to_enc_chars(rt, pstr, NULL, NULL);

    if (mkdir(cstr, modei) == -1) {
        r = rjs_throw_type_error(rt, _("mkdir failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Change the file's mode.*/
static RJS_NF(ext_chmod)
{
    RJS_Value  *path = rjs_argument_get(rt, args, argc, 0);
    RJS_Value  *mode = rjs_argument_get(rt, args, argc, 1);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *pstr = rjs_value_stack_push(rt);
    int32_t     modei;
    const char *cstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_int32(rt, mode, &modei)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, pstr, NULL, NULL);

    if (chmod(cstr, modei) == -1) {
        r = rjs_throw_type_error(rt, _("chmod failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get the environemt variable.*/
static RJS_NF(ext_getenv)
{
    RJS_Value  *name = rjs_argument_get(rt, args, argc, 0);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *nstr = rjs_value_stack_push(rt);
    const char *ncstr;
    char       *rcstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
        goto end;

    ncstr = rjs_string_to_enc_chars(rt, nstr, NULL, NULL);
    rcstr = getenv(ncstr);

    if (!rcstr) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
    } else {
        r = rjs_string_from_enc_chars(rt, rv, rcstr, -1, NULL);
    }
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Run system command.*/
static RJS_NF(ext_system)
{ 
    RJS_Value  *cmd  = rjs_argument_get(rt, args, argc, 0);
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *str  = rjs_value_stack_push(rt);
    const char *cstr;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, cmd, str)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, str, NULL, NULL);

    r = system(cstr);

    rjs_value_set_number(rt, rv, r);
    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Get current work directory.*/
static RJS_NF(ext_getcwd)
{
    char        buf[PATH_MAX];
    const char *cstr;
    RJS_Result  r;

    cstr = getcwd(buf, sizeof(buf));
    if (!cstr) {
        r = rjs_throw_type_error(rt, _("getcwd failed: %s"), strerror(errno));
        goto end;
    }

    r = rjs_string_from_enc_chars(rt, rv, cstr, -1, NULL);
end:
    return r;
}

/*Get the current script's pathname.*/
static RJS_NF(ext_scriptPath)
{
    RJS_Context       *ctxt = rjs_context_running(rt);
    RJS_ScriptContext *bctxt;
    RJS_Script        *script;

    assert(ctxt->bot);

    bctxt  = (RJS_ScriptContext*)ctxt->bot;
    script = bctxt->script;

    if (script->path)
        rjs_string_from_enc_chars(rt, rv, script->path, -1, NULL);
    else
        rjs_value_copy(rt, rv, rjs_s_empty(rt));

    return RJS_OK;
}

/*Solve jobs.*/
static RJS_NF(ext_solveJobs)
{
    rjs_solve_jobs(rt);
    rjs_value_set_undefined(rt, rv);

    return RJS_OK;
}

#if ENABLE_MODULE

/*Get all the modules loaded.*/
static RJS_NF(ext_modules)
{
    size_t      top  = rjs_value_stack_save(rt);
    RJS_Value  *name = rjs_value_stack_push(rt);
    int64_t     idx  = 0;
    RJS_Result  r;
    size_t      i;
    RJS_Module *mod;

    if ((r = rjs_array_new(rt, rv, 0, NULL)) == RJS_ERR)
        goto end;

    rjs_hash_foreach_c(&rt->mod_hash, i, mod, RJS_Module, he) {
        rjs_string_from_enc_chars(rt, name, mod->script.path, -1, NULL);
        if ((r = rjs_set_index(rt, rv, idx, name, RJS_TRUE)) == RJS_ERR)
            goto end;

        idx ++;
    }

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_MODULE*/

/*Extension functions description.*/
static const RJS_BuiltinFuncDesc
ext_function_descs[] = {
    {
        "print",
        0,
        ext_print
    },
    {
        "prerr",
        0,
        ext_prerr
    },
    {
        "dirname",
        1,
        ext_dirname
    },
    {
        "basename",
        1,
        ext_basename
    },
    {
        "realpath",
        1,
        ext_realpath
    },
    {
        "rename",
        2,
        ext_rename
    },
    {
        "unlink",
        1,
        ext_unlink
    },
    {
        "rmdir",
        1,
        ext_rmdir
    },
    {
        "mkdir",
        1,
        ext_mkdir
    },
    {
        "chmod",
        2,
        ext_chmod
    },
    {
        "getenv",
        1,
        ext_getenv
    },
    {
        "system",
        1,
        ext_system
    },
    {
        "getcwd",
        0,
        ext_getcwd
    },
    {
        "solveJobs",
        0,
        ext_solveJobs
    },
    {
        "scriptPath",
        0,
        ext_scriptPath
    },
#if ENABLE_MODULE
    {
        "modules",
        0,
        ext_modules
    },
#endif /*ENABLE_MODULE*/
    {NULL}
};

/**File format.*/
typedef enum {
    RJS_FILE_FORMAT_REG,  /**< Regular file.*/
    RJS_FILE_FORMAT_DIR,  /**< Directory.*/
    RJS_FILE_FORMAT_CHR,  /**< Character device.*/
    RJS_FILE_FORMAT_BLK,  /**< Block device.*/
    RJS_FILE_FORMAT_FIFO, /**< FIFO.*/
    RJS_FILE_FORMAT_SOCK, /**< Socket.*/
    RJS_FILE_FORMAT_LNK   /**< Symbolic link.*/
} RJS_FileFormat;

/*FileState*/
static RJS_NF(FileState_constructor)
{
    RJS_Value     *name   = rjs_argument_get(rt, args, argc, 0);
    size_t         top    = rjs_value_stack_save(rt);
    RJS_Value     *nstr   = rjs_value_stack_push(rt);
    RJS_Value     *pv     = rjs_value_stack_push(rt);
    RJS_Value     *cproto = rjs_value_stack_push(rt);
    RJS_Value     *proto;
    const char    *ncstr;
    struct stat    sb;
    RJS_Result     r;
    RJS_FileFormat fmt;

    if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
        goto end;

    ncstr = rjs_string_to_enc_chars(rt, nstr, NULL, NULL);

    if (stat(ncstr, &sb) == -1) {
        rjs_value_set_undefined(rt, rv);
        r = RJS_OK;
        goto end;
    }

    switch (sb.st_mode & S_IFMT) {
    case S_IFREG:
        fmt = RJS_FILE_FORMAT_REG;
        break;
    case S_IFDIR:
        fmt = RJS_FILE_FORMAT_DIR;
        break;
    case S_IFCHR:
        fmt = RJS_FILE_FORMAT_CHR;
        break;
    case S_IFBLK:
        fmt = RJS_FILE_FORMAT_BLK;
        break;
    case S_IFIFO:
        fmt = RJS_FILE_FORMAT_FIFO;
        break;
#if !defined WIN32 && !defined WIN64
    case S_IFSOCK:
        fmt = RJS_FILE_FORMAT_SOCK;
        break;
    case S_IFLNK:
        fmt = RJS_FILE_FORMAT_LNK;
        break;
#endif /*!defined WIN32 && !defined WIN64*/
    default:
        fmt = -1;
        break;
    }

    if (nt) {
        if ((r = rjs_constructor_prototype(rt, nt, cproto)) == RJS_ERR)
            goto end;

        if (!rjs_value_is_object(rt, cproto))
            proto = NULL;
        else
            proto = cproto;
    } else {
        proto = NULL;
    }

    rjs_ordinary_object_create(rt, proto, rv);

    /*size.*/
    rjs_value_set_number(rt, pv, sb.st_size);
    rjs_create_data_property_attrs(rt, rv, rjs_pn_size(rt), pv, RJS_PROP_ATTR_ENUMERABLE);

    /*format.*/
    rjs_value_set_number(rt, pv, fmt);
    rjs_create_data_property_attrs(rt, rv, rjs_pn_format(rt), pv, RJS_PROP_ATTR_ENUMERABLE);

    /*mode.*/
    rjs_value_set_number(rt, pv, sb.st_mode & 07777);
    rjs_create_data_property_attrs(rt, rv, rjs_pn_mode(rt), pv, RJS_PROP_ATTR_ENUMERABLE);

    /*atime.*/
    rjs_value_set_number(rt, pv, sb.st_atime);
    rjs_create_data_property_attrs(rt, rv, rjs_pn_atime(rt), pv, RJS_PROP_ATTR_ENUMERABLE);

    /*mtime.*/
    rjs_value_set_number(rt, pv, sb.st_mtime);
    rjs_create_data_property_attrs(rt, rv, rjs_pn_mtime(rt), pv, RJS_PROP_ATTR_ENUMERABLE);

    /*ctime.*/
    rjs_value_set_number(rt, pv, sb.st_ctime);
    rjs_create_data_property_attrs(rt, rv, rjs_pn_ctime(rt), pv, RJS_PROP_ATTR_ENUMERABLE);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
file_state_constructor_desc = {
    "FileState",
    1,
    FileState_constructor
};

static const RJS_BuiltinFieldDesc
file_state_field_descs[] = {
    {
        "FORMAT_REGULAR",
        RJS_VALUE_NUMBER,
        RJS_FILE_FORMAT_REG,
        NULL,
        0
    },
    {
        "FORMAT_DIR",
        RJS_VALUE_NUMBER,
        RJS_FILE_FORMAT_DIR,
        NULL,
        0
    },
    {
        "FORMAT_CHAR",
        RJS_VALUE_NUMBER,
        RJS_FILE_FORMAT_CHR,
        NULL,
        0
    },
    {
        "FORMAT_BLOCK",
        RJS_VALUE_NUMBER,
        RJS_FILE_FORMAT_BLK,
        NULL,
        0
    },
    {
        "FORMAT_SOCKET",
        RJS_VALUE_NUMBER,
        RJS_FILE_FORMAT_SOCK,
        NULL,
        0
    },
    {
        "FORMAT_FIFO",
        RJS_VALUE_NUMBER,
        RJS_FILE_FORMAT_FIFO,
        NULL,
        0
    },
    {
        "FORMAT_LINK",
        RJS_VALUE_NUMBER,
        RJS_FILE_FORMAT_LNK,
        NULL,
        0
    },
    {NULL}
};

/**File data.*/
typedef struct {
    FILE *fp; /**< The file pointer.*/
} RJS_File;

/*Scan the reference things in the file.*/
static void
file_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
}

/*Free the file.*/
static void
file_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_File *file = ptr;

    if (file->fp) {
        if ((file->fp != stdin) && (file->fp != stdout) && (file->fp != stderr))
            fclose(file->fp);
    }

    RJS_DEL(rt, file);
}

static const char* file_tag = "File";

/*File*/
static RJS_NF(File_constructor)
{
    RJS_Value     *path  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *mode  = rjs_argument_get(rt, args, argc, 1);
    size_t         top   = rjs_value_stack_save(rt);
    RJS_Value     *pstr  = rjs_value_stack_push(rt);
    RJS_Value     *mstr  = rjs_value_stack_push(rt);
    RJS_Realm     *realm = NULL;
    RJS_CharBuffer pcb, mcb;
    const char    *pcstr, *mcstr;
    FILE          *fp;
    RJS_File      *file;
    RJS_Result     r;

    rjs_char_buffer_init(rt, &pcb);
    rjs_char_buffer_init(rt, &mcb);

    if ((r = rjs_to_string(rt, path, pstr)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_string(rt, mode, mstr)) == RJS_ERR)
        goto end;

    pcstr = rjs_string_to_enc_chars(rt, pstr, &pcb, NULL);
    mcstr = rjs_string_to_enc_chars(rt, mstr, &mcb, NULL);

    if (!(fp = fopen(pcstr, mcstr))) {
        r = rjs_throw_type_error(rt, _("fopen failed: %s"), strerror(errno));
        goto end;
    }

    if (nt)
        realm = rjs_get_function_realm(rt, nt);

    if (!realm)
        realm = rjs_realm_current(rt);

    RJS_NEW(rt, file);
    file->fp  = fp;

    rjs_native_object_from_constructor(rt, nt, rjs_o_File_prototype(realm), rv);
    rjs_native_object_set_data(rt, rv, file_tag, file, file_op_gc_scan, file_op_gc_free);

    r = RJS_OK;
end:
    rjs_char_buffer_deinit(rt, &pcb);
    rjs_char_buffer_deinit(rt, &mcb);
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
file_constructor_desc = {
    "File",
    2,
    File_constructor
};

#if ENABLE_ARRAY_BUFFER

/*File.prototype.read*/
static RJS_NF(File_prototype_read)
{
    RJS_Value     *abuf = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *pos  = rjs_argument_get(rt, args, argc, 1);
    RJS_Value     *cnt  = rjs_argument_get(rt, args, argc, 2);
    int64_t        posi, cnti;
    size_t         size, rn;
    RJS_File      *file;
    RJS_DataBlock *db;
    void          *buf;
    RJS_Result     r;

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    if (!rjs_is_array_buffer(rt, abuf)) {
        r = rjs_throw_type_error(rt, _("the value is not an array buffer"));
        goto end;
    }

    size = rjs_array_buffer_get_size(rt, abuf);

    if (argc > 1) {
        if ((r = rjs_to_index(rt, pos, &posi)) == RJS_ERR)
            goto end;

        if (posi > size)
            posi = size;
    } else {
        posi = 0;
    }

    if (argc > 2) {
        if ((r = rjs_to_length(rt, cnt, &cnti)) == RJS_ERR)
            goto end;

        if (posi + cnti > size)
            cnti = size - posi;
    } else {
        cnti = size - posi;
    }

    if (cnti) {
        if (!(db = rjs_array_buffer_get_data_block(rt, abuf))) {
            r = rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        buf = rjs_data_block_get_buffer(db);

        rn = fread(buf, 1, cnti, file->fp);

        if ((rn == 0) && ferror(file->fp)) {
            r = rjs_throw_type_error(rt, _("fread failed: %s"), strerror(errno));
            goto end;
        }
    } else {
        rn = 0;
    }

    rjs_value_set_number(rt, rv, rn);
    r = RJS_OK;
end:
    return r;
}

/*File.prototype.write*/
static RJS_NF(File_prototype_write)
{
    RJS_Value     *abuf = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *pos  = rjs_argument_get(rt, args, argc, 1);
    RJS_Value     *cnt  = rjs_argument_get(rt, args, argc, 2);
    int64_t        posi, cnti;
    size_t         size, rn;
    RJS_File      *file;
    RJS_DataBlock *db;
    void          *buf;
    RJS_Result     r;

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    if (!rjs_is_array_buffer(rt, abuf)) {
        r = rjs_throw_type_error(rt, _("the value is not an array buffer"));
        goto end;
    }

    size = rjs_array_buffer_get_size(rt, abuf);

    if (argc > 1) {
        if ((r = rjs_to_index(rt, pos, &posi)) == RJS_ERR)
            goto end;

        if (posi > size)
            posi = size;
    } else {
        posi = 0;
    }

    if (argc > 2) {
        if ((r = rjs_to_length(rt, cnt, &cnti)) == RJS_ERR)
            goto end;

        if (posi + cnti > size)
            cnti = size - posi;
    } else {
        cnti = size - posi;
    }

    if (cnti) {
        if (!(db = rjs_array_buffer_get_data_block(rt, abuf))) {
            r = rjs_throw_type_error(rt, _("the array buffer is detached"));
            goto end;
        }

        buf = rjs_data_block_get_buffer(db);

        rn = fwrite(buf, 1, cnti, file->fp);

        if ((rn != cnti) && ferror(file->fp)) {
            r = rjs_throw_type_error(rt, _("fwrite failed: %s"), strerror(errno));
            goto end;
        }
    } else {
        rn = 0;
    }

    rjs_value_set_number(rt, rv, rn);
    r = RJS_OK;
end:
    return r;
}

#endif /*ENABLE_ARRAY_BUFFER*/

/*File.prototype.seek*/
static RJS_NF(File_prototype_seek)
{
    RJS_Value *offset = rjs_argument_get(rt, args, argc, 0);
    RJS_Value *whence = rjs_argument_get(rt, args, argc, 1);
    int64_t    offseti;
    int32_t    whencei;
    RJS_File  *file;
    RJS_Result r;

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    if ((r = rjs_to_index(rt, offset, &offseti)) == RJS_ERR)
        goto end;

    if ((r = rjs_to_int32(rt, whence, &whencei)) == RJS_ERR)
        goto end;

    if (fseek(file->fp, offseti, whencei) == -1) {
        r = rjs_throw_type_error(rt, _("fseek failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

/*File.prototype.tell*/
static RJS_NF(File_prototype_tell)
{
    RJS_File  *file;
    RJS_Result r;
    long       off;

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    off = ftell(file->fp);

    rjs_value_set_number(rt, rv, off);
    r = RJS_OK;
end:
    return r;
}

/*File.prototype.close*/
static RJS_NF(File_prototype_close)
{
    RJS_File  *file;
    RJS_Result r;

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (file->fp) {
        if ((file->fp != stdin) && (file->fp != stdout) && (file->fp != stderr)) {
            fclose(file->fp);
            file->fp = NULL;
        }
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    return r;
}

/*File.prototype.getChar*/
static RJS_NF(File_prototype_getChar)
{
    RJS_File  *file;
    RJS_Result r;
    int        c;

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    c = fgetc(file->fp);
    if (c == EOF) {
        rjs_value_set_undefined(rt, rv);
    } else {
        rjs_value_set_number(rt, rv, c);
    }

    r = RJS_OK;
end:
    return r;
}

/*File.prototype.putChar*/
static RJS_NF(File_prototype_putChar)
{
    RJS_Value *chr = rjs_argument_get(rt, args, argc, 0);
    int32_t    i;
    RJS_File  *file;
    RJS_Result r;

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    if ((r = rjs_to_int32(rt, chr, &i)) == RJS_ERR)
        goto end;

    r = fputc(i, file->fp);
    if (r == EOF) {
        r = rjs_throw_type_error(rt, _("futc failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    return r;
}

/*File.prototype.getString*/
static RJS_NF(File_prototype_getString)
{
    RJS_Value     *enc  = rjs_argument_get(rt, args, argc, 0);
    size_t         top  = rjs_value_stack_save(rt);
    RJS_Value     *estr = rjs_value_stack_push(rt);
    const char    *ecstr;
    RJS_File      *file;
    RJS_Result     r;
    int            c;
    RJS_CharBuffer cb;

    rjs_char_buffer_init(rt, &cb);

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    if (argc > 0) {
        if ((r = rjs_to_string(rt, enc, estr)) == RJS_ERR)
            goto end;

        ecstr = rjs_string_to_enc_chars(rt, estr, NULL, NULL);
    } else {
        ecstr = NULL;
    }

    while (1) {
        c = fgetc(file->fp);
        if (c == EOF)
            break;

        rjs_char_buffer_append_char(rt, &cb, c);

        if (c == '\n')
            break;
    }

    r = rjs_string_from_enc_chars(rt, rv, cb.items, cb.item_num, ecstr);
end:
    rjs_char_buffer_deinit(rt, &cb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*File.prototype.putString*/
static RJS_NF(File_prototype_putString)
{
    RJS_Value     *pv   = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *enc  = rjs_argument_get(rt, args, argc, 1);
    size_t         top  = rjs_value_stack_save(rt);
    RJS_Value     *pstr = rjs_value_stack_push(rt);
    RJS_Value     *estr = rjs_value_stack_push(rt);
    const char    *pcstr, *ecstr;
    RJS_CharBuffer pcb, ecb;
    RJS_File      *file;
    size_t         n;
    RJS_Result     r;

    rjs_char_buffer_init(rt, &pcb);
    rjs_char_buffer_init(rt, &ecb);

    if (rjs_native_object_get_tag(rt, thiz) != file_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a file"));
        goto end;
    }

    file = rjs_native_object_get_data(rt, thiz);
    if (!file->fp) {
        r = rjs_throw_type_error(rt, _("the file is closed"));
        goto end;
    }

    if ((r = rjs_to_string(rt, pv, pstr)) == RJS_ERR)
        goto end;

    if (argc > 1) {
        if ((r = rjs_to_string(rt, enc, estr)) == RJS_ERR)
            goto end;

        ecstr = rjs_string_to_enc_chars(rt, estr, &ecb, NULL);
    } else {
        ecstr = NULL;
    }

    pcstr = rjs_string_to_enc_chars(rt, pstr, &pcb, ecstr);

    n = fwrite(pcstr, 1, pcb.item_num, file->fp);
    if (n != pcb.item_num) {
        r = rjs_throw_type_error(rt, _("fwrite failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_copy(rt, rv, thiz);
    r = RJS_OK;
end:
    rjs_char_buffer_deinit(rt, &pcb);
    rjs_char_buffer_deinit(rt, &ecb);
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
file_prototype_function_descs[] = {
#if ENABLE_ARRAY_BUFFER
    {
        "read",
        1,
        File_prototype_read
    },
    {
        "write",
        1,
        File_prototype_write
    },
#endif /*ENABLE_ARRAY_BUFFER*/
    {
        "seek",
        2,
        File_prototype_seek
    },
    {
        "tell",
        0,
        File_prototype_tell
    },
    {
        "close",
        0,
        File_prototype_close
    },
    {
        "getChar",
        0,
        File_prototype_getChar
    },
    {
        "putChar",
        1,
        File_prototype_putChar
    },
    {
        "getString",
        0,
        File_prototype_getString
    },
    {
        "putString",
        1,
        File_prototype_putString
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
file_prototype_desc = {
    "File",
    NULL,
    NULL,
    NULL,
    NULL,
    file_prototype_function_descs,
    NULL,
    NULL,
    "File_prototype"
};

static const RJS_BuiltinFieldDesc
file_field_descs[] = {
    {
        "SEEK_SET",
        RJS_VALUE_NUMBER,
        SEEK_SET,
        NULL,
        0
    },
    {
        "SEEK_END",
        RJS_VALUE_NUMBER,
        SEEK_END,
        NULL,
        0
    },
    {
        "SEEK_CUR",
        RJS_VALUE_NUMBER,
        SEEK_CUR,
        NULL,
        0
    },
    {NULL}
};

/*File.loadString*/
static RJS_NF(File_loadString)
{
    RJS_Value     *name  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *enc   = rjs_argument_get(rt, args, argc, 1);
    size_t         top   = rjs_value_stack_save(rt);
    RJS_Value     *nstr  = rjs_value_stack_push(rt);
    RJS_Value     *estr  = rjs_value_stack_push(rt);
    const char    *ncstr, *ecstr;
    RJS_CharBuffer ncb, ecb;
    RJS_Result     r;

    rjs_char_buffer_init(rt, &ncb);
    rjs_char_buffer_init(rt, &ecb);

    if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
        goto end;

    ncstr = rjs_string_to_enc_chars(rt, nstr, &ncb, NULL);

    if (argc > 1) {
        if ((r = rjs_to_string(rt, enc, estr)) == RJS_ERR)
            goto end;

        ecstr = rjs_string_to_enc_chars(rt, estr, &ecb, NULL);
    } else {
        ecstr = NULL;
    }

    if ((r = rjs_string_from_file(rt, rv, ncstr, ecstr)) == RJS_ERR)
        r = rjs_throw_type_error(rt, _("load file \"%s\" failed"), ncstr);
end:
    rjs_char_buffer_deinit(rt, &ncb);
    rjs_char_buffer_deinit(rt, &ecb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Save string to file.*/
static RJS_Result
file_save_string (RJS_Runtime *rt, RJS_Value *args, size_t argc, RJS_Value *rv, const char *mode)
{
    RJS_Value     *name  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *out   = rjs_argument_get(rt, args, argc, 1);
    RJS_Value     *enc   = rjs_argument_get(rt, args, argc, 2);
    size_t         top   = rjs_value_stack_save(rt);
    RJS_Value     *nstr  = rjs_value_stack_push(rt);
    RJS_Value     *ostr  = rjs_value_stack_push(rt);
    RJS_Value     *estr  = rjs_value_stack_push(rt);
    FILE          *fp    = NULL;
    const char    *ncstr, *ocstr, *ecstr;
    RJS_CharBuffer ncb, ocb, ecb;
    RJS_Result     r;

    rjs_char_buffer_init(rt, &ncb);
    rjs_char_buffer_init(rt, &ocb);
    rjs_char_buffer_init(rt, &ecb);

    if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
        goto end;

    ncstr = rjs_string_to_enc_chars(rt, nstr, &ncb, NULL);

    if (!(fp = fopen(ncstr, mode))) {
        r = rjs_throw_type_error(rt, _("fopen \"%s\" failed: %s"), ncstr, strerror(errno));
        goto end;
    }

    if ((r = rjs_to_string(rt, out, ostr)) == RJS_ERR)
        goto end;

    if (argc > 2) {
        if ((r = rjs_to_string(rt, enc, estr)) == RJS_ERR)
            goto end;

        ecstr = rjs_string_to_enc_chars(rt, estr, &ecb, NULL);
    } else {
        ecstr = NULL;
    }

    ocstr = rjs_string_to_enc_chars(rt, ostr, &ocb, ecstr);

    if (fwrite(ocstr, 1, ocb.item_num, fp) != ocb.item_num) {
        r = rjs_throw_type_error(rt, _("fwrite failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    if (fp)
        fclose(fp);

    rjs_char_buffer_deinit(rt, &ncb);
    rjs_char_buffer_deinit(rt, &ocb);
    rjs_char_buffer_deinit(rt, &ecb);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*File.storeString*/
static RJS_NF(File_storeString)
{
    return file_save_string(rt, args, argc, rv, "wb");
}

/*File.appendString*/
static RJS_NF(File_appendString)
{
    return file_save_string(rt, args, argc, rv, "ab");
}

#if ENABLE_ARRAY_BUFFER

/*File.loadData*/
static RJS_NF(File_loadData)
{
    RJS_Value     *name = rjs_argument_get(rt, args, argc, 0);
    size_t         top  = rjs_value_stack_save(rt);
    RJS_Value     *nstr = rjs_value_stack_push(rt);
    FILE          *fp   = NULL;
    const char    *ncstr;
    RJS_DataBlock *db;
    void          *buf;
    struct stat    sb;
    RJS_Result     r;

    if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
        goto end;

    ncstr = rjs_string_to_enc_chars(rt, nstr, NULL, NULL);

    if (stat(ncstr, &sb) == -1) {
        r = rjs_throw_type_error(rt, _("stat \"%s\" failed: %s"), ncstr, strerror(errno));
        goto end;
    }

    if (!(fp = fopen(ncstr, "rb"))) {
        r = rjs_throw_type_error(rt, _("fopen \"%s\" failed: %s"), ncstr, strerror(errno));
        goto end;
    }

    if ((r = rjs_allocate_array_buffer(rt, NULL, sb.st_size, rv)) == RJS_ERR)
        goto end;

    db  = rjs_array_buffer_get_data_block(rt, rv);
    buf = rjs_data_block_get_buffer(db);

    if (fread(buf, 1, sb.st_size, fp) != sb.st_size) {
        r = rjs_throw_type_error(rt, _("fread failed: %s"), strerror(errno));
        goto end;
    }

    r = RJS_OK;
end:
    if (fp)
        fclose(fp);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Save the data to file.*/
static RJS_Result
file_save_data (RJS_Runtime *rt, RJS_Value *args, size_t argc, RJS_Value *rv, const char *mode)
{
    RJS_Value     *name = rjs_argument_get(rt, args, argc, 0);
    RJS_Value     *abuf = rjs_argument_get(rt, args, argc, 1);
    RJS_Value     *pos  = rjs_argument_get(rt, args, argc, 2);
    RJS_Value     *cnt  = rjs_argument_get(rt, args, argc, 3);
    size_t         top  = rjs_value_stack_save(rt);
    RJS_Value     *nstr = rjs_value_stack_push(rt);
    FILE          *fp   = NULL;
    const char    *ncstr;
    RJS_DataBlock *db;
    uint8_t       *buf;
    int64_t        size, posi, cnti;
    RJS_Result     r;

    if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
        goto end;

    ncstr = rjs_string_to_enc_chars(rt, nstr, NULL, NULL);

    if (!(fp = fopen(ncstr, mode))) {
        r = rjs_throw_type_error(rt, _("fopen \"%s\" failed: %s"), ncstr, strerror(errno));
        goto end;
    }

    if (!rjs_is_array_buffer(rt, abuf)) {
        r = rjs_throw_type_error(rt, _("the value is not an array buffer"));
        goto end;
    }

    if (!(db = rjs_array_buffer_get_data_block(rt, rv))) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    buf  = rjs_data_block_get_buffer(db);
    size = rjs_array_buffer_get_size(rt, abuf);

    if (argc > 2) {
        if ((r = rjs_to_index(rt, pos, &posi)) == RJS_ERR)
            goto end;

        posi = RJS_MIN(posi, size);
    } else {
        posi = 0;
    }

    if (argc > 3) {
        if ((r = rjs_to_length(rt, cnt, &cnti)) == RJS_ERR)
            goto end;

        cnti = RJS_MIN(size - posi, cnti);
    } else {
        cnti = size - posi;
    }

    if (fwrite(buf + posi, 1, cnti, fp) != cnti) {
        r = rjs_throw_type_error(rt, _("fwrite failed: %s"), strerror(errno));
        goto end;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    if (fp)
        fclose(fp);
    rjs_value_stack_restore(rt, top);
    return r;
}

/*File.storeData*/
static RJS_NF(File_storeData)
{
    return file_save_data(rt, args, argc, rv, "wb");
}

/*File.appendData*/
static RJS_NF(File_appendData)
{
    return file_save_data(rt, args, argc, rv, "ab");
}

#endif /*ENABLE_ARRAY_BUFFER*/

static const RJS_BuiltinFuncDesc
file_function_descs[] = {
    {
        "loadString",
        1,
        File_loadString
    },
    {
        "storeString",
        2,
        File_storeString
    },
    {
        "appendString",
        2,
        File_appendString
    },
#if ENABLE_ARRAY_BUFFER
    {
        "loadData",
        1,
        File_loadData
    },
    {
        "storeData",
        2,
        File_storeData
    },
    {
        "appendData",
        2,
        File_appendData
    },
#endif /*ENABLE_ARRAY_BUFFER*/
    {NULL}
};

/**Directory.*/
typedef struct {
    DIR *dir; /**< The directory.*/
} RJS_Dir;

/*Scan the referenced things in the directory.*/
static void
dir_op_gc_scan (RJS_Runtime *rt, void *ptr)
{
}

/*Free the directory.*/
static void
dir_op_gc_free (RJS_Runtime *rt, void *ptr)
{
    RJS_Dir *dir = ptr;

    if (dir->dir)
        closedir(dir->dir);

    RJS_DEL(rt, dir);
}

static const char *dir_tag = "Dir";

/*Dir*/
static RJS_NF(Dir_constructor)
{
    RJS_Value  *name  = rjs_argument_get(rt, args, argc, 0);
    size_t      top   = rjs_value_stack_save(rt);
    RJS_Value  *nstr  = rjs_value_stack_push(rt);
    RJS_Realm  *realm = NULL;
    const char *ncstr;
    RJS_Result  r;
    DIR        *dp;
    RJS_Dir    *dir;

    if ((r = rjs_to_string(rt, name, nstr)) == RJS_ERR)
        goto end;

    ncstr = rjs_string_to_enc_chars(rt, nstr, NULL, NULL);

    if (!(dp = opendir(ncstr))) {
        r = rjs_throw_type_error(rt, _("opendir \"%s\" failed: %s"), ncstr, strerror(errno));
        goto end;
    }

    if (nt)
        realm = rjs_get_function_realm(rt, nt);

    if (!realm)
        realm = rjs_realm_current(rt);

    RJS_NEW(rt, dir);
    dir->dir = dp;

    rjs_native_object_from_constructor(rt, nt, rjs_o_Dir_prototype(realm), rv);
    rjs_native_object_set_data(rt, rv, dir_tag, dir,
            dir_op_gc_scan, dir_op_gc_free);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const RJS_BuiltinFuncDesc
dir_constructor_desc = {
    "Dir",
    1,
    Dir_constructor
};

/*Dir.prototype.read*/
static RJS_NF(Dir_prototype_read)
{
    RJS_Dir       *dir;
    struct dirent *dent;
    RJS_Result     r;

    if (rjs_native_object_get_tag(rt, thiz) != dir_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a directory"));
        goto end;
    }

    dir = rjs_native_object_get_data(rt, thiz);
    if (!dir->dir) {
        r = rjs_throw_type_error(rt, _("the directory is closed"));
        goto end;
    }

    dent = readdir(dir->dir);
    if (!dent) {
        rjs_value_set_undefined(rt, rv);
    } else {
        rjs_string_from_enc_chars(rt, rv, dent->d_name, -1, NULL);
    }

    r = RJS_OK;
end:
    return r;
}

/*Dir.prototype.next*/
static RJS_NF(Dir_prototype_next)
{
    size_t         top = rjs_value_stack_save(rt);
    RJS_Value     *v   = rjs_value_stack_push(rt);
    RJS_Bool       done;
    RJS_Dir       *dir;
    struct dirent *dent;
    RJS_Result     r;

    if (rjs_native_object_get_tag(rt, thiz) != dir_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a directory"));
        goto end;
    }

    dir = rjs_native_object_get_data(rt, thiz);
    if (!dir->dir) {
        done = RJS_TRUE;
        rjs_value_set_undefined(rt, v);
    } else {
        dent = readdir(dir->dir);
        if (!dent) {
            rjs_value_set_undefined(rt, v);
            done = RJS_TRUE;

            closedir(dir->dir);
            dir->dir = NULL;
        } else {
            rjs_string_from_enc_chars(rt, v, dent->d_name, -1, NULL);
            done = RJS_FALSE;
        }
    }

    r = rjs_create_iter_result_object(rt, v, done, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*Dir.prototype.close*/
static RJS_NF(Dir_prototype_close)
{
    RJS_Dir   *dir;
    RJS_Result r;

    if (rjs_native_object_get_tag(rt, thiz) != dir_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a directory"));
        goto end;
    }

    dir = rjs_native_object_get_data(rt, thiz);
    if (dir->dir) {
        closedir(dir->dir);
        dir->dir = NULL;
    }

    rjs_value_set_undefined(rt, rv);
    r = RJS_OK;
end:
    return r;
}

/*Dir.prototype.return*/
static RJS_NF(Dir_prototype_return)
{
    RJS_Dir   *dir;
    RJS_Result r;

    if (rjs_native_object_get_tag(rt, thiz) != dir_tag) {
        r = rjs_throw_type_error(rt, _("the object is not a directory"));
        goto end;
    }

    dir = rjs_native_object_get_data(rt, thiz);
    if (dir->dir) {
        closedir(dir->dir);
        dir->dir = NULL;
    }

    rjs_create_iter_result_object(rt, rjs_v_undefined(rt), RJS_TRUE, rv);
    r = RJS_OK;
end:
    return r;
}

static const RJS_BuiltinFuncDesc
dir_prototype_function_descs[] = {
    {
        "read",
        0,
        Dir_prototype_read
    },
    {
        "next",
        0,
        Dir_prototype_next
    },
    {
        "close",
        0,
        Dir_prototype_close
    },
    {
        "return",
        0,
        Dir_prototype_return
    },
    {NULL}
};

static const RJS_BuiltinObjectDesc
dir_prototype_desc = {
    "Dir",
    "IteratorPrototype",
    NULL,
    NULL,
    NULL,
    dir_prototype_function_descs,
    NULL,
    NULL,
    "Dir_prototype"
};

/*Extension object descriptiosn.*/
static const RJS_BuiltinObjectDesc
ext_object_descs[] = {
    {
        "FileState",
        NULL,
        &file_state_constructor_desc,
        NULL,
        file_state_field_descs,
        NULL,
        NULL,
        NULL
    },
    {
        "File",
        NULL,
        &file_constructor_desc,
        &file_prototype_desc,
        file_field_descs,
        file_function_descs,
        NULL,
        NULL
    },
    {
        "Dir",
        NULL,
        &dir_constructor_desc,
        &dir_prototype_desc,
        NULL,
        NULL,
        NULL,
        NULL
    },
    {NULL}
};

/*Extension description.*/
static const RJS_BuiltinDesc
ext_desc = {
    NULL,
    ext_function_descs,
    ext_object_descs
};

/**
 * Load extension functions to the realm.
 * \param rt The current runtime.
 * \param realm The realm which extension functions to be loaded in.
 * If realm == NULL, functions will be loaded in current realm.
 * \retval RJS_OK On success.
 * \retval RJS_ERR On error.
 */
RJS_Result
rjs_realm_load_extension (RJS_Runtime *rt, RJS_Realm *realm)
{
    if (!realm)
        realm = rjs_realm_current(rt);

    rjs_load_builtin_desc(rt, realm, &ext_desc);

    return RJS_OK;
}
