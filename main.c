#include "common.h"
#include "ast.h"

int main(void)
{
        struct flt_node *un, *var, *num, *str, *b, *unit, *bin1, *bin2, *bin3, *bin4;

        var = flt_node_var_new(strdup("x"));
        num = flt_node_num_lit_new(123);
        str = flt_node_string_lit_new(strdup("Hello, World!"));
        b = flt_node_bool_lit_new(true);
        unit = flt_node_unit_lit_new();

        bin1 = flt_node_binop_new(FLT_PLUS, num, str);
        un = flt_node_unary_new(FLT_NOT, bin1);
        bin2 = flt_node_binop_new(FLT_PLUS, b, unit);
        bin3 = flt_node_binop_new(FLT_PLUS, un, bin2);
        bin4 = flt_node_binop_new(FLT_PLUS, bin3, var);

        flt_node_destroy(bin4);
}
