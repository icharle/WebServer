#CC=g++
#CCFLAGS=-std=c++11 -g -Wall -c
#CCLINK=-lpthread
#
#MAIN_SOURCE:=Main.cpp
#SOURCE:=$(wildcard *.cpp ./base/*.cpp)
#override SOURCE:=$(filter-out $(MAIN_SOURCE), $(SOURCE))
#OBJECTS:=$(patsubst %.cpp, %.o, $(SOURCE))
#
#MAIN_TARGET:=Server
#
#%.o:%.cpp
#	$(CC) $(CCFLAGS) -c -o $@ $<
#
#Main.o: Main.cpp
#	$(CC) $(CCFLAGS) -c -o $@ $^
#
#$(MAIN_TARGET):$(OBJECTS) Main.o
#	$(CC) $(CCFLAGS) -o $@ $^ $(CCLINK)
#
#clean:
#	rm $(OBJECTS) main.o -rf

PROJECT := $(shell pwd)
MAIN 	:= $(PROJECT)/Main.cpp
SRC  	:= $(wildcard $(PROJECT)/*.cpp $(PROJECT)/base/*.cpp)
override SRC := $(filter-out $(MAIN), $(SRC))
OBJECT  := $(patsubst %.cpp, %.o, $(SRC))
BIN 	:= $(PROJECT)/
TARGET  := webd
CXX     := g++
LIBS    := -lpthread
INCLUDE	:= -I ./usr/local/lib
CFLAGS  := -std=c++11 -g -pg -Wall -O3 -D_PTHREADS
CXXFLAGS:= $(CFLAGS)

all : $(BIN)/$(TARGET)

$(BIN)/$(TARGET) : $(OBJECT) $(PROJECT)/Main.o
	[ -e $(BIN) ] || mkdir $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET) | xargs rm -f

common :
	find . -name '*.o' | xargs rm -f