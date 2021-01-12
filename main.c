#include "common.h"
#include "ast.h"

int main(void)
{
        struct flt_node *un, *var, *num, *str, *b, *unit, *bin1, *bin2, *bin3, *bin4;

        flt_node_var_init(&var, strdup("x"));
        flt_node_num_lit_init(&num, 123);
        flt_node_string_lit_init(&str, strdup("Hello, World!"));
        flt_node_bool_lit_init(&b, true);
        flt_node_unit_lit_init(&unit);

        flt_node_binop_init(&bin1, FLT_PLUS, num, str);
        flt_node_unary_init(&un, FLT_NOT, bin1);
        flt_node_binop_init(&bin2, FLT_PLUS, b, unit);
        flt_node_binop_init(&bin3, FLT_PLUS, un, bin2);
        flt_node_binop_init(&bin4, FLT_PLUS, bin3, var);

        flt_node_destroy(bin4);
}
