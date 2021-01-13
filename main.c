#include "common.h"
#include "ast.h"
#include "vm.h"

int main(void)
{
        struct flt_node *un, *var, *num, *str, *b, *unit, *bin1, *bin2, *bin3, *ass;
        struct flt_value_map map;
        struct flt_value val;

        var = flt_node_var_new(strdup("x"));
        num = flt_node_num_lit_new(123);
        str = flt_node_string_lit_new(strdup("Hello, World!"));
        b = flt_node_bool_lit_new(true);
        unit = flt_node_unit_lit_new();

        bin1 = flt_node_binop_new(FLT_PLUS, num, str);
        un = flt_node_unary_new(FLT_NOT, bin1);
        /* ass = flt_node_assign_new(un ,var) */ /* fails b/c un isn't an lvalue */
        ass = flt_node_assign_new(var, un);
        bin2 = flt_node_binop_new(FLT_PLUS, b, unit);
        bin3 = flt_node_binop_new(FLT_PLUS, ass, bin2);

        flt_node_destroy(bin3);

        val.vtype = FLT_VAL_NUM;
        val.as.num = 123;
        flt_value_map_init(&map);
        flt_value_map_set(&map, "abc", &val);
        flt_value_map_set(&map, "hello", &val);
        flt_value_map_set(&map, "world", &val);
        flt_value_map_set(&map, "xyz", &val);
        flt_value_map_set(&map, "test", &val);
        flt_value_map_set(&map, "test", &val);
        flt_value_map_set(&map, "here", &val);

        printf("%Lf\n", flt_value_map_get(&map, "test")->as.num);
        printf("%s\n", flt_value_map_get(&map, "here") ? "true" : "false");
        printf("%s\n", flt_value_map_get(&map, "not here") ? "true" : "false");

        flt_value_map_destroy(&map);
}
