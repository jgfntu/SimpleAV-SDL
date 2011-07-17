/*
 * saplayer.c
 * Copyright (C) 2011 by Chenguang Wang(wecing)
 *
 * This file is part of SimpleAV.
 *
 * SimpleAV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleAV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleAV. If not, see <http://www.gnu.org/licenses/>.
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

     double delta = 0.0f;
     SDL_Event event;
     int width = SASDL_get_width(sasdl_ctx);
     int height = SASDL_get_height(sasdl_ctx);
     
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
     SDL_Quit();
     return 0;
}
