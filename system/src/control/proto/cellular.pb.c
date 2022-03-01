/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9 at Mon Jun 10 21:00:54 2019. */

#include "cellular.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t particle_ctrl_cellular_AccessPoint_fields[5] = {
    PB_FIELD(  1, STRING  , SINGULAR, CALLBACK, FIRST, particle_ctrl_cellular_AccessPoint, apn, apn, 0),
    PB_FIELD(  2, STRING  , SINGULAR, CALLBACK, OTHER, particle_ctrl_cellular_AccessPoint, user, apn, 0),
    PB_FIELD(  3, STRING  , SINGULAR, CALLBACK, OTHER, particle_ctrl_cellular_AccessPoint, password, user, 0),
    PB_FIELD(  4, BOOL    , SINGULAR, STATIC  , OTHER, particle_ctrl_cellular_AccessPoint, use_defaults, password, 0),
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_SetAccessPointRequest_fields[3] = {
    PB_FIELD(  1, UENUM   , SINGULAR, STATIC  , FIRST, particle_ctrl_cellular_SetAccessPointRequest, sim_type, sim_type, 0),
    PB_FIELD(  2, MESSAGE , SINGULAR, STATIC  , OTHER, particle_ctrl_cellular_SetAccessPointRequest, access_point, sim_type, &particle_ctrl_cellular_AccessPoint_fields),
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_SetAccessPointReply_fields[1] = {
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_GetAccessPointRequest_fields[2] = {
    PB_FIELD(  1, UENUM   , SINGULAR, STATIC  , FIRST, particle_ctrl_cellular_GetAccessPointRequest, sim_type, sim_type, 0),
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_GetAccessPointReply_fields[2] = {
    PB_FIELD(  1, MESSAGE , SINGULAR, STATIC  , FIRST, particle_ctrl_cellular_GetAccessPointReply, access_point, access_point, &particle_ctrl_cellular_AccessPoint_fields),
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_SetActiveSimRequest_fields[2] = {
    PB_FIELD(  1, UENUM   , SINGULAR, STATIC  , FIRST, particle_ctrl_cellular_SetActiveSimRequest, sim_type, sim_type, 0),
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_SetActiveSimReply_fields[1] = {
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_GetActiveSimRequest_fields[1] = {
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_GetActiveSimReply_fields[2] = {
    PB_FIELD(  1, UENUM   , SINGULAR, STATIC  , FIRST, particle_ctrl_cellular_GetActiveSimReply, sim_type, sim_type, 0),
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_GetIccidRequest_fields[1] = {
    PB_LAST_FIELD
};

const pb_field_t particle_ctrl_cellular_GetIccidReply_fields[2] = {
    PB_FIELD(  1, STRING  , SINGULAR, CALLBACK, FIRST, particle_ctrl_cellular_GetIccidReply, iccid, iccid, 0),
    PB_LAST_FIELD
};



/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_32BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in 8 or 16 bit
 * field descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(particle_ctrl_cellular_SetAccessPointRequest, access_point) < 65536 && pb_membersize(particle_ctrl_cellular_GetAccessPointReply, access_point) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_particle_ctrl_cellular_AccessPoint_particle_ctrl_cellular_SetAccessPointRequest_particle_ctrl_cellular_SetAccessPointReply_particle_ctrl_cellular_GetAccessPointRequest_particle_ctrl_cellular_GetAccessPointReply_particle_ctrl_cellular_SetActiveSimRequest_particle_ctrl_cellular_SetActiveSimReply_particle_ctrl_cellular_GetActiveSimRequest_particle_ctrl_cellular_GetActiveSimReply_particle_ctrl_cellular_GetIccidRequest_particle_ctrl_cellular_GetIccidReply)
#endif

#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_16BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in the default
 * 8 bit descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(particle_ctrl_cellular_SetAccessPointRequest, access_point) < 256 && pb_membersize(particle_ctrl_cellular_GetAccessPointReply, access_point) < 256), YOU_MUST_DEFINE_PB_FIELD_16BIT_FOR_MESSAGES_particle_ctrl_cellular_AccessPoint_particle_ctrl_cellular_SetAccessPointRequest_particle_ctrl_cellular_SetAccessPointReply_particle_ctrl_cellular_GetAccessPointRequest_particle_ctrl_cellular_GetAccessPointReply_particle_ctrl_cellular_SetActiveSimRequest_particle_ctrl_cellular_SetActiveSimReply_particle_ctrl_cellular_GetActiveSimRequest_particle_ctrl_cellular_GetActiveSimReply_particle_ctrl_cellular_GetIccidRequest_particle_ctrl_cellular_GetIccidReply)
#endif


/* @@protoc_insertion_point(eof) */
