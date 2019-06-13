/****************************************************************************
 * Nintendo Gamecube Mixer
 ****************************************************************************/

extern void MixStereoSound( unsigned short *dst, int samples );
extern void InitPSG();
extern void processPSGEvent( int event, unsigned char data );

/*** Main Channel Structure ***/
typedef struct {

        /*** Main ***/
        int enabled;
        unsigned int phase;

        /*** Volume ***/
        unsigned int balanceL;
        unsigned int balanceR;
        unsigned int channelVolume;
        int outVolumeL;
        int outVolumeR;

        /*** Frequency ***/
        unsigned int frequency;
        unsigned int deltaFreq;

        /*** Wave Data ***/
        int waveindex;
        int wavedata[32];

        /*** Direct Data ***/
	int ddabytecount;
	int ddabytesperframe;
        int ddaenabled;
        int ddaindex;
        int ddaphase;
        int ddadata[1024];		/*** Allow for a few frames ***/

        /*** Noise ***/
        int noiseenabled;
        unsigned int noisefrequency;
        unsigned int deltaNoise;

} PSGCHANNEL;

