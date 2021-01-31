#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

/*
 * TODO: don't gc the bottom of the stack in the repl, because
 * we want to put it in the _ variable, figure this out
 */

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
                fprintf(
                        stderr,
                        "Tokenizer error: %s%s\n",
                        error_message,
                        bad_character
                );
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

        return BRD_REPL_SUCCESS;
}

static void
brd_repl(void)
{
        printf("Welcome to the repl!\n");

        for (;;) {
                char code[1024];
                int empty_line;
                enum brd_compiler_status compiler_status;

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
                        char c[512];

                        if (compiler_status == BRD_REPL_TOKEN) {
                                goto loop_end;
                        }

                        printf(" <| ");
                        if (fgets(c, sizeof(c), stdin) == NULL) {
                                printf("\nKeyboard Interrupt\n");
                                clearerr(stdin);
                                goto loop_end;
                        }

                        strcat(code, c);
                        compiler_status = brd_parse_and_compile_repl(code);
                }

                brd_vm_run();

                // FIXME: see TODO at top of file
                //if (vm.stack.values[0].vtype != BRD_VAL_UNIT) {
                //        brd_value_debug(&vm.stack.values[0]);
                //        printf("\n");
                //}
                //brd_value_map_set(&vm.frame[0].vars, "_", &vm.stack.values[0]);
loop_end:;
        }

        printf("\nGoodbye!\n");
}

static void
brd_parse_and_compile(char *code)
{
        struct brd_token_list tokens;
        struct brd_node *program;

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
                brd_repl();
        } else {
                brd_run_file(argv[1]);
        }
        brd_vm_destroy();
}
