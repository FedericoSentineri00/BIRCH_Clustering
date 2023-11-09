#ifndef POINT_H
#define POINT_H

#include "comms.h"

typedef struct {
    double *coordinates; // Array delle coordinate
} PointND;

PointND* createPoint();
void freePoint(PointND *point);

#endif