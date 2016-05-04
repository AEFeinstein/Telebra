# Makefile for Linux terminal application

CXX          := gcc
CXXFLAGS     := -Wall -Wextra -pedantic -g -c
INC          :=
LDLIBS       := -lpthread
LDFLAGS      :=
SRCFILES_C   := $(shell find . -maxdepth 2 -name "*.c")
SRCFILES     := $(SRCFILES_C)
OBJECTS      := $(OBJECTS) $(patsubst %.c, %.o, $(SRCFILES_C))
EXECUTABLE   := MotorDriver

all: $(SRCFILES) $(EXECUTABLE)

clean:
	-rm -f $(OBJECTS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) -o $@ $(OBJECTS) $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@
	
%.o: %.ino
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

# To print a variable to the terminal:
# make print-VARIABLE
print-%  : ; @echo $* = $($*)