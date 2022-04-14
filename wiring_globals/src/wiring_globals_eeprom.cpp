#include "spark_wiring_eeprom.h"

// we don't use this global instance since there's no actual instance data
// Having this keeps the unoptimized build happy

EEPROMClass& __fetch_global_EEPROM()
{
	static EEPROMClass eeprom;
	return eeprom;
}
