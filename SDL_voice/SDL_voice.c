/* 
 */

#include "SDL_voice.h"
/*
#include <stdio.h>
#include <math.h>
#include	 <sndfile.h>
#include "SDL/SDL.h"
*/

#define Tao 6.283185307179f
#define ChromaticRatio 1.059463094359f
#define MaxAmp 32767
/*
#define SpecFreq 44100
*/
float masterAmpL = 1;
float masterAmpR = 1;

// typedef struct {
// 	Sint16  *samples; /* array of samples */
// 	SDL_bool engaged; /* do nothing unless this is true */
// 	SDL_bool repeat;  /* if false then disengage when the end of the samples is reached */
// 	int      len;     /* sampleCount; how many samples total in the 'samples' array */
// 	int      srcBeg;  /* where to start picking from the source samples to the dest samples */
// 	int      srcLen;  /* how many samples total will be spanned when picking */
// 	float    dstPos;  /* where the mixer left off in the dstLen */
// 	float    dstLen;  /* how many samples total will be picked to adjust frequency */
// 	float    ampL;    /* multiply amplitude for left channel */
// 	float    ampR;    /* multiply amplitude for right channel */
// } voice;

int buildVoice(voice *v, int len)
{
	if ( (v->samples = malloc( sizeof(Sint16)*len )) == NULL )
		return -1;
	v->engaged = SDL_FALSE;
	v->repeat  = SDL_TRUE;
	v->len     = len;
	v->srcBeg  = 0;
	v->srcLen  = len;
	v->dstPos  = 0;
	v->dstLen  = len;
	v->ampL    = 1;
	v->ampR    = 1;
	return 0;
}

int getDstLen(float pitch_0isC0)
{
	return (SpecFreq/(440*powf(ChromaticRatio, pitch_0isC0-57)))+0.5f;
}

void buildSineWave(voice *v)
{
	int i;
	for (i=0; i < v->len; i++)
		v->samples[i] = MaxAmp*sin( i*(Tao/v->len) );
}

/*
typedef struct {
	voice *v;
	int len;
} voiceList;
*/

int buildVoiceList(voiceList *vl, int len)
{
	if ( (vl->v = malloc( sizeof(voice)*len )) == NULL )
		return -1;
	vl->len = len;
	return 0;
}


void freeVoices(voiceList *vl)
{
	int i;
	for (i=0; i < vl->len; i++)
		free(vl->v[i].samples);
	free(vl->v);
}


int getVoiceFromFile(voice *v, char *infilename, float mixL, float mixR)
{
	SNDFILE	*infile = NULL;
	SF_INFO	sfinfo;
	
	if (( infile = sf_open(infilename, SFM_READ, &sfinfo) )==NULL)
	{	
		printf("Not able to open input file %s.\n", infilename);
		puts( sf_strerror(NULL) );
		return -1;
	}
	
	
	if ( buildVoice(v, 344000) != 0 )//until i figure out how to get the length of the SNDFILE 
		return -2;
	
	
	int blockSize = v->len;
	float buf[sfinfo.channels * blockSize];
	float sum;
	float mixLR[2] = {mixL, mixR};
	int k, m;
	while (( blockSize = sf_readf_float(infile, buf, blockSize) )>0)
	{
		for (k = 0; k < blockSize; k++)
		{
			for (m = 0; m < sfinfo.channels; m++)
			{
				if (m < 2)// ignore other channels for now...
				{
					//printf("buf[k * sfinfo.channels + m]:%6f\n", buf[k * sfinfo.channels + m]);
					sum += buf[k * sfinfo.channels + m] * mixLR[m];
				}
			}
			//printf("       sum:%f\n", sum);
			v->samples[k] = (Sint16)( (MaxAmp*sum)/m );//cast may not be needed
			//printf("v->samples[k]:%6d\n", v->samples[k]);
			sum = 0;
		}
	}
	
	sf_close(infile);
	return 0;
}



//for debugging:
void logVoice(voice *v, char c)
{
	printf("%c\n", c);
	if (v->samples == NULL)
		printf(" v->samples == NULL\n");
	printf(
		" v->len: %4d\n v->srcBeg: %4d\n v->srcLen: %4d\n v->dstPos: %f\n v->dstLen: %f\n v->ampL: %f\n v->ampR: %4f\n",
		v->len,
		v->srcBeg,
		v->srcLen,
		v->dstPos,
		v->dstLen,
		v->ampL,
		v->ampR
	);
	if (v->engaged)
		printf(" engaged\n");
	else
		printf(" not engaged\n");
	
	if (v->repeat)
		printf(" repeat is on\n");
	else
		printf(" repeat is off\n");
	
	printf("\n");
}
void logSpec(SDL_AudioSpec *as, char c)
{
	printf("%c\n", c);
	printf(
		" freq    : %d\n format  : %d\n channels: %d\n silence : %d\n samples : %d\n size    : %d\n\n",
		(int) as->freq,
		(int) as->format,
		(int) as->channels,
		(int) as->silence,
		(int) as->samples,
		(int) as->size
	);
}
void logSamples(voice *v, int i, int howMany, char c)
{
	printf("%c\n", c);
	for (; i < howMany  &&  i < v->len; i++)
		printf(" %4d: %4d\n", i, (int)v->samples[i]);
}



Sint16 clipAmp(int sampleVal)
{
	if (sampleVal > MaxAmp) return MaxAmp;
	else if (sampleVal < -MaxAmp) return -MaxAmp;
	else return sampleVal;
}

void fillAudioBuffer(void *voidVL, Uint8 *stream8, int streamLen)
{
	Sint16 *stream16 = (Sint16*)stream8;
	streamLen /= 2;
	
	voiceList *vl = (voiceList*)voidVL;
	
	float sampleSumL, sampleSumR;
	int i, j, srcSampleIndex, srcSample;
	for (i = 0; i<streamLen; i+=2)
	{
		for (j=0; j < vl->len; j++)
		{
			//v = &sharedVL.v[j];
			//logVoice(v, 'c');
			if (vl->v[j].engaged)
			{
				srcSampleIndex = (vl->v[j].dstPos / vl->v[j].dstLen) * (vl->v[j].srcLen - vl->v[j].srcBeg) + vl->v[j].srcBeg;
				//printf("srcSampleIndex: %d\n", srcSampleIndex);
				
				if (srcSampleIndex >= vl->v[j].len)
					srcSampleIndex -= vl->v[j].len;
				
				srcSample = vl->v[j].samples[srcSampleIndex];
				//printf("srcSample: %d\n", srcSample);
				
				sampleSumL += clipAmp(srcSample * vl->v[j].ampL);
				sampleSumR += clipAmp(srcSample * vl->v[j].ampR);
				//printf("sampleSumL: %d\n", sampleSumL);
				//printf("sampleSumR: %d\n", sampleSumR);
				//printf("\n");
				
				vl->v[j].dstPos++;
				
				if (vl->v[j].dstPos  >=  vl->v[j].dstLen)
				{
					vl->v[j].dstPos = 0;
					if (!vl->v[j].repeat)
					{
						vl->v[j].engaged = SDL_FALSE;
					}
				}
			}
		}
		stream16[i]   = clipAmp( (sampleSumL / vl->len) * masterAmpL );
		stream16[i+1] = clipAmp( (sampleSumR / vl->len) * masterAmpR );
		//printf("stream16[i]  : %4d\n", stream16[i]  );
		//printf("stream16[i+1]: %4d\n", stream16[i+1]);
		
		sampleSumL = sampleSumR = 0;
	}
}



