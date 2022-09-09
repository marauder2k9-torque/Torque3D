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
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "core/module.h"
#include "sfx2/openal/sfxALSystem.h"

#ifdef TORQUE_OS_WIN
#include <windows.h>
HINSTANCE hOpenALDLL = NULL;
#endif // TORQUE_OS_WIN

#if defined(__linux__) && !defined(TORQUE_OS_LINUX)
#define TORQUE_OS_LINUX
#endif
#ifdef TORQUE_OS_LINUX
#include <dlfcn.h>
#include <err.h>
#include <string.h>
void* hOpenALDLL = NULL;
#endif // TORQUE_OS_LINUX


MODULE_BEGIN(OpenAL)

   MODULE_INIT_BEFORE(SFX2)
   MODULE_SHUTDOWN_AFTER(SFX2)

   SFXALProvider* mProvider;

   MODULE_INIT
   {
      mProvider = new SFXALProvider;
   }

   MODULE_SHUTDOWN
   {
      delete mProvider;
   }

MODULE_END;

void SFXALProvider::init()
{
   if (_loadApi() != AL_TRUE)
   {
      Con::printf("SFXALProvider - OpenAL not available.");
      return;
   }

   // Cool, loop through them, and caps em
   const char* deviceFormat = "OpenAL v%d.%d %s";

   regProvider(this);
}

void SFXALProvider::findDevices()
{
   const char* devices;
   const char* defaultDeviceName;
   // find output devies first.
   if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
      devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
      defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
   }
   else
   {
      devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
      defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
   }
}

bool SFXALProvider::_loadApi()
{
   U32 err = 0;
#ifdef TORQUE_OS_WIN
   hOpenALDLL = LoadLibraryA("openal32.dll");
   #define LOAD_PROC_PLATFORM(x)                                              \
      x = reinterpret_cast<decltype(x)>(reinterpret_cast<void*>(              \
      GetProcAddress(hOpenALDLL, #x)));                                       \
      if (!x)                                                                 \
      {                                                                       \
         Con::printf("OpenAL Load: Failed to find entry point for %s", #x);   \
         err = 1;                                                             \
      }                                                                       
#endif

#ifdef TORQUE_OS_LINUX
   hOpenALDLL = dlopen("libopenal.so.1", RTLD_NOW);
#define LOAD_PROC_PLATFORM(x)                                                 \
      x = reinterpret_cast<decltype(x)>(reinterpret_cast<void*>(              \
      dlsym(hOpenALDLL, #x)));                                                \
      if (!x)                                                                 \
      {                                                                       \
         Con::printf("OpenAL Load: Failed to find entry point for %s", #x);   \
         err = 1;                                                             \
      }   
#endif

   if (!hOpenALDLL) {
      Con::errorf("Failed to load OpenAL Library. OpenAL provider not available!");
      return AL_FALSE;
   }


#define LOAD_PROC(x) do {                                                     \
    LOAD_PROC_PLATFORM(x)                                                     \
} while(0)
   LOAD_PROC(alcCreateContext);
   LOAD_PROC(alcMakeContextCurrent);
   LOAD_PROC(alcProcessContext);
   LOAD_PROC(alcSuspendContext);
   LOAD_PROC(alcDestroyContext);
   LOAD_PROC(alcGetCurrentContext);
   LOAD_PROC(alcGetContextsDevice);
   LOAD_PROC(alcOpenDevice);
   LOAD_PROC(alcCloseDevice);
   LOAD_PROC(alcGetError);
   LOAD_PROC(alcIsExtensionPresent);
   LOAD_PROC(alcGetProcAddress);
   LOAD_PROC(alcGetEnumValue);
   LOAD_PROC(alcGetString);
   LOAD_PROC(alcGetIntegerv);
   LOAD_PROC(alcCaptureOpenDevice);
   LOAD_PROC(alcCaptureCloseDevice); 
   LOAD_PROC(alcCaptureStart);
   LOAD_PROC(alcCaptureStop);
   LOAD_PROC(alcCaptureSamples);

   LOAD_PROC(alEnable);
   LOAD_PROC(alDisable);
   LOAD_PROC(alIsEnabled);
   LOAD_PROC(alGetString);
   LOAD_PROC(alGetBooleanv);
   LOAD_PROC(alGetIntegerv);
   LOAD_PROC(alGetFloatv);
   LOAD_PROC(alGetDoublev);
   LOAD_PROC(alGetBoolean);
   LOAD_PROC(alGetInteger);
   LOAD_PROC(alGetFloat);
   LOAD_PROC(alGetDouble);
   LOAD_PROC(alGetError);
   LOAD_PROC(alIsExtensionPresent);
   LOAD_PROC(alGetProcAddress);
   LOAD_PROC(alGetEnumValue);
   LOAD_PROC(alListenerf);
   LOAD_PROC(alListener3f);
   LOAD_PROC(alListenerfv);
   LOAD_PROC(alListeneri);
   LOAD_PROC(alListener3i);
   LOAD_PROC(alListeneriv);
   LOAD_PROC(alGetListenerf);
   LOAD_PROC(alGetListener3f);
   LOAD_PROC(alGetListenerfv);
   LOAD_PROC(alGetListeneri);
   LOAD_PROC(alGetListener3i);
   LOAD_PROC(alGetListeneriv);
   LOAD_PROC(alGenSources);
   LOAD_PROC(alDeleteSources);
   LOAD_PROC(alIsSource);
   LOAD_PROC(alSourcef);
   LOAD_PROC(alSource3f);
   LOAD_PROC(alSourcefv);
   LOAD_PROC(alSourcei);
   LOAD_PROC(alSource3i);
   LOAD_PROC(alSourceiv);
   LOAD_PROC(alGetSourcef);
   LOAD_PROC(alGetSource3f);
   LOAD_PROC(alGetSourcefv);
   LOAD_PROC(alGetSourcei);
   LOAD_PROC(alGetSource3i);
   LOAD_PROC(alGetSourceiv);
   LOAD_PROC(alSourcePlayv);
   LOAD_PROC(alSourceStopv);
   LOAD_PROC(alSourceRewindv);
   LOAD_PROC(alSourcePausev);
   LOAD_PROC(alSourcePlay);
   LOAD_PROC(alSourceStop);
   LOAD_PROC(alSourceRewind);
   LOAD_PROC(alSourcePause);
   LOAD_PROC(alSourceQueueBuffers);
   LOAD_PROC(alSourceUnqueueBuffers);
   LOAD_PROC(alGenBuffers);
   LOAD_PROC(alDeleteBuffers);
   LOAD_PROC(alIsBuffer);
   LOAD_PROC(alBufferf);
   LOAD_PROC(alBuffer3f);
   LOAD_PROC(alBufferfv);
   LOAD_PROC(alBufferi);
   LOAD_PROC(alBuffer3i);
   LOAD_PROC(alBufferiv);
   LOAD_PROC(alGetBufferf);
   LOAD_PROC(alGetBuffer3f);
   LOAD_PROC(alGetBufferfv);
   LOAD_PROC(alGetBufferi);
   LOAD_PROC(alGetBuffer3i);
   LOAD_PROC(alGetBufferiv);
   LOAD_PROC(alBufferData);
   LOAD_PROC(alDopplerFactor);
   LOAD_PROC(alDopplerVelocity);
   LOAD_PROC(alSpeedOfSound);
   LOAD_PROC(alDistanceModel);
   if (!err)
   {
      ALCint alc_ver[2] = { 0, 0 };
      alcGetIntegerv(nullptr, ALC_MAJOR_VERSION, 1, &alc_ver[0]);
      alcGetIntegerv(nullptr, ALC_MINOR_VERSION, 1, &alc_ver[1]);
      if (alcGetError(nullptr) == ALC_NO_ERROR)
         ALCVer = MAKE_ALC_VER(alc_ver[0], alc_ver[1]);
      else
      {
         Con::printf("Failed to query ALC, assuming 1.0");
         ALCVer = MAKE_ALC_VER(1, 0);
      }
#undef LOAD_PROC
#define LOAD_PROC(x) do {                                                        \
         x = reinterpret_cast<decltype(x)>(                                      \
               alcGetProcAddress(nullptr, #x));                                  \
         if(!x)                                                                  \
         {                                                                       \
            Con::printf("OpenAL Load: Failed to find entry point for %s",#x);    \
            err = 1;                                                             \
         }                                                                       \
   } while(0)
      if (alcIsExtensionPresent(nullptr, "ALC_EXT_thread_local_context"))
      {
         LOAD_PROC(alcSetThreadContext);
         LOAD_PROC(alcGetThreadContext);
      }
   }

   if (err)
   {
      return AL_FALSE;
   }
#undef LOAD_PROC

   return AL_TRUE;
}
