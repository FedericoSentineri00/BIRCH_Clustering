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

// Maximum number of step for the merging phase
#define MAX_STEPS 1000
// Util variables to manage MPI parallelization
int comm_sz;
int my_rank;

// Compute Euclidean distance between two different CF entries
double distance(Entry* e1, Entry* e2){
    double dist = 0;
    int i;
    for (i = 0; i < e1->dim; ++i){
        double diff = (e1->ls[i] / e1->n) - (e2->ls[i] / e2->n);
        dist += diff * diff;
    }
    return sqrt(dist);
}

// Error message for the program arguments
void print_arguments_message(){
    printf("\n>>> Invalid numbers of arguments! <<<\n");
    printf("\nThe following parameters must be informed in this order and separated by whitespace:\n");
    printf("\n1. Branching Factor (int);\n");
    printf("\n2. Threshold (float);\n");
    printf("\n3. Apply Merging Refinement (1: yes, 0: no);\n");
    printf("\n4. Dataset File Path (String);\n");
    printf("\n5. Dataset Delimiters (String);\n");
    printf("\n6. Ignore Last Column of the Dataset: (1: yes, 0: no).\n");
}

// Print the all the key info of the clusters (number of points in each cluster and linear sum of the various features) within a CF tree.
void print_clusters(Message_cluster mc, int dim){
    char clusterText[1024]="";
    int i;
    for(i=0; i<mc.nCluster; i++){
        sprintf(clusterText, "Process %d - Cluster %d with %d points\n", my_rank,i,mc.clusters[i].n);

        int d;
        for(d=0;d<dim;d++){
            sprintf(clusterText, "%s %d %f\n", clusterText,d,mc.clusters[i].ls[d]);
        }
        printf("%s" , clusterText);
    }
}

// Send clusters obtained from the computation of a partial CF tree to process with id = dest
void sendClustersTo(Message_cluster mc, int dest, int dim){
    // Send number of cluster presents in the tree
    MPI_Send(&mc.nCluster, 1, MPI_INT, dest,0, MPI_COMM_WORLD);
    int i;
    // For each cluster identified
    for(i=0;i<mc.nCluster;i++){
        int d;
        // For each dimension
        for(d=0;d<dim;d++){
            // Send linear sum
            MPI_Send(&mc.clusters[i].ls[d], 1,MPI_DOUBLE,dest,0,MPI_COMM_WORLD);
        }
        // Send number of data points belonging to the cluster
        MPI_Send(&mc.clusters[i].n, 1,MPI_INT,dest,0,MPI_COMM_WORLD);
    }
}

// Receive clusters obtained from the computation of the partial CF tree of process with id = source
Message_cluster receiveClustersFrom(int source, int dim){
    Message_cluster mc;
    int nClusters=0;
    // Receive number of clusters
    MPI_Recv(&nClusters, 1,MPI_INT,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    mc.nCluster=nClusters;
    int i;
    // For each cluster
    for(i=0;i<nClusters;i++){
        int d;
        // For each dimension
        for(d=0;d<dim;d++){
            // Receive linear sum
            MPI_Recv(&mc.clusters[i].ls[d], 1,MPI_DOUBLE,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        // Receive number of data points belonging to the cluster
        MPI_Recv(&mc.clusters[i].n, 1,MPI_DOUBLE,source,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    return mc;
}

// Print into a csv file the results of the clustering after the construction of the initial CF tree by a process
void print_output(Tree* tree, Array* indexes, char *output_file_path, int id)
{
    // Create and open the file for the output of the process with id = id
    char file_name[1024] = "";
    sprintf(file_name,"%s_%d.csv",output_file_path,id);
    FILE *f = fopen(file_name, "w");
    if (f == NULL)
    {
        printf("ERROR: main.c/print_output(): \"Error opening file\"\n");
        exit(1);
    }

    // Get clusters id
    int* cluster_id_by_entry_index = tree_get_cluster_id_by_instance_index(tree);

    // Get the cluster for each data samples in the tree
    int i;
    for (i = 0; i < array_size(indexes); ++i)
    {
        Integer* entry_index = (Integer*) array_get(indexes, i);
        fprintf(f, "%d\n", cluster_id_by_entry_index[entry_index->value]);
    }

    // Close file stream
    fclose(f);

    // Deelete tmp structures
    free(cluster_id_by_entry_index);
}

// Print into a csv file the results of the clustering after the merge of two partial trees
void merge_output(Tree* tree, Array* indexes, char *output_file_path, int id, int id_sender, int extra)
{
    char file_name[1024] = "";

    // Open the file for the outpout for process id
    sprintf(file_name,"%s_%d.csv",output_file_path,id);
    FILE *fw = fopen(file_name, "a");

    // Open the file of the output of process id_sender
    sprintf(file_name,"%s_%d.csv",output_file_path,id_sender);
    FILE *fr = fopen(file_name, "r");

    if (fw == NULL || fr == NULL)
    {
        printf("ERROR: main.c/merge_output(): \"Error opening file\"\n");
        exit(1);
    }

    // Get clusters id
    int* cluster_id_by_entry_index = tree_get_cluster_id_by_instance_index(tree);

    // Get the clusters id for the centroids inserted during the merging phase of the CF tree maintained by process id and process id_sender
    Array* cluster_id_centroids = array_create(20);
    int i;
    for (i = extra; i < array_size(indexes); ++i)
    {
        Integer* entry_index = (Integer*) array_get(indexes, i);
        array_add(cluster_id_centroids,integer_create(cluster_id_by_entry_index[entry_index->value]));
    }

    // Add in the output file of process with id = id the cluster associated to data samples on which process id_sender worked previuosly
    char line[100];
    while(fgets(line, 100, fr)){
        Integer* cluster_id = (Integer*) array_get(cluster_id_centroids, atoi(line));
        fprintf(fw, "%d\n", cluster_id->value);
    } 

    // Close files
    fclose(fw);
    fclose(fr);

    // Free tmp structures
    free(cluster_id_by_entry_index);

    array_free(cluster_id_centroids);
}

int main(int argc, char* argv[]){
    // Variable used to collect time spent for execution
    double t1,t2;

    // Initialize MPI
    MPI_Init(NULL,NULL);

    // If this is process 0, it has also to collect the time on which the execution started
    if(my_rank == 0){
        t1 = MPI_Wtime();
    }

    // Memorize number of processes and id
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    //Check program arguments and save them
    if (argc < 7){
        print_arguments_message();
        exit(1);
    }

    int branching_factor = atoi(argv[1]);
    double threshold = atof(argv[2]);
    int apply_merging_refinement = atoi(argv[3]);
    char* input_file_path = argv[4];
    char* column_delimiter = argv[5];
    int last_column_is_label = atoi(argv[6]);
    char* output_file_path = "./BIRCH_Clustering/output";
    
    FILE* stream;
    char line[1024];
    char* delimiters;
    int filesize;

    // If this is process 0, try to open the file containing the dataset and count the numer of elements presents
    if(my_rank == 0){
        stream = fopen(input_file_path, "r");
        if (stream == NULL){
            printf("ERROR: main.c/main(): \"Error opening file\"\n");
            exit(1);
        }

        filesize = 0;

        while(fgets(line, 1024, stream)){
            filesize++;
        }
        fclose(stream);
    }

    // Broadcast the size of the dataset
    MPI_Bcast(&filesize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Prepare the stream to read the file and skip data samples until the process is in the dataset part reserved to it
    stream = fopen(input_file_path, "r");
    int count;
    while(count < (filesize/comm_sz)*my_rank){
        fgets(line, 1024, stream);
        count++;
    }

    // Define the delimeters
    delimiters = smalloc(sizeof(char*) * (strlen(column_delimiter) + 3));
    strcpy(delimiters, column_delimiter);
    strcat(delimiters, "\r\n");

    // Start acquairing the data samples and initialize CF tree and other utilities
    fgets(line, 1024, stream);
    int dimensionality = instance_calculate_dimensionality(line, delimiters, last_column_is_label);
    Tree* tree = tree_create(dimensionality, branching_factor, threshold, distance, apply_merging_refinement == 1);
    Array* instances_indexes = array_create(1);

    do {      
        // Read data sample and insert it in the tree  
        double* instance = instance_read(line, dimensionality, delimiters);
        int instance_index = tree_insert(tree, instance);
        array_add(instances_indexes, integer_create(instance_index));

        free(instance);
        count++;
    } // Read a new line, until the process does not cross over into the partition of the next process.
     // If this is the last process, reads to the end of the file, to avoid not considering certain data samples
    while(fgets(line, 1024, stream) && ((my_rank<comm_sz-1) ? count < (filesize/comm_sz)*(my_rank+1) : true));    

    // Close the file stream
    fclose(stream);
    
    // Print the results obtained after the creation of the CF tree for the partion of dataset managed by this process
    print_output(tree, instances_indexes, output_file_path,my_rank);

    // Array containing, for each merging round, who has to send its clusters info
    int senders[MAX_STEPS];
    // Array containing, for each merging round, who has ro receive clusters info
    int receivers[MAX_STEPS];
    // Numbers of rounds planned
    int nMerge=0;

    // If this is process 0, it has to compute the schedule of the merging phase of the partical CF trees obtained by the various processes
    if(my_rank == 0){    
        int phases;
        int i;
        // The merging workflow will have N phases until converging in a single CF tree. Obviously, N is equal to the log, base 2, of the number of process present
        for(phases = 0; phases < ceil(log2(comm_sz)); phases++){
            // For each phase, each process that is still working on the merging phase must communicate with the nearest process, in terms of id
            // So initially process 0 with process 1, process 2 with process 3, etc...
            int this_pow = (int)pow(2,phases);
            int increment_support = (int)pow(2,phases+1);
		    for(i=0;i+this_pow<comm_sz; i=i+increment_support){
                // Set sender and receiver for merge round i. For each pair of processes, the process with the smaller id receives the info, while the other one send them
			    int matched = i+this_pow;
                senders[nMerge]=matched;
                receivers[nMerge]=i;
                nMerge++;
		    }
	    }
    }

    // Broadcast the number of rounds planned, senders and receivers
    MPI_Bcast(&nMerge, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(senders, nMerge, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(receivers, nMerge, MPI_INT, 0, MPI_COMM_WORLD);

    // Merging phase
    int i;
    int extra;
    for(i=0; i<nMerge;i++){
        // For each merging round, process checks senders and receivers
        if(senders[i] == my_rank){
            // If its id is in senders[i], retrieves clusters info from its CF tree and send them to process with id receivers[i]
            Message_cluster mc = tree_get_message_cluster_infos(tree);
            sendClustersTo(mc, receivers[i],dimensionality);
        }else if(receivers[i] == my_rank){
            // If its id is in receivers[i], waits and receives clusters info from process with id senders[i]
            Message_cluster mc2 = receiveClustersFrom(senders[i],dimensionality);
            // Get the number of entries preent in the CF tree at the moment
            extra = tree->instance_index;

            // Based on the received info, for each cluster create a new CF entry (it represents the centroid of the cluster) and insert the entry in the CF tree
            int n;
            for (n = 0; n < mc2.nCluster; n++){
                // Initialize CF entry
                Entry* e = entry_create_default(dimensionality);
                // Set numer of elements and dimensionality
                e->n=mc2.clusters[n].n;
                e->dim=dimensionality;

                // Save the linear sum and compute the square sum (via the linear sum)
                int d;
                for (d = 0; d < dimensionality; d++){
                    e->ls[d] = mc2.clusters[n].ls[d];
                    e->ss[d] = e->ls[d] * e->ls[d];
                }
                
                int instance_index = tree_insert_entry(tree,e);
                array_add(instances_indexes, integer_create(instance_index));
            }
            
            // Update the output file of this process after the merging pahse with process with id = senders[i]
            merge_output(tree, instances_indexes, output_file_path,my_rank,senders[i],extra);
        }
    }

    // If this is process 0, after the merging phase print the info of the final CF tree, execution time and remove intermediate csv files
    if(my_rank == 0){
        Message_cluster mc = tree_get_message_cluster_infos(tree);
        print_clusters(mc, dimensionality);
        t2 = MPI_Wtime();
        printf("\n\nEXECUTION TIME: %f s\n\n",(t2-t1));
        
        int i;
        for(i = 1; i < comm_sz; i++){
            char file_name[1024] = "";
            sprintf(file_name,"%s_%d.csv",output_file_path,i);
            remove(file_name);
        }
    }
    
    // Free all the structures used
    array_deep_clear(instances_indexes);
    array_free(instances_indexes);
    free(delimiters);
    tree_free(tree);

    // Finalize the parallelization
    MPI_Finalize();

    return 0;
}
