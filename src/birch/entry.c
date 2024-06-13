#include <stdbool.h>
#include "../../include/birch/entry.h"
#include "../../include/util/smem.h"
#include "../../include/util/integer.h"
#include "../../include/util/util.h"


#define INDEXES_INITIAL_SIZE 4

// Initialize a new CF entry
Entry* entry_create_default(int dim){
    Entry* entry = (Entry*) smalloc(sizeof(Entry));

    entry->dim = dim;
    entry->n = 0;
    entry->ls = (double*) scalloc(dim, sizeof(double));
    entry->ss = (double*) scalloc(dim, sizeof(double));
    entry->child = NULL;
    entry->indexes = NULL;
    entry->subcluster_id = -1;

    return entry;
}

// Create a new CF entry
Entry* entry_create(double* x, int dim, int index){
    int i;

    Entry* entry = entry_create_default(dim);

    entry->n = 1;
    entry->dim = dim;

    smemcpy(entry->ls, x, dim * sizeof(x));

    for (i = 0; i < entry->dim; i++){
        entry->ss[i] = entry->ls[i] * entry->ls[i];
    }

    entry->indexes = array_create(INDEXES_INITIAL_SIZE);
    array_add(entry->indexes, integer_create(index));

    return entry;
}

// Utility method to free entries at the end of the program execution
void entry_free(Entry *entry){
    entry->dim = 0;
    entry->n = 0;

    if (entry->ls != NULL){
        free(entry->ls);
    }

    if (entry->ss != NULL){
        free(entry->ss);
    }

    if (entry->indexes != NULL){
        array_free(entry->indexes);
    }

    free(entry);
}

// Update CF entry whenever during CF tree creation it can be joined by another CF entry 
void entry_update(Entry *e1, Entry *e2){
    int i;

    // Update the number of elements in the entry
    e1->n += e2->n;

    // Update the linear sum
    for(i = 0; i < e1->dim; ++i){
        e1->ls[i] += e2->ls[i];
    }

    // Update square sum
    for(i = 0; i < e1->dim; i++){
        e1->ss[i] += e2->ss[i];
    }

    if(e1->child == NULL){
        if(e1->indexes != NULL && e2->indexes != NULL){
            array_add_all(e1->indexes, e2->indexes);
        }
        else if(e1->indexes == NULL && e2->indexes != NULL){
            e1->indexes = array_clone(e2->indexes);
        }
    }

}

// Check if the distance between two CF entries is within the specified threshold
bool entry_is_within_threshold(Entry *e1, Entry *e2, double threshold, double (*distance)(Entry*, Entry*)){
    double dist = distance(e1, e2);

    if(dist == 0 || dist <= threshold){
        return true;
    }
    else{
        return false;
    }
}

// Compare to CF entries
bool entry_cmp(Entry *e1, Entry *e2){
    if (e1->n != e2->n)
        return false;

    if (e1->child != NULL && e2->child == NULL)
        return false;

    if (e1->child == NULL && e2->child != NULL)
        return false;

    if(e1->child != NULL && e1->child != e2->child)
        return false;

    if (e1->indexes == NULL && e2->indexes != NULL)
        return false;

    if (e1->indexes != NULL && e2->indexes == NULL)
        return false;

    if (double_cmp(e1->ls, e2->ls, e1->dim) == false)
        return false;

    if (double_cmp(e1->ss, e2->ss, e1->dim) == false)
        return false;

    if(e1->indexes != NULL && integer_array_cmp(e1->indexes, e2->indexes) == false)
        return false;

    return true;
}

// Utility method used to remove a CF entry from a set of entries
void entry_remove(Array* entries, Entry* entry){
    int index = 0;
    // Check the requested CF entry in the array
    while (index < array_size(entries) && entry_cmp(entry, array_get(entries, index)) == false){
        ++index;
    }

    // Remove the requested CF entry thanks to the index
    if (index < array_size(entries)){
        array_remove_by_index(entries, index);
    }
}
