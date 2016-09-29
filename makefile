CC = g++
DEBUG = -g -Wall -O0 -pedantic-errors
CFLAGS=-std=c++11
LFLAGS=-std=c++11
COMPILE=$(CC) $(DEBUG) $(CFLAGS) -c
LINK=$(CC) $(DEBUG) $(LFLAGS)
RM=rm -f

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)
EXE=$(OBJ:%.o=%)

.SUFFIXES: .cpp .o
.PHONY: clean all

all: $(EXE)

$(filter %.o, $(OBJ)): %.o: %.cpp
	$(COMPILE) $<

$(filter %, $(EXE)): %: %.o
	$(LINK) -o $@ $<

clean:
	@$(foreach exe, $(EXE), echo $(RM) $(exe); ($(RM) $(exe)) || exit;)
	@$(foreach obj, $(OBJ), echo $(RM) $(obj); ($(RM) $(obj)) || exit;)