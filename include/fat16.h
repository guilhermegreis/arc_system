#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>
#include <stdio.h>

#define DIR_FREE_ENTRY 0xE5

#define DIR_ATTR_READONLY 1 << 0  /* file is read only */
#define DIR_ATTR_HIDDEN 1 << 1    /* file is hidden */
#define DIR_ATTR_SYSTEM 1 << 2    /* system file (also hidden) */
#define DIR_ATTR_VOLUMEID 1 << 3  /* special entry containing disk volume lable */
#define DIR_ATTR_DIRECTORY 1 << 4 /* describes a subdirectory */
#define DIR_ATTR_ARCHIVE 1 << 5   /*  archive flag (always set when file is modified */
#define DIR_ATTR_LFN 0xf          /* not used */

#define SIG 0xAA55 /* boot sector signature -- sector is executable */

#pragma pack(push, 1)
struct fat_dir
{
    unsigned char name[11];    /* Short name + file extension */
    uint8_t attr;              /* file attributes */
    uint8_t ntres;             /* reserved for Windows NT, Set value to 0 when a file is created. */
    uint8_t creation_stamp;    /* milisecond timestamp at file creation time */
    uint16_t creation_time;    /* time file was created */
    uint16_t ctreation_date;   /* date file was created */
    uint16_t last_access_date; /* last access date (last read/written) */
    uint16_t reserved_fat32;   /* reserved for fat32 */
    uint16_t last_write_time;  /* time of last write */
    uint16_t last_write_date;  /* date of last write */
    uint32_t starting_cluster; /* starting cluster */
    uint32_t file_size;        /* 32-bit */
};

/* Boot Sector and BPB
 * Located at the first sector of the volume in the reserved region.
 * AKA as the boot sector, reserved sector or even the "0th" sector.
 */
struct fat_bpb
{
    uint8_t jmp_instruction[3]; /* Código para pular o bootstrap */
    unsigned char oem_id[8];    /* Identificador OEM */
    uint16_t bytes_p_sect;      /* Bytes por setor */
    uint8_t sector_p_clust;     /* Setores por cluster */
    uint16_t reserved_sect;     /* Setores reservados */
    uint8_t n_fat;              /* Número de FATs */
    uint16_t possible_rentries; /* Número de entradas no diretório raiz */
    uint16_t snumber_sect;      /* Número de setores pequenos */
    uint8_t media_desc;         /* Descritor de mídia */
    uint16_t sect_per_track;    /* Setores por trilha */
    uint16_t number_of_heads;   /* Número de cabeças */
    uint32_t hidden_sects;      /* Setores ocultos */
    uint32_t large_n_sects;     /* Número de setores grandes */
    uint32_t sect_per_fat;      /* Setores por FAT */
    uint32_t root_cluster;      /* Cluster inicial do diretório raiz (FAT32) */
    char fs_type[8];            /* Tipo do sistema de arquivos (FAT32, FAT16) */
};
/*
 * NOTE - Modificação
 * Motivo: IDE avisou de pragma não terminado
 * Diff: ø → #pragma pack(pop, 1)
 */
#pragma pack(pop)

int read_bytes(FILE *, unsigned int, void *, unsigned int);
void rfat(FILE *, struct fat_bpb *);

/* prototypes for calculating fat stuff */
uint32_t bpb_faddress(struct fat_bpb *);
uint32_t bpb_froot_addr(struct fat_bpb *);
uint32_t bpb_fdata_addr(struct fat_bpb *);
uint32_t bpb_fdata_sector_count(struct fat_bpb *);
uint32_t bpb_fdata_cluster_count(struct fat_bpb *bpb);

///

#define FAT32STR_SIZE_WNULL 256

#define RB_ERROR -1
#define RB_OK 0

#define FAT32_EOF 0x0FFFFFFF
#define FAT32STR_SIZE 255

#define FAT32_EOF_LO 0xfff8
#define FAT32_EOF_HI 0xffff

#endif