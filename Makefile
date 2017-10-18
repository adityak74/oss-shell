# Makefile for compiling oss and user program
# Aditya Karnam
# October, 2017
# Added extra function to clen .log and .out files for easy testing

CC	= gcc
CFLAGS = -lpthread
TARGETS	= oss user 
OBJS	= oss.o user.o
SRCDIR  = src
HEADER = shm_header.h

all: $(TARGETS)

$(TARGETS): % : %.o
		$(CC) -o $@ $<

$(OBJS) : %.o : $(SRCDIR)/%.c
		$(CC) -c $<

clean:
		/bin/rm -f *.o $(TARGETS) *.log *.out

cleanobj: 
		/bin/rm -f *.o