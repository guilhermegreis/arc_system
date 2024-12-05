#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdio.h>

#define DIR_FREE_ENTRY 0xE5
#define DIR_ATTR_READONLY 1 << 0
#define DIR_ATTR_HIDDEN 1 << 1
#define DIR_ATTR_SYSTEM 1 << 2
#define DIR_ATTR_DIRECTORY 1 << 4
#define DIR_ATTR_ARCHIVE 1 << 5
#define FAT32STR_SIZE 11
#define FAT32STR_SIZE_WNULL 12

#pragma pack(push, 1)
struct fat_dir {
    unsigned char name[11];
    uint8_t attr;
    uint8_t ntres;
    uint8_t creation_stamp;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t reserved_fat32;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t starting_cluster;
    uint32_t file_size;
};

struct fat_bpb {
    uint8_t jmp_instruction[3];
    unsigned char oem_id[8];
    uint16_t bytes_p_sect;
    uint8_t sector_p_clust;
    uint16_t reserved_sect;
    uint8_t n_fat;
    uint32_t root_cluster;  // Altere aqui
    uint16_t fs_info_sector;
    uint32_t large_n_sects;
    uint32_t hidden_sects;
    uint16_t sect_per_track;
    uint16_t number_of_heads;
    unsigned char volume_label[11];
    unsigned char fs_type[8];
};
#pragma pack(pop)

// Prototypes
int read_bytes(FILE *, unsigned int, void *, unsigned int);
uint32_t bpb_faddress(struct fat_bpb *);
uint32_t bpb_froot_addr(struct fat_bpb *);
uint32_t bpb_fdata_addr(struct fat_bpb *);
uint32_t bpb_fdata_sector_count(struct fat_bpb *);
uint32_t bpb_fdata_cluster_count(struct fat_bpb *);

#endif
