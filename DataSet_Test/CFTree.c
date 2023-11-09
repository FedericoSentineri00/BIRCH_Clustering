#include "CFTree.h"

CFTREE* createCFTree(){
    CFTREE* tree = (CFTREE*)malloc(sizeof(CFTREE));
    tree->mainNode = createCFNode();
    return tree;
}

void addPoint(CFTREE *treeMain,PointND *point){
    if(treeMain->mainNode->nEntries=0){
        addPointToNode(treeMain->mainNode, point);
    }
}

void clearTree(CFTREE *tree){
    clearNode(tree->mainNode);
    int i;
    for(i = 0; i<numberOfChildrenNode; i++){
        clearTree(tree->childrenNodes[i]);
    }
}