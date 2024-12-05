#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include "fat32.h"


struct fat32_dir_searchres {
    struct fat_dir fdir;  
    bool found;           
    int idx;              
};


struct fat32_newcluster_info {
    uint32_t cluster;  
    uint32_t address;  
};

struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb);

void mv(FILE *fp, char *source, char *dest, struct fat_bpb *bpb);

void rm(FILE *fp, char *filename, struct fat_bpb *bpb);

void cp(FILE *fp, char *source, char *dest, struct fat_bpb *bpb);

void cat(FILE *fp, char *filename, struct fat_bpb *bpb);

struct fat32_dir_searchres find_in_root(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb);

struct fat32_newcluster_info fat32_find_free_cluster(FILE *fp, struct fat_bpb *bpb);

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#endif
