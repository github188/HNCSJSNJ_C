PROGRAM=watchdog 
MAKEDATE=\"$(shell date +%G.%m.%d-%H:%M)\"
CROSS=arm-linux-
CC=gcc 
CXX=g++	
CFLAGS = -Wall -Werror -Os -DMAKEDATE=$(MAKEDATE) -DMEASURE_CALIBRATION -D_REENTRENT  -mstructure-size-boundary=8

PUB=../include	
PUBINTER=../include-inter
LIBDIR=../pub	
LIBSO= -pthread	

SOURCES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SOURCES))

#define .cpp suffix rule
.SUFFIXES: .cpp

all:$(PROGRAM)
	@-cp $(PROGRAM) ../bin

$(PROGRAM):$(OBJS)
	$(CROSS)$(CXX) $(OBJS) $(CFLAGS) $(LIBSO) -L$(LIBDIR) -I$(PUB) -I$(PUBINTER) -o $@ 
	$(CROSS)strip $(PROGRAM)

.cpp.o:
	$(CROSS)$(CXX) $(CFLAGS) -I$(PUB) -I$(PUBINTER) -c $< 

 PHONY:clean
 clean:
	rm -f $(PROGRAM) *.o core *~

 tags:
	etags  *.h *.c *.cpp *.hpp

strip:
	$(CROSS)strip $(PROGRAM)
