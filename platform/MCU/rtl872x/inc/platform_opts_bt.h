#ifndef __PLATFORM_OPTS_BT_H__
#define __PLATFORM_OPTS_BT_H__
#include "platform_autoconf.h"

#if defined CONFIG_BT && CONFIG_BT
#define CONFIG_FTL_ENABLED
#endif

#if defined CONFIG_BT_SCATTERNET && CONFIG_BT_SCATTERNET
#define CONFIG_BT_PERIPHERAL	1
#define CONFIG_BT_CENTRAL		1
#endif

#if defined CONFIG_BT_CENTRAL && CONFIG_BT_CENTRAL
#define CONFIG_BT_USER_COMMAND	0
#endif

#endif // __PLATFORM_OPTS_BT_H__

