CXX = g++
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
protobuflib = lib/libprotobuf.a

objects = src/handshake.o \
          src/protobufs.o \
          src/spark.pb.o

testobjects = tests/TestHandshake.o \
              tests/TestProtobufs.o


all: $(lib)

$(lib): $(objects)
	@echo Creating spark communication library...
	@ar cr $(lib) $(objects)

.cpp.o:
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	-@$(RM) $(objects) $(lib) $(testobjects) 2> /dev/null


############### tests #############

test: $(lib) $(testlib) $(testobjects) $(ssllib)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(test) $(testlib) \
    $(lib) $(ssllib) $(protobuflib) $(testobjects) $(testrunner)
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
