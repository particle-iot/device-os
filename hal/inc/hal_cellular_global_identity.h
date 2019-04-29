#ifndef HAL_CELLULAR_GLOBAL_IDENTITY_H
#define HAL_CELLULAR_GLOBAL_IDENTITY_H

typedef struct
{
    uint16_t mobile_country_code;
    uint16_t mobile_network_code;
    uint16_t location_area_code;
    uint32_t cell_id;
} CellularGlobalIdentity;

#endif // HAL_CELLULAR_GLOBAL_IDENTITY_H