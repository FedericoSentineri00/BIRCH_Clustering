#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "CFEntry.h"

CFENTRY*  createCFEntry(){
    CFENTRY* entry = (CFENTRY*)malloc(sizeof(CFENTRY));

    if (entry == NULL) {
        fprintf(stderr, "Errore nell'allocazione di memoria per il CFENTRY.\n");
        exit(1);
    }

    entry->numberOfPoints = 0;
    entry->points = createArrayList();
    
    int i;
    for (i = 0; i < dimension; i++) {
        entry->linearSum[i] = 0;
        entry->squareSum[i] = 0;
    }

    return entry;
}

void sddPoint(CFENTRY *entry,PointND *point){
    int i;
    for(i=0;i<dimension;i++){
        entry->linearSum[i]=entry->linearSum[i]+ point->coordinates[i];
        entry->squareSum[i]=entry->squareSum[i]+ (point->coordinates[i] * point->coordinates[i] );
    }
    entry->numberOfPoints++;
    add(entry->points,point);
}

void clearEntry(CFENTRY *entry){
    freeArrayList(entry->points);
    free(entry);
}