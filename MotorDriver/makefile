# Makefile for Linux terminal application

CXX          := gcc
CXXFLAGS     := -Wall -Wextra -pedantic -g -c
INC          :=
LDLIBS       := -lpthread
LDFLAGS      :=
SRCFILES_CPP := $(shell find . -maxdepth 2 -name "*.c")
SRCFILES     := $(SRCFILES_CPP)
OBJECTS      := $(OBJECTS) $(patsubst %.cpp, %.o, $(SRCFILES_CPP))
EXECUTABLE   := MotorDriver

all: $(SRCFILES) $(EXECUTABLE)

clean:
	-rm -f $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) -o $@ $(OBJECTS) $(LDLIBS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@
	
%.o: %.ino
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

# To print a variable to the terminal:
# make print-VARIABLE
print-%  : ; @echo $* = $($*)