// mp3player.c: MP3 Player Implementation in C for Sony PSP
//
////////////////////////////////////////////////////////////////////////////

#ifndef LINUX_MODE
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>
#include <pspdisplay.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <pspaudiolib.h>
#include "mp3player.h"

#define OUTPUT_BUFFER_SIZE    2048    /* Must be an integer multiple of 4. */

#define NUMCHANNELS 2
static u8 *mp3_data;
static long size;
static long samplesInOutput = 0;
static char mp3_curr_filename[256];

//////////////////////////////////////////////////////////////////////
// Global local variables
//////////////////////////////////////////////////////////////////////

//libmad lowlevel stuff

// The following variables contain the music data, ie they don't change value until you load a new file
static struct mad_stream Stream;
static struct mad_frame Frame;
static struct mad_synth Synth;
mad_timer_t Timer;
static signed short OutputBuffer[OUTPUT_BUFFER_SIZE];
static unsigned long FrameCount = 0;

// The following variables are maintained and updated by the tracker during playback
static int isPlaying;        // Set to true when a mod is being played
static int isRepeatMode = 0;

/* Define printf, just to make typing easier */

struct MP3Info MP3_info;
# endif

#ifndef LINUX_MODE
/****************************************************************************
* Converts a sample from libmad's fixed point number format to a signed    *
* short (16 bits).                                                            *
****************************************************************************/
static signed short MadFixedToSshort(mad_fixed_t Fixed)
{
    /* A fixed point number is formed of the following bit pattern:
     *
     * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
     * MSB                          LSB
     * S ==> Sign (0 is positive, 1 is negative)
     * W ==> Whole part bits
     * F ==> Fractional part bits
     *
     * This pattern contains MAD_F_FRACBITS fractional bits, one
     * should alway use this macro when working on the bits of a fixed
     * point number. It is not guaranteed to be constant over the
     * different platforms supported by libmad.
     *
     * The signed short value is formed, after clipping, by the least
     * significant whole part bit, followed by the 15 most significant
     * fractional part bits. Warning: this is a quick and dirty way to
     * compute the 16-bit number, madplay includes much better
     * algorithms.
     */

    /* Clipping */
    if (Fixed >= MAD_F_ONE)
    return (SHRT_MAX);
    if (Fixed <= -MAD_F_ONE)
    return (-SHRT_MAX);

    /* Conversion. */
    Fixed = Fixed >> (MAD_F_FRACBITS - 15);
    return ((signed short) Fixed);
}

static inline void
write44k_buf( short* tgt, short* src, int num_short )
{
  while (num_short-- > 0) {
    *tgt++ = *src++;
  }
}


static void 
MP3_getInfo()
{
    int FrameCount = 0;
    struct mad_stream stream;
    struct mad_header header; 
    mad_stream_init (&stream);
    mad_header_init (&header);

    MP3_info.fileSize = size;
    mad_timer_reset(&MP3_info.length);
    mad_stream_buffer (&stream, mp3_data, size);
    while (1){
        if (mad_header_decode (&header, &stream) == -1){
            if (MAD_RECOVERABLE(stream.error)){
                continue;                
            }else{
                break;
            }
        }
        //Informazioni solo dal primo frame:
        if (FrameCount == 0){
            switch (header.layer) {
            case MAD_LAYER_I:
                strcpy(MP3_info.layer,"I");
                break;
            case MAD_LAYER_II:
                strcpy(MP3_info.layer,"II");
                break;
            case MAD_LAYER_III:
                strcpy(MP3_info.layer,"III");
                break;
            default:
                strcpy(MP3_info.layer,"unknown");
                break;
            }

            MP3_info.kbit = header.bitrate / 1000;
            MP3_info.hz = header.samplerate;
            switch (header.mode) {
            case MAD_MODE_SINGLE_CHANNEL:
                strcpy(MP3_info.mode, "single channel");
                break;
            case MAD_MODE_DUAL_CHANNEL:
                strcpy(MP3_info.mode, "dual channel");
                break;
            case MAD_MODE_JOINT_STEREO:
                strcpy(MP3_info.mode, "joint (MS/intensity) stereo");
                break;
            case MAD_MODE_STEREO:
                strcpy(MP3_info.mode, "normal LR stereo");
                break;
            default:
                strcpy(MP3_info.mode, "unknown");
                break;
            }

            switch (header.emphasis) {
            case MAD_EMPHASIS_NONE:
                strcpy(MP3_info.emphasis,"no");
                break;
            case MAD_EMPHASIS_50_15_US:
                strcpy(MP3_info.emphasis,"50/15 us");
                break;
            case MAD_EMPHASIS_CCITT_J_17:
                strcpy(MP3_info.emphasis,"CCITT J.17");
                break;
            case MAD_EMPHASIS_RESERVED:
                strcpy(MP3_info.emphasis,"reserved(!)");
                break;
            default:
                strcpy(MP3_info.emphasis,"unknown");
                break;
            }            
        }
        FrameCount++;
        mad_timer_add (&MP3_info.length, header.duration);
    }
    mad_header_finish (&header);
    mad_stream_finish (&stream);

    MP3_info.frames = FrameCount;
    mad_timer_string(MP3_info.length, MP3_info.strLength, "%02lu:%02u:%02u", MAD_UNITS_HOURS, MAD_UNITS_MILLISECONDS, 0);
}


static void
MP3_Restart()
{
  memset(OutputBuffer, 0, OUTPUT_BUFFER_SIZE);
  samplesInOutput = 0;

  mad_stream_init(&Stream);
  mad_frame_init(&Frame);
  mad_synth_init(&Synth);
  mad_timer_reset(&Timer);
  MP3_getInfo();
}

static int
MP3Callback_44k(void *_buf2, unsigned int numSamples, void *pdata)
{
  int i;
  short *_buf = (short *)_buf2;
  unsigned long samplesOut = 0;

  //  Playing , so mix up a buffer
  if (samplesInOutput > 0) 
  {
    if (samplesInOutput > numSamples) {
      write44k_buf((short *) _buf, (short *) OutputBuffer, numSamples * 2);
      samplesOut = numSamples;
      samplesInOutput -= numSamples;
    } else {
      write44k_buf((short *) _buf, (short *) OutputBuffer, samplesInOutput * 2);
      samplesOut = samplesInOutput;
      samplesInOutput = 0;
    }
  }
  while (samplesOut < numSamples) 
  {
    if (Stream.buffer == NULL) {
      Stream.error = 0;
      mad_stream_buffer(&Stream, mp3_data, size);
    } else
    if (Stream.error == MAD_ERROR_BUFLEN) {
      if (isRepeatMode) {
        MP3_Restart();
        mad_stream_buffer(&Stream, mp3_data, size);
        samplesInOutput = 0;
      } else {
        MP3_Stop();
        return 0;
      }
    }

    if (mad_frame_decode(&Frame, &Stream)) {
      if (MAD_RECOVERABLE(Stream.error)) {
        return 0;
      } else
      if (Stream.error == MAD_ERROR_BUFLEN) {
        if (! isRepeatMode) {
          MP3_Stop();
          return 0;
        }
      } else {
        MP3_Stop();
      }
    }

    FrameCount++;
    mad_timer_add(&Timer, Frame.header.duration);
    mad_synth_frame(&Synth, &Frame);

    for (i = 0; i < Synth.pcm.length; i++) {
      signed short Sample;
      if (samplesOut < numSamples) {
        /* Left channel */
        Sample = MadFixedToSshort(Synth.pcm.samples[0][i]);
        _buf[samplesOut * 2] = Sample;
     
        /* Right channel. If the decoded stream is monophonic then
         * the right output channel is the same as the left one.
         */
        if (MAD_NCHANNELS(&Frame.header) == 2) {
          Sample = MadFixedToSshort(Synth.pcm.samples[1][i]);
        }
        _buf[samplesOut * 2 + 1] = Sample;
        samplesOut++;
      } else {
        Sample = MadFixedToSshort(Synth.pcm.samples[0][i]);
        OutputBuffer[samplesInOutput * 2] = Sample;
        if (MAD_NCHANNELS(&Frame.header) == 2)
        Sample = MadFixedToSshort(Synth.pcm.samples[1][i]);
        OutputBuffer[samplesInOutput * 2 + 1] = Sample;
        samplesInOutput++;
      }
    }
  } 
  return isPlaying;
}

static void
write22k_buf( short* tgt, short* src, int num_short )
{
  while (num_short-- > 0) {
    *tgt++ = *src;
    *tgt++ = *src++;
  }
}

static int
MP3Callback_22k(void *_buf2, unsigned int numSamples, void *pdata)
{
  int i;
  short *_buf = (short *)_buf2;
  unsigned long samplesOut = 0;

  numSamples /= 2;
  //  Playing , so mix up a buffer
  if (samplesInOutput > 0) 
  {
    if (samplesInOutput > numSamples) {
      write22k_buf((short *) _buf, (short *) OutputBuffer, numSamples * 2);
      samplesOut = numSamples;
      samplesInOutput -= numSamples;
    } else {
      write22k_buf((short *) _buf, (short *) OutputBuffer, samplesInOutput * 2);
      samplesOut = samplesInOutput;
      samplesInOutput = 0;
    }
  }
  while (samplesOut < numSamples) 
  {
    if (Stream.buffer == NULL) {
      Stream.error = 0;
      mad_stream_buffer(&Stream, mp3_data, size);
    }
    if (Stream.error == MAD_ERROR_BUFLEN) {
      Stream.error = 0;
      if (isRepeatMode) {
        MP3_Restart();
        mad_stream_buffer(&Stream, mp3_data, size);
        samplesInOutput = 0;
      } else {
        MP3_Stop();
        return 0;
      }
    }

    if (mad_frame_decode(&Frame, &Stream)) {
      if (MAD_RECOVERABLE(Stream.error)) {
        return 0;
      } else
      if (Stream.error == MAD_ERROR_BUFLEN) {
        if (! isRepeatMode) {
          MP3_Stop();
          return 0;
        }
      } else {
        MP3_Stop();
      }
    }

    FrameCount++;
    mad_timer_add(&Timer, Frame.header.duration);
    mad_synth_frame(&Synth, &Frame);

    for (i = 0; i < Synth.pcm.length; i++) {
      signed short Sample;
      if (samplesOut < numSamples) {
        /* Left channel */
        Sample = MadFixedToSshort(Synth.pcm.samples[0][i]);
        _buf[((samplesOut * 2) * 2)    ] = Sample;
        _buf[((samplesOut * 2) * 2) + 1] = Sample;
     
        /* Right channel. If the decoded stream is monophonic then
         * the right output channel is the same as the left one.
         */
        if (MAD_NCHANNELS(&Frame.header) == 2) {
          Sample = MadFixedToSshort(Synth.pcm.samples[1][i]);
        }
        _buf[(((samplesOut * 2) + 1) * 2)    ] = Sample;
        _buf[(((samplesOut * 2) + 1) * 2) + 1] = Sample;
        samplesOut++;
      } else {
        Sample = MadFixedToSshort(Synth.pcm.samples[0][i]);
        OutputBuffer[samplesInOutput * 2] = Sample;
        if (MAD_NCHANNELS(&Frame.header) == 2) {
          Sample = MadFixedToSshort(Synth.pcm.samples[1][i]);
        }
        OutputBuffer[samplesInOutput * 2 + 1] = Sample;
        samplesInOutput++;
      }
    }
  } 
  
  return isPlaying;
}
# endif //LINUX_MODE

int
MP3Callback(void *_buf2, unsigned int numSamples, void *pdata)
{
# ifndef LINUX_MODE
  if (! isPlaying) return 0;

  if (MP3_info.hz >= 44100) {
    return MP3Callback_44k( _buf2, numSamples, pdata );
  }
  return MP3Callback_22k( _buf2, numSamples, pdata );
# else
  return 0;
# endif
}

void MP3_Init()
{
# ifndef LINUX_MODE
    samplesInOutput = 0;
    isPlaying = 0;
    /* First the structures used by libmad must be initialized. */
    mad_stream_init(&Stream);
    mad_frame_init(&Frame);
    mad_synth_init(&Synth);
    mad_timer_reset(&Timer);
# endif
}


void 
MP3_FreeTune()
{
# ifndef LINUX_MODE
    /* The input file was completely read; the memory allocated by our
     * reading module must be reclaimed.
     */
    if (mp3_data) { free(mp3_data); mp3_data = 0; }

    /* Mad is no longer used, the structures that were initialized must
     * now be cleared.
     */
    mad_synth_finish(&Synth);
    mad_frame_finish(&Frame);
    mad_stream_finish(&Stream);
# endif
}


//MP3_End
void MP3_End()
{
# ifndef LINUX_MODE
    MP3_Stop();
    MP3_FreeTune();
# endif
}

int
psp_mp3_play_file(char* filename, int repeat)
{
# ifndef LINUX_MODE
  isRepeatMode = repeat;

  MP3_Stop();
  MP3_Init();
  if (! MP3_Load(filename)) {
    MP3_Stop();
  } else {
    MP3_Play();
  }
# endif
}

void
psp_mp3_pause()
{
# ifndef LINUX_MODE
  MP3_Pause();
# endif
}

void
psp_mp3_reset()
{
# ifndef LINUX_MODE
  MP3_End();
# endif
}

//////////////////////////////////////////////////////////////////////
// Functions - Local and not public
//////////////////////////////////////////////////////////////////////

//  This is the initialiser and module loader
//  This is a general call, which loads the module from the 
//  given address into the modplayer
//
//  It basically loads into an internal format, so once this function
//  has returned the buffer at 'data' will not be needed again.
//
# ifndef LINUX_MODE
int 
MP3_Load(char *filename)
{
  int fd;

  if (!strcmp(mp3_curr_filename, filename)) {
    // already loaded !
  } else
  if ((fd = sceIoOpen(filename, PSP_O_RDONLY, 0777)) > 0) {
    //  opened file, so get size now
    size = sceIoLseek(fd, 0, PSP_SEEK_END);
    sceIoLseek(fd, 0, PSP_SEEK_SET);
    if (mp3_data) { free(mp3_data); mp3_data = 0; }
    mp3_data = (unsigned char *) malloc(size + 8);
    if (mp3_data) memset(mp3_data, 0, size + 8);
    if (mp3_data != 0) {        // Read file in
      sceIoRead(fd, mp3_data, size);
    } else {
      //printf("Error allocing\n");
      sceIoClose(fd);
      return 0;
    }
    // Close file
    sceIoClose(fd);
    strcpy(mp3_curr_filename, filename);

  } else {
    return 0;
  }
  isPlaying = 0;
  FrameCount = 0;

  MP3_getInfo();
  return 1;
}

// This function initialises for playing, and starts
int MP3_Play()
{
    // See if I'm already playing
    if (isPlaying) return 0;
    isPlaying = 1;
    return 1;
}

void MP3_Pause()
{
    isPlaying = 0;
}

int MP3_Stop()
{
    //stop playing
    isPlaying = 0;
    //clear buffer
    memset(OutputBuffer, 0, OUTPUT_BUFFER_SIZE);
    return 1;
}

int MP3_IsPlaying()
{
  return isPlaying;
}

//Get info on file:
struct MP3Info 
MP3_GetInfo()
{
    return MP3_info;
}
# endif //LINUX_MODE
