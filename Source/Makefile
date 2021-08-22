## Nothing but -*- Text -*- mode is good enough.
##
## SAD makefile
##

## Never studied what I'm supposed to do in the Makefile..

#CC=cc
#ENV_CFLAGS= -Aa -DSYSV -D_HPUX_SOURCE
#COPTIONS= -O
#CFLAGS= $(ENV_CFLAGS) $(COPTIONS)

##CC=gcc
##CFLAGS=-pipe -O -g -fstrength-reduce -fomit-frame-pointer -finline-functions

CC= gcc
CFLAGS= -O2 -g -DSYSV

MV= /bin/mv

all:	sad xcom xsym xfmt sport

sad:	sad.o globals.o code.o rpl.o misc.o output.o formats.o macros.o
	$(CC) -o sad sad.o globals.o code.o rpl.o misc.o output.o \
	formats.o macros.o
sport:	sport.o
	$(CC) -o sport sport.o
xcom:	xcom.o misc.o output.o globals.o
	$(CC) -o xcom xcom.o misc.o output.o globals.o
xsym:	xsym.o misc.o output.o globals.o
	$(CC) -o xsym xsym.o misc.o output.o globals.o
xfmt:	xfmt.o formats.o misc.o output.o globals.o
	$(CC) -o xfmt xfmt.o formats.o misc.o output.o globals.o

clean:
	rm -f sad.o globals.o code.o rpl.o misc.o output.o formats.o \
	macros.o sport.o xcom.o xsym.o xfmt.o

strip:	all
	strip sad sport xcom xsym xfmt

install:	strip
	$(MV) sad sport xcom xsym xfmt $(HOME)/bin

moveup:
	$(MV) sad sport xcom xsym xfmt ..

sad.o:	sad.c formats.h macros.h globals.h
	$(CC) $(CFLAGS) -c sad.c
xfmt.o:	xfmt.c globals.h formats.h
	$(CC) $(CFLAGS) -c xfmt.c
xcom.o:	xcom.c globals.h
	$(CC) $(CFLAGS) -c xcom.c
xsym.o:	xsym.c globals.h
	$(CC) $(CFLAGS) -c xsym.c
code.o:	code.c globals.h
	$(CC) $(CFLAGS) -c code.c
rpl.o:	rpl.c macros.h formats.h globals.h
	$(CC) $(CFLAGS) -c rpl.c
globals.o: globals.c globals.h
	$(CC) $(CFLAGS) -c globals.c
misc.o:	misc.c globals.h
	$(CC) $(CFLAGS) -c misc.c
output.o: output.c globals.h
	$(CC) $(CFLAGS) -c output.c
formats.o: formats.c formats.h globals.h
	$(CC) $(CFLAGS) -c formats.c
macros.o: macros.c macros.h globals.h
	$(CC) $(CFLAGS) -c macros.c
sport.o: sport.c
	$(CC) $(CFLAGS) -c sport.c


