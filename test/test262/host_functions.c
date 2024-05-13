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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <ratjs.h>

/**Broadcast.*/
#define MSG_BROADCAST 1

/**Agent,*/
typedef struct {
    RJS_List        ln;     /**< List node data.*/
    pthread_t       thread; /**< The thread.*/
    char           *src;    /**< Source of the script.*/
#if ENABLE_SHARED_ARRAY_BUFFER
    RJS_DataBlock  *db;     /**< The data block.*/
    size_t          off;    /**< Offset.*/
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/
    pthread_cond_t  cond;   /**< Conditional vairable.*/
    int             flags;  /**< Flags.*/
} Agent;

/**Report.*/
typedef struct {
    RJS_List  ln;  /**< List node data.*/
    char     *msg; /**< The message string.*/
} Report;

/**Realm.*/
typedef struct {
    RJS_List    ln;      /**< List node data.*/
    RJS_Realm  *realm;   /**< The realm.*/
    RJS_Object *test262; /**< The test262 object.*/
} Realm;

/**Function description.*/
typedef struct {
    char          *name;     /**< Name of the function.*/
    int            length;   /**< Length of the function.*/
    RJS_NativeFunc function; /**< The native function's pointer.*/
} FuncDesc;

/**Object description.*/
typedef struct ObjectDesc_s ObjectDesc;
/**Object description.*/
struct ObjectDesc_s {
    char             *name;      /**< Name of the object.*/
    const FuncDesc   *functions; /**< The function properties.*/
    const ObjectDesc *objects;   /**< The object properties.*/
};

/*Async test is end.*/
static int             async_end = 0;
/*Agent list.*/
static RJS_List        agent_list;
/*Agent key.*/
static pthread_key_t   agent_key;
/*Agent lock.*/
static pthread_mutex_t agent_lock;
/*Report message.*/
static RJS_List        report_list;
/*Realms.*/
static RJS_List        realm_list;

/*Load host defined functions.*/
int load_host_functions (RJS_Runtime *rt, RJS_Realm *realm);

/*Free the agent.*/
static void
agent_free (Agent *agent)
{
    rjs_list_remove(&agent->ln);

    if (agent->src)
        free(agent->src);

#if ENABLE_SHARED_ARRAY_BUFFER
    if (agent->db)
        rjs_data_block_unref(agent->db);
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

    pthread_cond_destroy(&agent->cond);

    free(agent);
}

/*print*/
static RJS_NF(host_print)
{
    RJS_Value  *v   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *str = rjs_value_stack_push(rt);
    const char *cstr;
    RJS_Result  r;

    if (rjs_value_is_object(rt, v)) {
        r = RJS_OK;
        goto end;
    }

    if ((r = rjs_to_string(rt, v, str)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, str, NULL, NULL);

    printf("print: %s\n", cstr);

    if (strstr(cstr, "AsyncTestFailure"))
        async_end = -1;
    else
        async_end = 1;

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

static const FuncDesc
host_function_descs[] = {
    {
        "print",
        1,
        host_print
    },
    {NULL}
};

/*$262.createRealm*/
static RJS_NF(test262_createRealm)
{
    size_t           top    = rjs_value_stack_save(rt);
    RJS_Value       *str    = rjs_value_stack_push(rt);
    RJS_Value       *realmv = rjs_value_stack_push(rt);
    Realm           *realm;
    RJS_PropertyName pn;

    realm = malloc(sizeof(Realm));
    assert(realm);
    
    realm->realm = rjs_realm_new(rt, realmv);
    rjs_list_append(&realm_list, &realm->ln);

    load_host_functions(rt, realm->realm);

    rjs_string_from_chars(rt, str, "$262", -1);
    rjs_property_name_init(rt, &pn, str);
    rjs_get(rt, rjs_global_object(realm->realm), &pn, rv);
    rjs_property_name_deinit(rt, &pn);

    realm->test262 = rjs_value_get_object(rt, rv);

    RJS_LOGD("new realm %p", realm->realm);

    rjs_value_stack_restore(rt, top);
    return RJS_OK;
}

#if ENABLE_ARRAY_BUFFER
/*$262.detachArrayBuffer*/
static RJS_NF(test262_detachArrayBuffer)
{
    RJS_Value *abv = rjs_argument_get(rt, args, argc, 0);

    if (!rjs_is_array_buffer(rt, abv))
        return RJS_OK;

    rjs_detach_array_buffer(rt, abv);

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}
#endif /*ENABLE_ARRAY_BUFFER*/

#if ENABLE_EVAL

/*$262.evalScript*/
static RJS_NF(test262_evalScript)
{
    RJS_Value  *v      = rjs_argument_get(rt, args, argc, 0);
    size_t      top    = rjs_value_stack_save(rt);
    RJS_Value  *src    = rjs_value_stack_push(rt);
    RJS_Value  *script = rjs_value_stack_push(rt);
    RJS_Realm  *realm  = NULL;
    Realm      *rtmp;
    RJS_Object *test262;
    RJS_Result  r;

    test262 = rjs_value_get_object(rt, thiz);

    rjs_list_foreach_c(&realm_list, rtmp, Realm, ln) {
        if (rtmp->test262 == test262) {
            realm = rtmp->realm;
            break;
        }
    }

    if (!realm)
        realm = rjs_realm_current(rt);

    if ((r = rjs_to_string(rt, v, src)) == RJS_ERR)
        goto end;

    if ((r = rjs_script_from_string(rt, script, src, realm, RJS_FALSE)) == RJS_ERR) {
        r = rjs_throw_syntax_error(rt, _("syntax error"));
        goto end;
    }

    r = rjs_script_evaluation(rt, script, rv);
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#endif /*ENABLE_EVAL*/

/*$262.gc*/
static RJS_NF(test262_gc)
{
    rjs_gc_run(rt);

    rjs_value_set_undefined(rt, rv);

    return RJS_OK;
}

static const FuncDesc
test262_function_descs[] = {
    {
        "createRealm",
        0,
        test262_createRealm
    },
#if ENABLE_ARRAY_BUFFER
    {
        "detachArrayBuffer",
        1,
        test262_detachArrayBuffer
    },
#endif /*ENABLE_ARRAY_BUFFER*/
#if ENABLE_EVAL
    {
        "evalScript",
        1,
        test262_evalScript
    },
#endif /*ENABLE_EVAL*/
    {
        "gc",
        0,
        test262_gc
    },
    {NULL}
};

/*Agent thread entry.*/
static void*
agent_entry (void *arg)
{
    Agent       *agent = arg;
    RJS_Runtime *rt;
    RJS_Realm   *realm;
    RJS_Value   *str, *script;
    RJS_Result   r;

    pthread_setspecific(agent_key, agent);

    rt     = rjs_runtime_new();
    realm  = rjs_realm_current(rt);
    str    = rjs_value_stack_push(rt);
    script = rjs_value_stack_push(rt);

    load_host_functions(rt, realm);

    rjs_string_from_chars(rt, str, agent->src, -1);

    if ((r = rjs_script_from_string(rt, script, str, realm, RJS_FALSE)) == RJS_ERR) {
        fprintf(stderr, _("script parse error\n"));
        goto end;
    }

    r = rjs_script_evaluation(rt, script, NULL);
end:
    rjs_runtime_free(rt);
    return NULL;
}

/*$262.agent.start*/
static RJS_NF(agent_start)
{
    RJS_Value  *v   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *str = rjs_value_stack_push(rt);
    const char *src;
    Agent      *agent;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, v, str)) == RJS_ERR)
        goto end;

    src = rjs_string_to_enc_chars(rt, str, NULL, NULL);

    agent = malloc(sizeof(Agent));

    agent->src   = strdup(src);
    agent->flags = 0;
#if ENABLE_SHARED_ARRAY_BUFFER
    agent->db    = NULL;
    agent->off   = 0;
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

    pthread_cond_init(&agent->cond, NULL);
    rjs_list_append(&agent_list, &agent->ln);

    pthread_create(&agent->thread, NULL, agent_entry, agent);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

#if ENABLE_SHARED_ARRAY_BUFFER

/*$262.agent.receiveBroadcast*/
static RJS_NF(agent_receiveBroadcast)
{
    RJS_Value     *cb    = rjs_argument_get(rt, args, argc, 0);
    size_t         top   = rjs_value_stack_save(rt);
    RJS_Value     *sb    = rjs_value_stack_push(rt);
    RJS_Value     *idx   = rjs_value_stack_push(rt);
    RJS_DataBlock *db;
    size_t         off;
    Agent         *agent;
    RJS_Result     r;

    agent = pthread_getspecific(agent_key);

    pthread_mutex_lock(&agent_lock);

    while (!(agent->flags & MSG_BROADCAST)) {
        pthread_cond_wait(&agent->cond, &agent_lock);
    }

    db  = agent->db;
    off = agent->off;

    agent->db     = NULL;
    agent->flags &= ~MSG_BROADCAST;

    pthread_mutex_unlock(&agent_lock);

    rjs_allocate_shared_array_buffer(rt, NULL, rjs_data_block_get_size(db), db, sb);
    rjs_value_set_number(rt, idx, off);
    rjs_data_block_unref(db);

    r = rjs_call(rt, cb, rjs_v_undefined(rt), sb, 2, NULL);

    rjs_value_stack_restore(rt, top);

    return r;
}

/*$262.agent.broadcast*/
static RJS_NF(agent_broadcast)
{
    RJS_Value       *sb  = rjs_argument_get(rt, args, argc, 0);
    RJS_Value       *off = rjs_argument_get(rt, args, argc, 1);
    RJS_DataBlock   *db;
    RJS_Number       n;
    RJS_Result       r;
    Agent           *agent;

    if (!rjs_is_shared_array_buffer(rt, sb)) {
        r = rjs_throw_type_error(rt, _("the value is not a shared array buffer"));
        goto end;
    }

    db = rjs_array_buffer_get_data_block(rt, sb);

    if (!db) {
        r = rjs_throw_type_error(rt, _("the array buffer is detached"));
        goto end;
    }

    if ((r = rjs_to_number(rt, off, &n)) == RJS_ERR)
        goto end;

    pthread_mutex_lock(&agent_lock);

    rjs_list_foreach_c(&agent_list, agent, Agent, ln) {
        assert(!agent->db);

        agent->db     = rjs_data_block_ref(db);
        agent->off    = (size_t)off;
        agent->flags |= MSG_BROADCAST;

        pthread_cond_signal(&agent->cond);
    }

    pthread_mutex_unlock(&agent_lock);
end:
    return r;
}

#endif /*ENABLE_SHARED_ARRAY_BUFFER*/

/*$262.agent.sleep*/
static RJS_NF(agent_sleep)
{
    RJS_Value *v = rjs_argument_get(rt, args, argc, 0);
    RJS_Number n;
    RJS_Result r;

    if ((r = rjs_to_number(rt, v, &n)) == RJS_ERR)
        return r;

    if (n > 0)
        usleep(n * 1000);

    rjs_value_set_undefined(rt, rv);
    return RJS_OK;
}

/*$262.agent.monotonicNow*/
static RJS_NF(agent_monotonicNow)
{
    struct timespec ts;
    RJS_Number      now;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    rjs_value_set_number(rt, rv, now);
    return RJS_OK;
}

/*$262.agent.getReport*/
static RJS_NF(agent_getReport)
{
    pthread_mutex_lock(&agent_lock);

    if (rjs_list_is_empty(&report_list)) {
        rjs_value_set_undefined(rt, rv);
    } else {
        Report *r;

        r = RJS_CONTAINER_OF(report_list.next, Report, ln);

        rjs_list_remove(&r->ln);

        if (r->msg) {
            rjs_string_from_chars(rt, rv, r->msg, -1);
            free(r->msg);
        }

        free(r);
    }

    pthread_mutex_unlock(&agent_lock);

    return RJS_OK;
}

/*$262.agent.report*/
static RJS_NF(agent_report)
{
    RJS_Value  *v   = rjs_argument_get(rt, args, argc, 0);
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *str = rjs_value_stack_push(rt);
    const char *cstr;
    Report     *report;
    RJS_Result  r;

    if ((r = rjs_to_string(rt, v, str)) == RJS_ERR)
        goto end;

    cstr = rjs_string_to_enc_chars(rt, str, NULL, NULL);

    pthread_mutex_lock(&agent_lock);

    report = malloc(sizeof(Report));

    report->msg = strdup(cstr);
    rjs_list_append(&report_list, &report->ln);

    pthread_mutex_unlock(&agent_lock);

    r = RJS_OK;
end:
    rjs_value_stack_restore(rt, top);
    return r;
}

/*$262.agent_leaving*/
static RJS_NF(agent_leaving)
{
    return RJS_OK;
}

static const FuncDesc
agent_function_descs[] = {
    {
        "start",
        1,
        agent_start
    },
#if ENABLE_SHARED_ARRAY_BUFFER
    {
        "receiveBroadcast",
        0,
        agent_receiveBroadcast
    },
    {
        "broadcast",
        2,
        agent_broadcast
    },
#endif /*ENABLE_SHARED_ARRAY_BUFFER*/
    {
        "getReport",
        0,
        agent_getReport
    },
    {
        "report",
        1,
        agent_report
    },
    {
        "sleep",
        1,
        agent_sleep
    },
    {
        "leaving",
        0,
        agent_leaving
    },
    {
        "monotonicNow",
        0,
        agent_monotonicNow
    },
    {NULL}
};

static const ObjectDesc
test262_object_descs[] = {
    {
        "agent",
        agent_function_descs
    },
    {NULL}
};

static const ObjectDesc
host_object_descs[] = {
    {
        "$262",
        test262_function_descs,
        test262_object_descs
    },
    {NULL}
};

/*Load the descriptions.*/
static void
load_desc (RJS_Runtime *rt, RJS_Realm *realm, RJS_Value *o, const FuncDesc *funcs, const ObjectDesc *objs)
{
    size_t            top = rjs_value_stack_save(rt);
    RJS_Value        *pk  = rjs_value_stack_push(rt);
    RJS_Value        *pv  = rjs_value_stack_push(rt);
    const FuncDesc   *fd;
    const ObjectDesc *od;
    RJS_PropertyName  pn;

    if (funcs) {
        for (fd = funcs; fd->name; fd ++) {
            rjs_string_from_enc_chars(rt, pk, fd->name, -1, NULL);
            rjs_create_builtin_function(rt, NULL, fd->function, fd->length, pk, realm, NULL, NULL, pv);
            rjs_property_name_init(rt, &pn, pk);
            rjs_create_data_property(rt, o, &pn, pv);
            rjs_property_name_deinit(rt, &pn);
        }
    }

    if (objs) {
        for (od = objs; od->name; od ++) {
            rjs_string_from_enc_chars(rt, pk, od->name, -1, NULL);
            rjs_ordinary_object_create(rt, NULL, pv);
            load_desc(rt, realm, pv, od->functions, od->objects);
            rjs_property_name_init(rt, &pn, pk);
            rjs_create_data_property(rt, o, &pn, pv);
            rjs_property_name_deinit(rt, &pn);
        }
    }

    rjs_value_stack_restore(rt, top);
}

/*Load host defined functions.*/
int
load_host_functions (RJS_Runtime *rt, RJS_Realm *realm)
{
    size_t           top   = rjs_value_stack_save(rt);
    RJS_Value       *str   = rjs_value_stack_push(rt);
    RJS_Value       *o262  = rjs_value_stack_push(rt);
    RJS_PropertyName pn;

    load_desc(rt, realm, rjs_global_object(realm), host_function_descs, host_object_descs);

    rjs_string_from_chars(rt, str, "$262", -1);
    rjs_property_name_init(rt, &pn, str);
    rjs_get(rt, rjs_global_object(realm), &pn, o262);
    rjs_property_name_deinit(rt, &pn);

    rjs_string_from_chars(rt, str, "global", -1);
    rjs_property_name_init(rt, &pn, str);

    rjs_create_data_property_or_throw(rt, o262, &pn, rjs_global_object(realm));
    rjs_property_name_deinit(rt, &pn);

    rjs_value_stack_restore(rt, top);
    return 0;
}

/*Initialize the host resource.*/
void
host_init (void)
{
    async_end = 0;

    rjs_list_init(&agent_list);
    rjs_list_init(&report_list);
    rjs_list_init(&realm_list);

    pthread_mutex_init(&agent_lock, NULL);
    pthread_key_create(&agent_key, NULL);
}

/*Release the host resource.*/
void
host_deinit (void)
{
    Agent  *a, *na;
    Report *r, *nr;
    Realm  *re, *nre;

    rjs_list_foreach_safe_c(&agent_list, a, na, Agent, ln) {
        pthread_join(a->thread, NULL);
        agent_free(a);
    }

    rjs_list_foreach_safe_c(&report_list, r, nr, Report, ln) {
        if (r->msg)
            free(r->msg);

        free(r);
    }

    rjs_list_foreach_safe_c(&realm_list, re, nre, Realm, ln) {
        free(re);
    }

    pthread_mutex_destroy(&agent_lock);
    pthread_key_delete(agent_key);
}

/*Async wait.*/
int
async_wait (RJS_Runtime *rt)
{
    while (!async_end)
        rjs_solve_jobs(rt);

    if (async_end == -1)
        return -1;

    return 0;
}
