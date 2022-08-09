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
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <ratjs/rjs_config.h>

/**The token.*/
typedef struct {
    char *str;   /**< The token string.*/
} LEX_Token;

#include "../misc/identifier.h"
#include "../misc/punctuator.h"

/**Character entry.*/
typedef struct {
    int   c;      /**< The character.*/
    int   next;   /**< The next entry's index.*/
    int   child;  /**< The first child entry's index.*/
    void *result; /**< The result.*/
    char *flags;  /**< Flags.*/
} LEX_Entry;

/**The lexical table.*/
typedef struct {
    LEX_Entry *entries; /**< Entries in the table.*/
    size_t     num;     /**< Number of the entries.*/
    size_t     cap;     /**< Capacity of the entries buffer.*/
} LEX_Table;

#include "../misc/punct_char_name.h"

/**Identifier table.*/
static LEX_Table id_tab;
/**Punctuator table.*/
static LEX_Table punct_tab;

/*Create a new character entry.*/
static int
entry_add (LEX_Table *tab, int c)
{
    LEX_Entry *e;
    int        id = tab->num;

    if (tab->num >= tab->cap) {
        size_t cap = tab->cap * 2;

        if (cap < 16)
            cap = 16;

        tab->entries = realloc(tab->entries, sizeof(LEX_Entry) * cap);
        tab->cap     = cap;
    }

    e = &tab->entries[id];

    e->c      = c;
    e->next   = -1;
    e->child  = -1;
    e->result = NULL;

    tab->num ++;

    return id;
}

/*Build entries for a token.*/
static void
build_token (LEX_Table *tab, const LEX_Token *t, char *flags)
{
    const char *c  = t->str;
    LEX_Entry  *e, *p;
    int         parent_id, curr_id;

    parent_id = 0;

    while (*c) {
        p = &tab->entries[parent_id];

        curr_id = p->child;

        while (curr_id != -1) {
            e = &tab->entries[curr_id];

            if (e->c == *c)
                break;

            curr_id = e->next;
        }

        if (curr_id == -1) {
            curr_id = entry_add(tab, *c);

            p = &tab->entries[parent_id];
            e = &tab->entries[curr_id];

            e->next  = p->child;
            p->child = curr_id;
        }

        parent_id = curr_id;
        c ++;
    }

    e = &tab->entries[parent_id];

    e->result = (void*)t;
    e->flags  = flags;
}

/*Build the lexical table.*/
static void
build_table (LEX_Table *tab, const LEX_Token *tokens, char *flags)
{
    const LEX_Token *t = tokens;

    while (t->str) {
        build_token(tab, t, flags);
        t ++;
    }
}

/*Get the character's name.*/
static const char*
get_char_name (int c)
{
    const LEX_CharName *cn = punct_char_names;

    while (cn->name) {
        if (cn->c == c)
            return cn->name;
        cn ++;
    }

    return NULL;
}

/*Print the name of the punctuator.*/
static void
print_punct_name (const char *name)
{
    const char *c = name;

    while (*c) {
        const char *cn = get_char_name(*c);

        if (!cn)
            fprintf(stderr, "name of \'%c\' is undefined\n", *c);

        assert(cn);

        if (c != name)
            printf("_");

        printf("%s", cn);
        c ++;
    }
}

/*Generate the lexical table.*/
static void
gen_table (LEX_Table *tab, char *name)
{
    int i;

    printf("static const RJS_LexCharEntry %s_lex_table[] = {\n", name);
    for (i = 0; i < tab->num; i ++) {
        LEX_Entry *e = &tab->entries[i];
        LEX_Token *t = e->result;

        printf("\t{\'%c\', %d, %d, ",
                e->c,
                e->next,
                e->child);

        if (!t) {
            printf("-1");
        } else if (!strcmp(name, "identifier")) {
            printf("RJS_IDENTIFIER_%s", t->str);

            if (e->flags)
                printf("|RJS_TOKEN_FL_%s", e->flags);
        } else {
            printf("RJS_TOKEN_");
            print_punct_name(t->str);
        }

        printf("}%s /*%d*/\n",
                (i != tab->num - 1) ? "," : "",
                i);
    }
    printf("};\n\n");
}

/*Generate the token types.*/
static void
gen_token_types (void)
{
    const LEX_Token *t = reserved_word;

    printf("typedef enum {\n");
    printf("\tRJS_IDENTIFIER_START,\n");

    while (t->str) {
        printf("\tRJS_IDENTIFIER_%s,\n", t->str);
        t ++;
    }

    t = strict_reserved_word;
    while (t->str) {
        printf("\tRJS_IDENTIFIER_%s,\n", t->str);
        t ++;
    }

    t = identifier;
    while (t->str) {
        printf("\tRJS_IDENTIFIER_%s,\n", t->str);
        t ++;
    }

    printf("} RJS_IdentifierType;\n\n");

    printf("#define RJS_PUNCTUATOR_TOKENS\\\n");
    printf("\tRJS_TOKEN_PUNCT_START,\\\n");

    t = punctuator;
    while (t->str) {
        printf("\tRJS_TOKEN_");
        print_punct_name(t->str);
        printf(",\\\n");
        t ++;
    }

    printf("\n\n");
}

/*Generate the token names array.*/
static void
gen_token_names (void)
{
    const LEX_Token *t;

    printf("static const char* token_names[] = {\n");
    t = punctuator;
    while (t->str) {
        if (!strcmp(t->str, "\?\?=")) {
            printf("\t\"`\\?\\?=\\\'\",\n");
        } else {
            printf("\t\"`%s\\\'\",\n", t->str);
        }
        t ++;
    }
    printf("\tNULL\n");
    printf("};\n");

    printf("static const char* identifier_names[] = {\n");
    t = reserved_word;
    while (t->str) {
        printf("\t\"`%s\\\'\",\n", t->str);
        t ++;
    }
    t = strict_reserved_word;
    while (t->str) {
        printf("\t\"`%s\\\'\",\n", t->str);
        t ++;
    }

    t = identifier;
    while (t->str) {
        printf("\t\"`%s\\\'\",\n", t->str);
        t ++;
    }
    printf("\tNULL\n");
    printf("};\n");
}

int
main (int argc, char **argv)
{
    if ((argc > 1) && !strcmp(argv[1], "-t")) {
        gen_token_types();
    } else {
        entry_add(&id_tab, 'S');
        entry_add(&punct_tab, 'S');

        build_table(&id_tab, reserved_word, "RESERVED");
        build_table(&id_tab, strict_reserved_word, "STRICT_RESERVED");
        build_table(&id_tab, identifier, "KNOWN_IDENTIFIER");
        build_table(&punct_tab, punctuator, NULL);

        gen_table(&id_tab, "identifier");
        gen_table(&punct_tab, "punctuator");
        gen_token_names();
    }

    return 0;
}
