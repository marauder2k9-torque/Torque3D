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
#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif
#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif

//-----------------------------------------------------------------------------
// DEFINES
//-----------------------------------------------------------------------------

#define NUM_BUFFERS 4

//-----------------------------------------------------------------------------
//    SFXStatus.
//-----------------------------------------------------------------------------
enum SFXStatus
{
   SFXStatusNull,
   SFXStatusWaiting,
   SFXStatusPlaying,
   SFXStatusStopped,
   SFXStatusPaused,
};

DefineEnumType(SFXStatus);


inline const char* SFXStatusToString(SFXStatus status)
{
   switch (status)
   {
   case SFXStatusWaiting:
      return "waiting";
   case SFXStatusPlaying:
      return "playing";
   case SFXStatusStopped:
      return "stopped";
   case SFXStatusPaused:
      return "paused";
   case SFXStatusNull:
   default:;
   }

   return "null";
}

//-----------------------------------------------------------------------------
//    SFXReverb.
//-----------------------------------------------------------------------------


/// Reverb environment properties.
///
/// @note A given device may not implement all properties.
///restructure our reverbproperties to match openal

class SFXReverbProperties : public SimDataBlock
{
public:

   typedef SimDataBlock Parent;

protected:

   F32 flDensity;
   F32 flDiffusion;
   F32 flGain;
   F32 flGainHF;
   F32 flGainLF;
   F32 flDecayTime;
   F32 flDecayHFRatio;
   F32 flDecayLFRatio;
   F32 flReflectionsDelay;
   F32 flLateReverbDelay;
   F32 flEchoTime;
   F32 flEchoDepth;
   F32 flModulationTime;
   F32 flModulationDepth;
   F32 flAirAbsorptionGainHF;
   F32 flHFReference;
   F32 flLFReference;
   F32 flRoomRolloffFactor;
   S32 iDecayHFLimit;

public:

   SFXReverbProperties()
   {
      flDensity = 1.0f;
      flDiffusion = 1.0f;
      flGain = 0.4f;
      flGainHF = 0.7f;
      flGainLF = 0.0024f;
      flDecayTime = 1.0f;
      flDecayHFRatio = 1.0f;
      flDecayLFRatio = 1.0f;
      flReflectionsDelay = 0.1f;
      flLateReverbDelay = 0.1f;
      flEchoTime = 0.2f;
      flEchoDepth = 0.3f;
      flModulationTime = 2.0f;
      flModulationDepth = 0.3f;
      flAirAbsorptionGainHF = 0.9f;
      flHFReference = 1000.0f;
      flLFReference = 30.0f;
      flRoomRolloffFactor = 3.0f;
      iDecayHFLimit = 0;
   }

   void validate()
   {
      flDensity = mClampF(flDensity, 0.0f, 1.0f);
      flDiffusion = mClampF(flDiffusion, 0.0f, 1.0f);
      flGain = mClampF(flGain, 0.0f, 1.0f);
      flGainHF = mClampF(flGainHF, 0.0f, 1.0f);
      flGainLF = mClampF(flGainLF, 0.0f, 1.0f);
      flDecayTime = mClampF(flDecayTime, 0.1f, 20.0f);
      flDecayHFRatio = mClampF(flDecayHFRatio, 0.1f, 2.0f);
      flDecayLFRatio = mClampF(flDecayLFRatio, 0.1f, 2.0f);
      flReflectionsDelay = mClampF(flReflectionsDelay, 0.0f, 0.3f);
      flLateReverbDelay = mClampF(flLateReverbDelay, 0.0f, 0.1f);
      flEchoTime = mClampF(flEchoTime, 0.075f, 0.25f);
      flEchoDepth = mClampF(flEchoDepth, 0.0f, 1.0f);
      flModulationTime = mClampF(flModulationTime, 0.04f, 4.0f);
      flModulationDepth = mClampF(flModulationDepth, 0.0f, 1.0f);
      flAirAbsorptionGainHF = mClampF(flAirAbsorptionGainHF, 0.892f, 1.0f);
      flHFReference = mClampF(flHFReference, 1000.0f, 20000.0f);
      flLFReference = mClampF(flLFReference, 20.0f, 1000.0f);
      flRoomRolloffFactor = mClampF(flRoomRolloffFactor, 0.0f, 10.0f);
      iDecayHFLimit = mClampF(iDecayHFLimit, 0, 1);
   }

   /// SimDatablock

   DECLARE_CONOBJECT(SFXReverbProperties);
   DECLARE_CATEGORY("SFX");
   DECLARE_DESCRIPTION("Reverb Properties for an SFX Zone.");

   static void initPersistFields();

   virtual bool onAdd();
   virtual bool preload(bool server, String& errorStr);
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
   virtual void inspectPostApply();

};

//-----------------------------------------------------------------------------
//    SFXDistanceModel.
//-----------------------------------------------------------------------------
enum SFXDistanceModel
{
   SFXDistanceModelLinear,                /// Volume decreases linearly from min to max where it reaches zero.
   SFXDistanceModelLinearClamped,         /// Volume decreases after min distance, then decresease to max distance based on rolloff factor.
   SFXDistanceModelInverse,               /// Volume decreases in an inverse curve until it reaches zero.
   SFXDistanceModelInverseClamped,        /// Volume decreases after min distance then decreases in an inverse curve to max based on rolloff.
   SFXDistanceModelExponent,              /// exponential falloff for distance attenuation.
   SFXDistanceModelExponentClamped,       /// exponential falloff after min for distance attenuation.
};

DefineEnumType(SFXDistanceModel);

/// <summary>
/// Calculate the gain base on distance. These follow the methods
/// pointed out in the openal documentation.
/// </summary>
/// <param name="model"> Which distance moedl are we using. </param>
/// <param name="minDistance"> Min distance to start attenuation. </param>
/// <param name="maxDistance"> Max distance of attenuation. </param>
/// <param name="distance"> Source distance from the listener. </param>
/// <param name="rolloffFactor"> Rolloff factor. </param>
/// <returns> Gain between 0.0 and 1.0. </returns>
inline F32 SFXDistanceAttenuation(SFXDistanceModel model, F32 minDistance, F32 maxDistance, F32 distance, F32 rolloffFactor)
{
   F32 gain = 1.0f;

   switch (model)
   {
      // create the linear distance model.
      case SFXDistanceModelLinear:

         distance = getMin(distance, maxDistance);

         gain = (1 - (distance - minDistance) / (maxDistance - minDistance));
         break;

      // create the linear clamped distance model.
      case SFXDistanceModelLinearClamped:

         distance = getMax(distance, minDistance);
         distance = getMin(distance, maxDistance);

         gain = (1 - (distance - minDistance) / (maxDistance - minDistance));
         break;

      // create the inverse distance model.
      case SFXDistanceModelInverse:

         gain = minDistance / (minDistance + rolloffFactor * (distance - minDistance));
         break;

      // create the inverse clamped distance model.
      case SFXDistanceModelInverseClamped:

         distance = getMax(distance, minDistance);
         distance = getMin(distance, maxDistance);

         gain = minDistance / (minDistance + rolloffFactor * (distance - minDistance));
         break;

      // create exponential distance model.
      case SFXDistanceModelExponent:
         gain = pow((distance / minDistance), (-rolloffFactor));
         break;

      // create exponential clamped distance model.
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
class SFXDevice;

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
   String   mFileName;
   bool     mIsMusic;
   U32      mSize;
   U32      mFrequency;
   U16      mBitsPerSample;
   U8       mChannels;
   U8       mBlockAlign;

   SFXStream();
   SFXStream(bool isMusic);
   SFXStream(const SFXStream& clone);

   virtual bool _parseFile() = 0;
   virtual void _close() = 0;

public:
   Stream* mStream;
   bool    mOwnStream;

   static SFXStream* create(String fileName);
   static bool       exists(String fileName);

   // virtuals
   virtual ~SFXStream();
   virtual void   reset() = 0;
   virtual U32    read(U8* buffer, U32 length) = 0;
   virtual bool   isEOS() const = 0;

   bool open(Stream* stream, bool ownStream = false);
   void close();

   // Accessors that we may need.
   // Music streams will have no effects applied to their source.
   // The sound asset should pass in this value.
   bool   getIsMusic() const { return mIsMusic; }
   U32    getSize() const { return mSize; }
   U32    getFrequency() const { return mFrequency; }
   U16    getBitsPerSample() const { return mBitsPerSample; }
   U8     getChannels() const { return mChannels; }
   U8     getBlockAlign() const { return mBlockAlign; }
   String getFileName() const { return mFileName; }

};

typedef ThreadSafeRef< SFXStream > SFXStreamRef;

/// <summary
/// buffer holds sfxStream data.
/// </summary>
class SFXBuffer
{
public:
   SFXBuffer();
   virtual ~SFXBuffer();

   virtual U32 getCurrentOffset() { return -1; }

   virtual void freeBuffer();
};

/// <summary>
/// class to manage sources for each api. sfxSources are then routed
/// to each channel in the mixer, if they require effects then an aux
/// slot will be attached as well.
/// </summary>
class SFXSource
{
public:
   SFXSource(SFXDevice* device);
   virtual ~SFXSource();
   virtual bool Init(SFXStream* stream) = 0;
   virtual void update() = 0;

   virtual void _setStatus(SFXStatus status);
   virtual SFXStatus _getStatus() const;
   virtual void play() = 0;
   virtual void stop() = 0;
   virtual void pause() = 0;

protected:
   SFXDevice*  mDevice;
   SFXBuffer*  mBuffer;
   SFXStream*  mStream;
   SFXStatus   mStatus;
   bool        mReverb;

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
   virtual void findDevices() = 0;
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
   virtual bool init() = 0;

   /// <summary>
   /// Set the listener properties on the device. (pass on to each API)
   /// </summary>
   /// <param name="index">Index of the listener.</param>
   /// <param name="transform">Transform of the listener.</param>
   /// <param name="velocity">Velocity of the listener.</param>
   virtual void setListener(U32 index, MatrixF transform, Point3F velocity);

   /// <summary>
   /// Get the name for this device.
   /// </summary>
   /// <returns>String representing the name of this device.</returns>
   const String& getName() const { return mName; }

   /// <summary>
   /// is this a capture device?
   /// </summary>
   /// <returns>True if is a capture device.</returns>
   const bool getIsCapture() const { return mCaptureDevice; }

protected:
   SFXDevice(const String& name, const String& apiName, const bool sysDefault, const bool captureDevice);

   String   mName;
   String   mAPIDeviceName;
   bool     mCaptureDevice;
   bool     mDefaultDevice;
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

protected:
   
   // listeners zone reverb properties.
   SFXReverbProperties  mCurReverb;
   SFXReverbProperties  mDestReverb;

};

/// <summary>
/// this is where everything happens. We don't need access to other classes
/// we just need access to the system and then have it handle everything.
/// </summary>
class SFXSystem
{
public:
   typedef Vector< SFXSource* > SourceVector;
   typedef Vector< SFXBuffer* > BufferVector;
   typedef Vector< SFXDevice* > SystemDevices;
   typedef Vector< SFXStreamRef > Streams;

protected:
   static SFXSystem* smSingleton;
   SFXSystem();
   ~SFXSystem();

   // SFXSystems resources. System will loop through
   // these to update each cycle.
   SourceVector      mSources;
   BufferVector      mBufferList;
   SFXDevice*        mCurDevice;

   // These vectors do not need to be looped for obvious reasons.
   // @NOTE: These resources are filled when provider and a device is created.
   SourceVector      mFreeSources;

   // Device list filled out by the provider.
   SystemDevices     mDevicesList;

   // Input device list filled out by the provider.
   SystemDevices     mRecordDevicesList;

   // Vector to check to make sure we are not creating a stream of the same file.
   Streams           mCreatedStreams;

   // The current distance model.
   SFXDistanceModel  mDistanceModel;
   
   //---------------------------------------------

   // Stats
   U32 mLastSourceUpdateTime;
   S32 mStatNumSources;
   S32 mStatNumPlaying;
   S32 mStatNumCulled;
   S32 mStatSourceUpdateTime;

public:
   static SFXSystem* getSingleton() { return smSingleton; }

   static void init();
   static void destroy();
   void update();

   F32 distanceGain();

   // Initialize a device to be our current device.
   bool initDevice(const String& deviceName);
   void deinitDevice();

   // initialize sources and send back a reference.
   SFXSource* initSource(const ThreadSafeRef< SFXStream >& stream, const MatrixF* transform = NULL, const VectorF* velocity = NULL);

   // These will check to see if a stream with that filename exists and then return a threadsaferef to it.
   // Otherwise they will create a new one.
   SFXStreamRef createStream(String fileName, bool isMusic = false);

   // Set the distance model for gain attenuation.
   void setDistanceModel(SFXDistanceModel model);
   SFXDistanceModel getDistanceModel() const { return mDistanceModel; }

};

#define SFX SFXSystem::getSingleton()

#endif // !_SFXSYSTEM2_H_
