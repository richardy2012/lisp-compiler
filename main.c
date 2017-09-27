#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "codegen.h"
#include "utils.h"

struct object_file;
typedef struct object_file {
    char *file;
    char *sfile;
    char *ofile;
    char *binfile;
    struct object_file *next;
} object_file;

object_file *of_first = NULL, *of_last = NULL;
int of_length = 0;

void add_object_file(char *file) {
    object_file *of = malloc(sizeof(object_file));
    of->file = file;
    
    int i;
    
    of->sfile = malloc(strlen(file) + 2 + 1);
    strcpy(of->sfile, file);
    for( i = strlen(file); i > 0 ; i -- ){
        if(file[i] == '.') {
            of->sfile[i] = '.';
            of->sfile[i + 1] = 's';
            of->sfile[i + 2] = '\0';
            break;
        }
    }
    
    of->ofile = malloc(strlen(file) + 2 + 1);
    strcpy(of->ofile, file);
    for( i = strlen(file); i > 0 ; i -- ){
        if(file[i] == '.') {
            of->ofile[i] = '.';
            of->ofile[i + 1] = 'o';
            of->ofile[i + 2] = '\0';
            break;
        }
    }
    
    of->next = NULL;
    if(of_first == NULL) {
        of_first = of_last = of;
    } else {
        of_last->next = of;
        of_last = of;
    }
    of_length++;
}

// main
char outputf[256] = "";
char as[256] = ""; //assembler path
char ld[256] = ""; //linker path
int asm_stdout = 0;

int make_object(object_file *of) {
    char *argv_as[] = { as, "--gstabs+", "-o", of->ofile, of->sfile, NULL };
    mopen(argv_as);
    if(mopen_status) {
        printf("assembler returned with exit code %i\n", mopen_status);
        return 1;
    }
    return 0;
}

int link() {
    char *argv_ld[ 3 /* ld -o output */ + of_length /* a.o, b.o */ + 1 ];
    argv_ld[0] = ld;
    char *opt_o = "-o";
    argv_ld[1] = opt_o;
    argv_ld[2] = outputf;
    
    int ofn = 3;
    object_file *curr = of_first;
    while(curr != NULL) {
        argv_ld[ofn] = curr->ofile;
        curr = curr->next;
        ofn++;
    }
    
    argv_ld[ofn] = NULL;
    mopen(argv_ld);
    if(mopen_status) {
        printf("linker returned with exit code %i\n", mopen_status);
        return 1;
    }
    return 0;
}

int compile_lisp(object_file *of) {
    FILE *f = fopen(of->file, "r");
    if(f == NULL)
        error("Cannot open file %s\n", of->file);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    source = malloc(fsize + 1);
    fread(source, fsize, 1, f);
    fclose(f);
    source[fsize] = 0;
    
    strcpy(outputs, of->sfile);
    
    parser_reset();
    program* program = parse_program();
    if(program == NULL) {
        free(source);
        program_destroy(program);
        return 1;
    }
    
    asm_write(program, asm_stdout);
    free(source);
    program_destroy(program);
    flush_asm_var();
    return 0;
}

int make_binary() {
    object_file *curr = of_first;
    while(curr != NULL) {
        compile_lisp(curr);
        if(make_object(curr)) return 1;
        curr = curr->next;
    }
    if(!asm_lib)
        return link();
    else
        return 0;
}

int main(int argc, char *argv[]) {
    if(argc < 2)
        goto usage;
    
    int i;
    for( i = 0 ; i < argc ; i ++ ) {
        if(argv[i][0] == '-') {
            if(strcmp(argv[i], "-o") == 0) {
                if(i + 1 > argc) {
                    printf("-o requires output argument\n");
                    goto usage;
                }
                strcpy(outputf, argv[++i]);
            } else if (strcmp(argv[i], "-a") == 0)
                asm_stdout = 1;
            else if (strcmp(argv[i], "-as") == 0) {
                if(i + 1 > argc) {
                    printf("-as requires assembler argument\n");
                    goto usage;
                }
                strcpy(as, argv[++i]);
            } 
            else if (strcmp(argv[i], "-ld") == 0) {
                if(i + 1 > argc) {
                    printf("-ld requires linker argument\n");
                    goto usage;
                }
                strcpy(ld, argv[++i]);
            } else if (strcmp(argv[i], "-d") == 0)
                print_debug = 1;
            else if (strcmp(argv[i], "-lib") == 0)
                asm_lib = 1;
            else if (strcmp(argv[i], "-nostdlib") == 0)
                asm_nostdlib = 1;
            else if (strcmp(argv[i], "-h") == 0)
                goto usage;
         } else if(i > 0) {
            add_object_file(argv[i]);
         }
    }
    
    if(of_last == NULL)
        goto usage;
    
    if(outputf[0] == '\0') {
        int i;
        strcpy(outputf, of_last->file);
        for( i = strlen(of_last->file); i > 0 ; i -- ){
            if(of_last->file[i] == '.') {
                outputf[i] = '\0';
                break;
            }
        }
    }
    
    if(!strlen(as)) strcpy(as, "as");
    if(!strlen(ld)) strcpy(ld, "ld");
    
    return make_binary();

usage:
    printf("usage: %s [-a] [-as assembler] [-ld linker] [-lib] [-d] [-o output] source\n", argv[0]);
    exit(1);
}
