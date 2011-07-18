libav_link = `pkg-config --libs libavcodec libavformat libavutil libswscale`
sdl_link = -lSDL
simpleav_link = -lSimpleAV

simpleav_sdl_full_links = $(sdl_link) $(simpleav_link) $(libav_link)

all: libSimpleAV_SDL.a saplayer

saplayer: SimpleAV_SDL.o saplayer.o
	gcc -o saplayer SimpleAV_SDL.o saplayer.o $(simpleav_sdl_full_links)

SimpleAV_SDL.o: SimpleAV_SDL.h SimpleAV_SDL.c
	gcc -c SimpleAV_SDL.c $(simpleav_sdl_full_links)

saplayer.o: SimpleAV_SDL.h saplayer.c
	gcc -c saplayer.c $(simpleav_sdl_full_links)

libSimpleAV_SDL.a: SimpleAV_SDL.o
	ar cr libSimpleAV_SDL.a SimpleAV_SDL.o

install:
	cp saplayer /usr/local/bin
	cp libSimpleAV_SDL.a /usr/local/lib
	cp SimpleAV_SDL.h /usr/local/include

uninstall:
	rm /usr/local/bin/saplayer
	rm /usr/local/lib/libSimpleAV_SDL.a
	rm /usr/local/include/SimpleAV_SDL.h

clean:
	rm *.o saplayer libSimpleAV_SDL.a