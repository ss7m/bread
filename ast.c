#include "common.h"
#include "ast.h"

/*
 * Destroys the base node
 * Every destroy function should call this
 */
static void
_flt_node_destroy(struct flt_node *n)
{
        free(n);
}

size_t
flt_node_type_sizeof(enum flt_node_type t)
{
        switch(t) {
        case FLT_NODE_ASSIGN:
                return sizeof(struct flt_node_assign);
        case FLT_NODE_BINOP:
                return sizeof(struct flt_node_binop);
        case FLT_NODE_UNARY:
                return sizeof(struct flt_node_unary);
        case FLT_NODE_VAR:
                return sizeof(struct flt_node_var);
        case FLT_NODE_NUM_LIT:
                return sizeof(struct flt_node_num_lit);
        case FLT_NODE_STRING_LIT:
                return sizeof(struct flt_node_string_lit);
        case FLT_NODE_BOOL_LIT:
                return sizeof(struct flt_node_bool_lit);
        case FLT_NODE_UNIT_LIT:
                return sizeof(struct flt_node_unit_lit);
        case FLT_NODE_MAX:
                BARF("Invalid node of type FLT_NODE_MAX");
        }
        return -1;
}

static void
flt_node_assign_destroy(struct flt_node *n)
{
        struct flt_node_assign *a = (struct flt_node_assign *)n;
        flt_node_destroy(a->l);
        flt_node_destroy(a->r);
        _flt_node_destroy(n);
}

struct flt_node *
flt_node_assin_new(struct flt_node *l, struct flt_node *r)
{
        struct flt_node_assign *n = malloc(sizeof(*n));
        assert(flt_node_is_lvalue(l));
        n->_node.ntype = FLT_NODE_ASSIGN;
        n->_node.destroy = flt_node_assign_destroy;
        n->l = l;
        n->r = r;

        return (struct flt_node *)n;
}

static void
flt_node_binop_destroy(struct flt_node *n)
{
        struct flt_node_binop *b = (struct flt_node_binop *)n;
        flt_node_destroy(b->l);
        flt_node_destroy(b->r);
        _flt_node_destroy(n);
}

struct flt_node *
flt_node_binop_new(enum flt_binop btype, struct flt_node *l, struct flt_node *r)
{
        struct flt_node_binop *n = malloc(sizeof(*n));
        n->_node.ntype = FLT_NODE_BINOP;
        n->_node.destroy = flt_node_binop_destroy;
        n->btype = btype;
        n->l = l;
        n->r = r;

        return (struct flt_node *)n;
}

static void
flt_node_unary_destroy(struct flt_node *n)
{
        flt_node_destroy(((struct flt_node_unary *)n)->u);
        _flt_node_destroy(n);
}

struct flt_node *
flt_node_unary_new(enum flt_unary utype, struct flt_node *u)
{
        struct flt_node_unary *n = malloc(sizeof(*n));
        n->_node.ntype = FLT_NODE_UNARY;
        n->_node.destroy = flt_node_unary_destroy;
        n->utype = utype;
        n->u = u;
        return (struct flt_node *)n;
}

static void
flt_node_var_destroy(struct flt_node *n)
{
        free(((struct flt_node_var *)n)->id);
        _flt_node_destroy(n);
}

struct flt_node *
flt_node_var_new(char *id)
{
        struct flt_node_var *n = malloc(sizeof(*n));
        n->_node.ntype = FLT_NODE_VAR;
        n->_node.destroy = flt_node_var_destroy;
        n->id = id;
        return (struct flt_node *)n;
}

struct flt_node *
flt_node_num_lit_new(long double v)
{
        struct flt_node_num_lit *n = malloc(sizeof(*n));
        n->_node.ntype = FLT_NODE_NUM_LIT;
        n->_node.destroy = _flt_node_destroy;
        n->v = v;
        return (struct flt_node *)n;
}

static void
flt_node_string_lit_destroy(struct flt_node *n)
{
        free(((struct flt_node_string_lit *)n)->s);
        _flt_node_destroy(n);
}

struct flt_node *
flt_node_string_lit_new(char *s)
{
        struct flt_node_string_lit *n = malloc(sizeof(*n));
        n->_node.ntype = FLT_NODE_STRING_LIT;
        n->_node.destroy = flt_node_string_lit_destroy;
        n->s = s;
        return (struct flt_node *)n;
}

struct flt_node *
flt_node_bool_lit_new(int b)
{
        struct flt_node_bool_lit *n = malloc(sizeof(*n));
        n->_node.ntype = FLT_NODE_BOOL_LIT;
        n->_node.destroy = _flt_node_destroy;
        n->b = b;
        return (struct flt_node *)n;
}

struct flt_node *
flt_node_unit_lit_new()
{
        struct flt_node_unit_lit *n = malloc(sizeof(*n));
        n->_node.ntype = FLT_NODE_UNIT_LIT;
        n->_node.destroy = _flt_node_destroy;
        return (struct flt_node *)n;
}

/*
 * Utility functions
 */

int flt_node_is_lvalue(struct flt_node *n) {
        switch (n->ntype) {
        case FLT_NODE_VAR:
                return true;
        default:
                return false;
        }
}
