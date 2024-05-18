DBGFLAGS=-g -fsanitize=address -fsanitize=undefined
RELFLAGS=-O3 -march=native

override CFLAGS :=$(CFLAGS) -std=c++17 -pipe -Wall -Wextra -Wno-deprecated-declarations
override LDFLAGS :=$(LDFLAGS) -lgtest -pthread

#-fsanitize=thread
#-fsanitize=address -fsanitize=undefined

SRC=./src/test.cpp ./src/treetester.cpp ./src/bench.cpp

all: build
	
build: test

test:
	clang++ -o test.out $(SRC) $(CFLAGS) $(DBGFLAGS) $(LDFLAGS)

testrelease:
	clang++ -o test.out $(SRC) $(CFLAGS) $(RELFLAGS) $(LDFLAGS)

testrwd:
	clang++ -o test.out $(SRC) $(CFLAGS) $(RELFLAGS) $(LDFLAGS) -g

clean:
	rm test.out
