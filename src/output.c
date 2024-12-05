#include <stdio.h>
#include "output.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static struct pretty_int { 
    int num; 
    char* suff; 
} pretty_print(int value) {
    static char* B   = "B";
    static char* KiB = "KiB";
    static char* MiB = "MiB";

    struct pretty_int res = {
        .num  = value,
        .suff = B
    };

    if (res.num == 0) return res;

    if (res.num >= 1024) {
        res.num  /= 1024;
        res.suff = KiB;
    }

    if (res.num >= 1024) {
        res.num  /= 1024;
        res.suff = MiB;
    }

    return res;
}

void show_files(struct fat_dir *dirs) {
    struct fat_dir *cur;

    printf("ATTR  NAME           SIZE\n---------------------------\n");

    while (1) {
        cur = dirs++;

        if (cur->name[0] == 0x00) 
            break;

        if ((cur->name[0] == DIR_FREE_ENTRY) || (cur->attr == DIR_FREE_ENTRY)) 
            continue;

        if (cur->attr == DIR_ATTR_LFN)  
            continue;

        struct pretty_int size = pretty_print(cur->file_size);

        printf("0x%-2x  %.11s  %4i %s\n", cur->attr, cur->name, size.num, size.suff);
    }
}


void verbose(struct fat_bpb *bios_pb) {
    printf("Bios Parameter Block:\n");

    printf("Jump Instruction: ");
    for (int i = 0; i < 3; i++)
        printf("%02X ", bios_pb->jmp_instruction[i]);
    printf("\n");

    printf("OEM ID: %.8s\n", bios_pb->oem_id);
    printf("Bytes per Sector: %d\n", bios_pb->bytes_p_sect);
    printf("Sectors per Cluster: %d\n", bios_pb->sector_p_clust);
    printf("Reserved Sectors: %d\n", bios_pb->reserved_sect);
    printf("Number of FAT Copies: %d\n", bios_pb->n_fat);
    printf("Number of Possible Root Entries: %d\n", bios_pb->possible_rentries);
    printf("Small Number of Sectors: %d\n", bios_pb->snumber_sect);
    printf("Media Descriptor: 0x%02X\n", bios_pb->media_desc);
    printf("Sectors per FAT: %d\n", bios_pb->sect_per_fat);
    printf("Sectors per Track: %d\n", bios_pb->sect_per_track);
    printf("Number of Heads: %d\n", bios_pb->number_of_heads);

    printf("FAT Address: 0x%08X\n", bpb_faddress(bios_pb));
    printf("Root Address: 0x%08X\n", bpb_froot_addr(bios_pb));
    printf("Data Address: 0x%08X\n", bpb_fdata_addr(bios_pb));
}
