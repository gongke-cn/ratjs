#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "list.h"

/**The template entry.*/
typedef struct {
    List  ln;     /**< List node data.*/
    char *name;   /**< The name of the entry.*/
    char *value;  /**< The value of the entry.*/
} Entry;

/**Entroes' list.*/
static List    entries;
/**String buffer.*/
static char   *str_buf     = NULL;
/**String buffer's length.*/
static size_t  str_buf_len = 0;

/*Append a character to the file.*/
static void
append_char (size_t pos, int c)
{
    if (pos >= str_buf_len) {
        str_buf_len *= 2;
        if (str_buf_len < 256)
            str_buf_len = 256;

        str_buf = realloc(str_buf, str_buf_len);
    }

    str_buf[pos] = c;
}

/*Read a string from the file.*/
static char*
get_str (FILE *fp)
{
    size_t pos = 0;
    int    c;

    while (1) {
        c = fgetc(fp);
        if (c == EOF)
            break;

        append_char(pos ++, c);

        if (c == '\n')
            break; 
    }

    if (pos == 0)
        return NULL;

    append_char(pos, 0);

    return str_buf;
}

/*check if the line is a stub.*/
static char*
is_stub_line (char *line)
{
    char *c = line;

    while (*c && isspace(*c))
        c ++;

    if ((c[0] == '/') && (c[1] == '*')) {
        c = strstr(c + 2, "stub");
        if (c)
            return c;
    }

    return NULL;
}

/*Load an entry.*/
static void
load_entry (FILE *fp, char *name)
{
    char   *val_buf = NULL;
    size_t  val_len = 0;
    size_t  val_cap = 0;
    Entry  *e;

    while (1) {
        char   *str, *c;
        size_t  slen;

        str = get_str(fp);
        if (!str)
            break;

        c = is_stub_line(str);
        if (c) {
            char nbuf[1024];

            if (sscanf(c, "stub end: %s", nbuf) == 1) {
                if (!strcmp(nbuf, name))
                    break;
            }
        }

        slen = strlen(str);

        if (slen + val_len > val_cap) {
            val_cap *= 2;
            if (val_cap < slen + val_len)
                val_cap = slen + val_len;

            val_buf = realloc(val_buf, val_cap);
        }

        memcpy(val_buf + val_len, str, slen);
        val_len += slen;
    }

    e = malloc(sizeof(Entry));

    e->name = strdup(name);

    if (val_buf) {
        e->value = malloc(val_len + 1);
        memcpy(e->value, val_buf, val_len);
        e->value[val_len] = 0;
        free(val_buf);
    } else {
        e->value = NULL;
    }

    list_append(&entries, &e->ln);
}

/**
 * Load the template file.
 * \param filename The template filename.
 */
void
template_load (const char *filename)
{
    FILE *fp;

    list_init(&entries);

    fp = fopen(filename, "rb");
    if (!fp)
        return;

    while (1) {
        char *str, *c;

        str = get_str(fp);
        if (!str)
            break;

        c = is_stub_line(str);
        if (c) {
            char nbuf[1024];

            if (sscanf(c, "stub begin: %s", nbuf) == 1) {
                load_entry(fp, nbuf);
            }
        }
    }

    if (str_buf) {
        free(str_buf);
        str_buf_len = 0;
    }

    fclose(fp);
}

/**
 * Lookup the entry from the template file.
 * \param name The entry's name.
 * \return The entry's value.
 */
const char*
template_lookup (const char *name)
{
    Entry *e;

    list_foreach(&entries, e, Entry, ln) {
        if (!strcmp(name, e->name))
            return e->value;
    }

    return NULL;
}

/**
 * Clear the loaded template entries.
 */
void
template_clear (void)
{
    Entry *e, *ne;

    list_foreach_safe(&entries, e, ne, Entry, ln) {
        if (e->name)
            free(e->name);
        if (e->value)
            free(e->value);

        free(e);
    }
}
