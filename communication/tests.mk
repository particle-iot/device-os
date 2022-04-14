CXX ?= g++
CXXFLAGS ?= -g -Wall -W -Winline
CXXFLAGS += -I../hal/shared -I../services/inc -Ilib/tropicssl/include -Isrc -Itests/UnitTest++/src
RM = rm
CXXFLAGS += -DPLATFORM_ID=3

COMMON_BUILD = ../build

# Set C++ standard version
include $(COMMON_BUILD)/lang-std.mk
CXXFLAGS += $(CPPFLAGS)

.SUFFIXES: .o .cpp

name        = SparkCoreCommunication
lib         = src/lib$(name).a
test        = tests/test$(name)
testlibdir  = tests/UnitTest++
testlib     = UnitTest++
testlibpath = $(testlibdir)/lib$(testlib).a
testrunner  = tests/Main.cpp
ssllibdir   = lib/tropicssl/library
ssllib      = $(ssllibdir)/libtropicssl.a

objects = \
        src/coap.o \
        src/core_protocol.o \
        src/dsakeygen.o \
        src/events.o \
        src/handshake.o \
        src/spark_protocol_functions.o

testobjects = tests/ConstructorFixture.o \
              tests/TestHandshake.o \
              tests/TestAES.o \
              tests/TestCoAP.o \
              tests/TestQueue.o \
              tests/TestStateMachine.o \
              tests/TestCoreProtocol.o \
              tests/TestDescriptor.o \
              tests/TestUserFunctions.o \
              tests/TestEvents.o

LDFLAGS ?= -L$(ssllibdir) -ltropicssl -Lsrc -l$(name) -L$(testlibdir) -l$(testlib)

all: $(lib)

$(lib): $(objects)
	@echo Creating spark communication library...
	@$(AR) cr $(lib) $(objects)

.cpp.o:
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	-@$(RM) $(objects) $(lib) $(testobjects) 2> /dev/null


############### tests #############

test: $(lib) $(testlibpath) $(testobjects) $(ssllib)
	@$(CXX) $(testrunner) $(CXXFLAGS) $(testobjects) $(LDFLAGS) -o $(test)
	@echo running unit tests...
	@./$(test)

$(testlibpath):
	$(MAKE) -C $(testlibdir)

testclean:
	$(MAKE) -C $(testlibdir) clean


############# ssl library ###########

$(ssllib):
	$(MAKE) -C $(ssllibdir)

sslclean:
	$(MAKE) -C $(ssllibdir) clean
