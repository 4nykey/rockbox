/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Stepan Moskovchenko
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#define FRACTSIZE 10

#define BUF_SIZE 256
#define NBUF   2


#ifndef SIMULATOR

#if (HW_SAMPR_CAPS & SAMPR_CAP_22)
#define SAMPLE_RATE SAMPR_22  // 44100 22050 11025
#else
#define SAMPLE_RATE SAMPR_44  // 44100 22050 11025
#endif

#define MAX_VOICES 23 // Note: 24 midi channels is the minimum general midi
                        // spec implementation

#else	// Simulator requires 44100, and we can afford to use more voices

#define SAMPLE_RATE SAMPR_44
#define MAX_VOICES 48

#endif

#define BYTE unsigned char

//Data chunk ID types, returned by readID()
#define ID_UNKNOWN  -1
#define ID_MTHD     1
#define ID_MTRK     2
#define ID_EOF      3
#define ID_RIFF     4

//MIDI Commands
#define MIDI_NOTE_OFF   128
#define MIDI_NOTE_ON    144
#define MIDI_AFTERTOUCH 160
#define MIDI_CONTROL    176
#define MIDI_PRGM   192
#define MIDI_PITCHW 224

//MIDI Controllers
#define CTRL_VOLUME 7
#define CTRL_BALANCE    8
#define CTRL_PANNING    10
#define CHANNEL     1

//Most of these are deprecated.. rampdown is used, maybe one other one too
#define STATE_ATTACK        1
#define STATE_DECAY         2
#define STATE_SUSTAIN       3
#define STATE_RELEASE       4
#define STATE_RAMPDOWN      5

//Loop states
#define STATE_LOOPING           7
#define STATE_NONLOOPING    8

//Various bits in the GUS mode byte
#define LOOP_ENABLED    4
#define LOOP_PINGPONG   8
#define LOOP_REVERSE    16

#define LOOPDIR_FORWARD 0
#define LOOPDIR_REVERSE 1

struct MIDIfile
{
    int  Length;
    unsigned short numTracks;
    unsigned short div; /* Time division, X ticks per millisecond */
    struct Track * tracks[48];
    unsigned char patches[128];
    int numPatches;
};

/*
struct SynthObject
{
    struct GWaveform * wf;
    unsigned int delta;
    unsigned int decay;
    unsigned int cp;
    unsigned char state, loopState, loopDir;
    unsigned char note, vol, ch, isUsed;
    int curRate, curOffset, targetOffset;
    unsigned int curPoint;
};
*/

struct SynthObject
{
    struct GWaveform * wf;
    int delta;
    int decay;
    unsigned int cp; /* unsigned int */
    int state, loopState, loopDir;
    int note, vol, ch, isUsed;
    int curRate, curOffset, targetOffset;
    int curPoint;
    signed short int volscale;
};

struct Event
{
    unsigned int delta;
    unsigned char status, d1, d2;
    unsigned int len;
    unsigned char * evData;
};

struct Track
{
    unsigned int size;
    unsigned int numEvents;
    unsigned int delta; /* For sequencing */
    unsigned int pos;   /* For sequencing */
    void * dataBlock;
};

int printf(const char *fmt, ...);
unsigned char readChar(int file);
int readTwoBytes(int file);
int readFourBytes(int file);
int readVarData(int file);
int eof(int fd);
unsigned char * readData(int file, int len);
void exit(int code);

#define malloc(n) my_malloc(n)
void * my_malloc(int size);

extern struct SynthObject voices[MAX_VOICES];

extern int chVol[16];       /* Channel volume                */
extern int chPanLeft[16];   /* Channel panning               */
extern int chPanRight[16];
extern int chPat[16];       /* Channel patch                 */
extern int chPW[16];        /* Channel pitch wheel, MSB only */

extern struct GPatch * gusload(char *);
extern struct GPatch * patchSet[128];
extern struct GPatch * drumSet[128];

extern struct MIDIfile * mf;

extern int numberOfSamples;
extern long bpm;

