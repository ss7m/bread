#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

char *open_file(char *file_name);
void run_file(char *file_name);

char *
open_file(char *file_name)
{
        FILE *file;
        char *contents;
        size_t length;

        file = fopen(file_name, "rb");
        if (file == NULL) {
                return NULL;
        }
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        contents = malloc(length + 1);
        contents[fread(contents, sizeof(char), length, file)] = '\0';
        fclose(file);

        return contents;
}

void
run_file(char *file_name)
{
        char *code;
        struct brd_token_list tokens;
        struct brd_node *program;
        brd_bytecode_t *bytecode;

        code = open_file(file_name);

        if (code == NULL) {
                exit(EXIT_FAILURE);
        }

        brd_token_list_init(&tokens);
        if (!brd_token_list_tokenize(&tokens, code)) {
                fprintf(
                        stderr,
                        "Tokenizer error: %s%s\n",
                        error_message,
                        bad_character
                );
                brd_token_list_destroy(&tokens);
                exit(EXIT_FAILURE);
        }
        free(code);

        program = brd_parse_program(&tokens);
        brd_token_list_destroy(&tokens);
        if (program == NULL) {
                fprintf(
                        stderr,
                        "Parser error: %s on line %d\n",
                        error_message,
                        line_number
                );
                exit(EXIT_FAILURE);
        }

        bytecode = brd_node_compile(program);
        brd_node_destroy(program);

        brd_vm_init(bytecode);
        brd_vm_run();
        brd_vm_destroy();
}

int
main(int argc, char **argv)
{
        if (argc != 2) {
                BARF("Enter exactly 1 argument");
        }

        run_file(argv[1]);
}
