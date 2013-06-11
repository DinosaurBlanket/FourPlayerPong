/* 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "SDL/SDL.h"
#include "/home/jon/learningC/SDL_voice/SDL_voice.h"

#define Width 585
#define Height 585
#define PaddleThickness 10
#define PaddleLength 73
#define BallDiam 10
#define ExplosionLength 176
#define ExplosionThickness 70
#define ExplosionFrames 30
#define ExplosionInset 2
#define MinLoopTime 30




struct ball {
	SDL_Surface* surface;
	SDL_Rect destRect;
	signed int speedX;
	signed int speedY;
} theBall;

void faster(int* tooSlow, int inc)
{
	if (*tooSlow != 0)
		*tooSlow += *tooSlow < 0 ? -inc : inc;
}

typedef struct {
	SDL_Surface* surface;
	SDL_Rect destRect;
	Sint16* track;
	short hp;
} paddle;
paddle leftPaddle;
paddle rightPaddle;
paddle topPaddle;
paddle bottomPaddle;

struct explosionCounter {
	int count;
	char side;
	SDL_Rect srcRect;
	SDL_Rect destRect;
} theExplosionCounter;



int main(int argc, char* argv[])
{
	int i;
	
	int PlayerCount = 0;
	while (PlayerCount < 1 || PlayerCount > 4)
	{
		printf("how many players? 1, 2, 3, or 4?\n");
		scanf("%d", &PlayerCount);
	}
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	SDL_WM_SetCaption("QuadraPong", NULL);
	SDL_bool running = SDL_FALSE;
	int timeMark;
	
	
	voiceList sharedVL;
	buildVoiceList(&sharedVL, 2);
	
	buildVoice(&sharedVL.v[0], getDstLen(0));
	buildSineWave(&sharedVL.v[0]);
	//logVoice(&sharedVL.v[0], 'a');
	
	getVoiceFromFile(&sharedVL.v[1], "FourPlayerPong/explosionSound.wav", 1,1);
	sharedVL.v[1].repeat = SDL_FALSE;
	//logVoice(&sharedVL.v[1], 'b');
	
	
	SDL_AudioSpec as;
	as.freq = SpecFreq;
	as.format = AUDIO_S16SYS;
	as.channels = 2;
	as.samples = 1024;
	as.callback = fillAudioBuffer;
	as.userdata = &sharedVL;
	SDL_OpenAudio(&as, NULL);
	//logSpec(&as, 'a');
	
	short pauseAudioCountDown = 0;
	short paddleTone = 35;
	
	
	
	
	char fromArduino[14] = "356356356356";
	char oldFromArduino[14] = "356356356355";
	
	SDL_mutex* runningMutex = SDL_CreateMutex();
	SDL_mutex* fromArduinoMutex = SDL_CreateMutex();
	
	int getInput()
	{
		SDL_Event event;
		
		int fd;
		struct termios toptions;
		fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);
		if (fd < 0)
		{
			printf("Failed to connect to Arduino!\n");
			return 1;
		}
		//printf("fd opened as %i\n", fd);
		printf("waiting a few seconds for Arduino to restart...\n");
		usleep(3500000);
		tcgetattr(fd, &toptions);
		cfsetispeed(&toptions, B9600);
		cfsetospeed(&toptions, B9600);
		toptions.c_cflag &= ~PARENB;
		toptions.c_cflag &= ~CSTOPB;
		toptions.c_cflag &= ~CSIZE;
		toptions.c_cflag |= CS8;
		toptions.c_lflag |= ICANON;
		tcsetattr(fd, TCSANOW, &toptions);
		
		SDL_mutexP(runningMutex);
		running = SDL_TRUE;
		printf("running...\n");
		while (running)
		{
			SDL_mutexV(runningMutex);
			
			SDL_mutexP(fromArduinoMutex);
			write(fd, "0", 1);
			read(fd, fromArduino, 14);
			fromArduino[14] = 0;
			//printf("fromArduino: %s", fromArduino);
			SDL_mutexV(fromArduinoMutex);
			
			SDL_Delay(10);
			
			SDL_mutexP(runningMutex);
			while (SDL_PollEvent(&event))
			{
				switch (event.type) 
				{
					case SDL_QUIT:
						running = SDL_FALSE;
						break;
					case SDL_KEYDOWN:
						SDL_GetKeyName(event.key.keysym.sym);
						if (event.key.keysym.sym == SDLK_ESCAPE)
						{
							running = SDL_FALSE;
						}
						break;
				}
			}
		}
		SDL_mutexV(runningMutex);
		close(fd);
		return 0;
	}
	SDL_Thread* getInputThread = SDL_CreateThread(getInput, NULL);
	timeMark = SDL_GetTicks();
	
	
	SDL_Surface *screen = SDL_SetVideoMode(Width, Height, 32, SDL_SWSURFACE);
	
	
	Uint32 White    = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
	Uint32 OffWhite = SDL_MapRGB(screen->format, 0xBB, 0xBB, 0xBB);
	Uint32 Black    = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	Uint32 Colors[] = {
		SDL_MapRGB(screen->format, 0x80, 0x80, 0x80),/* Gray        */
		SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00),/* Red         */
		SDL_MapRGB(screen->format, 0xFF, 0x80, 0x00),/* RedYellow   */
		SDL_MapRGB(screen->format, 0xFF, 0xFF, 0x00),/* Yellow      */
		SDL_MapRGB(screen->format, 0x80, 0xFF, 0x00),/* YellowGreen */
		SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00),/* Green       */
		SDL_MapRGB(screen->format, 0x00, 0xFF, 0x80),/* GreenCyan   */
		SDL_MapRGB(screen->format, 0x00, 0xFF, 0xFF),/* Cyan        */
		SDL_MapRGB(screen->format, 0x00, 0x80, 0xFF),/* CyanBlue    */
		SDL_MapRGB(screen->format, 0x00, 0x00, 0xFF),/* Blue        */
		SDL_MapRGB(screen->format, 0x80, 0x00, 0xFF),/* BlueMagenta */
		SDL_MapRGB(screen->format, 0xFF, 0x00, 0xFF),/* Magenta     */
		SDL_MapRGB(screen->format, 0xFF, 0x00, 0x80) /* MagentaRed  */
	};
	Uint32 colorKey = SDL_MapRGB(screen->format, 17, 227, 7);
	
	
	SDL_Surface *leftExplosion   = SDL_LoadBMP("FourPlayerPong/explosionLeft.bmp"  );
	SDL_Surface *rightExplosion  = SDL_LoadBMP("FourPlayerPong/explosionRight.bmp" );
	SDL_Surface *topExplosion    = SDL_LoadBMP("FourPlayerPong/explosionTop.bmp"   );
	SDL_Surface *bottomExplosion = SDL_LoadBMP("FourPlayerPong/explosionBottom.bmp");
	SDL_SetColorKey(leftExplosion  , SDL_SRCCOLORKEY | SDL_RLEACCEL, colorKey);
	SDL_SetColorKey(rightExplosion , SDL_SRCCOLORKEY | SDL_RLEACCEL, colorKey);
	SDL_SetColorKey(topExplosion   , SDL_SRCCOLORKEY | SDL_RLEACCEL, colorKey);
	SDL_SetColorKey(bottomExplosion, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorKey);
	
	
	theBall.surface = SDL_CreateRGBSurface(
		SDL_SWSURFACE, 
		BallDiam, 
		BallDiam, 
		32, 
		0,0,0,0
	);
	SDL_FillRect(theBall.surface, NULL, White);
	void resetBall()
	{
		int sx = 1 + rand() / (RAND_MAX/3 + 1);
		int sy = 1 + rand() / (RAND_MAX/3 + 1);
		if ( rand() / (RAND_MAX/2 + 1) )
			sx *= -1;
		if ( rand() / (RAND_MAX/2 + 1) )
			sy *= -1;
		theBall.speedX = sx;
		theBall.speedY = sy;
		
		theBall.destRect.x = Width/2 - BallDiam/2;
		theBall.destRect.y = Height/2 - BallDiam/2;
		paddleTone = 35;
	}
	resetBall();
	
	
	leftPaddle.surface = SDL_CreateRGBSurface(
		SDL_SWSURFACE, 
		PaddleThickness, 
		PaddleLength,
		32, 
		0,0,0,0
	);
	rightPaddle.surface = SDL_CreateRGBSurface(
		SDL_SWSURFACE, 
		PaddleThickness, 
		PaddleLength, 
		32, 
		0,0,0,0
	);
	topPaddle.surface = SDL_CreateRGBSurface(
		SDL_SWSURFACE, 
		PaddleLength, 
		PaddleThickness, 
		32, 
		0,0,0,0
	);
	bottomPaddle.surface = SDL_CreateRGBSurface(
		SDL_SWSURFACE, 
		PaddleLength, 
		PaddleThickness, 
		32, 
		0,0,0,0
	);
	
	leftPaddle  .destRect.x = 0;
	rightPaddle .destRect.x = Width - PaddleThickness;
	topPaddle   .destRect.y = 0;
	bottomPaddle.destRect.y = Height - PaddleThickness;
	
	leftPaddle  .track = (Sint16*) &leftPaddle  .destRect.y;
	rightPaddle .track = (Sint16*) &rightPaddle .destRect.y;
	topPaddle   .track = (Sint16*) &topPaddle   .destRect.x;
	bottomPaddle.track = (Sint16*) &bottomPaddle.destRect.x;
	
	paddle *paddles[] = {
		&leftPaddle,
		&rightPaddle, 
		&topPaddle, 
		&bottomPaddle
	};
	
	
	for (i=0; i<4; i++)
	{
		if (i < PlayerCount)
			paddles[i]->hp = 9;/* player paddle */
		else
			paddles[i]->hp = 0;/* dummy paddle */
		SDL_FillRect( paddles[i]->surface, NULL, Colors[ paddles[i]->hp ] );
	}
	
	void paddleSound()
	{
		paddleTone++;
		SDL_LockAudio();
		sharedVL.v[0].dstLen = getDstLen(paddleTone);
		sharedVL.v[0].engaged = SDL_TRUE;
		SDL_UnlockAudio();
		pauseAudioCountDown = 10;
		SDL_FillRect(theBall.surface, NULL, OffWhite);
	}
	
	
	SDL_Delay( 3600 - (SDL_GetTicks()-timeMark) );/* wait for arduino... */
	
	SDL_PauseAudio(0);
	
	SDL_mutexP(runningMutex);
	while (running)
	{
		SDL_mutexV(runningMutex);
		timeMark = SDL_GetTicks();
		
		
		SDL_mutexP(fromArduinoMutex);
		if (fromArduino != oldFromArduino)
		{
			for (i=0; i<4; i++)
			{
				if (paddles[i]->hp > 0)
					*paddles[i]->track = 
						+ (fromArduino[i*3    ]-'0')*100 
						+ (fromArduino[i*3 + 1]-'0')*10 
						+ (fromArduino[i*3 + 2]-'0') 
						- 100
					;
				else
				{
					if (i<2)
						*paddles[i]->track = theBall.destRect.y - PaddleLength/2 + BallDiam/2;
					else
						*paddles[i]->track = theBall.destRect.x - PaddleLength/2 + BallDiam/2;
				}
			strcpy(oldFromArduino, fromArduino);
			}
		}
		SDL_mutexV(fromArduinoMutex);
		
		
		
		if (/* ball hits left paddle */
			theBall.destRect.x  <=  PaddleThickness  &&
			theBall.destRect.y  <=  leftPaddle.destRect.y + PaddleLength  &&
			theBall.destRect.y + BallDiam  >=  leftPaddle.destRect.y
		){
			theBall.speedX *= -1;
			faster(&theBall.speedX, 1);
			theBall.destRect.x = PaddleThickness + 1;
			if (theBall.destRect.y + BallDiam  <  leftPaddle.destRect.y + PaddleLength/3)
				theBall.speedY -= 1;
			else if (theBall.destRect.y  >  leftPaddle.destRect.y + PaddleLength-(PaddleLength/3))
				theBall.speedY += 1;
			paddleSound();
		}
		else if (/* ball hits right paddle */
			theBall.destRect.x  >=  Width - PaddleThickness - BallDiam  &&
			theBall.destRect.y  <=  rightPaddle.destRect.y + PaddleLength  &&
			theBall.destRect.y + BallDiam  >=  rightPaddle.destRect.y
		){
			theBall.speedX *= -1;
			faster(&theBall.speedX, 1);
			theBall.destRect.x = Width - PaddleThickness - BallDiam - 1;
			if (theBall.destRect.y + BallDiam  <  rightPaddle.destRect.y + PaddleLength/3)
				theBall.speedY -= 1;
			else if (theBall.destRect.y  >  rightPaddle.destRect.y + PaddleLength-(PaddleLength/3))
				theBall.speedY += 1;
			paddleSound();
		}
		else if (/* ball hits top paddle */
			theBall.destRect.x + BallDiam  >=  topPaddle.destRect.x  &&
			theBall.destRect.x  <=  topPaddle.destRect.x + PaddleLength  &&
			theBall.destRect.y  <=  PaddleThickness
		){
			theBall.speedY *= -1;
			faster(&theBall.speedY, 1);
			theBall.destRect.y = PaddleThickness + 1;
			if (theBall.destRect.x + BallDiam  <  topPaddle.destRect.x + PaddleLength/3)
				theBall.speedX -= 1;
			else if (theBall.destRect.x  >  topPaddle.destRect.x + PaddleLength-(PaddleLength/3))
				theBall.speedX += 1;
			paddleSound();
		}
		else if (/* ball hits bottom paddle */
			theBall.destRect.x + BallDiam  >=  bottomPaddle.destRect.x  &&
			theBall.destRect.x  <=  bottomPaddle.destRect.x + PaddleLength  &&
			theBall.destRect.y + BallDiam  >=  Height - PaddleThickness
		){
			theBall.speedY *= -1;
			faster(&theBall.speedY, 1);
			theBall.destRect.y = Height - PaddleThickness - BallDiam - 1;
			if (theBall.destRect.x + BallDiam  <  bottomPaddle.destRect.x + PaddleLength/3)
				theBall.speedX -= 1;
			else if (theBall.destRect.x  >  bottomPaddle.destRect.x + PaddleLength-(PaddleLength/3))
				theBall.speedX += 1;
			paddleSound();
		}
		else
		{
			if (theBall.destRect.x  <=  0)/* ball hits left goal */
			{
				if (0 < leftPaddle.hp)
				{
					leftPaddle.hp--;
					SDL_FillRect(leftPaddle.surface, NULL, Colors[leftPaddle.hp]);
				}
				theExplosionCounter.side = 'l';
				theExplosionCounter.count = ExplosionFrames;
				theExplosionCounter.destRect.x = -ExplosionInset;
				theExplosionCounter.destRect.y = (Sint16)
					+ theBall.destRect.y
					+ BallDiam/2
					- ExplosionThickness/2
				;
				theExplosionCounter.srcRect.w = ExplosionLength;
				theExplosionCounter.srcRect.h = ExplosionThickness;
				theExplosionCounter.srcRect.x = 0;
				
				SDL_LockAudio();
				sharedVL.v[1].dstPos = 0;
				sharedVL.v[1].engaged = SDL_TRUE;
				SDL_UnlockAudio();
				
				resetBall();
			}
			else if (theBall.destRect.x  >=  Width - BallDiam)/* ball hits right goal */
			{
				if (0 < rightPaddle.hp)
				{
					rightPaddle.hp--;
					SDL_FillRect(rightPaddle.surface, NULL, Colors[rightPaddle.hp]);
				}
				theExplosionCounter.side = 'r';
				theExplosionCounter.count = ExplosionFrames;
				theExplosionCounter.destRect.x = Width - ExplosionLength + ExplosionInset;
				theExplosionCounter.destRect.y = 
					+ theBall.destRect.y
					+ BallDiam/2
					- ExplosionThickness/2
				;
				theExplosionCounter.srcRect.w = ExplosionLength;
				theExplosionCounter.srcRect.h = ExplosionThickness;
				theExplosionCounter.srcRect.x = 0;
				
				SDL_LockAudio();
				sharedVL.v[1].dstPos = 0;
				sharedVL.v[1].engaged = SDL_TRUE;
				SDL_UnlockAudio();
				
				resetBall();
			}
			else if (theBall.destRect.y  <=  0)/* ball hits top goal */
			{
				if (0 < topPaddle.hp)
				{
					topPaddle.hp--;
					SDL_FillRect(topPaddle.surface, NULL, Colors[topPaddle.hp]);
				}
				theExplosionCounter.side = 't';
				theExplosionCounter.count = ExplosionFrames;
				theExplosionCounter.destRect.y = -ExplosionInset;
				theExplosionCounter.destRect.x = 
					+ theBall.destRect.x
					+ BallDiam/2
					- ExplosionThickness/2
				;
				theExplosionCounter.srcRect.w = ExplosionThickness;
				theExplosionCounter.srcRect.h = ExplosionLength;
				theExplosionCounter.srcRect.y = 0;
				
				SDL_LockAudio();
				sharedVL.v[1].dstPos = 0;
				sharedVL.v[1].engaged = SDL_TRUE;
				SDL_UnlockAudio();
				
				resetBall();
			}
			else if (theBall.destRect.y  >=  Height - BallDiam)/* ball hits bottom goal */
			{
				if (0 < bottomPaddle.hp)
				{
					bottomPaddle.hp--;
					SDL_FillRect(bottomPaddle.surface, NULL, Colors[bottomPaddle.hp]);
				}
				theExplosionCounter.side = 'b';
				theExplosionCounter.count = ExplosionFrames;
				theExplosionCounter.destRect.y = Height - ExplosionLength + ExplosionInset;
				theExplosionCounter.destRect.x = 
					+ theBall.destRect.x
					+ BallDiam/2
					- ExplosionThickness/2
				;
				theExplosionCounter.srcRect.w = ExplosionThickness;
				theExplosionCounter.srcRect.h = ExplosionLength;
				theExplosionCounter.srcRect.y = 0;
				
				SDL_LockAudio();
				sharedVL.v[1].dstPos = 0;
				sharedVL.v[1].engaged = SDL_TRUE;
				SDL_UnlockAudio();
				
				resetBall();
			}
		}
		
		
		if (pauseAudioCountDown)
		{
			pauseAudioCountDown--;
			if (!pauseAudioCountDown)
			{
				SDL_LockAudio();
				sharedVL.v[0].engaged = SDL_FALSE;
				SDL_UnlockAudio();
				SDL_FillRect(theBall.surface, NULL, White);
			}
		}
		
		
		SDL_FillRect(screen, NULL, Black);
		for (i=0; i<4; i++)
			SDL_BlitSurface(
				paddles[i]->surface, NULL, 
				screen, &paddles[i]->destRect
			);
		if (theExplosionCounter.count)
		{
			switch (theExplosionCounter.side)
			{
				case 'l':
					theExplosionCounter.srcRect.y = (ExplosionFrames - theExplosionCounter.count)*70;
					SDL_BlitSurface(
						leftExplosion, &theExplosionCounter.srcRect, 
						screen, &theExplosionCounter.destRect
					);
					break;
				case 'r':
					theExplosionCounter.srcRect.y = (ExplosionFrames - theExplosionCounter.count)*70;
					SDL_BlitSurface(
						rightExplosion, &theExplosionCounter.srcRect, 
						screen, &theExplosionCounter.destRect
					);
					break;
				case 't':
					theExplosionCounter.srcRect.x = (ExplosionFrames - theExplosionCounter.count)*70;
					SDL_BlitSurface(
						topExplosion, &theExplosionCounter.srcRect, 
						screen, &theExplosionCounter.destRect
					);
					break;
				case 'b':
					theExplosionCounter.srcRect.x = (ExplosionFrames - theExplosionCounter.count)*70;
					SDL_BlitSurface(
						bottomExplosion, &theExplosionCounter.srcRect, 
						screen, &theExplosionCounter.destRect
					);
					break;
			}
			theExplosionCounter.count--;
		}
		else
		{
			SDL_BlitSurface(
				theBall.surface, NULL, 
				screen, &theBall.destRect
			);
			theBall.destRect.x += theBall.speedX;
			theBall.destRect.y += theBall.speedY;
		}
		
		
		SDL_Flip(screen);
		
		//printf("%4d ms\n", SDL_GetTicks()-timeMark);
		SDL_Delay( MinLoopTime - (SDL_GetTicks()-timeMark) );
		//printf("\t%4d ms\n", SDL_GetTicks()-timeMark);
		SDL_mutexP(runningMutex);
	}
	SDL_mutexV(runningMutex);
	
	printf("stopping...\n");
	
	SDL_CloseAudio();
	freeVoices(&sharedVL);
	
	for (i=0; i<4; i++)
		SDL_FreeSurface(paddles[i]->surface);
	SDL_FreeSurface(theBall.surface);
	
	SDL_FreeSurface(leftExplosion  );
	SDL_FreeSurface(rightExplosion );
	SDL_FreeSurface(topExplosion   );
	SDL_FreeSurface(bottomExplosion);
	
	SDL_Quit();
	
	return 0;
}


/*  A R D U I N O   S K E T C H  */
/*
const int Apotpin = A6;
const int Bpotpin = A4;
const int Cpotpin = A2;
const int Dpotpin = A0;

String Apot;
String Bpot;
String Cpot;
String Dpot;

String output;


void setup()
{
	Serial.begin(9600);
}

void loop()
{
	Apot = String( (analogRead(Apotpin)>>1) + 100 );
	Bpot = String( (analogRead(Bpotpin)>>1) + 100 );
	Cpot = String( (analogRead(Cpotpin)>>1) + 100 );
	Dpot = String( (analogRead(Dpotpin)>>1) + 100 );
	
	output = Apot + Bpot + Cpot + Dpot;
	
	if (Serial.available() > 0)
	{
		//Serial.read();
		Serial.println(output);
	}
	
	delay(10);
}
*/