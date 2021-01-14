#include "common.h"
#include "token.h"

#include <ctype.h>

#define LIST_INITIAL_SIZE 32
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

/* idintifiers match [a-zA-Z][a-zA-Z0-9_]* */
static bool
is_id_char(char c)
{
        return isalnum(c) || c == '_';
}

static void
flt_parse_string_literal(char **string, char *out)
{
        char *p = out;
        (*string)++; /* first char is " */

        while (**string != '"') {
                if (**string == '\\') {
                        (*string)++;
                        switch (**string) {
                        case 'n': *p = '\n'; break;
                        case 't': *p = '\t'; break;
                        case '"': *p = '"'; break;
                        default:
                                  BARF("Bad escape character: <<%c>>", **string);
                        }
                        (*string)++;
                        p++;
                } else if (**string == '\0') {
                        BARF("Unexpcted EOF while parsing string literal");
                } else {
                        *p = **string;
                        (*string)++;
                        p++;
                }
        }

        *p = '\0';
        (*string)++; /* last char is " */
}

void
flt_token_list_tokenize(struct flt_token_list *list, char *string)
{
        /* 
         * NOTE: this limits variable names and strings listerals
         * to 1024 characters in length
         * */
        char buffer[1024];

        for (;;) {
                while (isspace(string[0])) string++;
                if (string[0] == '\0') {
                        break;
                }
                printf("%s\n", string);

                /* number */
                if (isdigit(string[0])) {
                        flt_token_list_add_token(list, FLT_TOK_NUM);
                        flt_token_list_add_num(list, strtold(string, &string));
                        continue;
                }

                if (isalpha(string[0])) {
                        char *p = buffer;
                        while (is_id_char(string[0])) {
                                *(p++) = *(string++);
                        }
                        *p = '\0';

                        if (strcmp(buffer, "true") == 0) {
                                flt_token_list_add_token(list, FLT_TOK_TRUE);
                        } else if (strcmp(buffer, "false") == 0) {
                                flt_token_list_add_token(list, FLT_TOK_FALSE);
                        } else if (strcmp(buffer, "and") == 0) {
                                flt_token_list_add_token(list, FLT_TOK_AND);
                        } else if (strcmp(buffer, "or") == 0) {
                                flt_token_list_add_token(list, FLT_TOK_OR);
                        } else if (strcmp(buffer, "set") == 0) {
                                flt_token_list_add_token(list, FLT_TOK_SET);
                        } else {
                                flt_token_list_add_token(list, FLT_TOK_VAR);
                                flt_token_list_add_string(list, buffer);
                        }
                        continue;
                }

                if (string[0] == '"') {
                        flt_parse_string_literal(&string, buffer);
                        flt_token_list_add_token(list, FLT_TOK_STR);
                        flt_token_list_add_string(list, buffer);
                        continue;
                }

                switch (string[0]) {
                case '(':
                        flt_token_list_add_token(list, FLT_TOK_LPAREN);
                        string++;
                        break;
                case ')':
                        flt_token_list_add_token(list, FLT_TOK_RPAREN);
                        string++;
                        break;
                case ';':
                        flt_token_list_add_token(list, FLT_TOK_SEMICOLON);
                        string++;
                        break;
                case '+':
                        flt_token_list_add_token(list, FLT_TOK_PLUS);
                        string++;
                        break;
                case '-':
                        flt_token_list_add_token(list, FLT_TOK_MINUS);
                        string++;
                        break;
                case '*':
                        flt_token_list_add_token(list, FLT_TOK_MUL);
                        string++;
                        break;
                case '/':
                        flt_token_list_add_token(list, FLT_TOK_DIV);
                        string++;
                        break;
                case '<':
                        if (string[1] == '=') {
                                flt_token_list_add_token(list, FLT_TOK_LEQ);
                                string += 2;
                        } else {
                                flt_token_list_add_token(list, FLT_TOK_LT);
                                string ++;
                        }
                        break;
                case '>':
                        if (string[1] == '=') {
                                flt_token_list_add_token(list, FLT_TOK_GEQ);
                                string += 2;
                        } else {
                                flt_token_list_add_token(list, FLT_TOK_GT);
                                string ++;
                        }
                        break;
                case '=':
                        flt_token_list_add_token(list, FLT_TOK_EQ);
                        string++;
                        break;
                default:
                        BARF("Tokenizer broke on char <<%c>>\n", *string);
                }
        }
}
