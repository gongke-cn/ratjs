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

#define RJS_LOG_TAG "test"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include "../src/lib/ratjs_internal.h"

static RJS_Runtime *rt;

static int test_total_num = 0;
static int test_pass_num  = 0;

/*Test.*/
static void
test (int v, const char *file, const char *func, int line, const char *msg)
{
    if (v) {
        test_pass_num ++;
    } else {
        rjs_log(RJS_LOG_ERROR, RJS_LOG_TAG, file, func, line, "test \"%s\" failed", msg);
    }

    test_total_num ++;
}

#define TEST(v)\
    test(v, __FILE__, __FUNCTION__, __LINE__, #v)

#define TEST_MSG(v, m)\
    test(v, __FILE__, __FUNCTION__, __LINE__, m)

/*Basic macros test.*/
static void
macro_test ()
{
    uint8_t  a8[32], b8[32];
    uint64_t a64[32], b64[32];
    int      i;

    RJS_ELEM_SET(a8, 0x7c, RJS_N_ELEM(a8));
    for (i = 0; i < RJS_N_ELEM(a8); i ++) {
        TEST(a8[i] == 0x7c);
        a8[i] = i;
    }

    RJS_ELEM_CPY(b8, a8, RJS_N_ELEM(a8));
    for (i = 0; i < RJS_N_ELEM(a8); i ++) {
        TEST(b8[i] == i);
    }

    TEST(RJS_ELEM_CMP(a8, b8, RJS_N_ELEM(a8)) == 0);

    b8[30] ++;

    TEST(RJS_ELEM_CMP(a8, b8, RJS_N_ELEM(a8)) < 0);

    RJS_ELEM_MOVE(a8 + 1, a8, RJS_N_ELEM(a8) - 1);
    TEST(a8[0] == 0);
    for (i = 1; i < RJS_N_ELEM(a8); i ++) {
        TEST(a8[i] == i - 1);
    }

    RJS_ELEM_SET(a64, 0x98765432, RJS_N_ELEM(a64));
    for (i = 0; i < RJS_N_ELEM(a64); i ++) {
        TEST(a64[i] == 0x98765432);
        a64[i] = i;
    }

    RJS_ELEM_CPY(b64, a64, RJS_N_ELEM(a64));
    for (i = 0; i < RJS_N_ELEM(a64); i ++) {
        TEST(b64[i] == i);
    }

    TEST(RJS_ELEM_CMP(a64, b64, RJS_N_ELEM(a64)) == 0);

    b64[30] ++;

    TEST(RJS_ELEM_CMP(a64, b64, RJS_N_ELEM(a64)) < 0);

    RJS_ELEM_MOVE(a64 + 1, a64, RJS_N_ELEM(a64) - 1);
    TEST(a64[0] == 0);
    for (i = 1; i < RJS_N_ELEM(a64); i ++) {
        TEST(a64[i] == i - 1);
    }
}

/*Test double linked list.*/
static void
list_test ()
{
    RJS_List list, l1, l2, *n, *t;
    RJS_List entries[1024];
    int      i;

    rjs_list_init(&list);
    TEST(rjs_list_is_empty(&list));

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        n = &entries[i];

        rjs_list_append(&list, n);
    }

    i = 0;
    rjs_list_foreach(&list, n) {
        TEST(n == &entries[i]);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    i = 0;
    rjs_list_foreach_safe(&list, n, t) {
        TEST(n == &entries[i]);
        rjs_list_remove(n);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    TEST(rjs_list_is_empty(&list));

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        n = &entries[i];

        rjs_list_prepend(&list, n);
    }

    i = 0;
    rjs_list_foreach(&list, n) {
        TEST(n == &entries[RJS_N_ELEM(entries) - i - 1]);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    rjs_list_init(&l1);
    rjs_list_init(&l2);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        n = &entries[i];
        rjs_list_append(&l1, n);
    }

    rjs_list_join(&l1, &l2);
    i = 0;
    rjs_list_foreach(&l1, n) {
        TEST(n == &entries[i]);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    rjs_list_join(&l2, &l1);
    i = 0;
    rjs_list_foreach(&l2, n) {
        TEST(n == &entries[i]);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    rjs_list_init(&l1);
    rjs_list_init(&l2);

    for (i = 0; i < RJS_N_ELEM(entries) / 2; i ++) {
        n = &entries[i];
        rjs_list_append(&l1, n);
    }

    for (; i < RJS_N_ELEM(entries); i ++) {
        n = &entries[i];
        rjs_list_append(&l2, n);
    }

    rjs_list_join(&l1, &l2);

    i = 0;
    rjs_list_foreach(&l1, n) {
        TEST(n == &entries[i]);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));
}

/*Container list test.*/
static void
container_list_test (void)
{
    typedef struct {
        int      v;
        RJS_List list;
    } Entry;

    RJS_List list;
    Entry    entries[1024], *e, *t;
    int      i;

    rjs_list_init(&list);
    TEST(rjs_list_is_empty(&list));

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        e = &entries[i];

        e->v = i;

        rjs_list_append(&list, &e->list);
    }

    i = 0;
    rjs_list_foreach_c(&list, e, Entry, list) {
        TEST(e->v == i);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    i = 0;
    rjs_list_foreach_safe_c(&list, e, t, Entry, list) {
        TEST(e->v == i);
        rjs_list_remove(&e->list);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));
    TEST(rjs_list_is_empty(&list));
}

/*String key hash table functions.*/

static void*
realloc_fn (void *data, void *optr, size_t osize, size_t nsize)
{
    return realloc(optr, nsize);
}

static size_t
key_fn (void *data, void *key)
{
    char  *c = key;
    size_t v = 0x19781009;

    while (*c) {
        v = (v << 5) | *c;
        c ++;
    }

    return v;
}

static RJS_Bool
equal_fn (void *data, void *k1, void *k2)
{
    char *s1 = k1;
    char *s2 = k2;

    return strcmp(s1, s2) == 0;
}

static const RJS_HashOps
hash_ops = {
    realloc_fn,
    key_fn,
    equal_fn
};

/*Hash table test.*/
static void
hash_test (void)
{
    RJS_Hash       hash;
    RJS_HashEntry  entris[1024];
    int            itab[1024];
    RJS_HashEntry *e, *t, **pe;
    size_t         i;
    RJS_Result     r;
    int            max_list_len;
    char           key[256];

    rjs_hash_init(&hash);

    TEST(rjs_hash_get_size(&hash) == 0);

    for (i = 0; i < RJS_N_ELEM(entris); i ++) {
        snprintf(key, sizeof(key), "%"PRIdPTR, i);
        r = rjs_hash_lookup(&hash, key, &e, &pe, &hash_ops, NULL);
        TEST(r == RJS_FALSE);

        e = &entris[i];
        rjs_hash_insert(&hash, strdup(key), e, pe, &hash_ops, NULL);
        TEST(rjs_hash_get_size(&hash) == i + 1);
    }

    max_list_len = 0;
    for (i = 0; i < hash.list_num; i ++) {
        int n = 0;

        for (e = hash.lists[i]; e; e = e->next)
            n ++;

        max_list_len = RJS_MAX(max_list_len, n);
    }
    RJS_LOGD("max list length in hash table: %d", max_list_len);

    memset(itab, 0, sizeof(itab));
    rjs_hash_foreach(&hash, i, e) {
        char *str = e->key;
        int   v;
        
        TEST(sscanf(str, "%d", &v) == 1);
        TEST(itab[v] == 0);
        itab[v] = 1;
    }

    for (i = 0; i < RJS_N_ELEM(entris); i ++) {
        TEST(itab[i] == 1);
    }

    for (i = 0; i < RJS_N_ELEM(entris); i ++) {
        snprintf(key, sizeof(key), "%"PRIdPTR, i);
        r = rjs_hash_lookup(&hash, key, &e, &pe, &hash_ops, NULL);
        TEST(r == RJS_TRUE);
        TEST(e == &entris[i]);
        
        rjs_hash_remove(&hash, pe, NULL);
        TEST(rjs_hash_get_size(&hash) == RJS_N_ELEM(entris) - i - 1);
    }

    for (i = 0; i < RJS_N_ELEM(entris); i ++) {
        free(entris[i].key);
    }
    rjs_hash_deinit(&hash, &hash_ops, NULL);

    rjs_hash_init(&hash);
    for (i = 0; i < RJS_N_ELEM(entris); i ++) {
        snprintf(key, sizeof(key), "%"PRIdPTR, i);
        rjs_hash_lookup(&hash, key, &e, &pe, &hash_ops, NULL);

        e = &entris[i];
        rjs_hash_insert(&hash, strdup(key), e, pe, &hash_ops, NULL);
    }

    rjs_hash_foreach_safe(&hash, i, e, t) {
        rjs_hash_lookup(&hash, e->key, NULL, &pe, &hash_ops, NULL);
        rjs_hash_remove(&hash, pe, NULL);
    }

    TEST(rjs_hash_get_size(&hash) == 0);

    for (i = 0; i < RJS_N_ELEM(entris); i ++) {
        free(entris[i].key);
    }
    rjs_hash_deinit(&hash, &hash_ops, NULL);
}

/*Container hash test.*/
static void
container_hash_test (void)
{
    typedef struct {
        int v;
        RJS_HashEntry hash;
    } Entry;

    RJS_Hash   hash;
    Entry      entries[1024];
    int        itab[1024];
    Entry     *e, *t;
    size_t     i;
    char       key[128];

    rjs_hash_init(&hash);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        snprintf(key, sizeof(key), "%"PRIdPTR, i);

        e = &entries[i];
        e->v = i;

        rjs_hash_insert(&hash, strdup(key), &e->hash, NULL, &hash_ops, NULL);
        TEST(rjs_hash_get_size(&hash) == i + 1);
    }

    memset(itab, 0, sizeof(itab));
    rjs_hash_foreach_c(&hash, i, e, Entry, hash) {
        TEST(itab[e->v] == 0);
        itab[e->v] = 1;
    }

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        TEST(itab[i] == 1);
    }

    rjs_hash_foreach_safe_c(&hash, i, e, t, Entry, hash) {
        RJS_HashEntry **pe;

        rjs_hash_lookup(&hash, e->hash.key, NULL, &pe, &hash_ops, NULL);
        rjs_hash_remove(&hash, pe, NULL);
    }
    TEST(rjs_hash_get_size(&hash) == 0);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        e = &entries[i];
        free(e->hash.key);
    }

    rjs_hash_deinit(&hash, &hash_ops, NULL);
}

static void
int_init (RJS_Runtime *rt, int *p, size_t n)
{
    size_t i;

    for (i = 0; i < n; i ++) {
        p[i] = 0x19781009;
    }
}

/*Vector test.*/
static void
vector_test (void)
{
    RJS_VECTOR_DECL(int) vec;
    int i, *pi;

    rjs_vector_init(&vec);
    TEST(rjs_vector_get_size(&vec) == 0);

    for (i = 0; i < 1024; i ++) {
        rjs_vector_set_item(&vec, i, i, rt);
        TEST(rjs_vector_get_size(&vec) == i + 1);
    }

    for (i = 0; i < 1024; i ++) {
        TEST(rjs_vector_get_item(&vec, i) == i);
    }

    rjs_vector_foreach(&vec, i, pi) {
        TEST(*pi == i);
    }
    TEST(i == 1024);

    rjs_vector_deinit(&vec, rt);

    rjs_vector_init(&vec);

    rjs_vector_resize_init(&vec, 10, rt, int_init);
    TEST(rjs_vector_get_size(&vec) == 10);
    for (i = 0; i < rjs_vector_get_size(&vec); i ++) {
        TEST(rjs_vector_get_item(&vec, i) == 0x19781009);
    }

    rjs_vector_deinit(&vec, rt);

    rjs_vector_init(&vec);
    for (i = 0; i < 1024; i ++) {
        rjs_vector_append(&vec, i, rt);
    }
    TEST(rjs_vector_get_size(&vec) == 1024);
    for (i = 0; i < 1024; i ++) {
        TEST(rjs_vector_get_item(&vec, i) == i);
    }
    rjs_vector_deinit(&vec, rt);
}

/*Red black tree entry.*/
typedef struct {
    int     v;   /**< The value.*/
    RJS_Rbt rbt; /**< The node of the tree.*/
} RbtEntry;

/*Lookup the node.*/
static RJS_Result
rbt_lookup (RJS_Rbt **root, int v, RbtEntry **e, RJS_Rbt **pp, RJS_Rbt ***ppos)
{
    RJS_Rbt   *n, *p, **pos;
    RJS_Result r = RJS_FALSE;

    pos = root;
    p   = NULL;

    while ((n = *pos)) {
        RbtEntry *t = RJS_CONTAINER_OF(n, RbtEntry, rbt);

        if (t->v == v) {
            *e = t;
            r  = RJS_TRUE;
            break;
        } else if (t->v > v) {
            pos = &n->left;
        } else {
            pos = &n->right;
        }

        p = n;
    }

    *pp   = p;
    *ppos = pos;

    return r;
}

/*Get the black nodes' number.*/
static int
rbt_node_black (RJS_Rbt *n)
{
    int b = 0;

    if (!n)
        return 0;

    if (n->parent_color & 1)
        b ++;

    return rbt_node_black(n->left) + b;
}

/*Check if the red black tree node is ok.*/
static void
rbt_node_check (RJS_Rbt *n)
{
    RJS_Rbt *p = rjs_rbt_get_parent(n);

    if (!p) {
        TEST(n->parent_color & 1);
    } else {
        TEST((n->parent_color & 1) || (p->parent_color & 1));
    }

    TEST(rbt_node_black(n->left) == rbt_node_black(n->right));
}

/*Check if the red black tree is ok.*/
static void
rbt_check (RJS_Rbt **root)
{
    RJS_Rbt *n = *root;

    n = rjs_rbt_first(root);
    while (n) {
        rbt_node_check(n);
        n = rjs_rbt_next(n);
    }
}

/*Red black tree test.*/
static void
rbt_test (void)
{
    RJS_Rbt   *root, *n, *p, **pos;
    RbtEntry   entries[1024], *e;
    int        imap[1024];
    int        i;
    RJS_Result r;

    /*add items from 0 to 1023*/
    rjs_rbt_init(&root);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_FALSE);

        e = &entries[i];
        e->v = i;

        rjs_rbt_link(&e->rbt, p, pos);
        rjs_rbt_insert(&root, &e->rbt);
    }

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_TRUE);
        TEST(e->v == i);
        TEST(e == &entries[i]);
    }

    i = 0;
    n = rjs_rbt_first(&root);
    while (n) {
        e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
        TEST(e->v == i);
        n = rjs_rbt_next(n);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    i = 0;
    n = rjs_rbt_last(&root);
    while (n) {
        e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
        TEST(e->v == RJS_N_ELEM(entries) - i - 1);
        n = rjs_rbt_prev(n);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    rbt_check(&root);

    /*add items from 1023 to 0*/
    rjs_rbt_init(&root);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, RJS_N_ELEM(entries) - i - 1, &e, &p, &pos);
        TEST(r == RJS_FALSE);

        e = &entries[i];
        e->v = RJS_N_ELEM(entries) - i - 1;

        rjs_rbt_link(&e->rbt, p, pos);
        rjs_rbt_insert(&root, &e->rbt);
    }

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_TRUE);
        TEST(e->v == i);
        TEST(e == &entries[RJS_N_ELEM(entries) - i - 1]);
    }

    i = 0;
    n = rjs_rbt_first(&root);
    while (n) {
        e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
        TEST(e->v == i);
        n = rjs_rbt_next(n);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    i = 0;
    n = rjs_rbt_last(&root);
    while (n) {
        e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
        TEST(e->v == RJS_N_ELEM(entries) - i - 1);
        n = rjs_rbt_prev(n);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    rbt_check(&root);

    /*random add.*/
    for (i = 0; i < RJS_N_ELEM(imap); i ++)
        imap[i] = i;
    for (i = 0; i < RJS_N_ELEM(imap); i ++) {
        int j = rand() % RJS_N_ELEM(imap);
        int t;

        t = imap[i];
        imap[i] = imap[j];
        imap[j] = t;
    }

    rjs_rbt_init(&root);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, imap[i], &e, &p, &pos);
        TEST(r == RJS_FALSE);

        e = &entries[i];
        e->v = imap[i];

        rjs_rbt_link(&e->rbt, p, pos);
        rjs_rbt_insert(&root, &e->rbt);
    }

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_TRUE);
        TEST(e->v == i);
    }

    i = 0;
    n = rjs_rbt_first(&root);
    while (n) {
        e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
        TEST(e->v == i);
        n = rjs_rbt_next(n);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    i = 0;
    n = rjs_rbt_last(&root);
    while (n) {
        e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
        TEST(e->v == RJS_N_ELEM(entries) - i - 1);
        n = rjs_rbt_prev(n);
        i ++;
    }
    TEST(i == RJS_N_ELEM(entries));

    rbt_check(&root);

    /*remove from 0 -> 1023.*/
    rjs_rbt_init(&root);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_FALSE);

        e = &entries[i];
        e->v = i;

        rjs_rbt_link(&e->rbt, p, pos);
        rjs_rbt_insert(&root, &e->rbt);
    }

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        int v;

        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_TRUE);

        rjs_rbt_remove(&root, &e->rbt);
        rbt_check(&root);

        n = rjs_rbt_first(&root);
        v = i + 1;
        while (n) {
            e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
            TEST(e->v == v);
            n = rjs_rbt_next(n);
            v ++;
        }
        TEST(v == RJS_N_ELEM(entries));
    }

    /*remove from 1023 -> 0.*/
    rjs_rbt_init(&root);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_FALSE);

        e = &entries[i];
        e->v = i;

        rjs_rbt_link(&e->rbt, p, pos);
        rjs_rbt_insert(&root, &e->rbt);
    }

    for (i = RJS_N_ELEM(entries) - 1; i >= 0; i --) {
        int v;

        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_TRUE);

        rjs_rbt_remove(&root, &e->rbt);
        rbt_check(&root);

        n = rjs_rbt_first(&root);
        v = 0;
        while (n) {
            e = RJS_CONTAINER_OF(n, RbtEntry, rbt);
            TEST(e->v == v);
            n = rjs_rbt_next(n);
            v ++;
        }
        TEST(v == i);
    }

    /*random remove.*/
    rjs_rbt_init(&root);

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, i, &e, &p, &pos);
        TEST(r == RJS_FALSE);

        e = &entries[i];
        e->v = i;

        rjs_rbt_link(&e->rbt, p, pos);
        rjs_rbt_insert(&root, &e->rbt);
    }

    for (i = 0; i < RJS_N_ELEM(entries); i ++)
        imap[i] = i;
    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        int j = rand() % RJS_N_ELEM(entries);
        int v = imap[i];

        imap[i] = imap[j];
        imap[j] = v;
    }

    for (i = 0; i < RJS_N_ELEM(entries); i ++) {
        r = rbt_lookup(&root, imap[i], &e, &p, &pos);
        TEST(r == RJS_TRUE);

        rjs_rbt_remove(&root, &e->rbt);
        rbt_check(&root);
    }
}

static size_t thing_num = 0;

typedef struct {
    RJS_GcThing gc_thing;
    void       *p1;
    void       *p2;
} Thing;

static void
thing_op_scan (RJS_Runtime *rt, void *ptr)
{
    Thing *th = ptr;

    if (th->p1)
        rjs_gc_mark(rt, th->p1);
    if (th->p2)
        rjs_gc_mark(rt, th->p2);
}

static void
thing_op_free (RJS_Runtime *rt, void *ptr)
{
    thing_num --;
}

static const RJS_GcThingOps
thing_ops = {
    0x100,
    thing_op_scan,
    thing_op_free
};

/*GC test.*/
static void
gc_test (void)
{
    size_t       top = rjs_value_stack_save(rt);
    RJS_Value   *tmp;
    Thing things[1024], *th;
    int   i;

    for (i = 0; i < RJS_N_ELEM(things); i ++) {
        th = &things[i];

        th->p1 = NULL;
        th->p2 = NULL;

        rjs_gc_add(rt, th, &thing_ops);
        thing_num ++;
    }
    //TEST(thing_num == RJS_N_ELEM(things));

    rjs_gc_run(rt);
    TEST(thing_num == 0);

    tmp = rjs_value_stack_push(rt);

    for (i = 0; i < RJS_N_ELEM(things); i ++) {
        th = &things[i];

        th->p1 = NULL;
        th->p2 = NULL;

        if (i == 0) {
            rjs_value_set_gc_thing(rt, tmp, th);
        } else {
            Thing *pth = &things[i - 1];

            pth->p1 = th;
        }

        rjs_gc_add(rt, th, &thing_ops);
        thing_num ++;
    }
    TEST(thing_num == RJS_N_ELEM(things));

    rjs_gc_run(rt);
    TEST(thing_num == RJS_N_ELEM(things));

    rjs_value_stack_restore(rt, top);
    rjs_gc_run(rt);
    TEST(thing_num == 0);
}

/*String test.*/
static void
string_test (void)
{
    size_t       top = rjs_value_stack_save(rt);
    RJS_Value   *s1  = rjs_value_stack_push(rt);
    RJS_Value   *s2  = rjs_value_stack_push(rt);
    char      chars[1024];
    RJS_UChar uchars[1024];
    size_t    i;

    for (i = 0; i < RJS_N_ELEM(chars); i ++) {
        chars[i] = i % 26 + 'a';
        uchars[i] = i % 26 + 'A';
    }

    rjs_string_from_chars(rt, s1, chars, RJS_N_ELEM(chars));
    TEST(rjs_string_get_length(rt, s1) == RJS_N_ELEM(chars));
    for (i = 0; i < RJS_N_ELEM(chars); i ++)
        TEST(rjs_string_get_uchar(rt, s1, i) == i % 26 + 'a');

    chars[RJS_N_ELEM(chars) - 1] = 0;
    rjs_string_from_chars(rt, s1, chars, -1);
    TEST(rjs_string_get_length(rt, s1) == RJS_N_ELEM(chars) - 1);
    for (i = 0; i < RJS_N_ELEM(chars) - 1; i ++)
        TEST(rjs_string_get_uchar(rt, s1, i) == i % 26 + 'a');

    rjs_string_from_uchars(rt, s1, uchars, RJS_N_ELEM(uchars));
    TEST(rjs_string_get_length(rt, s1) == RJS_N_ELEM(uchars));
    for (i = 0; i < RJS_N_ELEM(uchars); i ++)
        TEST(rjs_string_get_uchar(rt, s1, i) == i % 26 + 'A');

    uchars[RJS_N_ELEM(uchars) - 1] = 0;
    rjs_string_from_uchars(rt, s1, uchars, -1);
    TEST(rjs_string_get_length(rt, s1) == RJS_N_ELEM(uchars) - 1);
    for (i = 0; i < RJS_N_ELEM(uchars) - 1; i ++)
        TEST(rjs_string_get_uchar(rt, s1, i) == i % 26 + 'A');

    rjs_string_from_static_uchars(rt, s1, uchars, -1);
    TEST(rjs_string_get_length(rt, s1) == RJS_N_ELEM(uchars) - 1);
    for (i = 0; i < RJS_N_ELEM(uchars) - 1; i ++)
        TEST(rjs_string_get_uchar(rt, s1, i) == i % 26 + 'A');

    rjs_string_from_chars(rt, s1, "abcdefghijklmn", -1);
    rjs_string_from_chars(rt, s2, "abcdefghijklmn", -1);
    TEST(rjs_value_get_string(rt, s1) != rjs_value_get_string(rt, s2));

    rjs_string_to_property_key(rt, s1);
    rjs_string_to_property_key(rt, s2);
    TEST(rjs_value_get_string(rt, s1) == rjs_value_get_string(rt, s2));

    rjs_string_from_chars(rt, s2, "abcdefghijklmN", -1);
    rjs_string_to_property_key(rt, s2);
    TEST(rjs_value_get_string(rt, s1) != rjs_value_get_string(rt, s2));

    rjs_value_stack_restore(rt, top);
}

/*Character convertor test.*/
static void
conv_test (void)
{
    size_t       top  = rjs_value_stack_save(rt);
    RJS_Value   *s    = rjs_value_stack_push(rt);
    char        *cstr = "天地玄黄，宇宙洪荒";
    RJS_CharBuffer cb;
    const char  *ocstr;

    rjs_string_from_enc_chars(rt, s, cstr, -1, NULL);
    TEST(rjs_string_get_length(rt, s) == 9);

    rjs_char_buffer_init(rt, &cb);
    ocstr = rjs_string_to_enc_chars(rt, s, &cb, NULL);
    TEST(!strcmp(cstr, ocstr));
    rjs_char_buffer_deinit(rt, &cb);

    rjs_value_stack_restore(rt, top);
}

/*Run test cases for an input instance.*/
static void
input_inst_test (RJS_Input *input)
{
    int line = 1, col = 0;
    int chars[200];

    while (1) {
        RJS_Location loc;
        int          c;

        c = rjs_input_get_uc(rt, input);
        if (c == RJS_INPUT_END)
            break;

        rjs_input_get_location(input, &loc);

        chars[col] = c;

        col ++;
        TEST(line == loc.first_line && line == loc.last_line);
        TEST(col == loc.first_column && col == loc.last_column);

        if (c == '\n') {
            int p, n;

            TEST(col == line);
            line ++;

            n = RJS_MIN(16, col);
            p = col;
            while (n > 0) {
                rjs_input_unget_uc(rt, input, chars[p - 1]);
                p --;
                n --;
            }

            while (p < col) {
                c = rjs_input_get_uc(rt, input);
                TEST(c == chars[p]);

                p ++;
            }

            col = 0;
        } else {
            TEST(c == (col - 1) % 26 + 'a');
        }

        /*rjs_note(rt, input, &loc, "char: %04x", c);*/
    }

    TEST(line == 101);
}

/*Input test.*/
static void
input_test (void)
{
    size_t          top = rjs_value_stack_save(rt);
    RJS_Value      *s   = rjs_value_stack_push(rt);
    const char     *cstr;
    RJS_CharBuffer  cb;
    RJS_Input       si;
    RJS_Input       fi;
    FILE           *fp;
    int             i, j;

    rjs_char_buffer_init(rt, &cb);

    for (i = 0; i < 100; i ++) {
        for (j = 0; j < i; j ++) {
            rjs_char_buffer_append_char(rt, &cb, j % 26 + 'a');
        }
        rjs_char_buffer_append_char(rt, &cb, '\n');
    }

    cstr = rjs_char_buffer_to_c_string(rt, &cb);
    rjs_string_from_chars(rt, s, cstr, -1);
    rjs_string_input_init(rt, &si, s);
    input_inst_test(&si);
    rjs_input_deinit(rt, &si);

    fp = fopen("test.txt", "wb");
    fwrite(cstr, 1, strlen(cstr), fp);
    fclose(fp);
    rjs_file_input_init(rt, &fi, "test.txt", NULL);
    input_inst_test(&fi);
    rjs_input_deinit(rt, &fi);
    unlink("test.txt");

    rjs_char_buffer_deinit(rt, &cb);
    rjs_value_stack_restore(rt, top);
}

/*Unicode character test.*/
static void
uchar_test (void)
{
    TEST(rjs_uchar_is_line_terminator('\n'));
    TEST(rjs_uchar_is_line_terminator('\r'));
    TEST(rjs_uchar_is_line_terminator(0x2028));
    TEST(rjs_uchar_is_line_terminator(0x2029));
    TEST(!rjs_uchar_is_line_terminator(' '));
    TEST(!rjs_uchar_is_line_terminator('a'));

    TEST(rjs_uchar_is_white_space(' '));
    TEST(rjs_uchar_is_white_space('\t'));
    TEST(rjs_uchar_is_white_space('\f'));
    TEST(rjs_uchar_is_white_space('\v'));
    TEST(rjs_uchar_is_white_space(0xfeff));
    TEST(rjs_uchar_is_white_space(0x202f));
    TEST(rjs_uchar_is_white_space(0x205f));
    TEST(!rjs_uchar_is_white_space('a'));

    TEST(rjs_uchar_is_id_start('$'));
    TEST(rjs_uchar_is_id_start('_'));
    TEST(rjs_uchar_is_id_start('a'));
    TEST(rjs_uchar_is_id_start('z'));
    TEST(!rjs_uchar_is_id_start('0'));
    TEST(!rjs_uchar_is_id_start('9'));

    TEST(rjs_uchar_is_id_continue('$'));
    TEST(rjs_uchar_is_id_continue('_'));
    TEST(rjs_uchar_is_id_continue('a'));
    TEST(rjs_uchar_is_id_continue('z'));
    TEST(rjs_uchar_is_id_continue('0'));
    TEST(rjs_uchar_is_id_continue('9'));
    TEST(rjs_uchar_is_id_continue(0x200c));
    TEST(rjs_uchar_is_id_continue(0x200d));
}

/*Lexical test case*/
static void
lex_case (const char *src, ...)
{
    size_t          top = rjs_value_stack_save(rt);
    RJS_Input       si;
    RJS_Lex         lex;
    RJS_Token       token;
    RJS_CharBuffer  cb1, cb2;
    RJS_Value      *str = rjs_value_stack_push(rt);
    RJS_Result      r;
    va_list         ap;
    RJS_TokenType   tt;

    rjs_string_from_chars(rt, str, src, -1);
    rjs_string_input_init(rt, &si, str);
    rjs_lex_init(rt, &lex, &si);
    rjs_char_buffer_init(rt, &cb1);
    rjs_char_buffer_init(rt, &cb2);
    rjs_token_init(rt, &token);

    rjs_value_set_undefined(rt, token.value);

    va_start(ap, src);

    while (1) {
        r = rjs_lex_get_token(rt, &lex, &token);
        assert(r == RJS_OK);

        tt = va_arg(ap, RJS_TokenType);
        //RJS_LOGD("xxxx %s %s", rjs_token_type_get_name(tt), rjs_token_type_get_name(token.type));
        TEST_MSG(tt == token.type, src);

        if (token.type == RJS_TOKEN_END)
            break;

        switch (token.type) {
        case RJS_TOKEN_NUMBER: {
            RJS_Number n;

            n = va_arg(ap, RJS_Number);

            TEST_MSG(rjs_value_get_type(rt, token.value) == RJS_VALUE_NUMBER, src);
            TEST_MSG(rjs_value_get_number(rt, token.value) == n, src);
            break;
        }
        case RJS_TOKEN_REGEXP: {
            break;
        }
        case RJS_TOKEN_STRING:
        case RJS_TOKEN_IDENTIFIER: {
            const char *str, *rstr;

            str = va_arg(ap, const char*);

            TEST_MSG(rjs_value_get_type(rt, token.value) == RJS_VALUE_STRING, src);
            rstr = rjs_string_to_enc_chars(rt, token.value, &cb1, NULL);
            TEST_MSG(!strcmp(str, rstr), src);
            break;
        }
        case RJS_TOKEN_TEMPLATE:
        case RJS_TOKEN_TEMPLATE_HEAD:
        case RJS_TOKEN_TEMPLATE_MIDDLE:
        case RJS_TOKEN_TEMPLATE_TAIL: {
            const char *str, *raw;
            RJS_AstTemplateEntry *te;
            const char *rstr, *rraw;

            str = va_arg(ap, const char*);
            raw = va_arg(ap, const char*);

            te = rjs_value_get_gc_thing(rt, token.value);

            TEST_MSG(rjs_value_get_type(rt, &te->str) == RJS_VALUE_STRING, src);
            rstr = rjs_string_to_enc_chars(rt, &te->str, &cb1, NULL);
            TEST_MSG(!strcmp(str, rstr), src);

            TEST_MSG(rjs_value_get_type(rt, &te->raw_str) == RJS_VALUE_STRING, src);
            rraw = rjs_string_to_enc_chars(rt, &te->raw_str, &cb1, NULL);
            TEST_MSG(!strcmp(raw, rraw), src);
            break;
        }
        default:
            break;
        }
    }

    va_end(ap);

    rjs_token_deinit(rt, &token);
    rjs_char_buffer_deinit(rt, &cb1);
    rjs_char_buffer_deinit(rt, &cb2);
    rjs_lex_deinit(rt, &lex);
    rjs_input_deinit(rt, &si);
    rjs_value_stack_restore(rt, top);
}

/*Lexical analyzer test.*/
static void
lex_test (void)
{
    lex_case("true;", RJS_TOKEN_IDENTIFIER, "true", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("false;", RJS_TOKEN_IDENTIFIER, "false", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("null;", RJS_TOKEN_IDENTIFIER, "null", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("if;", RJS_TOKEN_IDENTIFIER, "if", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("if_;", RJS_TOKEN_IDENTIFIER, "if_", RJS_TOKEN_semicolon, RJS_TOKEN_END);

    lex_case("0;", RJS_TOKEN_NUMBER, 0., RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("1;", RJS_TOKEN_NUMBER, 1., RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0b11110000;", RJS_TOKEN_NUMBER, (double)0xf0, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0B00001111;", RJS_TOKEN_NUMBER, (double)0x0f, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0B1111_0000_1111_0000;", RJS_TOKEN_NUMBER, (double)0xf0f0, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0o377;", RJS_TOKEN_NUMBER, (double)0xff, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0O377;", RJS_TOKEN_NUMBER, (double)0xff, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0o3_77;", RJS_TOKEN_NUMBER, (double)0xff, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0x1457abef;", RJS_TOKEN_NUMBER, (double)0x1457abef, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0X1457ABEF;", RJS_TOKEN_NUMBER, (double)0x1457abef, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("0x1457_abef;", RJS_TOKEN_NUMBER, (double)0x1457abef, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("3.1415926;", RJS_TOKEN_NUMBER, 3.1415926, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case(".123456789;", RJS_TOKEN_NUMBER, 0.123456789, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("100.;", RJS_TOKEN_NUMBER, 100.0, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("1.2e10;", RJS_TOKEN_NUMBER, 1.2e10, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("1.2e-10;", RJS_TOKEN_NUMBER, 1.2e-10, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("1e10;", RJS_TOKEN_NUMBER, 1e10, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("1.e10;", RJS_TOKEN_NUMBER, 1e10, RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case(".1e10;", RJS_TOKEN_NUMBER, 1e9, RJS_TOKEN_semicolon, RJS_TOKEN_END);

    lex_case("\'\';", RJS_TOKEN_STRING, "", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\"\';", RJS_TOKEN_STRING, "\"", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'a\';", RJS_TOKEN_STRING, "a", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\n\';", RJS_TOKEN_STRING, "\n", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\t\';", RJS_TOKEN_STRING, "\t", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\r\';", RJS_TOKEN_STRING, "\r", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\f\';", RJS_TOKEN_STRING, "\f", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\v\';", RJS_TOKEN_STRING, "\v", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\b\';", RJS_TOKEN_STRING, "\b", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\\'\';", RJS_TOKEN_STRING, "\'", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\\n\';", RJS_TOKEN_STRING, "", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\0a\';", RJS_TOKEN_STRING, "", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\x5a\';", RJS_TOKEN_STRING, "Z", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\u005A\';", RJS_TOKEN_STRING, "Z", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\u{005A}\';", RJS_TOKEN_STRING, "Z", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\u{5A}\';", RJS_TOKEN_STRING, "Z", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("\'\\132\';", RJS_TOKEN_STRING, "Z", RJS_TOKEN_semicolon, RJS_TOKEN_END);

    lex_case("`abcdefg`;", RJS_TOKEN_TEMPLATE, "abcdefg", "abcdefg", RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("`abc${a}defg`;",
            RJS_TOKEN_TEMPLATE_HEAD, "abc", "abc",
            RJS_TOKEN_IDENTIFIER, "a",
            RJS_TOKEN_TEMPLATE_TAIL, "defg", "defg",
            RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("`abc${a}de${b}fg`;",
            RJS_TOKEN_TEMPLATE_HEAD, "abc", "abc",
            RJS_TOKEN_IDENTIFIER, "a",
            RJS_TOKEN_TEMPLATE_MIDDLE, "de", "de",
            RJS_TOKEN_IDENTIFIER, "b",
            RJS_TOKEN_TEMPLATE_TAIL, "fg", "fg",
            RJS_TOKEN_semicolon, RJS_TOKEN_END);
    lex_case("`abc${{}}defg`;",
            RJS_TOKEN_TEMPLATE_HEAD, "abc", "abc",
            RJS_TOKEN_lbrace,
            RJS_TOKEN_rbrace,
            RJS_TOKEN_TEMPLATE_TAIL, "defg", "defg",
            RJS_TOKEN_semicolon, RJS_TOKEN_END);

    lex_case("let", RJS_TOKEN_IDENTIFIER, "let", RJS_TOKEN_END);
}

#if ENABLE_SCRIPT

/*Run a parser case.*/
static void
script_case (const char *src)
{
    size_t      top = rjs_value_stack_save(rt);
    RJS_Value  *str = rjs_value_stack_push(rt);
    RJS_Value  *s   = rjs_value_stack_push(rt);
    RJS_Input   si;
    RJS_Result  r;

    rjs_string_from_chars(rt, str, src, -1);
    rjs_string_input_init(rt, &si, str);

    r = rjs_parse_script(rt, &si, NULL, 0, s);
    TEST_MSG(r == RJS_OK, src);

    rjs_input_deinit(rt, &si);
    rjs_value_stack_restore(rt, top);
}

#endif /*ENABLE_SCRIPT*/

/*Parser test.*/
static void
parser_test (void)
{
#if ENABLE_SCRIPT
    script_case("");
    /*script_case(";");
    script_case("var a");
    script_case("var a,b=1");
    script_case("let a=1");
    script_case("let[a]=1");*/
    //script_case("a=>-a");
    /*script_case("a={a:1}");
    script_case("l2: l1: while(1) continue l2");
    script_case("[a,b,...c]=[1,2,3]");*/
#endif /*ENABLE_SCRIPT*/
}

/*Object test.*/
static void
object_test (void)
{
    size_t     top    = rjs_value_stack_save(rt);
    RJS_Value *o      = rjs_value_stack_push(rt);
    RJS_Value *keys   = rjs_value_stack_push(rt);
    RJS_Value *p      = rjs_value_stack_push(rt);
    RJS_Value *v      = rjs_value_stack_push(rt);
    RJS_PropertyKeyList *pkl;
    RJS_PropertyName     pn;
    int        i;
    RJS_Result r;

    r = rjs_object_new(rt, o, NULL);
    TEST(r == RJS_OK);

    for (i = 0; i < 26; i ++) {
        char c = 'a' + i;

        rjs_string_from_chars(rt, p, &c, 1);
        rjs_value_set_number(rt, v, i);
        rjs_property_name_init(rt, &pn, p);
        r = rjs_create_data_property(rt, o, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_OK);
    }

    for (i = 0; i < 1024; i ++) {
        char buf[64];

        snprintf(buf, sizeof(buf), "%d", i);

        rjs_string_from_chars(rt, p, buf, -1);
        rjs_value_set_number(rt, v, i);
        rjs_property_name_init(rt, &pn, p);
        r = rjs_create_data_property(rt, o, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_OK);
    }

    for (i = 0; i < 26; i ++) {
        char c = 'a' + i;

        rjs_string_from_chars(rt, p, &c, 1);
        rjs_property_name_init(rt, &pn, p);
        r = rjs_get(rt, o, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_OK);
        TEST(rjs_value_is_number(rt, v));
        TEST(rjs_value_get_number(rt, v) == i);
    }

    for (i = 0; i < 1024; i ++) {
        char buf[64];

        snprintf(buf, sizeof(buf), "%d", i);

        rjs_string_from_chars(rt, p, buf, -1);
        rjs_property_name_init(rt, &pn, p);
        r = rjs_get(rt, o, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_OK);
        TEST(rjs_value_is_number(rt, v));
        TEST(rjs_value_get_number(rt, v) == i);
    }

    for (i = 0; i < 1024; i ++) {
        char buf[64];

        snprintf(buf, sizeof(buf), "%u", 0xfffffffe - i);

        rjs_string_from_chars(rt, p, buf, -1);
        rjs_value_set_number(rt, v, i);
        rjs_property_name_init(rt, &pn, p);
        r = rjs_create_data_property(rt, o, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_OK);
    }

    for (i = 0; i < 1024; i ++) {
        char buf[64];

        snprintf(buf, sizeof(buf), "%d", i);

        rjs_string_from_chars(rt, p, buf, -1);
        rjs_property_name_init(rt, &pn, p);
        r = rjs_get(rt, o, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_OK);
        TEST(rjs_value_is_number(rt, v));
        TEST(rjs_value_get_number(rt, v) == i);
    }

    for (i = 0; i < 1024; i ++) {
        char buf[64];

        snprintf(buf, sizeof(buf), "%u", 0xfffffffe - i);

        rjs_string_from_chars(rt, p, buf, -1);
        rjs_property_name_init(rt, &pn, p);
        r = rjs_get(rt, o, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_OK);
        TEST(rjs_value_is_number(rt, v));
        TEST(rjs_value_get_number(rt, v) == i);
    }

    rjs_object_own_property_keys(rt, o, keys);
    pkl = rjs_value_get_gc_thing(rt, keys);
    for (i = 0; i < pkl->keys.item_num; i ++) {
        RJS_Value *k = &pkl->keys.items[i];

        if (i < 1024) {
            TEST(rjs_value_is_index_string(rt, k));
            TEST(rjs_value_get_index_string(rt, k) == i);
        } else if (i < 2048) {
            TEST(rjs_value_is_index_string(rt, k));
            TEST(rjs_value_get_index_string(rt, k) == 0xfffffffe + i - 2047);
        } else {
            const char *cstr;

            TEST(rjs_value_is_string(rt, k));

            cstr = rjs_string_to_enc_chars(rt, k, NULL, NULL);
            TEST(cstr[0] == (i - 2048) + 'a');
        }
    }

    rjs_value_stack_restore(rt, top);
}

/*Regular expression test case.*/
static void
regexp_case (const char *c_src, const char *c_flags, const char *c_str, const char *c_match)
{
    size_t     top   = rjs_value_stack_save(rt);
    RJS_Value *src   = rjs_value_stack_push(rt);
    RJS_Value *flags = rjs_value_stack_push(rt);
    RJS_Value *re    = rjs_value_stack_push(rt);
    RJS_Value *str   = rjs_value_stack_push(rt);
    RJS_Value *match = rjs_value_stack_push(rt);
    RJS_Value *idx   = rjs_value_stack_push(rt);
    RJS_Value *mstr  = rjs_value_stack_push(rt);
    RJS_Value *estr  = rjs_value_stack_push(rt);
    RJS_Result r;
    char       msg[1024];
    RJS_PropertyName pn;

    rjs_string_from_chars(rt, src, c_src, -1);
    rjs_string_from_chars(rt, flags, c_flags, -1);

    r = rjs_regexp_new(rt, re, src, flags);
    TEST(r == RJS_OK);

    /*rjs_regexp_disassemble(rt, re, stdout);*/

    rjs_string_from_chars(rt, str, c_str, -1);
    r = rjs_regexp_exec(rt, re, str, match);
    TEST(r == RJS_OK);

    snprintf(msg, sizeof(msg), "/%s/%s \"%s\" expect %s",
            c_src, c_flags, c_str, c_match);

    if (c_match) {
        TEST_MSG(!rjs_value_is_null(rt, match), msg);

        rjs_value_set_index_string(rt, idx, 0);
        rjs_property_name_init(rt, &pn, idx);
        rjs_get_v(rt, match, &pn, mstr);
        rjs_property_name_deinit(rt, &pn);

        rjs_string_from_chars(rt, estr, c_match, -1);

        TEST_MSG(rjs_string_equal(rt, estr, mstr), msg);
    } else {
        TEST_MSG(rjs_value_is_null(rt, match), msg);
    }

    rjs_value_stack_restore(rt, top);
}

/*Test regular expression.*/
static void
regexp_test (void)
{
    regexp_case(".", "", "a", "a");
    regexp_case(".", "", "0", "0");
    regexp_case(".", "", "?", "?");
    regexp_case(".", "", "", NULL);
    regexp_case(".", "", "\n", NULL);
    regexp_case(".", "s", "\n", "\n");
    regexp_case("a", "", "ab", "a");
    regexp_case("a", "", "ba", "a");
    regexp_case("a", "", "A", NULL);
    regexp_case("a", "i", "A", "A");
    regexp_case("\\d", "", "0", "0");
    regexp_case("\\d", "", "9", "9");
    regexp_case("\\d", "", "a", NULL);
    regexp_case("\\s", "", " ", " ");
    regexp_case("\\s", "", "\t", "\t");
    regexp_case("\\s", "", "a", NULL);
    regexp_case("\\w", "", "a", "a");
    regexp_case("\\w", "", "A", "A");
    regexp_case("\\w", "", "0", "0");
    regexp_case("\\w", "", "9", "9");
    regexp_case("\\w", "", "_", "_");
    regexp_case("\\w", "", "?", NULL);
    regexp_case("\\D", "", "0", NULL);
    regexp_case("\\D", "", "9", NULL);
    regexp_case("\\D", "", "a", "a");
    regexp_case("\\D", "", " ", " ");
    regexp_case("\\S", "", " ", NULL);
    regexp_case("\\S", "", "\t", NULL);
    regexp_case("\\S", "", "a", "a");
    regexp_case("\\S", "", "?", "?");
    regexp_case("\\W", "", "a", NULL);
    regexp_case("\\W", "", "A", NULL);
    regexp_case("\\W", "", "0", NULL);
    regexp_case("\\W", "", "9", NULL);
    regexp_case("\\W", "", "_", NULL);
    regexp_case("\\W", "", "?", "?");
    regexp_case("^a", "", "a", "a");
    regexp_case("^a", "", "ab", "a");
    regexp_case("^a", "", "ba", NULL);
    regexp_case("a$", "", "a", "a");
    regexp_case("a$", "", "ba", "a");
    regexp_case("a$", "", "ab", NULL);
    regexp_case("\\ba", "", "ab", "a");
    regexp_case("\\ba", "", "c a", "a");
    regexp_case("\\ba", "", "ca", NULL);
    regexp_case("a\\b", "", "ca", "a");
    regexp_case("a\\b", "", "cab", NULL);
    regexp_case("a\\b", "", "ca b", "a");
    regexp_case("\\Ba", "", "ab", NULL);
    regexp_case("\\Ba", "", "c ab", NULL);
    regexp_case("\\Ba", "", "ca", "a");
    regexp_case("a?", "", "a", "a");
    regexp_case("a?", "", "", "");
    regexp_case("a?", "", "aa", "a");
    regexp_case("a+", "", "", NULL);
    regexp_case("a+", "", "a", "a");
    regexp_case("a+", "", "aa", "aa");
    regexp_case("a+", "", "aaa", "aaa");
    regexp_case("a*", "", "", "");
    regexp_case("a*", "", "a", "a");
    regexp_case("a*", "", "aa", "aa");
    regexp_case("a*", "", "aaa", "aaa");
    regexp_case("a{1,1}", "", "aaa", "a");
    regexp_case("a{1}", "", "aaa", "a");
    regexp_case("a{1,2}", "", "aaa", "aa");
    regexp_case("a{1,}", "", "aaa", "aaa");
    regexp_case("[abc]", "", "a", "a");
    regexp_case("[abc]", "", "b", "b");
    regexp_case("[abc]", "", "c", "c");
    regexp_case("[abc]", "", "d", NULL);
    regexp_case("[abc]", "", "A", NULL);
    regexp_case("[abc]", "", "B", NULL);
    regexp_case("[abc]", "", "C", NULL);
    regexp_case("[ABC]", "i", "a", "a");
    regexp_case("[ABC]", "i", "b", "b");
    regexp_case("[ABC]", "i", "c", "c");
    regexp_case("[a-c]", "", "a", "a");
    regexp_case("[a-c]", "", "b", "b");
    regexp_case("[a-c]", "", "c", "c");
    regexp_case("[a-c]", "", "d", NULL);
    regexp_case("[a-c]", "", "A", NULL);
    regexp_case("[a-c]", "", "B", NULL);
    regexp_case("[a-c]", "", "C", NULL);
    regexp_case("[A-C]", "i", "a", "a");
    regexp_case("[A-C]", "i", "b", "b");
    regexp_case("[A-C]", "i", "c", "c");
    regexp_case("[\\d\\s\\w]", "", "0", "0");
    regexp_case("[\\d\\s\\w]", "", "9", "9");
    regexp_case("[\\d\\s\\w]", "", " ", " ");
    regexp_case("[\\d\\s\\w]", "", "a", "a");
    regexp_case("[\\d\\s\\w]", "", "_", "_");
    regexp_case("[^abc]", "", "a", NULL);
    regexp_case("[^abc]", "", "b", NULL);
    regexp_case("[^abc]", "", "c", NULL);
    regexp_case("[^abc]", "", "d", "d");
    regexp_case("[a-z]|[0-9]", "", "a", "a");
    regexp_case("[a-z]|[0-9]", "", "z", "z");
    regexp_case("[a-z]|[0-9]", "", "0", "0");
    regexp_case("[a-z]|[0-9]", "", "9", "9");
    regexp_case("(?<=a)b", "", "ab", "b");
    regexp_case("(?<=a)b", "", "Ab", NULL);
    regexp_case("(?<=a)b", "", "b", NULL);
    regexp_case("(?<!a)b", "", "ab", NULL);
    regexp_case("(?<!a)b", "", "Ab", "b");
    regexp_case("(?<!a)b", "", "b", "b");
    regexp_case("a(?=b)", "", "ab", "a");
    regexp_case("a(?=b)", "", "a", NULL);
    regexp_case("a(?=b)", "", "aB", NULL);
    regexp_case("a(?!b)", "", "ab", NULL);
    regexp_case("a(?!b)", "", "a", "a");
    regexp_case("a(?!b)", "", "aB", "a");
    regexp_case("(?:abc)def", "", "abcdef", "abcdef");
    regexp_case("([a-z]+)=\\1", "", "abc=abc", "abc=abc");
    regexp_case("(?<name>[a-z]+)=\\k<name>", "", "abc=abc", "abc=abc");
    regexp_case("([a-z]+)\\1", "", "abcabc", "abcabc");
    regexp_case("\\d+\\w", "", "01234567", "01234567");
    regexp_case("\\d+?\\w", "", "01234567", "01");
}

/*Array test.*/
static void
array_test (void)
{
    size_t     top = rjs_value_stack_save(rt);
    RJS_Value *a   = rjs_value_stack_push(rt);
    RJS_Value *v   = rjs_value_stack_push(rt);
    RJS_Value *idx = rjs_value_stack_push(rt);
    int        i;
    RJS_Result r;
    RJS_PropertyName pn;

    rjs_array_new(rt, a, 0, NULL);
    rjs_get_v(rt, a, rjs_pn_length(rt), v);
    TEST(rjs_value_is_number(rt, v));
    TEST(rjs_value_get_number(rt, v) == 0);

    for (i = 0; i < 1024; i ++) {
        rjs_value_set_index_string(rt, idx, i);
        rjs_value_set_number(rt, v, i + 100);
        rjs_property_name_init(rt, &pn, idx);
        r = rjs_set(rt, a, &pn, v, RJS_TRUE);
        rjs_property_name_deinit(rt, &pn);
        TEST(r == RJS_TRUE);

        r = rjs_get(rt, a, rjs_pn_length(rt), v);
        TEST(r == RJS_TRUE);
        TEST(rjs_value_is_number(rt, v));
        TEST(rjs_value_get_number(rt, v) == i + 1);
    }

    for (i = 0; i < 1024; i ++) {
        rjs_value_set_index_string(rt, idx, i);
        rjs_property_name_init(rt, &pn, idx);
        rjs_get(rt, a, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(rjs_value_is_number(rt, v));
        TEST(rjs_value_get_number(rt, v) == i + 100);
    }

    r = rjs_set_number(rt, a, rjs_pn_length(rt), 512, RJS_TRUE);
    TEST(r == RJS_TRUE);

    r = rjs_get(rt, a, rjs_pn_length(rt), v);
    TEST(r == RJS_TRUE);
    TEST(rjs_value_is_number(rt, v));
    TEST(rjs_value_get_number(rt, v) == 512);

    for (i = 0; i < 512; i ++) {
        rjs_value_set_index_string(rt, idx, i);
        rjs_property_name_init(rt, &pn, idx);
        rjs_get(rt, a, &pn, v);
        rjs_property_name_deinit(rt, &pn);
        TEST(rjs_value_is_number(rt, v));
        TEST(rjs_value_get_number(rt, v) == i + 100);
    }

    rjs_value_stack_restore(rt, top);
}

/*Comapre 2 integers.*/
static RJS_CompareResult
int_compare (const void *p1, const void *p2, void *arg)
{
    const int *i1 = p1;
    const int *i2 = p2;
    int r = *i1 - *i2;

    if (r < 0)
        return RJS_COMPARE_LESS;
    if (r > 0)
        return RJS_COMPARE_GREATER;

    return RJS_COMPARE_EQUAL;
}

/*Comapre 2 strings.*/
static RJS_CompareResult
str_compare (const void *p1, const void *p2, void *arg)
{
    const char **s1 = (const char**)p1;
    const char **s2 = (const char**)p2;
    int r = strcmp(*s1, *s2);

    if (r < 0)
        return RJS_COMPARE_LESS;
    if (r > 0)
        return RJS_COMPARE_GREATER;

    return RJS_COMPARE_EQUAL;
}

/*Sort test.*/
static void
sort_test (void)
{
    int   ivec[] = {9,8,7,6,5,4,3,2,1,0,2,3,4,5,8};
    char* svec[] = {"a","b","c","d","e","f","g","b","c","a"};
    int   i, j;

    rjs_sort(ivec, RJS_N_ELEM(ivec), sizeof(int), int_compare, NULL);

    for (i = 0; i < RJS_N_ELEM(ivec); i ++) {
        for (j = i + 1; j < RJS_N_ELEM(ivec); j ++) {
            int v1 = ivec[i];
            int v2 = ivec[j];

            TEST(int_compare(&v1, &v2, NULL) != RJS_COMPARE_GREATER);
        }
    }

    rjs_sort(svec, RJS_N_ELEM(svec), sizeof(char*), str_compare, NULL);

    for (i = 0; i < RJS_N_ELEM(svec); i ++) {
        for (j = i + 1; j < RJS_N_ELEM(svec); j ++) {
            char *v1 = svec[i];
            char *v2 = svec[j];

            TEST(str_compare(&v1, &v2, NULL) != RJS_COMPARE_GREATER);
        }
    }
}

int
main (int argc, char **argv)
{
    rjs_log_set_level(RJS_LOG_ALL);
    
    rt = rjs_runtime_new();

    macro_test();
    list_test();
    container_list_test();
    hash_test();
    container_hash_test();
    vector_test();
    rbt_test();
    string_test();
    conv_test();
    input_test();
    uchar_test();
    lex_test();
    parser_test();
    object_test();
    regexp_test();
    array_test();
    sort_test();
    gc_test();

    rjs_runtime_free(rt);

    RJS_LOGI("test: %d passed: %d failed: %d",
            test_total_num,
            test_pass_num,
            test_total_num - test_pass_num);

    return (test_total_num == test_pass_num) ? 0 : 1;
}
