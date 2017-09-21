#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "parser.h"
#include "utils.h"

char *source = NULL;

void parse_ws() {
    while(parser_pos < strlen(source)) {
        if(parser_getch() == ' '  ||
           parser_getch() == '\t' ||
           parser_getch() == '\n')
            parser_pos++;
        else
            break;
    }
}

value *parse_value() {
    deprintf("PARSE_VALUE\n");
    if(parser_pos >= strlen(source)) {
        printf("Expected value, got end of file.\n");
        return NULL;
    }
    char ch = parser_getch();
    deprintf("%c\n", ch);
    value *val = malloc(sizeof(value));
    if(isalpha(ch)) {
        char *string = malloc(2);
        unsigned int ssize = 1;
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
        unsigned int ssize = 1;
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
            printf("Expected end of string\n");
            free(string);
            free(val);
            return NULL;
        }
        parser_pos++;
        
        val->val.string = string;
        val->type = VALUE_STR_TYPE;
    } else if (isdigit(ch) || ch == '-') {
        int neg = 1;
        if(ch == '-') { neg = -1; parser_pos++; }
        
        char *string = malloc(2);
        unsigned int ssize = 1;
        string[0] = ch;
        parser_pos++;
        while(isdigit(parser_getch())) {
            string = realloc(string, ssize + 1);
            string[ssize] = source[parser_pos++];
            ssize++;
        }
        string[ssize] = '\0';
        
        deprintf("Number %s\n",string);
        val->val.nint = atoi(string) * neg;
        val->type = VALUE_NINT_TYPE;
    } else if (ch == '(') {
        deprintf("VALFNCALL\n");
        fn_call *call = parse_fn_call();
        val->val.fn_call = call;
        val->type = VALUE_CALL_TYPE;
    } else {
        printf("Expected value, got %c\n", ch);
        free(val);
        return NULL;
    }
    return val;
}

fn_arg *parse_fn_arg() {
    deprintf("PARSE_FN_ARG\n");
    value *val = parse_value();
    if(val == NULL) return NULL;
    fn_arg *arg = malloc(sizeof(fn_arg));
    arg->val = val;
    arg->next = NULL;
    return arg;
}

fn_args *parse_fn_args() {
    deprintf("PARSE_FN_ARGS\n");
    fn_args *args = malloc(sizeof(fn_args));
    if(parser_getch() == ')') {
        args->first = NULL;
        args->last = NULL;
        return args;
    }
    fn_arg *first = parse_fn_arg();
    fn_arg *last = first;
    while(parser_getch() == ' ') {
        parser_pos++;
        parse_ws();
        fn_arg *arg = parse_fn_arg();
        if(arg == NULL) {
            free(args);
            return NULL;
        }
        last->next = arg;
        last = arg;
        if(parser_getch() == ')') break;
    }
    args->first = first;
    args->last = last;
    return args;
}

fn_call *parse_fn_call() {
    deprintf("FN_CALL\n");
    
    if(parser_getch() != '(') {
        printf("Expected ( in function call start\n");
        deprintf("%i\n", parser_getch());
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
        printf("Expected name in function call\n");
        return NULL;
    }
    
    unsigned int isize = 1;
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
    
    if(parser_getch() != ' ') {
        printf("Expected space after function name\n");
        free(call);
        free(name);
        return NULL;
    }
    parser_pos++;
    parse_ws();
    
    if(parser_getch() == ')') {
        return call;
    }
    
    call->args = parse_fn_args();
    if(call->args == NULL) {
        return call;
    }
    call->next = NULL;
    parse_ws();
    
    if(parser_getch() != ')') {
        printf("Expected ) in function call end\n");
        free(name);
        fn_args_destroy(call->args);
        free(call);
        return NULL;
    }
    parser_pos++;
    
    return call;
}

program *parse_program() {
    deprintf("PROGRAM\n");
    program *program = malloc(sizeof(program));
    
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
