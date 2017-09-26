#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

// compiler
FILE *asm_file;
char outputs[256]; // .s file
int asm_lib;
int asm_nostdlib;

// compiler function variables
struct asm_fn;
typedef struct asm_fn {
    char *name;
    fn_call *fn_call;
    struct asm_fn *next;
} asm_fn;
extern asm_fn *asm_fn_first, *asm_fn_last;
void asm_fn_add(char *name, fn_call *fn_call);

// compiler string variables
struct asm_var;
typedef struct asm_var {
    char *name;
    union {
        int nint;
        float nfloat;
        char *string;
        int reserve;
    } value;
    int type;
    struct asm_var *next;
} asm_var;
#define ASM_VAR_NINT_TYPE   0
#define ASM_VAR_NFLOAT_TYPE 1
#define ASM_VAR_STRING_TYPE 2
#define ASM_VAR_RES_TYPE    3
extern asm_var *as_first, *as_last;
extern int asm_var_len;

void add_asm_var(asm_var *as);
asm_var *get_asm_var(char *name);
char *add_asm_nint(int nint, char *name);
char *add_asm_nfloat(float nfloat, char *name);
char *add_asm_str(char *value, char *name);
char *reserve_asm_var(int len, char *name);
void flush_asm_var();
void print_asm_var(asm_var * str);
void print_all_asm_var();

// code generator
void asm_write(program *program, int force_stdout);
void asm_write_program(program *program);
void asm_write_fn_call(fn_call *fn_call);
void asm_write_fn_args(fn_args *args);
void asm_write_fn_arg(fn_arg *arg);
void asm_write_arg(value *val);
char *asm_write_value(value *val);
void asm_write_defun(fn_call *fn_call);
void asm_write_end(program *program);

#endif
