#ifndef FLT_AST_H
#define FLT_AST_H

/*
 * Nodes are ALWAYS allocated on the heap!
 */

enum flt_node_type {
        FLT_NODE_ASSIGN,
        FLT_NODE_BINOP,
        FLT_NODE_UNARY,
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

struct flt_node_assign {
        struct flt_node _node;
        struct flt_node *l, *r;
};

enum flt_binop {
        /* arith */
        FLT_PLUS,
        FLT_MINUS,
        FLT_MUL,
        FLT_DIV,

        /* comp */
        FLT_LT,
        FLT_LEQ,
        FLT_EQ,

        /* boolean */
        FLT_AND,
        FLT_OR,
};

struct flt_node_binop {
        struct flt_node _node;
        enum flt_binop btype;
        struct flt_node *l, *r;
};

enum flt_unary {
        FLT_NEGATE,
        FLT_NOT,
};

struct flt_node_unary {
        struct flt_node _node;
        enum flt_unary utype;
        struct flt_node *u;
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

struct flt_node *flt_node_assign_new(struct flt_node *l, struct flt_node *r);
struct flt_node *flt_node_binop_new(enum flt_binop btype, struct flt_node *l, struct flt_node *r);
struct flt_node *flt_node_unary_new(enum flt_unary utype, struct flt_node *u);
struct flt_node *flt_node_var_new(char *id);
struct flt_node *flt_node_num_lit_new(long double v);
struct flt_node *flt_node_string_lit_new(char *s);
struct flt_node *flt_node_bool_lit_new(int b);
struct flt_node *flt_node_unit_lit_new();

#define flt_node_destroy(n) (((struct flt_node *)n)->destroy((struct flt_node *)n))

int flt_node_is_lvalue(struct flt_node *n);

#endif
