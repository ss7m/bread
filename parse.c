#include "common.h"
#include "token.h"
#include "ast.h"
#include "parse.h"

struct brd_node *
brd_parse_expression(struct brd_token_list *tokens)
{
        struct brd_token_list copy = *tokens;
        struct brd_node *l, *r;

        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_SET:
                if ((l = brd_parse_lvalue(tokens)) == NULL) {
                        BARF("Goodbye.");
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) != BRD_TOK_EQ) {
                        BARF("Goodbye.");
                        return NULL;
                } else if ((r = brd_parse_expression(tokens)) == NULL) {
                        BARF("Goodbye.");
                        return NULL;
                } else {
                        return brd_node_assign_new(l, r);
                }
        default:
                *tokens = copy;
                return brd_parse_orexp(tokens);
        }
}

struct brd_node *
brd_parse_lvalue(struct brd_token_list *tokens)
{
        struct brd_token_list copy = *tokens;
        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_VAR:
                return brd_node_var_new(brd_token_list_pop_string(tokens));
        default:
                *tokens = copy;
                return NULL;
        }
}

struct brd_node *
brd_parse_orexp(struct brd_token_list *tokens)
{
        struct brd_token_list copy;
        struct brd_node *l, *r;

        l = brd_parse_andexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        /* these if (r == NULL) clauses might just be a failure state */
        copy = *tokens;
        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_OR:
                r = brd_parse_orexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_OR, l, r);
                }
        default:
                *tokens = copy;
                return l;
        }
}

struct brd_node *
brd_parse_andexp(struct brd_token_list *tokens)
{
        struct brd_token_list copy;
        struct brd_node *l, *r;

        l = brd_parse_compexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        /* these if (r == NULL) clauses might just be a failure state */
        copy = *tokens;
        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_AND:
                r = brd_parse_andexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_AND, l, r);
                }
        default:
                *tokens = copy;
                return l;
        }
}

struct brd_node *
brd_parse_compexp(struct brd_token_list *tokens)
{
        struct brd_token_list copy;
        struct brd_node *l, *r;

        l = brd_parse_addexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        copy = *tokens;
        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_LT:
                r = brd_parse_addexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_LT, l, r);
                }
        case BRD_TOK_LEQ:
                r = brd_parse_addexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_LEQ, l, r);
                }
        case BRD_TOK_EQ:
                r = brd_parse_addexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_EQ, l, r);
                }
        case BRD_TOK_GT:
                r = brd_parse_addexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_GT, l, r);
                }
        case BRD_TOK_GEQ:
                r = brd_parse_addexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_GEQ, l, r);
                }
        default:
                *tokens = copy;
                return l;
        }
}

struct brd_node *
brd_parse_addexp(struct brd_token_list *tokens)
{
        struct brd_token_list copy;
        struct brd_node *l, *r;

        l = brd_parse_mulexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        /* these if (r == NULL) clauses might just be a failure state */
        copy = *tokens;
        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_PLUS:
                r = brd_parse_addexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_PLUS, l, r);
                }
        case BRD_TOK_MINUS:
                r = brd_parse_addexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_MINUS, l, r);
                }
        default:
                *tokens = copy;
                return l;
        }
}

struct brd_node *
brd_parse_mulexp(struct brd_token_list *tokens)
{
        struct brd_token_list copy;
        struct brd_node *l, *r;

        l = brd_parse_prefix(tokens);
        if (l == NULL) {
                return NULL;
        }

        /* these if (r == NULL) clauses might just be a failure state */
        copy = *tokens;
        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_MUL:
                r = brd_parse_mulexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_MUL, l, r);
                }
        case BRD_TOK_DIV:
                r = brd_parse_mulexp(tokens);
                if (r == NULL) {
                        *tokens = copy;
                        return l;
                } else {
                        return brd_node_binop_new(BRD_DIV, l, r);
                }
        default:
                *tokens = copy;
                return l;
        }
}

struct brd_node *
brd_parse_prefix(struct brd_token_list *tokens)
{
        struct brd_token_list copy = *tokens;
        struct brd_node *node;

        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_NOT:
                node = brd_parse_prefix(tokens);
                if (node == NULL) {
                        *tokens = copy;
                        return NULL;
                } else {
                        return brd_node_unary_new(BRD_NOT, node);
                }
        case BRD_TOK_MINUS:
                node = brd_parse_prefix(tokens);
                if (node == NULL) {
                        *tokens = copy;
                        return NULL;
                } else {
                        return brd_node_unary_new(BRD_NEGATE, node);
                }
        default:
                *tokens = copy;
                return brd_parse_base(tokens);
        }
}

struct brd_node *
brd_parse_base(struct brd_token_list *tokens)
{
        struct brd_token_list copy = *tokens;
        struct brd_node *node;

        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_LPAREN:
                node = brd_parse_expression(tokens);
                if (node == NULL) {
                        *tokens = copy;
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) == BRD_TOK_RPAREN) {
                        return node;
                } else {
                        BARF("You're missing a right paren there m8");
                        *tokens = copy;
                        return NULL;
                }
        case BRD_TOK_VAR:
                return brd_node_var_new(brd_token_list_pop_string(tokens));
        case BRD_TOK_NUM:
                return brd_node_num_lit_new(brd_token_list_pop_num(tokens));
        case BRD_TOK_STR:
                return brd_node_string_lit_new(brd_token_list_pop_string(tokens));
        case BRD_TOK_UNIT:
                return brd_node_unit_lit_new();
        case BRD_TOK_TRUE:
                return brd_node_bool_lit_new(true);
        case BRD_TOK_FALSE:
                return brd_node_bool_lit_new(false);
        default:
                *tokens = copy;
                return NULL;
        }
}
