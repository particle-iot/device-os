#include <cstdio>
#include <stdexcept>
#include <fstream>

#include "service_debug.h"
#include "filesystem.h"

namespace particle {

bool exists_file(const char* filename)
{
    if (FILE *file = fopen(filename, "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

std::string read_file(const std::string& filename)
{
    std::ifstream in(filename, std::ios::binary);
    in.seekg(0, std::ios::end);
    auto size = in.tellg();
    if (size == (std::ifstream::pos_type)-1) {
        throw std::runtime_error("Failed to read file");
    }
    in.seekg(0);
    std::string str(size, '\0');
    in.read(&str[0], size);
    return str;
}

void read_file(const char* filename, void* data, size_t length)
{
    FILE *f = fopen(filename, "rb");
    if (f!=NULL) {
        length = fread(data, 1, length, f);
        INFO("read file %s length %d", filename, length);
        fclose(f);
    }
    else
    {
        throw std::invalid_argument(std::string("unable to read file '") + filename + "'");
    }
}


void write_file(const char* filename, const void* data, size_t length)
{
    FILE *f = fopen(filename, "wb");
    if (f!=NULL) {
        length = fwrite(data, 1, length, f);
        INFO("written file %s length %d", filename, length);
        fclose(f);
    }
    else
    {
        throw std::invalid_argument(std::string("unable to write file '") + filename + "'");
    }
}

} // namespace particle
