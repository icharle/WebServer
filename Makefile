PROJECT := $(shell pwd)
MAIN 	:= $(PROJECT)/Main.cpp
SOURCE  	:= $(wildcard $(PROJECT)/*.cpp $(PROJECT)/src/*.cpp $(PROJECT)/base/*.cpp)
override SOURCE := $(filter-out $(MAIN), $(SOURCE))
OBJECT  := $(patsubst %.cpp, %.o, $(SOURCE))
BIN 	:= $(PROJECT)/bin
TARGET  := webserver
CC      := g++
CCFLAGS := -std=c++11 -g -O3 -Wall -D_PTHREADS
CCLINK  := -lpthread

.PHONY: all
all : $(BIN)/$(TARGET)

$(BIN)/$(TARGET) : $(OBJECT) $(PROJECT)/Main.o
	$(CC) $(CCFLAGS) -o $@ $^ $(CCLINK)

.PHONY: clean
clean :
	rm $(OBJECT) $(BIN)/$(TARGET) $(PROJECT)/Main.o -rf