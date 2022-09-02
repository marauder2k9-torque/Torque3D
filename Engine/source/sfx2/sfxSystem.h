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

/// <summary>
/// class to bring in all audio files this should be routed through the sfxSystem in
/// to a sfxBuffer. We could have the sfxBuffer do this also but each file type has
/// its own way to manage where blocks are aligned and how to split the buffer into
/// pieces.
/// </summary>
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
/// <summary
/// buffer holds sfxStream data in a list of buffers. Should define
/// if buffer is music or otherwise, music shouldn't have any effects applied.
/// </summary>
class SFXBuffer
{

};

/// <summary>
/// class to manage sources for each api. sfxSources are then routed
/// to each channel in the mixer, if they require effects then an aux
/// slot will be attached as well.
/// </summary>
class SFXSource
{

};

/// <summary>
/// class to hold all audio device providers.
/// </summary>
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

/// <summary>
/// this is as simple as it gets for a device class. It shouldn't
/// hold much more than the device name and whether or not it is
/// a capture device. API's take care of the rest. Pointer for the device
/// then gets sent around each apis buffer and source classes.
/// </summary>
class SFXDevice
{
public:
   // default deconstructor.
   virtual ~SFXDevice();
   
   // initalise the hardware device.
   virtual void init() = 0;
protected:
   String   mName;
   String   mDeviceName;
   bool     mCaptureDevice;
   
};

/// <summary>
/// where to send each source. Also sets up all channels. If a source
/// has effects then the mixer will send the source to an aux slot with the
/// effect loaded. How many aux slots is dependent on api.
/// </summary>
class SFXMixer
{

};

/// <summary>
/// class for setting all the effects parameters. Can add a lot more here.
/// </summary>
class SFXEffectManager
{
public:
   virtual void SetReverb();
};

/// <summary>
/// this is where everything happens. We don't need access to other classes
/// we just need access to the system and then have it handle everything.
/// </summary>
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
