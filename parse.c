#include "common.h"
#include "token.h"
#include "ast.h"
#include "parse.h"

#define GROW 1.5
#define LIST_SIZE 4

/* declare parser global variables */
int skip_newlines = false;
int line_number = 1;
int found_eof = false;
const char *error_message = "";
char bad_character[2] = {0};

struct brd_node *
brd_parse_program(struct brd_token_list *tokens)
{
        size_t length = 0, capacity = LIST_SIZE;
        struct brd_node **stmts = malloc(sizeof(struct brd_node *) * capacity);
        struct brd_node *node;

        skip_newlines = false;
        found_eof = false;

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

        if (!found_eof) {
                for (int i = 0; i < length; i++) {
                        brd_node_destroy(stmts[i]);
                }
                free(stmts);
                return NULL;
        }

        return brd_node_program_new(stmts, length);
}

/* skip_newlines should be false when this is called */
struct brd_node *
brd_parse_expression_stmt(struct brd_token_list *tokens)
{
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
                        found_eof = true;
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
                                brd_node_destroy(node);
                                error_message = "expression statement with no "
                                                 "terminating newline";
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
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) != BRD_TOK_EQ) {
                        brd_node_destroy(l);
                        error_message = "expected an equals sign";
                        return NULL;
                } else if ((r = brd_parse_expression(tokens)) == NULL) {
                        brd_node_destroy(l);
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
                error_message = "expected variable name";
                return NULL;
        }

        for (;;) {
                switch (brd_token_list_peek(tokens)) {
                case BRD_TOK_LBRACKET:
                        brd_token_list_pop_token(tokens);
                        idx = brd_parse_expression(tokens);
                        if (idx == NULL) {
                                brd_node_destroy(node);
                                return NULL;
                        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RBRACKET) {
                                brd_node_destroy(node);
                                error_message = "expected a right bracket";
                                return NULL;
                        }
                        node = brd_node_index_new(node, idx);
                        break;
                case BRD_TOK_FIELD:
                        brd_token_list_pop_token(tokens);
                        if (brd_token_list_pop_token(tokens) != BRD_TOK_VAR) {
                                brd_node_destroy(node);
                                error_message = "expected an identifier";
                                return NULL;
                        }
                        node = brd_node_field_new(
                                node,
                                brd_token_list_pop_string(tokens)
                        );
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
                                brd_node_destroy(l);
                                return NULL;
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
                        brd_node_destroy(l);
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
                        brd_node_destroy(l);
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
        int neq = false;

        l = brd_parse_addexp(tokens);
        if (l == NULL) {
                return NULL;
        }

        switch (brd_token_list_peek(tokens)) {
        case BRD_TOK_LT:
                binop = BRD_LT;
                break;
        case BRD_TOK_LEQ:
                binop = BRD_LEQ;
                break;
        case BRD_TOK_EQ:
                binop = BRD_EQ;
                break;
        case BRD_TOK_GT:
                binop = BRD_GT;
                break;
        case BRD_TOK_GEQ:
                binop = BRD_GEQ;
                break;
        case BRD_TOK_NEQ:
                neq = true;
                binop = BRD_EQ;
                break;
        default:
                return l;
        }

        brd_token_list_pop_token(tokens); /* pop binop */
        r = brd_parse_addexp(tokens);
        if (r == NULL) {
                brd_node_destroy(l);
                return NULL;
        } else {
                l = brd_node_binop_new(binop, l, r);
                return neq ? brd_node_unary_new(BRD_NOT, l) : l;
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
                        break;
                case BRD_TOK_MINUS:
                        binop = BRD_MINUS;
                        break;
                default:
                        return l;
                }

                brd_token_list_pop_token(tokens); /* pop binop */
                r = brd_parse_mulexp(tokens);
                if (r == NULL) {
                        brd_node_destroy(l);
                        return NULL;
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
                        break;
                case BRD_TOK_DIV:
                        binop = BRD_DIV;
                        break;
                case BRD_TOK_IDIV:
                        binop = BRD_IDIV;
                        break;
                case BRD_TOK_MOD:
                        binop = BRD_MOD;
                        break;
                default:
                        return l;
                }

                brd_token_list_pop_token(tokens); /* pop binop */
                r = brd_parse_powexp(tokens);
                if (r == NULL) {
                        brd_node_destroy(l);
                        return NULL;
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
                        brd_node_destroy(l);
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
        struct brd_node *node;

        switch (brd_token_list_peek(tokens)) {
        case BRD_TOK_NOT:
                brd_token_list_pop_token(tokens);
                node = brd_parse_prefix(tokens);
                return (node == NULL) ? NULL : brd_node_unary_new(BRD_NOT, node);
        case BRD_TOK_MINUS:
                brd_token_list_pop_token(tokens);
                node = brd_parse_prefix(tokens);
                return (node == NULL) ? NULL : brd_node_unary_new(BRD_NEGATE, node);
        default:
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
                                goto error_exit;
                        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RPAREN) {
                                brd_node_arglist_destroy(args);
                                error_message = "expected a right parenthesis";
                                goto error_exit;
                        }
                        skip_newlines = skip_copy;
                        node = brd_node_funcall_new(node, args);
                        break;
                case BRD_TOK_LBRACKET:
                        brd_token_list_pop_token(tokens);
                        skip_newlines = true;
                        idx = brd_parse_expression(tokens);
                        if (idx == NULL) {
                                goto error_exit;
                        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RBRACKET) {
                                error_message = "expected a right bracket";
                                goto error_exit;
                        }
                        skip_newlines = skip_copy;
                        node = brd_node_index_new(node, idx);
                        break;
                case BRD_TOK_FIELD:
                        brd_token_list_pop_token(tokens);
                        if (brd_token_list_pop_token(tokens) != BRD_TOK_VAR) {
                                brd_node_destroy(node);
                                error_message = "expected an identifier";
                                return NULL;
                        }
                        node = brd_node_field_new(
                                node,
                                brd_token_list_pop_string(tokens)
                        );
                        break;
                case BRD_TOK_ACC_OBJ:
                        brd_token_list_pop_token(tokens);
                        if (brd_token_list_pop_token(tokens) != BRD_TOK_VAR) {
                                brd_node_destroy(node);
                                error_message = "expected an identifier";
                                return NULL;
                        }
                        node = brd_node_acc_obj_new(
                                node,
                                brd_token_list_pop_string(tokens)
                        );
                        break;
                default:
                        return node;
                }
        }

error_exit:
        brd_node_destroy(node);
        return NULL;
}

static int
brd_parse_skip_newlines(struct brd_token_list *tokens)
{
        if (brd_token_list_peek(tokens) != BRD_TOK_NEWLINE) {
                return false;
        } else {
                while (brd_token_list_peek(tokens) == BRD_TOK_NEWLINE) {
                        brd_token_list_pop_token(tokens);
                }
        }
        return true;
}

struct brd_node *
brd_parse_base(struct brd_token_list *tokens)
{
        struct brd_node_arglist *items;
        struct brd_node *node;
        int skip_copy = skip_newlines;

        switch (brd_token_list_peek(tokens)) {
        case BRD_TOK_LPAREN:
                brd_token_list_pop_token(tokens);
                skip_newlines = true;
                node = brd_parse_expression(tokens);
                if (node == NULL) {
                        skip_newlines = skip_copy;
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) == BRD_TOK_RPAREN) {
                        skip_newlines = skip_copy;
                        return node;
                } else {
                        error_message = "expected a right parenthesis";
                        return NULL;
                }
        case BRD_TOK_LBRACKET:
                brd_token_list_pop_token(tokens);
                skip_newlines = true;
                items = brd_parse_arglist(tokens);
                if (items == NULL) {
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RBRACKET) {
                        error_message = "expected a right bracket";
                        return NULL;
                }
                skip_newlines = skip_copy;
                return brd_node_list_lit_new(items);
        case BRD_TOK_VAR:
                brd_token_list_pop_token(tokens);
                return brd_node_var_new(brd_token_list_pop_string(tokens));
        case BRD_TOK_NUM:
                brd_token_list_pop_token(tokens);
                return brd_node_num_lit_new(brd_token_list_pop_num(tokens));
        case BRD_TOK_STR:
                brd_token_list_pop_token(tokens);
                return brd_node_string_lit_new(brd_token_list_pop_string(tokens));
        case BRD_TOK_UNIT:
                brd_token_list_pop_token(tokens);
                return brd_node_unit_lit_new();
        case BRD_TOK_TRUE:
                brd_token_list_pop_token(tokens);
                return brd_node_bool_lit_new(true);
        case BRD_TOK_FALSE:
                brd_token_list_pop_token(tokens);
                return brd_node_bool_lit_new(false);
        case BRD_TOK_BUILTIN:
                brd_token_list_pop_token(tokens);
                return brd_node_builtin_new(brd_token_list_pop_string(tokens));
        case BRD_TOK_BEGIN:
                brd_token_list_pop_token(tokens);
                node = brd_parse_body(tokens);
                if (node == NULL) {
                        return NULL;
                } else if (brd_token_list_pop_token(tokens) == BRD_TOK_END) {
                        return node;
                } else {
                        error_message = "expected an \"end\" token";
                        return NULL;
                }
        case BRD_TOK_IF:
                brd_token_list_pop_token(tokens);
                return brd_parse_ifexpr(tokens); /* could be null */
        case BRD_TOK_WHILE:
                brd_token_list_pop_token(tokens);
                return brd_parse_while(tokens); /* could be null */
        case BRD_TOK_FUNC:
                brd_token_list_pop_token(tokens);
                return brd_parse_func(tokens); /* could be null */
        case BRD_TOK_SUBCLASS:
                brd_token_list_pop_token(tokens);
                return brd_parse_subclass(tokens);
        default:
                error_message = "expected... something";
                return NULL;
        }
}

struct brd_node *
brd_parse_func(struct brd_token_list *tokens)
{
        struct brd_node *body;
        size_t length = 0, capacity = 4;
        char **args = malloc(sizeof(char *) * capacity);
        char *str;
        int skip_copy = skip_newlines;

        skip_newlines = true;
        if (brd_token_list_pop_token(tokens) != BRD_TOK_LPAREN) {
                error_message = "expected a left parenthesis";
                return NULL;
        }

        for (;;) {
                if (brd_token_list_peek(tokens) == BRD_TOK_VAR) {
                        brd_token_list_pop_token(tokens);
                        str = brd_token_list_pop_string(tokens);
                } else {
                        break;
                }

                if (length == capacity) {
                        capacity *= GROW;
                        args =realloc(args, sizeof(char *) * capacity);
                }
                args[length++] = strdup(str);

                if (brd_token_list_peek(tokens) == BRD_TOK_COMMA) {
                        brd_token_list_pop_token(tokens);
                } else {
                        break;
                }
        }

        if (brd_token_list_pop_token(tokens) != BRD_TOK_RPAREN) {
                error_message = "expected a right parenthesis";
                goto error_exit;
        }

        body = brd_parse_body(tokens);
        if (body == NULL) {
                goto error_exit;
        } else if (brd_token_list_pop_token(tokens) == BRD_TOK_END) {
                skip_newlines = skip_copy;
                return brd_node_closure_new(args, length, body);
        } else {
                brd_node_destroy(body);
                error_message = "expected an \"end\" token";
                goto error_exit;
        }

error_exit:
        for (int i = 0; i < length; i++) {
                free(args[i]);
        }
        free(args);
        return NULL;
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
                error_message = "body must have at least one statement";
                free(stmts);
                return NULL;
        }
}

/* skip_newlines should be false when this is called */
struct brd_node *
brd_parse_body_stmt(struct brd_token_list *tokens)
{
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
                                error_message = "expected a newline";
                                brd_node_destroy(node);
                                return NULL;
                        }
                }
        }
}

/* the if token has already been consumed */
struct brd_node *
brd_parse_ifexpr(struct brd_token_list *tokens)
{
        size_t length = 0, capacity = LIST_SIZE;
        struct brd_node *cond, *body, *els;
        struct brd_node_elif *elifs;
        int skip_copy = skip_newlines;

        skip_newlines = true;
        cond = brd_parse_expression(tokens);
        if (cond == NULL) {
                return NULL;
        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_THEN) {
                error_message = "expected a \"then\" token";
                return NULL;
        }
        
        body = brd_parse_body(tokens);
        if (body == NULL) {
                brd_node_destroy(cond);
                return NULL;
        }

        elifs = malloc(sizeof(struct brd_node_elif) * capacity);
        for (;;) {
                struct brd_node_elif e;
                if (!brd_parse_elif(tokens, &e)) {
                        break;
                }

                if (length >= capacity) {
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
                        goto error_exit;
                }
        } else {
                els = NULL;
        }

        if (brd_token_list_pop_token(tokens) == BRD_TOK_END) {
                skip_newlines = skip_copy;
                return brd_node_ifexpr_new(cond, body, elifs, length, els);
        } else {
                error_message = "expected an \"end\" token";
                goto error_exit;
        }

error_exit:
        brd_node_destroy(cond);
        brd_node_destroy(body);
        if (els != NULL) {
                brd_node_destroy(els);
        }
        for (int i = 0; i < length; i++) {
                brd_node_destroy(elifs[i].cond);
                brd_node_destroy(elifs[i].body);
        }
        free(elifs);
        return NULL;
}

/* 
 * return true on success, false otherwise
 */
int
brd_parse_elif(struct brd_token_list *tokens, struct brd_node_elif *elif)
{
        if (brd_token_list_peek(tokens) == BRD_TOK_ELIF) {
                brd_token_list_pop_token(tokens);
        } else {
                return false;
        }

        elif->cond = brd_parse_expression(tokens);
        if (elif->cond == NULL) {
                return false;
        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_THEN) {
                error_message = "expected a \"then\" token";
                brd_node_destroy(elif->cond);
                return false;
        }

        elif->body = brd_parse_body(tokens);
        if (elif->body == NULL) {
                brd_node_destroy(elif->cond);
                return false;
        } else {
                return true;
        }
}

/* the while token has already been consumed */
struct brd_node *
brd_parse_while(struct brd_token_list *tokens)
{
        struct brd_node *cond, *body;
        int skip_copy = skip_newlines, no_list;

        skip_newlines = true;

        if (brd_token_list_peek(tokens) == BRD_TOK_MUL) {
                brd_token_list_pop_token(tokens);
                no_list = true;
        } else {
                no_list = false;
        }

        cond = brd_parse_expression(tokens);
        if (cond == NULL) {
                return NULL;
        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_DO) {
                error_message = "expected a \"do\" token";
                brd_node_destroy(cond);
                return NULL;
        }

        skip_newlines = skip_copy;
        body = brd_parse_body(tokens);
        if (body == NULL) {
                brd_node_destroy(cond);
                return NULL;
        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_END) {
                error_message = "expected an \"end\" token";
                brd_node_destroy(cond);
                brd_node_destroy(body);
                return NULL;
        }

        return brd_node_while_new(no_list, cond, body);
}
struct brd_node *
brd_parse_subclass(struct brd_token_list *tokens)
{
        /* subclass token has already been popped */
        struct brd_node *constructor, *class;
        struct brd_node_subclass_set *decs;
        size_t length, capacity;
        int skip_copy = skip_newlines;

        skip_newlines = true;
        if (brd_token_list_pop_token(tokens) != BRD_TOK_LPAREN) {
                error_message = "expected a left parenthesis";
                return NULL;
        }
        class = brd_parse_expression(tokens);
        if (class == NULL) {
                return NULL;
        } else if (brd_token_list_pop_token(tokens) != BRD_TOK_RPAREN) {
                error_message= "expected a right parenthesis";
                return NULL;
        }

        skip_newlines = false;
        if (!brd_parse_skip_newlines(tokens)) {
                error_message = "expected a newline";
                brd_node_destroy(class);
                return NULL;
        }

        if (brd_token_list_pop_token(tokens) != BRD_TOK_CONSTRUCTOR) {
                error_message = "expected a constructor";
                brd_node_destroy(class);
                return NULL;
        }

        constructor = brd_parse_func(tokens);
        if (constructor == NULL) {
                brd_node_destroy(class);
                return NULL;
        } else if (!brd_parse_skip_newlines(tokens)) {
                error_message = "expected a newline";
                brd_node_destroy(constructor);
                brd_node_destroy(class);
                return NULL;
        }

        capacity = 4;
        length = 0;
        decs = malloc(sizeof(*decs) * capacity);

        for (;;) {
                char *id;
                struct brd_node *expression;

                if (brd_token_list_peek(tokens) != BRD_TOK_SET) {
                        break;
                } else {
                        brd_token_list_pop_token(tokens);
                }

                if (brd_token_list_pop_token(tokens) != BRD_TOK_VAR) {
                        error_message = "expected an identifier";
                        goto error_exit;
                } else {
                        id = brd_token_list_pop_string(tokens);
                }

                if (brd_token_list_pop_token(tokens) != BRD_TOK_EQ) {
                        error_message = "expected an = token";
                        goto error_exit;
                }

                expression = brd_parse_expression(tokens);
                if (expression == NULL) {
                        goto error_exit;
                } else if (!brd_parse_skip_newlines(tokens)) {
                        brd_node_destroy(tokens);
                        error_message = "expected a newline";
                        goto error_exit;
                }

                if (length >= capacity) {
                        capacity *= 1.5;
                        decs = realloc(decs, sizeof(*decs) * capacity);
                }
                decs[length].id = strdup(id);
                decs[length].expression = expression;
                length++;
        }

        if (brd_token_list_pop_token(tokens) != BRD_TOK_END) {
                error_message = "expected a newline";
                goto error_exit;
        } else {
                skip_newlines = skip_copy;
                return brd_node_subclass_new(
                        class,
                        constructor,
                        decs,
                        length
                );
        }

error_exit:
        brd_node_destroy(constructor);
        brd_node_destroy(class);
        for (int i = 0; i < length; i++) {
                free(decs[i].id);
                brd_node_destroy(decs[i].expression);
        }
        free(decs);
        return NULL;
}

