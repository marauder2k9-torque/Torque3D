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


MODULE_BEGIN(SFX2)

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
