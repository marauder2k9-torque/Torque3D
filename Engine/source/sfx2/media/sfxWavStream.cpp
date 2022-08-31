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
#include "sfx2/media/sfxWavStream.h"
#include "core/stream/stream.h"
#include "core/strings/stringFunctions.h"
#include "core/stream/fileStream.h"
#include "console/console.h"
#include "core/util/safeDelete.h"

typedef struct
{
   U8    szRIFF[4];
   U32   ulRIFFSize;
   U8    szWAVE[4];
} WAVEFILEHEADER;

typedef struct
{
   U8    szChunkName[4];
   U32   ulChunkSize;
} RIFFCHUNK;

typedef struct
{
   U16   usFormatTag;
   U16   usChannels;
   U32   ulSamplesPerSec;
   U32   ulAvgBytesPerSec;
   U16   usBlockAlign;
   U16   usBitsPerSample;
   U16   usSize;
   U16   usReserved;
   U32   ulChannelMask;
   GUID  guidSubFormat;
} WAVEFMT;

SFXWavStream::SFXWavStream()
   :mDataStart(-1)
{
   dMemset(&mWaveIDs, 0, sizeof(mWaveIDs));
}

SFXWavStream::~SFXWavStream()
{
   _close();
}

void SFXWavStream::_close()
{
   if (!mStream)
      return;

   if (mOwnStream)
      SAFE_DELETE(mStream);

   mDataStart = -1;
   mSamples = 0;
}

bool SFXWavStream::_parseFile()
{
   mDataStart = -1;

   WAVEFILEHEADER fileHdr;
   RIFFCHUNK      riffChunk;
   WAVEFMT        waveFmt;

   mStream->read(sizeof(WAVEFILEHEADER), &fileHdr);

   mStream->read(sizeof(RIFFCHUNK), &riffChunk);

}
