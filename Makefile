CXX = g++
CXXFLAGS ?= -g -Wall -W -Winline -ansi
CXXFLAGS += -Ilib/tropicssl/include -Isrc -Itests/UnitTest++/src
LDFLAGS ?=
RM = rm

.SUFFIXES: .o .cpp

name       = SparkCoreCommunication
lib        = src/lib$(name).a
test       = tests/test$(name)
testlibdir = tests/UnitTest++
testlib    = $(testlibdir)/libUnitTest++.a
testrunner = tests/Main.cpp
ssllibdir  = lib/tropicssl/library
ssllib     = $(ssllibdir)/libtropicssl.a

objects = src/handshake.o

testobjects = tests/TestHandshake.o

all: $(lib)

$(lib): $(objects) $(ssllib)
	@echo Creating $(lib) library...
	@ar cr $(lib) $(objects)

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(ssllib):
	$(MAKE) -C $(ssllibdir)

test: $(lib) $(testlib) $(testobjects)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(test) \
    $(testlib) $(lib) $(ssllib) $(testobjects) $(testrunner)
	@echo running unit tests...
	@./$(test)

$(testlib):
	$(MAKE) -C $(testlibdir)

clean:
	-@$(RM) $(objects) $(lib) $(testobjects) 2> /dev/null

sslclean:
	$(MAKE) -C $(ssllibdir) clean

testclean:
	$(MAKE) -C $(testlibdir) clean
