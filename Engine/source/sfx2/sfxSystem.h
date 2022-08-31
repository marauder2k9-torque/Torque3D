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

#ifndef _SFXSYSTEM2_H_
#define _SFXSYSTEM2_H_

#ifndef _TSIGNAL_H_
#include "core/util/tSignal.h"
#endif
#ifndef _REFBASE_H_
#include "core/util/refBase.h"
#endif
#ifndef _TSTREAM_H_
#include "core/stream/tStream.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif
#ifndef _THREADSAFEREFCOUNT_H_
#include "platform/threads/threadSafeRefCount.h"
#endif
#ifndef _TSTREAM_H_
#include "core/stream/tStream.h"
#endif

/// <summary>
/// SFXSystem.h holds everything we need for the rest of the engine to use sound.
/// SFXStream includes the functionality of SFXStream and SFXFileStream - really no need for separates.
/// </summary>

class Stream;
class SFXStream;

typedef SFXStream* (*SFXSTREAM_CREATE_FN)(Stream* stream);
typedef ThreadSafeRef< SFXStream > SFXStreamRef;

class SFXStream : public ThreadSafeRefCount< SFXStream >,
                  public IInputStream< U8 >,
                  public IResettable
{
protected:
   typedef Vector< String > ExtensionsVector;
   typedef Vector< SFXSTREAM_CREATE_FN > CreateFnsVector;

   static ExtensionsVector smExtensions;
   static CreateFnsVector smCreateFns;

   // data needed by apis.
   Stream*  mStream;
   bool     mOwnStream;
   U32      mSamples;
   U32      mSamplesPerSec;
   U8       mBytesPerSample;
   U8       mBitsPerSample;
   U8       mChannels;
   U8       mBlockAlign;

   SFXStream();
   SFXStream(const SFXStream& clone);


   virtual bool _parseFile() = 0;
   virtual void _close() = 0;

public:

   static void registerExtension(String ext, SFXSTREAM_CREATE_FN create_fn);
   static void unregisterExtension(String ext);

   static SFXStream* create(String fileName);
   static bool exists(String fileName);

   // virtuals
   virtual ~SFXStream();
   virtual void reset() = 0;
   virtual U32 read(U8* buffer, U32 length) = 0;
   virtual bool isEOS() const = 0;

   bool open(Stream* stream, bool ownStream = false);
   void close();

   // accessors that we may need.
   U32 getSampleCount() const { return mSamples; }
   U32 getDataLength() const { return mSamples * mBytesPerSample; }
   U32 getDuration() const { return ((U64)mSamples *(U64)1000) / (U64)mSamplesPerSec; }
   U8 getChannels() const { return mChannels; }
   U8 getBitsPerSample() const { return mBitsPerSample; }
   U8 getBitsPerChannel() const { return mBitsPerSample / mChannels; }
   U8 getBytesPerSample() const { return mBytesPerSample; }

};

class SFXBuffer
{

};

class SFXSource
{

};

class SFXProvider
{
protected:
   // register this provider.
   static void regProvider(SFXProvider* provider);

   // initialize provider - override by child class.
   virtual void init() = 0;

   // provide a name for provider.
   SFXProvider(const String& name);

   //deconstruct.
   ~SFXProvider();

public:

   // initalizeAllProviders.
   static void initializeAllProviders();
};

class SFXDevice
{

};

class SFXMixer
{

};

class SFXEffectManager
{

};

class SFXSystem
{
public:
   typedef Vector< SFXSource* > SourceVector;
   typedef Vector< SFXSource* > FreeSourceVector;

protected:
   static SFXSystem* smSingleton;
   SFXSystem();
   ~SFXSystem();

public:

   static void init();

   static void destroy();

};


#endif // !_SFXSYSTEM2_H_
