#include <stdlib.h>
#include "ast.h"

void value_destroy(value *val) {
    if(val->type == VALUE_STR_TYPE || val->type == VALUE_IDENT_TYPE) {
        free(val->val.string);
    } else if (val->type == VALUE_CALL_TYPE) {
        fn_call_destroy(val->val.fn_call);
    }
    free(val);
}

void fn_arg_destroy(fn_arg *arg) {
    value_destroy(arg->val);
    free(arg);
}

void fn_args_destroy(fn_args *args) {
    fn_arg *curr = args->first, *next;
    while(curr != NULL) {
        next = curr->next;
        fn_arg_destroy(curr);
        curr = next;
    }
    free(args);
}

void fn_call_destroy(fn_call *call) {
    if(call->args != NULL) fn_args_destroy(call->args);
    free(call);
}

void program_destroy(program *program) {
    fn_call *curr = program->first, *next;
    while(curr != NULL) {
        next = curr->next;
        fn_call_destroy(curr);
        curr = next;
    }
    free(program);
}
