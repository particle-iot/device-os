CXX = g++
CXXFLAGS ?= -g -Wall -W -Winline -ansi
LDFLAGS ?= -Itests/UnitTest++/src -Isrc
RM = rm

.SUFFIXES: .o .cpp

name = SparkCoreCommunication
lib = lib$(name).a
test = test$(name)
testlib = tests/UnitTest++/libUnitTest++.a
testrunner = tests/Main.cpp

src = src/handshake.cpp

objects = $(patsubst %.cpp, %.o, $(src))

all: $(lib)

$(lib): $(objects)
	@echo Creating $(lib) library...
	@ar cr $(lib) $(objects)

clean:
	-@$(RM) $(objects) $(dependencies) $(lib) 2> /dev/null

test: $(lib) $(testlib)
	@$(CXX) $(LDFLAGS) -o $(test) $(testlib) $(lib) $(testrunner)
	@echo running unit tests...
	@./$(test)

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
