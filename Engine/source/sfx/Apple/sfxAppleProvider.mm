//
//  sfxAppleProvider.m
//  TestProject
//
//  Created by ctrlintelligence on 28/08/2022.
//

#import <Foundation/Foundation.h>
#include "core/module.h"
#include "sfx/sfxProvider.h"
#include "sfxAppleDevice.h"

MODULE_BEGIN( SFXApple )
   MODULE_INIT_BEFORE(SFX)
   MODULE_SHUTDOWN_AFTER(SFX)

   SFXAppleProvider* mProvider;

   MODULE_INIT
   {
      mProvider = new SFXAppleProvider;
   }

   MODULE_SHUTDOWN
   {
      delete mProvider;
   }

MODULE_END;

void SFXAppleProvider::init()
{
   AppleDeviceInfo* info = new AppleDeviceInfo;
   info->driver = "Apple";
   info->internalName = "Apple AVAudioEngine Device";
   info->name = "Apple Device";
   mDeviceInfo.push_back(info);
   regProvider(this);
}

SFXAppleProvider::~SFXAppleProvider()
{
   
}

SFXDevice* SFXAppleProvider::createDevice(const String& deviceName, bool useHardware, S32 maxBuffers)
{
   AppleDeviceInfo *info = dynamic_cast<AppleDeviceInfo*>(_findDeviceInfo(deviceName));
   
   if(info)
      return new SFXAppleDevice(this, info->internalName, useHardware, maxBuffers);
   
   return NULL;
}
