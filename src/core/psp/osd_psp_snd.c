#include "utils.h"
#include "osd_psp_snd.h"

#include <SDL.h>
# ifdef PSP_AUDIO
# include "psp_audio.h"
# endif

# ifndef PSP_AUDIO
static void 
osd_snd_fill_audio(void *data, Uint8 *stream, int len) 
{
  MixSound( stream, len >> 2 );
}
# endif

void 
osd_snd_set_volume(UChar v)
{
}


int 
osd_snd_init_sound(void)
{
  SDL_AudioSpec wanted;
  SDL_AudioSpec got;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO))
  {
    printf("SDL_InitSubSystem AUDIO failed at %s:%d - %s\n", __FILE__, __LINE__, SDL_GetError());
    return 0;
  }

  memset( &wanted, 0, sizeof(wanted));
  wanted.freq     = 44100;
  wanted.format   = AUDIO_S16;
  wanted.samples  = 1024;  /* Good low-latency value for callback */
  wanted.channels = 2;
# ifndef PSP_AUDIO
  wanted.callback = osd_snd_fill_audio;
  wanted.userdata = 0;     /* Open the audio device, forcing the desired format */
# endif

# ifdef LINUX_MODE
fprintf(stdout, "wanted: channels %d\n", wanted.channels);
fprintf(stdout, "wanted: freq     %d\n", wanted.freq    );
fprintf(stdout, "wanted: samples  %d\n", wanted.samples );
# endif

# ifndef PSP_AUDIO
  if (SDL_OpenAudio(&wanted, &got) < 0)
  {
    Log("Couldn't open audio: %s\n", SDL_GetError());
    return 0;
  }
# ifdef LINUX_MODE
fprintf(stdout, "got: format %x\n", got.format );
fprintf(stdout, "got: signed %d\n", got.format >= 0x8000 );
# endif

  host.sound.stereo = (got.channels == 2);
  host.sound.sample_size = got.samples;
  host.sound.freq = got.freq;
  host.sound.signed_sound = (got.format >= 0x8000);

# ifdef LINUX_MODE
fprintf(stdout, "got: sample: %d\n", got.samples);
fprintf(stdout, "got: freq  : %d\n", got.freq);
# endif

  SDL_PauseAudio(SDL_DISABLE);
# else // PSP_AUDIO
  host.sound.stereo = (wanted.channels == 2);
  host.sound.sample_size = wanted.samples;
  host.sound.freq = wanted.freq;
  host.sound.signed_sound = (wanted.format >= 0x8000);

  psp_audio_init();
# endif

# ifndef PSP_AUDIO
   atexit(SDL_CloseAudio);
# else
   atexit(psp_audio_end);
# endif

  /*** initialize PSG channels */
  InitPSG();

  return 1;
}


void 
osd_snd_trash_sound(void)
{
  UChar chan;
#if !defined(SDL_mixer)
	SDL_CloseAudio();
	
#else //SDL_mixer
	//needed to stop properly...
	Callback_Stop=TRUE;
	//SDL_Delay(1000);
	Mix_CloseAudio();
#endif

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
