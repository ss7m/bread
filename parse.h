#ifndef BRD_PARSE_H
#define BRD_PARSE_H

struct brd_node *brd_parse_program(struct brd_token_list *tokens);
struct brd_node *brd_parse_expression_stmt(struct brd_token_list *tokens);
struct brd_node *brd_parse_expression(struct brd_token_list *tokens);
struct brd_node *brd_parse_lvalue(struct brd_token_list *tokens);
struct brd_node *brd_parse_concatexp(struct brd_token_list *tokens);
struct brd_node *brd_parse_orexp(struct brd_token_list *tokens);
struct brd_node *brd_parse_andexp(struct brd_token_list *tokens);
struct brd_node *brd_parse_compexp(struct brd_token_list *tokens);
struct brd_node *brd_parse_addexp(struct brd_token_list *tokens);
struct brd_node *brd_parse_mulexp(struct brd_token_list *tokens);
struct brd_node *brd_parse_powexp(struct brd_token_list *tokens);
struct brd_node *brd_parse_prefix(struct brd_token_list *tokens);
struct brd_node *brd_parse_postfix(struct brd_token_list *tokens);
struct brd_node *brd_parse_base(struct brd_token_list *tokens);
struct brd_node *brd_parse_func(struct brd_token_list *tokens);
struct brd_node_arglist *brd_parse_arglist(struct brd_token_list *tokens);
struct brd_node *brd_parse_builtin(struct brd_token_list *tokens);
struct brd_node *brd_parse_body(struct brd_token_list *tokens);
struct brd_node *brd_parse_body_stmt(struct brd_token_list *tokens);
struct brd_node *brd_parse_ifexpr(struct brd_token_list *tokens);
int brd_parse_elif(struct brd_token_list *tokens, struct brd_node_elif *e);
struct brd_node *brd_parse_while(struct brd_token_list *tokens);

#endif
