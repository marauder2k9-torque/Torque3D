//
//  sfxAppleCAProvider.cpp
//  TestProject
//
//  Created by ctrlintelligence on 01/09/2022.
//

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
   regProvider(this);
}
