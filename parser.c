#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "parser.h"
#include "utils.h"

char *source = NULL;
int is_in_defun = 0; // HACKY way to parse defun's arguments

void parse_ws() {
    while(parser_pos < strlen(source)) {
        if(is_ws(parser_getch()))
            parser_pos++;
        else
            break;
    }
}

value *parse_value(int is_args) {
    deprintf("PARSE_VALUE\n");
    if(parser_pos >= strlen(source)) {
        error("Expected value, got end of file.\n");
        return NULL;
    }
    char ch = parser_getch();
    deprintf("%c\n", ch);
    value *val = malloc(sizeof(value));
    if(isalpha(ch)) {
        char *string = malloc(2);
        int ssize = 1;
        string[0] = ch;
        parser_pos++;
        while(isalpha(parser_getch())) {
            string = realloc(string, ssize + 1);
            string[ssize] = source[parser_pos++];
            ssize++;
        }
        string[ssize] = '\0';
        
        val->val.string = string;
        val->type = VALUE_IDENT_TYPE;
    } else if (ch == '"' || ch == '\'') {
        char startch = ch;
        parser_pos++;
        ch = parser_getch();
        
        char *string = malloc(2);
        int ssize = 1;
        string[0] = ch;
        parser_pos++;
        while(parser_getch() != startch) {
            string = realloc(string, ssize + 1);
            string[ssize] = source[parser_pos++];
            ssize++;
        }
        string[ssize] = '\0';
        deprintf("STRING %s\n", string);
        
        if(parser_getch() != startch) {
            error("Expected end of string\n");
            free(string);
            free(val);
            return NULL;
        }
        parser_pos++;
        
        val->val.string = string;
        val->type = VALUE_STR_TYPE;
    } else if (isdigit(ch) || ch == '-') {
        int neg = 1;
        if(ch == '-') {
            neg = -1;
            parser_pos++;
            ch = parser_getch();
        }
        
        char *string = malloc(2);
        int ssize = 1;
        string[0] = ch;
        parser_pos++;
        while(isdigit(parser_getch())) {
            string = realloc(string, ssize + 1);
            string[ssize] = source[parser_pos++];
            ssize++;
        }
        string[ssize] = '\0';
        
        val->val.nint = atoi(string) * neg;
        deprintf("Number %i\n",val->val.nint);
        val->type = VALUE_NINT_TYPE;
    } else if (ch == '(') {
        if(is_args) {
            deprintf("VALARGS\n");
            array *arr = parse_array('(', ')');
            if(arr == NULL) { free(val); return NULL; }
            val->val.array = arr;
            val->type = VALUE_ARRAY_TYPE;
        } else {
            deprintf("VALFNCALL\n");
            fn_call *call = parse_fn_call();
            if(call == NULL) { free(val); return NULL; }
            val->val.fn_call = call;
            val->type = VALUE_CALL_TYPE;
        }
    } else if (ch == '{') {
        deprintf("VALBLOCK\n");
        block *block = parse_block();
        if(block == NULL) { free(val); return NULL; }
        val->val.block = block;
        val->type = VALUE_BLOCK_TYPE;
    } else if (ch == '[') {
        deprintf("VALARRAY\n");
        array *arr = parse_array('[', ']');
        if(arr == NULL) { free(val); return NULL; }
        val->val.array = arr;
        val->type = VALUE_ARRAY_TYPE;
    } else {
        error("Expected value, got %c\n", ch);
        free(val);
        return NULL;
    }
    return val;
}

array *parse_array(char start, char end) {
    if(parser_getch() != start) {
        error("Expected array start %c\n", start);
        return NULL;
    }
    parser_pos++;
    
    array *arr = malloc(sizeof(array));
    arr->first = NULL;
    arr->last = NULL;
    if(parser_getch() == end) {
        parser_pos++;
        return arr;
    }
    
    value *val = parse_value(0);
    array_item *item = malloc(sizeof(array_item));
    item->val = val;
    item->prev = NULL;
    item->next = NULL;
    arr->first = item;
    arr->last = item;
    if(arr->first == NULL) {
        free(arr);
        return NULL;
    }
    
    while(parser_pos < strlen(source) && is_ws(parser_getch()) && parser_getch() != end) {
        parse_ws();
        value *val = parse_value(0);
        if(val == NULL) {
            free(arr);
            return NULL;
        }
        array_item *item = malloc(sizeof(array_item));
        item->val = val;
        item->prev = arr->last;
        item->next = NULL;
        arr->last->next = item;
        arr->last = item;
    }
    parser_pos++;
    
    return arr;
}

fn_arg *parse_fn_arg(int is_args) {
    deprintf("PARSE_FN_ARG\n");
    value *val = NULL;
    if(is_args) {
        val = parse_value(1);
    } else {
        val = parse_value(0);
    }
    if(val == NULL) return NULL;
    fn_arg *arg = malloc(sizeof(fn_arg));
    arg->val = val;
    arg->next = NULL;
    return arg;
}

fn_args *parse_fn_args() {
    deprintf("PARSE_FN_ARGS\n");
    deprintf("IN DEFUN: %i\n", is_in_defun);
    fn_args *args = malloc(sizeof(fn_args));
    if(parser_getch() == ')') {
        args->first = NULL;
        args->last = NULL;
        return args;
    }
    fn_arg *first = parse_fn_arg(0);
    fn_arg *last = first;
    int argn = 1;
    while(is_ws(parser_getch()) && parser_getch() != ')') {
        parse_ws();
        fn_arg *arg;
        if(is_in_defun && argn == 1) {
            arg = parse_fn_arg(1);
        } else {
            arg = parse_fn_arg(0);
        }
        if(arg == NULL) {
            free(args);
            return NULL;
        }
        last->next = arg;
        last = arg;
        argn++;
    }
    args->first = first;
    args->last = last;
    return args;
}

fn_call *parse_fn_call() {
    deprintf("FN_CALL\n");
    
    if(parser_getch() != '(') {
        error("Expected ( in function call start\n");
        deprintf("%c\n", parser_getch());
        return NULL;
    }
    parser_pos++;
    
    char ch = parser_getch();
    char *name;
    if(isalpha(ch) || is_op(ch)) {
        name = malloc(2);
        name[0] = ch;
        name[1] = 0;
        parser_pos++;
    } else {
        error("Expected name in function call\n");
        return NULL;
    }
    
    int isize = 1;
    while(isalnum(parser_getch())) {
        name = realloc(name, isize + 1);
        name[isize] = source[parser_pos++];
        isize++;
    }
    name[isize] = 0;
    deprintf("NAME: %s\n", name);
    
    fn_call *call = malloc(sizeof(fn_call));
    call->name = name;
    call->args = NULL;
    call->next = NULL;
    
    if(parser_getch() == ')') {
        parser_pos++;
        call->args = malloc(sizeof(fn_args));
        call->args->first = NULL;
        call->args->last = NULL;
        return call;
    }
    
    if(strcmp(name, "defun") == 0) {
        is_in_defun = 1;
    }
    
    parse_ws();
    call->args = parse_fn_args();
    if(call->args == NULL) {
        return call;
    }
    call->next = NULL;
    parse_ws();
    
    if(strcmp(name, "defun") == 0) {
        is_in_defun = 0;
    }
    
    if(parser_getch() != ')') {
        error("Expected ) in function call end\n");
        deprintf("%c\n", parser_getch());
        free(name);
        fn_args_destroy(call->args);
        free(call);
        return NULL;
    }
    parser_pos++;
    
    return call;
}

block *parse_block() {
    deprintf("BLOCK\n");
    
    if(parser_getch() != '{') {
        error("Expected block\n");
        return NULL;
    }
    parser_pos++;
    
    block *block = malloc(sizeof(block));
    
    parse_ws();
    block->first = parse_fn_call();
    block->last = NULL;
    if(block->first == NULL) {
        return block;
    }
    parse_ws();
    
    fn_call *cur = block->first;
    while(parser_getch() != '}') {
        fn_call *next = parse_fn_call();
        if(next == NULL) {
            block_destroy(block);
            return NULL;
        }
        cur->next = next;
        cur = next;
        parse_ws();
    }
    parser_pos++;
    
    return block;
}

program *parse_program() {
    deprintf("PROGRAM\n");
    program *program = malloc(sizeof(program));
    
    parse_ws();
    program->first = parse_fn_call();
    if(program->first == NULL) {
        free(program);
        return NULL;
    }
    program->last = NULL;
    parse_ws();
    
    fn_call *cur = program->first;
    while(parser_pos < strlen(source)) {
        fn_call *next = parse_fn_call();
        if(next == NULL) {
            program_destroy(program);
            return NULL;
        }
        cur->next = next;
        cur = next;
        parse_ws();
    }
    
    return program;
}
