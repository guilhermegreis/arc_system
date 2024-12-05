#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "commands.h"
#include "fat32.h"  
#include "support.h"

#include <errno.h>
#include <err.h>
#include <error.h>
#include <assert.h>

#include <sys/types.h>


struct far_dir_searchres find_in_root(struct fat_dir *dirs, char *filename, struct fat_bpb *bpb)
{
    struct far_dir_searchres res = { .found = false };

    for (size_t i = 0; i < bpb->possible_rentries; i++)
    {
        if (dirs[i].name[0] == '\0') continue;

        if (memcmp((char *) dirs[i].name, filename, FAT32STR_SIZE) == 0)
        {
            res.found = true;
            res.fdir  = dirs[i];
            res.idx   = i;
            break;
        }
    }

    return res;
}

struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb)
{
    struct fat_dir *dirs = malloc(sizeof(struct fat_dir) * bpb->possible_rentries);

    for (int i = 0; i < bpb->possible_rentries; i++)
    {
        uint32_t offset = bpb_froot_addr(bpb) + i * sizeof(struct fat_dir);
        read_bytes(fp, offset, &dirs[i], sizeof(dirs[i]));
    }

    return dirs;
}

void mv(FILE *fp, char *source, char* dest, struct fat_bpb *bpb)
{
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    bool badname = cstr_to_fat32wnull(source, source_rname) || cstr_to_fat32wnull(dest, dest_rname);

    if (badname)
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_froot_addr(bpb);
    uint32_t root_size = sizeof(struct fat_dir) * bpb->possible_rentries;

    struct fat_dir root[root_size];

    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "erro ao ler struct fat_dir");

    struct far_dir_searchres dir1 = find_in_root(root, source_rname, bpb);
    struct far_dir_searchres dir2 = find_in_root(root, dest_rname, bpb);

    if (dir2.found == true)
        error(EXIT_FAILURE, 0, "Não permitido substituir arquivo %s via mv.", dest);

    if (dir1.found == false)
        error(EXIT_FAILURE, 0, "Não foi possivel encontrar o arquivo %s.", source);

    memcpy(dir1.fdir.name, dest_rname, sizeof(char) * FAT32STR_SIZE);

    uint32_t source_address = sizeof(struct fat_dir) * dir1.idx + root_address;

    (void) fseek(fp, source_address, SEEK_SET);
    (void) fwrite(&dir1.fdir, sizeof(struct fat_dir), 1, fp);

    printf("mv %s → %s.\n", source, dest);
    return;
}

void rm(FILE* fp, char* filename, struct fat_bpb* bpb)
{
    char fat32_rname[FAT32STR_SIZE_WNULL];

    if (cstr_to_fat32wnull(filename, fat32_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t root_address = bpb_froot_addr(bpb);
    uint32_t root_size = sizeof(struct fat_dir) * bpb->possible_rentries;

    struct fat_dir root[root_size];

    if (read_bytes(fp, root_address, &root, root_size) == RB_ERROR)
    {
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "erro ao ler struct fat_dir");
    }

    struct far_dir_searchres dir = find_in_root(root, fat32_rname, bpb);

    if (dir.found == false)
    {
        error(EXIT_FAILURE, 0, "Não foi possível encontrar o arquivo %s.", filename);
    }

    dir.fdir.name[0] = DIR_FREE_ENTRY;

    uint32_t file_address = sizeof(struct fat_dir) * dir.idx + root_address;

    (void) fseek(fp, file_address, SEEK_SET);
    (void) fwrite(&dir.fdir, sizeof(struct fat_dir), 1, fp);

    uint32_t fat_address = bpb_faddress(bpb);
    uint16_t cluster_number = dir.fdir.starting_cluster;
    uint16_t null = 0x0;
    size_t count = 0;

    while (cluster_number < FAT32_EOF_LO)
    {
        uint32_t infat_cluster_address = fat_address + cluster_number * sizeof(uint16_t);
        read_bytes(fp, infat_cluster_address, &cluster_number, sizeof(uint16_t));

        (void) fseek(fp, infat_cluster_address, SEEK_SET);
        (void) fwrite(&null, sizeof(uint16_t), 1, fp);

        count++;
    }

    printf("rm %s, %li clusters apagados.\n", filename, count);
    return;
}