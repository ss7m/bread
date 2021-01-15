#include "common.h"
#include "ast.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

int
main(void)
{
        char *test_string = "set x = (2.8 + \n y * \n\n-4e9 = \"Hello, World!\\n\") or (set ab_r = not true and false) \n\n set zd_d = 8";
        struct brd_token_list list, copy;
        struct brd_node_program *program;
        void *bytecode;

        brd_token_list_init(&list);
        brd_token_list_tokenize(&list, test_string);
        copy = list;
        program = brd_parse_program(&list);

        bytecode = brd_node_compile((struct brd_node *)program);
        
        free(bytecode);
        brd_node_destroy(program);
        brd_token_list_destroy(&copy);
}
