#######################################
# Makefile for building: SchedulerBot #
#######################################

.PHONY: all anneau server robot

all: anneau server robot

build/common.o: src/common.h src/common.c
	gcc -c src/common.c -o build/common.o -I./src

build/anneau.o: build/common.o src/anneau.h src/anneau.c
	gcc -c src/anneau.c -o build/anneau.o -I./src

build/server.o: build/common.o src/server.h src/server.c
	gcc -c src/server.c -o build/server.o -I./src

build/robot.o: build/common.o src/robot.h src/robot.c
	gcc -c src/robot.c -o build/robot.o -I./src

anneau: build/common.o build/anneau.o
	gcc -o run/anneau build/anneau.o -lpthread -I./src

server: build/common.o build/server.o
	gcc -o run/server build/server.o -lpthread -I./src

robot: build/common.o build/robot.o
	gcc -o run/robot build/robot.o -lpthread -I./src
	
clean:
	rm -rf build
	rm -rf run
	rm -rf start_*.sh