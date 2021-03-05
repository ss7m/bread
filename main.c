#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

/* reason the parser failed (used for repl */
enum brd_compiler_status {
        BRD_REPL_SUCCESS,
        BRD_REPL_TOKEN,
        BRD_REPL_PARSER,
};

static char *
brd_read_file(const char *file_name)
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

static int
brd_parse_and_compile_repl(char *code)
{
        /* return true upon success */
        struct brd_token_list tokens;
        struct brd_node *program;

        brd_token_list_init(&tokens);
        if (!brd_token_list_tokenize(&tokens, code)) {
                fprintf(stderr, "Error: %s%s\n", error_message, bad_character);
                brd_token_list_destroy(&tokens);
                return BRD_REPL_TOKEN;
        }

        program = brd_parse_program(&tokens);
        brd_token_list_destroy(&tokens);
        if (program == NULL) {
                return BRD_REPL_PARSER;
        }

        brd_node_compile(program);
        brd_node_destroy(program);

        /* overwrite the last pop so that it can later be saved into "_" */
        vm.bc_length -= sizeof(enum brd_bytecode);
        *(enum brd_bytecode *)(vm.bytecode + vm.bc_length - 1) = BRD_VM_RETURN;

        return BRD_REPL_SUCCESS;
}

static void
brd_repl(void)
{
        char code[1024], extra[512];
        int empty_line;
        enum brd_compiler_status compiler_status;

        for (;;) {
start_loop:
                printf(">>> ");
                if (fgets(code, sizeof(code), stdin) == NULL) {
                        break;
                }

                empty_line = true;
                for (int i = 0; code[i] != '\0'; i++) {
                        if (!isspace(code[i])) {
                                empty_line = false;
                                break;
                        }
                }
                if (empty_line) {
                        continue;
                }

                compiler_status = brd_parse_and_compile_repl(code);
                while (compiler_status != BRD_REPL_SUCCESS) {
                        if (compiler_status == BRD_REPL_TOKEN) {
                                goto start_loop;
                        }

                        printf(" <| ");
                        if (fgets(extra, sizeof(extra), stdin) == NULL) {
                                printf("\nKeyboard Interrupt\n");
                                clearerr(stdin);
                                goto start_loop;
                        }

                        strcat(code, extra);
                        compiler_status = brd_parse_and_compile_repl(code);
                }

                brd_vm_run();

                if (brd_stack_peek(&vm.stack)->vtype != BRD_VAL_UNIT) {
                        struct brd_value *val = brd_stack_pop(&vm.stack);
                        brd_value_map_set(&vm.frame[0].locals, "_", val);
                        brd_value_debug(val);
                        printf("\n");
                }
        }
}

static void
brd_parse_and_compile(char *code)
{
        struct brd_token_list tokens;
        struct brd_node *program;

        brd_token_list_init(&tokens);
        if (!brd_token_list_tokenize(&tokens, code)) {
                fprintf(stderr, "Error: %s%s\n", error_message, bad_character);
                brd_token_list_destroy(&tokens);
                exit(EXIT_FAILURE);
        }

        program = brd_parse_program(&tokens);
        brd_token_list_destroy(&tokens);
        if (program == NULL) {
                fprintf(stderr, "Error: %s on line %d\n", error_message, line_number);
                exit(EXIT_FAILURE);
        }

        brd_node_compile(program);
        brd_node_destroy(program);
}

static void
brd_run_file(char *file_name)
{
        char *code;

        code = brd_read_file(file_name);
        if (code == NULL) {
                fprintf(stderr, "Unable to open file %s\n", file_name);
                exit(EXIT_FAILURE);
        }
        brd_parse_and_compile(code);
        free(code);

        brd_vm_run();
}

int
main(int argc, char **argv)
{
        brd_vm_init();
        if (argc == 1) {
                printf("Welcome to the repl!\n");
                brd_repl();
                printf("\nGoodbye!\n");
        } else {
                brd_run_file(argv[1]);
        }
        brd_vm_destroy();
}
