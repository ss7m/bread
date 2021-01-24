#include "common.h"
#include "token.h"
#include "ast.h"
#include "parse.h"

#define GROW 1.5
#define LIST_SIZE 4

int skip_newlines = false;

struct brd_node_program *
brd_parse_program(struct brd_token_list *tokens)
{
        size_t length = 0, capacity = LIST_SIZE;
        struct brd_node **stmts = malloc(sizeof(struct brd_node *) * capacity);
        struct brd_node *node;

        skip_newlines = false;

        while(brd_token_list_peek(tokens) == BRD_TOK_NEWLINE) {
                brd_token_list_pop_token(tokens);
        }

        while ((node = brd_parse_expression_stmt(tokens)) != NULL) {
                if (length == capacity) {
                        capacity *= GROW;
                        stmts = realloc(stmts, sizeof(struct brd_node *) * capacity);
                }
                stmts[length] = node;
                length++;

                if (tokens->data == tokens->end) {
                        break;
                }
        }

        if (tokens->data != tokens->end) {
                BARF("Parsing finished prematurely");
                return NULL;
        }

        return (struct brd_node_program *)brd_node_program_new(stmts, length);
}

/* skip_newlines should be false when this is called */
struct brd_node *
brd_parse_expression_stmt(struct brd_token_list *tokens)
{
        struct brd_token_list copy = *tokens;
        struct brd_node *node;
        int found_term;

        node = brd_parse_expression(tokens);
        if (node == NULL) {
                return NULL;
        }

        found_term = false;
        for (;;) {
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_EOF:
                        brd_token_list_pop_token(tokens);
                        return node;
                case BRD_TOK_NEWLINE:
                        brd_token_list_pop_token(tokens);
                        found_term = true;
                        break;
                default:
                        if (found_term) {
                                return node;
                        } else {
                                *tokens = copy;
                                return NULL;
                        }
                }
        }
}

struct brd_node *
brd_parse_expression(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;
        int skip_copy = skip_newlines;

        switch (brd_token_list_peek(tokens)) {
        case BRD_TOK_SET:
                brd_token_list_pop_token(tokens);
                skip_newlines = true;
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
                        skip_newlines = skip_copy;
                        return brd_node_assign_new(l, r);
                }
        default:
                return brd_parse_concatexp(tokens);
        }
}

struct brd_node *
brd_parse_lvalue(struct brd_token_list *tokens)
{
        struct brd_node *node, *idx;

        if (brd_token_list_pop_token(tokens) == BRD_TOK_VAR) {
                node = brd_node_var_new(brd_token_list_pop_string(tokens));
        } else {
                BARF("error parsing lvalue");
        }

        for (;;) {
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_LBRACKET:
                        brd_token_list_pop_token(tokens);
                        idx = brd_parse_expression(tokens);
                        if (idx == NULL) {
                                BARF("bad index");
                        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RBRACKET) {
                                BARF("you're missing a right bracket");
                        }
                        node = brd_node_index_new(node, idx);
                        break;
                default:
                        return node;
                }
        }
}

struct brd_node *
brd_parse_concatexp(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;

        l = brd_parse_orexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        for (;;) {
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_CONCAT:
                        brd_token_list_pop_token(tokens);
                        r = brd_parse_orexp(tokens);
                        if (r == NULL) {
                                BARF("Bad concat expression");
                        } else {
                                l = brd_node_binop_new(BRD_CONCAT, l, r);
                        }
                        break;
                default:
                        return l;
                }
        }
}

struct brd_node *
brd_parse_orexp(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;

        l = brd_parse_andexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        switch (brd_token_list_peek(tokens)) {
        case BRD_TOK_OR:
                brd_token_list_pop_token(tokens);
                r = brd_parse_orexp(tokens);
                if (r == NULL) {
                        BARF("Bad or expression");
                        return NULL;
                } else {
                        return brd_node_binop_new(BRD_OR, l, r);
                }
        default:
                return l;
        }
}

struct brd_node *
brd_parse_andexp(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;

        l = brd_parse_compexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        switch (brd_token_list_peek(tokens)) {
        case BRD_TOK_AND:
                brd_token_list_pop_token(tokens);
                r = brd_parse_andexp(tokens);
                if (r == NULL) {
                        BARF("Bad and expression");
                        return NULL;
                } else {
                        return brd_node_binop_new(BRD_AND, l, r);
                }
        default:
                return l;
        }
}

struct brd_node *
brd_parse_compexp(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;
        enum brd_binop binop;

        l = brd_parse_addexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        switch (brd_token_list_peek(tokens)) {
        case BRD_TOK_LT:
                brd_token_list_pop_token(tokens);
                binop = BRD_LT;
                break;
        case BRD_TOK_LEQ:
                brd_token_list_pop_token(tokens);
                binop = BRD_LEQ;
                break;
        case BRD_TOK_EQ:
                brd_token_list_pop_token(tokens);
                binop = BRD_EQ;
                break;
        case BRD_TOK_GT:
                brd_token_list_pop_token(tokens);
                binop = BRD_GT;
                break;
        case BRD_TOK_GEQ:
                brd_token_list_pop_token(tokens);
                binop = BRD_GEQ;
                break;
        default:
                return l;
        }
        
        r = brd_parse_addexp(tokens);
        if (r == NULL) {
                BARF("Bad comparison expression");
                return NULL;
        } else {
                return brd_node_binop_new(binop, l, r);
        }
}

struct brd_node *
brd_parse_addexp(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;

        l = brd_parse_mulexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        for (;;) {
                enum brd_binop binop;
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_PLUS:
                        binop = BRD_PLUS;
                        brd_token_list_pop_token(tokens);
                        break;
                case BRD_TOK_MINUS:
                        binop = BRD_MINUS;
                        brd_token_list_pop_token(tokens);
                        break;
                default:
                        return l;
                }

                r = brd_parse_mulexp(tokens);
                if (r == NULL) {
                        BARF("Bad addition expression");
                } else {
                        l = brd_node_binop_new(binop, l, r);
                }
        }
}

struct brd_node *
brd_parse_mulexp(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;

        l = brd_parse_powexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        for (;;) {
                enum brd_binop binop;
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_MUL:
                        binop = BRD_MUL;
                        brd_token_list_pop_token(tokens);
                        break;
                case BRD_TOK_DIV:
                        binop = BRD_DIV;
                        brd_token_list_pop_token(tokens);
                        break;
                case BRD_TOK_IDIV:
                        binop = BRD_IDIV;
                        brd_token_list_pop_token(tokens);
                        break;
                case BRD_TOK_MOD:
                        binop = BRD_MOD;
                        brd_token_list_pop_token(tokens);
                        break;
                default:
                        return l;
                }

                r = brd_parse_powexp(tokens);
                if (r == NULL) {
                        BARF("Bad multiplication expression");
                } else {
                        l = brd_node_binop_new(binop, l, r);
                }
        }
}

struct brd_node *
brd_parse_powexp(struct brd_token_list *tokens)
{
        struct brd_node *l, *r;

        l = brd_parse_prefix(tokens);
        if (l == NULL) {
                return NULL;
        }

        if (brd_token_list_peek(tokens) == BRD_TOK_POW) {
                brd_token_list_pop_token(tokens);
                r = brd_parse_powexp(tokens);
                if (r == NULL) {
                        BARF("Bad pow expression");
                        return NULL;
                } else {
                        return brd_node_binop_new(BRD_POW, l, r);
                }
        } else {
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
                return brd_parse_postfix(tokens);
        }
}

struct brd_node *
brd_parse_postfix(struct brd_token_list *tokens)
{
        struct brd_node *node, *idx;
        struct brd_node_arglist *args;
        int skip_copy = skip_newlines;

        node = brd_parse_base(tokens);
        if (node == NULL) {
                return NULL;
        }

        for (;;) {
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_LPAREN:
                        brd_token_list_pop_token(tokens);
                        skip_newlines = true;
                        args = brd_parse_arglist(tokens);
                        if (args == NULL) {
                                BARF("bad argument list");
                        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RPAREN) {
                                BARF("you're missing a right paren");
                        }
                        skip_newlines = skip_copy;
                        node = brd_node_funcall_new(node, args);
                        break;
                case BRD_TOK_LBRACKET:
                        brd_token_list_pop_token(tokens);
                        skip_newlines = true;
                        idx = brd_parse_expression(tokens);
                        if (idx == NULL) {
                                BARF("bad index");
                        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RBRACKET) {
                                BARF("you're missing a right bracket");
                        }
                        skip_newlines = skip_copy;
                        node = brd_node_index_new(node, idx);
                        break;
                default:
                        return node;
                }
        }
}

struct brd_node *
brd_parse_base(struct brd_token_list *tokens)
{
        struct brd_token_list copy = *tokens;
        struct brd_node_arglist *items;
        struct brd_node *node;
        int skip_copy = skip_newlines;

        switch (brd_token_list_pop_token(tokens)) {
        case BRD_TOK_LPAREN:
                skip_newlines = true;
                node = brd_parse_expression(tokens);
                if (node == NULL) {
                        *tokens = copy;
                        skip_newlines = skip_copy;
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) == BRD_TOK_RPAREN) {
                        skip_newlines = skip_copy;
                        return node;
                } else {
                        BARF("You're missing a right paren there m8");
                        return NULL;
                }
        case BRD_TOK_LBRACKET:
                skip_newlines = true;
                items = brd_parse_arglist(tokens);
                if (items == NULL) {
                        BARF("bad argument list");
                } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RBRACKET) {
                        BARF("you're missing a right bracket");
                }
                skip_newlines = skip_copy;
                return brd_node_list_lit_new(items);
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
        case BRD_TOK_BUILTIN:
                return brd_node_builtin_new(brd_token_list_pop_string(tokens));
        case BRD_TOK_BEGIN:
                node = brd_parse_body(tokens);
                if (node == NULL) {
                        BARF("bad begin/end expression");
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) == BRD_TOK_END) {
                        return node;
                } else {
                        BARF("begin must be followed by end");
                        return NULL;
                }
                break;
        case BRD_TOK_IF:
                node = brd_parse_ifexpr(tokens);
                if (node == NULL) {
                        BARF("bad if expression");
                } else {
                        return node;
                }
                break;
        case BRD_TOK_WHILE:
                node = brd_parse_while(tokens);
                if (node == NULL) {
                        BARF("bad while expression");
                } else {
                        return node;
                }
                break;
        default:
                *tokens = copy;
                return NULL;
        }
}

struct brd_node_arglist *
brd_parse_arglist(struct brd_token_list *tokens)
{
        /* skip newlines handled by caller */
        /* I'm going to allow trailing commas */
        size_t length = 0, capacity = LIST_SIZE;
        struct brd_node **args = malloc(sizeof(struct brd_node *) * capacity);
        struct brd_node *node;
        for (;;) {

                node = brd_parse_expression(tokens);
                if (node != NULL) {
                        if (length == capacity) {
                                capacity *= GROW;
                                args = realloc(args, sizeof(struct brd_node *) * capacity);
                        }
                        args[length] = node;
                        length++;
                } else {
                        break;
                }

                if (brd_token_list_peek(tokens) == BRD_TOK_COMMA) {
                        brd_token_list_pop_token(tokens);
                } else {
                        break;
                }
        }

        return brd_node_arglist_new(args, length);
}

struct brd_node *
brd_parse_body(struct brd_token_list *tokens)
{
        size_t length = 0, capacity = LIST_SIZE;
        struct brd_node **stmts;
        struct brd_node *node;
        struct brd_token_list copy = *tokens;
        int skip_copy = skip_newlines;

        skip_newlines = false;

        /* if the next token isn't a newline, then  the body
         * is an inline single expression */
        if (brd_token_list_peek(tokens) != BRD_TOK_NEWLINE) {
                node = brd_parse_expression(tokens);
                skip_newlines = skip_copy;
                return node;
        } else {
                while (brd_token_list_peek(tokens) == BRD_TOK_NEWLINE) {
                        brd_token_list_pop_token(tokens);
                }
        }

        stmts = malloc(sizeof(struct brd_node *) * capacity);
        while ((node = brd_parse_body_stmt(tokens)) != NULL) {
                if (length == capacity){
                        capacity *= GROW;
                        stmts = realloc(stmts, sizeof(struct brd_node *) * capacity);
                }
                stmts[length] = node;
                length++;
        }

        skip_newlines = skip_copy;

        /* body must have at leat 1 statement */
        if (length >= 1) {
                return brd_node_body_new(stmts, length);
        } else {
                *tokens = copy;
                return NULL;
        }
}

/* skip_newlines should be false when this is called */
struct brd_node *
brd_parse_body_stmt(struct brd_token_list *tokens)
{
        struct brd_token_list copy = *tokens;
        struct brd_node *node;
        int found_term;

        node = brd_parse_expression(tokens);
        if (node == NULL) {
                return NULL;
        }

        found_term = false;
        for (;;) {
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_NEWLINE:
                        brd_token_list_pop_token(tokens);
                        found_term = true;
                        break;
                default:
                        if (found_term) {
                                return node;
                        } else {
                                *tokens = copy;
                                return NULL;
                        }
                }
        }
}

/* the if token has already been consumed */
struct brd_node *brd_parse_ifexpr(struct brd_token_list *tokens)
{
        size_t length = 0, capacity = LIST_SIZE;
        struct brd_node *cond, *body, *els;
        struct brd_node_elif *elifs;
        int skip_copy = skip_newlines;

        skip_newlines = true;
        cond = brd_parse_expression(tokens);
        if (cond == NULL) {
                BARF("error parsing if condition");
        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_THEN) {
                BARF("You're missing a then");
        }
        
        body = brd_parse_body(tokens);
        if (body == NULL) {
                BARF("error parsing if body");
        }
        elifs = malloc(sizeof(struct brd_node_elif) * capacity);
        for (;;) {
                struct brd_node_elif e = brd_parse_elif(tokens);
                if (e.cond == NULL) {
                        break;
                } else if (length >= capacity) {
                        capacity *= GROW;
                        elifs = realloc(elifs, sizeof(struct brd_node_elif) * capacity);
                }
                elifs[length] = e;
                length++;
        }

        if (brd_token_list_peek(tokens) == BRD_TOK_ELSE) {
                brd_token_list_pop_token(tokens);
                els = brd_parse_body(tokens);
                if (els == NULL) {
                        BARF("can't parse else body");
                }
        } else {
                els = NULL;
        }

        if (brd_token_list_pop_token(tokens) == BRD_TOK_END) {
                skip_newlines = skip_copy;
                return brd_node_ifexpr_new(cond, body, elifs, length, els);
        } else {
                BARF("there is no end");
                return NULL;
        }
}

/* 
 * yeah I'm returning a struct, sue me
 * the struct is only two points so, like, it's fine ig
 */
struct brd_node_elif brd_parse_elif(struct brd_token_list *tokens)
{
        struct brd_node_elif elif;

        if (brd_token_list_peek(tokens) == BRD_TOK_ELIF) {
                brd_token_list_pop_token(tokens);
        } else {
                return (struct brd_node_elif){ NULL, NULL };
        }

        elif.cond = brd_parse_expression(tokens);
        if (elif.cond == NULL) {
                BARF("oops");
        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_THEN) {
                BARF("You're missing a 'then'");
        }

        elif.body = brd_parse_body(tokens);
        if (elif.body == NULL) {
                BARF("oops");
        }

        return elif;
}

/* the while token has already been consumed */
struct brd_node *brd_parse_while(struct brd_token_list *tokens)
{
        struct brd_node *cond, *body;
        int skip_copy = skip_newlines;

        skip_newlines = true;
        cond = brd_parse_expression(tokens);
        if (brd_token_list_pop_token(tokens) != BRD_TOK_DO) {
                BARF("you're missing a do");
        }
        skip_newlines = skip_copy;
        body = brd_parse_body(tokens);
        if (brd_token_list_pop_token(tokens) != BRD_TOK_END) {
                BARF("there is no end");
        }

        return brd_node_while_new(cond, body);
}
