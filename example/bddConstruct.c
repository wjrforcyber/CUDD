#include "../util/util.h"
#include "../include/cudd.h"
#include <stdio.h>


/**Function********************************************************************

  Synopsis    [Example of constructing a BDD.]

  Description [Simply Construct a BDD.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int main()
{
    //initialize a manager with n variables
    DdManager * manager = Cudd_Init(4,0,CUDD_UNIQUE_SLOTS,CUDD_CACHE_SLOTS,0);
    DdNode *f, *var, *tmp;
    //prepare blif file
    FILE *pFile = fopen("test_b.blif", "w");
    if (pFile == NULL) {
        perror("Error opening file");
        return 1;
    }
    //prepare dot file
    FILE *pFileDot = fopen("test_d.dot", "w");
    if (pFileDot == NULL) {
        perror("Error opening file");
        return 1;
    }

    char * modelName = "test";
    char **inputNames = (char **)malloc(sizeof(char *) * 4);
    char **outputNames = (char **)malloc(sizeof(char *) * 1);
    for(int i = 0 ; i < 4; i++)
    {
        inputNames[i] = (char *)malloc(sizeof(char) * 10);
        sprintf(inputNames[i], "in_%d", i);
    }
    outputNames[0] = (char *)malloc(sizeof(char) * 10);
    sprintf(outputNames[0], "f");

    int i;
    f = Cudd_ReadOne(manager);
    Cudd_Ref(f);
    for (i = 3; i >= 0; i--) {
        var = Cudd_bddIthVar(manager,i);
        tmp = Cudd_bddAnd(manager,Cudd_Not(var),f);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(manager,f);
        f = tmp;
    }
    //dumping blif results with input names, output names and model name
    Cudd_DumpBlif(manager, 1, &f, inputNames, outputNames,  modelName, pFile);
    fclose(pFile);
    //dump dot results
    Cudd_DumpDot(manager, 1, &f, inputNames, outputNames, pFileDot);
    fclose(pFileDot);
    //free the manager
    Cudd_Quit(manager);

    return 0;
}
