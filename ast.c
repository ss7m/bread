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

size_t
brd_node_type_sizeof(enum brd_node_type t)
{
        switch(t) {
        case BRD_NODE_ASSIGN:
                return sizeof(struct brd_node_assign);
        case BRD_NODE_BINOP:
                return sizeof(struct brd_node_binop);
        case BRD_NODE_UNARY:
                return sizeof(struct brd_node_unary);
        case BRD_NODE_VAR:
                return sizeof(struct brd_node_var);
        case BRD_NODE_NUM_LIT:
                return sizeof(struct brd_node_num_lit);
        case BRD_NODE_STRING_LIT:
                return sizeof(struct brd_node_string_lit);
        case BRD_NODE_BOOL_LIT:
                return sizeof(struct brd_node_bool_lit);
        case BRD_NODE_UNIT_LIT:
                return sizeof(struct brd_node_unit_lit);
        case BRD_NODE_MAX:
                BARF("Invalid node of type BRD_NODE_MAX");
        }
        return -1;
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
        assert(brd_node_is_lvalue(l));
        n->_node.ntype = BRD_NODE_ASSIGN;
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
        n->_node.destroy = brd_node_var_destroy;
        n->id = id;
        return (struct brd_node *)n;
}

struct brd_node *
brd_node_num_lit_new(long double v)
{
        struct brd_node_num_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_NUM_LIT;
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
        n->_node.destroy = brd_node_string_lit_destroy;
        n->s = s;
        return (struct brd_node *)n;
}

struct brd_node *
brd_node_bool_lit_new(int b)
{
        struct brd_node_bool_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BOOL_LIT;
        n->_node.destroy = _brd_node_destroy;
        n->b = b;
        return (struct brd_node *)n;
}

struct brd_node *
brd_node_unit_lit_new()
{
        struct brd_node_unit_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_UNIT_LIT;
        n->_node.destroy = _brd_node_destroy;
        return (struct brd_node *)n;
}

/*
 * Utility functions
 */

int brd_node_is_lvalue(struct brd_node *n) {
        switch (n->ntype) {
        case BRD_NODE_VAR:
                return true;
        default:
                return false;
        }
}
