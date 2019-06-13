/***************************************************************************/
/*                                                                         */
/*                         Sound Source File                               */
/*                                                                         */
/*     Initialisation, shuting down and PC Engine generation of sound      */
/*                                                                         */
/***************************************************************************/

/* Header */

#include <malloc.h>
#include "utils.h"
#include "sound.h"

/* Variables definition */

UChar sound_driver = 1;
// 0 =-¯ No sound driver
// 1 =-¯ Allegro sound driver
// 2 =-¯ Seal sound driver
// 3 =-¯ SDL/SDL_Mixer driver

char MP3_playing = 0;
// is MP3 playing ?

char *sbuf[6];
// the buffer where the "DATA-TO-SEND-TO-THE-SOUND-CARD" go
// one for each channel

UChar new_adpcm_play = 0;
// Have we begun a new adpcm sample (i.e. must be reset adpcm index/prev value)

// indicates the last time music has been "released"

// the freq of the internal PC Engine CPU
// the sound use a kind of "relative" frequency
// I think there's a pb with this value that cause troubles with some cd sync

// mono or stereo, to remove later

UInt32 dwNewPos;

UInt32 AdpcmFilledBuf = 0;
// Size (in nibbles) of adpcm buf that has been filled with new data

/* Functions definition */
int InitSound(void)
{
  silent = 0;

  for (silent = 0; silent < 6; silent++) {
    sbuf[silent] = (char *) calloc(sizeof(char), 2*SBUF_SIZE_BYTE);
  }

  return 0;
}

void
TrashSound (void)		/* Shut down sound  */
{
}

/* TODO : doesn't support repeat mode for now */


