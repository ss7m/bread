#ifndef FLT_AST_H
#define FLT_AST_H

/*
 * Nodes are ALWAYS allocated on the heap!
 */

enum flt_node_type {
        FLT_NODE_BINOP,
        FLT_NODE_VAR,
        FLT_NODE_NUM_LIT,
        FLT_NODE_STRING_LIT,
        FLT_NODE_BOOL_LIT,
        FLT_NODE_UNIT_LIT,
        FLT_NODE_MAX,
};

struct flt_node;
typedef void (*flt_node_fn)(struct flt_node *);

struct flt_node {
        enum flt_node_type ntype;
        flt_node_fn destroy;
};

enum flt_binop {
        FLT_PLUS,
        FLT_MINUS,
        FLT_MUL,
        FLT_DIV,
        FLT_AND,
        FLT_OR,
};

struct flt_node_binop {
        struct flt_node _node;
        enum flt_binop btype;
        struct flt_node *l, *r;
};

struct flt_node_var {
        struct flt_node _node;
        char *id;
};

struct flt_node_num_lit {
        struct flt_node _node;
        long double v;
};

struct flt_node_string_lit {
        struct flt_node _node;
        char *s;
};

struct flt_node_bool_lit {
        struct flt_node _node;
        int b;
};

struct flt_node_unit_lit {
        struct flt_node _node;
};

size_t flt_node_type_sizeof(enum flt_node_type t);
#define flt_node_sizeof(n) flt_node_type_sizeof(((struct flt_node *)n)->ntype)

void flt_node_binop_init(struct flt_node **n, enum flt_binop btype, struct flt_node *l, struct flt_node *r);
void flt_node_var_init(struct flt_node **n, char *id);
void flt_node_num_lit_init(struct flt_node **n, long double v);
void flt_node_string_lit_init(struct flt_node **n, char *s);
void flt_node_bool_lit_init(struct flt_node **n, int b);
void flt_node_unit_lit_init(struct flt_node **n);

#define flt_node_destroy(n) (((struct flt_node *)n)->destroy((struct flt_node *)n))

#endif
