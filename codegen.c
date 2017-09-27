#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "codegen.h"
#include "utils.h"

static inline int is_op(char op) {
    return
        op == '+' || op == '-' ||
        op == '*' || op == '/' || op == '%' ||
        op == '=' || op == '<' || op == '>' ||
        op == '!';
}


// compiler function variables
asm_fn *asm_fn_first = NULL, *asm_fn_last = NULL;
void asm_fn_add(char *name, fn_call *fn_call) {
    asm_fn *fn = malloc(sizeof(asm_fn));
    fn->name = name;
    fn->fn_call = fn_call;
    fn->next = NULL;
    if(asm_fn_first == NULL) {
        asm_fn_first = asm_fn_last = fn;
    } else {
        asm_fn_last->next = fn;
        asm_fn_last = fn;
    }
}

// compiler string variables
asm_var *as_first = NULL, *as_last = NULL;
int asm_var_len = 0;

void add_asm_var(asm_var *as) {
    if(as_first == NULL) {
        as_first = as_last = as;
    } else {
        as_last->next = as;
        as_last = as;
    }
    asm_var_len++;
}

asm_var *get_asm_var(char *name) {
    asm_var *cur = as_first;
    while(cur != NULL) {
        if(strcmp(cur->name, name) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

char *gen_asm_name() {
    char num[12];
    sprintf(num, "%d", asm_var_len);
    char *name = malloc(strlen(num) + 2);
    name[0] = 'L';
    name[1] = '\0';
    strcat(name, num);
    name[strlen(num) + 1] = '\0';
    return name;
}
#define NAME_OR_NULL if(name == NULL) name = gen_asm_name();
char *add_asm_nint(int nint, char *name) {
    NAME_OR_NULL
    asm_var *as = malloc(sizeof(asm_var));
    as->name = name;
    as->type = ASM_VAR_NINT_TYPE;
    as->value.nint = nint;
    as->next = NULL;
    add_asm_var(as);
    return name;
}

char *add_asm_nfloat(float nfloat, char *name) {
    NAME_OR_NULL
    asm_var *as = malloc(sizeof(asm_var));
    as->name = name;
    as->type = ASM_VAR_NFLOAT_TYPE;
    as->value.nfloat = nfloat;
    as->next = NULL;
    add_asm_var(as);
    return name;
}

char *add_asm_str(char *str, char *name) {
    NAME_OR_NULL
    asm_var *as = malloc(sizeof(asm_var));
    as->name = name;
    as->type = ASM_VAR_STRING_TYPE;
    as->value.string = str;
    as->next = NULL;
    add_asm_var(as);
    return name;
}

char *reserve_asm_var(int len, char *name) {
    NAME_OR_NULL
    asm_var *as = malloc(sizeof(asm_var));
    as->name = name;
    as->type = ASM_VAR_RES_TYPE;
    as->value.reserve = len;
    as->next = NULL;
    add_asm_var(as);
    return name;
}

void flush_asm_var() {
    asm_var *as_cur = as_first, *as_next;
    while(as_cur != NULL) {
        as_next = as_cur->next;
        if(as_cur->name[0] == 'L')
            free(as_cur->name);
        free(as_cur);
        as_cur = as_next;
    }
    as_first = NULL;
    as_next = NULL;
}

void print_asm_var(asm_var *as) {
    fprintf(asm_file, "%s:\n", as->name);
    if(as->type == ASM_VAR_RES_TYPE) {
        fprintf(asm_file, "    .space %i\n", as->value.reserve);
    } else if (as->type == ASM_VAR_STRING_TYPE) {
        fprintf(asm_file, "    .ascii \"");
        int i, len = strlen(as->value.string);
        for(i = 0; i < len; i++) {
            switch (as->value.string[i]) {
                case '\0': fputs("\\0", asm_file);     break;
                case '\a': fputs("\\a", asm_file);     break;
                case '\b': fputs("\\b", asm_file);     break;
                case '\f': fputs("\\f", asm_file);     break;
                case '\n': fputs("\\n", asm_file);     break;
                case '\r': fputs("\\r", asm_file);     break;
                case '\t': fputs("\\t", asm_file);     break;
                case '\v': fputs("\\v", asm_file);     break;
                case '\\': fputs("\\", asm_file);    break;
                case '\?': fputs("\\\?", asm_file);    break;
                case '\'': fputs("\\\'", asm_file);    break;
                case '\"': fputs("\\\"", asm_file);    break;
                default  : fputc(as->value.string[i], asm_file); break;
            }
        }
        fprintf(asm_file, "\\0\"\n");
    } else if(as->type == ASM_VAR_NINT_TYPE) {
        fprintf(asm_file, "    .long %i\n", as->value.nint);
    } else {
        error("Unrecognized asm_var type %i\n", as->type);
    }
}

void print_all_asm_var() {
    asm_var *cur = as_first;
    while(cur != NULL) {
        print_asm_var(cur);
        cur = cur->next;
    }
}

char *scope = NULL;
char *asm_get_variable_name(char *name) {
    char *vname = malloc(1 + (scope == NULL ? 0 : strlen(scope)) + 1 + strlen(name) + 1);
    strcpy(vname, "V");
    if(scope != NULL)
        strcat(vname, scope);
    strcat(vname, "_");
    strcat(vname, name);
    return vname;
}

// write main program
void asm_write(program *program, int force_stdout) {
    if(force_stdout) {
        asm_file = stdout;
    } else {
        asm_file = fopen(outputs,"wb");
        if(asm_file == NULL) {
            error("Unable to open %s for writing assembly\n",outputs);
        }
    }
    asm_write_program(program);
    if(!force_stdout) fclose(asm_file);
}

void asm_write_program(program *program) {
    if(!asm_lib) {
        fprintf(asm_file, ".text\n");
        fprintf(asm_file, "    .global _start\n\n");
        fprintf(asm_file, "_start:\n");
    }
    fn_call *call = program->first;
    while(call != NULL) {
        asm_write_fn_call(call, 1);
        call = call->next;
    }
    asm_write_end(program);
}

// write standard library + end

int uses_syswrite_stdout = 0,
    uses_strlen = 0,
    uses_abs = 0,
    uses_atoi = 0,
    uses_itoa = 0,
    uses_malloc = 0,
    uses_printc = 0;

int block_is_in_defun = 0;

// swap top variables
void asm_write_swap2() { // also used reverse
    fprintf(asm_file, "    pop %%eax\n"); // pop the caller function
    fprintf(asm_file, "    pop %%ebx\n"); // pop the argument
    // swap top 2 elements of array
    fprintf(asm_file, "    push %%eax\n"); // caller function (bottom)
    fprintf(asm_file, "    push %%ebx\n"); // argument (top)
}

void asm_write_defun(fn_call *defun_call) {
    fn_args *args = defun_call->args;
    fprintf(asm_file, "%s:\n", args->first->val->val.string);

    // pop the caller -> ($Dcaller)
    fprintf(asm_file, "    pop %%edi\n");
    
    scope = malloc(strlen(args->first->val->val.string) + 1);
    strcpy(scope, args->first->val->val.string);
    
    array_item *item = args->first->next->val->val.array->first;
    while(item != NULL) {
        if(item->val->type != VALUE_CALL_TYPE || item->val->val.fn_call->args->first == NULL) {
            error("defun expects an array of arguments ((type name) ...)\n");
        }
        char *vname = asm_get_variable_name(item->val->val.fn_call->args->first->val->val.string);
        if(strcmp(item->val->val.fn_call->name, "int") == 0 || strcmp(item->val->val.fn_call->name, "str") == 0) {
            add_asm_nint(0, vname);
        } else if(strcmp(item->val->val.fn_call->name, "float") == 0) {
            add_asm_nfloat(0, vname);
        } else {
            error("Unknown type name %s\n", item->val->val.fn_call->name);
        }
        item = item->next;
    }
    
    array_item *curr = args->first->next->val->val.array->last;
    while(curr != NULL) {
        fn_call *call = curr->val->val.fn_call;
        char *vname = asm_get_variable_name(call->args->first->val->val.string);
        // (second argument of function call)
        asm_var *as = get_asm_var(vname);
        if(as == NULL)
            error("defun: variable %s doesn't exist\n", vname);
        fprintf(asm_file, "    mov $%s, %%eax\n", vname);
        fprintf(asm_file, "    pop (%%eax)\n");
        curr = curr->prev;
    }
    
    fprintf(asm_file, "    push %%edi\n");

    block_is_in_defun = 1;
    asm_write_fn_arg(args->first->next->next);
    block_is_in_defun = 0;
    
    //fprintf(asm_file, "    push $0\n"); // TODO only if no return

    free(scope);
    scope = NULL;
    
    asm_write_swap2();
    fprintf(asm_file, "    ret\n");
}

void asm_write_end(program *program) {
    if(!asm_lib) {
        fprintf(asm_file, "    mov $0, %%ebx\n");
        fprintf(asm_file, "    call sysexit\n");
    }
    // end _start
    
    asm_fn *call = asm_fn_first;
    while(call != NULL) {
        asm_write_defun(call->fn_call);
        call = call->next;
    }
    
    // stdlib
    if(!asm_nostdlib) {
        // syscalls
        // sys_write
        if(uses_printc) {
            uses_syswrite_stdout = 1;
            fprintf(asm_file, "printc:\n");
            asm_write_swap2();
            fprintf(asm_file, "    pop %%ecx\n");
            fprintf(asm_file, "    mov $1, %%edx\n");
            asm_write_swap2();
            fprintf(asm_file, "    call syswrite_stdout\n");
            fprintf(asm_file, "    ret\n");
        }
        
        if(uses_syswrite_stdout) {
            fprintf(asm_file, "syswrite_stdout:\n");
            fprintf(asm_file, "    mov $0x04, %%eax\n");
            fprintf(asm_file, "    mov $1, %%ebx\n");
            fprintf(asm_file, "    int $0x80\n");
            fprintf(asm_file, "    ret\n");
        }
        
        // sys_exit
        if(!asm_lib) {
            fprintf(asm_file, "sysexit:\n");
            fprintf(asm_file, "    mov $0x01, %%eax\n");
            fprintf(asm_file, "    int $0x80\n");
            fprintf(asm_file, "    ret\n");
        }
        
        // string manip
        if(uses_strlen) {
            fprintf(asm_file, "strlen: \n");
            fprintf(asm_file, "    pop %%esi\n");
            fprintf(asm_file, "    sub %%al, %%al\n"); // comparison character for EOF '\0'
            fprintf(asm_file, "    sub %%ecx, %%ecx\n");
            fprintf(asm_file, "    not %%ecx\n");
            fprintf(asm_file, "    cld\n");
            fprintf(asm_file, "    sub %%dl, %%dl\n"); // direction
            fprintf(asm_file, "    repne scasb\n"); // repeat scasb til not eq
            fprintf(asm_file, "    not %%ecx\n");
            fprintf(asm_file, "    dec %%ecx\n");
            fprintf(asm_file, "    push %%ecx\n");
            fprintf(asm_file, "    push %%esi\n");
            fprintf(asm_file, "    ret\n");
        }
        
        // string <-> int manip
        if(uses_itoa) {
            uses_abs = 1;
            char *s = reserve_asm_var(12, NULL);
            
            fprintf(asm_file, "itoa:\n");
            asm_write_swap2();
            fprintf(asm_file, "    pushl (%%esp)\n");
            fprintf(asm_file, "    call abs\n");
            fprintf(asm_file, "    popl %%eax\n");
            fprintf(asm_file, "    cmp $10, %%eax\n");
            fprintf(asm_file, "    jl .itoa_end\n");
            fprintf(asm_file, "    jge .itoa_loop_start\n");
            fprintf(asm_file, "    ret\n");
            
            fprintf(asm_file, ".itoa_end:\n");
            fprintf(asm_file, "    add $48, %%eax\n");
            fprintf(asm_file, "    mov $%s, %%ebx\n", s);
            fprintf(asm_file, "    popl %%ecx\n");
            fprintf(asm_file, "    cmpl $0, %%ecx\n");
            fprintf(asm_file, "    jge .itoa_end_2\n");
            fprintf(asm_file, ".itoa_end_1:\n");
            fprintf(asm_file, "    movb $%i, (%%ebx)\n", '-');
            fprintf(asm_file, "    mov %%eax, 1(%%ebx)\n");
            fprintf(asm_file, "    movb $0, 2(%%ebx)\n");
            fprintf(asm_file, "    jmp .itoa_end_3\n");
            fprintf(asm_file, ".itoa_end_2:\n");
            fprintf(asm_file, "    mov %%eax, (%%ebx)\n");
            fprintf(asm_file, "    movb $0, 1(%%ebx)\n");
            // jmp itoa_end_3
            fprintf(asm_file, ".itoa_end_3:\n");
            fprintf(asm_file, "    push %%ebx\n");
            asm_write_swap2();
            fprintf(asm_file, "    ret\n");
            
            fprintf(asm_file, ".itoa_loop_start:\n");
            fprintf(asm_file, "    mov $0, %%ecx\n"); // length of number
            fprintf(asm_file, "    pop %%edx\n");
            fprintf(asm_file, "    cmpl $0, %%edx\n");
            fprintf(asm_file, "    jl .itoa_loop_start_2\n");
            fprintf(asm_file, "    mov $0, %%ebp\n");
            fprintf(asm_file, "    jmp .itoa_loop\n");
            fprintf(asm_file, ".itoa_loop_start_2:\n");
            fprintf(asm_file, "    mov $1, %%ebp\n");
            fprintf(asm_file, "    jmp .itoa_loop\n");
            fprintf(asm_file, ".itoa_loop:\n");
            fprintf(asm_file, "    mov $10, %%ebx\n"); // divided by 10
            fprintf(asm_file, "    mov $0, %%edx\n"); // remainder
            fprintf(asm_file, "    divl %%ebx\n");
            fprintf(asm_file, "    add $48, %%edx\n"); // ord('0') == 48
            fprintf(asm_file, "    push %%edx\n");
            fprintf(asm_file, "    inc %%ecx\n"); // len++
            fprintf(asm_file, "    cmpl $10, %%eax\n");
            fprintf(asm_file, "    jge .itoa_loop\n");
            fprintf(asm_file, "    addl $48, %%eax\n");
            fprintf(asm_file, "    pushl %%eax\n");
            fprintf(asm_file, "    mov $0, %%ebx\n"); // length of string
            fprintf(asm_file, "    jmp .itoa_loop_end_start\n");
            
            fprintf(asm_file, ".itoa_loop_end_start:\n");
            fprintf(asm_file, "    cmp $0, %%ebp\n");
            fprintf(asm_file, "    jg .itoa_loop_end_1\n");
            fprintf(asm_file, "    jmp .itoa_loop_end\n");
            fprintf(asm_file, ".itoa_loop_end_1:\n");
            fprintf(asm_file, "    pushl $'-'\n");
            fprintf(asm_file, "    inc %%ecx\n");
            fprintf(asm_file, "    jmp .itoa_loop_end\n");
            fprintf(asm_file, ".itoa_loop_end:\n");
            // stack if "10" passed: 0 1
            fprintf(asm_file, "    popl %%edx\n");
            fprintf(asm_file, "    mov $%s, %%ebp\n", s);
            fprintf(asm_file, "    mov %%edx, (%%ebp,%%ebx)\n");
            fprintf(asm_file, "    inc %%ebx\n");
            fprintf(asm_file, "    dec %%ecx\n");
            fprintf(asm_file, "    cmpl $0, %%ecx\n");
            fprintf(asm_file, "    jge .itoa_loop_end\n");
            fprintf(asm_file, "    movb $0, (%%ebp,%%ebx)\n");
            fprintf(asm_file, "    push %%ebp\n");
            asm_write_swap2();
            fprintf(asm_file, "    ret\n");
        }
        
        if(uses_atoi) {
        
        }
        
        // int manip
        if(uses_abs) {
            fprintf(asm_file, "abs:\n");
            asm_write_swap2();
            fprintf(asm_file, "    popl %%eax\n"); // number to convert
            fprintf(asm_file, "    movl %%eax, %%ebx\n");
            fprintf(asm_file, "    neg %%eax\n");
            fprintf(asm_file, "    cmovl %%ebx, %%eax\n");
            fprintf(asm_file, "    pushl %%eax\n");
            asm_write_swap2();
            fprintf(asm_file, "    ret\n");
        }
    }
    
    fprintf(asm_file,".section .data\n");
    print_all_asm_var();
}

int asm_label_length = 0;
void asm_write_fn_call(fn_call *fn_call, int is_top_fn) {
    // TODO external calls
    fn_args *args = fn_call->args;
    deprintf("%s\n", fn_call->name);
    
    // keywords, operators
    if( (strlen(fn_call->name) == 1 && is_op(fn_call->name[0])) ||
        strcmp(fn_call->name, ">>>") == 0 || strcmp(fn_call->name, "<<<") == 0 ||
        strcmp(fn_call->name, "and") == 0 || strcmp(fn_call->name, "or") == 0 ||
        strcmp(fn_call->name, "xor") == 0 ||
        strcmp(fn_call->name, "!=") == 0 ) {
        if(is_top_fn) return;
        char operator = fn_call->name[0];
        asm_write_fn_arg(args->first);
        if(operator == '-' && args->first->next == NULL) { // negative function
            fprintf(asm_file, "    popl %%eax\n");
            fprintf(asm_file, "    negl %%eax\n");
            fprintf(asm_file, "    pushl %%eax\n");
            return;
        }
        if(args->first->next == NULL)
            error("Binary operator %c requires 2 arguments.\n", operator);
        asm_write_fn_arg(args->first->next);
        if(strcmp(fn_call->name, ">>>") == 0 || strcmp(fn_call->name, "<<<") == 0) {
            // TODO this doesnt work
            fprintf(asm_file, "    popl %%eax\n");
            fprintf(asm_file, "    popl %%ebx\n");
            fprintf(asm_file, "    movb %%bl, %%cl\n"); // lowest 8 bit of ebx
            if(strcmp(fn_call->name, ">>>") == 0) {
                fprintf(asm_file, "    sarl %%cl, %%eax\n");
            } else {
                fprintf(asm_file, "    sall %%cl, %%eax\n");
            }
            fprintf(asm_file, "    push %%eax\n");
            return;
        }
        fprintf(asm_file, "    popl %%ebx\n");
        fprintf(asm_file, "    popl %%eax\n");
        if(operator == '/' || operator == '%') {
            fprintf(asm_file, "    xorl %%edx, %%edx\n");
            fprintf(asm_file, "    cdq\n");
            fprintf(asm_file, "    idivl %%ebx\n");
        }
        if (operator == '/') {
            fprintf(asm_file, "    pushl %%eax\n");
            return;
        } else if (operator == '%') {
            fprintf(asm_file, "    pushl %%edx\n");
            return;
        } else if(operator == '+') {
            fprintf(asm_file, "    addl %%ebx, %%eax\n");
            fn_arg *curr = args->first->next->next;
            while(curr != NULL) {
                asm_write_fn_arg(curr);
                fprintf(asm_file, "    popl %%ebx\n");
                fprintf(asm_file, "    addl %%ebx, %%eax\n");
                curr = curr->next;
            }
        } else if(operator == '-')
            fprintf(asm_file, "    subl %%ebx, %%eax\n");
        else if(operator == '*') {
            fprintf(asm_file, "    imul %%ebx\n");
            fn_arg *curr = args->first->next->next;
            while(curr != NULL) {
                asm_write_fn_arg(curr);
                fprintf(asm_file, "    popl %%ebx\n");
                fprintf(asm_file, "    imul %%ebx\n");
                curr = curr->next;
            }
        } else if(strcmp(fn_call->name, "!=") == 0 || operator == '=' || operator == '<' || operator == '>') {
            fprintf(asm_file, "    cmpl %%eax, %%ebx\n");
            // https://stackoverflow.com/questions/27284895/how-to-compare-a-signed-value-and-an-unsigned-value-in-x86-assembly
            if(operator == '=') {
                fprintf(asm_file, "    sete %%al\n");
            } else if (strcmp(fn_call->name, "!=") == 0) {
                fprintf(asm_file, "    setne %%al\n");
            } else if(operator == '>') {
                fprintf(asm_file, "    setl %%al\n");
            } else if (operator == '<') {
                fprintf(asm_file, "    setg %%al\n");
            }
            fprintf(asm_file, "    movzb %%al, %%eax\n");
            fprintf(asm_file, "    push %%eax\n");
            return;
        } else if (strcmp(fn_call->name, "and") == 0 ||
                   strcmp(fn_call->name, "or") == 0  ||
                   strcmp(fn_call->name, "xor") == 0 ) {
            fprintf(asm_file, "    %sl %%ebx, %%eax\n", fn_call->name);
            fn_arg *curr = args->first->next->next;
            while(curr != NULL) {
                asm_write_fn_arg(curr);
                fprintf(asm_file, "    popl %%ebx\n");
                fprintf(asm_file, "    %sl %%ebx, %%eax\n", fn_call->name);
                curr = curr->next;
            }
            return;
        } else
            error("Unrecognized operator %c\n", operator);
        fprintf(asm_file, "    pushl %%eax\n");
    } else if (
    strcmp(fn_call->name, "define")    == 0 ||
    strcmp(fn_call->name, "defint")    == 0 ||
    strcmp(fn_call->name, "defptr")    == 0 || // syntatic sugar
    strcmp(fn_call->name, "deffloat")  == 0 ||
    strcmp(fn_call->name, "defstr")    == 0
    ) {
        if(args->first->next == NULL)
            error("define function expects 2 arguments\n");
        if(args->first->val->type != VALUE_IDENT_TYPE)
            error("define function expects identifier as first argument\n");
        #define EXPECT_TYPE_BY_NAME(x,y,z) if(strcmp(fn_call->name, x) == 0 && args->first->next->val->type != y && \
                                           args->first->next->val->type != VALUE_CALL_TYPE) \
            {   error("%s expects %s type for value %s\n", x, z, args->first->val->val.string); return; }
        EXPECT_TYPE_BY_NAME("defint", VALUE_NINT_TYPE, "integer");
        EXPECT_TYPE_BY_NAME("defptr", VALUE_NINT_TYPE, "integer");
        EXPECT_TYPE_BY_NAME("deffloat", VALUE_NFLOAT_TYPE, "float");
        EXPECT_TYPE_BY_NAME("defstr", VALUE_STR_TYPE, "string");
        
        int type = args->first->next->val->type;
        char *name = asm_get_variable_name(args->first->val->val.string);
        deprintf("Var %s\n", name);
        asm_var *as = get_asm_var(name);
        if(as == NULL) {
            if(type == VALUE_NINT_TYPE)
                add_asm_nint(args->first->next->val->val.nint, name);
            else if(type == VALUE_NFLOAT_TYPE)
                add_asm_nfloat(args->first->next->val->val.nfloat, name);
            else if(type == VALUE_STR_TYPE)
                add_asm_str(args->first->next->val->val.string, name);
            else if(type == VALUE_CALL_TYPE) {
                add_asm_nint(0, name);
                asm_write_fn_arg(args->first->next);
                fprintf(asm_file, "    pop %%eax\n");
                fprintf(asm_file, "    mov $%s, %%ebx\n", name);
                fprintf(asm_file, "    mov %%eax, (%%ebx)\n");
            }
            else
                error("Unsupported asm value type: %i\n", type);
        } else { // redefinition
            #define EXPECT_TYPE(x,y,z) if(as->type == x && args->first->next->val->type != y && \
                                          args->first->next->val->type != VALUE_CALL_TYPE) \
            error("Expected %s type for value %s on redefinition\n", z, args->first->val->val.string);
            deprintf("NAME:%s\n", name);
            EXPECT_TYPE(ASM_VAR_NINT_TYPE, VALUE_NINT_TYPE, "integer");
            EXPECT_TYPE(ASM_VAR_NFLOAT_TYPE, VALUE_NFLOAT_TYPE, "float");
            EXPECT_TYPE(ASM_VAR_STRING_TYPE, VALUE_STR_TYPE, "string");
            if(args->first->next->val->type == VALUE_CALL_TYPE)
                asm_write_fn_arg(args->first->next);
            if(as->type == ASM_VAR_NINT_TYPE) {
                fprintf(asm_file, "    mov $%s, %%ebp\n", name);
                if(args->first->next->val->type == VALUE_CALL_TYPE) {
                    fprintf(asm_file, "    pop %%eax\n");
                    fprintf(asm_file, "    mov %%eax, 0(%%ebp)\n");
                } else {
                    fprintf(asm_file, "    movl $%i, 0(%%ebp)\n", args->first->next->val->val.nint);
                }
            } else if (as->type == ASM_VAR_NFLOAT_TYPE) {
                // TODO
            } else if (as->type == ASM_VAR_STRING_TYPE) {
                // TODO
                if(strlen(as->value.string) > strlen(args->first->next->val->val.string)) {
                    error("Cannot redefine a static string to a bigger string.\n");
                }
            }
            free(name);
        }
    } else if (strcmp(fn_call->name, "resvstr") == 0) {
        if(args->first == NULL || args->first->next == NULL)
            error("resvstr requires 2 arguments\n");
        if(args->first->val->type != VALUE_IDENT_TYPE)
            error("resvstr requires identifier argument\n");
        if(args->first->next->val->type != VALUE_NINT_TYPE)
            error("resvstr requires integer argument\n");
        char *name = asm_get_variable_name(args->first->val->val.string);
        reserve_asm_var(args->first->next->val->val.nint, name);
    } else if (strcmp(fn_call->name, "defun") == 0) {
        fn_args *args = fn_call->args;
        if(args->first == NULL || args->first->next == NULL || args->first->next->next == NULL)
            error("defun expects at least 3 arguments\n");
        if(args->first->next->val->type != VALUE_ARRAY_TYPE)
            error("defun expects an array of arguments ((type name) ...)\n");
        asm_fn_add(args->first->val->val.string, fn_call);
    } else if (strcmp(fn_call->name, "if") == 0 || strcmp(fn_call->name, "while") == 0) {
        if(args->first == NULL || args->first->next == NULL)
            error("%s expects at least 2 function arguments\n", fn_call->name);
        int label_then = asm_label_length++,
            label_end = asm_label_length++,
            label_else = 0,
            label_condition = 0;
        if(args->first->next->next != NULL) {
            label_else = asm_label_length++;
        }
        if(strcmp(fn_call->name, "while") == 0) {
            label_condition = asm_label_length++;
            fprintf(asm_file, "l%i:\n", label_condition);
        }
        asm_write_fn_arg(args->first);
        fprintf(asm_file, "    pop %%eax\n");
        fprintf(asm_file, "    cmp $0, %%eax\n");
        fprintf(asm_file, "    jne l%i\n", label_then);
        if(args->first->next->next != NULL) {
            fprintf(asm_file, "    je l%i\n", label_else);
        }
        fprintf(asm_file, "    jmp l%i\n", label_end);
        fprintf(asm_file, "l%i:\n", label_then);
        asm_write_fn_arg(args->first->next);
        if(strcmp(fn_call->name, "while") == 0)
            fprintf(asm_file, "    jmp l%i\n", label_condition);
        else
            fprintf(asm_file, "    jmp l%i\n", label_end);
        if(args->first->next->next != NULL) {
            fprintf(asm_file, "l%i:\n", label_else);
            asm_write_fn_arg(args->first->next->next);
            fprintf(asm_file, "    jmp l%i\n", label_end);
        }
        fprintf(asm_file, "l%i:\n", label_end);
    } else if (strcmp(fn_call->name, "asm") == 0) {
         if(args->first == NULL || args->first->val->type != VALUE_STR_TYPE)
            error("asm function expects string argument\n");
        
        fputs("    ", asm_file);
        char *str = args->first->val->val.string;
        int i;
        for(i = 0; i < strlen(str); i++) {
            if(str[i] == '#') {
                i++;
                fputc('$', asm_file);
                char *var = calloc(1,2);
                int ssize = 0;
                while (i < strlen(str)) {
                    var = realloc(var, ssize + 1);
                    var[ssize] = str[i];
                    ssize++;
                    i++;
                    if(str[i] == '#') break;
                }
                var[ssize] = '\0';
                char *name = asm_get_variable_name(var);
                fputs(name, asm_file);
                free(var);
                free(name);
            } else
                fputc(str[i], asm_file);
        }
        fputc('\n', asm_file);
    } else if (strcmp(fn_call->name, "setptr") == 0) {
        if(args->first == NULL || args->first->next == NULL)
            error("ptr function expects 2 arguments\n");
        asm_write_fn_arg(args->first->next);
        asm_write_fn_arg(args->first);
        fprintf(asm_file, "    pop %%eax\n");
        fprintf(asm_file, "    pop (%%eax)\n");
    } else if (strcmp(fn_call->name, "ptr") == 0) {
        if(args->first == NULL)
            error("ptr function expects variable\n");
        asm_write_fn_arg(args->first);
        fprintf(asm_file, "    pop %%eax\n");
        fprintf(asm_file, "    push (%%eax)\n");
    } else if (strcmp(fn_call->name, "addr") == 0) {
        if(args->first == NULL || args->first->val->type != VALUE_IDENT_TYPE)
            error("addr function expects identifier variable\n");
        fprintf(asm_file, "    push $V%s_%s\n", scope == NULL ? "" : scope, args->first->val->val.string);
    } else if(strcmp(fn_call->name, "at") == 0) {
        if(args->first == NULL || args->first->next == NULL)
            error("ptr function expects 2 arguments\n");
        asm_write_fn_arg(args->first->next);
        asm_write_fn_arg(args->first);
        fprintf(asm_file, "    pop %%eax\n");
        fprintf(asm_file, "    pop %%ebx\n");
        fprintf(asm_file, "    push (%%ebx,%%eax)\n");
    } else if(strcmp(fn_call->name, "return") == 0) {
        asm_write_fn_arg(args->first);
        if(!block_is_in_defun)
            error("Cannot return on a non-function block\n");
        asm_write_swap2();
        fprintf(asm_file, "    ret\n");
    }
    
    // stdlib
    else if (!asm_nostdlib && (strcmp(fn_call->name, "println") == 0 || strcmp(fn_call->name, "print") == 0)) {
        uses_syswrite_stdout = 1;
        uses_strlen = 1;
        
        // https://syscalls.kernelgrok.com/
        asm_write_fn_arg(args->first);
        fprintf(asm_file, "    push (%%esp)\n"); // duplicate for sys_write
        
        // strlen(arg1)
        fprintf(asm_file, "    pop %%edi\n"); // memory operand
        fprintf(asm_file, "    call strlen\n");
        fprintf(asm_file, "    pop %%ecx\n");
        
        // sys_write syscall
        // syscall(0x04, fd, buf, count)
        fprintf(asm_file, "    mov %%ecx, %%edx\n"); // strlen -> count
        fprintf(asm_file, "    pop %%ecx\n"); // memory operand
        
        if(strcmp(fn_call->name, "println") == 0) {
            fprintf(asm_file, "    push %%ecx\n"); // duplicate for newline
            fprintf(asm_file, "    push %%edx\n");
            fprintf(asm_file, "    movb $'\\n', (%%edx,%%ecx)\n");
            fprintf(asm_file, "    inc %%edx\n");
        }
        
        fprintf(asm_file, "    call syswrite_stdout\n");
        
        if(strcmp(fn_call->name, "println") == 0) {
            fprintf(asm_file, "    pop %%edx\n");
            fprintf(asm_file, "    pop %%ecx\n");
            fprintf(asm_file, "    movb $0, (%%edx,%%ecx)\n");
        }
    }
    
    // function calls
    else {
        if      (strcmp(fn_call->name, "syswrite_stdout") == 0) uses_syswrite_stdout = 1;
        else if (strcmp(fn_call->name, "printc") == 0)          uses_printc = 1;
        else if (strcmp(fn_call->name, "strlen") == 0)          uses_strlen = 1;
        else if (strcmp(fn_call->name, "atoi") == 0)            uses_atoi = 1;
        else if (strcmp(fn_call->name, "itoa") == 0)            uses_itoa = 1;
        asm_write_fn_args(fn_call->args);
        fprintf(asm_file, "    call %s\n", fn_call->name);
        if(is_top_fn) fprintf(asm_file, "    pop %%eax\n");
    }

    return;
}

void asm_write_fn_args(fn_args *args) {
    fn_arg *curr = args->first;
    while(curr != NULL) {
        asm_write_fn_arg(curr);
        curr = curr->next;
    }
    return;
}

void asm_write_fn_arg(fn_arg *arg) {
    return asm_write_arg(arg->val);
}

void asm_write_arg(value *val) {
    char *s = asm_write_value(val);
    if(s == NULL) {
        error("Expected value for fn_arg\n");
    } else if (strlen(s) == 0) {
        return;
    }
    if(val->type == VALUE_IDENT_TYPE) {
        char *id = malloc((scope == NULL ? 0 : strlen(scope) + 1) + strlen(val->val.string) + 2);
        strcpy(id, "V");
        if(scope != NULL)
            strcat(id, scope);
        strcat(id, "_");
        strcat(id, val->val.string);
        asm_var *as = get_asm_var(id);
        if(as == NULL) {
            error("Variable %s does not exist\n", id);
        }
        if(as->type == ASM_VAR_NINT_TYPE || as->type == ASM_VAR_NFLOAT_TYPE) {
             fprintf(asm_file, "    mov %s, %%edx\n", s);
             fprintf(asm_file, "    pushl (%%edx)\n");
        } else {
            fprintf(asm_file, "    push %s\n", s);
        }
        free(id);
    } else if (val->type == VALUE_NINT_TYPE)
        fprintf(asm_file, "    pushl %s\n", s);
    else
        fprintf(asm_file, "    push %s\n", s);
    free(s);
    return;
}

char *asm_write_value(value *val) {
    deprintf("%i\n", val->type);
    if(val->type == VALUE_STR_TYPE) {
        char *name = add_asm_str(val->val.string, NULL);
        char *neo = malloc(strlen(name) + 1);
        neo[0] = '$';
        neo[1] = '\0';
        strcat(neo, name);
        return neo;
    } else if (val->type == VALUE_NINT_TYPE) {
        char *n;
        if(val->val.nint == 0) {
            n = malloc(3);
            strcpy(n, "$0");
        } else {
            n = malloc((int)floor(log10(abs(val->val.nint))) + 2);
            sprintf(n, "$%i", val->val.nint);
        }
        return n;
    } else if (val->type == VALUE_IDENT_TYPE) {
        char *name;
        // boolean keywords
        if(strcmp(val->val.string, "false") == 0) {
            name = malloc(3);
            strcpy(name, "$0");
        } else if(strcmp(val->val.string, "true") == 0) {
            name = malloc(3);
            strcpy(name, "$1");
        }
        // actual identifiers
        else {
            name = malloc(2 + (scope == NULL ? 0 : strlen(scope)) + 1 + strlen(val->val.string) + 2);
            strcpy(name, "$V");
            if(scope != NULL)
                strcat(name, scope);
            strcat(name, "_");
            strcat(name, val->val.string);
            return name;
        }
    } else if (val->type == VALUE_CALL_TYPE) {
        asm_write_fn_call(val->val.fn_call, 0);
        return "";
    } else if (val->type == VALUE_BLOCK_TYPE) {
        fn_call *cur = val->val.block->first;
        while(cur != NULL) {
            asm_write_fn_call(cur, 1);
            cur = cur->next;
        }
        return "";
    }
    return NULL;
}
