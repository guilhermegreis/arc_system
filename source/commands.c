#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "commands.h"
#include "fat16.h"
#include "support.h"

#include <errno.h>
#include <err.h>
#include <error.h>
#include <assert.h>

#include <sys/types.h>

uint32_t get_next_cluster(FILE *fp, uint32_t current_cluster, struct fat_bpb *bpb)
{
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t entry_address = fat_address + current_cluster * sizeof(uint32_t);
    uint32_t next_cluster;

    if (read_bytes(fp, entry_address, &next_cluster, sizeof(uint32_t)) == RB_ERROR)
    {
        error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler tabela FAT.");
    }

    return next_cluster;
}

/*
 * Função de busca na pasta raíz. Codigo original do professor,
 * altamente modificado.
 *
 * Ela itera sobre todas as bpb->possible_rentries do struct fat_dir* dirs, e
 * retorna a primeira entrada com nome igual à filename.
 */
struct far_dir_searchres find_in_dir(FILE *fp, struct fat_dir *dirs, char *filename, uint32_t start_cluster, struct fat_bpb *bpb)
{
    struct far_dir_searchres res = {.found = false};
    uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t cluster_number = start_cluster;

    do
    {
        uint32_t cluster_address = (cluster_number - 2) * cluster_width + bpb_fdata_addr(bpb);
        if (read_bytes(fp, cluster_address, dirs, cluster_width) == RB_ERROR)
        {
            error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao ler cluster de diretório.");
        }

        for (size_t i = 0; i < cluster_width / sizeof(struct fat_dir); i++)
        {
            if (dirs[i].name[0] == '\0')
                continue;

            if (memcmp((char *)dirs[i].name, filename, FAT32STR_SIZE) == 0)
            {
                res.found = true;
                res.fdir = dirs[i];
                res.idx = i;
                break;
            }
        }

        cluster_number = get_next_cluster(fp, cluster_number, bpb);
    } while (!res.found && cluster_number < FAT32_EOF);

    return res;
}

/*
 * Função de ls
 *
 * Ela itéra todas as bpb->possible_rentries do diretório raiz
 * e as lê via read_bytes().
 */
struct fat_dir *ls(FILE *fp, struct fat_bpb *bpb)
{
    uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t cluster_number = bpb->root_cluster;
    struct fat_dir *dirs = malloc(cluster_width);

    printf("ATTR  NAME    SIZE\n------------------\n");
    do
    {
        uint32_t cluster_address = (cluster_number - 2) * cluster_width + bpb_fdata_addr(bpb);
        if (read_bytes(fp, cluster_address, dirs, cluster_width) == RB_ERROR)
        {
            error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao listar diretório.");
        }

        for (size_t i = 0; i < cluster_width / sizeof(struct fat_dir); i++)
        {
            if (dirs[i].name[0] == '\0')
                continue;

            printf("0x%02X  %11s  %u\n", dirs[i].attr, dirs[i].name, dirs[i].file_size);
        }

        cluster_number = get_next_cluster(fp, cluster_number, bpb);
    } while (cluster_number < FAT32_EOF);

    return dirs;
}

void mv(FILE *fp, char *source, char *dest, struct fat_bpb *bpb)
{
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    // Converte os nomes
    if (cstr_to_fat16wnull(source, source_rname) || cstr_to_fat16wnull(dest, dest_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    struct fat_dir dirs[bpb->bytes_p_sect * bpb->sector_p_clust / sizeof(struct fat_dir)];
    struct far_dir_searchres dir1 = find_in_dir(fp, dirs, source_rname, bpb->root_cluster, bpb);
    struct far_dir_searchres dir2 = find_in_dir(fp, dirs, dest_rname, bpb->root_cluster, bpb);

    if (dir2.found)
    {
        fprintf(stderr, "Não permitido substituir arquivo %s via mv.\n", dest);
        exit(EXIT_FAILURE);
    }

    if (!dir1.found)
    {
        fprintf(stderr, "Arquivo %s não encontrado.\n", source);
        exit(EXIT_FAILURE);
    }

    // Renomeia
    memcpy(dir1.fdir.name, dest_rname, FAT32STR_SIZE);

    uint32_t cluster_number = bpb->root_cluster;
    uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t cluster_address = (cluster_number - 2) * cluster_width + bpb_fdata_addr(bpb);

    // Atualiza a entrada no disco
    (void)fseek(fp, cluster_address + dir1.idx * sizeof(struct fat_dir), SEEK_SET);
    (void)fwrite(&dir1.fdir, sizeof(struct fat_dir), 1, fp);

    printf("mv %s → %s.\n", source, dest);
}

void rm(FILE *fp, char *filename, struct fat_bpb *bpb)
{
    char fat16_rname[FAT32STR_SIZE_WNULL];
    uint32_t zero = 0; // Variável zero inicializada

    if (cstr_to_fat16wnull(filename, fat16_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    struct fat_dir dirs[bpb->bytes_p_sect * bpb->sector_p_clust / sizeof(struct fat_dir)];
    struct far_dir_searchres dir = find_in_dir(fp, dirs, fat16_rname, bpb->root_cluster, bpb);

    if (!dir.found)
    {
        fprintf(stderr, "Arquivo %s não encontrado.\n", filename);
        exit(EXIT_FAILURE);
    }

    // Marca a entrada como livre
    dir.fdir.name[0] = DIR_FREE_ENTRY;

    uint32_t cluster_number = bpb->root_cluster;
    uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t cluster_address = (cluster_number - 2) * cluster_width + bpb_fdata_addr(bpb);

    // Atualiza o disco
    (void)fseek(fp, cluster_address + dir.idx * sizeof(struct fat_dir), SEEK_SET);
    (void)fwrite(&dir.fdir, sizeof(struct fat_dir), 1, fp);

    // Libera os clusters associados
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t cluster = dir.fdir.starting_cluster;

    while (cluster < FAT32_EOF)
    {
        uint32_t next_cluster;
        uint32_t entry_address = fat_address + cluster * sizeof(uint32_t);

        (void)read_bytes(fp, entry_address, &next_cluster, sizeof(uint32_t));
        (void)fseek(fp, entry_address, SEEK_SET);
        (void)fwrite(&zero, sizeof(uint32_t), 1, fp);

        cluster = next_cluster;
    }

    printf("rm %s concluído.\n", filename);
}

struct fat32_newcluster_info fat32_find_free_cluster(FILE *fp, struct fat_bpb *bpb)
{
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t total_clusters = bpb_fdata_cluster_count(bpb);

    for (uint32_t cluster = 2; cluster < total_clusters; cluster++)
    {
        uint32_t entry;
        uint32_t entry_address = fat_address + cluster * sizeof(uint32_t);

        if (read_bytes(fp, entry_address, &entry, sizeof(uint32_t)) == RB_ERROR)
        {
            error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao verificar cluster livre.");
        }

        if (entry == 0x0)
        {
            return (struct fat32_newcluster_info){.cluster = cluster, .address = entry_address};
        }
    }

    return (struct fat32_newcluster_info){0};
}

void cp(FILE *fp, char *source, char *dest, struct fat_bpb *bpb)
{
    char source_rname[FAT32STR_SIZE_WNULL], dest_rname[FAT32STR_SIZE_WNULL];

    // Converte os nomes para o formato FAT32
    if (cstr_to_fat16wnull(source, source_rname) || cstr_to_fat16wnull(dest, dest_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    struct fat_dir dirs[bpb->bytes_p_sect * bpb->sector_p_clust / sizeof(struct fat_dir)];

    // Localiza o arquivo de origem no diretório raiz
    struct far_dir_searchres dir1 = find_in_dir(fp, dirs, source_rname, bpb->root_cluster, bpb);

    if (!dir1.found)
    {
        fprintf(stderr, "Arquivo %s não encontrado.\n", source);
        exit(EXIT_FAILURE);
    }

    // Verifica se o arquivo de destino já existe
    struct far_dir_searchres dir2 = find_in_dir(fp, dirs, dest_rname, bpb->root_cluster, bpb);

    if (dir2.found)
    {
        fprintf(stderr, "Arquivo %s já existe. Não permitido sobrescrever.\n", dest);
        exit(EXIT_FAILURE);
    }

    // Procura uma entrada livre no diretório
    bool entry_found = false;
    uint32_t cluster_number = bpb->root_cluster;
    uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;

    struct fat_dir new_dir = dir1.fdir;
    memcpy(new_dir.name, dest_rname, FAT32STR_SIZE);

    do
    {
        uint32_t cluster_address = (cluster_number - 2) * cluster_width + bpb_fdata_addr(bpb);

        if (read_bytes(fp, cluster_address, dirs, cluster_width) == RB_ERROR)
        {
            error_at_line(EXIT_FAILURE, EIO, __FILE__, __LINE__, "Erro ao acessar o diretório raiz.");
        }

        for (size_t i = 0; i < cluster_width / sizeof(struct fat_dir); i++)
        {
            if (dirs[i].name[0] == DIR_FREE_ENTRY || dirs[i].name[0] == '\0')
            {
                uint32_t dest_address = cluster_address + i * sizeof(struct fat_dir);

                // Grava a nova entrada no diretório
                (void)fseek(fp, dest_address, SEEK_SET);
                (void)fwrite(&new_dir, sizeof(struct fat_dir), 1, fp);

                entry_found = true;
                break;
            }
        }

        cluster_number = get_next_cluster(fp, cluster_number, bpb);
    } while (!entry_found && cluster_number < FAT32_EOF);

    if (!entry_found)
    {
        fprintf(stderr, "Não foi possível encontrar uma entrada livre no diretório.\n");
        exit(EXIT_FAILURE);
    }

    // Alocação de clusters para o arquivo de destino
    uint32_t fat_address = bpb_faddress(bpb);
    uint32_t data_start = bpb_fdata_addr(bpb);
    uint32_t cluster_width_bytes = bpb->bytes_p_sect * bpb->sector_p_clust;

    uint32_t source_cluster = dir1.fdir.starting_cluster;
    uint32_t dest_cluster = 0, prev_cluster = FAT32_EOF;

    size_t bytes_to_copy = dir1.fdir.file_size;

    while (bytes_to_copy > 0)
    {
        struct fat32_newcluster_info new_cluster = fat32_find_free_cluster(fp, bpb);

        if (new_cluster.cluster == 0x0)
        {
            fprintf(stderr, "Erro: Não foi possível alocar um novo cluster.\n");
            exit(EXIT_FAILURE);
        }

        if (prev_cluster != FAT32_EOF)
        {
            uint32_t prev_cluster_address = fat_address + prev_cluster * sizeof(uint32_t);
            (void)fseek(fp, prev_cluster_address, SEEK_SET);
            (void)fwrite(&new_cluster.cluster, sizeof(uint32_t), 1, fp);
        }
        else
        {
            dest_cluster = new_cluster.cluster;
        }

        prev_cluster = new_cluster.cluster;

        uint32_t eof_marker = FAT32_EOF;
        uint32_t final_cluster_address = fat_address + prev_cluster * sizeof(uint32_t);
        (void)fseek(fp, final_cluster_address, SEEK_SET);
        (void)fwrite(&eof_marker, sizeof(uint32_t), 1, fp);
    }

    // Atualiza o cluster final na FAT
    uint32_t eof_marker = FAT32_EOF;

    uint32_t final_cluster_address = fat_address + prev_cluster * sizeof(uint32_t);
    (void)fseek(fp, final_cluster_address, SEEK_SET);
    (void)fwrite(&eof_marker, sizeof(uint32_t), 1, fp);

    // Atualiza o cluster inicial e o tamanho do arquivo no diretório
    new_dir.starting_cluster = dest_cluster;
    new_dir.file_size = dir1.fdir.file_size;

    printf("cp %s → %s concluído.\n", source, dest);
}

void cat(FILE *fp, char *filename, struct fat_bpb *bpb)
{
    char fat16_rname[FAT32STR_SIZE_WNULL];

    if (cstr_to_fat16wnull(filename, fat16_rname))
    {
        fprintf(stderr, "Nome de arquivo inválido.\n");
        exit(EXIT_FAILURE);
    }

    struct fat_dir dirs[bpb->bytes_p_sect * bpb->sector_p_clust / sizeof(struct fat_dir)];
    struct far_dir_searchres dir = find_in_dir(fp, dirs, fat16_rname, bpb->root_cluster, bpb);

    if (!dir.found)
    {
        fprintf(stderr, "Arquivo %s não encontrado.\n", filename);
        exit(EXIT_FAILURE);
    }

    uint32_t cluster = dir.fdir.starting_cluster;
    uint32_t cluster_width = bpb->bytes_p_sect * bpb->sector_p_clust;
    uint32_t data_start = bpb_fdata_addr(bpb);
    size_t remaining_bytes = dir.fdir.file_size;

    while (remaining_bytes > 0 && cluster < FAT32_EOF)
    {
        uint32_t cluster_address = (cluster - 2) * cluster_width + data_start;
        size_t bytes_to_read = MIN(remaining_bytes, cluster_width);
        char buffer[cluster_width];

        read_bytes(fp, cluster_address, buffer, bytes_to_read);
        fwrite(buffer, 1, bytes_to_read, stdout);

        cluster = get_next_cluster(fp, cluster, bpb);
        remaining_bytes -= bytes_to_read;
    }
}