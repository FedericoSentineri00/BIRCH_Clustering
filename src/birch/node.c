#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <stdbool.h>
#include "../../include/birch/entry.h"
#include "../../include/birch/node.h"
#include "../../include/util/smem.h"


Entry* find_closest_entry(Node *node, Entry* entry);
void merging_refinement(Node* node, Pair* split_entries);
Pair* split_entry(Node* node, Entry* closest_entry);
void replace_closest_pair_with_new_entries(Node *node, Pair* pair, Entry* new_entry_1, Entry* new_entry_2);
void redistribute_entries(Node* node, Array* old_entries_1, Array* old_entries_2, Pair* close_entries, Entry* new_entry_1, Entry* new_entry_2);
void redistribute_entries_merge(Node *node, Array* old_entries_1, Array* old_entries_2, Entry* new_entry);
void replace_closest_pair_with_new_merged_entry(Node* node, Pair* pair, Entry* new_entry);

// Create a new node for the CF tree
Node* node_create(int branching_factor, double threshold, double (*distance)(struct entry*, struct entry*), bool is_leaf, bool apply_merging_refinement){
    Node* node = (Node*) smalloc(sizeof(Node));
    node->branching_factor = branching_factor;
    node->threshold = threshold;
    node->distance = distance;
    node->entries = array_create(branching_factor);
    node->is_leaf = is_leaf;
    node->next_leaf = NULL;
    node->prev_leaf = NULL;
    node->apply_merging_refinement = apply_merging_refinement;
    return node;
}

// Utility method to free all the nodes created at the end of the program execution
void node_free(Node *node){
    array_free(node->entries);
    free(node);
}

// Check if the node passed is a placeholder or not
bool node_is_dummy(Node* node){
    if ((node->prev_leaf != NULL || node->next_leaf != NULL) && node->branching_factor == 0 && node->threshold == 0 && array_size(node->entries)){
        return true;
    }
    else{
        return false;
    }
}

// Check all the CF entries in the node to identify the closest one with the CF entry in input
Entry* find_closest_entry(Node *node, Entry* entry){
    int i;
    double min_dist;
    double curr_dist;
    Entry* curr_entry;
    Entry* closest_entry;

    closest_entry = NULL;
    min_dist = DBL_MAX;

    // Explore all CF entries in the node
    for (i = 0; i < array_size(node->entries); ++i){
        curr_entry = (Entry*) array_get(node->entries, i);
        // Compute the euclidean distance between the entries
        curr_dist = node->distance(curr_entry, entry);
        
        // Update min_dist and closest_entry if it is found a new closest entry
        if (curr_dist < min_dist){
            min_dist = curr_dist;
            closest_entry = curr_entry;
        }
    }

    return closest_entry;
}

// Split entry in case there is not enough space in the node
Pair* split_entry(Node* node, Entry* closest_entry){
    // IF there was a child, but we could not insert the new entry without problems THAN
    // split the child of closest entry

    Array* old_entries;
    Pair* farthest_pair;
    Pair* new_pair;
    Node* old_node;
    Node* new_node_1;
    Node* new_node_2;
    Node* prev;
    Node* next;
    Entry* new_entry_1;
    Entry* new_entry_2;

    old_node = closest_entry->child;
    old_entries = closest_entry->child->entries;
    farthest_pair = pair_find_farthest(old_entries, node->distance);

    new_entry_1 = entry_create_default(closest_entry->dim);
    new_node_1 = node_create(node->branching_factor, node->threshold, node->distance, old_node->is_leaf, node->apply_merging_refinement);
    new_entry_1->child = new_node_1;

    new_entry_2 = entry_create_default(closest_entry->dim);
    new_node_2 = node_create(node->branching_factor, node->threshold, node->distance, old_node->is_leaf, node->apply_merging_refinement);
    new_entry_2->child = new_node_2;


    if(old_node->is_leaf){
        // we do this to preserve the pointers in the leafList

        prev = old_node->prev_leaf;
        next = old_node->next_leaf;

        if(prev != NULL){
            prev->next_leaf = new_node_1;
        }

        if(next != NULL){
            next->prev_leaf = new_node_2;
        }

        new_node_1->prev_leaf = prev;
        new_node_1->next_leaf = new_node_2;
        new_node_2->prev_leaf = new_node_1;
        new_node_2->next_leaf = next;
    }

    node_redistribute_entries(node, old_entries, farthest_pair, new_entry_1, new_entry_2);
    // redistributes the entries in n between newEntry1 and newEntry2
    // according to the distance to p.e1 and p.e2

    node_free(old_node);

    free(farthest_pair);

    entry_remove(node->entries, closest_entry); // this will be substitute by two new entries

    if (closest_entry->indexes != NULL){
        array_deep_clear(closest_entry->indexes);
    }

    entry_free(closest_entry);

    array_add(node->entries, new_entry_1);
    array_add(node->entries, new_entry_2);

    new_pair = pair_create(new_entry_1, new_entry_2);

    return new_pair;
}
// Redistribute entries of node of the CF tree whenever a split of entries is taking place
void node_redistribute_entries(Node* node, Array* old_entries, Pair *far_entries, Entry* new_entry_1, Entry* new_entry_2){
    int i;
    double dist_1;
    double dist_2;
    Entry* curr_entry;

    for (i = 0; i < array_size(old_entries); ++i){
        curr_entry = (Entry*) array_get(old_entries, i);
        dist_1 = node->distance(far_entries->e1, curr_entry);
        dist_2 = node->distance(far_entries->e2, curr_entry);

        if (dist_1 <= dist_2){
            array_add(new_entry_1->child->entries, curr_entry);
            entry_update(new_entry_1, curr_entry);
        }
        else{
            array_add(new_entry_2->child->entries, curr_entry);
            entry_update(new_entry_2, curr_entry);
        }
    }
}

// Redistribute CF entries of node in case of merging refinement
void redistribute_entries(Node* node, Array* old_entries_1, Array* old_entries_2, Pair* close_entries, Entry* new_entry_1, Entry* new_entry_2){
    int i;
    Array* v;
    Entry* curr_entry;
    double dist_1;
    double dist_2;

    v = array_create(node->branching_factor * 2);
    array_add_all(v, old_entries_1);
    array_add_all(v, old_entries_2);

    for (i = 0; i < array_size(v); ++i){
        curr_entry = (Entry*) array_get(v, i);
        dist_1 = node->distance(close_entries->e1, curr_entry);
        dist_2 = node->distance(close_entries->e2, curr_entry);

        if (dist_1 <= dist_2){
            if(array_size(new_entry_1->child->entries) < node->branching_factor){
                array_add(new_entry_1->child->entries, curr_entry);
                entry_update(new_entry_1, curr_entry);
            }
            else{
                array_add(new_entry_2->child->entries, curr_entry);
                entry_update(new_entry_2, curr_entry);
            }
        }
        else if(dist_2 < dist_1){
            if(array_size(new_entry_2->child->entries) < node->branching_factor){
                array_add(new_entry_2->child->entries, curr_entry);
                entry_update(new_entry_2, curr_entry);
            }
            else{
                array_add(new_entry_1->child->entries, curr_entry);
                entry_update(new_entry_1, curr_entry);
            }
        }
    }
}

// Redistribute CF entries of two nodes whenever the merging refinement mthod requires the merge of two different nodes of the CF tree 
void redistribute_entries_merge(Node *node, Array* old_entries_1, Array* old_entries_2, Entry* new_entry){
    int i;
    Array* v;
    Entry* curr_entry;

    v = array_create(node->branching_factor);
    array_add_all(v, old_entries_1);
    array_add_all(v, old_entries_2);

    for (i = 0; i < array_size(v); ++i){
        curr_entry = (Entry*) array_get(v, i);
        array_add(new_entry->child->entries, curr_entry);
        entry_update(new_entry, curr_entry);
    }
}

// Replaces the closest pair of entries in the node with two new entries when it is required by the merging refinement
void replace_closest_pair_with_new_entries(Node *node, Pair* pair, Entry* new_entry_1, Entry* new_entry_2){
    int i;

    for (i = 0; i < array_size(node->entries); i++){
        if(entry_cmp((Entry*) array_get(node->entries, i), pair->e1) == true){
            array_set(node->entries, i, new_entry_1);
        }
        else if(entry_cmp((Entry*) array_get(node->entries, i), pair->e2) == true){
            array_set(node->entries, i, new_entry_2);
        }
    }
}

// Replaces the closest pair of entries in the node with a single new merged entry when it is required by the merging refinement
void replace_closest_pair_with_new_merged_entry(Node* node, Pair* pair, Entry* new_entry){
    int i;

    for (i = 0; i < array_size(node->entries); i++){
        if (entry_cmp((Entry*) array_get(node->entries, i), pair->e1) == true){
            array_set(node->entries, i, new_entry);
        }
        else if (entry_cmp((Entry*) array_get(node->entries, i), pair->e2) == true){
            array_remove_by_index(node->entries, i);
        }
    }
}

// Apply merging refinement to a node (Performed, when necessary, only if specified by the user when launching the execution)
void merging_refinement(Node* node, Pair* split_entries){

    Node* old_node_1;
    Node* old_node_2;
    Node* new_node_1;
    Node* new_node_2;
    Node* new_node;
    Node* dummy_node;
    Array* old_node_1_entries;
    Array* old_node_2_entries;
    Array* node_entries;
    Entry* new_entry_1;
    Entry* new_entry_2;
    Entry* new_entry;
    Pair* pair;

    node_entries = node->entries;
    pair = pair_find_closest(node_entries, node->distance);

    if (pair == NULL){
        // not possible to find a valid pair
        return;
    }

    if (pair_cmp(pair, split_entries)){
        return; // if the closet pair is the one that was just split, we terminate
    }

    old_node_1 = pair->e1->child;
    old_node_2 = pair->e2->child;

    old_node_1_entries = array_clone(old_node_1->entries);
    old_node_2_entries = array_clone(old_node_2->entries);

    if(old_node_1->is_leaf != old_node_2->is_leaf){
        // just to make sure everything is going ok
        printf("ERROR: node.c/merging_refinement(): \"Nodes at the same level must have same leaf status\"\n");
        exit(1);
    }

    if((array_size(old_node_1_entries) + array_size(old_node_2_entries)) > node->branching_factor){
        // the two nodes cannot be merged into one (they will not fit)
        // in this case we simply redistribute them between p.e1 and p.e2

        new_entry_1 = entry_create_default(split_entries->e1->dim);
        // note: in the CFNode construction below the last parameter is false
        // because a split cannot happen at the leaf level
        // (the only exception is when the root is first split, but that's treated separately)
        new_node_1 = old_node_1;
        array_clear(new_node_1->entries);
        new_entry_1->child = new_node_1;

        new_entry_2 = entry_create_default(split_entries->e1->dim);
        new_node_2 = old_node_2;
        array_clear(new_node_2->entries);
        new_entry_2->child = new_node_2;

        redistribute_entries(node, old_node_1_entries, old_node_2_entries, pair, new_entry_1, new_entry_2);
        replace_closest_pair_with_new_entries(node, pair, new_entry_1, new_entry_2);
    }
    else{
        // if the the two closest entries can actually be merged into one single entry

        new_entry = entry_create_default(split_entries->e1->dim);
        // note: in the CFNode construction below the last parameter is false
        // because a split cannot happen at the leaf level
        // (the only exception is when the root is first split, but that's treated separately)
        new_node = node_create(node->branching_factor, node->threshold, node->distance, old_node_1->is_leaf, node->apply_merging_refinement);
        new_entry->child = new_node;

        redistribute_entries_merge(node, old_node_1_entries, old_node_2_entries, new_entry);

        if (old_node_1->is_leaf && old_node_2->is_leaf){
            // this is done to maintain proper links in the leafList
            if(old_node_1->prev_leaf != NULL){
                old_node_1->prev_leaf->next_leaf = new_node;
            }
            if(old_node_1->next_leaf != NULL){
                old_node_1->next_leaf->prev_leaf = new_node;
            }
            new_node->prev_leaf = old_node_1->prev_leaf;
            new_node->next_leaf = old_node_1->next_leaf;

            // this is a dummy node that is only used to maintain proper links in the leafList
            // no CFEntry will ever point to this leaf
            dummy_node = node_create(0, 0, NULL, true, false);

            if (old_node_2->prev_leaf != NULL){
                old_node_2->prev_leaf->next_leaf = dummy_node;
            }
            if (old_node_2->next_leaf != NULL){
                old_node_2->next_leaf->prev_leaf = dummy_node;
            }

            dummy_node->prev_leaf = old_node_2->prev_leaf;
            dummy_node->next_leaf = old_node_2->next_leaf;
        }

        replace_closest_pair_with_new_merged_entry(node, pair, new_entry);
    }

    // merging refinement is done
}

// Method to insert a new CF entry in one of the node of the CF tree.
bool node_insert_entry(Node* node, Entry* entry, bool* hold_memory){

    Entry* closest_entry;
    bool dont_split;
    Pair* split_pair;

    if(array_size(node->entries) == 0){
        *hold_memory = true;
        array_add(node->entries, entry);
        return true;
    }

    closest_entry = find_closest_entry(node, entry);
    dont_split = false;

    if(closest_entry->child != NULL){
        dont_split = node_insert_entry(closest_entry->child, entry, hold_memory);

        if(dont_split == true){
            entry_update(closest_entry, entry); // this updates the CF to reflect the additional entry
            return true;
        }
        else{
            // if the node below /closest/ didn't have enough room to host the new entry
            // we need to split it
            split_pair = split_entry(node, closest_entry);

            // after adding the new entries derived from splitting /closest/ to this node,
            // if we have more than maxEntries we return false,
            // so that the parent node will be split as well to redistribute the "load"
            if(array_size(node->entries) > node->branching_factor){
                free(split_pair);
                return false;
            }
            else{
                // splitting stops at this node
                if(node->apply_merging_refinement){
                    // performs step 4 of insert process (see BIRCH paper, Section 4.3)
                    merging_refinement(node, split_pair);
                }
                free(split_pair);
                return true;
            }
        }
    }
    else if(entry_is_within_threshold(closest_entry, entry, node->threshold, node->distance)){
        // if  dist(closest,e) <= T, /e/ will be "absorbed" by /closest/
        entry_update(closest_entry, entry);
        return true; // no split necessary at the parent level
    }
    else if(array_size(node->entries) < node->branching_factor){
        // if /closest/ does not have children, and dist(closest,e) > T
        // if there is enough room in this node, we simply add e to it
        *hold_memory = true;
        array_add(node->entries, entry);
        return true; // no split necessary at the parent level
    }
    else{
        // not enough space on this node
        *hold_memory = true;
        array_add(node->entries, entry); // adds it momentarily to this node
        return false;   // returns false so that the parent entry will be split
    }

}
