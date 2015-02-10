
/**
 * This is only ever used as a pointer, so we don't need the full struct definition.
 */
typedef struct resource_hnd_t {
    
} resource_hnd_t;

/**
 * By overriding the default wwd_firmware_image_resource() implementation in WICED, 
 * we can use the dynamicly linked function to retrieve the address of the structure.
 * @return 
 */
const resource_hnd_t* wwd_firmware_image_resource(void)
{
    return 0;
}
