#ifndef FLT_AST_H
#define FLT_AST_H

enum flt_node_type {
        FLT_NODE_INT_LIT,
        FLT_NODE_STRING_LIT,
        FLT_NODE_MAX,
};

struct flt_node;
typedef void (*flt_node_fn)(struct flt_node *);

struct flt_node {
        enum flt_node_type ntype;
        flt_node_fn destroy;
};

struct flt_node_int_lit {
        struct flt_node _node;
        long long v;
};

struct flt_node_string_lit {
        struct flt_node _node;
        char *s;
};

void flt_node_int_lit_init(struct flt_node_int_lit *n, long long v);
void flt_node_string_lit_init(struct flt_node_string_lit *n, char *s);

#define flt_node_destroy(n) (((struct flt_node *)n)->destroy((struct flt_node *)n))

#endif
