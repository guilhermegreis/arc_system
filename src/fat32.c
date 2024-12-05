#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat32.h"
#include "support.h"

int read_bytes(FILE *fp, unsigned int offset, void *buff, unsigned int len) {
    if (fseek(fp, offset, SEEK_SET) != 0) {
        fprintf(stderr, "Erro ao buscar o offset %u\n", offset);
        return -1;
    }
    if (fread(buff, 1, len, fp) != len) {
        fprintf(stderr, "Erro ao ler o arquivo\n");
        return -1;
    }
    return 0;
}

void rfat(FILE *fp, struct fat_bpb *bpb) {
    if (read_bytes(fp, 0x0, bpb, sizeof(struct fat_bpb)) != 0) {
        fprintf(stderr, "Erro ao ler o BPB.\n");
    }
}

struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb) {
    uint32_t root_addr = bpb_froot_addr(bpb);           
    uint32_t entry_count = bpb->possible_rentries;     
    struct fat_dir *dirs = malloc(entry_count * sizeof(struct fat_dir));

    if (dirs == NULL) {
        fprintf(stderr, "Erro ao alocar memória para diretórios.\n");
        return NULL;
    }

    if (read_bytes(fp, root_addr, dirs, entry_count * sizeof(struct fat_dir)) != 0) {
        fprintf(stderr, "Erro ao ler diretórios.\n");
        free(dirs);
        return NULL;
    }

    show_files(dirs);  
    return dirs;
}

uint32_t bpb_faddress(struct fat_bpb *bpb) {
    return bpb->reserved_sect * bpb->bytes_p_sect;
}

uint32_t bpb_froot_addr(struct fat_bpb *bpb) {
    return bpb_faddress(bpb) + bpb->n_fat * bpb->sect_per_fat * bpb->bytes_p_sect;
}

uint32_t bpb_fdata_addr(struct fat_bpb *bpb) {
    return bpb_froot_addr(bpb) + bpb->possible_rentries * sizeof(struct fat_dir);
}

uint32_t bpb_fdata_sector_count(struct fat_bpb *bpb) {
    return bpb->large_n_sects - (bpb_fdata_addr(bpb) / bpb->bytes_p_sect);
}

uint32_t bpb_fdata_cluster_count(struct fat_bpb *bpb) {
    return bpb_fdata_sector_count(bpb) / bpb->sector_p_clust;
}
