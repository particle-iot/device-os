#include "platform_ncp.h"

MeshNCPIdentifier platform_current_ncp_identifier() {
	// todo - retrieve the NCP identifier from the OTP region.
	return MESH_NCP_UNKNOWN;
}

hal_update_complete_t platform_ncp_update_module(const hal_module_t* module) {
	return HAL_UPDATE_ERROR;
}
