#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

int
main(int argc, char **argv)
{
        FILE *file;
        char *code;
        size_t file_length;

        struct brd_token_list list, copy;
        struct brd_node_program *program;
        void *bytecode;


        if (argc != 2) {
                BARF("Enter exactly 1 argument");
        }

        file = fopen(argv[1], "rb");
        if (file == NULL) {
                BARF("File %s doesn't exist", argv[1]);
        }
        fseek(file, 0, SEEK_END);
        file_length = ftell(file);
        fseek(file, 0, SEEK_SET);
        code = malloc(file_length + 1);
        code[fread(code, sizeof(char), file_length, file)] = '\0';
        fclose(file);

        brd_token_list_init(&list);
        brd_token_list_tokenize(&list, code);
        free(code);
        copy = list;

        program = brd_parse_program(&list);
        brd_token_list_destroy(&copy);

        bytecode = brd_node_compile((struct brd_node *)program);
        brd_node_destroy(program);

        brd_vm_init(bytecode);
        brd_vm_run();
        brd_vm_destroy();
}
