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

#include "platform/platform.h"
#include "sfx2/sfxSystem.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "T3D/gameBase/processList.h"
#include "platform/profiler.h"
#include "platform/platformTimer.h"
#include "core/util/autoPtr.h"
#include "core/module.h"


MODULE_BEGIN(SFX)

   MODULE_INIT_BEFORE(Sim)
   MODULE_SHUTDOWN_BEFORE(Sim) // Make sure all SimObjects disappear in time.

   MODULE_INIT
   {
      SFXSystem::init();
   }

   MODULE_SHUTDOWN
   {
      SFXSystem::destroy();
   }

MODULE_END;

SFXSystem* SFXSystem::smSingleton = NULL;

ImplementEnumType(SFXStatus,
   "Playback status of sound source.\n"
   "@ingroup SFX")
   {
      SFXStatusWaiting, "Waiting",
      "The source is currently waiting for a call to play."
   },
   {
      SFXStatusPlaying, "Playing",
      "The source is currently playing."
   },
   {
      SFXStatusStopped, "Stopped",
      "Playback of the source is stopped.  When transitioning to Playing state, playback will start at the beginning "
         "of the source."
   },
   {
      SFXStatusPaused, "Paused",
      "Playback of the source is paused.  Resuming playback will play from the current playback position."
   },
EndImplementEnumType;

ImplementEnumType(SFXDistanceModel,
   "Type of volume distance attenuation curve.\n"
   "The distance model determines the falloff curve applied to the volume of 3D sounds over distance.\n\n"
   "@ref SFXSource_volume\n\n"
   "@ref SFX_3d\n\n"
   "@ingroup SFX")
   {
      SFXDistanceModelLinear, "Linear",
      "Volume attenuates linearly."
   },
   {
      SFXDistanceModelLinearClamped, "Linear Clamped",
      "Volume attenuates linearly from the reference distance."
   },
   {
      SFXDistanceModelInverse, "Inverse",
      "Volume attenuates in an inverse curve, attenuating by the rolloff factor. "
   },
   {
      SFXDistanceModelInverseClamped, "Inverse Clamped",
      "Volume attenuates in an inverse curve, attenuating by the rolloff factor from reference distance. "
   },
   {
      SFXDistanceModelExponent, "Exponential",
      "Volume attenuates exponentially by the rolloff factor. "
   },
   {
      SFXDistanceModelExponentClamped, "Exponential Clamped",
      "Volume attenuates exponentially starting from the reference distance, attenuating by the rolloff factor. "
   },
EndImplementEnumType;

SFXSystem::SFXSystem()
   :  mCurDevice(NULL),
      mLastSourceUpdateTime(0),
      mStatNumSources(0),
      mStatNumPlaying(0),
      mStatNumCulled(0),
      mDistanceModel(SFXDistanceModelInverseClamped)
{
   VECTOR_SET_ASSOCIATION(mSources);
   VECTOR_SET_ASSOCIATION(mFreeSources);
   VECTOR_SET_ASSOCIATION(mBufferList);
   VECTOR_SET_ASSOCIATION(mDevicesList);
   VECTOR_SET_ASSOCIATION(mRecordDevicesList);
}

//-----------------------------------------------------------------------------

void SFXSystem::init()
{
   AssertWarn(smSingleton == NULL, "SFX has already been initialized!");

   SFXProvider::initializeAllProviders();

   // Create the system.
   smSingleton = new SFXSystem();

}

void SFXSystem::destroy()
{
   AssertWarn(smSingleton != NULL, "SFX has not been initialized!");

   delete smSingleton;
   smSingleton = NULL;
}

void SFXSystem::deinitDevice()
{
   if (!mCurDevice)
      return;

   for (SourceVector::iterator iter = mSources.begin(); iter != mSources.end();)
   {
      SFXSource* source = *iter;
      source->stop();
   }

   // clear the sources vector.
   mSources.clear();

   // clear the free sources vector, this is filled when a new device is created.
   mFreeSources.clear();

   // deinit device here.
   mCurDevice = NULL;
}

//-----------------------------------------------------------------------------

SFXStreamRef SFXSystem::createStream(String fileName, bool isMusic)
{
   for (Streams::iterator iter = mCreatedStreams.begin(); iter != mCreatedStreams.end();)
   {
      SFXStreamRef stream = *iter;
      if (stream->getFileName() == fileName)
         return stream;
   }

   return nullptr;
}

void SFXSystem::setDistanceModel(SFXDistanceModel model)
{
   const bool changed = (model != mDistanceModel);

   mDistanceModel = model;
}
