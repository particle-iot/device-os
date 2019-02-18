#include "spark_wiring_ipaddress.h"

#if !HAL_USE_SOCKET_HAL_POSIX
const IPAddress INADDR_NONE(0, 0, 0, 0);
#endif /* !HAL_USE_SOCKET_HAL_POSIX */
