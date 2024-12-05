#include <string.h>
#include <ctype.h>
#include "support.h"

bool cstr_to_fat32wnull(char *filename, char output[12]) {
    memset(output, ' ', 11);  
    char *strptr = filename;
    char *dot = strchr(filename, '.');  

    if (dot == NULL) return true;  

    int i;
    for (i = 0; strptr != dot && i < 8; strptr++, i++) {
        output[i] = toupper(*strptr);
    }

    for (; i < 8; i++) {
        output[i] = ' ';
    }

    strptr = dot + 1;
    for (i = 8; i < 11 && *strptr != '\0'; strptr++, i++) {
        output[i] = toupper(*strptr);
    }

    output[11] = '\0';

    return false;  
