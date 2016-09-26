/*
 * File:   filesystem.h
 * Author: mat1
 *
 * Created on December 2, 2014, 11:49 AM
 */

#ifndef FILESYSTEM_H
#define	FILESYSTEM_H

#include <stddef.h>

void read_file(const char* filename, void* data, size_t length);
void write_file(const char* filename, const void* data, size_t length);
bool exists_file(const char* filename);


void set_root_dir(const char* dir);


#endif	/* FILESYSTEM_H */

