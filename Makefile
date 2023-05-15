SRCMAIN=src/main.c
CFLAGS=-Wall -Wextra
LDFLAGS=$(shell pkg-config --cflags --libs gtk4)

build:
	gcc ${SRCMAIN} ${CFLAGS} ${LDFLAGS} -o output

