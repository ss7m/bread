#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

/* reason the parser failed (used for repl */
static enum brd_repl_parser_failure_reason {
        BRD_REPL_TOKEN,
        BRD_REPL_PARSER,
} failure_reason;

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

static brd_bytecode_t *
brd_parse_and_compile_repl(char *code)
{
        struct brd_token_list tokens;
        struct brd_node *program;
        brd_bytecode_t *bytecode;

        brd_token_list_init(&tokens);
        if (!brd_token_list_tokenize(&tokens, code)) {
                fprintf(
                        stderr,
                        "Tokenizer error: %s%s\n",
                        error_message,
                        bad_character
                );
                brd_token_list_destroy(&tokens);
                failure_reason = BRD_REPL_TOKEN;
                return NULL;
        }

        program = brd_parse_program(&tokens);
        brd_token_list_destroy(&tokens);
        if (program == NULL) {
                failure_reason = BRD_REPL_PARSER;
                return NULL;
        }

        bytecode = brd_node_compile(program);
        brd_node_destroy(program);

        return bytecode;
}

static void
brd_repl(void)
{
        /*
         * need to hold onto old_code because string constants
         * point into the bytecode
         */
        brd_bytecode_t **old_code;
        size_t oc_length, oc_capacity;

        oc_length = 0;
        oc_capacity = 4;
        old_code = malloc(sizeof(brd_bytecode_t *) * oc_capacity);

        brd_vm_init(NULL);

        for (;;) {
                brd_bytecode_t *bytecode;
                char code[1024];
                int empty_line;

                printf("> ");
                if (fgets(code, sizeof(code), stdin) == NULL) {
                        goto loop_end;
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

                bytecode = brd_parse_and_compile_repl(code);
                while (bytecode == NULL) {
                        char c[512];

                        if (failure_reason == BRD_REPL_TOKEN) {
                                goto loop_end;
                        }

                        printf("| ");
                        if (fgets(c, sizeof(c), stdin) == NULL) {
                                goto loop_end;
                        }

                        strcat(code, c);
                        bytecode = brd_parse_and_compile_repl(code);
                }

                brd_vm_reset(bytecode);
                brd_vm_run();

                if (oc_length >= oc_capacity) {
                        oc_capacity *= 1.5;
                        old_code = realloc(
                                old_code,
                                sizeof(brd_bytecode_t *) * oc_capacity
                        );
                }
                old_code[oc_length++] = bytecode;

        }

loop_end:
        brd_vm_reset(NULL);
        for (int i = 0; i < oc_length; i++) {
                free(old_code[i]);
        }
        free(old_code);
        brd_vm_destroy();
}

static brd_bytecode_t *
brd_parse_and_compile(char *code)
{
        struct brd_token_list tokens;
        struct brd_node *program;
        brd_bytecode_t *bytecode;

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

        bytecode = brd_node_compile(program);
        brd_node_destroy(program);

        return bytecode;
}

static void
brd_run_file(char *file_name)
{
        char *code;
        brd_bytecode_t *bytecode;

        code = brd_read_file(file_name);
        if (code == NULL) {
                fprintf(stderr, "Unable to open file %s\n", file_name);
        }
        bytecode = brd_parse_and_compile(code);
        free(code);

        brd_vm_init(bytecode);
        brd_vm_run();
        brd_vm_destroy();
}

int
main(int argc, char **argv)
{
        if (argc == 1) {
                brd_repl();
        } else {
                brd_run_file(argv[1]);
        }
}
