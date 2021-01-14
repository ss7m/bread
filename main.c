#include "common.h"
#include "ast.h"
#include "vm.h"
#include "token.h"

int main(void)
{
        struct brd_node *un, *var, *num, *str, *b, *unit, *bin1, *bin2, *bin3, *ass;
        struct brd_value_map map;
        struct brd_value val;
        struct brd_token_list list;

        /* test ast node */
        var = brd_node_var_new(strdup("x"));
        num = brd_node_num_lit_new(123);
        str = brd_node_string_lit_new(strdup("Hello, World!"));
        b = brd_node_bool_lit_new(true);
        unit = brd_node_unit_lit_new();

        bin1 = brd_node_binop_new(BRD_PLUS, num, str);
        un = brd_node_unary_new(BRD_NOT, bin1);
        /* ass = brd_node_assign_new(un ,var) */ /* fails b/c un isn't an lvalue */
        ass = brd_node_assign_new(var, un);
        bin2 = brd_node_binop_new(BRD_PLUS, b, unit);
        bin3 = brd_node_binop_new(BRD_PLUS, ass, bin2);

        brd_node_destroy(bin3);

        /* test hash map */
        val.vtype = BRD_VAL_NUM;
        val.as.num = 123;
        brd_value_map_init(&map);
        brd_value_map_set(&map, "abc", &val);
        brd_value_map_set(&map, "hello", &val);
        brd_value_map_set(&map, "world", &val);
        brd_value_map_set(&map, "xyz", &val);
        brd_value_map_set(&map, "test", &val);
        brd_value_map_set(&map, "test", &val);
        brd_value_map_set(&map, "here", &val);

        printf("%Lf\n", brd_value_map_get(&map, "test")->as.num);
        printf("%s\n", brd_value_map_get(&map, "here") ? "true" : "false");
        printf("%s\n", brd_value_map_get(&map, "not here") ? "true" : "false");

        brd_value_map_destroy(&map);

        /* test token list */
        brd_token_list_init(&list);
        brd_token_list_tokenize(&list, "123.5 + - <= > ( ) \n = < >=");
        brd_token_list_tokenize(&list, "false true and set or sdf_e312_dD   ");
        brd_token_list_tokenize(&list, "\"Hello, World!\\n\" 33422342.23  ");
        /* brd_token_list_tokenize(&list, "\"Hello, World!\\n "); */ /* parsing error */
        brd_token_list_destroy(&list);
}
