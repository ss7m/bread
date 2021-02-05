#include "common.h"
#include "token.h"

#define LIST_INITIAL_SIZE 32
#define GROW 1.5

static void
brd_token_list_grow(struct brd_token_list *list)
{
        ptrdiff_t diff1 = list->end - list->start;
        ptrdiff_t diff2 = list->data - list->start;
        list->capacity *= GROW;
        list->start = realloc(list->data, list->capacity);
        list->end = list->start + diff1;
        list->data = list->start + diff2;
}

void
brd_token_list_init(struct brd_token_list *list)
{
        list->capacity = LIST_INITIAL_SIZE;
        list->end = list->data = list->start = malloc(list->capacity);
}

void
brd_token_list_destroy(struct brd_token_list *list)
{
        free(list->start);
}


/* does not respect skip_newlines */
enum brd_token
brd_token_list_peek(struct brd_token_list *list)
{
        enum brd_token tok;
        brd_token_t *data = list->data;

        if (skip_newlines) {
                do {
                        tok = *(enum brd_token *)data;
                        data += sizeof(enum brd_token);
                } while (tok == BRD_TOK_NEWLINE);
        } else {
                tok = *(enum brd_token *)list->data;
        }

        return tok;
}

enum brd_token
brd_token_list_pop_token(struct brd_token_list *list)
{
        enum brd_token tok;

        if (skip_newlines) {
                line_number -= 1;
                do {
                        line_number++;
                        tok = *(enum brd_token *)list->data;
                        list->data += sizeof(enum brd_token);
                } while (tok == BRD_TOK_NEWLINE);
        } else {
                tok = *(enum brd_token *)list->data;
                list->data += sizeof(enum brd_token);
                if (tok == BRD_TOK_NEWLINE) {
                        line_number++;
                }
        }

        return tok;
}

long double
brd_token_list_pop_num(struct brd_token_list *list)
{
        long double num = *(long double *)list->data;
        list->data += sizeof(long double);
        return num;
}

char *
brd_token_list_pop_string(struct brd_token_list *list)
{
        char *string = (char *)list->data;
        list->data += strlen(string) + 1;
        return string;
}

void
brd_token_list_add_token(struct brd_token_list *list, enum brd_token tok)
{
        if (sizeof(tok) + list->end - list->data >= list->capacity) {
                brd_token_list_grow(list);
        }

        *((enum brd_token *)list->end) = tok;
        list->end += sizeof(tok);
}

void
brd_token_list_add_num(struct brd_token_list *list, long double num)
{
        if (sizeof(num) + list->end - list->data >= list->capacity) {
                brd_token_list_grow(list);
        }

        *((long double *)list->end) = num;
        list->end += sizeof(num);
}

void
brd_token_list_add_string(struct brd_token_list *list, const char *string)
{
        /* 
         * we copy the whole string into the list.
         * Not sure if this is a good idea but I'm doing it
         */
        size_t len = strlen(string) + 1;
        while (len + list->end - list->data >= list->capacity) {
                brd_token_list_grow(list);
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

/* is a whitespace character that isn't a newline */
static bool
is_insig_space(char c)
{
        return isspace(c) && c != '\n';
}

/* return true upon success */
static int
brd_parse_string_literal(char **string, char *out)
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
                                  error_message = "Bad escape character: ";
                                  bad_character[0] = **string;
                                  return false;
                        }
                        (*string)++;
                        p++;
                } else if (**string == '\0') {
                        error_message = "Unexpected EOF while parsing string literal";
                        bad_character[0] = '\0';
                        return false;
                } else {
                        *p = **string;
                        (*string)++;
                        p++;
                }
        }

        *p = '\0';
        (*string)++; /* last char is " */
        return true;
}

static void
brd_parse_identifier(char **string, char *out)
{
        char *p = out;
        while (is_id_char((*string)[0])) {
                *(p++) = *((*string)++);
        }
        *p = '\0';
}

int /* return true upon success */
brd_token_list_tokenize(struct brd_token_list *list, char *string)
{
        /* 
         * NOTE: this limits variable names and strings listerals
         * to 1024 characters in length
         * */
        char buffer[1024];

        for (;;) {
                while (is_insig_space(string[0])) string++;
                if (string[0] == '\0') {
                        brd_token_list_add_token(list, BRD_TOK_EOF);
                        return true;
                }

                /* number */
                if (isdigit(string[0])) {
                        brd_token_list_add_token(list, BRD_TOK_NUM);
                        brd_token_list_add_num(list, strtold(string, &string));
                        continue;
                }

                if (isalpha(string[0]) || string[0] == '_') {
                        brd_parse_identifier(&string, buffer);

                        if (strcmp(buffer, "true") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_TRUE);
                        } else if (strcmp(buffer, "false") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_FALSE);
                        } else if (strcmp(buffer, "and") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_AND);
                        } else if (strcmp(buffer, "or") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_OR);
                        } else if (strcmp(buffer, "not") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_NOT);
                        } else if (strcmp(buffer, "set") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_SET);
                        } else if (strcmp(buffer, "unit") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_UNIT);
                        } else if (strcmp(buffer, "begin") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_BEGIN);
                        } else if (strcmp(buffer, "end") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_END);
                        } else if (strcmp(buffer, "if") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_IF);
                        } else if (strcmp(buffer, "elif") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_ELIF);
                        } else if (strcmp(buffer, "else") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_ELSE);
                        } else if (strcmp(buffer, "then") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_THEN);
                        } else if (strcmp(buffer, "for") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_FOR);
                        } else if (strcmp(buffer, "while") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_WHILE);
                        } else if (strcmp(buffer, "do") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_DO);
                        } else if (strcmp(buffer, "func") == 0) {
                                brd_token_list_add_token(list, BRD_TOK_FUNC);
                        } else {
                                brd_token_list_add_token(list, BRD_TOK_VAR);
                                brd_token_list_add_string(list, buffer);
                        }
                        continue;
                }

                if (string[0] == '"') {
                        if (!brd_parse_string_literal(&string, buffer)) {
                                return false;
                        }
                        brd_token_list_add_token(list, BRD_TOK_STR);
                        brd_token_list_add_string(list, buffer);
                        continue;
                }

                switch (string[0]) {
                case '(':
                        brd_token_list_add_token(list, BRD_TOK_LPAREN);
                        string++;
                        break;
                case ')':
                        brd_token_list_add_token(list, BRD_TOK_RPAREN);
                        string++;
                        break;
                case '[':
                        brd_token_list_add_token(list, BRD_TOK_LBRACKET);
                        string++;
                        break;
                case ']':
                        brd_token_list_add_token(list, BRD_TOK_RBRACKET);
                        string++;
                        break;
                case '\n':
                        brd_token_list_add_token(list, BRD_TOK_NEWLINE);
                        string++;
                        break;
                case '+':
                        brd_token_list_add_token(list, BRD_TOK_PLUS);
                        string++;
                        break;
                case '-':
                        brd_token_list_add_token(list, BRD_TOK_MINUS);
                        string++;
                        break;
                case '*':
                        brd_token_list_add_token(list, BRD_TOK_MUL);
                        string++;
                        break;
                case '/':
                        if (string[1] == '/') {
                                brd_token_list_add_token(list, BRD_TOK_IDIV);
                                string += 2;
                        } else {
                                brd_token_list_add_token(list, BRD_TOK_DIV);
                                string++;
                        }
                        break;
                case '%':
                        brd_token_list_add_token(list, BRD_TOK_MOD);
                        string++;
                        break;
                case '^':
                        brd_token_list_add_token(list, BRD_TOK_POW);
                        string++;
                        break;
                case '<':
                        if (string[1] == '=') {
                                brd_token_list_add_token(list, BRD_TOK_LEQ);
                                string += 2;
                        } else {
                                brd_token_list_add_token(list, BRD_TOK_LT);
                                string++;
                        }
                        break;
                case '>':
                        if (string[1] == '=') {
                                brd_token_list_add_token(list, BRD_TOK_GEQ);
                                string += 2;
                        } else {
                                brd_token_list_add_token(list, BRD_TOK_GT);
                                string++;
                        }
                        break;
                case '=':
                        brd_token_list_add_token(list, BRD_TOK_EQ);
                        string++;
                        break;
                case '.':
                        if (string[1] == '.') {
                                brd_token_list_add_token(list, BRD_TOK_CONCAT);
                                string += 2;
                        } else {
                                brd_token_list_add_token(list, BRD_TOK_FIELD);
                                string++;
                        }
                        break;
                case ':':
                        if (string[1] == ':') {
                                brd_token_list_add_token(list, BRD_TOK_ACC_OBJ);
                                string += 2;
                        } else {
                                error_message =
                                        "':' is not a valid token, did you mean '::'?";
                                return false;
                        }
                        break;
                case '!':
                        if (string[1] == '=') {
                                brd_token_list_add_token(list, BRD_TOK_NEQ);
                                string += 2;
                        } else {
                                error_message = "! is not a valid operator";
                                return false;
                        }
                        break;
                case '@':
                        string++;
                        brd_parse_identifier(&string, buffer);
                        brd_token_list_add_token(list, BRD_TOK_BUILTIN);
                        brd_token_list_add_string(list, buffer);
                        break;
                case ',':
                        brd_token_list_add_token(list, BRD_TOK_COMMA);
                        string++;
                        break;
                case '#':
                        while (string[0] != '\0' && string[0] != '\n') {
                                string++;
                        }
                        break;
                default:
                        error_message = "unrecognized character: ";
                        bad_character[0] = string[0];
                        return false;
                }
        }
}
