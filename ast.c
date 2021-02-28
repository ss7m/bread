#include "common.h"
#include "ast.h"

static struct brd_node *
brd_node_program_copy(struct brd_node *n)
{
        struct brd_node_program *p = (struct brd_node_program *)n;
        struct brd_node **stmts = malloc(sizeof(struct brd_node *) * p->num_stmts);
        for (size_t i = 0; i < p->num_stmts; i++) {
                stmts[i] = brd_node_copy(p->stmts[i]);
        }
        return brd_node_program_new(stmts, p->num_stmts);
}

static void
brd_node_program_destroy(struct brd_node *n)
{
        struct brd_node_program *p = (struct brd_node_program *)n;
        for (size_t i = 0; i < p->num_stmts; i++) {
                brd_node_destroy(p->stmts[i]);
        }
        free(p->stmts);
}

struct brd_node *
brd_node_program_new(struct brd_node **stmts, size_t num_stmts)
{
        struct brd_node_program *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_PROGRAM;
        n->_node.line_number = line_number;
        n->stmts = stmts;
        n->num_stmts = num_stmts;

        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_assign_copy(struct brd_node *n)
{
        struct brd_node_assign *a = (struct brd_node_assign *)n;
        return brd_node_assign_new(
                brd_node_copy(a->l),
                brd_node_copy(a->r)
        );
}

static void
brd_node_assign_destroy(struct brd_node *n)
{
        struct brd_node_assign *a = (struct brd_node_assign *)n;
        brd_node_destroy(a->l);
        brd_node_destroy(a->r);
}

struct brd_node *
brd_node_assign_new(struct brd_node *l, struct brd_node *r)
{
        struct brd_node_assign *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_ASSIGN;
        n->_node.line_number = line_number;
        n->l = l;
        n->r = r;

        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_binop_copy(struct brd_node *n)
{
        struct brd_node_binop *b = (struct brd_node_binop *)n;
        return brd_node_binop_new(
                b->btype,
                brd_node_copy(b->l),
                brd_node_copy(b->r)
        );
}

static void
brd_node_binop_destroy(struct brd_node *n)
{
        struct brd_node_binop *b = (struct brd_node_binop *)n;
        brd_node_destroy(b->l);
        brd_node_destroy(b->r);
}

struct brd_node *
brd_node_binop_new(enum brd_binop btype, struct brd_node *l, struct brd_node *r)
{
        struct brd_node_binop *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BINOP;
        n->_node.line_number = line_number;
        n->btype = btype;
        n->l = l;
        n->r = r;

        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_unary_copy(struct brd_node *n)
{
        struct brd_node_unary *u = (struct brd_node_unary *)n;
        return brd_node_unary_new(u->utype, brd_node_copy(u->u));
}

static void
brd_node_unary_destroy(struct brd_node *n)
{
        brd_node_destroy(((struct brd_node_unary *)n)->u);
}

struct brd_node *
brd_node_unary_new(enum brd_unary utype, struct brd_node *u)
{
        struct brd_node_unary *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_UNARY;
        n->_node.line_number = line_number;
        n->utype = utype;
        n->u = u;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_var_copy(struct brd_node *n)
{
        return brd_node_var_new(strdup(((struct brd_node_var *) n)->id));
}

static void
brd_node_var_destroy(struct brd_node *n)
{
        free(((struct brd_node_var *)n)->id);
}

struct brd_node *
brd_node_var_new(char *id)
{
        struct brd_node_var *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_VAR;
        n->_node.line_number = line_number;
        n->id = strdup(id);
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_num_lit_copy(struct brd_node *n)
{
        return brd_node_num_lit_new(((struct brd_node_num_lit *) n)->v);
}

static void
brd_node_num_lit_destroy(struct brd_node *n)
{
        (void)n;
}

struct brd_node *
brd_node_num_lit_new(long double v)
{
        struct brd_node_num_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_NUM_LIT;
        n->_node.line_number = line_number;
        n->v = v;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_string_lit_copy(struct brd_node *n)
{
        return brd_node_string_lit_new(strdup(((struct brd_node_string_lit *) n)->s));
}

static void
brd_node_string_lit_destroy(struct brd_node *n)
{
        free(((struct brd_node_string_lit *)n)->s);
}

struct brd_node *
brd_node_string_lit_new(char *s)
{
        struct brd_node_string_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_STRING_LIT;
        n->_node.line_number = line_number;
        n->s = strdup(s);
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_bool_lit_copy(struct brd_node *n)
{
        return brd_node_bool_lit_new(((struct brd_node_bool_lit *) n)->b);
}

static void
brd_node_bool_lit_destroy(struct brd_node *n)
{
        (void)n;
}

struct brd_node *
brd_node_bool_lit_new(int b)
{
        struct brd_node_bool_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BOOL_LIT;
        n->_node.line_number = line_number;
        n->b = b;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_unit_lit_copy(struct brd_node *n)
{
        (void)n;
        return brd_node_unit_lit_new();
}

static void
brd_node_unit_lit_destroy(struct brd_node *n)
{
        (void)n;
}

struct brd_node *
brd_node_unit_lit_new(void)
{
        struct brd_node_unit_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_UNIT_LIT;
        n->_node.line_number = line_number;
        return (struct brd_node *)n;
}

static struct brd_node_arglist *
brd_node_arglist_copy(struct brd_node_arglist *a)
{
        struct brd_node **args = malloc(sizeof(struct brd_node *) * a->num_args);
        for (size_t i = 0; i < a->num_args; i++) {
                args[i] = brd_node_copy(a->args[i]);
        }
        return brd_node_arglist_new(args, a->num_args);
}

void
brd_node_arglist_destroy(struct brd_node_arglist *args)
{
        for (size_t i = 0; i < args->num_args; i++) {
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

static struct brd_node *
brd_node_list_lit_copy(struct brd_node *n)
{
        return brd_node_list_lit_new(
                brd_node_arglist_copy(((struct brd_node_list_lit *) n)->items)
        );
}

static void
brd_node_list_lit_destroy(struct brd_node *n)
{
        brd_node_arglist_destroy(((struct brd_node_list_lit *)n)->items);
}

struct brd_node *
brd_node_list_lit_new(struct brd_node_arglist *items)
{
        struct brd_node_list_lit *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_LIST_LIT;
        n->_node.line_number = line_number;
        n->items = items;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_funcall_copy(struct brd_node *n)
{
        struct brd_node_funcall *f = (struct brd_node_funcall *)n;
        return brd_node_funcall_new(
                brd_node_copy(f->fn),
                brd_node_arglist_copy(f->args)
        );
}

static void
brd_node_funcall_destroy(struct brd_node *n)
{
        struct brd_node_funcall *f = (struct brd_node_funcall *)n;
        brd_node_destroy(f->fn);
        brd_node_arglist_destroy(f->args);
}

struct brd_node *
brd_node_funcall_new(struct brd_node *fn, struct brd_node_arglist *args)
{
        struct brd_node_funcall *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_FUNCALL;
        n->_node.line_number = line_number;
        n->fn = fn;
        n->args = args;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_closure_copy(struct brd_node *n)
{
        struct brd_node_closure *c = (struct brd_node_closure *)n;
        char **args = malloc(sizeof(char *) * c->num_args);
        for (size_t i = 0; i < c->num_args; i++) {
                args[i] = strdup(c->args[i]);
        }
        return brd_node_closure_new(args, c->num_args, brd_node_copy(c->body));
}

static void
brd_node_closure_destroy(struct brd_node *n)
{
        struct brd_node_closure *c = (struct brd_node_closure *)n;
        for (size_t i = 0; i < c->num_args; i++) {
                free(c->args[i]);
        }
        free(c->args);
        brd_node_destroy(c->body);
}

struct brd_node *
brd_node_closure_new(char **args, size_t num_args, struct brd_node *body)
{
        struct brd_node_closure *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_CLOSURE;
        n->_node.line_number = line_number;
        n->args = args;
        n->num_args = num_args;
        n->body = body;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_builtin_copy(struct brd_node *n)
{
        return brd_node_builtin_new(strdup(((struct brd_node_builtin *) n)->builtin));
}

static void
brd_node_builtin_destroy(struct brd_node *n)
{
        struct brd_node_builtin *b = (struct brd_node_builtin *)n;
        free(b->builtin);
}

struct brd_node *
brd_node_builtin_new(char *builtin)
{
        struct brd_node_builtin *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BUILTIN;
        n->_node.line_number = line_number;
        n->builtin = strdup(builtin);
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_body_copy(struct brd_node *n)
{
        struct brd_node_body *b = (struct brd_node_body *)n;
        struct brd_node **stmts = malloc(sizeof(struct brd_node*) * b->num_stmts);
        for (size_t i = 0; i < b->num_stmts; i++) {
                stmts[i] = brd_node_copy(b->stmts[i]);
        }
        return brd_node_body_new(stmts, b->num_stmts);
}

static void
brd_node_body_destroy(struct brd_node *n)
{
        struct brd_node_body *b = (struct brd_node_body *)n;
        for (size_t i = 0; i < b->num_stmts; i++) {
                brd_node_destroy(b->stmts[i]);
        }
        free(b->stmts);
}

struct brd_node *
brd_node_body_new(struct brd_node **stmts, size_t num_stmts)
{
        struct brd_node_body *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_BODY;
        n->_node.line_number = line_number;
        n->stmts = stmts;
        n->num_stmts = num_stmts;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_ifexpr_copy(struct brd_node *n)
{
        struct brd_node_ifexpr *b = (struct brd_node_ifexpr *)n;
        struct brd_node_elif *elifs =
                malloc(sizeof(struct brd_node_elif) * b->num_elifs);
        for (size_t i = 0; i < b->num_elifs; i++) {
                elifs[i].cond = brd_node_copy(b->elifs[i].cond);
                elifs[i].body = brd_node_copy(b->elifs[i].body);
        }
        return brd_node_ifexpr_new(
                brd_node_copy(b->cond),
                brd_node_copy(b->body),
                elifs, b->num_elifs,
                b->els ? brd_node_copy(b->els) : NULL
        );
}

static void
brd_node_ifexpr_destroy(struct brd_node *n)
{
        struct brd_node_ifexpr *b = (struct brd_node_ifexpr *)n;
        brd_node_destroy(b->cond);
        brd_node_destroy(b->body);
        for (size_t i = 0; i < b->num_elifs; i++) {
                brd_node_destroy(b->elifs[i].cond);
                brd_node_destroy(b->elifs[i].body);
        }
        free(b->elifs);
        if (b->els != NULL) {
                brd_node_destroy(b->els);
        }
}

struct brd_node *
brd_node_ifexpr_new(
        struct brd_node *cond,
        struct brd_node *body,
        struct brd_node_elif *elifs,
        size_t num_elifs,
        struct brd_node *els)
{
        struct brd_node_ifexpr *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_IFEXPR;
        n->_node.line_number = line_number;
        n->cond = cond;
        n->body = body;
        n->elifs = elifs;
        n->num_elifs = num_elifs;
        n->els = els;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_index_copy(struct brd_node *n)
{
        struct brd_node_index *i = (struct brd_node_index *)n;
        return brd_node_index_new(brd_node_copy(i->list), brd_node_copy(i->idx));
}

static void
brd_node_index_destroy(struct brd_node *n)
{
        struct brd_node_index *i = (struct brd_node_index *)n;
        brd_node_destroy(i->list);
        brd_node_destroy(i->idx);
}

struct brd_node *
brd_node_index_new(struct brd_node *list, struct brd_node *idx)
{
        struct brd_node_index *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_INDEX;
        n->_node.line_number = line_number;
        n->list = list;
        n->idx = idx;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_while_copy(struct brd_node *n)
{
        struct brd_node_while *w = (struct brd_node_while *)n;
        return brd_node_while_new(
                w->no_list,
                brd_node_copy(w->cond),
                brd_node_copy(w->body),
                w->inc ? brd_node_copy(w->inc) : NULL
        );
}

static void
brd_node_while_destroy(struct brd_node *n)
{
        struct brd_node_while *w = (struct brd_node_while *)n;
        brd_node_destroy(w->cond);
        brd_node_destroy(w->body);
        if (w->inc != NULL) {
                brd_node_destroy(w->inc);
        }
}

struct brd_node *
brd_node_while_new(int no_list, struct brd_node *cond, struct brd_node *body, struct brd_node *inc)
{
        struct brd_node_while *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_WHILE;
        n->_node.line_number = line_number;
        n->no_list = no_list;
        n->cond = cond;
        n->body = body;
        n->inc = inc;
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_field_copy(struct brd_node *n)
{
        struct brd_node_field *m = (struct brd_node_field *)n;
        return brd_node_field_new(
                brd_node_copy(m->object),
                strdup(m->field)
        );
}

static void
brd_node_field_destroy(struct brd_node *n)
{
        struct brd_node_field *m = (struct brd_node_field *)n;
        brd_node_destroy(m->object);
        free(m->field);
}

struct brd_node *
brd_node_field_new(struct brd_node *object, char *field)
{
        struct brd_node_field *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_FIELD;
        n->_node.line_number = line_number;
        n->object = object;
        n->field = strdup(field);
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_acc_obj_copy(struct brd_node *n)
{
        struct brd_node_acc_obj *m = (struct brd_node_acc_obj *)n;
        return brd_node_acc_obj_new(
                brd_node_copy(m->object),
                strdup(m->id)
        );
}

static void
brd_node_acc_obj_destroy(struct brd_node *n)
{
        struct brd_node_acc_obj *m = (struct brd_node_acc_obj *)n;
        brd_node_destroy(m->object);
        free(m->id);
}

struct brd_node *
brd_node_acc_obj_new(struct brd_node *object, char *id)
{
        struct brd_node_acc_obj *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_ACC_OBJ;
        n->_node.line_number = line_number;
        n->object = object;
        n->id = strdup(id);
        return (struct brd_node *)n;
}

static struct brd_node *
brd_node_subclass_copy(struct brd_node *n)
{
        struct brd_node_subclass *s = (struct brd_node_subclass *)n;
        struct brd_node_subclass_set *decs =
                malloc(sizeof(struct brd_node_subclass_set) * s->num_decs);
        for (size_t i = 0; i < s->num_decs; i++) {
                decs[i].id = strdup(s->decs[i].id);
                decs[i].expression = brd_node_copy(s->decs[i].expression);
        }
        return brd_node_subclass_new(
                brd_node_copy(s->super),
                brd_node_copy(s->constructor),
                decs, s->num_decs
        );
}

static void
brd_node_subclass_destroy(struct brd_node *n)
{
        struct brd_node_subclass *s = (struct brd_node_subclass *)n;
        brd_node_destroy(s->super);
        brd_node_destroy((struct brd_node *)s->constructor);
        for (size_t i = 0; i < s->num_decs; i++) {
                free(s->decs[i].id);
                brd_node_destroy(s->decs[i].expression);
        }
        free(s->decs);
}

struct brd_node *
brd_node_subclass_new(
        struct brd_node *super,
        struct brd_node *constructor,
        struct brd_node_subclass_set *decs,
        size_t num_decs)
{
        struct brd_node_subclass *n = malloc(sizeof(*n));
        n->_node.ntype = BRD_NODE_SUBCLASS;
        n->_node.line_number = line_number;
        n->super = super;
        n->constructor = constructor;
        n->decs = decs;
        n->num_decs = num_decs;
        return (struct brd_node *)n;
}

void
brd_node_destroy(struct brd_node *node)
{
        switch(node->ntype) {
        case BRD_NODE_ASSIGN: brd_node_assign_destroy(node); break;
        case BRD_NODE_BINOP: brd_node_binop_destroy(node); break;
        case BRD_NODE_UNARY: brd_node_unary_destroy(node); break;
        case BRD_NODE_VAR: brd_node_var_destroy(node); break;
        case BRD_NODE_NUM_LIT: brd_node_num_lit_destroy(node); break;
        case BRD_NODE_STRING_LIT: brd_node_string_lit_destroy(node); break;
        case BRD_NODE_BOOL_LIT: brd_node_bool_lit_destroy(node); break;
        case BRD_NODE_UNIT_LIT: brd_node_unit_lit_destroy(node); break;
        case BRD_NODE_LIST_LIT: brd_node_list_lit_destroy(node); break;
        case BRD_NODE_FUNCALL: brd_node_funcall_destroy(node); break;
        case BRD_NODE_CLOSURE: brd_node_closure_destroy(node); break;
        case BRD_NODE_BUILTIN: brd_node_builtin_destroy(node); break;
        case BRD_NODE_BODY: brd_node_body_destroy(node); break;
        case BRD_NODE_IFEXPR: brd_node_ifexpr_destroy(node); break;
        case BRD_NODE_INDEX: brd_node_index_destroy(node); break;
        case BRD_NODE_WHILE: brd_node_while_destroy(node); break;
        case BRD_NODE_FIELD: brd_node_field_destroy(node); break;
        case BRD_NODE_ACC_OBJ: brd_node_acc_obj_destroy(node); break;
        case BRD_NODE_SUBCLASS: brd_node_subclass_destroy(node); break;
        case BRD_NODE_PROGRAM: brd_node_program_destroy(node); break;
        }
        free(node);
}

struct brd_node *
brd_node_copy(struct brd_node *node)
{
        switch(node->ntype) {
        case BRD_NODE_ASSIGN: return brd_node_assign_copy(node);
        case BRD_NODE_BINOP: return brd_node_binop_copy(node);
        case BRD_NODE_UNARY: return brd_node_unary_copy(node);
        case BRD_NODE_VAR: return brd_node_var_copy(node);
        case BRD_NODE_NUM_LIT: return brd_node_num_lit_copy(node);
        case BRD_NODE_STRING_LIT: return brd_node_string_lit_copy(node);
        case BRD_NODE_BOOL_LIT: return brd_node_bool_lit_copy(node);
        case BRD_NODE_UNIT_LIT: return brd_node_unit_lit_copy(node);
        case BRD_NODE_LIST_LIT: return brd_node_list_lit_copy(node);
        case BRD_NODE_FUNCALL: return brd_node_funcall_copy(node);
        case BRD_NODE_CLOSURE: return brd_node_closure_copy(node);
        case BRD_NODE_BUILTIN: return brd_node_builtin_copy(node);
        case BRD_NODE_BODY: return brd_node_body_copy(node);
        case BRD_NODE_IFEXPR: return brd_node_ifexpr_copy(node);
        case BRD_NODE_INDEX: return brd_node_index_copy(node);
        case BRD_NODE_WHILE: return brd_node_while_copy(node);
        case BRD_NODE_FIELD: return brd_node_field_copy(node);
        case BRD_NODE_ACC_OBJ: return brd_node_acc_obj_copy(node);
        case BRD_NODE_SUBCLASS: return brd_node_subclass_copy(node);
        case BRD_NODE_PROGRAM: return brd_node_program_copy(node);
        }
        BARF("what?");
        return NULL;
}
