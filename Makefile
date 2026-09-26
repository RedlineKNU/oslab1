CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

wish: wish.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f wish

.PHONY: clean
