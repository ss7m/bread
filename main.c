#include "common.h"
#include "ast.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

int
main(void)
{
        char *test_string = "3 + 10 * 10\n \"Hello, World!\" \n true or false \n"
                "set x = \"woweeee\"\n"
                "set x = 10 * (set y \n = 2) \n x + y\n 10 - 2\n"
                "17 + \"19.3\"\n"
                "not \"\" \n"
                "unit <= -1";
        struct brd_token_list list, copy;
        struct brd_node_program *program;
        void *bytecode;
        struct brd_vm vm;

        brd_token_list_init(&list);
        brd_token_list_tokenize(&list, test_string);
        copy = list;

        program = brd_parse_program(&list);
        brd_token_list_destroy(&copy);

        bytecode = brd_node_compile((struct brd_node *)program);
        brd_node_destroy(program);

        brd_vm_init(&vm, bytecode);
        brd_vm_run(&vm);
        brd_vm_destroy(&vm);
}
