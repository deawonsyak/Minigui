#include <stdio.h>
#include <sys/types.h>


/****************/
/* Sound Errors */
/****************/
// General error
#define SOUND_ERROR_OK                0
#define SOUND_ERROR_FINISH           -1

// Device error (for player)
#define SOUND_ERROR_DEVOPENFAIL       1
#define SOUND_ERROR_DEVBUSY           2
#define SOUND_ERROR_DEVBADBUFFERSIZE  3
#define SOUND_ERROR_DEVCTRLERROR      4

#define MODE_MONO   0
#define MODE_STEREO 1


/**************************/
/* Define values for MPEG */
/**************************/
#define SCALEBLOCK     12
#define CALCBUFFERSIZE 512
#define MAXSUBBAND     32
#define MAXCHANNEL     2
#define MAXTABLE       2
#define SCALE          32768
#define MAXSCALE       (SCALE-1)
#define MINSCALE       (-SCALE)
#define RAWDATASIZE    (2*2*32*SSLIMIT)

#define LS 0
#define RS 1

#define SSLIMIT      18
#define SBLIMIT      32

#define WINDOWSIZE    4096

// Huffmancode
#define HTN 34



// Class for playing raw data
class Rawplayer 
{
public:
  Rawplayer() {__errorcode=SOUND_ERROR_OK;};
  ~Rawplayer();

  bool initialize(char *filename);
  void abort(void);
  int  getprocessed(void);

  bool setsoundtype(int stereo,int samplesize,int speed);
  bool resetsoundtype(void);

  bool putblock(void *buffer,int size);

  int  getblocksize(void);

  static char *defaultdevice;
  static int  setvolume(int volume);

  int geterrorcode(void) {return __errorcode;};

protected:
  bool seterrorcode(int errorno) {__errorcode=errorno; return false;};

private:
  int  __errorcode;
  short int rawbuffer[RAWDATASIZE];
  int  rawbuffersize;
  int  audiohandle,audiobuffersize;
  int  rawstereo,rawsamplesize,rawspeed;
  bool forcetomono,forceto8;
};
