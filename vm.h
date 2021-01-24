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
        BRD_VM_IDIV,
        BRD_VM_MOD,
        BRD_VM_POW,
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
        BRD_VM_BUILTIN, /* has arg: size_t */
        BRD_VM_CALL,
        BRD_VM_JMP, /* has arg: size_t */

        /* this will do more when we have functions and classes */
        BRD_VM_RETURN,
        BRD_VM_POP,

        BRD_VM_GET_IDX,
        BRD_VM_SET_IDX,

        BRD_VM_LIST, /* initializes an empty list */
        /* this is poorly named, it's a list operation */
        BRD_VM_PUSH, /* x = pop(), peek().push(x) */
};

struct brd_stack {
        struct brd_value values[STACK_SIZE];
        struct brd_value *sp;
};

extern struct brd_vm {
        struct brd_stack stack;
        struct brd_heap_entry *heap;
        void *bytecode;
        size_t pc;
        struct brd_value_map globals;
} vm;

void brd_stack_push(struct brd_stack *stack, struct brd_value *value);
struct brd_value *brd_stack_pop(struct brd_stack *stack);
struct brd_value *brd_stack_peek(struct brd_stack *stack);

void *brd_node_compile(struct brd_node *node);

void brd_vm_destroy();
void brd_vm_init(void *bytecode);
void brd_vm_allocate(struct brd_heap_entry *entry);
void brd_vm_run();

void brd_vm_gc();
#endif
