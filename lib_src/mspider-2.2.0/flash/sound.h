/////////////////////////////////////////////////////////////
// Flash Plugin and Player
// Copyright (C) 1998 Olivier Debon
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
#ifndef _SOUND_H_
#define _SOUND_H_

#define GET_SOUND_RATE_CODE(f) (((f)&0x0c)>>2)
#include "mgdconfig.h"
#include "rawplayer.h"
#include <pthread.h>

class Sound : public Character {
	long		 compression;		//  "uncompressed", "ADPCM", "MP3"
	long		 soundRate;			// 
	long		 stereo;			// True if stereo sound
	long		 sampleSize;		// 1 or 2 bytes

	char		*samples;			// Array of samples
	long		 nbSamples;

public:
	Sound(long id);
	~Sound();
	void    	 setSoundFlags(long _compression, long _soundRate, long _stereo, long _sampleSize);
	char		*setNbSamples(long n);

	long		 getCompression();
	long		 getRate();
	long		 getChannel();
	long		 getNbSamples();
	long		 getSampleSize();
	char		*getSamples();
};

//*******************************//
//define SoundStream Struct   //
//*******************************//

struct SoundStreamBlock {
	int	 compression;
	int	 rate;
	int	 sampleSize;
	int	 stereo;
	int  datasize;
	char *data;
};

struct SoundList {
	long	 compression;	
	long	 rate;
	long	 stereo;
	long	 sampleSize;
	long	 nbSamples;
	long	 remaining;
	char	*current;

	Rawplayer   *player;
	SoundList *next;
};


class SoundMixer {
public:
	struct mpstr mp;
	SoundList	*list;
	Rawplayer   *player;
	int         rate;
	int         stereo;
	int         sampleSize;
	pthread_t   thread;
	SoundMixer(char*);
	~SoundMixer();

	void		 startSound(Sound *sound);	// Register a sound to be played
	void		 stopSounds();		// Stop every current sounds in the instance

	long		 playSounds(SoundStreamBlock *sndstream);		//hz add play sound stream 
};

#endif /* _SOUND_H_ */
