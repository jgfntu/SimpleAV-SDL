# FIXME: on Linux, you should use "ldconfig" after installation.

CFLAGS=-g -W -Wall -O2 `pkg-config --cflags SimpleAV sdl SDL_mixer`
LIBS=`pkg-config --libs SimpleAV sdl SDL_mixer`

BUILD_DIR=build

all: libSimpleAV_SDL.a saplayer

libSimpleAV_SDL.a: $(BUILD_DIR)/SimpleAV_SDL.o
	ar cr libSimpleAV_SDL.a $(BUILD_DIR)/SimpleAV_SDL.o

$(BUILD_DIR)/SimpleAV_SDL.o: SimpleAV_SDL.h SimpleAV_SDL.c
	gcc $(CFLAGS) -c SimpleAV_SDL.c -o $(BUILD_DIR)/SimpleAV_SDL.o
#	gcc $(CFLAGS) -fPIC -c SimpleAV_SDL.c -o $(BUILD_DIR)/SimpleAV_SDL.o

saplayer: $(BUILD_DIR)/saplayer.o libSimpleAV_SDL.a
	gcc $(LIBS) -o saplayer $(BUILD_DIR)/saplayer.o $(BUILD_DIR)/SimpleAV_SDL.o # -L. -lSimpleAV_SDL

$(BUILD_DIR)/saplayer.o: SimpleAV_SDL.h saplayer.c
	gcc $(CFLAGS) -c saplayer.c -o $(BUILD_DIR)/saplayer.o

install:
	mkdir -p /usr/local/bin /usr/local/lib /usr/local/lib/pkgconfig
	cp saplayer /usr/local/bin
	cp libSimpleAV_SDL.a /usr/local/lib
	cp SimpleAV_SDL.h /usr/local/include
	cp SimpleAV_SDL.pc /usr/local/lib/pkgconfig

uninstall:
	rm /usr/local/bin/saplayer
	rm /usr/local/lib/libSimpleAV_SDL.a
	rm /usr/local/include/SimpleAV_SDL.h
	rm /usr/local/lib/pkgconfig/SimpleAV_SDL.pc

clean:
	rm $(BUILD_DIR)/*
