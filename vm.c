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
        printf(" ======== ");
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
        printf(" -------- ");
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
        case BRD_VM_POP: printf("BRD_VM_POP\n"); break;
        case BRD_VM_CONCAT: printf("BRD_VM_CONCAT\n"); break;
        case BRD_VM_BUILTIN: printf("BRD_VM_BUILTIN\n"); break;
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
brd_value_coerce_string(struct brd_value *value)
{
        /* return true if string is malloced */
        int malloced = false;
        char *string = "";
        switch (value->vtype) {
        case BRD_VAL_NUM:
                string = malloc(sizeof(char) * 75); /* that's enough, right? */
                sprintf(string, "%Lg", value->as.num);
                malloced = true;
                break;
        case BRD_VAL_STRING:
                string = value->as.string;
                break;
        case BRD_VAL_BOOL:
                string = value->as.boolean ? "true" : "false";
                break;
        case BRD_VAL_UNIT:
                string = "unit";
                break;
        }
        value->vtype = BRD_VAL_STRING;
        value->as.string = string;
        return malloced;
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

void
brd_vm_allocate(struct brd_vm *vm, char *string)
{
        struct brd_heap *new = malloc(sizeof(*new));
        new->next = vm->heap;
        new->string = string;
        vm->heap = new;
}

#define ADD_OP(x) do {\
        if(sizeof(enum brd_bytecode) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(enum brd_bytecode *)(*bytecode + *length) = (x);\
        *length += sizeof(enum brd_bytecode);\
} while (0)

#define ADD_SIZET(x) do {\
        if(sizeof(size_t) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(size_t *)(*bytecode + *length) = (x);\
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

#define RECURSE_ON(x) (_brd_node_compile((x), bytecode, length, capacity))

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
                RECURSE_ON(AS(assign, node)->r);
                brd_node_compile_lvalue(AS(assign, node)->l, bytecode, length, capacity);
                break;
        case BRD_NODE_BINOP:
                /* yeah this code is ugly */
                switch (AS(binop, node)->btype) {
                case BRD_OR:
                case BRD_AND: 
                        RECURSE_ON(AS(binop, node)->l);
                        op = (AS(binop, node)->btype == BRD_AND)
                                ? BRD_VM_TEST : BRD_VM_TESTN;
                        ADD_OP(op);
                        ADD_OP(BRD_VM_JMP);
                        temp = *length;
                        ADD_SIZET(0);
                        RECURSE_ON(AS(binop, node)->r);
                        jmp = *length - temp;
                        *(size_t *)(*bytecode + temp) = jmp;
                        break;
                case BRD_PLUS: op = BRD_VM_PLUS; goto mkbinop;
                case BRD_MINUS: op = BRD_VM_MINUS; goto mkbinop;
                case BRD_MUL: op = BRD_VM_MUL; goto mkbinop;
                case BRD_DIV: op = BRD_VM_DIV; goto mkbinop;
                case BRD_CONCAT: op = BRD_VM_CONCAT; goto mkbinop;
                case BRD_LT: op = BRD_VM_LT; goto mkbinop;
                case BRD_LEQ: op = BRD_VM_LEQ; goto mkbinop;
                case BRD_GT: op = BRD_VM_GT; goto mkbinop;
                case BRD_GEQ: op = BRD_VM_GEQ; goto mkbinop;
                case BRD_EQ: op = BRD_VM_EQ; goto mkbinop;
mkbinop:
                        RECURSE_ON(AS(binop, node)->l);
                        RECURSE_ON(AS(binop, node)->r);
                        ADD_OP(op);
                }
                break;
        case BRD_NODE_UNARY:
                RECURSE_ON(AS(unary, node)->u);
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
        case BRD_NODE_BUILTIN:
                for (int i = 0; i < AS(builtin, node)->num_args; i++) {
                        RECURSE_ON(AS(builtin, node)->args[i]);
                }
                ADD_OP(BRD_VM_BUILTIN);
                ADD_STR(AS(builtin, node)->builtin);
                ADD_SIZET(AS(builtin, node)->num_args);
                break;
        case BRD_NODE_PROGRAM:
                for (int i = 0; i < AS(program, node)->num_stmts; i++) {
                        RECURSE_ON(AS(program, node)->stmts[i]);
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
#undef RECURSE_ON
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
        struct brd_heap *heap = vm->heap;
        while (heap != NULL) {
                struct brd_heap *n = heap->next;
                free(heap->string);
                free(heap);
                heap = n;
        }
        brd_value_map_destroy(&vm->globals);
        free(vm->bytecode);
}

void
brd_vm_init(struct brd_vm *vm, void *bytecode)
{
        vm->heap = NULL;
        brd_value_map_init(&vm->globals);
        vm->bytecode = bytecode;
        vm->pc = 0;
        vm->stack.sp = vm->stack.values;
}

static int
brd_run_builtin(char *builtin, struct brd_value *args, size_t nargs, struct brd_value *ret)
{
        if (strcmp(builtin, "writeln") == 0 || strcmp(builtin, "write") == 0) {
                ret->vtype = BRD_VAL_UNIT;
                for (int i = 0; i < nargs; i++) {
                        int malloced = brd_value_coerce_string(&args[i]);
                        printf("%s", args[i].as.string);
                        if (malloced) {
                                free(args[i].as.string);
                        }
                }
                if (builtin[5] == 'l') {
                        printf("\n");
                }
                return false;
        } else if (strcmp(builtin, "readln") == 0) {
                char *input = NULL;
                size_t n = 0;
                ret->vtype = BRD_VAL_STRING;
                if (nargs > 0) {
                        BARF("readln accepts no arguments");
                }
                n = getline(&input, &n, stdin);
                input[n - 1] = '\0';
                ret->as.string = input;
                return true;
        } else if (strcmp(builtin, "length") == 0) {
                ret->vtype = BRD_VAL_NUM;
                if (nargs != 1) {
                        BARF("length accepts exactly 1 argument");
                } else if (args[0].vtype == BRD_VAL_STRING) {
                        ret->as.num = strlen(args[0].as.string);
                } else {
                        ret->as.num = 0;
                }
                return false;
        } else if (strcmp(builtin, "typeof") == 0) {
                ret->vtype = BRD_VAL_STRING;
                if (nargs != 1) {
                        BARF("typeof accepts exactly 1 argument");
                }
                switch (args[0].vtype) {
                case BRD_VAL_NUM:
                        ret->as.string = "number";
                        break;
                case BRD_VAL_STRING:
                        ret->as.string = "string";
                        break;
                case BRD_VAL_BOOL:
                        ret->as.string = "boolean";
                        break;
                case BRD_VAL_UNIT:
                        ret->as.string = "unit";
                        break;
                }
                return false;
        } else {
                BARF("Unknown builtin: %s", builtin);
                return false;
        }
}

void
brd_vm_run(struct brd_vm *vm)
{
        enum brd_bytecode op;
        struct brd_value value1, value2;
        char *id;
        int go = true;
        size_t jmp, nargs;

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
                case BRD_VM_CONCAT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        if (brd_value_coerce_string(&value1)) {
                                brd_vm_allocate(vm, value1.as.string);
                        }
                        if (brd_value_coerce_string(&value2)) {
                                brd_vm_allocate(vm, value2.as.string);
                        }

                        {
                                size_t len1, len2;
                                char *string;
                                len1 = strlen(value1.as.string);
                                len2 = strlen(value2.as.string);
                                string = malloc(len1 + len2 + 2);
                                brd_vm_allocate(vm, string);
                                strcpy(string, value2.as.string);
                                strcat(string, value1.as.string);
                                value2.as.string = string;
                                value2.vtype = BRD_VAL_STRING;
                                brd_stack_push(&vm->stack, &value2);
                        }
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
                                vm->pc += sizeof(enum brd_bytecode) + sizeof(size_t);
                        }
                        break;
                case BRD_VM_TESTN:
                        if (!brd_value_truthify(brd_stack_peek(&vm->stack))) {
                                brd_stack_pop(&vm->stack);
                                vm->pc += sizeof(enum brd_bytecode) + sizeof(size_t);
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
                case BRD_VM_BUILTIN:
                        id = (char *)(vm->bytecode + vm->pc);
                        while (*(char *)(vm->bytecode + vm->pc++));
                        nargs = *(size_t *)(vm->bytecode + vm->pc);
                        vm->pc += sizeof(size_t);
                        for (int i = 0; i < nargs; i++) {
                                brd_stack_pop(&vm->stack);
                        }
                        if (brd_run_builtin(id, vm->stack.values, nargs, &value1)) {
                                brd_vm_allocate(vm, value1.as.string);
                        }
                        brd_stack_push(&vm->stack, &value1);
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
