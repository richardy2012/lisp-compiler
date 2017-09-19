/*
changelog:
18/9: parser + small subset of asm works
19/9: implemented itoa, -- total pain in the asm
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

// main
char outputf[256] = "";
char outputs[256] = ""; // .s file
char sourcef[256] = "";
char as[256] = ""; //assembler path
char ld[256] = ""; //linker path

// execute file
unsigned int mopen_stdout = 0;
int mopen_status;
void mopen(char *argv[]) {
    pid_t pid;
    int pipe = 0;
    if( (pid = fork()) < 0 ) {
        fputs("Forking error\n", stdout);
    } else if (pid == 0) { // child
        if(mopen_stdout) dup2(pipe, STDOUT_FILENO);
        if(execvp(argv[0], argv) < 0) {
            fputs(argv[0], stdout);
            if(errno == EACCES) {
                fputs(": permission denied", stdout);
            } else if (errno == ENOEXEC) {
                fputs(": can't be executed", stdout);
            } else {
                printf(": error code %i", errno);
            }
            fputc('\n',stdout);
        }
    } else { // parent
        mopen_status = 1;
        waitpid(pid, &mopen_status, 0);
    }
}

// debug functions
unsigned int print_debug = 0;
int deprintf(const char *restrict fmt, ...)
{
    if(!print_debug) return 0;
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return ret;
}

// ast
struct fn_call;
typedef struct {
    union {
        int nint;
        float nfloat;
        char *string;
        struct fn_call *fn_call;
    } val;
    unsigned int type;
} value;
#define VALUE_NINT_TYPE   0
#define VALUE_NFLOAT_TYPE 1
#define VALUE_STR_TYPE    2
#define VALUE_IDENT_TYPE  3
#define VALUE_CALL_TYPE   4
void value_destroy(value *val);

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
void fn_call_destroy(fn_call *call);

typedef struct {
    fn_call *first, *last;
} program;
void program_destroy(program *program);

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

// parser
char *source = NULL;
unsigned int parser_pos = 0;

void parser_reset() {
    parser_pos = 0;
}

inline char parser_getch() {
    if(parser_pos >= strlen(source)) return '\0';
    return source[parser_pos];
}

fn_call *parse_fn_call();
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
        fn_arg *arg = parse_fn_arg();
        if(arg == NULL) {
            free(args);
            return NULL;
        }
        last->next = arg;
        last = arg;
        if(parser_getch() != ')') break;
    }
    args->first = first;
    args->last = last;
    return args;
}

fn_call *parse_fn_call() {
    deprintf("FN_CALL\n");
    
    if(parser_getch() != '(') {
        printf("Expected ( in function call start\n");
        return NULL;
    }
    parser_pos++;
    
    char ch = parser_getch();
    char *name;
    if(isalpha(ch) || ch == '+' || ch == '-' || ch == '*' || ch == '/') {
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
    
    if(parser_getch() == ')') {
        return call;
    }
    
    call->args = parse_fn_args();
    if(call->args == NULL) {
        return call;
    }
    call->next = NULL;
    
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

program *parse_program() { // TODO
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
    }
    
    return program;
}

// compiler
FILE *asm_file = NULL;

// compiler variables
struct asm_str;
typedef struct asm_str {
    char *name;
    char *value;
    struct asm_str *next;
} asm_str;
asm_str *as_first = NULL, *as_last = NULL;
unsigned int asm_str_len = 0;

char *add_asm_str(char *value) {
    asm_str *cur = as_first;
    while(cur != NULL) {
        if(strcmp(value, cur->value) == 0) {
            return cur->name;
        }
        cur = cur->next;
    }
    
    asm_str *as = malloc(sizeof(asm_str));
    char num[11];
    sprintf(num, "%d", asm_str_len);
    as->name = malloc(strlen(num) + 2);
    as->name[0] = 'L';
    as->name[1] = '\0';
    strcat(as->name, num);
    as->name[strlen(num) + 1] = '\0';
    as->value = value;
    as->next = NULL;
    
    if(as_first == NULL) {
        as_first = as_last = as;
    } else {
        as_last->next = as;
        as_last = as;
    }
    
    asm_str_len++;
    return as->name;
}

char *reserve_asm_str(int len) {
    char str[len * 2 + 1];
    unsigned int i;
    for(i = 0; i < (len * 2); i += 2) {
        str[i] = '\\';
        str[i + 1] = '0';
    }
    str[len * 2] = '\0';
    return add_asm_str(str);
}

void flush_asm_str() {
    asm_str *curr = as_first, *next;
    unsigned int i = 0;
    while(curr != NULL && i < asm_str_len) {
        next = curr->next;
        free(curr);
        curr = next;
        i++;
    }
}

void print_asm_str(asm_str * str) {
    fprintf(asm_file, "%s:\n", str->name);
    fprintf(asm_file, "    .ascii \"");
    unsigned int i;
    for(i = 0; i < strlen(str->value); i++) {
        if(str->value[i] == '"')
            fprintf(asm_file, "\\\"");
        else
            fputc(str->value[i], asm_file);
    }
    fprintf(asm_file, "\\0\"\n");
}

void print_all_asm_str() {
    asm_str *cur = as_first;
    while(cur != NULL) {
        print_asm_str(cur);
        cur = cur->next;
    }
}

// compiler lisp->asm
int asm_write(program *program, unsigned int force_stdout);
int asm_write_program(program *program);
int asm_write_fn_call(fn_call *fn_call);
int asm_write_fn_args(fn_args *args);
int asm_write_fn_arg(fn_arg *arg);
char *asm_write_value(value *val);
int asm_write_end();

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
    int status = asm_write_fn_call(program->first);
    if(status ) return status;
    return asm_write_end();
}

// write standard library + end
unsigned int uses_syswrite_stdout = 0,
             uses_atoi = 0,
             uses_itoa = 0,
             uses_sbrk = 0;
int asm_write_end() {
    fprintf(asm_file, "    mov $0, %%ebx\n");
    fprintf(asm_file, "    call sysexit\n");
    
    // stdlib
    // syscalls
    // sys_write
    if(uses_syswrite_stdout) {
        fprintf(asm_file, "syswrite_stdout:\n");
        fprintf(asm_file, "    mov $0x04, %%eax\n");
        fprintf(asm_file, "    mov $0, %%ebx\n");
        fprintf(asm_file, "    int $0x80\n");
        fprintf(asm_file, "    ret\n");
    }
    
    // sys_exit
    fprintf(asm_file, "sysexit:\n");
    fprintf(asm_file, "    mov $0x01, %%eax\n");
    fprintf(asm_file, "    int $0x80\n");
    fprintf(asm_file, "    ret\n");
    
    // string/int manip
    if(uses_itoa) {
        char *s = reserve_asm_str(12);
        
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
    print_all_asm_str();
    return 0;
}

int asm_write_fn_call(fn_call *fn_call) {
    // TODO external calls
    fn_args *args = fn_call->args;
    deprintf("%s\n", fn_call->name);
    if(strlen(fn_call->name) == 1 && (fn_call->name[0] == '+' || fn_call->name[0] == '-' || fn_call->name[0] == '*' || fn_call->name[0] == '/')) {
        asm_write_fn_arg(args->first);
        if(args->first->next == NULL) {
            printf("Binary operator %c requires 2 arguments.\n", fn_call->name[0]);
            return 1;
        }
        asm_write_fn_arg(args->first->next);
        fprintf(asm_file, "    pop %%eax\n");
        fprintf(asm_file, "    pop %%ebx\n");
        if(fn_call->name[0] == '+')
            fprintf(asm_file, "    add %%ebx, %%eax\n");
        else if(fn_call->name[0] == '-')
            fprintf(asm_file, "    sub %%ebx, %%eax\n");
        else if(fn_call->name[0] == '*') {
        
        } else {
        
        }
        fprintf(asm_file, "    push %%eax\n");
    } else if (strcmp(fn_call->name, "println") == 0 || strcmp(fn_call->name, "print") == 0) {
        uses_syswrite_stdout = 1;
        
        // https://syscalls.kernelgrok.com/
        asm_write_fn_arg(args->first);
        // duplicate for sys_write
        fprintf(asm_file, "    push (%%esp)\n");
        
        // strlen(arg1)
        fprintf(asm_file, "    pop %%edi\n"); // memory operand
        fprintf(asm_file, "    sub %%al, %%al\n"); // comparison character
        fprintf(asm_file, "    sub %%ecx, %%ecx\n");
        fprintf(asm_file, "    not %%ecx\n");
        fprintf(asm_file, "    cld\n");
        fprintf(asm_file, "    sub %%dl, %%dl\n"); // direction
        fprintf(asm_file, "    repne scasb\n"); // repeat scasb til not eq
        fprintf(asm_file, "    not %%ecx\n");
        fprintf(asm_file, "    dec %%ecx\n");
        
        // sys_write syscall
        // syscall(0x04, fd, buf, count)
        fprintf(asm_file, "    mov %%ecx, %%edx\n"); // strlen -> count
        fprintf(asm_file, "    pop %%ecx\n"); // memory operand
        fprintf(asm_file, "    call syswrite_stdout\n");
        
        if(strcmp(fn_call->name, "println") == 0) {
            fprintf(asm_file, "    mov $%s, %%ecx\n", add_asm_str("\\n"));
            fprintf(asm_file, "    mov $1, %%edx\n");
            fprintf(asm_file, "    call syswrite_stdout\n");
        }
    } else {
        if(strcmp(fn_call->name, "atoi") == 0) uses_atoi = 1;
        else if(strcmp(fn_call->name, "itoa") == 0) uses_itoa = 1;
        else if(strcmp(fn_call->name, "syswrite_stdout") == 0) uses_syswrite_stdout = 1;
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
        char *name = add_asm_str(val->val.string);
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
    
    } else if (val->type == VALUE_CALL_TYPE) {
        asm_write_fn_call(val->val.fn_call);
        return "";
    }
    return NULL;
}

int make_binary() {
    char *obj = malloc(strlen(outputf) + 3);
    strcpy(obj, outputf);
    strcat(obj, ".o");
    obj[strlen(outputf) + 2] = '\0';
    char *argv_as[] = { "as", "-o", obj, outputs, NULL };
    mopen(argv_as);
    if(mopen_status) {
        printf("assembler returned with exit code %i\n", mopen_status);
        return 1;
    }
    char *argv_ld[] = { "ld", "-o", outputf, obj, NULL };
    mopen(argv_ld);
    if(mopen_status) {
        printf("linker returned with exit code %i\n", mopen_status);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if(argc < 2)
        goto usage;
    
    unsigned int i;
    unsigned int asm_stdout = 0;
    for( i = 0 ; i < argc ; i ++ ) {
        if(argv[i][0] == '-') {
            if(strcmp(argv[i], "-o") == 0) {
                if(i + 1 > argc) {
                    printf("-o requires output argument\n");
                    goto usage;
                }
                strcpy(outputf, argv[i + 1]);
            } else if (strcmp(argv[i], "-a") == 0)
                asm_stdout = 1;
            else if (strcmp(argv[i], "-d") == 0)
                print_debug = 1;
            else if (strcmp(argv[i], "-h") == 0)
                goto usage;
         }
    }
    strcpy(sourcef, argv[argc - 1]);
    
    strcpy(outputs, sourcef);
    for( i = strlen(sourcef); i > 0 ; i -- ){
        if(sourcef[i] == '.') {
            outputs[i + 1] = 's';
            outputs[i + 2] = '\0';
            break;
        }
    }
    deprintf("OUTPUTS: %s\n", outputs);
    
    if(outputf[0] == '\0') {
        strcpy(outputf, sourcef);
        for( i = strlen(sourcef); i > 0 ; i -- ){
            if(sourcef[i] == '.') {
                outputf[i] = '\0';
                break;
            }
        }
        deprintf("OUTPUTF: %s\n", outputs);
    }
    
    FILE *f = fopen(sourcef, "r");
    if(f == NULL) {
        printf("Cannot open file %s\n", sourcef);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    source = malloc(fsize + 1);
    fread(source, fsize, 1, f);
    fclose(f);
    source[fsize] = 0;
    
    parser_reset();
    program* program = parse_program();
    if(program == NULL) {
        free(source);
        return 1;
    }
    
    deprintf("------------\n");
    int status = asm_write(program, asm_stdout);
    if(!status) {
        status = make_binary();
    }
    
    // clean
    asm_str *as_cur = as_first, *as_next;
    while(as_cur != NULL) {
        as_next = as_cur->next;
        free(as_cur);
        as_cur = as_next;
    }
    free(source);
    program_destroy(program);
    flush_asm_str();
    return status;

usage:
    printf("usage: %s [-a] [-d] [-o output] source\n", argv[0]);
    exit(1);
}
