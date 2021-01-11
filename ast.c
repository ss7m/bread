#include "common.h"
#include "ast.h"

/*
 * Private functions
 */

static void
flt_node_nop(struct flt_node *n)
{
        (void) n;
}

size_t
flt_node_type_sizeof(enum flt_node_type t)
{
        switch(t) {
        case FLT_NODE_BINOP:
                return sizeof(struct flt_node_binop);
        case FLT_NODE_INT_LIT:
                return sizeof(struct flt_node_int_lit);
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
flt_node_binop_destroy(struct flt_node *n)
{
        struct flt_node_binop *b = (struct flt_node_binop *)n;
        flt_node_destroy(b->l);
        flt_node_destroy(b->r);
        free(b->l);
        free(b->r);
}

void
flt_node_binop_init(struct flt_node_binop *n, enum flt_binop btype, struct flt_node *l, struct flt_node *r)
{
        n->_node.ntype = FLT_NODE_BINOP;
        n->_node.destroy = flt_node_binop_destroy;
        n->btype = btype;
        n->l = l;
        n->r = r;
}

void
flt_node_int_lit_init(struct flt_node_int_lit *n, long long v)
{
        n->_node.ntype = FLT_NODE_INT_LIT;
        n->_node.destroy = flt_node_nop;
        n->v = v;
}

static void
flt_node_string_lit_destroy(struct flt_node *n)
{
        free(((struct flt_node_string_lit *)n)->s);
}

void
flt_node_string_lit_init(struct flt_node_string_lit *n, char *s)
{
        n->_node.ntype = FLT_NODE_STRING_LIT;
        n->_node.destroy = flt_node_string_lit_destroy;
        n->s = s;
}

void
flt_node_bool_lit_init(struct flt_node_bool_lit *n, int b)
{
        n->_node.ntype = FLT_NODE_BOOL_LIT;
        n->_node.destroy = flt_node_nop;
        n->b = b;
}

void
flt_node_unit_lit_init(struct flt_node_unit_lit *n)
{
        n->_node.ntype = FLT_NODE_UNIT_LIT;
        n->_node.destroy = flt_node_nop;
}
