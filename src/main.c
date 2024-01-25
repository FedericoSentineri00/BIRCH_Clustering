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

#define MAX_STEPS 1000

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
    printf("\n\nThe line bellow is an example of a valid command line for running this program:\n");
}

void print_clusters(Message_cluster mc, int dim){
    char clusterText[1024]="";
    int i;
    for(i=0; i<mc.nCluster; i++){
        sprintf(clusterText, "rank %d- %d cluster with %d point\n", my_rank,i,mc.clusters[i].n);

        int d;
        for(d=0;d<dim;d++){
            sprintf(clusterText, "%s %d %f\n", clusterText,d,mc.clusters[i].ls[d]);
        }
        printf("%s" , clusterText);
    }
}

void sendClustersTo(Message_cluster mc, int dest, int dim){
    MPI_Send(&mc.nCluster, 1, MPI_INT, dest,0, MPI_COMM_WORLD);
    int i;
    for(i=0;i<mc.nCluster;i++){
        int d;
        for(d=0;d<dim;d++){
            MPI_Send(&mc.clusters[i].ls[d], 1,MPI_DOUBLE,dest,0,MPI_COMM_WORLD);
        }
        MPI_Send(&mc.clusters[i].n, 1,MPI_INT,dest,0,MPI_COMM_WORLD);
    }
}

Message_cluster receiveClusterFrom(int source, int dim){
    Message_cluster mc;
    int nClusters=0;
    MPI_Recv(&nClusters, 1,MPI_INT,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    mc.nCluster=nClusters;

    int i;
    for(i=0;i<nClusters;i++){
        int d;
        for(d=0;d<dim;d++){
            MPI_Recv(&mc.clusters[i].ls[d], 1,MPI_DOUBLE,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        MPI_Recv(&mc.clusters[i].n, 1,MPI_DOUBLE,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    return mc;
}

int main(int argc, char* argv[])
{
    MPI_Init(NULL,NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (argc < 7)
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

    int senders[MAX_STEPS];
    int receivers[MAX_STEPS];
    int nMerge=0;

    if(my_rank == 0){
        

        int partner,round,proc;
        for(proc=0; proc<comm_sz;proc++){

            for (round = 1; round < comm_sz; round *= 2) {
                if (proc % (2 * round) == 0) {
                    partner = proc + round;
                    
                    if (partner < comm_sz) {
                        senders[nMerge]=partner;
                        receivers[nMerge]=proc;
                        nMerge++;
                    }
                }
            }
        }
    }

    MPI_Bcast(&nMerge, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(senders, nMerge, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(receivers, nMerge, MPI_INT, 0, MPI_COMM_WORLD);

    int i;
    for(i=0; i<nMerge;i++){
        if(senders[i] == my_rank){
            Message_cluster mc = tree_get_message_cluster_infos(tree);
            sendClustersTo(mc, receivers[i],dimensionality);
            
            
        }else if(receivers[i] == my_rank){

            Message_cluster mc = tree_get_message_cluster_infos(tree);

            Message_cluster mc2 = receiveClusterFrom(senders[i],dimensionality);

            int n;
            for (n = 0; n < mc2.nCluster; n++){

                Entry* e = entry_create_default(dimensionality);
                e->n=mc2.clusters[n].n;
                e->ls=mc2.clusters[n].ls;
                e->dim=dimensionality;

                int d;
                for (d = 0; d < dimensionality; d++)
                {
                    e->ss[d] = e->ls[d] * e->ls[d];
                }
                
                tree_insert_entry(tree,e);
                //entry_free(e);
            }
        }
    }

    if(my_rank == 0){
        Message_cluster mc = tree_get_message_cluster_infos(tree);
        print_clusters(mc, dimensionality);
    }

    MPI_Finalize();

    return 0;
}