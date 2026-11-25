# Native unit tests for the pure (hardware independent) TheSpace code.
# Usage: make -C tests/native
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O1 -g
SRC      := ../../firmware/FluidNC/FluidNC/src/TheSpace
OBJS     := test_native.cpp $(SRC)/MksPendantFilter.cpp $(SRC)/GcodeBounds.cpp

test: test_native
	./test_native

test_native: $(OBJS) $(SRC)/MksPendantFilter.h $(SRC)/GcodeBounds.h
	$(CXX) $(CXXFLAGS) -I$(SRC) -o $@ $(OBJS)

clean:
	rm -f test_native

.PHONY: test clean
