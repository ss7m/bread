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

void brd_value_coerce_num(struct brd_value *value);
int brd_value_coerce_string(struct brd_value *value);
int brd_value_truthify(struct brd_value *value);
int brd_value_compare(struct brd_value *a, struct brd_value *b);

struct brd_value_map_list {
        char *key;
        struct brd_value_map_list *next;
        struct brd_value val;
};

struct brd_value_map {
        struct brd_value_map_list bucket[BUCKET_SIZE];
};

void brd_value_map_init(struct brd_value_map *map);
void brd_value_map_destroy(struct brd_value_map *map);
void brd_value_map_set(struct brd_value_map *map, char *key, struct brd_value *val);
struct brd_value *brd_value_map_get(struct brd_value_map *map, char *key);

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

        BRD_VM_CONCAT,

        /* boolean */
        BRD_VM_NOT,
        BRD_VM_TEST,  /* if peek()     then pop(); pc++ */
        BRD_VM_TESTN, /* if not peek() then pop(); pc++*/
        BRD_VM_TESTP, /* if pop()      then pc++ */
        /* these three instructions are ALWAYS to be followed by a JMP instruction */

        BRD_VM_SET_VAR, /* has arg: string */
        BRD_VM_BUILTIN, /* has args: string, size_t */
        BRD_VM_JMP, /* has arg: size_t */

        /* this will do more when we have functions and classes */
        BRD_VM_RETURN,
        BRD_VM_POP,
};

struct brd_stack {
        struct brd_value values[STACK_SIZE];
        struct brd_value *sp;
};

struct brd_heap {
        struct brd_heap *next;
        char *string;
};

struct brd_vm {
        struct brd_stack stack;
        struct brd_heap *heap;
        void *bytecode;
        size_t pc;
        struct brd_value_map globals;
};

void brd_stack_push(struct brd_stack *stack, struct brd_value *value);
struct brd_value *brd_stack_pop(struct brd_stack *stack);
struct brd_value *brd_stack_peek(struct brd_stack *stack);

void brd_vm_allocate(struct brd_vm *vm, char *string);

void *brd_node_compile(struct brd_node *node);

void brd_vm_destroy(struct brd_vm *vm);
void brd_vm_init(struct brd_vm *vm, void *bytecode);
void brd_vm_allocate(struct brd_vm *vm, char *s);
void brd_vm_run(struct brd_vm *vm);
#endif
