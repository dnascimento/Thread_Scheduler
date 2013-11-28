PROGRAMS = server 

INCLUDES = -I . -I ../include
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS)
CC = gcc
CFLAGS = -g -O0 -Wall -m32 -std=c99
DEFS = -DHAVE_CONFIG_H -DSIMULATE_IO_DELAY 
LIBSTHREAD = ../sthread_lib/libsthread.a 
LIBSOCKS =  -lpthread -lnsl
OBJECTS = server.o snfs.o fs.o block.o io_delay.o cache.o list.o hash.o


all: libs $(PROGRAMS)

.SUFFIXES: .c .o

server: $(OBJECTS)
	$(CC) $(CFLAGS) ../sthread_lib/sthread_start.o -o server $(OBJECTS) $(LIBSTHREAD) $(LIBSOCKS)

libs:
	$(MAKE) libsthread.a -C ../sthread_lib

.c.o:
	$(COMPILE) -c -o $@ $<


clean: clean-PROGRAMS
	rm -f *.o

clean-PROGRAMS:
	@list='$(PROGRAMS)'; for p in $$list; do \
	  f=`echo $$p|sed 's/$$//'`; \
	  echo " rm -f $$p $$f"; \
	  rm -f $$p $$f ; \
	done


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
