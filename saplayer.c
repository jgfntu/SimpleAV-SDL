/*
 * saplayer.c
 * Copyright (C) 2011 by Chenguang Wang(wecing)
 *
 * This file is part of SimpleAV-SDL.
 *
 * SimpleAV-SDL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleAV-SDL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleAV-SDL. If not, see <http://www.gnu.org/licenses/>.
 */

#include <SDL/SDL.h>
#include <stdio.h>

#include "SimpleAV_SDL.h"

int main(int argc, char *argv[])
{
     if(argc != 2)
     {
          fprintf(stderr, "Usage:\nsaplayer <filename>\n");
          return 1;
     }
     
     SDL_Init(SDL_INIT_EVERYTHING);
     SASDL_init();
     SASDLContext *sasdl_ctx = SASDL_open(argv[1]);
     if(sasdl_ctx == NULL)
     {
          fprintf(stderr, "failed to open video file?\n");
          SDL_Quit();
          return 1;
     }

     // or use Mix_SetPostMix, if you like.
     // but remember to call Mix_SetPostMix(NULL, NULL) or
     // Mix_CloseAudio() before calling SASDL_Close().

     SDL_AudioSpec wanted_spec;
     // FIXME: this looks dirty. I HATE IT.
     wanted_spec.freq = sasdl_ctx->sa_ctx->a_codec_ctx->sample_rate;
     wanted_spec.format = AUDIO_S16SYS;
     wanted_spec.channels = sasdl_ctx->sa_ctx->a_codec_ctx->channels;
     wanted_spec.silence = 0;
     wanted_spec.samples = 512;
     // set your own callback here if you like.
     wanted_spec.callback = SASDL_audio_decode;
     wanted_spec.userdata = sasdl_ctx;

     if(SDL_OpenAudio(&wanted_spec, NULL) < 0)
     {
          fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
          SASDL_close(sasdl_ctx);
          SDL_Quit();
          return 1;
     }
     SDL_PauseAudio(0);

     double delta = 0.0f;
     SDL_Event event;
     int width = SASDL_get_video_width(sasdl_ctx);
     int height = SASDL_get_video_height(sasdl_ctx);
     
     SDL_Surface *screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
     int (*get_event)(SDL_Event *) = SDL_PollEvent;
     
     printf("video duration: %.3fs\n", SASDL_get_video_duration(sasdl_ctx));
     SASDL_play(sasdl_ctx);
     
     while(SASDL_eof(sasdl_ctx) == FALSE)
     {
          SASDL_draw(sasdl_ctx, screen);
          SDL_Flip(screen);

          while(get_event(&event))
               if(event.type == SDL_QUIT) {
                    SASDL_close(sasdl_ctx);
                    goto PROGRAM_QUIT;
               } else if(event.type == SDL_KEYDOWN) {
                    switch(event.key.keysym.sym) {
                    case SDLK_LEFT:
                         delta = -10.0;
                         break;
                    case SDLK_RIGHT:
                         delta = 10.0;
                         break;
                    case SDLK_UP:
                         delta = -60.0;
                         break;
                    case SDLK_DOWN:
                         delta = 60.0;
                         break;
                    case SDLK_SPACE:
                         if(SASDL_get_video_status(sasdl_ctx) == SASDL_is_playing)
                         {
                              SASDL_pause(sasdl_ctx);
                              get_event = SDL_WaitEvent;
                              continue;
                         } else
                         {
                              SASDL_play(sasdl_ctx);
                              get_event = SDL_PollEvent;
                              goto NEXT_LOOP;
                         }
                    case SDLK_s:
                         SASDL_stop(sasdl_ctx);
                         SASDL_draw(sasdl_ctx, screen); // fill screen with black
                         SDL_Flip(screen);
                         get_event = SDL_WaitEvent;
                         continue;
                    default:
                         // ignore this event. get the next one.
                         continue;
                    }

                    if(SASDL_seek(sasdl_ctx, SASDL_get_video_clock(sasdl_ctx) + delta) < 0)
                    {
                         SASDL_close(sasdl_ctx);
                         goto PROGRAM_QUIT;
                    }

                    SASDL_draw(sasdl_ctx, screen);
                    SDL_Flip(screen);
               }

          SASDL_wait_for_next_frame(sasdl_ctx);
     NEXT_LOOP:;
     }
                    
PROGRAM_QUIT:
     SDL_CloseAudio();
     SDL_Quit();
     return 0;
}
