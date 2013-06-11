/* 
 * compile with:
 * 	gcc -Wall SDL_voice.c test_SDL_voice.c -o test_SDL_voice -lSDL
 */

#include "SDL_voice.h"

#include "SDL/SDL.h"
#include <stdio.h>


/*
#define SpecFreq 44100
*/

int main(void)
{
	voiceList sharedVL;
	buildVoiceList(&sharedVL, 3);
	
	buildVoice(&sharedVL.v[0], 2);
	sharedVL.v[0].samples[0] =  10000;
	sharedVL.v[0].samples[1] = -10000;
	//logVoice(&sharedVL.v[0], 'a');
	
	buildVoice(&sharedVL.v[1], getDstLen(0));
	buildSineWave(&sharedVL.v[1]);
	//logVoice(&sharedVL.v[1], 'b');
	
	getVoiceFromFile(&sharedVL.v[2], "explosionSound.wav", 1,1);
	//logVoice(&sharedVL.v[2], 'c');
	//logSamples(sharedVL.v[2], 0, 1000, 'a');
	
	
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER);
	SDL_AudioSpec as;
	as.freq = SpecFreq;
	as.format = AUDIO_S16SYS;
	as.channels = 2;
	as.samples = 1024;
	as.callback = fillAudioBuffer;
	as.userdata = &sharedVL;
	//logSpec(&as, 'a');
	SDL_OpenAudio(&as, NULL);
	//logSpec(&as, 'b');
	
	
	SDL_PauseAudio(0);
	
	
	int i, newDstLen0, newDstLen1;
	for (i=34; i<37; i++)
	{
		newDstLen0 = getDstLen(i);
		newDstLen1 = getDstLen(i+7);
		
		// sine left
		SDL_LockAudio();
		sharedVL.v[0].dstLen = newDstLen0;
		sharedVL.v[1].dstLen = newDstLen1;
		sharedVL.v[0].engaged = SDL_FALSE;
		sharedVL.v[1].engaged = SDL_TRUE;
		sharedVL.v[1].ampR = 0;
		sharedVL.v[1].ampL = 1;
		SDL_UnlockAudio();
		
		SDL_Delay(300);
		
		// sine right, square left
		SDL_LockAudio();
		sharedVL.v[0].engaged = SDL_TRUE;
		sharedVL.v[0].ampR = 0;
		sharedVL.v[0].ampL = 1;
		sharedVL.v[1].ampR = 1;
		sharedVL.v[1].ampL = 0;
		SDL_UnlockAudio();
		
		SDL_Delay(300);
		
		// square right
		SDL_LockAudio();
		sharedVL.v[1].engaged = SDL_FALSE;
		sharedVL.v[0].ampR = 1;
		sharedVL.v[0].ampL = 0;
		SDL_UnlockAudio();
		
		SDL_Delay(300);
	}
	SDL_LockAudio();
	sharedVL.v[0].engaged = SDL_FALSE;
	sharedVL.v[2].engaged = SDL_TRUE; /* play explosion sound */
	SDL_UnlockAudio();
	
	SDL_Delay(3000);
	
	
	SDL_CloseAudio();
	
	freeVoices(&sharedVL);
	SDL_Quit();
	return 0;
}
