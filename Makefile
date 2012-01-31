# Makefile for camlsh

#Config
CC=cc
OPTS=
LIBDIR=/usr/local/lib/caml-light
BINDIR=/usr/local/bin

# Test to see whether ranlib exists on the machine
RANLIBTEST=test -f /usr/bin/ranlib -o -f /bin/ranlib

# How to invoke ranlib
RANLIB=ranlib


# Compilation options
CFLAGS=-O -I./cl75/src/runtime -lreadline
CAMLCOMP=camlrun ./cl75/src/camlcomp
INCLUDES=-stdlib ./cl75/src/lib -I ./cl75/src/compiler  -I ./cl75/src/toplevel
COMPFLAGS=-W -O fast $(INCLUDES)
LINKFLAGS=-g $(INCLUDES)
COBJS=readline.o

all: libreadline.a readline.zo camlsh

libreadline.a: $(COBJS)
	rm -f libreadline.a
	ar rc libreadline.a $(COBJS)
	if test -f /bin/ranlib -o -f /usr/bin/ranlib; then ranlib libreadline.a; fi

camlsh: libreadline.a readline.zo topmain.zo
	camlmktop -o camlsh -custom topmain.zo readline.zo libreadline.a -lreadline

clean:
	rm -f libreadline.a *.o *.zo *.zi *.out camlsh
	rm -f *.dvi *.log *.aux

install:
	cp libreadline.a $(LIBDIR)/libreadline.a
	if $(RANLIBTEST); then cd $(LIBDIR); $(RANLIB) libreadline.a; else true; fi
	cp readline.zo readline.zi $(LIBDIR)
	cp camlsh $(LIBDIR)

.SUFFIXES: .ml .mli .zo .zi .tex

.mli.zi:
	$(CAMLCOMP) $(COMPFLAGS) $<

.ml.zo:
	$(CAMLCOMP) $(COMPFLAGS) $<

depend:
	mv Makefile Makefile.bak; \
	(sed -n -e '1,/^### DO NOT DELETE THIS LINE/p' Makefile.bak; \
         gcc -MM $(CFLAGS) *.c; \
         ./cl75/src/tools/camldep *.mli *.ml) > Makefile
	rm -f Makefile.bak

### DO NOT DELETE THIS LINE
readline.o: readline.c
readline.zo: readline.zi
topmain.zo: ./cl75/src/compiler/location.zi ./cl75/src/compiler/interntl.zo \
    ./cl75/src/compiler/config.zi ./cl75/src/compiler/compiler.zo ./cl75/src/compiler/misc.zo \
    ./cl75/src/toplevel/do_phr.zo
