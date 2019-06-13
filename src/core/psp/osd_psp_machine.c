/****************************************************************************
 * Hu-Go! PSP
 *
 * machine functions
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include <pce.h>
#include <utils.h>
#include <malloc.h>

#include <pspctrl.h>
#include <psptypes.h>
#include <png.h>

# ifndef LINUX_MODE
#include <pspgu.h>
#include <pspdisplay.h>
#include <psprtc.h>
# endif


char initial_path[PATH_MAX] = "";
// prefered path for for searching

UInt16* osd_gfx_buffer = NULL;

UChar gamepad = 0;
// gamepad detected ?

//LUDO: UChar* XBuf;
// The screen buffer where we draw before blitting it on screen

int gamepad_driver = 0;
// what kind of jypad must we have to handle

char dump_snd = 0;
// Do we write sound to file

char synchro;
// … fond, … fond, … fond? (french joke ;)

UInt32 interrupt_60hz(UInt32, void*);
// declaration of the actual callback to call 60 times a second

extern void ogc_video__init(void);
extern void dvd_drive_detect();
extern void credits();
extern void pourlogo();
extern void unpack();

int hugoromsize;
unsigned char *hugorom;

int osd_init_machine(void)
{
# if 0 //LUDO: TO_BE_DONE !
  /* Initialize Video */
  ogc_video__init();

#ifndef HW_RVL
  /* initialize DVD drive */
  DVD_Init ();
  dvd_drive_detect();
#endif

  /* initialize LibFAT */
  fatInitDefault();

  /* Intro Screen */
  unpack();
  pourlogo();
  credits();
  
  if (!(XBuf = (UChar*)malloc(XBUF_WIDTH * XBUF_HEIGHT)))
  {
    printf (MESSAGE[language][failed_init]);
    return (0);
  }
  bzero (XBuf, XBUF_WIDTH * XBUF_HEIGHT);

  InitSound();

  osd_gfx_buffer = XBuf + 32 + 64 * XBUF_WIDTH; // We skip the left border of 32 pixels and the 64 first top lines

  /* Allocate cart_rom here */
  hugorom = memalign(32, 2621440);
  memset(hugorom, 0, 2621440);
  hugoromsize = 0;
# else

  //LUDO: osd_gfx_buffer = malloc( XBUF_WIDTH * XBUF_HEIGHT );
  //LUDO: bzero( osd_gfx_buffer, XBUF_WIDTH * XBUF_HEIGHT );
  extern SDL_Surface* blit_surface;
  osd_gfx_buffer = blit_surface->pixels;

  InitSound();

# endif
  return 1;
}


/*****************************************************************************

    Function: osd_shut_machine

    Description: Deinitialize all stuff that have been inited in osd_int_machine
    Parameters: none
    Return: nothing

*****************************************************************************/
void
osd_shut_machine (void)
{
}

/*****************************************************************************

    Function: osd_keypressed

    Description: Tells if a key is available for future call of osd_readkey
    Parameters: none
    Return: 0 is no key is available
            else any non zero value

*****************************************************************************/
SChar osd_keypressed(void)
{
	return 0;
}

/*****************************************************************************

    Function: osd_readkey

    Description: Return the first available key stroke, waiting if needed
    Parameters: none
    Return: the key value (currently, lower byte is ascii and higher is scancode)

*****************************************************************************/
UInt16 osd_readkey(void)
{
	return 0;
}

 /*****************************************************************************

    Function: osd_fix_filename_slashes

    Description: Changes slashes in a filename to correspond to an os need
    Parameters: char* s
    Return: nothing but the char* is updated

*****************************************************************************/
void osd_fix_filename_slashes(char* s)
{
}

/*****************************************************************************

    Function: osd_init_paths

    Description: set global variables for paths and filenames
    Parameters: int argc, char* argv[]   same as the command line parameters
    Return: nothing

*****************************************************************************/
void
osd_init_paths(int argc, char* argv[])
{
}
