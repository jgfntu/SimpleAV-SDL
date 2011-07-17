/*
 * SimpleAV_SDL.c
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

#include <SimpleAV.h>
#include "SimpleAV_SDL.h"

#include <stdlib.h>
#include <libswscale/swscale.h>

////
//// FIXME: description in this file is out of date.
////      update them now.
////

int SASDL_init(void)
{
     // FIXME: "silent" mode?
     SA_init();
     return 0;
}

SASDLContext *SASDL_open(char *filename)
{
     SAContext *sa_ctx = SA_open(filename);
     if(sa_ctx == NULL)
     {
          fprintf(stderr, "SA_open failed.\n");
          return NULL;
     }

     SASDLContext *sasdl_ctx = malloc(sizeof(SASDLContext));
     if(sasdl_ctx == NULL)
     {
          SA_close(sa_ctx);
          fprintf(stderr, "malloc for sasdl_ctx failed!\n");
          return NULL;
     } else
     {
          memset(sasdl_ctx, 0, sizeof(SASDLContext));
          
          sasdl_ctx->status = SASDL_is_stopped;
          sasdl_ctx->video_start_at = 0.0f;
          sasdl_ctx->start_time = 0.0f;
          
          sasdl_ctx->sa_ctx = sa_ctx;
     }
     
     int width = SASDL_get_width(sasdl_ctx);
     int height = SASDL_get_height(sasdl_ctx);

     // FIXME: is SWS_FAST_BILINEAR the fastest?
     //        we don't change the size here. so speed is everything.
     struct SwsContext *swsctx = sws_getContext(width, height, sa_ctx->v_codec_ctx->pix_fmt,
                                                width, height, PIX_FMT_RGB32, SWS_FAST_BILINEAR,
                                                NULL, NULL, NULL);
     if(swsctx == NULL)
     {
          SASDL_close(sasdl_ctx);
          fprintf(stderr, "sws_getContext failed!\n");
          return NULL;
     } else
          sasdl_ctx->swsctx = swsctx;

     sasdl_ctx->ap_lock = SDL_CreateMutex();

     /*
     SDL_AudioSpec wanted_spec;
     wanted_spec.freq = sa_ctx->a_codec_ctx->sample_rate;
     wanted_spec.format = AUDIO_S16SYS;
     wanted_spec.channels = sa_ctx->a_codec_ctx->channels;
     wanted_spec.silence = 0;
     wanted_spec.samples = 512;
     wanted_spec.callback = SASDL_audio_callback; // FIXME: set user's own callback?
     wanted_spec.userdata = sa_ctx;

     if(SDL_OpenAudio(&wanted_spec, NULL) < 0)
     {
          fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
          SASDL_close(sa_ctx);
          return NULL;
     }

     // don't forget to call SDL_CloseAudio().
     */

     sasdl_ctx->frame_cur = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32,
                                                 0, 0, 0, 0);
     if(sasdl_ctx->frame_cur == NULL) {
          SASDL_close(sasdl_ctx);
          fprintf(stderr, "failed to create SDL RGB surface.\n");
          return NULL;
     }

     _SASDL_fill_frame_cur_black(sasdl_ctx);

     return sasdl_ctx;
}

int SASDL_close(SASDLContext *sasdl_ctx)
{
     if(sasdl_ctx == NULL)
          return -1;

     if(sasdl_ctx->ap_lock != NULL)
     {
          SDL_mutexP(sasdl_ctx->ap_lock);
          if(sasdl_ctx->ap != NULL)
               SA_free_ap(sasdl_ctx->ap);
          SDL_DestroyMutex(sasdl_ctx->ap_lock);
     }

     if(sasdl_ctx->swsctx != NULL)
          sws_freeContext(sasdl_ctx->swsctx);

     if(sasdl_ctx->frame_cur != NULL)
          SDL_FreeSurface(sasdl_ctx->frame_cur);
     
     SA_close(sasdl_ctx->sa_ctx);
     
     free(sasdl_ctx);
     return 0;
}

void SASDL_play(SASDLContext *sasdl_ctx)
{
     if(sasdl_ctx->status == SASDL_is_playing)
          return;
     sasdl_ctx->status = SASDL_is_playing;
     sasdl_ctx->start_time = SA_get_clock() - sasdl_ctx->video_start_at;
}

/*
void SASDL_pause(SAContext *sa_ctx)
{
     SASDLContext *sasdl_ctx = sa_ctx->lib_data;
     if(sasdl_ctx->status != SASDL_is_playing)
          return;
     sasdl_ctx->video_start_at = SASDL_get_video_clock(sa_ctx);
     sasdl_ctx->status = SASDL_is_paused;
     // SDL_PauseAudio(1);
}

int SASDL_stop(SAContext *sa_ctx)
{
     int ret;
     SASDLContext *sasdl_ctx = sa_ctx->lib_data;
     if(sasdl_ctx->status == SASDL_is_stopped)
          return 0;
     sasdl_ctx->status = SASDL_is_stopped;
     sasdl_ctx->video_start_at = 0.0f;
     
     //
     // SASDL_seek() will handle ap in sasdl and aq in core for us.
     //
     // but still, we call SDL_PauseAudio(1) here, to make sure it
     // works correctly.
     // SDL_PauseAudio(1);
     ret = SASDL_seek(sa_ctx, 0.0f);
     
     return ret;
}

// currently, SASDL_seek() will return -1 on both EOF and error.
// so the user should directly stop the video loop when gotting -1.
int SASDL_seek(SAContext *sa_ctx, double seek_dst)
{
     if(seek_dst < 0)
          seek_dst = 0;

     // FIXME: seek forehead a little to do a precise seek
     
     SASDLContext *sasdl_ctx = sa_ctx->lib_data;
     SAVideoPacket *vp_cur = sasdl_ctx->vp_cur;
     SAVideoPacket *vp_next = sasdl_ctx->vp_next;
     if(vp_cur != NULL)
          SA_free_vp(vp_cur);
     if(vp_next != NULL)
          SA_free_vp(vp_next);
     sasdl_ctx->vp_cur = sasdl_ctx->vp_next = NULL;

     SDL_mutexP(sasdl_ctx->ap_lock);
     SAAudioPacket *ap = sasdl_ctx->ap;
     if(ap != NULL)
     {
          SA_free_ap(ap);
          sasdl_ctx->ap = NULL;
          sasdl_ctx->audio_buf_index = 0;
     }

     int ret = SA_seek(sa_ctx, seek_dst,
                       seek_dst - sasdl_ctx->last_pts);
     
     sasdl_ctx->vp_cur = vp_cur = SA_get_vp(sa_ctx);
     if(vp_cur == NULL)
     {
          SDL_mutexV(sasdl_ctx->ap_lock);
          return -1; // FIXME: EOF? seeking error?
     }
     vp_next = sasdl_ctx->vp_next = SA_get_vp(sa_ctx);

     // FIXME: what if vp_cur->pts is...????????????
     sasdl_ctx->last_pts = vp_cur->pts;
     sasdl_ctx->video_start_at = vp_cur->pts;
     if(sasdl_ctx->status == SASDL_is_playing)
          sasdl_ctx->start_time = SA_get_clock() - vp_cur->pts;
     else if(sasdl_ctx->status == SASDL_is_stopped)
          sasdl_ctx->status = SASDL_is_paused;

     // FIXME:
     //
     // if paused: draw()
     // else if stopped: draw black
     
     SDL_mutexV(sasdl_ctx->ap_lock);
     return ret;
}

*/

/*
 * SASDL_draw() always draw the correct frame,
 * even when the user is not using SASDL_delay().
 *
 * "correct frame" means, we will always have:
 *     (pts of frame_cur <=) video clock < frame_next_pts
 * under the "playing" mode.
 */
void SASDL_draw(SASDLContext *sasdl_ctx, SDL_Surface *surface)
{
     if(sasdl_ctx->status != SASDL_is_playing ||
        sasdl_ctx->video_eof) {
          SDL_BlitSurface(sasdl_ctx->frame_cur, NULL, surface, NULL);
          return;
     }

     // From now on, we are sure of these facts:
     //   1. the video is under the "playing" mode.
     //   2. we have not reach EOF of the video yet.
     
     SAVideoPacket *vp;
     while(sasdl_ctx->frame_next == NULL ||
           sasdl_ctx->frame_next_pts <= SASDL_get_video_clock(sasdl_ctx)) {
          vp = SA_get_vp(sasdl_ctx->sa_ctx);
          if(vp == NULL) {
               sasdl_ctx->video_eof = TRUE;
               sasdl_ctx->frame_next = NULL;
               break;
          } else {
               sasdl_ctx->frame_next = vp->frame_ptr;
               sasdl_ctx->frame_next_pts = vp->pts;
               SA_free_vp(vp);

               _SASDL_convert_frame_next_to_cur(sasdl_ctx);
          }
     }
     
     SDL_BlitSurface(sasdl_ctx->frame_cur, NULL, surface, NULL);
     return;
}

/*
 * wait for the next frame.
 * return instantly when not under "playing" mode or encountered video EOF,
 * or we already have next_frame_pts <= video clock.
 */
void SASDL_wait_for_next_frame(SASDLContext *sasdl_ctx)
{
     if(sasdl_ctx->status != SASDL_is_playing ||
        sasdl_ctx->video_eof) {
          return;
     }
     
     double w_time = SASDL_get_video_clock(sasdl_ctx) - sasdl_ctx->frame_next_pts;
     if(w_time > 0.0f) {
          // FIXME: do we need a 2nd iteration?
          SDL_Delay(w_time * 1000);
     }
     
     return;
}

// attention please: the first parameter is sasdl_ctx,
// but NOT sa_ctx!!!!!!!!
//
// output silence if encountered audio EOF.
/*
void SASDL_audio_decode(void *data, uint8_t *stream, int len)
{
     // FIXME: not finished yet.
     
     // FIXME: detect video status and call SDL_AudioPause() here?
     SAContext *sa_ctx = data;
     SASDLContext *sasdl_ctx = sa_ctx->lib_data;
     
     SDL_mutexP(sasdl_ctx->ap_lock);
     
     SAAudioPacket *ap = sasdl_ctx->ap;
     unsigned int audio_buf_index = sasdl_ctx->audio_buf_index;
     unsigned int size_to_copy = 0;
     double size_per_sec = 2 * sa_ctx->a_codec_ctx->channels *
                           sa_ctx->a_codec_ctx->sample_rate;
     if(sa_ctx->audio_eof)
     {
          SDL_PauseAudio(1);
          SDL_mutexV(sasdl_ctx->ap_lock);
          return;
     }

     while(len > 0)
     {
          if(ap == NULL)
               sasdl_ctx->ap = ap = SA_get_ap(sa_ctx);

          if(ap == NULL)
          {
               sa_ctx->audio_eof = 1;
               memset(stream, 0, len);
               SDL_PauseAudio(1);
               sasdl_ctx->ap = ap;
               sasdl_ctx->audio_buf_index = audio_buf_index;
               SDL_mutexV(sasdl_ctx->ap_lock);
               return; // FIXME: *MAYBE* eof encountered. what if... ?
          }

          // DEBUG
          // double t = ap->pts - SASDL_get_video_clock(sa_ctx);
          // if(t < 0) t = -t;
          // printf("%f\n", t);
          // printf("%f\n", ap->pts);
          
          double delay = ap->pts - SASDL_get_video_clock(sa_ctx);
          if(-SASDL_AUDIO_ADJUST_THRESHOLD <= delay &&
             delay <= SASDL_AUDIO_ADJUST_THRESHOLD)
               delay = 0.0f;
          int delay_size = delay * size_per_sec;
          if(delay_size > 0) // 'wait' for the external clock
          {
               int silent_size = len < delay_size ? len : delay_size;
               memset(stream, 0, silent_size);
               len -= silent_size;
               stream += silent_size;
               continue;
          } else if(delay_size < 0) // shrink the buffer
          {
               audio_buf_index -= delay_size;
               
               // copy the code directly to prevent infinite looping
               if(audio_buf_index >= ap->len)
               {
                    av_free(ap->abuffer);
                    free(ap);
                    ap = NULL;
                    audio_buf_index = 0;
                    continue;
               }
          }
          
          size_to_copy = len < (ap->len - audio_buf_index) ?
               len : (ap->len - audio_buf_index);
          memcpy(stream, ap->abuffer + audio_buf_index, size_to_copy);

          len -= size_to_copy;
          stream += size_to_copy;
          audio_buf_index += size_to_copy;

          if(audio_buf_index >= ap->len)
          {
               av_free(ap->abuffer);
               free(ap);
               ap = NULL;
               audio_buf_index = 0;
          }
     }

     sasdl_ctx->ap = ap;
     sasdl_ctx->audio_buf_index = audio_buf_index;
     SDL_mutexV(sasdl_ctx->ap_lock);
}
*/

/*
 * other functions - welcome to the easy part!
 */

int SASDL_get_width(SASDLContext *sasdl_ctx)
{
     return SA_get_width(sasdl_ctx->sa_ctx);
}

int SASDL_get_height(SASDLContext *sasdl_ctx)
{
     return SA_get_height(sasdl_ctx->sa_ctx);
}

double SASDL_get_video_duration(SASDLContext *sasdl_ctx)
{
     return SA_get_duration(sasdl_ctx->sa_ctx);
}

double SASDL_get_video_clock(SASDLContext *sasdl_ctx)
{
     if(sasdl_ctx->status == SASDL_is_playing)
          return SA_get_clock() - sasdl_ctx->start_time;
     if(sasdl_ctx->status == SASDL_is_paused)
          return sasdl_ctx->video_start_at;
     
     // status == SASDL_is_stopped, or source code hacked
     return 0.0f;
}

enum SASDLVideoStatus SASDL_get_video_status(SASDLContext *sasdl_ctx)
{
     return sasdl_ctx->status;
}

int SASDL_eof(SASDLContext *sasdl_ctx)
{
     return sasdl_ctx->video_eof && sasdl_ctx->audio_eof;
}

////
//// here comes the "private" part.
////

void _SASDL_convert_frame_next_to_cur(SASDLContext *sasdl_ctx)
{
     AVFrame *frame = sasdl_ctx->frame_next;
     SDL_Surface *surface = sasdl_ctx->frame_cur;
     int h = SASDL_get_height(sasdl_ctx);
     
     SDL_LockSurface(surface);
     AVPicture pict;
     pict.data[0] = surface->pixels; 
     pict.linesize[0] = surface->pitch;
     sws_scale(sasdl_ctx->swsctx, (const uint8_t * const *)(frame->data),
               frame->linesize, 0, h, pict.data, pict.linesize);
     SDL_UnlockSurface(surface);
}

void _SASDL_fill_frame_cur_black(SASDLContext *sasdl_ctx)
{
     SDL_Rect full_screen = {
          .x = 0, .y = 0,
          .w = SASDL_get_width(sasdl_ctx), .h = SASDL_get_height(sasdl_ctx)
     };
     
     SDL_FillRect(sasdl_ctx->frame_cur, &full_screen, 0x000000);
}
