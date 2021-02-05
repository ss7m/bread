#ifndef BRD_TOKEN_H
#define BRD_TOKEN_H

typedef char brd_token_t;

enum brd_token {
        /* literals */
        BRD_TOK_NUM, /* has arg: long double */
        BRD_TOK_STR, /* has arg: string */
        BRD_TOK_VAR, /* has arg: string */
        BRD_TOK_TRUE,
        BRD_TOK_FALSE,
        BRD_TOK_UNIT,

        /* grammatical stuff */
        BRD_TOK_LPAREN,
        BRD_TOK_RPAREN,
        BRD_TOK_LBRACKET, /* [ ] */
        BRD_TOK_RBRACKET,
        BRD_TOK_NEWLINE,
        BRD_TOK_EOF,

        /* arithmetic */
        BRD_TOK_PLUS,
        BRD_TOK_MINUS,
        BRD_TOK_MUL,
        BRD_TOK_DIV,
        BRD_TOK_IDIV,
        BRD_TOK_MOD,
        BRD_TOK_POW,

        BRD_TOK_CONCAT,

        /* comp */
        BRD_TOK_LT,
        BRD_TOK_LEQ,
        BRD_TOK_GT,
        BRD_TOK_GEQ,
        BRD_TOK_EQ,
        BRD_TOK_NEQ,
        /* let's use = for settings and equality, what could go wrong */

        /* boolean */
        BRD_TOK_AND,
        BRD_TOK_OR,
        BRD_TOK_NOT,

        BRD_TOK_BUILTIN,
        BRD_TOK_COMMA,

        BRD_TOK_BEGIN,
        BRD_TOK_END,

        BRD_TOK_IF,
        BRD_TOK_ELIF,
        BRD_TOK_ELSE,
        BRD_TOK_THEN,

        BRD_TOK_FUNC,

        BRD_TOK_FOR,
        BRD_TOK_WHILE,
        BRD_TOK_DO,

        BRD_TOK_FIELD,
        BRD_TOK_ACC_OBJ,

        /* keywords */
        BRD_TOK_SET,
};

struct brd_token_list {
        brd_token_t *start, *data, *end;
        size_t capacity;
};

void brd_token_list_init(struct brd_token_list *list);
void brd_token_list_destroy(struct brd_token_list *list);

enum brd_token brd_token_list_peek(struct brd_token_list *list);
enum brd_token brd_token_list_pop_token(struct brd_token_list *list);
long double brd_token_list_pop_num(struct brd_token_list *list);
char *brd_token_list_pop_string(struct brd_token_list *list);

void brd_token_list_add_token(struct brd_token_list *list, enum brd_token tok);
void brd_token_list_add_num(struct brd_token_list *list, long double num);
void brd_token_list_add_string(struct brd_token_list *list, const char *string);

int brd_token_list_tokenize(struct brd_token_list *list, char *string);

#endif
