#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include "fat16.h"

/*
 * Esta struct encapsula o resultado de find(), carregando informações sobre a
 * busca consigo.
 */
struct far_dir_searchres
{
    struct fat_dir fdir; // Diretório encontrado
    bool found;          // Encontrou algo?
    int idx;             // Index relativo ao diretório de busca
};

/*
 * Esta struct encapsula o resultado de fat32_find_free_cluster()
 */
struct fat32_newcluster_info
{
    uint32_t cluster; // Número do cluster
    uint32_t address; // Endereço na FAT
};

/* List files in FAT32 */
struct fat_dir *ls(FILE *, struct fat_bpb *);

/* Move um arquivo da fonte ao destino */
void mv(FILE *fp, char *source, char *dest, struct fat_bpb *bpb);

/* Deleta o arquivo do diretório FAT32 */
void rm(FILE *fp, char *filename, struct fat_bpb *bpb);

/* Copia o arquivo para o diretório FAT32 */
void cp(FILE *fp, char *source, char *dest, struct fat_bpb *bpb);

/* Esta função escreve no terminal os conteúdos de um arquivo. */
void cat(FILE *fp, char *filename, struct fat_bpb *bpb);

/* Helper function: find specific filename in FAT32 directories */
struct far_dir_searchres find_in_dir(FILE *fp, struct fat_dir *dirs, char *filename, uint32_t start_cluster, struct fat_bpb *bpb);

/* Procura cluster vazio no FAT32 */
struct fat32_newcluster_info fat32_find_free_cluster(FILE *fp, struct fat_bpb *bpb);

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#endif
