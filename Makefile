CXX=clang++
CXXFLAGS=-g -std=c++0x
LDFLAGS=-lboost_system -pthread

main: main.cpp irc.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
