CC               ?= cc

VERSION          := 0.1.0
NAME             := r_time

BINDIR           := bin
INCDIR           := /usr/local/include
LIBDIR           := /usr/local/lib

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
$(NAME).$(VERSION).so:
	$(CC)  -c $(CFLAGS) -shared -o $(NAME).so r_time.c  $(LDFLAGS)
endif
ifeq ($(UNAME_S),Darwin)
$(NAME).$(VERSION).dylib:
	$(CC) -c $(CFLAGS) -dynamiclib -o $(NAME).dylib r_time.c  $(LDFLAGS)
endif

override LDFLAGS +=
override CFLAGS  += -Dgit_sha=$(shell git rev-parse HEAD) -O3

.PHONY: clean
clean:
	rm -f $(BINDIR)/*

