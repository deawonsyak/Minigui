/////////////////////////////////////////////////////////////
// Flash Plugin and Player
// Copyright (C) 1998,1999 Olivier Debon
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// 
///////////////////////////////////////////////////////////////
//  Author : Olivier Debon  <odebon@club-internet.fr>
//

#include "swf.h"

#ifdef _NOUNIX_
#define NOSOUND
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifndef NOSOUND
#include <linux/soundcard.h>
#endif
#endif/*NOUNIX*/

#ifdef RCSID
static char *rcsid = "$Id: sound.cc,v 1.6 2006-02-17 05:52:20 jpzhang Exp $";
#endif

#define PRINT 0

extern pthread_mutex_t count_mutex;
int tt=0;// sound device can just be set once!
int dealingmp=0;
Sound::Sound(long id) : Character(SoundType, id)
{
	samples = 0;
	stereo = 0;
	soundRate = 0;
	sampleSize = 8;
}

Sound::~Sound()
{
	if (samples) {
		delete samples;
	}
}

void
Sound::setSoundFlags(long _compression, long _soundRate, long _stereo, long _sampleSize)
{
	compression = _compression;	
	stereo = _stereo;
	if(_sampleSize) sampleSize = 16;
	else sampleSize = 8;

	switch (_soundRate) {
        case 0:
            soundRate = 5500;
            break;
        case 1:
            soundRate = 11000;
            break;
        case 2:
            soundRate = 22000;
            break;
        case 3:
            soundRate = 44000;
            break;
    }

#if PRINT
	char* ppszCompression[3] = {"uncompressed", "ADPCM", "MP3"};
	printf("Sound   %s ", ppszCompression[compression]);
	printf("Rate = %d Hz  ", soundRate);
	printf("SampleSize = %d byte(s) ", sampleSize);
	if (stereo)	printf("Stereo  ");
	else 		printf("Mono  ");
	printf("\n");
#endif
}

char *
Sound::setNbSamples(long n) {
	long size;
	nbSamples = n;

	size = nbSamples * (stereo ? 2 : 1) * sampleSize;
	samples = new char[size];
	memset((char *)samples,0, size);
	return samples;
}

long
Sound::getCompression() {
	return compression;
}

long
Sound::getRate() {
	return soundRate;
}

long
Sound::getChannel() {
	return stereo ? 2 : 1;
}

long
Sound::getNbSamples() {
	return nbSamples;
}

long
Sound::getSampleSize() {
	return sampleSize;
}

char *
Sound::getSamples() {
	return samples;
}




//////////// SOUND MIXER


SoundMixer::SoundMixer(char *device)
{
	sampleSize = 0;
	stereo = 0;
	rate = 0;
	list = NULL;
	player = new Rawplayer;
	player->initialize(device);
	player->setvolume(60);
	InitMP3(&mp);	
    //printf("a soundmixer is created!&&&&&&&&&&&&\n");
}

SoundMixer::~SoundMixer()
{
	SoundList *sl,*del;
printf("delete SoundMixer\n");
	if(player) delete player;
//printf("a soundmixer is destroyed !!!!!!!!!!!!\n");	
#ifndef _NOUNIX_
    if(dealingmp==1)
        usleep(100000);
#endif
	ExitMP3(&mp);	
	for(sl = list; sl; ) {
		del = sl;
		sl = sl->next;
		if(del->current) delete del->current;
		delete del;
	}
	list = 0;
}

void
SoundMixer::stopSounds()
{
	SoundList *sl,*del;

	for(sl = list; sl; ) {
		del = sl;
		sl = sl->next;
		if(del->current) delete del->current;
		delete del;
	}
	list = 0;
}

void *playsound(void *arg)
{
	char *buffer;
	int size;
	int blockSize;
	SoundList *sl;
	Rawplayer   *rplayer;
	
	sl = (SoundList *)arg;
	
	if(sl)
	{	
		if(sl->player->setsoundtype(sl->stereo, sl->sampleSize , sl->rate) == false) return NULL;
		size = sl->nbSamples * (sl->stereo ? 2 : 1) * (sl->sampleSize / 8);
		blockSize = sl->player->getblocksize();
		
		int curpoint = 0;	
		buffer = sl->current;
          //  printf("ct %d\n",size);
		
		while (curpoint <= size)
		{
        	sl->player->putblock(&buffer[curpoint], blockSize);
			curpoint += blockSize;
		}
	
		sl = sl->next;
	}
	return NULL;
}

void
SoundMixer::startSound(Sound *sound)
{
	SoundList *sl;
    char *temp;
    int i;
	if (sound) {
        sl = new SoundList;
        sl->compression = sound->getCompression();
        sl->rate = sound->getRate();
        sl->stereo = (sound->getChannel() == 2);
        sl->sampleSize = sound->getSampleSize();
        //sl->current = sound->getSamples();
        sl->nbSamples = sound->getNbSamples();
		i = sl->nbSamples * (sl->stereo ? 2 : 1) * (sl->sampleSize / 8);
    //    printf("temp %d\n",i);
#if 1
        temp = sound->getSamples();
        sl->current =(char *)malloc(i);
        memcpy(sl->current,temp,i);
#endif     
        sl->remaining = sound->getSampleSize()*sound->getNbSamples()*sound->getChannel();
        sl->next = list;
        sl->player = player;
		list = sl;
		
		rate 	   = sl->rate;
		stereo     = sl->stereo;
		sampleSize = sl->sampleSize;
		
		pthread_create(&thread, 0, playsound, (void *)list);	
	}
	
}

long
SoundMixer::playSounds(SoundStreamBlock* sndblock)
{
	pthread_mutex_lock(&count_mutex);
	int  i_compression = sndblock->compression;
    int  i_rate = sndblock->rate;
    int  i_sampleSize = sndblock->sampleSize;
    int  i_stereo = sndblock->stereo;
    int  i_datasize = sndblock->datasize;
   // char *  i_data = sndblock->data;	
    char *i_data =(char *) malloc(i_datasize);	
	int  rawlength;
	struct timeval tm3, tm4;
    dealingmp=1;
    if(!sndblock->data)
        return 0;
    memcpy(i_data,sndblock->data,i_datasize); 
	pthread_mutex_unlock(&count_mutex);

	switch (i_compression)
    {
        case 0:
        {
            printf("uncompressed samples\n");
        	player->putblock(i_data, i_datasize);
			return i_datasize;	   	 
			break;
        }
        case 1:
        {
            printf("Adpcm  samples\n");
           break;
        }

        case 2:
        {
			int size,ret;
		    char out[8192];
		    int total = 0;
			
			if(i_datasize > 4)
			{ 
				int  nsample = (unsigned char)i_data[0] + (unsigned char)i_data[1] * 256;
				//int delay   = (unsigned char)i_data[2] + (unsigned char)i_data[3] * 256;
			
				if(nsample > 0)	rawlength = nsample * (i_stereo + 1) * (i_sampleSize / 8) ;
				else
					return -1;
                if(tt==1)
                {
                    //printf("the sound device is set! %d~~~~~~\n",tt);
                   tt=0;
				if (rate != i_rate || sampleSize != i_sampleSize || stereo != i_stereo) ; 
				{
					if(player->setsoundtype(i_stereo, i_sampleSize , i_rate) ) 
					{
						 rate = i_rate;//
						 sampleSize = i_sampleSize;//
						 stereo = i_stereo;
					}
				}
                }
			    ret = decodeMP3(&mp, &i_data[4], i_datasize-4, out, 8192, &size);
		    	while(ret == MP3_OK) 
				{
    	        	player->putblock((char *)out, size);

				    total += size;  
        			ret = decodeMP3(&mp,NULL,0,out,8192,&size);
		  	 	}
                free(i_data);
                dealingmp=0;
				return total;
			}
	 	}   // case 2	
	}   // switch 	
    free(i_data);
   dealingmp=0;
	return 0;
}

