#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#include "CFNode.h"
#include "arraylist.h"


const char filePath[] = "./BIRCH_CLUSTERING/DataSet_Test/dataset1D.txt";

ArrayList* readFromFileToArrayList(char* path){

    ArrayList *pointsArrayList = createArrayList();
    // Apre il file in modalità di lettura
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        perror("Errore nell'apertura del file");
        return 1;
    }

    char line[1024]; // Un buffer per leggere le righe del file
    
    while (fgets(line, sizeof(line), file) != NULL) {
        const char delimiter[] = ",";

        // Utilizziamo strtok per suddividere la stringa
        char *token = strtok(line, delimiter);
        int i = 0;
        // Continua a ottenere i token finché non ci sono più virgole
        PointND *point = createPoint(dimension);
        while (token != NULL) {
            point->coordinates[i] = atoi(token);
            token = strtok(NULL, delimiter);
            i++;
        }
        
        add(pointsArrayList,point);
    }

    return pointsArrayList;
}

int main (void) {
    ArrayList *pointsArrayList = readFromFileToArrayList(filePath);
    


    printf("Dimensione dello spazio: %d\n", getElementOnTop(dimension));

    while(getElementOnTop(pointsArrayList) != NULL){
        printf("punto cordinate [");
        int i;
        PointND *puntino= removeOnTop(pointsArrayList);
        for(i = 0; i < dimension-1; i++){
            printf("%i , ", puntino->coordinates[i]);
        }

        printf("%i]\n", puntino->coordinates[i]);
        freePoint(puntino);
    }

    // Liberiamo la memoria
    freeArrayList(pointsArrayList);

    return 0;
}