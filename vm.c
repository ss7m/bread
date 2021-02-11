#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"

#include <math.h>

#define LIST_SIZE 32
#define GROW 1.5

struct brd_vm vm;

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
        case BRD_VM_GET_FIELD: printf("BRD_VM_GET_FIELD\n"); return;
        case BRD_VM_SET_FIELD: printf("BRD_VM_SET_FIELD\n"); return;
        case BRD_VM_ACC_OBJ: printf("BRD_VM_ACC_OBJ\n"); return;
        case BRD_VM_SUBCLASS: printf("BRD_VM_SUBCLASS\n"); return;
        case BRD_VM_SET_CLASS: printf("BRD_VM_SET_CLASS\n"); return;
        }
        printf("oops\n");
}
#endif

void
brd_stack_push(struct brd_stack *stack, struct brd_value *value)
{
        if (stack->sp - stack->values >= STACK_SIZE) {
                BARF("stack overflow error");
        } else {
                *(stack->sp++) = *value;
        }
}

struct brd_value *
brd_stack_pop(struct brd_stack *stack)
{
#ifdef DEBUG
        if (stack->sp - stack->values == 0) {
                BARF("stack underflow error");
        }
#endif
        return --stack->sp;
}

struct brd_value *
brd_stack_peek(struct brd_stack *stack)
{
        return stack->sp - 1;
}

void
brd_vm_allocate(struct brd_heap_entry *entry)
{
        entry->next = vm.heap->next;
        vm.heap->next = entry;
}

struct brd_string_constant_list *
brd_vm_add_string_constant(char *string)
{
        /* basic string interning
         * this is kinda slow, considering this will be called every time
         * a variable name is compiled
         */
        size_t length;
        struct brd_string_constant_list *entry = vm.strings;
        while (entry != NULL) {
                if (strcmp(string, entry->string.s) == 0) {
                        return entry;
                }
                entry = entry->next;
        }

        /* new string constant */
        length = strlen(string);
        entry = malloc(sizeof(*entry));
        entry->string.s = malloc(length + 1);
        strcpy(entry->string.s, string);
        entry->string.length = length;
        entry->next = vm.strings;
        vm.strings = entry;
        return entry;
}

#define ADD_OP(x) do {\
        if(sizeof(enum brd_bytecode) + vm.bc_length >= vm.bc_capacity) {\
                vm.bc_capacity *= GROW;\
                vm.bytecode = realloc(vm.bytecode, vm.bc_capacity);\
        }\
        *(enum brd_bytecode *)(vm.bytecode + vm.bc_length) = (x);\
        vm.bc_length += sizeof(enum brd_bytecode);\
} while (0)

#define ADD_SIZET(x) do {\
        if(sizeof(size_t) + vm.bc_length >= vm.bc_capacity) {\
                vm.bc_capacity *= GROW;\
                vm.bytecode = realloc(vm.bytecode, vm.bc_capacity);\
        }\
        *(size_t *)(vm.bytecode + vm.bc_length) = (x);\
        vm.bc_length += sizeof(size_t);\
} while (0)

#define ADD_NUM(x) do {\
        if(sizeof(long double) + vm.bc_length >= vm.bc_capacity) {\
                vm.bc_capacity *= GROW;\
                vm.bytecode = realloc(vm.bytecode, vm.bc_capacity);\
        }\
        *(long double *)(vm.bytecode + vm.bc_length) = (x);\
        vm.bc_length += sizeof(long double);\
} while (0)

#define ADD_STR(x) do {\
        struct brd_string_constant_list *entry;\
        if (sizeof(struct brd_string_constant_list *) + vm.bc_length >= vm.bc_capacity) {\
                vm.bc_capacity *= GROW;\
                vm.bytecode = realloc(vm.bytecode, vm.bc_capacity);\
        }\
        entry = brd_vm_add_string_constant(x);\
        *(struct brd_string_constant_list **)(vm.bytecode + vm.bc_length) = entry;\
        vm.bc_length += sizeof(struct brd_string_constant_list *);\
} while (0)

#define AS(type, x) ((struct brd_node_ ## type *)(x))

static void
brd_node_compile_lvalue(struct brd_node *node)
{
        switch (node->ntype) {
        case BRD_NODE_VAR:
                ADD_OP(BRD_VM_SET_VAR);
                ADD_STR(AS(var, node)->id);
                break;
        case BRD_NODE_INDEX:
                brd_node_compile(AS(index, node)->list);
                brd_node_compile(AS(index, node)->idx);
                ADD_OP(BRD_VM_SET_IDX);
                break;
        case BRD_NODE_FIELD:
                brd_node_compile(AS(field, node)->object);
                ADD_OP(BRD_VM_SET_FIELD);
                ADD_STR(AS(field, node)->field);
                break;
        default:
                BARF("This shouldn't happen.");
        }
}

void
brd_node_compile(struct brd_node *node)
{
        enum brd_bytecode op;
        size_t temp, temp2, jmp, *ifexpr_temps;

        switch (node->ntype) {
        case BRD_NODE_ASSIGN: 
                brd_node_compile(AS(assign, node)->r);
                brd_node_compile_lvalue(AS(assign, node)->l);
                break;
        case BRD_NODE_BINOP:
                /* yeah this code is ugly */
                switch (AS(binop, node)->btype) {
                case BRD_OR:
                case BRD_AND: 
                        brd_node_compile(AS(binop, node)->l);
                        op = (AS(binop, node)->btype == BRD_AND)
                                ? BRD_VM_TEST : BRD_VM_TESTN;
                        ADD_OP(op);
                        ADD_OP(BRD_VM_JMP);
                        temp = vm.bc_length;
                        ADD_SIZET(0);
                        brd_node_compile(AS(binop, node)->r);
                        jmp = vm.bc_length - temp;
                        *(size_t *)(vm.bytecode + temp) = jmp;
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
                        brd_node_compile(AS(binop, node)->l);
                        brd_node_compile(AS(binop, node)->r);
                        ADD_OP(op);
                }
                break;
        case BRD_NODE_UNARY:
                brd_node_compile(AS(unary, node)->u);
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
                        brd_node_compile(AS(list_lit, node)->items->args[i]);
                        ADD_OP(BRD_VM_PUSH);
                }
                break;
        case BRD_NODE_FUNCALL:
                for (int i = 0; i < AS(funcall, node)->args->num_args; i++) {
                        brd_node_compile(AS(funcall, node)->args->args[i]);
                }
                brd_node_compile(AS(funcall, node)->fn);
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
                temp = vm.bc_length;
                ADD_SIZET(0);
                brd_node_compile(AS(closure, node)->body);
                ADD_OP(BRD_VM_RETURN);
                jmp = vm.bc_length - temp;
                *(size_t *)(vm.bytecode + temp) = jmp;
                break;
        case BRD_NODE_BUILTIN:
                ADD_OP(BRD_VM_BUILTIN);
                ADD_SIZET(brd_lookup_builtin(AS(builtin, node)->builtin));
                break;
        case BRD_NODE_BODY:
                for (int i = 0; i < AS(body, node)->num_stmts; i++) {
                        brd_node_compile(AS(body, node)->stmts[i]);
                        if (i < AS(body, node)->num_stmts - 1) {
                                ADD_OP(BRD_VM_POP);
                        }
                }
                break;
        case BRD_NODE_INDEX:
                brd_node_compile(AS(index, node)->list);
                brd_node_compile(AS(index, node)->idx);
                ADD_OP(BRD_VM_GET_IDX);
                break;
        case BRD_NODE_IFEXPR:
                ifexpr_temps = malloc(sizeof(size_t)*(1+AS(ifexpr, node)->num_elifs));
                brd_node_compile(AS(ifexpr, node)->cond);
                ADD_OP(BRD_VM_TESTP);
                ADD_OP(BRD_VM_JMP);
                temp = vm.bc_length;
                ADD_SIZET(0);
                brd_node_compile(AS(ifexpr, node)->body);
                ADD_OP(BRD_VM_JMP);
                ifexpr_temps[0] = vm.bc_length;
                ADD_SIZET(0);
                jmp = vm.bc_length - temp;
                *(size_t *)(vm.bytecode + temp) = jmp;

                for (int i = 0; i < AS(ifexpr, node)->num_elifs; i++) {
                        brd_node_compile(AS(ifexpr, node)->elifs[i].cond);
                        ADD_OP(BRD_VM_TESTP);
                        ADD_OP(BRD_VM_JMP);
                        temp = vm.bc_length;
                        ADD_SIZET(0);
                        brd_node_compile(AS(ifexpr, node)->elifs[i].body);
                        ADD_OP(BRD_VM_JMP);
                        ifexpr_temps[i+1] = vm.bc_length;
                        ADD_SIZET(0);
                        jmp = vm.bc_length - temp;
                        *(size_t *)(vm.bytecode + temp) = jmp;
                }

                if (AS(ifexpr, node)->els != NULL) {
                        brd_node_compile(AS(ifexpr, node)->els);
                } else {
                        ADD_OP(BRD_VM_UNIT);
                }

                for (int i = 0; i < AS(ifexpr, node)->num_elifs + 1; i++) {
                        jmp = vm.bc_length - ifexpr_temps[i];
                        *(size_t *)(vm.bytecode + ifexpr_temps[i]) = jmp;
                }

                free(ifexpr_temps);
                break;
        case BRD_NODE_WHILE:
                if (!AS(while, node)->no_list) {
                        ADD_OP(BRD_VM_LIST);
                }
                temp = vm.bc_length;
                brd_node_compile(AS(while, node)->cond);
                ADD_OP(BRD_VM_TESTP);
                ADD_OP(BRD_VM_JMP);
                temp2 = vm.bc_length;
                ADD_SIZET(0);
                brd_node_compile(AS(while, node)->body);
                if (!AS(while, node)->no_list) {
                        ADD_OP(BRD_VM_PUSH);
                } else {
                        ADD_OP(BRD_VM_POP);
                }
                ADD_OP(BRD_VM_JMPB);
                jmp = vm.bc_length - temp;
                ADD_SIZET(jmp);
                jmp = vm.bc_length - temp2;
                *(size_t *)(vm.bytecode + temp2) = jmp;
                if (AS(while, node)->no_list) {
                        ADD_OP(BRD_VM_UNIT);
                }
                break;
        case BRD_NODE_FIELD:
                brd_node_compile(AS(field, node)->object);
                ADD_OP(BRD_VM_GET_FIELD);
                ADD_STR(AS(field, node)->field);
                break;
        case BRD_NODE_ACC_OBJ:
                brd_node_compile(AS(acc_obj, node)->object);
                ADD_OP(BRD_VM_ACC_OBJ);
                ADD_STR(AS(acc_obj, node)->id);
                break;
        case BRD_NODE_SUBCLASS:
                brd_node_compile(AS(subclass, node)->super);
                brd_node_compile(AS(subclass, node)->constructor);
                ADD_OP(BRD_VM_SUBCLASS);
                for (int i = 0; i < AS(subclass, node)->num_decs; i++) {
                        brd_node_compile(AS(subclass, node)->decs[i].expression);
                        ADD_OP(BRD_VM_SET_CLASS);
                        ADD_STR(AS(subclass, node)->decs[i].id);
                }
                break;
        case BRD_NODE_PROGRAM:
                for (int i = 0; i < AS(program, node)->num_stmts; i++) {
                        brd_node_compile(AS(program, node)->stmts[i]);
                        ADD_OP(BRD_VM_POP);
                }
                ADD_OP(BRD_VM_RETURN);
                break;
        }
}

#undef ADD_OP
#undef ADD_NUM
#undef ADD_STR
#undef AS

void
brd_vm_destroy(void)
{
        /* destroy globals */
        brd_heap_destroy(object_class.as.heap);
        free(object_class.as.heap);

        while (vm.heap != NULL) {
                struct brd_heap_entry *n = vm.heap->next;
                brd_heap_destroy(vm.heap);
                free(vm.heap);
                vm.heap = n;
        }

        while (vm.strings != NULL) {
                struct brd_string_constant_list *n = vm.strings->next;
                free(vm.strings->string.s);
                free(vm.strings);
                vm.strings = n;
        }

        brd_value_map_destroy(&vm.frame[0].vars);
        free(vm.bytecode);
}

void
brd_vm_init(void)
{
        /* initialize gloabls */
        object_class.vtype = BRD_VAL_HEAP;
        object_class.as.heap = malloc(sizeof(*object_class.as.heap));
        object_class.as.heap->htype = BRD_HEAP_CLASS;
        object_class.as.heap->as.class = malloc(sizeof(struct brd_value_class));
        brd_value_class_init(object_class.as.heap->as.class);
        object_class.as.heap->as.class->super = &object_class.as.heap->as.class;

        vm.heap = malloc(sizeof(*vm.heap));
        vm.heap->next = NULL;
        vm.heap->htype = BRD_HEAP_STRING;
        vm.heap->as.string = malloc(sizeof(*vm.heap->as.string));
        vm.heap->as.string->s = malloc(1);
        vm.stack.sp = vm.stack.values;

        vm.strings = malloc(sizeof(*vm.strings));
        vm.strings->string.s = malloc(1);
        vm.strings->string.s[0] = '\0';
        vm.strings->next = NULL;

        /* the initial instructions are for the @Object constructor */
        vm.bc_length = 0;
        vm.bc_capacity = LIST_SIZE;
        vm.bytecode = malloc(vm.bc_capacity);

        vm.fp = 0;
        vm.frame[0].pc = 0;
        brd_value_map_init(&vm.frame[0].vars);
}

static void
brd_value_call_closure(struct brd_value_closure *closure, struct brd_value*args, size_t num_args)
{

        if (vm.fp >= FRAME_SIZE - 1) {
                BARF("stack overflow error");
        } else if (num_args != closure->num_args) {
                BARF("wrong number of arguments");
        }

        vm.fp++;
        vm.frame[vm.fp].pc = closure->pc;
        vm.frame[vm.fp].vars = closure->env;

        for (int i = 0; i < num_args; i++) {
                brd_value_map_set(
                        &vm.frame[vm.fp].vars,
                        closure->args[i],
                        &args[i]
                );
        }
}

static void
brd_value_call(struct brd_value *f, struct brd_value *args, size_t num_args)
{
        if (IS_HEAP(*f, BRD_HEAP_CLOSURE)) {
                brd_value_call_closure(f->as.heap->as.closure, args, num_args);
        } else if (IS_HEAP(*f, BRD_HEAP_CLASS)) {
                struct brd_value object;

                object.vtype = BRD_VAL_HEAP;
                object.as.heap = malloc(sizeof(*object.as.heap));
                object.as.heap->htype = BRD_HEAP_OBJECT;
                object.as.heap->as.object = malloc(sizeof(struct brd_value_object));
                brd_value_object_init(
                        object.as.heap->as.object,
                        &f->as.heap->as.class
                );
                brd_vm_allocate(object.as.heap);

                /*
                 * if we're constructing from @Object, then just push
                 * the new object onto the stack
                 */
                if (f->as.heap->as.class == object_class.as.heap->as.class) {
                        brd_stack_push(&vm.stack, &object);
                } else {
                        struct brd_value_class *class = f->as.heap->as.class;
                        brd_value_map_set(&(**class->constructor).env, "this", &object);
                        brd_value_call_closure(*class->constructor, args, num_args);
                }
        } else if (IS_VAL(*f, BRD_VAL_METHOD)) {
                struct brd_value_method method = f->as.method;
                struct brd_value this = brd_heap_value(object, method.this);
                brd_value_map_set(
                        &(**method.fn).env,
                        "this",
                        &this
                );
                brd_value_call_closure(*method.fn, args, num_args);
        } else {
                BARF("attempted to call a non-callable");
        }
}

static void
brd_value_acc_obj(struct brd_value *object, char *id)
{
        struct brd_value value;
        if (!IS_HEAP(*object, BRD_HEAP_OBJECT)) {
                BARF("can only :: on objects");
        } else if (strcmp(id, "super") == 0) {
                // FIXME: I don't like that super always allocates
                value.vtype = BRD_VAL_HEAP;
                value.as.heap = malloc(sizeof(*value.as.heap));
                value.as.heap->htype = BRD_HEAP_OBJECT;
                value.as.heap->as.object = malloc(sizeof(struct brd_value_object));
                brd_vm_allocate(value.as.heap);
                brd_value_object_super(
                        object->as.heap->as.object,
                        value.as.heap->as.object
                );
                brd_stack_push(&vm.stack, &value);
        } else {
                struct brd_value *vp = brd_value_map_get(
                        &(**object->as.heap->as.object->class).methods, id
                );
                if (vp == NULL) {
                        value.vtype = BRD_VAL_UNIT;
                        brd_stack_push(&vm.stack, &value);
                } else if (IS_HEAP(*vp, BRD_HEAP_CLOSURE)) {
                        value.vtype = BRD_VAL_METHOD;
                        value.as.method.this = &object->as.heap->as.object;
                        value.as.method.fn = &vp->as.heap->as.closure;
                        brd_stack_push(&vm.stack, &value);
                } else {
                        brd_stack_push(&vm.stack, vp);
                }
        }
}

void
brd_vm_run(void)
{
        enum brd_bytecode op;
        enum brd_builtin b;
        struct brd_value value1, value2, value3, *valuep;
        char *id;
        char **args;
        size_t jmp, num_args;

#define READ_STRING_INTO(v) do {\
        v = &(*(struct brd_string_constant_list **)(vm.bytecode + vm.frame[vm.fp].pc))\
                ->string;\
        vm.frame[vm.fp].pc += sizeof(struct brd_string_constant_list *);\
}while(0)

        for (;;) {
                op = *(enum brd_bytecode *)(vm.bytecode + vm.frame[vm.fp].pc);
                vm.frame[vm.fp].pc += sizeof(enum brd_bytecode);

#ifdef DEBUG
                brd_bytecode_debug(op);
#endif

                switch (op) {
                case BRD_VM_NUM:
                        value1.vtype = BRD_VAL_NUM;
                        value1.as.num = *(long double *)(vm.bytecode + vm.frame[vm.fp].pc);
                        vm.frame[vm.fp].pc += sizeof(long double);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_STR:
                        value1.vtype = BRD_VAL_STRING;
                        READ_STRING_INTO(value1.as.string);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_GET_VAR:
                        READ_STRING_INTO(value1.as.string);
                        id = value1.as.string->s;
                        valuep = brd_value_map_get(&vm.frame[vm.fp].vars, id);
                        if (valuep == NULL) {
                                value1.vtype = BRD_VAL_UNIT;
                                brd_stack_push(&vm.stack, &value1);
                        } else {
                                brd_stack_push(&vm.stack, valuep);
                        }
                        break;
                case BRD_VM_TRUE:
                        value1.vtype = BRD_VAL_BOOL;
                        value1.as.boolean = true;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_FALSE:
                        value1.vtype = BRD_VAL_BOOL;
                        value1.as.boolean = false;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_UNIT:
                        value1.vtype = BRD_VAL_UNIT;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_PLUS:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num += value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_MINUS:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num -= value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_MUL:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num *= value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_DIV:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num /= value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_IDIV:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = floorl(
                                value2.as.num / value1.as.num
                        );
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_MOD:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = (long long int) value2.as.num
                                % (long long int) value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_POW:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = powl(
                                value2.as.num,
                                value1.as.num
                        );
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_CONCAT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_concat(&value2, &value1);
                        brd_vm_allocate(value2.as.heap);
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_LT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) < 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_LEQ:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) <= 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_GT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) > 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_GEQ:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) >= 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_EQ:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) == 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_NEGATE:
                        value1 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        value1.as.num *= -1;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_NOT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value1.as.boolean = !brd_value_truthify(&value1);
                        value1.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_TEST:
                        if (brd_value_truthify(brd_stack_peek(&vm.stack))) {
                                brd_stack_pop(&vm.stack);
                                vm.frame[vm.fp].pc += sizeof(enum brd_bytecode);
                                vm.frame[vm.fp].pc += sizeof(size_t);
                        }
                        break;
                case BRD_VM_TESTN:
                        if (!brd_value_truthify(brd_stack_peek(&vm.stack))) {
                                brd_stack_pop(&vm.stack);
                                vm.frame[vm.fp].pc += sizeof(enum brd_bytecode);
                                vm.frame[vm.fp].pc += sizeof(size_t);
                        }
                        break;
                case BRD_VM_TESTP:
                        if (brd_value_truthify(brd_stack_pop(&vm.stack))) {
                                vm.frame[vm.fp].pc += sizeof(enum brd_bytecode);
                                vm.frame[vm.fp].pc += sizeof(size_t);
                        }
                        break;
                case BRD_VM_SET_VAR:
                        READ_STRING_INTO(value1.as.string);
                        id = value1.as.string->s;
                        value1 = *brd_stack_pop(&vm.stack);
                        brd_value_map_set(&vm.frame[vm.fp].vars, id, &value1);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_JMP:
                        jmp = *(size_t *)(vm.bytecode + vm.frame[vm.fp].pc);
                        vm.frame[vm.fp].pc += jmp;
                        break;
                case BRD_VM_JMPB:
                        jmp = *(size_t *)(vm.bytecode + vm.frame[vm.fp].pc);
                        vm.frame[vm.fp].pc -= jmp;
                        break;
                case BRD_VM_BUILTIN:
                        b = *(size_t *)(vm.bytecode + vm.frame[vm.fp].pc);
                        vm.frame[vm.fp].pc += sizeof(size_t);
                        if (b == BRD_GLOBAL_OBJECT) {
                                value1 = object_class;
                        } else {
                                value1.vtype = BRD_VAL_BUILTIN;
                                value1.as.builtin = b;
                        }
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_CALL:
                        num_args = *(size_t *)(vm.bytecode + vm.frame[vm.fp].pc);
                        vm.frame[vm.fp].pc += sizeof(size_t);
                        value1 = *brd_stack_pop(&vm.stack);
                        vm.stack.sp -= num_args;
                        if (IS_VAL(value1, BRD_VAL_BUILTIN)) {
                                int new = builtin_function[value1.as.builtin](
                                        vm.stack.sp,
                                        num_args,
                                        &value2
                                );
                                if (new) {
                                        brd_vm_allocate(value2.as.heap);
                                }
                                brd_stack_push(&vm.stack, &value2);
                        } else {
                                brd_value_call(&value1, vm.stack.sp, num_args);
                        }
                        break;
                case BRD_VM_CLOSURE:
                        value1.vtype = BRD_VAL_HEAP;
                        value1.as.heap = malloc(sizeof(struct brd_heap_entry));
                        brd_vm_allocate(value1.as.heap);
                        value1.as.heap->htype = BRD_HEAP_CLOSURE;
                        value1.as.heap->as.closure =
                                malloc(sizeof(struct brd_value_closure));
                        num_args = *(size_t *)(vm.bytecode + vm.frame[vm.fp].pc);
                        vm.frame[vm.fp].pc += sizeof(size_t);
                        args = malloc(sizeof(char *) * num_args);
                        for (int i = 0; i < num_args; i++) {
                                READ_STRING_INTO(value2.as.string);
                                args[i] = value2.as.string->s;
                        }
                        brd_value_closure_init(
                                value1.as.heap->as.closure,
                                args,
                                num_args,
                                vm.frame[vm.fp].pc
                                + sizeof(enum brd_bytecode) + sizeof(size_t)
                        );
                        brd_value_map_set( /* for recursive functions */
                                &value1.as.heap->as.closure->env,
                                "self",
                                &value1
                        );
                        brd_value_map_copy(
                                &value1.as.heap->as.closure->env,
                                &vm.frame[vm.fp].vars
                        );
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_LIST:
                        value1.vtype = BRD_VAL_HEAP;
                        value1.as.heap = malloc(sizeof(*value1.as.heap));
                        value1.as.heap->htype = BRD_HEAP_LIST;
                        value1.as.heap->as.list = malloc(
                                sizeof(struct brd_value_list)
                        );
                        brd_value_list_init(value1.as.heap->as.list);
                        brd_vm_allocate(value1.as.heap);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_GET_IDX:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        if (!IS_VAL(value1, BRD_VAL_NUM)) {
                                BARF("attempted to index with a non-number");
                        }
                        if (brd_value_index(&value2, floorl(value1.as.num))) {
                                brd_vm_allocate(value2.as.heap);
                        }
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_SET_IDX:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        if (!IS_HEAP(value2, BRD_HEAP_LIST)) {
                                BARF("attempted to index a non-list");
                        } else if (!IS_VAL(value1, BRD_VAL_NUM)) {
                                BARF("attempted to index with a non-number");
                        }
                        value3 = *brd_stack_pop(&vm.stack);
                        brd_value_list_set(
                                value2.as.heap->as.list,
                                (size_t) floorl(value1.as.num),
                                &value3
                        );
                        brd_stack_push(&vm.stack, &value3);
                        break;
                case BRD_VM_PUSH:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_peek(&vm.stack);
                        brd_value_list_push(
                                value2.as.heap->as.list,
                                &value1
                        );
                        break;
                case BRD_VM_GET_FIELD:
                        READ_STRING_INTO(value1.as.string);
                        id = value1.as.string->s;
                        value1 = *brd_stack_pop(&vm.stack);
                        if (!IS_HEAP(value1, BRD_HEAP_OBJECT)) {
                                BARF("can only access fields of objects");
                        }
                        valuep = brd_value_map_get(
                                &value1.as.heap->as.object->fields,
                                id
                        );
                        if (valuep == NULL) {
                                value1.vtype = BRD_VAL_UNIT;
                                brd_stack_push(&vm.stack, &value1);
                        } else {
                                brd_stack_push(&vm.stack, valuep);
                        }
                        break;
                case BRD_VM_ACC_OBJ:
                        READ_STRING_INTO(value1.as.string);
                        id = value1.as.string->s;
                        value1 = *brd_stack_pop(&vm.stack);
                        brd_value_acc_obj(&value1, id);
                        break;
                case BRD_VM_SET_FIELD:
                        READ_STRING_INTO(value1.as.string);
                        id = value1.as.string->s;
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        if (!IS_HEAP(value1, BRD_HEAP_OBJECT)) {
                                BARF("can only access fields of objects");
                        }
                        brd_value_map_set(
                                &value1.as.heap->as.object->fields,
                                id, &value2
                        );
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_SUBCLASS:
                        value1 = *brd_stack_pop(&vm.stack); /* constructor */
                        value2 = *brd_stack_pop(&vm.stack); /* super */
                        value3.vtype = BRD_VAL_HEAP;
                        value3.as.heap = malloc(sizeof(*value3.as.heap));
                        value3.as.heap->htype = BRD_HEAP_CLASS;
                        value3.as.heap->as.class = malloc(sizeof(struct brd_value_class));
                        brd_vm_allocate(value3.as.heap);
                        if (!IS_HEAP(value2, BRD_HEAP_CLASS)) {
                                BARF("attempted to make a subclass of a non-class");
                        }
                        brd_value_class_subclass(
                                value3.as.heap->as.class,
                                &value2.as.heap->as.class,
                                &value1.as.heap->as.closure
                        );
                        brd_stack_push(&vm.stack, &value3);
                        break;
                case BRD_VM_SET_CLASS:
                        READ_STRING_INTO(value1.as.string);
                        id = value1.as.string->s;
                        value1 = *brd_stack_pop(&vm.stack); /* method */
                        value2 = *brd_stack_peek(&vm.stack); /* class */
                        brd_value_map_set(
                                &value2.as.heap->as.class->methods,
                                id, &value1
                        );
                        break;
                case BRD_VM_RETURN:
                        if (vm.fp == 0) {
                                goto exit_loop;
                        } else {
                                vm.fp--;
                        }
                        break;
                case BRD_VM_POP:
#ifdef DEBUG
                        value1 = *brd_stack_pop(&vm.stack);
                        printf(" ======== ");
                        brd_value_debug(&value1);
                        printf("\n");
#else
                        brd_stack_pop(&vm.stack);
#endif
                        brd_vm_gc();
                        break;
                }
        }

exit_loop:
        ;
#undef READ_STRING_INTO
}

void
brd_vm_gc(void)
{
        struct brd_heap_entry *heap, *prev;

        heap = vm.heap->next;
        while (heap != NULL) {
                heap->marked = false;
                heap = heap->next;
        }
        object_class.as.heap->marked = true;

        /* mark values in the stack */
        for (struct brd_value *p = vm.stack.values; p < vm.stack.sp; p++) {
                brd_value_gc_mark(p);
        }

        /* mark values held by variables */
        for (int i = 0; i <= vm.fp; i++) {
                brd_value_map_mark(&vm.frame[i].vars);
        }

        prev = vm.heap;
        heap = prev->next;
        while (heap != NULL) {
                struct brd_heap_entry *next = heap->next;
                if (heap->marked) {
                        prev = heap;
                        heap = next;
                        continue;
                }

                prev->next = next;
                brd_heap_destroy(heap);
                free(heap);
                heap = next;
        }
}
