SRCDIR=src
BUILDDIR=build
MODE=debug # unset to enable production mode 


COMPILER = g++ -std=c++2a  -l OpenCL
ifeq ($(strip $(MODE)),$(strip debug))
	COMPILER += -g -Og -Wall
else
	COMPILER += -O3
endif

# WILD CARDS FOR COMPILATION
H_SRCS := $(wildcard $(SRCDIR)/*.hpp)
C_SRCS := $(wildcard $(SRCDIR)/*.cpp)
C_OBJS := $(C_SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# TEST, IT'S ALSO THE DEFAULT
test/test: $(C_OBJS) $(H_SRCS) $(BUILDDIR)/test.o | $(BUILDDIR)
	$(COMPILER) -o $@ $(C_OBJS) $(BUILDDIR)/test.o

$(BUILDDIR)/test.o: test/test.cpp flint.h flint.hpp 
	$(COMPILER) -c -o $(BUILDDIR)/test.o test/test.cpp

# THE OBJECT FILES
$(BUILDDIR)/%.o: src/%.cpp $(H_SRCS)
	$(COMPILER) -c -o $@ $<

# OTHER TARGETS
build:
	mkdir build

clean:
	rm build/*
