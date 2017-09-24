#ifndef AST_H
#define AST_H

// ast value
struct array;
struct fn_call;
struct block;
typedef struct {
    union {
        int nint;
        float nfloat;
        char *string;
        struct array *array;
        struct fn_call *fn_call;
        struct block *block;
    } val;
    int type;
} value;
#define VALUE_NINT_TYPE   0
#define VALUE_NFLOAT_TYPE 1
#define VALUE_STR_TYPE    2
#define VALUE_IDENT_TYPE  3
#define VALUE_CALL_TYPE   4
#define VALUE_BLOCK_TYPE  5
#define VALUE_ARRAY_TYPE  6
void value_destroy(value *val);

// item
struct array_item;
typedef struct array_item {
    value *val;
    struct array_item *prev, *next;
} array_item;
void array_item_destroy(array_item *array_item);

typedef struct array {
    array_item *first, *last;
} array;
void array_destroy(array *array);

// functions
struct fn_arg;
typedef struct fn_arg {
    value *val;
    struct fn_arg *next;
} fn_arg;
void fn_arg_destroy(fn_arg *arg);

typedef struct {
    fn_arg *first, *last;
} fn_args;
void fn_args_destroy(fn_args *args);

typedef struct fn_call {
    fn_args *args;
    char *name;
    struct fn_call *next;
} fn_call;
inline int is_op(char op) {
    return
        op == '+' || op == '-' ||
        op == '*' || op == '/' || op == '%' ||
        op == '=' || op == '<' || op == '>';
}
void fn_call_destroy(fn_call *call);

// block
typedef struct block {
    fn_call *first, *last;
} block;
void block_destroy(block *block);

// program
typedef struct {
    fn_call *first, *last;
} program;
void program_destroy(program *program);

#endif
