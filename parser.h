#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

extern char *source;
unsigned int parser_pos;
inline void parser_reset() {
    parser_pos = 0;
}
inline char parser_getch() {
    if(parser_pos >= strlen(source)) return '\0';
    return source[parser_pos];
}
value *parse_value();
fn_arg *parse_fn_arg();
fn_args *parse_fn_args();
fn_call *parse_fn_call();
void parse_ws();
program *parse_program();

#endif
