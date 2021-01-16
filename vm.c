#include "common.h"
#include "ast.h"
#include "vm.h"

/* http://www.cse.yorku.ca/~oz/hash.html djb2 hash algorithm */
static unsigned long
hash(char *str)
{
        unsigned long hash = 5381;
        int c;

        while ((c = *str++))
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
}

void
brd_value_map_init(struct brd_value_map *map)
{
        for (int i = 0; i < BUCKET_SIZE; i++) {
                map->bucket[i].key = "";
                map->bucket[i].next = NULL;
        }
}

void
brd_value_map_destroy(struct brd_value_map *map)
{
        /* At this point in time I'm not going to free the strings */

        for (int i = 0; i < BUCKET_SIZE; i++) {
                struct brd_value_map_list *list = map->bucket[i].next;
                
                while (list != NULL) {
                        struct brd_value_map_list *next = list->next;
                        free(list);
                        list = next;
                }
        }
}

void
brd_value_map_set(struct brd_value_map *map, char *key, struct brd_value *val)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct brd_value_map_list *list = &map->bucket[h];

        /*
         * Always at least 1 iteration since each bucket entry
         * has an initial dummy value
         */
        for (;;) {
                if (strcmp(list->key, key) == 0) {
                        list->val = *val;
                        return;
                } else if (list->next == NULL) {
                        break;
                } else {
                        list = list->next;
                }
        }

        /* key not in list */
        list->next = malloc(sizeof(struct brd_value_map_list));
        list->next->key = key;
        list->next->val = *val;
        list->next->next = NULL;
}

struct brd_value *
brd_value_map_get(struct brd_value_map *map, char *key)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct brd_value_map_list *list = &map->bucket[h];

        do {
                if (strcmp(list->key, key) == 0) {
                        return &list->val;
                }
        } while ((list = list->next) != NULL);

        return NULL;
}

void
brd_value_debug(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                printf("%Lf\n", value->as.num);
                break;
        case BRD_VAL_STRING:
                printf("%s\n", value->as.string);
                break;
        case BRD_VAL_BOOL:
                printf("%s\n", value->as.boolean ? "true" : "false");
                break;
        case BRD_VAL_UNIT:
                printf("unit\n");
                break;
        }
}

void
brd_stack_push(struct brd_stack *stack, struct brd_value *value)
{
        *(stack->sp++) = *value;
}

struct brd_value *
brd_stack_pop(struct brd_stack *stack)
{
        return --stack->sp;
}

#define ADD_OP(x) do {\
        if(sizeof(enum brd_bytecode) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(enum brd_bytecode *)(*bytecode + *length) = (x);\
        *length += sizeof(enum brd_bytecode);\
} while (0)

#define ADD_NUM(x) do {\
        if(sizeof(long double) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(long double *)(*bytecode + *length) = (x);\
        *length += sizeof(long double);\
} while (0)

#define ADD_STR(x) do {\
        size_t len = strlen(x) + 1;\
        while (len + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        strcpy((*bytecode) + *length, (x));\
        *length += len;\
} while (0)

#define AS(type, x) ((struct brd_node_ ## type *)(x))

static void
brd_node_compile_lvalue(
        struct brd_node *node,
        void **bytecode,
        size_t *length,
        size_t *capacity)
{
        switch (node->ntype) {
        case BRD_NODE_VAR:
                ADD_OP(BRD_VM_SET_VAR);
                ADD_STR(AS(var, node)->id);
                break;
        default:
                BARF("This shouldn't happen.");
        }
}

static void
_brd_node_compile(
        struct brd_node *node,
        void **bytecode,
        size_t *length,
        size_t *capacity)
{
        switch (node->ntype) {
        case BRD_NODE_ASSIGN: 
                _brd_node_compile(AS(assign, node)->r, bytecode, length, capacity);
                brd_node_compile_lvalue(AS(assign, node)->l, bytecode, length, capacity);
                break;
        case BRD_NODE_BINOP:
                _brd_node_compile(AS(binop, node)->l, bytecode, length, capacity);
                _brd_node_compile(AS(binop, node)->r, bytecode, length, capacity);

                switch (AS(binop, node)->btype) {
                case BRD_PLUS: ADD_OP(BRD_VM_PLUS); break;
                case BRD_MINUS: ADD_OP(BRD_VM_MINUS); break;
                case BRD_MUL: ADD_OP(BRD_VM_MUL); break;
                case BRD_DIV: ADD_OP(BRD_VM_DIV); break;
                case BRD_LT: ADD_OP(BRD_VM_LT); break;
                case BRD_LEQ: ADD_OP(BRD_VM_LEQ); break;
                case BRD_GT: ADD_OP(BRD_VM_GT); break;
                case BRD_GEQ: ADD_OP(BRD_VM_GEQ); break;
                case BRD_EQ: ADD_OP(BRD_VM_EQ); break;
                case BRD_AND: ADD_OP(BRD_VM_AND); break;
                case BRD_OR: ADD_OP(BRD_VM_OR); break;
                }
                break;
        case BRD_NODE_UNARY:
                _brd_node_compile(AS(unary, node)->u, bytecode, length, capacity);
                switch (AS(unary, node)->utype) {
                case BRD_NEGATE: ADD_OP(BRD_VM_NEGATE); break;
                case BRD_NOT: ADD_OP(BRD_VM_NOT); break;
                }
                break;
        case BRD_NODE_VAR:
                ADD_OP(BRD_VM_GET_VAR);
                ADD_STR(AS(var, node)->id);
                break;
        case BRD_NODE_NUM_LIT:
                ADD_OP(BRD_VM_NUM);
                ADD_NUM(AS(num_lit, node)->v);
                break;
        case BRD_NODE_STRING_LIT:
                ADD_OP(BRD_VM_STR);
                ADD_STR(AS(string_lit, node)->s);
                break;
        case BRD_NODE_BOOL_LIT:
                ADD_OP(AS(bool_lit, node)->b ? BRD_VM_TRUE : BRD_VM_FALSE);
                break;
        case BRD_NODE_UNIT_LIT:
                ADD_OP(BRD_VM_UNIT);
                break;
        case BRD_NODE_PROGRAM:
                for (int i = 0; i < AS(program, node)->num_stmts; i++) {
                        _brd_node_compile(
                                AS(program, node)->stmts[i],
                                bytecode,
                                length,
                                capacity
                        );
                        ADD_OP(BRD_VM_CLEAR);
                }
                ADD_OP(BRD_VM_RETURN);
                break;
        case BRD_NODE_MAX:
                BARF("This shouldn't happen.");
        }
}

#undef ADD_OP
#undef ADD_NUM
#undef ADD_STR
#undef AS

void *
brd_node_compile(struct brd_node *node)
{
        size_t length = 0, capacity = LIST_SIZE;
        void *bytecode = malloc(capacity);
        (void)brd_node_compile_lvalue;

        _brd_node_compile(node, &bytecode, &length, &capacity);
        return bytecode;
}

void
brd_vm_destroy(struct brd_vm *vm)
{
        brd_value_map_destroy(&vm->globals);
        free(vm->bytecode);
}

void
brd_vm_init(struct brd_vm *vm, void *bytecode)
{
        brd_value_map_init(&vm->globals);
        vm->bytecode = bytecode;
        vm->pc = 0;
        vm->stack.sp = vm->stack.values;
}

void
brd_vm_run(struct brd_vm *vm)
{
        enum brd_bytecode op;
        struct brd_value value1, value2;
        char *id;
        int go = true;

        while (go) {
                op = *(enum brd_bytecode *)(vm->bytecode + vm->pc);
                vm->pc += sizeof(enum brd_bytecode);

                switch (op) {
                case BRD_VM_NUM:
                        value1.vtype = BRD_VAL_NUM;
                        value1.as.num = *(long double *)(vm->bytecode + vm->pc);
                        vm->pc += sizeof(long double);
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_STR:
                        value1.vtype = BRD_VAL_STRING;
                        value1.as.string = (char *)(vm->bytecode + vm->pc);
                        while (*(char *)(vm->bytecode + vm->pc++));
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_GET_VAR:
                        id = (char *)(vm->bytecode + vm->pc);
                        while (*(char *)(vm->bytecode + vm->pc++));
                        value1 = *brd_value_map_get(&vm->globals, id);
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_TRUE:
                        value1.vtype = BRD_VAL_BOOL;
                        value1.as.boolean = true;
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_FALSE:
                        value1.vtype = BRD_VAL_BOOL;
                        value1.as.boolean = false;
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_UNIT:
                        value1.vtype = BRD_VAL_UNIT;
                        brd_stack_push(&vm->stack, &value1);
                        break;
                        /* TODO: error checking/coersion on math ops */
                case BRD_VM_PLUS:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.num += value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_MINUS:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.num -= value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_MUL:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.num *= value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_DIV:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.num /= value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_LT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = value2.as.num < value1.as.num;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_LEQ:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = value2.as.num <= value1.as.num;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_GT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = value2.as.num > value1.as.num;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_GEQ:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = value2.as.num >= value1.as.num;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_EQ:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = value2.as.num == value1.as.num;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                        /* TODO boolean short circuiting */
                        /* I'll need a JMP op code */
                case BRD_VM_AND:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean &= value1.as.boolean;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_OR:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean |= value1.as.boolean;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_NEGATE:
                        value1 = *brd_stack_pop(&vm->stack);
                        value1.as.num = -value1.as.num;
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_NOT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value1.as.boolean = !value1.as.boolean;
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_SET_VAR:
                        id = (char *)(vm->bytecode + vm->pc);
                        while (*(char *)(vm->bytecode + vm->pc++));
                        value1 = *brd_stack_pop(&vm->stack);
                        brd_value_map_set(&vm->globals, id, &value1);
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_RETURN:
                        go = false;
                        break;
                case BRD_VM_CLEAR:
#ifdef DEBUG
                        value1 = *brd_stack_pop(&vm->stack);
                        brd_value_debug(&value1);
#endif
                        vm->stack.sp = vm->stack.values;
                        break;
                }
        }
}
