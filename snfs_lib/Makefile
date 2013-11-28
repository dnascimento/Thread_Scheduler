LIBRARIES = libsnfs.a

OBJECTS = snfs_api.o myfs.o queue.o

DEFAULT_INCLUDES = -I. -I. -I../include
CCASCOMPILE = $(CCAS) $(CCASFLAGS)
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CFLAGS)
AR = ar
CC = gcc
CCAS = gcc
CCASFLAGS = -g -O0 -Wall -m32 -I ../include
CFLAGS = -g -O0 -Wall -m32 -std=c99
ARFLAGS = cru
# DEFS = -DHAVE_CONFIG_H
RANLIB = ranlib
INCLUDES = -I ../include

all: $(LIBRARIES)

libsnfs.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

.S.o:
	$(CCASCOMPILE) -c $<

.c.o:
	$(COMPILE) -c -o $@ $<

clean: 
	rm -f *.o
	rm -f *.a

