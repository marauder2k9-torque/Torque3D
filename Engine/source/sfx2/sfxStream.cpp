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

SFXStream::ExtensionsVector SFXStream::smExtensions(__FILE__, __LINE__);
SFXStream::CreateFnsVector SFXStream::smCreateFns(__FILE__, __LINE__);

SFXStream::SFXStream()
   :  mStream( NULL ),
      mOwnStream(false),
      mSamples(0),
      mSamplesPerSec(0),
      mBytesPerSample(0),
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

   mOwnStream        = true;
   mSamples          = clone.mSamples;
   mSamplesPerSec    = clone.mSamplesPerSec;
   mBytesPerSample   = clone.mBytesPerSample;
   mBitsPerSample    = clone.mBitsPerSample;
   mChannels         = clone.mChannels;
   mBlockAlign       = clone.mBlockAlign;

}

void SFXStream::registerExtension(String ext, SFXSTREAM_CREATE_FN create_fn)
{
   // Register the stream creation first.
   smExtensions.push_back(ext);
   smCreateFns.push_back(create_fn);
}

void SFXStream::unregisterExtension(String ext)
{
   for (ExtensionsVector::iterator iter = smExtensions.begin();
      iter != smExtensions.end(); ++iter)
      if ((*iter).equal(ext, String::NoCase))
      {
         smExtensions.erase(iter);
         return;
      }
}

SFXStream* SFXStream::create(String fileName)
{

   if (exists(fileName))
   {
      String noExtension = Platform::stripExtension(fileName, smExtensions);

      SFXStream* sfxStream = NULL;

      for (U32 i = 0; i < smExtensions.size(); i++)
      {
         String testName = noExtension + smExtensions[i];

         Stream* stream = FileStream::createAndOpen(testName, Torque::FS::File::Read);
         if (!stream)
            continue;

         // Note that the creation function swallows up the 
         // resource stream and will take care of deleting it.
         sfxStream = smCreateFns[i](stream);
         if (sfxStream)
            return sfxStream;
      }
   }

   return NULL;
}

bool SFXStream::exists(String filename)
{
   // First strip off our current extension (validating 
   // against a list of known extensions so that we don't
   // strip off the last part of a file name with a dot in it.

   String noExtension = Platform::stripExtension(filename, smExtensions);

   for (U32 i = 0; i < smExtensions.size(); i++)
   {
      String testName = noExtension + smExtensions[i];
      if (Torque::FS::IsFile(testName))
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

   mSamples = 0;
}
