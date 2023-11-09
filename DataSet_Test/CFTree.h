#ifndef CFTREE_H
#define CFTREE_H

#include "CFNode.h"

typedef struct {
    CFNODE *mainNode;
    CFTREE childrenNodes[numberOfChildrenNode];
} CFTREE;

CFTREE* createCFTree();
void addPoint(CFTREE *tree,PointND *point);
void clearTree(CFTREE *tree);

#endif