

#include "spark_wiring_cellular.h"

#if Wiring_Cellular

namespace spark {
    CellularClass Cellular;
    NetworkClass& Network = Cellular;
}

#endif
