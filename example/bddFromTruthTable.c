#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../util/util.h"
#include "../include/cudd.h"

void print_usage(const char *prog_name) {
    printf("Usage: %s <num_vars> <hex_truth> [output_name]\n", prog_name);
    printf("Example: %s 3 E8 Majority\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    int num_vars = atoi(argv[1]);
    const char *hex_truth = argv[2];
    const char *output_name = (argc > 3) ? argv[3] : "F";
    
    if (num_vars <= 0 || num_vars > 16) {
        printf("Error: Number of variables must be 1-16\n");
        return 1;
    }
    
    /* Initialize CUDD */
    DdManager *manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, 
                                   CUDD_CACHE_SLOTS, 0);
    //Cudd_AutodynEnable(manager, CUDD_REORDER_SIFT);
    
    /* Build BDD */
    int hex_len = strlen(hex_truth);
    int num_rows = 1 << num_vars;
    int max_hex_digits = (num_rows + 3) / 4;
    
    DdNode *F = Cudd_ReadLogicZero(manager);
    Cudd_Ref(F);
    
    printf("Building BDD from:\n");
    printf("  Variables: %d\n", num_vars);
    printf("  Hex truth: %s\n", hex_truth);
    printf("  Max hex digits needed: %d\n", max_hex_digits);
    
    /* Process hex string */
    for (int hex_pos = 0; hex_pos < hex_len; hex_pos++) {
        char c = hex_truth[hex_len - 1 - hex_pos];
        int hex_val;
        
        if (c >= '0' && c <= '9') hex_val = c - '0';
        else if (c >= 'A' && c <= 'F') hex_val = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') hex_val = c - 'a' + 10;
        else {
            printf("Error: Invalid hex character '%c'\n", c);
            Cudd_RecursiveDeref(manager, F);
            Cudd_Quit(manager);
            return 1;
        }
        
        /* Process 4 bits */
        for (int bit = 0; bit < 4; bit++) {
            int minterm = hex_pos * 4 + bit;
            if (minterm >= num_rows) break;
            
            if ((hex_val >> bit) & 1) {
                /* Create cube for this minterm */
                DdNode *cube = Cudd_ReadOne(manager);
                Cudd_Ref(cube);
                
                for (int var = 0; var < num_vars; var++) {
                    DdNode *var_node = Cudd_bddIthVar(manager, var);
                    /* MSB is first variable in order */
                    if (minterm & (1 << (num_vars - var - 1))) {
                        cube = Cudd_bddAnd(manager, cube, var_node);
                    } else {
                        cube = Cudd_bddAnd(manager, cube, Cudd_Not(var_node));
                    }
                    Cudd_Ref(cube);
                }
                
                DdNode *new_F = Cudd_bddOr(manager, F, cube);
                Cudd_Ref(new_F);
                Cudd_RecursiveDeref(manager, F);
                Cudd_RecursiveDeref(manager, cube);
                F = new_F;
            }
        }
    }
    
    /* Generate variable names */
    char **inames = (char**)malloc(num_vars * sizeof(char*));
    for (int i = 0; i < num_vars; i++) {
        inames[i] = (char*)malloc(3);
        sprintf(inames[i], "x%d", i);
    }
    
    char *onames[] = { (char*)output_name };
    
    /* Create DOT file */
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_bdd.dot", output_name);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot create file %s\n", filename);
    } else {
        Cudd_DumpDot(manager, 1, &F, (char**)inames, onames, fp);
        fclose(fp);
        
        printf("\nBDD Statistics:\n");
        printf("  Number of nodes: %d\n", Cudd_DagSize(F));
        printf("  DOT file created: %s\n", filename);
        
        /* Create PNG if Graphviz is available */
        char command[200];
        snprintf(command, sizeof(command), 
                "dot -Tpng %s -o %s.png 2>/dev/null", filename, output_name);
        if (system(command) == 0) {
            printf("  PNG image created: %s.png\n", output_name);
        }
    }
    
    /* Clean up */
    for (int i = 0; i < num_vars; i++) {
        free(inames[i]);
    }
    free(inames);
    
    Cudd_RecursiveDeref(manager, F);
    Cudd_Quit(manager);
    
    return 0;
}