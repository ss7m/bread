#include "common.h"
#include "ast.h"
#include "vm.h"

#define ABS(x) (((x) > 0) ? (x) : (-x))

#define LIST_SIZE 32
#define GROW 1.5

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

#ifdef DEBUG
static void
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

static void
brd_bytecode_debug(enum brd_bytecode op)
{
        switch (op) {
        case BRD_VM_NUM: printf("BRD_VM_NUM\n"); break;
        case BRD_VM_STR: printf("BRD_VM_STR\n"); break;
        case BRD_VM_GET_VAR: printf("BRD_VM_GET_VAR\n"); break;
        case BRD_VM_TRUE: printf("BRD_VM_TRUE\n"); break;
        case BRD_VM_FALSE: printf("BRD_VM_FALSE\n"); break;
        case BRD_VM_UNIT: printf("BRD_VM_UNIT\n"); break;
        case BRD_VM_PLUS: printf("BRD_VM_PLUS\n"); break;
        case BRD_VM_MINUS: printf("BRD_VM_MINUS\n"); break;
        case BRD_VM_MUL: printf("BRD_VM_MUL\n"); break;
        case BRD_VM_DIV: printf("BRD_VM_DIV\n"); break;
        case BRD_VM_NEGATE: printf("BRD_VM_NEGATE\n"); break;
        case BRD_VM_LT: printf("BRD_VM_LT\n"); break;
        case BRD_VM_LEQ: printf("BRD_VM_LEQ\n"); break;
        case BRD_VM_GT: printf("BRD_VM_GT\n"); break;
        case BRD_VM_GEQ: printf("BRD_VM_GEQ\n"); break;
        case BRD_VM_EQ: printf("BRD_VM_EQ\n"); break;
        case BRD_VM_NOT: printf("BRD_VM_NOT\n"); break;
        case BRD_VM_TEST: printf("BRD_VM_TEST\n"); break;
        case BRD_VM_TESTN: printf("BRD_VM_TESTN\n"); break;
        case BRD_VM_SET_VAR: printf("BRD_VM_SET_VAR\n"); break;
        case BRD_VM_JMP: printf("BRD_VM_JMP\n"); break;
        case BRD_VM_RETURN: printf("BRD_VM_RETURN\n"); break;
        case BRD_VM_POP: printf("BRD_VM_CLEAR\n"); break;
        default: printf("oops\n");
        }
}
#endif

void
brd_value_coerce_num(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                break;
        case BRD_VAL_STRING:
                value->as.num = strtold(value->as.string, NULL);
                break;
        case BRD_VAL_BOOL:
                value->as.num = value->as.boolean ? 1 : 0;
                break;
        case BRD_VAL_UNIT:
                value->as.num = 0;
        }

        value->vtype = BRD_VAL_NUM;
}

int
brd_value_truthify(struct brd_value *value)
{
        switch (value->vtype) {
        case BRD_VAL_NUM:
                return value->as.num != 0;
        case BRD_VAL_STRING:
                return value->as.string[0] != '\0';
        case BRD_VAL_BOOL:
                return value->as.boolean;
        case BRD_VAL_UNIT:
                return false;
        }
        BARF("what?");
        return false;
}

int
brd_value_compare(struct brd_value *a, struct brd_value *b)
{
        switch (a->vtype) {
        case BRD_VAL_NUM:
                brd_value_coerce_num(b);
                return (int) (a->as.num - b->as.num);
        case BRD_VAL_STRING:
                switch (b->vtype) {
                case BRD_VAL_NUM:
                        brd_value_coerce_num(a);
                        return (int) (a->as.num - b->as.num);
                case BRD_VAL_STRING:
                        return strcmp(a->as.string, b->as.string);
                case BRD_VAL_BOOL:
                        /* This is obviously a very good idea */
                        return strcmp(a->as.string, b->as.boolean ? "true" : "false");
                case BRD_VAL_UNIT:
                        return a->as.string[0] != '\0';
                }
                break;
        case BRD_VAL_BOOL:
                switch (b->vtype) {
                case BRD_VAL_NUM:
                        brd_value_coerce_num(a);
                        return (int) (a->as.num - b->as.num);
                case BRD_VAL_STRING:
                        /* Again, this is the obvious thing to do there */
                        return strcmp(a->as.boolean ? "true" : "false", b->as.string);
                case BRD_VAL_BOOL:
                        return a->as.boolean - b->as.boolean;
                case BRD_VAL_UNIT:
                        return a->as.boolean;
                }
                break;
        case BRD_VAL_UNIT:
                switch (b->vtype) {
                case BRD_VAL_NUM:
                        return -ABS(b->as.num);
                case BRD_VAL_STRING:
                        return -(b->as.string[0] != '\0');
                case BRD_VAL_BOOL:
                        return -b->as.boolean;
                case BRD_VAL_UNIT:
                        return 0;
                }
                break;
        }
        BARF("what?");
        return -1;
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
brd_stack_push(struct brd_stack *stack, struct brd_value *value)
{
        *(stack->sp++) = *value;
}

struct brd_value *
brd_stack_pop(struct brd_stack *stack)
{
        return --stack->sp;
}

struct brd_value *
brd_stack_peek(struct brd_stack *stack)
{
        return stack->sp - 1;
}

#define ADD_OP(x) do {\
        if(sizeof(enum brd_bytecode) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(enum brd_bytecode *)(*bytecode + *length) = (x);\
        *length += sizeof(enum brd_bytecode);\
} while (0)

#define MK_OFFSET do {\
        if(sizeof(size_t) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(size_t *)(*bytecode + *length) = 0;\
        *length += sizeof(size_t);\
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
        enum brd_bytecode op;
        size_t temp, jmp;

        switch (node->ntype) {
        case BRD_NODE_ASSIGN: 
                _brd_node_compile(AS(assign, node)->r, bytecode, length, capacity);
                brd_node_compile_lvalue(AS(assign, node)->l, bytecode, length, capacity);
                break;
        case BRD_NODE_BINOP:
                /* yeah this code is ugly */
                switch (AS(binop, node)->btype) {
                case BRD_OR:
                case BRD_AND: 
                        _brd_node_compile(AS(binop, node)->l, bytecode, length, capacity);
                        op = (AS(binop, node)->btype == BRD_AND) ? BRD_VM_TEST : BRD_VM_TESTN;
                        ADD_OP(op);
                        ADD_OP(BRD_VM_JMP);
                        temp = *length;
                        MK_OFFSET;
                        _brd_node_compile(AS(binop, node)->r, bytecode, length, capacity);
                        jmp = *length - temp;
                        *(size_t *)(*bytecode + temp) = jmp;
                        break;
                case BRD_PLUS: op = BRD_VM_PLUS; goto mkbinop;
                case BRD_MINUS: op = BRD_VM_MINUS; goto mkbinop;
                case BRD_MUL: op = BRD_VM_MUL; goto mkbinop;
                case BRD_DIV: op = BRD_VM_DIV; goto mkbinop;
                case BRD_LT: op = BRD_VM_LT; goto mkbinop;
                case BRD_LEQ: op = BRD_VM_LEQ; goto mkbinop;
                case BRD_GT: op = BRD_VM_GT; goto mkbinop;
                case BRD_GEQ: op = BRD_VM_GEQ; goto mkbinop;
                case BRD_EQ: op = BRD_VM_EQ; goto mkbinop;
mkbinop:
                        _brd_node_compile(AS(binop, node)->l, bytecode, length, capacity);
                        _brd_node_compile(AS(binop, node)->r, bytecode, length, capacity);
                        ADD_OP(op);
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
                        ADD_OP(BRD_VM_POP);
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
        ptrdiff_t jmp;

        while (go) {
                op = *(enum brd_bytecode *)(vm->bytecode + vm->pc);
                vm->pc += sizeof(enum brd_bytecode);

#ifdef DEBUG
                brd_bytecode_debug(op);
#endif

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
                        value1.vtype = BRD_VAL_UNIT;
                        value1 = *(brd_value_map_get(&vm->globals, id) ?: &value1);
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
                case BRD_VM_PLUS:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num += value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_MINUS:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num -= value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_MUL:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num *= value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_DIV:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num /= value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_LT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) < 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_LEQ:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) <= 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_GT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) > 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_GEQ:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) >= 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_EQ:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) == 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_NEGATE:
                        value1 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        value1.as.num *= -1;
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_NOT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value1.as.boolean = !brd_value_truthify(&value1);
                        value1.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_TEST:
                        if (brd_value_truthify(brd_stack_peek(&vm->stack))) {
                                brd_stack_pop(&vm->stack);
                                vm->pc += sizeof(enum brd_bytecode) + sizeof(ptrdiff_t);
                        }
                        break;
                case BRD_VM_TESTN:
                        if (!brd_value_truthify(brd_stack_peek(&vm->stack))) {
                                brd_stack_pop(&vm->stack);
                                vm->pc += sizeof(enum brd_bytecode) + sizeof(ptrdiff_t);
                        }
                        break;
                case BRD_VM_SET_VAR:
                        id = (char *)(vm->bytecode + vm->pc);
                        while (*(char *)(vm->bytecode + vm->pc++));
                        value1 = *brd_stack_pop(&vm->stack);
                        brd_value_map_set(&vm->globals, id, &value1);
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_JMP:
                        jmp = *(size_t *)(vm->bytecode + vm->pc);
                        vm->pc += jmp;
                        break;
                case BRD_VM_RETURN:
                        go = false;
                        break;
                case BRD_VM_POP:
#ifdef DEBUG
                        value1 = *brd_stack_pop(&vm->stack);
                        brd_value_debug(&value1);
#else
                        brd_stack_pop(&vm->stack);
#endif
                        break;
                }
        }
}
