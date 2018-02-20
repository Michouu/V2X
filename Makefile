# Makefile racine*
.Phony: clean mrproper


# Makefile racine*
.SUFFIXES:
.Phony: clean mrproper


CC?=gcc
EXEC=test4stack
SOURCE= $(wildcard *.c) initStack.h
CFLAGS = -W



all: $(SOURCE)
	@echo "Compilation : +++ All good. "
	$(CC) $(SOURCE)  -o $(EXEC) $(CFLAGS) -lpthread

	@file $(EXEC)

clean:
	@echo "+++ Running Clang Static Analyzer..."
	rm -f $(EXEC)

install:
	install -d ${DESTDIR}/usr/bin
	install -m 0755 canSend ${DESTDIR}/usr/bin
