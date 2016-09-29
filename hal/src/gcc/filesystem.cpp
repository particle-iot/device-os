#include <string>
#include <string.h>
#include <stddef.h>
#include <cstdio>
#include <stdexcept>
#include "service_debug.h"
#include "filesystem.h"

const char* rootDir = NULL;

using namespace std;

void set_root_dir(const char* dir) {
    rootDir = dir;
}

bool exists_file(const char* filename)
{
    char buf[256];
    buf[0] = 0;
    if (rootDir) {
        strcpy(buf, rootDir);
        strcat(buf, "/");
    }
    strcat(buf, filename);
    if (FILE *file = fopen(buf, "r")) {
		fclose(file);
		return true;
	} else {
		return false;
	}
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
        length = fread(data, 1, length, f);
        INFO("read file %s length %d", buf, length);
        fclose(f);
    }
    else
    {
        throw invalid_argument(string("unable to read file '") + buf + "'");
    }
}


void write_file(const char* filename, const void* data, size_t length)
{
    char buf[256];
    buf[0] = 0;
    if (rootDir) {
        strcpy(buf, rootDir);
        strcat(buf, "/");
    }
    strcat(buf, filename);
    FILE *f = fopen(buf, "wb");
    if (f!=NULL) {
        length = fwrite(data, 1, length, f);
        INFO("written file %s length %d", buf, length);
        fclose(f);
    }
    else
    {
        throw invalid_argument(string("unable to write file '") + buf + "'");
    }
}

