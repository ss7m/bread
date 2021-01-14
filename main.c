#include "common.h"
#include "ast.h"
#include "vm.h"
#include "token.h"
#include "parse.h"

int main(void)
{
        char *test_string = "set x = (2.8 + \n y * \n\n-4e9 = \"Hello, World!\\n\") or (set ab_r = not true and false)";
        struct brd_token_list list, copy;
        struct brd_node *node;

        brd_token_list_init(&list);
        brd_token_list_tokenize(&list, test_string);
        copy = list;
        node = brd_parse_expression(&list);
        
        brd_node_destroy(node);
        brd_token_list_destroy(&copy);
}
