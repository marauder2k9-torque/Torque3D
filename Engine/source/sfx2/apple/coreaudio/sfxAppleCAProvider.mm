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
#include "sfxAppleCASystem.h"

#include "core/string/stringFunctions.h"
#include "console/console.h"
#include "core/module.h"

MODULE_BEGIN(CoreAudio)
   MODULE_INIT_BEFORE(SFX2)
   MODULE_SHUTDOWN_AFTER(SFX2)
   
   SFXCAProvider *mProvider;
   
   MODULE_INIT
   {
      mProvider = new SFXCAProvider;
   }

   MODULE_SHUTDOWN
   {
      delete mProvider;
   }

MODULE_END

void SFXCAProvider::init()
{
   // this should return the number of output audio devices.
   UInt32 dataSize = 0;
   AudioObjectPropertyAddress propertyAddress;
   propertyAddress.mSelector = kAudioHardwarePropertyDevices;
   propertyAddress.mScope    = kAudioObjectPropertyScopeOutput; // for input kAudioObjectPropertyScopeInput
   propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
   OSStatus result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);
   
   int count = -1;
   if (result == noErr) {
      count = dataSize / sizeof(AudioDeviceID);
   }
   
   regProvider(this);
}

void SFXCAProvider::findDevices()
{
   // find our output devices first
   UInt32 dataSize = 0;
   AudioObjectPropertyAddress propertyAddress;
   propertyAddress.mSelector = kAudioHardwarePropertyDevices;
   propertyAddress.mScope    = kAudioObjectPropertyScopeOutput; // for input kAudioObjectPropertyScopeInput
   propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
   OSStatus result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);

   int count = -1;
   if (result == noErr) {
      count = dataSize / sizeof(AudioDeviceID);
   }

   AudioDeviceID *audioDevices = dMalloc(dataSize);

   result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize, audioDevices);
   if(kAudioHardwareNoError != result)
   {
      Con::printf("SFXCAProvider - failed to finde system devices!");
      free(audioDevices), audioDevices = NULL;
      return;
   }
   
   for(U32 i = 0; i < count; i++)
   {
      audioDevices[i];
   }
   
   // now do input devices
   AudioObjectPropertyAddress propertyAddress;
   propertyAddress.mSelector = kAudioHardwarePropertyDevices;
   propertyAddress.mScope    = kAudioObjectPropertyScopeInput; // for input kAudioObjectPropertyScopeInput
   propertyAddress.mElement  = kAudioObjectPropertyElementMaster;
   OSStatus result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);
   
   int count = -1;
   if (result == noErr) {
      count = dataSize / sizeof(AudioDeviceID);
   }
   
   for(U32 i = 0; i < count; i++)
   {
      
   }
}
