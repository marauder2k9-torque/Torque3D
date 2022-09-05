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

#include "sfx2/sfxSystem.h"
#include "core/stream/fileStream.h"
#include "console/console.h"
#include "core/util/safeDelete.h"
#include "sfx2/media/sfxWavStream.h"

SFXStream::SFXStream()
   :  mStream( NULL ),
      mFileName(String::EmptyString),
      mOwnStream(false),
      mIsMusic(false),
      mSize(0),
      mFrequency(0),
      mBitsPerSample(0),
      mChannels(0),
      mBlockAlign(0)
{
}

SFXStream::SFXStream(bool isMusic)
   :  mStream(NULL),
      mFileName(String::EmptyString),
      mOwnStream(false),
      mIsMusic(isMusic),
      mSize(0),
      mFrequency(0),
      mBitsPerSample(0),
      mChannels(0),
      mBlockAlign(0)
{
}

SFXStream::SFXStream(const SFXStream& clone)
{
   mStream = clone.mStream->clone();
   if (!mStream)
   {
      Con::errorf("SFXFileStream::SFXFileStream() - Failed to clone source stream");
      return;
   }

   mFileName = clone.mFileName;
   mOwnStream        = true;
   mIsMusic          = clone.mIsMusic;
   mSize             = clone.mSize;
   mFrequency        = clone.mFrequency;
   mBitsPerSample    = clone.mBitsPerSample;
   mChannels         = clone.mChannels;
   mBlockAlign       = clone.mBlockAlign;

}

SFXStream* SFXStream::create(String fileName)
{

   if (exists(fileName))
   {
      String ext;

      S32 dotPos = fileName.find('.', 0, String::Right);

      if (dotPos != String::NPos)
         ext = fileName.substr(dotPos);

      if (ext.isNotEmpty())
      {
         SFXStream* sfxStream = NULL;

         Stream* stream = FileStream::createAndOpen(fileName, Torque::FS::File::Read);
         if (!stream)
         {
            Con::printf("SFXStream - Could not create file stream for %s", fileName);
            return NULL;
         }

         // are we wav?
         if (ext.equal(".wav", String::NoCase))
         {
            sfxStream = SFXWavStream::create(stream);
            if (sfxStream)
               return sfxStream;
         }
         // are we ogg?
         if (ext.equal(".ogg", String::NoCase))
         {

            Stream* stream = FileStream::createAndOpen(fileName, Torque::FS::File::Read);
            if (!stream)
               return NULL;

            sfxStream = SFXWavStream::create(stream);
            if (sfxStream)
               return sfxStream;
         }

         // we have reached here, we must be an unknown format?
         Con::printf("SFXWavStream - Unhandled format!");
         SAFE_DELETE(stream);
      }
   }

   return NULL;
}

bool SFXStream::exists(String fileName)
{
   // First strip off our current extension (validating 
   // against a list of known extensions so that we don't
   // strip off the last part of a file name with a dot in it.

   String ext;

   S32 dotPos = fileName.find('.', 0, String::Right);

   if (dotPos != String::NPos)
      ext = fileName.substr(dotPos);

   if (ext.equal(".wav", String::NoCase))
   {
      if (Torque::FS::IsFile(fileName))
         return true;
   }
   else if (ext.equal(".ogg", String::NoCase))
   {
      if (Torque::FS::IsFile(fileName))
         return true;
   }

   return false;
}

bool SFXStream::open(Stream* stream, bool ownStream)
{
   AssertFatal(stream, "SFXStream::open() - NULL Stream!");
   close();

   mStream = stream;
   mOwnStream = ownStream;

   if (_parseFile())
   {
      reset();
      return true;
   }
   else
      return false;

}

void SFXStream::close()
{
   if (!mStream)
      return;

   // Let the overloaded class cleanup.
   _close();

   // We only close it if we own it.
   if (mOwnStream)
      SAFE_DELETE(mStream);

   mSize = 0;
}
