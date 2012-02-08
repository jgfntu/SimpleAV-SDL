SimpleAV-SDL is the SDL binding of SimpleAV, with a futher simplified API.
It is released under WTFPL v2.

--------------------------------------------------------------------------------
### COMPILATION & (UN)INSTALL:

    $ make
    $ sudo make install

    $ sudo make uninstall

--------------------------------------------------------------------------------
### USING SIMPLEAV-SDL:

You could use pkg-config to help compiling your program that uses SimpleAV_SDL, like this:

    $ gcc saplayer.c `pkg-config --cflags --libs SimpleAV_SDL`
