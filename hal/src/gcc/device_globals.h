

#pragma once

#pragma GCC diagnostic push
#ifdef __clang__
// compiling with gcc this produces
// src/gcc/device_globals.h:6:32: error: unknown option after '#pragma GCC diagnostic' kind [-Werror=pragmas]                            ^
// cc1plus: error: unrecognized command line option "-Wno-return-type-c-linkage" [-Werror]
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#endif
#include "boost_asio_wrap.h"
#pragma GCC diagnostic pop

extern boost::asio::io_service device_io_service;
