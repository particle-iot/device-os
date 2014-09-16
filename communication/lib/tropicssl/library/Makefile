
# Also see "include/tropicssl/config.h"

# Suggested value for TROPICSSL_CFLAGS_EXTRA_WARNINGS to pass
# on the command line, for gcc (GCC) 4.4.6 20120305
# (Red Hat 4.4.6-4):
#
# make  all 'TROPICSSL_CFLAGS_EXTRA_WARNINGS= -Wextra -Wformat=2
#   -fstrict-aliasing -Wstrict-aliasing -Wfloat-equal
#   -Wundef -Wpointer-arith -Wbad-function-cast
#   -Wcast-qual -Wcast-align -Wwrite-strings -Waddress
#   -Waggregate-return -Wstrict-prototypes
#   -Wold-style-declaration -Wold-style-definition
#   -Wmissing-parameter-type -Wmissing-prototypes
#   -Wmissing-declarations -Wmissing-field-initializers
#   -Wmissing-noreturn -Wmissing-format-attribute -Wpacked
#   -Wredundant-decls -Wnested-externs'
#

CFLAGS	= -I../include -D_FILE_OFFSET_BITS=64 -Wall $(TROPICSSL_CFLAGS_EXTRA_WARNINGS)
OFLAGS	= -Os

ifndef TROPICSSL_CFLAGS_NO_WARNINGS_AS_ERRORS
CFLAGS += -Werror
endif # TROPICSSL_CFLAGS_NO_WARNINGS_AS_ERRORS

# MicroBlaze specific options:
# CFLAGS += -mno-xl-soft-mul -mxl-barrel-shift

# To compile on Plan9:
# CFLAGS += -D_BSD_EXTENSION

# To compile as a shared library:
ifdef TROPICSSL_SHARED
CFLAGS += -fPIC
endif # TROPICSSL_SHARED

DLEXT=so
# OSX shared library extension:
# DLEXT=dylib

# OBJS=	aes.o		arc4.o		base64.o	\
# 	bignum.o	certs.o		debug.o		\
# 	des.o		dhm.o		havege.o	\
# 	md2.o		md4.o		md5.o		\
# 	net.o		padlock.o	rsa.o		\
# 	sha1.o		sha2.o		sha4.o		\
# 	ssl_cli.o	ssl_srv.o	ssl_tls.o	\
# 	timing.o	x509parse.o	xtea.o		\
# 	camellia.o
OBJS = aes.o bignum.o padlock.o rsa.o sha1.o

.PHONY: all

ifdef TROPICSSL_SHARED
all: shared
else # TROPICSSL_SHARED
all: static
endif # TROPICSSL_SHARED

.PHONY: static

static: libtropicssl.a

.PHONY: shared

shared: libtropicssl.$(DLEXT)

libtropicssl.a: $(OBJS)
	@echo "  AR    $@"
	$(AR) r $@ $(OBJS)
	@echo "  RL    $@"
	ranlib $@

libtropicssl.so: libtropicssl.a
	@echo "  LD    $@"
	$(CC) -shared -Wl,-soname,$@ -o $@ $(OBJS)

libtropicssl.dylib: libtropicssl.a
	@echo "  LD    $@"
	$(CC) -dynamiclib -o $@ $(OBJS)

.c.o:
	@echo "  CC    $<"
	$(CC) $(CFLAGS) $(OFLAGS) -c $<

.PHONY: clean

clean:
	rm -f *.o libtropicssl.*

