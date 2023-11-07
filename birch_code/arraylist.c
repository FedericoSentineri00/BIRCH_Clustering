#include "arraylist.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Funzione per inizializzare l'ArrayList
ArrayList* createArrayList() {
    ArrayList *list = (ArrayList*)malloc(sizeof(ArrayList));
    if (list == NULL) {
        perror("Errore nell'allocazione di memoria per l'ArrayList");
        exit(1);
    }

    list->array = (PointND*)malloc(10 * sizeof(PointND)); // Inizializziamo con una capacità di 10 elementi
    if (list->array == NULL) {
        perror("Errore nell'allocazione di memoria per l'array");
        free(list);
        exit(1);
    }

    list->lastIndex = 0;
    list->capacity = 10;

    return list;
}

// Funzione per aggiungere un elemento all'ArrayList
void add(ArrayList *list, PointND *element) {
    if (list->lastIndex >= list->capacity) {
        // Raddoppia la capacità se l'ArrayList è pieno
        list->capacity *= 2;
        list->array = (PointND*)realloc(list->array, list->capacity * sizeof(PointND));
        if (list->array == NULL) {
            perror("Errore nella reallocazione di memoria per l'array");
            exit(1);
        }
    }
    list->array[list->lastIndex] = element;
    list->lastIndex++;
}

// Funzione per ottenere un elemento dall'ArrayList
PointND* get(ArrayList *list, int index) {
    if (index >= 0 && index < list->lastIndex) {
        return list->array[index];
    } else {
        fprintf(stderr, "Indice fuori dai limiti.\n");
        exit(1);
    }
}

PointND* getElementOnTop(ArrayList *list) {
    if(list->lastIndex>0){
        return list->array[(list->lastIndex-1)];
    }
    return NULL;
}

PointND* removeOnTop(ArrayList *list) {
    if(list->lastIndex>0){
        list->lastIndex--;
        PointND *p = list->array[list->lastIndex];
        list->array[list->lastIndex] = NULL;
        return p;
    }
    return NULL;
}

// Funzione per liberare la memoria allocata per l'ArrayList
void freeArrayList(ArrayList *list) {
    int i;
    for(i = 0; i < list->lastIndex; i++){
        freePoint(list->array[i]);
    }
    free(list->array);
    free(list);
}