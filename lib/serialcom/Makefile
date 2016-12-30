##################################################
#
#  Makefile
#
#  email: pengshuiming@wasion.com
#
#  Copyright(c) 2016 wasion
#  All rights reserved.
#
##################################################
DEMO = Demoserial
LIBSO = libserialcom.so

CC = arm-linux-g++
SOURCES = ./src/serial.cpp
OBJS = $(SOURCES:%.cpp=%.o)

DEMO_SOURCE = ./demo.cpp
DEMO_OBJS = $(DEMO_SOURCE:%.cpp=%.o)
LDFLAGS = -L ../../../bin/lib/ -llog
DEMO_LDFLAGS = -L ../../../bin/lib/ -llog -L ./ -lserialcom

CPPFLAGS = -Wall -g -fPIC -D_REENTRANT -Wno-psabi -Wno-unknown-pragmas -Wno-format -mstructure-size-boundary=8 -Werror
INCLUDE_DIR = -I ./inc -I ../log/inc
LD_RUN_PATH = -Wl,-rpath,./lib

all:$(LIBSO) $(DEMO)
	arm-linux-strip $(LIBSO)
	@chmod 755 $(LIBSO)
	@if [ ! -d ../../../bin/lib ]; then \
	mkdir ../../../bin/lib; \
	fi
	@mv $(LIBSO) ../../../bin/lib/
	@echo "$(LIBSO) has been compiled successfully!!!"

	arm-linux-strip $(DEMO)
	@chmod 755 $(DEMO)
	@mv $(DEMO) ../../../bin
	@echo "$(DEMO) has been compiled successfully!!!"

$(LIBSO):$(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): %.o : %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@ $(INCLUDE_DIR)

$(DEMO_OBJS): %.o : %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@ $(INCLUDE_DIR)

$(DEMO):$(DEMO_OBJS)
	$(CC) -o $@ $(DEMO_OBJS) $(LD_RUN_PATH) $(DEMO_LDFLAGS)

clean:
	@rm -f src/*.o src/*~ *.o
	@rm -f ../../../bin/lib/$(LIBSO)
	@rm -f ../../../bin/$(DEMO)
