#ifndef FLT_TOKEN_H
#define FLT_TOKEN_H

enum flt_token {
        /* literals */
        FLT_TOK_NUM,
        FLT_TOK_STR,
        FLT_TOK_VAR,
        FLT_TOK_TRUE,
        FLT_TOK_FALSE,

        /* grammatical stuff */
        FLT_TOK_LPAREN,
        FLT_TOK_RPAREN,
        FLT_TOK_NEWLINE,

        /* arithmetic */
        FLT_TOK_PLUS,
        FLT_TOK_MINUS,
        FLT_TOK_MUL,
        FLT_TOK_DIV,

        /* comp */
        FLT_TOK_LT,
        FLT_TOK_LEQ,
        FLT_TOK_GT,
        FLT_TOK_GEQ,
        FLT_TOK_EQ,
        /* let's use = for settings and equality, what could go wrong */

        /* boolean */
        FLT_TOK_AND,
        FLT_TOK_OR,

        /* keywords */
        FLT_TOK_SET,
};

struct flt_token_list {
        void *data, *end;
        size_t capacity;
};

void flt_token_list_init(struct flt_token_list *list);
void flt_token_list_destroy(struct flt_token_list *list);

void flt_token_list_add_token(struct flt_token_list *list, enum flt_token tok);
void flt_token_list_add_num(struct flt_token_list *list, long double num);
void flt_token_list_add_string(struct flt_token_list *list, const char *string);

void flt_token_list_tokenize(struct flt_token_list *list, char *string);

#endif
