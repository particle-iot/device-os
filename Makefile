CXX ?= g++
CXXFLAGS ?= -g -Wall -W -Winline -ansi
CXXFLAGS += -Ilib/tropicssl/include -Isrc -Itests/UnitTest++/src
LDFLAGS ?=
RM = rm

.SUFFIXES: .o .cpp

name        = SparkCoreCommunication
lib         = src/lib$(name).a
test        = tests/test$(name)
testlibdir  = tests/UnitTest++
testlib     = $(testlibdir)/libUnitTest++.a
testrunner  = tests/Main.cpp
ssllibdir   = lib/tropicssl/library
ssllib      = $(ssllibdir)/libtropicssl.a

objects = src/handshake.o \
          src/coap.o \
          src/spark_protocol.o

testobjects = tests/ConstructorFixture.o \
              tests/TestHandshake.o \
              tests/TestAES.o \
              tests/TestCoAP.o \
              tests/TestQueue.o \
              tests/TestStateMachine.o \
              tests/TestSparkProtocol.o \
              tests/TestDescriptor.o \
              tests/TestUserFunctions.o


all: $(lib)

$(lib): $(objects)
	@echo Creating spark communication library...
	@$(AR) cr $(lib) $(objects)

.cpp.o:
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	-@$(RM) $(objects) $(lib) $(testobjects) 2> /dev/null


############### tests #############

test: $(lib) $(testlib) $(testobjects) $(ssllib)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(test) $(testlib) \
    $(lib) $(ssllib) $(testobjects) $(testrunner)
	@echo running unit tests...
	@./$(test)

$(testlib):
	$(MAKE) -C $(testlibdir)

testclean:
	$(MAKE) -C $(testlibdir) clean


############# ssl library ###########

$(ssllib):
	$(MAKE) -C $(ssllibdir)

sslclean:
	$(MAKE) -C $(ssllibdir) clean
