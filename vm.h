#ifndef BRD_VM_H
#define BRD_VM_H

/* 
 * If I were smarter I'd make the bucket grow
 * But for the mean time that feels like a premature optimization
 */
#define BUCKET_SIZE 32

/* stack is limited to 256 values */
/* That should be enough, right? */
#define STACK_SIZE 256

#define LIST_SIZE 32
#define GROW 1.5

enum brd_value_type {
        BRD_VAL_NUM,
        BRD_VAL_STRING,
        BRD_VAL_BOOL,
        BRD_VAL_UNIT,
};

struct brd_value {
        enum brd_value_type vtype;
        union {
                long double num;
                char *string;
                int boolean;
        } as;
};

struct brd_value_map_list {
        char *key;
        struct brd_value val;
        struct brd_value_map_list *next;
};

struct brd_value_map {
        struct brd_value_map_list bucket[BUCKET_SIZE];
};

void brd_value_map_init(struct brd_value_map *map);
void brd_value_map_destroy(struct brd_value_map *map);
void brd_value_map_set(struct brd_value_map *map, char *key, struct brd_value *val);
struct brd_value *brd_value_map_get(struct brd_value_map *map, char *key);

void brd_value_debug(struct brd_value *value);

/* VM bytecode */
/* Stack based virtual machine */

enum brd_bytecode {
        BRD_VM_NUM, /* has arg: long double */
        BRD_VM_STR, /* has arg: string */
        BRD_VM_GET_VAR, /* has arg: string */
        BRD_VM_TRUE,
        BRD_VM_FALSE,
        BRD_VM_UNIT,

        /* arithmetic */
        BRD_VM_PLUS,
        BRD_VM_MINUS,
        BRD_VM_MUL,
        BRD_VM_DIV,
        BRD_VM_NEGATE,

        /* comp */
        BRD_VM_LT,
        BRD_VM_LEQ,
        BRD_VM_GT,
        BRD_VM_GEQ,
        BRD_VM_EQ,

        /* boolean */
        BRD_VM_AND,
        BRD_VM_OR,
        BRD_VM_NOT,

        BRD_VM_SET_VAR, /* has arg: string */

        /* this will do more when we have functions and classes */
        BRD_VM_RETURN,
};

struct brd_stack {
        struct brd_value values[STACK_SIZE];
        struct brd_value *sp;
};

struct brd_vm {
        struct brd_value_map globals;
        struct brd_stack stack;
        void *bytecode;
        size_t pc;
};

void brd_stack_push(struct brd_stack *stack, struct brd_value *value);
struct brd_value *brd_stack_pop(struct brd_stack *stack);

void *brd_node_compile(struct brd_node *node);
#endif
