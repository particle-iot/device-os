#include <cstdio>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <random>

#include <boost/algorithm/hex.hpp>

#include "service_debug.h"
#include "filesystem.h"

namespace fs = std::filesystem;

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
    if (in.fail()) {
        throw std::runtime_error("Failed to open file");
    }
    in.seekg(0, std::ios::end);
    auto size = in.tellg();
    if (size == (std::ifstream::pos_type)-1) {
        throw std::runtime_error("Failed to read file");
    }
    std::string str(size, '\0');
    in.seekg(0);
    in.read(&str[0], size);
    if (in.fail()) {
        throw std::runtime_error("Failed to read file");
    }
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

void write_file(const std::string& filename, const std::string& data)
{
    std::ofstream out(filename, std::ios::binary | std::ios::trunc);
    if (out.fail()) {
        throw std::runtime_error("Failed to create file");
    }
    out.write(data.data(), data.size());
    if (out.fail()) {
        throw std::runtime_error("Failed to write to file");
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

std::string temp_file_name(const std::string& prefix, const std::string& suffix)
{
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);
    std::string rand(16, '\0');
    for (size_t i = 0; i < rand.size(); ++i) {
        rand[i] = dist(rd);
    }
    auto path = fs::temp_directory_path().append(prefix + boost::algorithm::hex(rand) + suffix);
    return path.string();
}

} // namespace particle
