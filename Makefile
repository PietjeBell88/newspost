CC=gcc
OPT_FLAGS = -O2 -Wall
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/man/man1

SOLARIS_LIBS = -lsocket -lnsl
QNX_LIBS = -lsocket

PEDANTIC_FLAGS = -g -O2 -Wall -pedantic
DEV_FLAGS = -g -O2 -Wall

all: opt

solaris: solaris-opt

clean:
	-rm -rf newspost newspost.exe core *~
	cd base ; $(MAKE) clean
	cd ui ; $(MAKE) clean
	cd enc ; $(MAKE) clean
	cd cksfv ; $(MAKE) clean
	cd parchive ; $(MAKE) clean

main:
	cd base ; $(MAKE) CC="$(CC)" CFLAGS="$(CFLAGS)"
	cd ui ; $(MAKE) CC="$(CC)" CFLAGS="$(CFLAGS)"
	cd enc ; $(MAKE) CC="$(CC)" CFLAGS="$(CFLAGS)"
	cd cksfv ; $(MAKE) CC="$(CC)" CFLAGS="$(CFLAGS)"
	cd parchive ; $(MAKE) CC="$(CC)" CFLAGS="$(CFLAGS)"
	$(CC) -o newspost $(LIBS) base/*.o ui/*.o enc/*.o cksfv/*.o \
		parchive/*.o

dev:
	$(MAKE) main CFLAGS="$(DEV_FLAGS)" LIBS=""

pedantic:
	$(MAKE) main CFLAGS="$(PEDANTIC_FLAGS)" LIBS=""

opt:
	$(MAKE) main CFLAGS="$(OPT_FLAGS)" LIBS=""
	-strip newspost


solaris-dev:
	$(MAKE) main CFLAGS="$(DEV_FLAGS)" LIBS="$(SOLARIS_LIBS)"

solaris-pedantic:
	$(MAKE) main CFLAGS="$(PEDANTIC_FLAGS)" LIBS="$(SOLARIS_LIBS)"

solaris-opt:
	$(MAKE) main CFLAGS="$(OPT_FLAGS)" LIBS="$(SOLARIS_LIBS)"
	-strip newspost

qnx:
	$(MAKE) main CFLAGS="$(OPT_FLAGS)" LIBS="$(QNX_LIBS)"
	-strip newspost

install:
	sh mkinstalldirs -m 755 $(BINDIR)
	cp newspost $(BINDIR)
	chmod 755 $(BINDIR)/newspost
	sh mkinstalldirs -m 755 $(MANDIR)
	cp man/man1/newspost.1 $(MANDIR)
	chmod 644 $(MANDIR)/newspost.1
