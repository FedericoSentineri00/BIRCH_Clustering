#ifndef POINT_H
#define POINT_H

typedef struct {
    int dimension;      // Numero di dimensioni
    int *coordinates; // Array delle coordinate
} PointND;

PointND* createPoint(int dimension);
void freePoint(PointND *point);

#endif