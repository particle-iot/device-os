/*
 * File:   filesystem.h
 * Author: mat1
 *
 * Created on December 2, 2014, 11:49 AM
 */

#ifndef FILESYSTEM_H
#define	FILESYSTEM_H

#include <string>
#include <cstddef>

namespace particle {

std::string read_file(const std::string& filename);
void read_file(const char* filename, void* data, size_t length);
void write_file(const std::string& filename, const std::string& data);
void write_file(const char* filename, const void* data, size_t length);
bool exists_file(const char* filename);
std::string temp_file_name(const std::string& prefix = "", const std::string& suffix = "");

} // namespace particle

#endif	/* FILESYSTEM_H */
