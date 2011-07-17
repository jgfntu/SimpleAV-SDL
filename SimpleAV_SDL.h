/*
 * SimpleAV_SDL.h
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

#ifndef __SIMPLEAV_SDL_H__DEFINED__
#define __SIMPLEAV_SDL_H__DEFINED__

#include <SDL/SDL.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <SimpleAV.h>

#define  TRUE   1
#define  FALSE  0

/*
 * the users should avoid using lower-lever SA_*() functions.
 * so, simply reading SimpleAV_SDL.h should be enough.
 */

// the video status. i think they are self-descriptive enough.
enum SASDLVideoStatus {
     SASDL_is_playing, SASDL_is_paused, SASDL_is_stopped
};

// FIXME: I need ... a better threshold.
//        I hate magic numbers.
// for developers only.
#define SASDL_AUDIO_ADJUST_THRESHOLD   (1.00f / 24)

// the important big struct.
// its inner content is for developers only.
typedef struct SASDLContext {
     enum SASDLVideoStatus status;

     // video_start_at is used to handle pausing.
     // it records where to restart when calling SASDL_play()
     // under "paused" or "stopped" mode.
     double video_start_at;

     // start_time is used to calculate the video clock.
     // in other words, "where we are now".
     //
     // however, start_time it self records the time when we "started playing",
     // video_start_at is used to revise the "delta".
     //
     // under "playing" mode, video clock could be calculated with this equation:
     // video_clock == SA_get_clock() - start_time
     // 
     // start_time is used only under "playing" mode.
     double start_time;
     
     struct SwsContext *swsctx;
     
     SAAudioPacket *ap; // FIXME: should ap be locked?
     // or, will SDL_PauseAudio(1) kill the audio callback function if it is executing?
     SDL_mutex *ap_lock; // FIXME: why not SAMutex?

     // we will ensure frame_next == NULL when video_eof == TRUE.
     // and frame_next_pts will be an arbitrary value.
     //
     // on the other hand, frame_cur should always be the last frame before
     // current video clock. if video_clock == 0, it will be a black surface.
     SDL_Surface *frame_cur;
     AVFrame *frame_next;
     double frame_next_pts;
     
     unsigned int audio_buf_index;

     int video_eof, audio_eof;

     SAContext *sa_ctx;
} SASDLContext;

/*
 * init, open/close
 */

// initialize SimpleAV(which initializes libav).
// call this before calling SASDL_*().
int SASDL_init(void);

// open the video file.
// as this will request to create SDL_mutex and SDL_Surface variables,
// call this after SDL_Init().
SASDLContext *SASDL_open(char *);

// close files referenced by SAContext
int SASDL_close(SASDLContext *);



/*
 * video control: play/pause/stop, seek
 */

// set the video to "playing" mode.
// under this mode, everytime calling SASDL_draw(sa_ctx, surf)
// will make surf be filled with corresponding decoded frame at
// that time point.
//
// it will return instantly if the video is
// already under "playing" mode.
void SASDL_play(SASDLContext *);

// FIXME: implement this later
// set the video to "pausing" mode. it does nothing when not under "playing" mode.
// void SASDL_pause(SAContext *);

// FIXME: implement this later
// int SASDL_stop(SAContext *);

// FIXME: implement this later
// *** for developers only:
// *** seek a little forehead.
// *** don't forget to get frame_cur.
// int SASDL_seek(SAContext *, double);



/*
 * output: draw & delay & audio callback
 */

// fill SDL_Surface with the corresponding data.
// when under "playing" mode: decoded frame at that time point;
//            "paused"  mode: decoded frame at the time
//                            the "paused" mode is activited.
//            "stopped" mode: all black.
//
// when encountered end of video: the last frame.
//
// *** for developers only:
// *** SASDL_seek will seek a little forehead
// *** for both making a precise seeking and
// *** getting the last frame.
void SASDL_draw(SASDLContext *, SDL_Surface *);

// this function is optional. you could use it to reduce CPU occupying.
// use this to wait for the next frame.
// it returns immediately when under "paused" mode and "stopped" mode
//                        or encountered video EOF.
// SASDL_wait_for_next_frame() does not do very exact waiting.
void SASDL_wait_for_next_frame(SASDLContext *);

// it uses the first parameter as pointer to a SASDLContext struct, and
// get data from the video to fill the uint8_t buffer.
// so, the users can use this function as both a simply SDL audio callback
// and a audio decoding function.
void SASDL_audio_decode(void *, uint8_t *, int);



/*
 * others
 */

// get width and height of the video.
int SASDL_get_video_width(SASDLContext *);

int SASDL_get_video_height(SASDLContext *);

double SASDL_get_video_duration(SASDLContext *);

// it will tell you "where we are now".
double SASDL_get_video_clock(SASDLContext *);

// it will return one of SASDL_is_playing, SASDL_is_paused, and SASDL_is_stopped.
enum SASDLVideoStatus SASDL_get_video_status(SASDLContext *);

// it will return true only when (sa_ctx->video_eof && sa_ctx->audio_eof) is true.
int SASDL_eof(SASDLContext *);


/*
 * private functions - don't use them!
 */

// convert the video frame in sasdl_ctx->frame_next onto sasdl_ctx->frame_cur.
void _SASDL_convert_frame_next_to_cur(SASDLContext *);

// fill sasdl_ctx->frame_cur with black.
// currently only used in SASDL_draw() under "stopped" mode.
void _SASDL_fill_frame_cur_black(SASDLContext *);

#endif
