/**
 * The user module export table
 */

#include "dynalib.h"

DYNALIB_TABLE_EXTERN(user);

/**
 * The module export table. This lists the addresses of individual library dynalib jump tables.
 */
const void* const user_part_module[] = {
    DYNALIB_TABLE_NAME(user),
};

extern char link_module_start;
extern char link_module_end;

const void* const user_part_length[] = {
    &link_module_start,
    &link_module_end,
    0,                      // version?
};

