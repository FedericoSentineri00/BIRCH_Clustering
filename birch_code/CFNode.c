#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "CFNode.h"

CFNODE* createCFNode(CFNODE *node){
    CFNODE* entry = (CFNODE*)malloc(sizeof(CFNODE));
}

void addEntry(CFNODE *node,CFENTRY *cfentry){

}

void clearNode(CFNODE *node){
    int i;
    for(i=0; i<numberOfEntriesPerNode;i++){
        clearentry(node->entries[i]);
    }
    free(node);
}