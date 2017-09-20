#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.h"
#include "codegen.h"
#include "utils.h"

// main
char outputf[256] = "";
char sourcef[256] = "";
char as[256] = ""; //assembler path
char ld[256] = ""; //linker path

int make_binary() {
    char *obj = malloc(strlen(outputf) + 3);
    strcpy(obj, outputf);
    strcat(obj, ".o");
    obj[strlen(outputf) + 2] = '\0';
    char *argv_as[] = { as, "-o", obj, outputs, NULL };
    mopen(argv_as);
    if(mopen_status) {
        printf("assembler returned with exit code %i\n", mopen_status);
        return 1;
    }
    char *argv_ld[] = { ld, "-o", outputf, obj, NULL };
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
            else if (strcmp(argv[i], "-as") == 0) {
                if(i + 1 > argc) {
                    printf("-as requires assembler argument\n");
                    goto usage;
                }
                strcpy(as, argv[i + 1]);
            } 
            else if (strcmp(argv[i], "-ld") == 0) {
                if(i + 1 > argc) {
                    printf("-ld requires linker argument\n");
                    goto usage;
                }
                strcpy(ld, argv[i + 1]);
            } else if (strcmp(argv[i], "-d") == 0)
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
    
    if(!strlen(as)) strcpy(as, "as");
    if(!strlen(ld)) strcpy(ld, "ld");
    
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
    free(source);
    program_destroy(program);
    flush_asm_str();
    return status;

usage:
    printf("usage: %s [-a] [-as assembler] [-ld linker] [-d] [-o output] source\n", argv[0]);
    exit(1);
}
