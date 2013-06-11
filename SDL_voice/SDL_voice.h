/* 
 */

#include <stdio.h>
#include <math.h>
#include	 <sndfile.h>
#include "SDL/SDL.h"

#define SpecFreq 44100

float masterAmpL;
float masterAmpR;


typedef struct {
	Sint16  *samples; /* array of samples */
	SDL_bool engaged; /* do nothing unless this is true */
	SDL_bool repeat;  /* if false then disengage when the end of the samples is reached */
	int      len;     /* sampleCount; how many samples total in the 'samples' array */
	int      srcBeg;  /* where to start picking from the source samples to the dest samples */
	int      srcLen;  /* how many samples total will be spanned when picking */
	float    dstPos;  /* where the mixer left off in the dstLen */
	float    dstLen;  /* how many samples total will be picked to adjust frequency */
	float    ampL;    /* multiply amplitude for left channel */
	float    ampR;    /* multiply amplitude for right channel */
} voice;

typedef struct {
	voice *v;
	int len;
} voiceList;


int  buildVoice      (voice *v,      int len);
int  getDstLen       (float pitch_0isC0);
void buildSineWave   (voice *v);
int  buildVoiceList  (voiceList *vl, int len);
void freeVoices      (voiceList *vl);
int  getVoiceFromFile(voice *v,      char *infilename, float mixL, float mixR);

Sint16 clipAmp(int sampleVal);
void fillAudioBuffer(void *voidVL, Uint8 *stream8, int streamLen);

//for debugging:
void logVoice(voice *v, char c);
void logSpec(SDL_AudioSpec *as, char c);
void logSamples(voice *v, int i, int howMany, char c);
