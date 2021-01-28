#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"

#include <math.h>

#define LIST_SIZE 32
#define GROW 1.5

#ifdef DEBUG
static void
brd_bytecode_debug(enum brd_bytecode op)
{
        printf(" -------- ");
        switch (op) {
        case BRD_VM_NUM: printf("BRD_VM_NUM\n"); return;
        case BRD_VM_STR: printf("BRD_VM_STR\n"); return;
        case BRD_VM_GET_VAR: printf("BRD_VM_GET_VAR\n"); return;
        case BRD_VM_TRUE: printf("BRD_VM_TRUE\n"); return;
        case BRD_VM_FALSE: printf("BRD_VM_FALSE\n"); return;
        case BRD_VM_UNIT: printf("BRD_VM_UNIT\n"); return;
        case BRD_VM_PLUS: printf("BRD_VM_PLUS\n"); return;
        case BRD_VM_MINUS: printf("BRD_VM_MINUS\n"); return;
        case BRD_VM_MUL: printf("BRD_VM_MUL\n"); return;
        case BRD_VM_DIV: printf("BRD_VM_DIV\n"); return;
        case BRD_VM_IDIV: printf("BRD_VM_IDIV\n"); return;
        case BRD_VM_MOD: printf("BRD_VM_MOD\n"); return;
        case BRD_VM_POW: printf("BRD_VM_POW\n"); return;
        case BRD_VM_NEGATE: printf("BRD_VM_NEGATE\n"); return;
        case BRD_VM_LT: printf("BRD_VM_LT\n"); return;
        case BRD_VM_LEQ: printf("BRD_VM_LEQ\n"); return;
        case BRD_VM_GT: printf("BRD_VM_GT\n"); return;
        case BRD_VM_GEQ: printf("BRD_VM_GEQ\n"); return;
        case BRD_VM_EQ: printf("BRD_VM_EQ\n"); return;
        case BRD_VM_NOT: printf("BRD_VM_NOT\n"); return;
        case BRD_VM_TEST: printf("BRD_VM_TEST\n"); return;
        case BRD_VM_TESTN: printf("BRD_VM_TESTN\n"); return;
        case BRD_VM_TESTP: printf("BRD_VM_TESTP\n"); return;
        case BRD_VM_SET_VAR: printf("BRD_VM_SET_VAR\n"); return;
        case BRD_VM_JMP: printf("BRD_VM_JMP\n"); return;
        case BRD_VM_JMPB: printf("BRD_VM_JMPB\n"); return;
        case BRD_VM_RETURN: printf("BRD_VM_RETURN\n"); return;
        case BRD_VM_POP: printf("BRD_VM_POP\n"); return;
        case BRD_VM_CONCAT: printf("BRD_VM_CONCAT\n"); return;
        case BRD_VM_CALL: printf("BRD_VM_CALL\n"); return;
        case BRD_VM_CLOSURE: printf("BRD_VM_CLOSURE\n"); return;
        case BRD_VM_BUILTIN: printf("BRD_VM_BUILTIN\n"); return;
        case BRD_VM_LIST: printf("BRD_VM_LIST\n"); return;
        case BRD_VM_PUSH: printf("BRD_VM_PUSH\n"); return;
        case BRD_VM_GET_IDX: printf("BRD_VM_GET_IDX\n"); return;
        case BRD_VM_SET_IDX: printf("BRD_VM_SET_IDX\n"); return;
        }
        printf("oops\n");
}
#endif

void
brd_stack_push(struct brd_stack *stack, struct brd_value *value)
{
        if (stack->sp - stack->values >= STACK_SIZE) {
                BARF("stack overflow error");
        }
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
brd_vm_allocate(struct brd_vm *vm, struct brd_heap_entry *entry)
{
        entry->next = vm->heap->next;
        vm->heap->next = entry;
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

static void _brd_node_compile(
        struct brd_node *node,
        void **bytecode,
        size_t *length,
        size_t *capacity
);

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
        case BRD_NODE_INDEX:
                RECURSE_ON(AS(index, node)->list);
                RECURSE_ON(AS(index, node)->idx);
                ADD_OP(BRD_VM_SET_IDX);
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
        size_t temp, temp2, jmp, *ifexpr_temps;

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
                case BRD_IDIV: op = BRD_VM_IDIV; goto mkbinop;
                case BRD_MOD: op = BRD_VM_MOD; goto mkbinop;
                case BRD_POW: op = BRD_VM_POW; goto mkbinop;
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
        case BRD_NODE_LIST_LIT:
                ADD_OP(BRD_VM_LIST);
                for (int i = 0; i < AS(list_lit, node)->items->num_args; i++) {
                        RECURSE_ON(AS(list_lit, node)->items->args[i]);
                        ADD_OP(BRD_VM_PUSH);
                }
                break;
        case BRD_NODE_FUNCALL:
                for (int i = 0; i < AS(funcall, node)->args->num_args; i++) {
                        RECURSE_ON(AS(funcall, node)->args->args[i]);
                }
                RECURSE_ON(AS(funcall, node)->fn);
                ADD_OP(BRD_VM_CALL);
                ADD_SIZET(AS(funcall, node)->args->num_args);
                break;
        case BRD_NODE_CLOSURE:
                ADD_OP(BRD_VM_CLOSURE);
                ADD_SIZET(AS(closure, node)->num_args);
                for (int i = 0; i < AS(closure, node)->num_args; i++) {
                        ADD_STR(AS(closure, node)->args[i]);
                }
                ADD_OP(BRD_VM_JMP);
                temp = *length;
                ADD_SIZET(0);
                RECURSE_ON(AS(closure, node)->body);
                ADD_OP(BRD_VM_RETURN);
                jmp = *length - temp;
                *(size_t *)(*bytecode + temp) = jmp;
                break;
        case BRD_NODE_BUILTIN:
                ADD_OP(BRD_VM_BUILTIN);
                ADD_SIZET(brd_lookup_builtin(AS(builtin, node)->builtin));
                break;
        case BRD_NODE_BODY:
                for (int i = 0; i < AS(body, node)->num_stmts; i++) {
                        RECURSE_ON(AS(body, node)->stmts[i]);
                        if (i < AS(body, node)->num_stmts - 1) {
                                ADD_OP(BRD_VM_POP);
                        }
                }
                break;
        case BRD_NODE_INDEX:
                RECURSE_ON(AS(index, node)->list);
                RECURSE_ON(AS(index, node)->idx);
                ADD_OP(BRD_VM_GET_IDX);
                break;
        case BRD_NODE_IFEXPR:
                ifexpr_temps = malloc(sizeof(size_t)*(1+AS(ifexpr, node)->num_elifs));
                RECURSE_ON(AS(ifexpr, node)->cond);
                ADD_OP(BRD_VM_TESTP);
                ADD_OP(BRD_VM_JMP);
                temp = *length;
                ADD_SIZET(0);
                RECURSE_ON(AS(ifexpr, node)->body);
                ADD_OP(BRD_VM_JMP);
                ifexpr_temps[0] = *length;
                ADD_SIZET(0);
                jmp = *length - temp;
                *(size_t *)(*bytecode + temp) = jmp;

                for (int i = 0; i < AS(ifexpr, node)->num_elifs; i++) {
                        RECURSE_ON(AS(ifexpr, node)->elifs[i].cond);
                        ADD_OP(BRD_VM_TESTP);
                        ADD_OP(BRD_VM_JMP);
                        temp = *length;
                        ADD_SIZET(0);
                        RECURSE_ON(AS(ifexpr, node)->elifs[i].body);
                        ADD_OP(BRD_VM_JMP);
                        ifexpr_temps[i+1] = *length;
                        ADD_SIZET(0);
                        jmp = *length - temp;
                        *(size_t *)(*bytecode + temp) = jmp;
                }

                if (AS(ifexpr, node)->els != NULL) {
                        RECURSE_ON(AS(ifexpr, node)->els);
                } else {
                        ADD_OP(BRD_VM_UNIT);
                }

                for (int i = 0; i < AS(ifexpr, node)->num_elifs + 1; i++) {
                        jmp = *length - ifexpr_temps[i];
                        *(size_t *)(*bytecode + ifexpr_temps[i]) = jmp;
                }

                free(ifexpr_temps);
                break;
        case BRD_NODE_WHILE:
                ADD_OP(BRD_VM_LIST);
                temp = *length;
                RECURSE_ON(AS(while, node)->cond);
                ADD_OP(BRD_VM_TESTP);
                ADD_OP(BRD_VM_JMP);
                temp2 = *length;
                ADD_SIZET(0);
                RECURSE_ON(AS(while, node)->body);
                ADD_OP(BRD_VM_PUSH);
                ADD_OP(BRD_VM_JMPB);
                jmp = *length - temp;
                ADD_SIZET(jmp);
                jmp = *length - temp2;
                *(size_t *)(*bytecode + temp2) = jmp;
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
        /* note to self: for container types, don't free the members! */
        struct brd_heap_entry *heap = vm->heap;
        while (heap != NULL) {
                struct brd_heap_entry *n = heap->next;
                switch(heap->htype) {
                case BRD_HEAP_STRING:
                        free(heap->as.string);
                        break;
                case BRD_HEAP_LIST:
                        free(heap->as.list->items);
                        free(heap->as.list);
                        break;
                case BRD_HEAP_CLOSURE:
                        brd_value_closure_destroy(heap->as.closure);
                        free(heap->as.closure);
                        break;
                }
                free(heap);
                heap = n;
        }
        brd_value_map_destroy(&vm->globals);
        free(vm->bytecode);
}

void
brd_vm_init(struct brd_vm *vm, void *bytecode)
{
        vm->heap = malloc(sizeof(*vm->heap));
        vm->heap->next = NULL;
        vm->heap->htype = BRD_HEAP_STRING;
        vm->heap->as.string = malloc(1);
        brd_value_map_init(&vm->globals);
        vm->bytecode = bytecode;
        vm->pc = 0;
        vm->stack.sp = vm->stack.values;
}

static int
brd_value_call(struct brd_value *f, struct brd_value *args, size_t num_args, struct brd_value *out)
{
        if (f->vtype == BRD_VAL_BUILTIN) {
                return builtin_function[f->as.builtin](args, num_args, out);
        } else if (f->vtype == BRD_VAL_HEAP &&
                        f->as.heap->htype == BRD_HEAP_CLOSURE) {
                /* this is suuuuper hacky */
                int on_heap, new;
                struct brd_value_closure *closure = f->as.heap->as.closure;
                struct brd_vm *vm = malloc(sizeof(*vm));
                brd_vm_init(vm, closure->bytecode);
                brd_value_map_copy(&vm->globals, &closure->env);
                brd_value_map_set(&vm->globals, "self", f); /* for recursive funcs */

                if (num_args != closure->num_args) {
                        BARF("wrong number of arguments");
                }

                for (int i = 0; i < num_args; i++) {
                        brd_value_map_set(&vm->globals, closure->args[i], &args[i]);
                }
                brd_vm_run(vm, out);

                /* cleanup
                 * if the return value is on the heap, we need to not free it
                 * and we need to allocate the return value on the callee's vm
                 * only if the value is on the heap (i.e not in the closure's env)
                 */
                on_heap = out->vtype == BRD_VAL_HEAP;
                new = false;
                while (vm->heap != NULL) {
                        struct brd_heap_entry *n = vm->heap->next;
                        if (on_heap && vm->heap == out->as.heap) {
                                new = true;
                                vm->heap = n;
                                continue;
                        }

                        switch(vm->heap->htype) {
                        case BRD_HEAP_STRING:
                                free(vm->heap->as.string);
                                break;
                        case BRD_HEAP_LIST:
                                free(vm->heap->as.list->items);
                                free(vm->heap->as.list);
                                break;
                        case BRD_HEAP_CLOSURE:
                                brd_value_closure_destroy(vm->heap->as.closure);
                                free(vm->heap->as.closure);
                                break;
                        }
                        free(vm->heap);
                        vm->heap = n;
                }
                brd_value_map_destroy(&vm->globals);
                free(vm);

                return new;
        } else {
                BARF("attempted to call a non-callable");
                return -1;
        }
}

void
brd_vm_run(struct brd_vm *vm, struct brd_value *out)
{
        enum brd_bytecode op;
        struct brd_value value1, value2, value3;
        char *id;
        char **args;
        size_t jmp, num_args;

        for (;;) {
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
                case BRD_VM_IDIV:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = floorl(
                                value2.as.num / value1.as.num
                        );
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_MOD:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = (long long int) value2.as.num
                                % (long long int) value1.as.num;
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_POW:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = powl(
                                value2.as.num,
                                value1.as.num
                        );
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_CONCAT:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        brd_value_concat(&value2, &value1);
                        brd_vm_allocate(vm, value2.as.heap);
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
                                vm->pc += sizeof(enum brd_bytecode) + sizeof(size_t);
                        }
                        break;
                case BRD_VM_TESTN:
                        if (!brd_value_truthify(brd_stack_peek(&vm->stack))) {
                                brd_stack_pop(&vm->stack);
                                vm->pc += sizeof(enum brd_bytecode) + sizeof(size_t);
                        }
                        break;
                case BRD_VM_TESTP:
                        if (brd_value_truthify(brd_stack_pop(&vm->stack))) {
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
                case BRD_VM_JMPB:
                        jmp = *(size_t *)(vm->bytecode + vm->pc);
                        vm->pc -= jmp;
                        break;
                case BRD_VM_BUILTIN:
                        value1.vtype = BRD_VAL_BUILTIN;
                        value1.as.builtin = *(size_t *)(vm->bytecode + vm->pc);
                        vm->pc += sizeof(size_t);
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_CALL:
                        num_args = *(size_t *)(vm->bytecode + vm->pc);
                        vm->pc += sizeof(size_t);
                        value1 = *brd_stack_pop(&vm->stack);
                        vm->stack.sp -= num_args;
                        if (brd_value_call(&value1, vm->stack.sp, num_args, &value2)) {
                                brd_vm_allocate(vm, value2.as.heap);
                        }
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_CLOSURE:
                        value1.vtype = BRD_VAL_HEAP;
                        value1.as.heap = malloc(sizeof(struct brd_heap_entry));
                        brd_vm_allocate(vm, value1.as.heap);
                        value1.as.heap->htype = BRD_HEAP_CLOSURE;
                        value1.as.heap->as.closure =
                                malloc(sizeof(struct brd_value_closure));
                        num_args = *(size_t *)(vm->bytecode + vm->pc);
                        vm->pc += sizeof(size_t);
                        args = malloc(sizeof(char *) * num_args);
                        for (int i = 0; i < num_args; i++) {
                                args[i] = (char *)(vm->bytecode + vm->pc);
                                while (*(char *)(vm->bytecode + vm->pc++));
                        }
                        brd_value_closure_init(
                                value1.as.heap->as.closure,
                                args,
                                num_args,
                                vm->bytecode + vm->pc
                                + sizeof(enum brd_bytecode) + sizeof(size_t)
                        );
                        brd_value_map_copy(
                                &value1.as.heap->as.closure->env,
                                &vm->globals
                        );
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_LIST:
                        value1.vtype = BRD_VAL_HEAP;
                        value1.as.heap = malloc(sizeof(*value1.as.heap));
                        value1.as.heap->htype = BRD_HEAP_LIST;
                        value1.as.heap->as.list = malloc(
                                sizeof(struct brd_value_list)
                        );
                        brd_value_list_init(value1.as.heap->as.list);
                        brd_vm_allocate(vm, value1.as.heap);
                        brd_stack_push(&vm->stack, &value1);
                        break;
                case BRD_VM_GET_IDX:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        if (value2.vtype != BRD_VAL_HEAP
                                        || value2.as.heap->htype != BRD_HEAP_LIST) {
                                BARF("attempted to index a non-list");
                        } else if (value1.vtype != BRD_VAL_NUM) {
                                BARF("attempted to index with a non-number");
                        }
                        value2 = *brd_value_list_get(
                                value2.as.heap->as.list,
                                (size_t) floorl(value1.as.num)
                        );
                        brd_stack_push(&vm->stack, &value2);
                        break;
                case BRD_VM_SET_IDX:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_pop(&vm->stack);
                        if (value2.vtype != BRD_VAL_HEAP
                                        || value2.as.heap->htype != BRD_HEAP_LIST) {
                                BARF("attempted to index a non-list");
                        } else if (value1.vtype != BRD_VAL_NUM) {
                                BARF("attempted to index with a non-number");
                        }
                        value3 = *brd_stack_pop(&vm->stack);
                        brd_value_list_set(
                                value2.as.heap->as.list,
                                (size_t) floorl(value1.as.num),
                                &value3
                        );
                        brd_stack_push(&vm->stack, &value3);
                        break;
                case BRD_VM_PUSH:
                        value1 = *brd_stack_pop(&vm->stack);
                        value2 = *brd_stack_peek(&vm->stack);
                        brd_value_list_push(
                                value2.as.heap->as.list,
                                &value1
                        );
                        break;
                case BRD_VM_RETURN:
                        goto exit_loop;
                case BRD_VM_POP:
#ifdef DEBUG
                        value1 = *brd_stack_pop(&vm->stack);
                        brd_value_debug(&value1);
#else
                        brd_stack_pop(&vm->stack);
#endif
                        brd_vm_gc(vm);
                        break;
                }
        }

exit_loop:
        if (out != NULL) {
                *out = *brd_stack_pop(&vm->stack);
        }
        return;
}

void
brd_vm_gc(struct brd_vm *vm)
{
        struct brd_heap_entry *heap, *prev;

        heap = vm->heap->next;
        while (heap != NULL) {
                heap->marked = false;
                heap = heap->next;
        }

        /* mark values in the stack */
        for (struct brd_value *p = vm->stack.values; p < vm->stack.sp; p++) {
                if (p->vtype == BRD_VAL_HEAP) {
                        p->as.heap->marked = true;
                }
        }

        /* mark values held by variables */
        brd_value_map_mark(&vm->globals);

        prev = vm->heap;
        heap = prev->next;
        while (heap != NULL) {
                struct brd_heap_entry *next = heap->next;
                if (heap->marked) {
                        prev = heap;
                        heap = next;
                        continue;
                }

                prev->next = next;
                switch(heap->htype) {
                case BRD_HEAP_STRING:
                        free(heap->as.string);
                        break;
                case BRD_HEAP_LIST:
                        free(heap->as.list->items);
                        free(heap->as.list);
                        break;
                case BRD_HEAP_CLOSURE:
                        brd_value_closure_destroy(heap->as.closure);
                        free(heap->as.closure);
                        break;
                }
                free(heap);
                heap = next;
        }
}
