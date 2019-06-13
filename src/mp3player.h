// mp3player.h: headers for psp mp3player code
//
// All public functions for mp3player
//
//////////////////////////////////////////////////////////////////////
#ifndef _MP3PLAYER_H_
#define _MP3PLAYER_H_

#include <mad.h>
//#include "../../codec.h"

#ifdef __cplusplus
extern "C" {
#endif

//  Function prototypes for public functions
//    void MP3setStubs(codecStubs * stubs);
struct MP3Info
{
	int fileSize;
	char layer[10];
	int kbit;
	long hz;
	char mode[50];
	char emphasis[10];
	mad_timer_t length;
	char strLength[10];
	int frames;
};

//private functions
    void MP3_Init();
    int MP3_Play();
    void MP3_Pause();
    int MP3_Stop();
    void MP3_End();
    void MP3_FreeTune();
    int MP3_Load(char *filename);
    void MP3_GetTimeString(char *dest);
    int MP3_EndOfStream();
	struct MP3Info MP3_GetInfo();
	int MP3_GetStatus();
	int MP3_GetPercentace();
#ifdef __cplusplus
}
#endif
#endif
