#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include "entry.h"

//#include <stdio.h>

#define MAX_DIMENSION 1000
#define MAX_CLUSTERS 50

struct tree
{
    struct node* root;
    struct node* leaf_list;
    int instance_index;
    int dimensionality;
};
typedef struct tree Tree;

struct cluster_info{
    int n;
    double ls[MAX_DIMENSION];
    //double ss[MAX_DIMENSION];
};
typedef struct cluster_info Cluster_info;

struct message_cluster{
    int nCluster;
    Cluster_info clusters[MAX_CLUSTERS];
};
typedef struct message_cluster Message_cluster;

Tree* tree_create(int dimensionality, int branching_factor, double threshold, double (*distance)(Entry*, Entry*), bool apply_merging_refinement);
int tree_insert(Tree* tree, double* sample);
void tree_insert_entry(Tree* tree, Entry* entry);
void tree_free(Tree* tree);
Message_cluster tree_get_message_cluster_infos(Tree *tree);

#endif
