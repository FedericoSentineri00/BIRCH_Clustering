#ifndef CFNODE_H
#define CFNODE_H

#include "CFEntry.h"

typedef struct {
    int nEntries;
    CFNODE *nodeFather;                     //references to father node
    CFENTRY *entries[numberOfEntriesPerNode]; //Array delle entries
    CFNODE *children[numberOfChildrenNode]; //Array dei figli
} CFNODE;

CFNODE* createCFNode();
void addPointToNode(CFNODE *node,PointND *point);
void clearNode(CFNODE *node);

#endif