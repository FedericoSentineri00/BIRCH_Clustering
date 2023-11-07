#ifndef CFENTRY_H
#define CFENTRY_H

#include "arraylist.h"
#include "comms.h"

typedef struct {
    int numberOfPoints;      // Numero di dimensioni
    int linearSum[dimension];
    int squareSum[dimension];
    ArrayList *points; // Array delle coordinate
} CFENTRY;

CFENTRY* createCFEntry();
void addPoint(CFENTRY *entry,PointND *point);
void clearEntry(CFENTRY *entry);

#endif