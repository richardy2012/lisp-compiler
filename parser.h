#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

extern char *source;
int parser_pos;
inline void parser_reset() {
    parser_pos = 0;
}
inline char parser_getch() {
    if(parser_pos >= strlen(source)) return '\0';
    return source[parser_pos];
}
value *parse_value(int is_args);
array *parse_array(char start, char end);
fn_arg *parse_fn_arg(int is_args);
fn_args *parse_fn_args();
fn_call *parse_fn_call();
void parse_ws();
block *parse_block();
program *parse_program();

#endif
