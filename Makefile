CFLAGS=-g -Wall -O2 `pkg-config --cflags SimpleAV sdl SDL_mixer`
LIBS=`pkg-config --libs SimpleAV sdl SDL_mixer`

BUILD_DIR=build

all: libSimpleAV_SDL.so saplayer

libSimpleAV_SDL.so: $(BUILD_DIR)/SimpleAV_SDL.o
	gcc -shared -o libSimpleAV_SDL.so $(BUILD_DIR)/SimpleAV_SDL.o

$(BUILD_DIR)/SimpleAV_SDL.o: SimpleAV_SDL.h SimpleAV_SDL.c
	gcc $(CFLAGS) -fPIC -c SimpleAV_SDL.c -o $(BUILD_DIR)/SimpleAV_SDL.o

saplayer: $(BUILD_DIR)/saplayer.o
	gcc $(LIBS) -o saplayer $(BUILD_DIR)/saplayer.o -L. -lSimpleAV_SDL # -lSimpleAV

$(BUILD_DIR)/saplayer.o: SimpleAV_SDL.h saplayer.c
	gcc $(CFLAGS) -c saplayer.c -o $(BUILD_DIR)/saplayer.o

install:
	mkdir -p /usr/local/bin /usr/local/lib # /usr/local/lib/pkgconfig
	cp saplayer /usr/local/bin
	cp libSimpleAV_SDL.so /usr/local/lib
	cp SimpleAV_SDL.h /usr/local/include
	ldconfig

uninstall:
	rm /usr/local/bin/saplayer
	rm /usr/local/lib/libSimpleAV_SDL.so
	rm /usr/local/include/SimpleAV_SDL.h
	ldconfig

clean:
	rm $(BUILD_DIR)/*
