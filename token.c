#include "common.h"
#include "token.h"

#define LIST_INITIAL_SIZE 8
#define GROW 1.5

static void
flt_token_list_grow(struct flt_token_list *list)
{
        ptrdiff_t diff = list->end - list->data;
        list->capacity *= GROW;
        list->data = realloc(list->data, list->capacity);
        list->end = list->data + diff;
}

void
flt_token_list_init(struct flt_token_list *list)
{
        list->capacity = LIST_INITIAL_SIZE;
        list->end = list->data = malloc(list->capacity);
}

void
flt_token_list_destroy(struct flt_token_list *list)
{
        free(list->data);
}

void
flt_token_list_add_token(struct flt_token_list *list, enum flt_token tok)
{
        if (sizeof(tok) + list->end - list->data >= list->capacity) {
                flt_token_list_grow(list);
        }

        *((enum flt_token *)list->end) = tok;
        list->end += sizeof(tok);
}

void
flt_token_list_add_num(struct flt_token_list *list, long double num)
{
        if (sizeof(num) + list->end - list->data >= list->capacity) {
                flt_token_list_grow(list);
        }

        *((long double *)list->end) = num;
        list->end += sizeof(num);
}

void
flt_token_list_add_string(struct flt_token_list *list, const char *string)
{
        /* 
         * we copy the whole string into the list.
         * Not sure if this is a good idea but I'm doing it
         */
        size_t len = strlen(string) + 1;
        while (len + list->end - list->data >= list->capacity) {
                flt_token_list_grow(list);
        }

        strcpy((char *)list->end, string);
        list->end += len;
}
