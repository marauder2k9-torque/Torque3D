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
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _DYNAMIC_CONSOLETYPES_H_
#include "console/dynamicTypes.h"
#endif


//-----------------------------------------------------------------------------
//    SFXDistanceModel.
//-----------------------------------------------------------------------------
enum SFXDistanceModel
{
   SFXDistanceModelLinear,                ///< Volume decreases linearly from min to max where it reaches zero.
   SFXDistanceModelLinearClamped,         ///< Volume decreases after min distance, then decresease to max distance based on rolloff factor.
   SFXDistanceModelInverse,               ///< Volume decreases in an inverse curve until it reaches zero.
   SFXDistanceModelInverseClamped,        ///< Volume decreases after min distance then decreases in an inverse curve to max based on rolloff.
   SFXDistanceModelExponent,              ///< exponential falloff for distance attenuation.
   SFXDistanceModelExponentClamped,       ///< exponential falloff after min for distance attenuation. 
};

DefineEnumType(SFXDistanceModel);

/// <summary>
/// Calculate the gain base on distance. These follow the methods pointed out in the
/// openal documentation.
/// </summary>
/// <param name="model">Which distance moedl are we using.</param>
/// <param name="minDistance">Min distance to start attenuation.</param>
/// <param name="maxDistance">Max distance of attenuation.</param>
/// <param name="distance">Source distance from the listener.</param>
/// <param name="rolloffFactor">Rolloff factor.</param>
/// <returns>Gain between 0.0 and 1.0 to be multiplied by the volume of the source.</returns>
inline F32 SFXDistanceAttenuation(SFXDistanceModel model, F32 minDistance, F32 maxDistance, F32 distance, F32 rolloffFactor)
{
   F32 gain = 1.0f;

   switch (model)
   {
   case SFXDistanceModelLinear:

      distance = getMin(distance, maxDistance);

      gain = (1 - (distance - minDistance) / (maxDistance - minDistance));
      break;

   case SFXDistanceModelLinearClamped:

      distance = getMax(distance, minDistance);
      distance = getMin(distance, maxDistance);

      gain = (1 - (distance - minDistance) / (maxDistance - minDistance));
      break;

   case SFXDistanceModelInverse:

      gain = minDistance / (minDistance + rolloffFactor * (distance - minDistance));
      break;

   case SFXDistanceModelInverseClamped:

      distance = getMax(distance, minDistance);
      distance = getMin(distance, maxDistance);

      gain = minDistance / (minDistance + rolloffFactor * (distance - minDistance));
      break;

      ///create exponential distance model    
   case SFXDistanceModelExponent:
      gain = pow((distance / minDistance), (-rolloffFactor));
      break;

   case SFXDistanceModelExponentClamped:
      distance = getMax(distance, minDistance);
      distance = getMin(distance, maxDistance);

      gain = pow((distance / minDistance), (-rolloffFactor));
      break;

   }

   return gain;
}

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

   // data needed by apis.
   Stream*  mStream;
   bool     mOwnStream;
   U32      mSize;
   U32      mFrequency;
   U16      mBitsPerSample;
   U8       mChannels;
   U8       mBlockAlign;

   SFXStream();
   SFXStream(const SFXStream& clone);


   virtual bool _parseFile() = 0;
   virtual void _close() = 0;

public:

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
   U32   getSize() const { return mSize; }
   U32   getFrequency() const { return mFrequency; }
   U16   getBitsPerSample() const { return mBitsPerSample; }
   U8    getChannels() const { return mChannels; }
   U8    getBlockAlign() const { return mBlockAlign; }

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
   
   /// <summary>
   /// Initialize the hardware on each API.
   /// </summary>
   virtual void init() = 0;

   /// <summary>
   /// Set the listener properties on the device. (pass on to each API)
   /// </summary>
   /// <param name="index">Index of the listener.</param>
   /// <param name="transform">Transform of the listener.</param>
   /// <param name="velocity">Velocity of the listener.</param>
   virtual void setListener(U32 index, MatrixF transform, Point3F velocity);

protected:
   String   mName;
   String   mAPIDeviceName;
   bool     mCaptureDevice;
   U32      mMaxSources;
   
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

   SourceVector      mSources;
   FreeSourceVector  mFreeSources;

   // stats
   U32 mLastSourceUpdateTime;
   S32 mStatNumSources;
   S32 mStatNumPlaying;
   S32 mStatNumCulled;
   S32 mStatSourceUpdateTime;
   S32 mStatParameterUpdateTime;

public:

   static void init();
   static void destroy();

};

#endif // !_SFXSYSTEM2_H_
