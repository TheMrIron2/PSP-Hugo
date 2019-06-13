/****************************************************************************
 * Nintendo Gamecube Audio - Hu-Go!
 *
 * This audio mixer is based on those contained in 
 *
 *      Charles McDonald's TG16 Emulator
 *      Ki's PC2E
 *      Zeograd's Hu-Go!
 *
 * ... and my little additions :)
 *
 * softdev December 2005
 *****************************************************************************/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "osd_psp_mix.h"
#include "Hugo.h"
#include "hard_pce.h"

#define PSG_FREQ 3579545
#define FMUL 65536.0 * 256.0 * 8.0
#define OVERSAMPLE 8
#define MAXCHAN 6
#define MAX_AMP 512
#define PCE_RATE 22050
#define CHRATE PCE_RATE / 4

# define hostSampleRate22K 22050
# define hostSampleRate44K 44100

/*** Globals ***/
static int hostSampleMode = 0;
static int hostSampleRate = hostSampleRate22K;

unsigned int mainVolumeL;
unsigned int mainVolumeR;
unsigned int FMEnabled = 0;
unsigned int LFOShift = 0;
unsigned int LFOFrequency = 0;
unsigned int LFOControl = 0;
unsigned int psgchannel = 0;

static int UserVolumePerCent = 100; /*** This could be used as a volume control ***/

#define DDATEMP 6
int ddai[7][2048];                        /*** Allow up to 2k sample reads ***/

/*** Support ***/
int VolumeTable[92];
int NoiseTable[32768];

/*** Main Channel Structure ***/
PSGCHANNEL psg[8];              /*** Six PSG channels ***/

static inline int saturate(int val, int min, int max)
{    
  if (val < min) return min;
  if (val > max) return max;
  return val;
}

/****************************************************************************
 * ResetSound
 ****************************************************************************/
void ResetSound()
{
  int i;

  memset(&psg[0], 0, sizeof(PSGCHANNEL) * 6);

  mainVolumeL = mainVolumeR = 0;
  FMEnabled = 0;
  LFOShift = LFOFrequency = LFOControl = 0;
  psgchannel = 0;

  io.psg_volume = 0;
  io.psg_ch = 0;

  for (i = 0; i < 6; i++) {
      io.PSG[i][4] = 0x80;
  }

  if (hostSampleMode & 1) hostSampleRate = hostSampleRate44K;
  else                    hostSampleRate = hostSampleRate22K;

  HCD_mp3_reset();
}

/****************************************************************************
 * GenerateNoiseTable
 *
 * This is based on the SN76489 15-bit pseudo random sequencer.
 * Documented by John Kortink.
 ****************************************************************************/
void GenerateNoiseTable()
{
     int i;    
     unsigned int bit0, bit1, bit14;
     unsigned int seed = 0x100;

     for ( i = 0; i < 32768; i++ )
     {
         bit0 = seed & 1;
         bit1 = (seed & 2) >> 1;
         bit14 = ( bit0 ^ bit1 );
         seed >>= 1;
         seed |= ( bit14 << 14 );
         NoiseTable[i] = ( bit0 ) ? 15 : -16 ;
     }  
}

/****************************************************************************
 * UpdateVolume
 *
 * Ki's original system was based on additive volumes. However, this still
 * produced sound output, even though the channel was muted through chvl.
 *
 * As a side issue, this also seems to give better stereo separation.
 ****************************************************************************/
void UpdateVolume( int channel )
{   
     int l,r;
     
     l = ( psg[channel].channelVolume ) ?
         ( psg[channel].channelVolume + 
         ( psg[channel].balanceL + mainVolumeL ) * 2 ) : 0;

     r = ( psg[channel].channelVolume ) ?
         ( psg[channel].channelVolume + 
         ( psg[channel].balanceR + mainVolumeR ) * 2 ) : 0;
         
     psg[channel].outVolumeL = VolumeTable[l];
     psg[channel].outVolumeR = VolumeTable[r];
}

/****************************************************************************
 * GenerateVolumeTable
 *
 * This is taken from Ki's PC2E, but it's pretty standard stuff.
 ****************************************************************************/
void GenerateVolumeTable()
{
        int        i;

        for (i = 0; i <= 91; i++)
        {
                double v = (double)(91 - i);
                VolumeTable[i] = (int)(32768.0 * pow(10.0, v * (-1.5) / 20.0));
        }
        
}

/****************************************************************************
 * Update PSG Frequency
 *
 * Can be used for all channels, including 1.
 * However, any value will be overwritten if FM Synthesis in enabled.
 ****************************************************************************/
void UpdatePSGFrequency( unsigned int freq, int channel )
{
     freq = ( psg[channel].frequency - 1 ) & 0xfff;
     
     if ( psg[channel].frequency )
        psg[channel].deltaFreq = (unsigned int)(FMUL * PSG_FREQ / 
                             (double) freq / hostSampleRate / OVERSAMPLE);
     else
        psg[channel].deltaFreq = 0;
     
}

static inline void UpdatePSGFrequency_44K( unsigned int freq, int channel )
{
     freq = ( psg[channel].frequency - 1 ) & 0xfff;
     
     if ( psg[channel].frequency )
        psg[channel].deltaFreq = (unsigned int)(FMUL * PSG_FREQ / 
                             (double) freq / hostSampleRate44K / OVERSAMPLE);
     else
        psg[channel].deltaFreq = 0;
     
}

static inline void UpdatePSGFrequency_22K( unsigned int freq, int channel )
{
     freq = ( psg[channel].frequency - 1 ) & 0xfff;
     
     if ( psg[channel].frequency )
        psg[channel].deltaFreq = (unsigned int)(FMUL * PSG_FREQ / 
                             (double) freq / hostSampleRate22K / OVERSAMPLE);
     else
        psg[channel].deltaFreq = 0;
     
}


/****************************************************************************
 * DecodePSGEvent
 *
 * This is a replacement for the event decoder in PCE.C
 * I have tried to interpret the values according to Paul Clifford's doc.
 ****************************************************************************/
void processPSGEvent( int event, unsigned char data )
{
   if (hostSampleMode & 0x2) {
     int i;
     
     switch( event )     
     {
             case 0:    /*** Channel Select ***/
             
                  psgchannel = data & 7;
                  break;

             case 1:    /*** Global Sound Balance ***/
             
                  mainVolumeL = (( data & 0xf0 ) >> 4 );
                  mainVolumeR = ( data & 0x0f );
                  
                  /*** Update sub volumes ***/
                  for( i = 0; i < MAXCHAN; i++ )
                       UpdateVolume(i);
                       
                  break;
                               
             case 2:    /*** Fine frequency adjust ***/

                  psg[psgchannel].frequency = ( psg[psgchannel].frequency & 0xf00 ) | data;
                  UpdatePSGFrequency( psg[psgchannel].frequency, psgchannel );
                  break;
                  
             case 3:    /*** Rough Frequency adjust ***/    

                  psg[psgchannel].frequency = ( psg[psgchannel].frequency & 0x0ff ) 
                                              | ( ( data & 0x0f ) << 8 );
                  UpdatePSGFrequency( psg[psgchannel].frequency, psgchannel );
                  break;
                  
             case 4:    /*** Channel On/Off DDA ***/

                  psg[psgchannel].channelVolume = data & 0x1f;
                  psg[psgchannel].enabled = data & 0x80;
                  psg[psgchannel].ddaenabled = data & 0x40;
                                   
                  if ( ( data & 0xc0 ) == 0x40 )
                     psg[psgchannel].waveindex = 0;
                  
                  UpdateVolume(psgchannel);
                  
                  break;
                  
             case 5:    /*** Channel Balance ***/

                  psg[psgchannel].balanceL = ( data & 0xf0 ) >> 4;
                  psg[psgchannel].balanceR = ( data & 0x0f );
                  UpdateVolume(psgchannel);                  
                  break;
                  
             case 6:    /*** Channel Data ***/

                  if ( !psg[psgchannel].enabled && !psg[psgchannel].ddaenabled )
                  {  /*** Loading wave data ***/
                     psg[psgchannel].wavedata[psg[psgchannel].waveindex++] = (long)( data & 0x1f ) - 16;
                     psg[psgchannel].waveindex &= 0x1f;
                  }
                  
                  if ( psg[psgchannel].enabled && psg[psgchannel].ddaenabled )
                  {                      
                       psg[psgchannel].ddadata[psg[psgchannel].ddaindex++] = (long)( data & 0x1f ) - 16;
                       psg[psgchannel].ddaindex &= 0x3ff;
                       psg[psgchannel].ddabytecount++;
                  }
                  
                  break;
                  
             case 7:    /*** Noise ***/

                  psg[psgchannel].noiseenabled = data & 0x80;
                  psg[psgchannel].noisefrequency = 0x1f ^ ( data & 0x1f );
                  if ( psg[psgchannel].noisefrequency == 0 )
                     psg[psgchannel].noisefrequency = 1;

                  psg[psgchannel].deltaNoise = (unsigned int)(131072.0 * PSG_FREQ / 64.0
                                               / (double) psg[psgchannel].noisefrequency /
                                               hostSampleRate / OVERSAMPLE );
                  break;     
                  
             case 8:     /*** LFO Frequency ***/

                  LFOFrequency = data;
                  break;
                  
             case 9:    /*** LFO FM Control ***/

                  FMEnabled = (( data & 0x80 ) == 0);
                  
                  LFOControl = data & 3;
                  if ( LFOControl > 1 )
                     LFOShift = 1 << ( data & 3 );
                  else
                     LFOShift = 0;
                     
                  /*** Setup channel l for FM or not ***/
                  if ( FMEnabled )
                  {
                       if ( psg[1].frequency && LFOFrequency )
                       {  
                            psg[1].frequency *= LFOFrequency;
                            UpdatePSGFrequency( psg[1].frequency, 1 );
                       }
                       else
                       {
                           FMEnabled = 0;
                       }
                  }
                  else
                  {
                      if ( psg[1].frequency )
                         UpdatePSGFrequency( psg[1].frequency, 1 );
                      else
                         psg[1].deltaFreq = 0;
                  }
                  break;
     }

  } else {

     switch ( event )
     {

       /* Select PSG channel */
     case 0:
       io.psg_ch = data & 7;
       return;

       /* Select global volume */
     case 1:
       io.psg_volume = data;
       return;

     /* Frequency setting, 8 lower bits */
     case 2:
       io.PSG[io.psg_ch][2] = data;
       break;

     /* Frequency setting, 4 upper bits */
     case 3:
       io.PSG[io.psg_ch][3] = data & 15;
       break;

     case 4:
       io.PSG[io.psg_ch][4] = data;
       break;

       /* Set channel specific volume */
     case 5:
       io.PSG[io.psg_ch][5] = data;
       break;

       /* Put a value into the waveform or direct audio buffers */
     case 6:
       if (io.PSG[io.psg_ch][PSG_DDA_REG] & PSG_DDA_DIRECT_ACCESS)
         {
           io.psg_da_data[io.psg_ch][io.psg_da_index[io.psg_ch]] = data;
           io.psg_da_index[io.psg_ch] = (io.psg_da_index[io.psg_ch] + 1) & 0x3FF;
           if (io.psg_da_count[io.psg_ch]++ > (PSG_DIRECT_ACCESS_BUFSIZE - 1))
       {
         io.psg_da_count[io.psg_ch] = 0;
       }
         }
       else
         {
           io.wave[io.psg_ch][io.PSG[io.psg_ch][PSG_DATA_INDEX_REG]] = data;
           io.PSG[io.psg_ch][PSG_DATA_INDEX_REG] = (io.PSG[io.psg_ch][PSG_DATA_INDEX_REG] + 1) & 0x1F;
         }
       break;

     case 7:
       io.PSG[io.psg_ch][7] = data;
       break;

     case 8:
       io.psg_lfo_freq = data;
       break;

     case 9:
       io.psg_lfo_ctrl = data;
       break;
     }
  }
}

/****************************************************************************
 * DDASampleRate
 *
 * This is called at the completion of each video frame, although it could
 * also be called at a regular 60hz interval
 ****************************************************************************/
void DDASampleRate()
{
        int i;

        for ( i = 0; i < 6; i++ )
        {
                if ( psg[i].ddabytecount ) {
                        psg[i].ddabytesperframe = psg[i].ddabytecount;
                        psg[i].ddabytecount = 0;
                }
        }
}

/****************************************************************************
 * Interpolate Samples
 *
 * This appears to work fine with Speech samples in SF2 and Strip Fighter.
 ****************************************************************************/
void 
interpolate( int channel, int samples )
{
        int i;
        int osamples;
        double xPos, interpolatefactor;
        int chrate;

        chrate = psg[channel].ddabytesperframe * 60;

        if ( chrate == 0 ) {
                memset(&ddai[channel], 0, samples);
                return;
        }

        interpolatefactor = (double)hostSampleRate / (double) chrate;
        osamples = psg[channel].ddabytesperframe;

        /*** Build the sample buffer ***/
        for ( i = 0; i < osamples; i++ )
        {
                /*** Don't read more than I have ***/
                if ( psg[channel].ddaphase != psg[channel].ddaindex )
                {
                        ddai[DDATEMP][i] = psg[channel].ddadata[psg[channel].ddaphase];
                        /*
                         * v0.0.1a
                         * Be defensive - clear out as you take
                         */
                        psg[channel].ddadata[psg[channel].ddaphase++] = 0;
                        psg[channel].ddaphase &= 0x3ff;
                } else {
                        ddai[DDATEMP][i] = 0;
                }
        }        

        /*** Now build and interpolate to new channel ***/
        xPos = 0.0;
        interpolatefactor = (double)chrate / (double)hostSampleRate;
        for ( i = 0; i < samples; i++ )
        {
                ddai[channel][i] = ddai[DDATEMP][(int)xPos];
                xPos += interpolatefactor;
        }        
}

/****************************************************************************
 * MixStereoSound
 *
 * This function takes care of decoding the data provided by processPSGEvent
 * 
 * NOTE: The `samples' parameter is seen from the PCE point of view!
 *       So pass samples as 1/4 the output buffer size!
 *
 * On the NGC, this is the audio DMA callback. It's relatively easy to
 * add the main_block, and use as a standard SDL callback.
 ****************************************************************************/
static void 
loc_MixStereoSound_44K( unsigned short *dst, int samples )
{
     int i, j;
     unsigned int phase, dp;
     int volumeL, volumeR;
     int sampleL;
     int tsampleL, tsampleR;
     int *wave;
     int lfo;

     int dda[6] = { 0, 0, 0, 0, 0, 0 };
     for ( j = 0; j < samples; j++ )        /*** Outer loop for all samples ***/
     {
         tsampleL = tsampleR = 0;
         
         for ( i = 0; i < 6; i++ )    /*** Inner loop on channels ***/
         {
            PSGCHANNEL *p_psgi = &psg[i];

             /*** First check, is this channel enabled ***/
             if ( p_psgi->enabled == 0 )
                continue;
            
             /*** Is this channel 1, with FM Enabled ***/
             if ( ( i == 1 ) && FMEnabled )
                continue;

             /*** Copy volumes ***/
             volumeL = p_psgi->outVolumeL;
             volumeR = p_psgi->outVolumeR;
                          
             /*** Is there any Direct Data ***/
             if ( p_psgi->ddaenabled )
             {
                /*** DDA Data should be pushed into the current output stream 'as is'.
                 *** However, for my purposes, I'm assuming an 8Khz DAC was used ***/
                if ( dda[i] == 0 ) {
                   interpolate(i, samples);
                   dda[i] = 1;
                }
                sampleL = ddai[i][j];
                tsampleL += ( sampleL * volumeL );
                tsampleR += ( sampleL * volumeR );
                continue;

             }   
          
             /*** Noise ***/
             /*
              * v0.0.1a - Check channel is valid
               * This is due to Devil's Crush Enabling noise on CH 0
              */
             if ( p_psgi->noiseenabled && ( i > 3 ))
             {
                dp = p_psgi->deltaNoise;
                phase = p_psgi->phase;
                  
                sampleL  = NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL /= OVERSAMPLE;
                
                tsampleL += ( sampleL * volumeL );
                tsampleR += ( sampleL * volumeR );
                
                p_psgi->phase = phase;
                
             } else 
             {
                    /*** Apply FM Synthesis ***/
                    if ( i == 0 && FMEnabled && LFOControl )
                    {
                       lfo = psg[1].wavedata[(psg[1].deltaFreq * 8 ) >> 27] << LFOShift;

                       /*** Paul Clifford's document states that the frequency is updated permanently ***/
                       psg[0].frequency += lfo;
                       UpdatePSGFrequency_44K(psg[0].frequency, 0);

                       dp = psg[0].deltaFreq; 
                       phase = psg[0].phase;
                       wave = &psg[0].wavedata[0];
                       
                       sampleL  = wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL /= OVERSAMPLE;
                       
                       tsampleL += ( sampleL * volumeL );
                       tsampleR += ( sampleL * volumeR );
                              
                       psg[0].phase = phase;                   
                    }
                    else
                    {
                        if ( p_psgi->deltaFreq )
                        {
                             
                           dp = p_psgi->deltaFreq;
                           phase = p_psgi->phase;
                           wave = &p_psgi->wavedata[0];
                           
                           sampleL  = wave[phase>>27];        phase += dp;                                           
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL /= OVERSAMPLE;                   

                           tsampleL += ( sampleL * volumeL );
                           tsampleR += ( sampleL * volumeR );
                           
                           p_psgi->phase = phase;
                                                            
                        }
                    }
              } /*** End enabled channel ***/
                                         
         } /*** End channel loop ***/

         /*** Normalise ***/
         tsampleL *= UserVolumePerCent;
         tsampleR *= UserVolumePerCent;

         tsampleL = saturate(tsampleL >> 12, -32768, 32767);
         tsampleR = saturate(tsampleR >> 12, -32768, 32767);

         *dst++ = (unsigned short)tsampleL;
         *dst++ = (unsigned short)tsampleR;
                                                            
     }   /*** End outer loop ***/
}

static void 
loc_MixStereoSound_22K( unsigned short *dst, int samples )
{
     int i, j;
     unsigned int phase, dp;
     int volumeL, volumeR;
     int sampleL;
     int tsampleL, tsampleR;
     int *wave;
     int lfo;

     int dda[6] = { 0, 0, 0, 0, 0, 0 };
     for ( j = 0; j < samples; j++ )        /*** Outer loop for all samples ***/
     {
         tsampleL = tsampleR = 0;
         
         for ( i = 0; i < 6; i++ )    /*** Inner loop on channels ***/
         {
            PSGCHANNEL *p_psgi = &psg[i];

             /*** First check, is this channel enabled ***/
             if ( p_psgi->enabled == 0 )
                continue;
            
             /*** Is this channel 1, with FM Enabled ***/
             if ( ( i == 1 ) && FMEnabled )
                continue;

             /*** Copy volumes ***/
             volumeL = p_psgi->outVolumeL;
             volumeR = p_psgi->outVolumeR;
                          
             /*** Is there any Direct Data ***/
             if ( p_psgi->ddaenabled )
             {
                /*** DDA Data should be pushed into the current output stream 'as is'.
                 *** However, for my purposes, I'm assuming an 8Khz DAC was used ***/
                if ( dda[i] == 0 ) {
                   interpolate(i, samples);
                   dda[i] = 1;
                }
                sampleL = ddai[i][j];
                tsampleL += ( sampleL * volumeL );
                tsampleR += ( sampleL * volumeR );
                continue;

             }   
          
             /*** Noise ***/
             /*
              * v0.0.1a - Check channel is valid
               * This is due to Devil's Crush Enabling noise on CH 0
              */
             if ( p_psgi->noiseenabled && ( i > 3 ))
             {
                dp = p_psgi->deltaNoise;
                phase = p_psgi->phase;
                  
                sampleL  = NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL += NoiseTable[phase>>17];        phase += dp;
                sampleL /= OVERSAMPLE;
                
                tsampleL += ( sampleL * volumeL );
                tsampleR += ( sampleL * volumeR );
                
                p_psgi->phase = phase;
                
             } else 
             {
                    /*** Apply FM Synthesis ***/
                    if ( i == 0 && FMEnabled && LFOControl )
                    {
                       lfo = psg[1].wavedata[(psg[1].deltaFreq * 8 ) >> 27] << LFOShift;

                       /*** Paul Clifford's document states that the frequency is updated permanently ***/
                       psg[0].frequency += lfo;
                       UpdatePSGFrequency_22K(psg[0].frequency, 0);

                       dp = psg[0].deltaFreq; 
                       phase = psg[0].phase;
                       wave = &psg[0].wavedata[0];
                       
                       sampleL  = wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL += wave[phase>>27];        phase += dp;
                       sampleL /= OVERSAMPLE;
                       
                       tsampleL += ( sampleL * volumeL );
                       tsampleR += ( sampleL * volumeR );
                              
                       psg[0].phase = phase;                   
                    }
                    else
                    {
                        if ( p_psgi->deltaFreq )
                        {
                             
                           dp = p_psgi->deltaFreq;
                           phase = p_psgi->phase;
                           wave = &p_psgi->wavedata[0];
                           
                           sampleL  = wave[phase>>27];        phase += dp;                                           
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL += wave[phase>>27];        phase += dp;
                           sampleL /= OVERSAMPLE;                   

                           tsampleL += ( sampleL * volumeL );
                           tsampleR += ( sampleL * volumeR );
                           
                           p_psgi->phase = phase;
                                                            
                        }
                    }
              } /*** End enabled channel ***/
                                         
         } /*** End channel loop ***/

         /*** Normalise ***/
         tsampleL *= UserVolumePerCent;
         tsampleR *= UserVolumePerCent;

         tsampleL = saturate(tsampleL >> 12, -32768, 32767);
         tsampleR = saturate(tsampleR >> 12, -32768, 32767);

         *dst++ = (unsigned short)tsampleL;
         *dst++ = (unsigned short)tsampleR;
         *dst++ = (unsigned short)tsampleL;
         *dst++ = (unsigned short)tsampleR;
                                                            
     }   /*** End outer loop ***/
}

extern char *sbuf[6];

void 
loc_MixMonoSound_22K(unsigned short *p_stream, int len)
{
  int i;

  for (i = 0; i < 6; i++) {
    WriteBuffer_22K(sbuf[i], i, len);
  }

  /*
   * Adjust the final post-mixed left/right volumes.  0-15 * 1.22 comes out to
   * (0..18) which when multiplied by the ((-127..127) * 7) we get in the final
   * stream mix below we have (-16002..16002) which we then divide by 128 to get
   * a nice unsigned 8-bit value of 128 + (-125..125).
   */
  UChar vol = (((io.psg_volume >> 4) * 1.22) + ((io.psg_volume & 0x0F) * 1.22)) / 2;
  /*
   * Mix streams and apply master volume.
   */
  char* p_sbuf0 = sbuf[0];
  char* p_sbuf1 = sbuf[1];
  char* p_sbuf2 = sbuf[2];
  char* p_sbuf3 = sbuf[3];
  char* p_sbuf4 = sbuf[4];
  char* p_sbuf5 = sbuf[5];
  while (len-- > 0) {
    Int32 a_sample = ((Int32) 
      (((*p_sbuf0++) + (*p_sbuf1++) + (*p_sbuf2++) + (*p_sbuf3++) +
        (*p_sbuf4++) + (*p_sbuf5++)) * vol));
    a_sample *= UserVolumePerCent;
    a_sample  = saturate(a_sample >> 7, -32768, 32767);
    *p_stream++ = a_sample;
    *p_stream++ = a_sample;
    *p_stream++ = a_sample;
    *p_stream++ = a_sample;
  }
}

void 
loc_MixMonoSound_44K(unsigned short *p_stream, int len)
{
  int i;

  for (i = 0; i < 6; i++) {
    WriteBuffer_44K(sbuf[i], i, len);
  }
  /*
   * Adjust the final post-mixed left/right volumes.  0-15 * 1.22 comes out to
   * (0..18) which when multiplied by the ((-127..127) * 7) we get in the final
   * stream mix below we have (-16002..16002) which we then divide by 128 to get
   * a nice unsigned 8-bit value of 128 + (-125..125).
   */
  UChar vol = (((io.psg_volume >> 4) * 1.22) + ((io.psg_volume & 0x0F) * 1.22)) / 2;
  /*
   * Mix streams and apply master volume.
   */
  char* p_sbuf0 = sbuf[0];
  char* p_sbuf1 = sbuf[1];
  char* p_sbuf2 = sbuf[2];
  char* p_sbuf3 = sbuf[3];
  char* p_sbuf4 = sbuf[4];
  char* p_sbuf5 = sbuf[5];
  while (len-- > 0) {
    Int32 a_sample = ((Int32) 
      (((*p_sbuf0++) + (*p_sbuf1++) + (*p_sbuf2++) + (*p_sbuf3++) +
        (*p_sbuf4++) + (*p_sbuf5++)) * vol));
    a_sample *= UserVolumePerCent;
    a_sample  = saturate(a_sample >> 7, -32768, 32767);
    *p_stream++ = a_sample;
    *p_stream++ = a_sample;
  }
}

void 
MixSound( unsigned short *dst, int samples )
{
  switch (hostSampleMode) {
    case 0: loc_MixMonoSound_22K( dst, samples >> 1);
    break;
    case 1: loc_MixMonoSound_44K( dst, samples);
    break;
    case 2: loc_MixStereoSound_22K( dst, samples >> 1);
    break;
    default: loc_MixStereoSound_44K( dst, samples);
    break;
  }
}

/****************************************************************************
 * Initialise
 ****************************************************************************/
void InitPSG()
{
     GenerateNoiseTable();
     GenerateVolumeTable();
    
     /*
      * You may be thinking, why 8 PSG channels?
      *
      * This is because some ROMs have used trackers which
      * generate no output for channels 6 and 7, but select
      * them just the same, and then output updates!
      *
      * So although these 2 channels are unused, they must
      * exist to stop the decoder from eating itself :)
      */

     memset(&psg, 0, sizeof(PSGCHANNEL) * 8);
}

void
osd_psp_set_sound_freq( int mode )
{
  int channel;

  if (mode == hostSampleMode) return;

  if (mode & 1) {
    hostSampleRate = hostSampleRate44K;
    hostSampleMode = mode;
    for (channel = 0; channel < 8; channel++) {
      psg[channel].frequency /= 2;
    }
   
  } else {
    hostSampleRate = hostSampleRate22K;
    hostSampleMode = mode;
    for (channel = 0; channel < 8; channel++) {
      psg[channel].frequency *= 2;
    }
  }
}

void
osd_psp_set_sound_volume( int volume )
{
  UserVolumePerCent = volume;
}

