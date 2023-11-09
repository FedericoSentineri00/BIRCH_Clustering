#ifndef COMMON_H
#define COMMON_H

#include "point.h"

#define dimension 1
#define numberOfEntriesPerNode 2
#define numberOfChildrenNode 2
const int threshold[] = {2.0,5.0};

double sqrt(double num) {
    if (num < 0) {
        return -1.0; // Valore non valido
    }

    double x = num;
    double epsilon = 0.000001; // Tolleranza per l'approssimazione

    while ((x * x - num) > epsilon) {
        x = (x + num / x) / 2.0;
    }

    return x;
}

// Funzione per calcolare la distanza euclidea tra due punti in uno spazio di n dimensioni
double euclideanDistance(PointND *point1, PointND *point2) {
    double sum = 0.0;
    for (int i = 0; i < dimension; i++) {
        double diff = point1->coordinates[i] - point2->coordinates[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

double * computeDiamter(int n, double LS[], double SS[]){
    double diameter[dimension];
    int i;
    for(i = 0; i < dimension; i++){
        diameter[i] = sqrt((2*n*SS[i]-2*(LS[i]*LS[i]))/(n*(n-1)));
    }
    return diameter;
}

double * computeCentroids(int n, double LS[]){
    double centroid[dimension];
    int i;
    for(i = 0; i < dimension; i++){
        centroid[i] = LS[i] / n;
    }
    return centroid;
}

#endif