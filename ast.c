#include "common.h"
#include "ast.h"

/*
 * Destroys the base node
 * Every destroy function should call this
 */
static void
_brd_node_destroy(struct brd_node *n)
{
        free(n);
}

static void
brd_node_program_destroy(struct brd_node *n)
{
        struct brd_node_program *p = (struct brd_node_program *)n;
        for (int i = 0; i < p->num_stmts; i++) {
                brd_node_destroy(p->stmts[i]);
        }
        free(p->stmts);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_program_new(struct brd_node **stmts, size_t num_stmts)
{
        struct brd_node_program *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_PROGRAM;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_program_destroy;
        n->stmts = stmts;
        n->num_stmts = num_stmts;

        return (struct brd_node *)n;
}

static void
brd_node_assign_destroy(struct brd_node *n)
{
        struct brd_node_assign *a = (struct brd_node_assign *)n;
        brd_node_destroy(a->l);
        brd_node_destroy(a->r);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_assign_new(struct brd_node *l, struct brd_node *r)
{
        struct brd_node_assign *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_ASSIGN;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_assign_destroy;
        n->l = l;
        n->r = r;

        return (struct brd_node *)n;
}

static void
brd_node_binop_destroy(struct brd_node *n)
{
        struct brd_node_binop *b = (struct brd_node_binop *)n;
        brd_node_destroy(b->l);
        brd_node_destroy(b->r);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_binop_new(enum brd_binop btype, struct brd_node *l, struct brd_node *r)
{
        struct brd_node_binop *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BINOP;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_binop_destroy;
        n->btype = btype;
        n->l = l;
        n->r = r;

        return (struct brd_node *)n;
}

static void
brd_node_unary_destroy(struct brd_node *n)
{
        brd_node_destroy(((struct brd_node_unary *)n)->u);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_unary_new(enum brd_unary utype, struct brd_node *u)
{
        struct brd_node_unary *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_UNARY;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_unary_destroy;
        n->utype = utype;
        n->u = u;
        return (struct brd_node *)n;
}

static void
brd_node_var_destroy(struct brd_node *n)
{
        free(((struct brd_node_var *)n)->id);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_var_new(char *id)
{
        struct brd_node_var *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_VAR;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_var_destroy;
        n->id = strdup(id);
        return (struct brd_node *)n;
}

struct brd_node *
brd_node_num_lit_new(long double v)
{
        struct brd_node_num_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_NUM_LIT;
        n->_node.line_number = line_number;
        n->_node.destroy = _brd_node_destroy;
        n->v = v;
        return (struct brd_node *)n;
}

static void
brd_node_string_lit_destroy(struct brd_node *n)
{
        free(((struct brd_node_string_lit *)n)->s);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_string_lit_new(char *s)
{
        struct brd_node_string_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_STRING_LIT;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_string_lit_destroy;
        n->s = strdup(s);
        return (struct brd_node *)n;
}

struct brd_node *
brd_node_bool_lit_new(int b)
{
        struct brd_node_bool_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BOOL_LIT;
        n->_node.line_number = line_number;
        n->_node.destroy = _brd_node_destroy;
        n->b = b;
        return (struct brd_node *)n;
}

struct brd_node *
brd_node_unit_lit_new(void)
{
        struct brd_node_unit_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_UNIT_LIT;
        n->_node.line_number = line_number;
        n->_node.destroy = _brd_node_destroy;
        return (struct brd_node *)n;
}

static void
brd_node_arglist_destroy(struct brd_node_arglist *args)
{
        for (int i = 0; i < args->num_args; i++) {
                brd_node_destroy(args->args[i]);
        }
        free(args->args);
        free(args);
}

struct brd_node_arglist *
brd_node_arglist_new(struct brd_node **args, size_t num_args)
{
        struct brd_node_arglist *n = malloc(sizeof(*n));
        n->args = args;
        n->num_args = num_args;
        return n;
}

static void
brd_node_list_lit_destroy(struct brd_node *n)
{
        brd_node_arglist_destroy(((struct brd_node_list_lit *)n)->items);
        _brd_node_destroy(n);
}

struct brd_node *brd_node_list_lit_new(struct brd_node_arglist *items)
{
        struct brd_node_list_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_LIST_LIT;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_list_lit_destroy;
        n->items = items;
        return (struct brd_node *)n;
}

static void
brd_node_funcall_destroy(struct brd_node *n)
{
        struct brd_node_funcall *f = (struct brd_node_funcall *)n;
        brd_node_destroy(f->fn);
        brd_node_arglist_destroy(f->args);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_funcall_new(struct brd_node *fn, struct brd_node_arglist *args)
{
        struct brd_node_funcall *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_FUNCALL;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_funcall_destroy;
        n->fn = fn;
        n->args = args;
        return (struct brd_node *)n;
}

static void
brd_node_closure_destroy(struct brd_node *n)
{
        struct brd_node_closure *c = (struct brd_node_closure *)n;
        for (int i = 0; i < c->num_args; i++) {
                free(c->args[i]);
        }
        free(c->args);
        brd_node_destroy(c->body);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_closure_new(char **args, size_t num_args, struct brd_node *body)
{
        struct brd_node_closure *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_CLOSURE;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_closure_destroy;
        n->args = args;
        n->num_args = num_args;
        n->body = body;
        return (struct brd_node *)n;
}

static void
brd_node_builtin_destroy(struct brd_node *n)
{
        struct brd_node_builtin *b = (struct brd_node_builtin *)n;
        free(b->builtin);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_builtin_new(char *builtin)
{
        struct brd_node_builtin *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BUILTIN;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_builtin_destroy;
        n->builtin = strdup(builtin);
        return (struct brd_node *)n;
}

static void
brd_node_body_destroy(struct brd_node *n)
{
        struct brd_node_body *b = (struct brd_node_body *)n;
        for (int i = 0; i < b->num_stmts; i++) {
                brd_node_destroy(b->stmts[i]);
        }
        free(b->stmts);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_body_new(struct brd_node **stmts, size_t num_stmts)
{
        struct brd_node_body *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BODY;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_body_destroy;
        n->stmts = stmts;
        n->num_stmts = num_stmts;
        return (struct brd_node *)n;
}

static void
brd_node_ifexpr_destroy(struct brd_node *n)
{
        struct brd_node_ifexpr *b = (struct brd_node_ifexpr *)n;
        brd_node_destroy(b->cond);
        brd_node_destroy(b->body);
        for (int i = 0; i < b->num_elifs; i++) {
                brd_node_destroy(b->elifs[i].cond);
                brd_node_destroy(b->elifs[i].body);
        }
        free(b->elifs);
        if (b->els != NULL) {
                brd_node_destroy(b->els);
        }
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_ifexpr_new(struct brd_node *cond, struct brd_node *body, struct brd_node_elif *elifs, size_t num_elifs, struct brd_node *els)
{
        struct brd_node_ifexpr *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_IFEXPR;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_ifexpr_destroy;
        n->cond = cond;
        n->body = body;
        n->elifs = elifs;
        n->num_elifs = num_elifs;
        n->els = els;
        return (struct brd_node *)n;
}

static void
brd_node_index_destroy(struct brd_node *n)
{
        struct brd_node_index *i = (struct brd_node_index *)n;
        brd_node_destroy(i->list);
        brd_node_destroy(i->idx);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_index_new(struct brd_node *list, struct brd_node *idx)
{
        struct brd_node_index *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_INDEX;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_index_destroy;
        n->list = list;
        n->idx = idx;
        return (struct brd_node *)n;
}

static void
brd_node_while_destroy(struct brd_node *n)
{
        struct brd_node_while *w = (struct brd_node_while *)n;
        brd_node_destroy(w->cond);
        brd_node_destroy(w->body);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_while_new(int no_list, struct brd_node *cond, struct brd_node *body)
{
        struct brd_node_while *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_WHILE;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_while_destroy;
        n->no_list = no_list;
        n->cond = cond;
        n->body = body;
        return (struct brd_node *)n;
}

static void
brd_node_field_destroy(struct brd_node *n)
{
        struct brd_node_field *m = (struct brd_node_field *)n;
        brd_node_destroy(m->object);
        free(m->field);
        _brd_node_destroy(n);
}

struct brd_node *
brd_node_field_new(struct brd_node *object, char *field)
{
        struct brd_node_field *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_FIELD;
        n->_node.line_number = line_number;
        n->_node.destroy = brd_node_field_destroy;
        n->object = object;
        n->field = strdup(field);
        return (struct brd_node *)n;
}
