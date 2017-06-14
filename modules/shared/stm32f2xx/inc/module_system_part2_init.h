#ifndef MODULE_SYSTEM_PART2_INIT_H
#define	MODULE_SYSTEM_PART2_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize this module. This should erase the BSS area, copy initialized
 * variables from flash to RAM.
 * Returns a pointer to the address following the statically allocated memory.
 */
void* module_system_part2_pre_init();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MODULE_SYSTEM_PART2_INIT_H */
