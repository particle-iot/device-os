#include "device_code.h"
#include <string.h>

bool fetch_or_generate_ssid_prefix(device_code_t* result) {
	strcpy(reinterpret_cast<char*>(result->value), "Electron");
	result->length = 8;
	return true;
}
