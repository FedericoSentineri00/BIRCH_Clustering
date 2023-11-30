#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../include/util/smem.h"
#include "../include/util/instance.h"
#include "../include/util/integer.h"
#include "../include/birch/tree.h"
#include "../include/birch/node.h"
#include "../include/util/array.h"
#include <mpi.h>

int comm_sz;
int my_rank;

double distance(Entry* e1, Entry* e2)
{
    double dist = 0;
    int i;
    for (i = 0; i < e1->dim; ++i)
    {
        double diff = (e1->ls[i] / e1->n) - (e2->ls[i] / e2->n);
        dist += diff * diff;
    }
    return sqrt(dist);
}

void print_output(Tree* tree, Array* indexes, char *output_file_path)
{
    FILE *f = fopen(output_file_path, "w");
    if (f == NULL)
    {
        printf("ERROR: main.c/print_output(): \"Error opening file\"\n");
        exit(1);
    }

    int* cluster_id_by_entry_index = tree_get_cluster_id_by_instance_index(tree);
    
    int i;
    for (i = 0; i < array_size(indexes); ++i)
    {
        Integer* entry_index = (Integer*) array_get(indexes, i);
        fprintf(f, "cluster_%d\n", cluster_id_by_entry_index[entry_index->value]);
    }

    fclose(f);

    free(cluster_id_by_entry_index);
}

void print_arguments_message()
{
    printf("\n>>> Invalid numbers of arguments! <<<\n");
    printf("\nThe following parameters must be informed in this order and separated by whitespace:\n");
    printf("\n1. Branching Factor: An integer value;\n");
    printf("\n2. Threshold: A float value;\n");
    printf("\n3. Apply Merging Refinement: 1 for yes, 0 for no;\n");
    printf("\n4. Dataset File Path: String;\n");
    printf("\n5. Dataset Delimiters: String;\n");
    printf("\n6. Ignore Last Column of the Dataset: 1 for yes, 0 for no;\n");
    printf("\n7. Output File Name: String.\n");
    printf("\n\nThe line bellow is an example of a valid command line for running this program:\n");
    printf("./main 100 0.8 1 IRIS.csv , 1 output.csv\n\n");
}

void print_clusters(Message_cluster mc, int dim){
    char *clusterText="";
    int i;
    for(i=0; i<mc.nCluster; i++){
        sprintf(clusterText, "%d-%d cluster with %d point\n", my_rank,i,mc.clusters[i].n);

        int d;
        for(d=0;d<dim;d++){
            sprintf(clusterText, "%s %d %f\n", clusterText,i,mc.clusters[i].ls[d]);
        }
        printf("%s" , clusterText);
    }
}

int main(int argc, char* argv[])
{

    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (argc < 8)
    {
        print_arguments_message();
        exit(1);
    }

    //________________CONFIGURATION________________
    int branching_factor = atoi(argv[1]);
    double threshold = atof(argv[2]);
    int apply_merging_refinement = atoi(argv[3]);
    char* input_file_path = argv[4];
    char* column_delimiter = argv[5];
    int last_column_is_label = atoi(argv[6]);
    char* output_file_path = argv[7];
    sprintf(output_file_path, "%s_%d.csv", output_file_path, my_rank);
    //----------------CONFIGURATION----------------
    
    FILE* stream;
    char line[1024];
    char* delimiters;
    int filesize;

    if(my_rank == 0){
        stream = fopen(input_file_path, "r");
        if (stream == NULL)
        {
            printf("ERROR: main.c/main(): \"Error opening file\"\n");
            exit(1);
        }

        filesize = 0;

        while(fgets(line, 1024, stream)){
            filesize++;
        }
        fclose(stream);
    }

    MPI_Bcast(&filesize, 1, MPI_INT, 0, MPI_COMM_WORLD);


    stream = fopen(input_file_path, "r");
    int count;
    
    while(count < (filesize/comm_sz)*my_rank){
        fgets(line, 1024, stream);
        count++;
    }

    if(my_rank == 0){
        printf("sono lo 0 figli di troia, il mio file size vale %d: leggo da riga %d a riga %d\n" , filesize, count, (filesize/comm_sz)*(my_rank+1));
    }else{
        printf("%d il mio file size vale %d: leggo da riga %d a riga %d\n" ,my_rank, filesize, count, (filesize/comm_sz)*(my_rank+1));
    }

    delimiters = smalloc(sizeof(char*) * (strlen(column_delimiter) + 3));
    strcpy(delimiters, column_delimiter);
    strcat(delimiters, "\r\n");

    fgets(line, 1024, stream);
    int dimensionality = instance_calculate_dimensionality(line, delimiters, last_column_is_label);
    Tree* tree = tree_create(dimensionality, branching_factor, threshold, distance, apply_merging_refinement == 1);
    Array* instances_indexes = array_create(1);

    do {
        double* instance = instance_read(line, dimensionality, delimiters);
        int instance_index = tree_insert(tree, instance);
        array_add(instances_indexes, integer_create(instance_index));

        free(instance);
        count++;
    } while(fgets(line, 1024, stream) && ((my_rank<comm_sz-1) ? count < (filesize/comm_sz)*(my_rank+1) : true));    //se l'ultimo processo, legge fino a fine file, per evitare di non perdere valori

    fclose(stream);

    Message_cluster mc = tree_get_message_cluster_infos(tree);
    print_clusters(mc, dimensionality);
    print_output(tree, instances_indexes, output_file_path);

    array_deep_clear(instances_indexes);
    array_free(instances_indexes);
    free(delimiters);
    tree_free(tree);

    
    MPI_Finalize();

    return 0;
}
