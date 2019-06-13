/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Hugo.h"

 static volatile int psp_audio_sleep = 0;

# ifdef PSP_AUDIO
 static int psp_audio_chid   = -1;
 static int psp_audio_thid   = -1;
 static int psp_cdaudio_chid = -1;
 static int psp_cdaudio_thid = -1;

 static int psp_audio_term = 0;

    extern volatile unsigned char *pbSndStream;

 static int psp_audio_empty[ 2048 ];
 static int psp_audio_buffer[2][ 2048 ];
 static int psp_cdaudio_buffer[2][ 2048 ];

void
psp_audio_thread(int args, void *argp)
{
  int buff_id = 0;
  char* snd_buff = 0;
	while (psp_audio_term == 0) {
    if (psp_audio_sleep || (! HUGO.hugo_snd_enable)) {
      snd_buff = psp_audio_empty;
    } else {
      snd_buff = psp_audio_buffer[buff_id];
      MixSound( snd_buff, 1024 );
      buff_id = ! buff_id;
    }
    sceAudioOutputPannedBlocking(psp_audio_chid, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, snd_buff);
	}
	sceKernelExitThread(0);
}

void
psp_cdaudio_thread(int args, void *argp)
{
  char* snd_buff = 0;
  int buff_id = 0;
	while (psp_audio_term == 0) {
    if (psp_audio_sleep || (! HUGO.hugo_snd_enable) || (! HUGO.hugo_cdaudio_enable)) {
      myPowerSetClockFrequency(HUGO.psp_cpu_clock);
      snd_buff = psp_audio_empty;
    } else {
      snd_buff = psp_cdaudio_buffer[buff_id];
      if (! MP3Callback(snd_buff, 1024, NULL)) {
        snd_buff = psp_audio_empty;
      } else {
        myPowerSetClockFrequency(HUGO.psp_cpu_cdclock);
        buff_id = ! buff_id;
      }
    }
    sceAudioOutputPannedBlocking(psp_cdaudio_chid, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, snd_buff);
	}
	sceKernelExitThread(0);
}

void
psp_audio_init()
{
  psp_audio_chid = sceAudioChReserve(-1, 1024, 0);
  psp_audio_thid = sceKernelCreateThread( "audio_thread",(void*)&psp_audio_thread,0x12,0x10000,0,NULL);
	sceKernelStartThread( psp_audio_thid, 0, 0);

  MP3_Init();

  psp_cdaudio_chid = sceAudioChReserve(-1, 1024, 0);
  psp_cdaudio_thid = sceKernelCreateThread( "cdaudio_thread",(void*)&psp_cdaudio_thread,0x12,0x10000,0,NULL);
	sceKernelStartThread( psp_cdaudio_thid, 0, 0);
}

void
psp_audio_end()
{
  psp_audio_term = 1;
  if (psp_audio_thid != -1) {
		sceKernelDeleteThread( psp_audio_thid );
    psp_audio_thid = -1;
  }
  if (psp_cdaudio_thid != -1) {
		sceKernelDeleteThread( psp_cdaudio_thid );
    psp_cdaudio_thid = -1;
  }
  if (psp_audio_chid != -1) {
    sceAudioChRelease( psp_audio_chid );
    psp_audio_chid = -1;
  }
  if (psp_cdaudio_chid != -1) {
    sceAudioChRelease( psp_cdaudio_chid );
    psp_cdaudio_chid = -1;
  }
}
# endif //PSP_AUDIO

void
psp_cdaudio_play_file( const char* filename, int repeat )
{
  if (HUGO.hugo_cdaudio_enable) {
    psp_mp3_play_file( filename, repeat );
  } else {
    psp_mp3_reset();
  }
}


void
psp_audio_pause()
{
  psp_audio_sleep = 1;
}

void
psp_audio_resume()
{
  psp_audio_sleep = 0;
}


