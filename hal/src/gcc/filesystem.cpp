#include "filesystem.h"
#include <stddef.h>
#include <string.h>
#include <cstdio>

const char* rootDir = NULL;

void set_root_dir(const char* dir) {
    rootDir = dir;
}

void read_file(const char* filename, void* data, size_t length)
{
    char buf[256];
    buf[0] = 0;
    if (rootDir) {
        strcpy(buf, rootDir);
        strcat(buf, "/");
    }
    strcat(buf, filename);
    FILE *f = fopen(buf, "rb");    
    if (f!=NULL) {
        fread(data, length, 1, f);
        fclose(f);
    }
}


