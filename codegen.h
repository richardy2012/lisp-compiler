#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

// compiler
FILE *asm_file;
char outputs[256]; // .s file

// compiler string variables
struct asm_str;
typedef struct asm_str {
    char *name;
    char *value;
    unsigned int reserve;
    struct asm_str *next;
} asm_str;
extern asm_str *as_first, *as_last;
extern unsigned int asm_str_len;

char *add_asm_str(char *value);
char *reserve_asm_str(int len);
void flush_asm_str();
void print_asm_str(asm_str * str);
void print_all_asm_str();

// code generator
int asm_write(program *program, unsigned int force_stdout);
int asm_write_program(program *program);
int asm_write_fn_call(fn_call *fn_call);
int asm_write_fn_args(fn_args *args);
int asm_write_fn_arg(fn_arg *arg);
char *asm_write_value(value *val);
int asm_write_end();

extern unsigned int
    uses_syswrite_stdout,
    uses_strlen,
    uses_atoi,
    uses_itoa,
    uses_sbrk;

#endif
