/* Copyright (C) 2011 by Chenguang Wang(wecing)
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#ifndef __SIMPLEAV_SDL_H__DEFINED__
#define __SIMPLEAV_SDL_H__DEFINED__


#ifdef __cplusplus
// FIXME: should this be added to SimpleAV as well?
// dirty hack for C++
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
#endif


// #include <SDL/SDL.h>
#include <SDL.h>
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
     SASDL_is_playing = 0, SASDL_is_paused, SASDL_is_stopped
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
     //
     // FIXME: rename it to video_restart_at.
     double video_restart_at;

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
     int64_t start_time;

     // this is used to convert AVFrame.
     // it should be freed using sws_freeContext,
     // but dranger forgot to do so... :-P
     struct SwsContext *swsctx;
     
     // FIXME: should ap be locked?
     // or, will SDL_PauseAudio(1) kill the audio callback function if it is executing?
     SAAudioPacket *ap;
     // FIXME: why not SAMutex?
     SDL_mutex *ap_lock;
     // audio_buf_index records how much data is used in ap.
     unsigned int audio_buf_index;

     // we will ensure frame_next == NULL when video_eof == TRUE.
     // and frame_next_pts will be an arbitrary value.
     //
     // on the other hand, frame_cur should always be the last frame before
     // current video clock. if video_clock == 0, it will be a black surface.
     SDL_Surface *frame_cur;
     AVFrame *frame_next;
     double frame_next_pts;

     // they should be either TRUE or FALSE.
     // (PS: wecing defined TRUE as 1, and FALSE as 0.)
     int video_eof, audio_eof;

     // this controls whether SASDL_audio_decoder will overwrite data in the stream or not.
     // default value is FALSE: overwrite.
     int audio_decode_mix;
     int audio_mix_volume;

     // i know you know i know you know this.
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

// this should be called with the second parameter equals to 1
// if you need to mix other sounds.
// or SASDL_audio_decode will just overwrite data in the given target buffer.
void SASDL_set_audio_mix(SASDLContext *, int);

// set mix volume of the audio output in SASDL_audio_decode().
// should be called after enabling audio mix, or would be ignored.
// the second parameter should be an integer between 0 and SDL_MIX_MAXVOLUME.
void SASDL_set_mix_volume(SASDLContext *, int);



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

// set the video to "pausing" mode.
// under this mode, the audio decoding function will output silence.
// it does nothing if not under "playing" mode.
void SASDL_pause(SASDLContext *);

// set the video to "stopped" mode, and seek to the beginning of video.
// returns immediately when already under "stopped" mode.
//
// as SASDL_stop involves seeking, it may fail.
// it returns a negative value on error.
int SASDL_stop(SASDLContext *);

// if the requested destination of seeking is out of the video duration,
// it will be same as calling SASDL_stop().
//
// when success:
//     if called under "stopped" mode, the video status will be set to "paused".
//     else the video status will stay untouched.
//
// so be careful if you are writing a video player; if an insane user keeps asking
// "seek forward 10sec", SASDL will first seek to the end after a while, then
// stop the video, and finally continue seeking forward under paused mode.
//
// *** BE CAREFUL: ***
//     SimpleAV will always seek to the last key frame before the requested time.
//     so this function is *NOT ACCURATE*.
//
// *** for developers only:
// *** don't forget to get frame_cur.
// (I wrote the previous line more than one year ago, and I don't know what it means now.
//            -- wecing, Aug 31, 2012)
int SASDL_seek(SASDLContext *, double seek_dst);

// same as SASDL_seek, but the accurate seeking version.
int SASDL_seek_accurate(SASDLContext *, double seek_dst);

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

// output video info to the console.
void SASDL_dump_info(SASDLContext *);

// it will tell you "where we are now".
double SASDL_get_video_clock(SASDLContext *);

// it will return one of SASDL_is_playing, SASDL_is_paused, and SASDL_is_stopped.
enum SASDLVideoStatus SASDL_get_video_status(SASDLContext *);

int SASDL_video_is_playing(SASDLContext *);

int SASDL_video_is_paused(SASDLContext *);

int SASDL_video_is_stopped(SASDLContext *);

// it will return true only when (sa_ctx->video_eof && sa_ctx->audio_eof) is true.
int SASDL_eof(SASDLContext *);

#endif

#ifdef __cplusplus
}
#endif
