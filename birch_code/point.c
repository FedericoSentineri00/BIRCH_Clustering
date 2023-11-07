#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "point.h"

PointND* createPoint(int dimension) {
    PointND *point = (PointND*)malloc(sizeof(PointND));
    if (point == NULL) {
        fprintf(stderr, "Errore nell'allocazione di memoria per il punto.\n");
        exit(1);
    }

    point->dimension = dimension;
    point->coordinates = (int*)malloc(dimension * sizeof(int));
    if (point->coordinates == NULL) {
        fprintf(stderr, "Errore nell'allocazione di memoria per le coordinate.\n");
        free(point);
        exit(1);
    }

    // Inizializziamo le coordinate a zero
    int i;
    for (i = 0; i < dimension; i++) {
        point->coordinates[i] = 0;
    }

    return point;
}

void freePoint(PointND *point) {
    free(point->coordinates);
    free(point);
}