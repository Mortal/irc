CXX=clang++
CXXFLAGS=-g -std=c++0x
LDFLAGS=-lboost_system -pthread

all: main

main.o: main.cpp irc.h parser.h linewise_socket.h stream_slurper.h

parser.o: parser.cpp parser.h

main: main.o parser.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

parsertest: parsertest.o parser.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
