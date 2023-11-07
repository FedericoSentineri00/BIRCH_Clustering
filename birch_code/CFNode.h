#ifndef CFTREE_H
#define CFTREE_H

#include "CFEntry.h"
#include "comms.h"

typedef struct {
    CFNODE *entries[numberOfEntriesPerNode]; // Array delle coordinate
} CFNODE;

CFNODE* createCFNode(CFNODE *node);
void addEntry(CFNODE *node,CFENTRY *cfentry);
void clearNode(CFNODE *node);

#endif