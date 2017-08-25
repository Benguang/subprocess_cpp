CXX=g++
CXXFLAGS=-MD -std=c++11 -O2 -g -pthread -pedantic -Wall -Wextra -Wfatal-errors
LDFLAGS=-lpthread

PROGRAM=$(shell basename $$(pwd))
SOURCES=$(shell find . -name '*.cpp')

OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))
DEPENDENCIES=$(patsubst %.cpp, %.d, $(SOURCES))

$(PROGRAM): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: run
run: $(PROGRAM)
	./$(PROGRAM)

.PHONY: valgrind
valgrind:
	valgrind ./$(PROGRAM)

.PHONY: clean
clean:
	rm -f $(OBJECTS)
	rm -f $(DEPENDENCIES)
	rm -f $(PROGRAM)

-include $(DEPENDENCIES)
