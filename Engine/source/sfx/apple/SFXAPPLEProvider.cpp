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

#include "sfx/sfxProvider.h"
#include "console/console.h"
#include "core/module.h"

#include "sfx/apple/sfxAPPLEDevice.h"

#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>

class SFXAPPLEProvider : public SFXProvider
{
public:
   SFXAPPLEProvider() : SFXProvider( "Apple" )
   {}
   
   virtual ~SFXAPPLEProvider() {}
   
protected:
   void init() override;
   String getDefaultDeviceName();
   
public:
   SFXDevice *createDevice(const String& deviceName, bool useHardware, S32 maxBuffers) override;
};

MODULE_BEGIN(SFXAPPLE)

   MODULE_INIT_BEFORE( SFX )
   MODULE_SHUTDOWN_AFTER( SFX )

   SFXAPPLEProvider* mProvider = NULL;
   
   MODULE_INIT
   {
      mProvider = new SFXAPPLEProvider;
   }

   MODULE_SHUTDOWN
   {
      delete mProvider;
   }

MODULE_END;

String SFXAPPLEProvider::getDefaultDeviceName(){
   AudioDeviceID devid = kAudioObjectUnknown;
   
   U32 size = sizeof(devid);
   AudioObjectPropertyAddress addr = {
      kAudioHardwarePropertyDefaultOutputDevice,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
   };
   
   OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, NULL, &size, &devid);
   if(status != noErr)
   {
      Con::errorf("SFXAppleProvider: Could not find default device!");
      return String::EmptyString;
   }
   
   CFStringRef devName = NULL;
   size = sizeof(devName);
   AudioObjectPropertyAddress nameAddress = {
      kAudioDevicePropertyDeviceNameCFString,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
   };
   
   status = AudioObjectGetPropertyData(devid, &nameAddress, 0, NULL, &size, &devName);
   if(status != noErr)
   {
      Con::errorf("SFXAppleProvider: Could not find default device!");
      return String::EmptyString;
   }
   
   char name[256];
   CFStringGetCString(devName, name, sizeof(name), kCFStringEncodingUTF8);
   
   return String::ToString(name);
}

void SFXAPPLEProvider::init()
{
   String deviceName = getDefaultDeviceName();
   
   SFXDeviceInfo* info = new SFXDeviceInfo;
   
   info->internalName = deviceName;
   info->name = deviceName;
   
   mDeviceInfo.push_back(info);
   
   regProvider(this);
}

SFXDevice *SFXAPPLEProvider::createDevice(const String &deviceName, bool useHardware, int maxBuffers)
{
   SFXDeviceInfo* info = _findDeviceInfo(deviceName);
   return new SFXAPPLEDevice(this, info->internalName, useHardware, maxBuffers);
}
