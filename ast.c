#include <stdlib.h>

#include "ast.h"

static void
flt_node_nop(struct flt_node *n)
{
        (void) n;
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
flt_node_string_lit_init(struct flt_node_string_lit *n, char *s) {
        n->_node.ntype = FLT_NODE_STRING_LIT;
        n->_node.destroy = flt_node_string_lit_destroy;
        n->s = s;
}
