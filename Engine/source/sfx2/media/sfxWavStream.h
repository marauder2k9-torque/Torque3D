//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _SFXWAVSTREAM_H_
#define _SFXWAVSTREAM_H_

#ifndef _SFXSYSTEM2_H_
#include "sfx2/sfxSystem.h"
#endif // !_SFXSYSTEM2_H_

#include "core/util/safeDelete.h"

class Stream;

#define MAX_NUM_WAVEID			1024

enum WAVEFILETYPE
{
   WF_EX = 1,
   WF_EXT = 2
};

typedef struct _GUID {
   U32   Data1;
   U16   Data2;
   U16   Data3;
   U8    Data4[8];
} GUID;

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct
{
   U16      wFormatTag;
   U16      nChannels;
   U32      nSamplesPerSec;
   U32      nAvgBytesPerSec;
   U16      nBlockAlign;
   U16      wBitsPerSample;
   U16      cbSize;
} WAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */

#ifndef _WAVEFORMATEXTENSIBLE_
#define _WAVEFORMATEXTENSIBLE_
typedef struct {
   WAVEFORMATEX   Format;
   union {
      U16 wValidBitsPerSample;       /* bits of precision  */
      U16 wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
      U16 wReserved;                 /* If neither applies, set to zero. */
   } Samples;
   U32             dwChannelMask;    /* which channels are */
                                     /* present in stream  */
   GUID            SubFormat;
} WAVEFORMATEXTENSIBLE;
#endif // !_WAVEFORMATEXTENSIBLE_

typedef struct {
   U16    wFormatTag;        /* format type */
   U16    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
   U32   nSamplesPerSec;    /* sample rate */
   U32   nAvgBytesPerSec;   /* for buffer estimation */
   U16    nBlockAlign;       /* block size of data */
} WAVEFORMAT;

typedef struct {
   WAVEFORMAT  wf;
   U16         wBitsPerSample;
} PCMWAVEFORMAT;

typedef struct
{
   WAVEFILETYPE         wfType;
   WAVEFORMATEXTENSIBLE wfEXT;		// For non-WAVEFORMATEXTENSIBLE wavefiles, the header is stored in the Format member of wfEXT
   U8*                  pData;
   U32                  ulDataSize;
   U32                  ulDataOffset;
} WAVEFILEINFO;

typedef U32	WAVEID;

class SFXWavStream : public SFXStream,
                     public IPositionable<U32>
{
public:

   typedef SFXStream Parent;

protected:

   U32 mDataStart;
   virtual bool _parseFile();
   virtual void _close();

private:

   WAVEFILEINFO* mWaveIDs[MAX_NUM_WAVEID];

public:

   static SFXWavStream* create(Stream* stream);
   SFXWavStream();
   SFXWavStream(const SFXWavStream& cloneFrom);

   /// Destructor.
   virtual ~SFXWavStream();

   // SFXStream
   virtual void reset();
   virtual U32 read(U8* buffer, U32 length);
   virtual SFXStream* clone() const
   {
      SFXWavStream* stream = new SFXWavStream(*this);
      if (!stream->mStream)
         SAFE_DELETE(stream);
      return stream;
   }

   // IPositionable
   virtual U32 getPosition() const;
   virtual void setPosition(U32 offset);
   bool isEOS() const;
};

#endif
