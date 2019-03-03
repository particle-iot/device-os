#ifndef CUSTOM_DYNALIB_H
#define CUSTOM_DYNALIB_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include <stdint.h>
#endif // defined(DYNALIB_EXPORT)

DYNALIB_BEGIN(custom)

// LittleFS file functions.
DYNALIB_FN(0, custom, file_remove, int(const char*))
DYNALIB_FN(1, custom, file_rename, int(const char*, const char*))
DYNALIB_FN(2, custom, file_stat, int(const char*, struct lfs_info*))
DYNALIB_FN(3, custom, file_open, int(lfs_file_t*, const char*, unsigned int))
DYNALIB_FN(4, custom, file_close, int(lfs_file_t*))
DYNALIB_FN(5, custom, file_sync, int(lfs_file_t*))
DYNALIB_FN(6, custom, file_read, int32_t(lfs_file_t*, void *, uint32_t))
DYNALIB_FN(7, custom, file_write, int32_t(lfs_file_t*, void *, uint32_t))
DYNALIB_FN(8, custom, file_seek, int32_t(lfs_file_t*, int32_t, int))
//DYNALIB_FN(9, custom, file_truncate, int(lfs_file_t*, uint32_t))
//DYNALIB_FN(10, custom, file_tell, int32_t(lfs_file_t*))
//DYNALIB_FN(11, custom, file_rewind, int(lfs_file_t*))
//DYNALIB_FN(12, custom, file_size, int32_t(lfs_file_t*))
// LittleFS directory functions.
DYNALIB_FN(9, custom, dir_mkdir, int(const char*))
DYNALIB_FN(10, custom, dir_open, int(lfs_dir_t*, const char*))
DYNALIB_FN(11, custom, dir_close, int(lfs_dir_t*))
DYNALIB_FN(12, custom, dir_read, int(lfs_dir_t*, struct lfs_info*))
//DYNALIB_FN(16, custom, dir_seek, int(lfs_dir_t*, uint32_t))
//DYNALIB_FN(17, custom, dir_tell, int32_t(lfs_dir_t*))
//DYNALIB_FN(18, custom, dir_rewind, int(lfs_dir_t*))

DYNALIB_END(custom)

#endif  /* CUSTOM_DYNALIB_H */
