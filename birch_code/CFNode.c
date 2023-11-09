#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "CFNode.h"

CFNODE* createCFNode(){
    CFNODE* node = (CFNODE*)malloc(sizeof(CFNODE));
    
    node->nEntries = 0;
    return node;
}

void addPointToNode(CFNODE *node,PointND *point){
    if(node->nEntries == 0){
        node->entries[0] = (CFENTRY*)malloc(sizeof(CFENTRY));
        addPointToEntry(node->entries[0],point);
    }
    else{
        int indexClosestEntry = 0;

        PointND *centroid = (PointND*)malloc(sizeof(PointND));
        centroid->coordinates = computeCentroids(node->entries[0]->numberOfPoints,node->entries[0]->linearSum);
        double minDistance = euclideanDistance(centroid,point);
        
        freePoint(centroid);
        
        int i;
        for(i=1; i<node->nEntries;i++){
            centroid = (PointND*)malloc(sizeof(PointND));
            centroid->coordinates = computeCentroids(node->entries[i]->numberOfPoints,node->entries[i]->linearSum);
            if(euclideanDistance(centroid,point) < minDistance){
                indexClosestEntry = i;
            }
            freePoint(centroid);
        }

        bool enoughClose = false;

        PointND *centroid = (PointND*)malloc(sizeof(PointND));
        centroid->coordinates = computeCentroids(node->entries[indexClosestEntry]->numberOfPoints,node->entries[indexClosestEntry]->linearSum);

        CFENTRY *tmpEntry = createCFEntry();
        tmpEntry->numberOfPoints = node->entries[indexClosestEntry]->numberOfPoints+1;
        for(i=0;i<dimension;i++){
            tmpEntry->linearSum[i] = node->entries[indexClosestEntry]->linearSum[i] + point->coordinates[i];
            tmpEntry->squareSum[i] = node->entries[indexClosestEntry]->squareSum[i] + point->coordinates[i]*point->coordinates[i];
        }

        for(i=0;i<dimension;i++){
            if(computeDiamter(tmpEntry->numberOfPoints,tmpEntry->linearSum,tmpEntry->squareSum)>threshold[i])
                enoughClose = false;
                //BREAK NECESSARIO
        }

        if(enoughClose){
            clearEntry(node);
            node->entries[indexClosestEntry]=tmpEntry;
        }else{
            //CREO LA NUOVA ENTRY E SPOSTO TUTTO
        }

        
    }
}

void clearNode(CFNODE *node){
    int i;
    for(i=0; i<numberOfEntriesPerNode;i++){
        clearentry(node->entries[i]);
    }
    free(node);
}