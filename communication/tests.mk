CXX ?= g++
CXXFLAGS ?= -g -Wall -W -Winline -ansi
CXXFLAGS += -Ilib/tropicssl/include -Isrc -Itests/UnitTest++/src
RM = rm

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

objects = src/handshake.o \
          src/coap.o \
          src/particle_protocol.o \
          src/events.o \
          src/functions.o

testobjects = tests/ConstructorFixture.o \
              tests/TestHandshake.o \
              tests/TestAES.o \
              tests/TestCoAP.o \
              tests/TestQueue.o \
              tests/TestStateMachine.o \
              tests/TestSparkProtocol.o \
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
