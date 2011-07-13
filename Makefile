libav_link = `pkg-config --libs libavcodec libavformat libavutil libswscale`
sdl_link = -lSDL
simpleav_link = -lSimpleAV

simpleav_sdl_full_links = $(sdl_link) $(simpleav_link) $(libav_link)

saplayer: SimpleAV_SDL.o saplayer.o
	gcc -o saplayer SimpleAV_SDL.o saplayer.o $(simpleav_sdl_full_links)

SimpleAV_SDL.o: SimpleAV_SDL.h SimpleAV_SDL.c
	gcc -c SimpleAV_SDL.c $(simpleav_sdl_full_links)

saplayer.o: SimpleAV_SDL.h saplayer.c
	gcc -c saplayer.c $(simpleav_sdl_full_links)

# FIXME: need to be implemented:
# 
# libSimpleAV_SDL.a
# install
# uninstall

clean:
	rm *.o saplayer #libSimpleAV_SDL.a