#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "codegen.h"
#include "utils.h"

unsigned int uses_syswrite_stdout = 0,
             uses_strlen = 0,
             uses_atoi = 0,
             uses_itoa = 0,
             uses_sbrk = 0;

asm_var *as_first = NULL, *as_last = NULL;
unsigned int asm_var_len = 0;

void add_asm_var(asm_var *as) {
    if(as_first == NULL) {
        as_first = as_last = as;
    } else {
        as_last->next = as;
        as_last = as;
    }
    asm_var_len++;
}

char *gen_asm_name() {
    char num[11];
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
    } else {
        fprintf(asm_file, "    .ascii \"");
        unsigned int i, len = strlen(as->value.string);
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
                case '\\': fputs("\\\\", asm_file);    break;
                case '\?': fputs("\\\?", asm_file);    break;
                case '\'': fputs("\\\'", asm_file);    break;
                case '\"': fputs("\\\"", asm_file);    break;
                default  : fputc(as->value.string[i], asm_file); break;
            }
        }
        fprintf(asm_file, "\\0\"\n");
    }
}

void print_all_asm_var() {
    asm_var *cur = as_first;
    while(cur != NULL) {
        print_asm_var(cur);
        cur = cur->next;
    }
}

int asm_write(program *program, unsigned int force_stdout) {
    if(force_stdout) {
        asm_file = stdout;
    } else {
        asm_file = fopen(outputs,"wb");
        if(asm_file == NULL) {
            printf("Unable to open %s for writing assembly\n",outputs);
            return 1;
        }
    }
    int status = asm_write_program(program);
    if(!force_stdout) fclose(asm_file);
    return status;
}

int asm_write_program(program *program) {
    fprintf(asm_file, ".text\n");
    fprintf(asm_file, "    .global _start\n\n");
    fprintf(asm_file, "_start:\n");
    fn_call *call = program->first;
    while(call != NULL) {
        int status = asm_write_fn_call(call);
        if(status ) return status;
        call = call->next;
    }
    return asm_write_end();
}

// write standard library + end
int asm_write_end() {
    fprintf(asm_file, "    mov $0, %%ebx\n");
    fprintf(asm_file, "    call sysexit\n");
    
    // stdlib
    // syscalls
    // sys_write
    if(uses_syswrite_stdout) {
        fprintf(asm_file, "syswrite_stdout:\n");
        fprintf(asm_file, "    mov $0x04, %%eax\n");
        fprintf(asm_file, "    mov $1, %%ebx\n");
        fprintf(asm_file, "    int $0x80\n");
        fprintf(asm_file, "    ret\n");
    }
    
    // sys_exit
    fprintf(asm_file, "sysexit:\n");
    fprintf(asm_file, "    mov $0x01, %%eax\n");
    fprintf(asm_file, "    int $0x80\n");
    fprintf(asm_file, "    ret\n");
    
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
        char *s = reserve_asm_var(12, NULL);
        
        fprintf(asm_file, "itoa:\n");
        fprintf(asm_file, "    pop %%edi\n"); // pop the call function
        fprintf(asm_file, "    pop %%eax\n"); // number to convert
        fprintf(asm_file, "    cmp $10, %%eax\n");
        fprintf(asm_file, "    jl .itoa_end\n");
        fprintf(asm_file, "    jge .itoa_loop_start\n");
        fprintf(asm_file, "    ret\n");
        
        fprintf(asm_file, ".itoa_end:\n");
        fprintf(asm_file, "    add $48, %%eax\n");
        fprintf(asm_file, "    mov $%s, %%ebx\n", s);
        fprintf(asm_file, "    mov %%eax, (%%ebx)\n");
        fprintf(asm_file, "    movb $0, 1(%%ebx)\n");
        fprintf(asm_file, "    push %%ebx\n");
        fprintf(asm_file, "    push %%edi\n"); // push back to call stack
        fprintf(asm_file, "    ret\n");
        
        fprintf(asm_file, ".itoa_loop_start:\n"); 
        fprintf(asm_file, "    mov $0, %%ecx\n"); // length of number
        fprintf(asm_file, "    jmp .itoa_loop\n");
        
        fprintf(asm_file, ".itoa_loop:\n"); // TODO
        fprintf(asm_file, "    mov $10, %%ebx\n"); // divided by 10
        fprintf(asm_file, "    mov $0, %%edx\n"); // remainder
        fprintf(asm_file, "    div %%ebx\n");
        fprintf(asm_file, "    add $48, %%edx\n"); // ord('0') == 48
        fprintf(asm_file, "    push %%edx\n");
        fprintf(asm_file, "    inc %%ecx\n"); // len++
        fprintf(asm_file, "    cmp $10, %%eax\n");
        fprintf(asm_file, "    jge .itoa_loop\n");
        fprintf(asm_file, "    add $48, %%eax\n");
        fprintf(asm_file, "    push %%eax\n");
        fprintf(asm_file, "    mov $0, %%ebx\n"); // length of string
        fprintf(asm_file, "    jmp .itoa_loop_end\n");
        
        fprintf(asm_file, ".itoa_loop_end:\n");
        // stack if "10" passed: 0 1
        fprintf(asm_file, "    pop %%edx\n");
        fprintf(asm_file, "    mov $%s, %%ebp\n", s);
        fprintf(asm_file, "    mov %%edx, (%%ebp,%%ebx)\n");
        fprintf(asm_file, "    inc %%ebx\n");
        fprintf(asm_file, "    dec %%ecx\n");
        fprintf(asm_file, "    cmp $0, %%ecx\n");
        fprintf(asm_file, "    jge .itoa_loop_end\n");
        fprintf(asm_file, "    movb $0, (%%ebp,%%ebx)\n");
        fprintf(asm_file, "    push %%ebp\n");
        fprintf(asm_file, "    push %%edi\n"); // push itoa's caller to call stack
        fprintf(asm_file, "    ret\n");
    }
    
    if(uses_atoi) {
    
    }
    
    // memory manip
    if(uses_sbrk) {
        fprintf(asm_file, "sbrk:\n");
        fprintf(asm_file, "    mov $0x2d, %%eax\n");
        fprintf(asm_file, "    mov $0, %%ebx\n");
        fprintf(asm_file, "    int $0x80\n");
        fprintf(asm_file, "    mov %%eax, %%ebx\n");
        fprintf(asm_file, "    pop %%ecx\n");
        fprintf(asm_file, "    pop %%ecx\n");
        fprintf(asm_file, "    add %%ecx, %%ebx\n");
        fprintf(asm_file, "    mov $0x2d, %%eax\n");
        fprintf(asm_file, "    int $0x80\n");
        fprintf(asm_file, "    ret\n");
    }
    
    fprintf(asm_file,".section .data\n");
    print_all_asm_var();
    return 0;
}

int asm_write_fn_call(fn_call *fn_call) {
    // TODO external calls
    fn_args *args = fn_call->args;
    deprintf("%s\n", fn_call->name);
    if(strlen(fn_call->name) == 1 &&
      (fn_call->name[0] == '+' ||
       fn_call->name[0] == '-' ||
       fn_call->name[0] == '*' ||
       fn_call->name[0] == '/' ||
       fn_call->name[0] == '%'
      )) {
        asm_write_fn_arg(args->first);
        if(args->first->next == NULL) {
            printf("Binary operator %c requires 2 arguments.\n", fn_call->name[0]);
            return 1;
        }
        asm_write_fn_arg(args->first->next);
        if(fn_call->name[0] == '/' || fn_call->name[0] == '%') {
            fprintf(asm_file, "    mov $0, %%edx\n");
            fprintf(asm_file, "    pop %%ebx\n");
            fprintf(asm_file, "    pop %%eax\n");
            fprintf(asm_file, "    div %%ebx\n");
        }
        if (fn_call->name[0] == '/') {
            fprintf(asm_file, "    push %%eax\n");
            return 0;
        } else if (fn_call->name[0] == '%') {
            fprintf(asm_file, "    push %%edx\n");
            return 0;
        } else {
            fprintf(asm_file, "    pop %%eax\n");
            fprintf(asm_file, "    pop %%ebx\n");
            if(fn_call->name[0] == '+')
                fprintf(asm_file, "    add %%ebx, %%eax\n");
            else if(fn_call->name[0] == '-')
                fprintf(asm_file, "    sub %%eax, %%ebx\n");
            else if(fn_call->name[0] == '*')
                fprintf(asm_file, "    mul %%ebx\n");
        }
        fprintf(asm_file, "    push %%eax\n");
    } else if (strcmp(fn_call->name, "define") == 0) {
        if(args->first->val->type != VALUE_IDENT_TYPE) {
            printf("define function expects identifier as first argument\n");
            return 1;
        }
        if(args->first->next == NULL) {
            printf("define function expects 2 arguments\n");
            return 1;
        }
        unsigned int type = args->first->next->val->type;
        char *name = malloc(strlen(args->first->val->val.string) + 2);
        name[0] = 'V';
        name[1] = '\0';
        strcat(name, args->first->val->val.string);
        if(type == VALUE_NINT_TYPE)
            add_asm_nint(args->first->next->val->val.nint, name);
        else if(type == VALUE_NFLOAT_TYPE)
            add_asm_nfloat(args->first->next->val->val.nfloat, name);
        else if(type == VALUE_STR_TYPE)
            add_asm_str(args->first->next->val->val.string, name);
    } else if (strcmp(fn_call->name, "println") == 0 || strcmp(fn_call->name, "print") == 0) {
        uses_syswrite_stdout = 1;
        uses_strlen = 1;
        
        // https://syscalls.kernelgrok.com/
        asm_write_fn_arg(args->first);
        // duplicate for sys_write
        fprintf(asm_file, "    push (%%esp)\n");
        
        // strlen(arg1)
        fprintf(asm_file, "    pop %%edi\n"); // memory operand
        fprintf(asm_file, "    call strlen\n");
        fprintf(asm_file, "    pop %%ecx\n");
        
        // sys_write syscall
        // syscall(0x04, fd, buf, count)
        fprintf(asm_file, "    mov %%ecx, %%edx\n"); // strlen -> count
        fprintf(asm_file, "    pop %%ecx\n"); // memory operand
        fprintf(asm_file, "    call syswrite_stdout\n");
        
        if(strcmp(fn_call->name, "println") == 0) {
            fprintf(asm_file, "    mov $%s, %%ecx\n", add_asm_str("\n", NULL));
            fprintf(asm_file, "    mov $1, %%edx\n");
            fprintf(asm_file, "    call syswrite_stdout\n");
        }
    } else {
        if      (strcmp(fn_call->name, "syswrite_stdout") == 0) uses_syswrite_stdout = 1;
        else if (strcmp(fn_call->name, "strlen") == 0)          uses_strlen = 1;
        else if (strcmp(fn_call->name, "atoi") == 0)            uses_atoi = 1;
        else if (strcmp(fn_call->name, "itoa") == 0)            uses_itoa = 1;
        else if (strcmp(fn_call->name, "syswrite_stdout") == 0) uses_syswrite_stdout = 1;
        asm_write_fn_args(fn_call->args);
        fprintf(asm_file, "    call %s\n", fn_call->name);
    }
    return 0;
}

int asm_write_fn_args(fn_args *args) {
    fn_arg *curr = args->first;
    while(curr != NULL) {
        int status = asm_write_fn_arg(curr);
        if(status) return status;
        curr = curr->next;
    }
    return 0;
}

int asm_write_fn_arg(fn_arg *arg) {
    char *s = asm_write_value(arg->val);
    if(s == NULL) {
        printf("Expected value for fn_arg\n");
        return 1;
    } else if (strlen(s) == 0) {
        return 0;
    }
    fprintf(asm_file, "    push %s\n", s);
    free(s);
    return 0;
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
            n[0] = '$';
            n[1] = '0';
            n[2] = '\0';
        } else {
            n = malloc((int)floor(log10(abs(val->val.nint))) + 2);
            sprintf(n, "$%d", val->val.nint);
            n[(int)floor(log10(abs(val->val.nint))) + 2] = '\0';
        }
        return n;
    } else if (val->type == VALUE_IDENT_TYPE) {
        char *id = malloc(strlen(val->val.string) + 3);
        id[0] = '$';
        id[1] = 'V';
        id[2] = '\0';
        strcat(id, val->val.string);
        id[strlen(val->val.string) + 2] = 0;
        return id;
    } else if (val->type == VALUE_CALL_TYPE) {
        asm_write_fn_call(val->val.fn_call);
        return "";
    }
    return NULL;
}
