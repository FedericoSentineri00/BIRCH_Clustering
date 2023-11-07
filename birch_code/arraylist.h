#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include "point.h"

typedef struct {
    PointND **array;
    int lastIndex;
    int capacity;
} ArrayList;

ArrayList* createArrayList();

void add(ArrayList *list, PointND *element);

PointND* get(ArrayList *list, int index);

PointND* getElementOnTop(ArrayList *list);

PointND* removeOnTop(ArrayList *list);

void freeArrayList(ArrayList *list);

#endif 