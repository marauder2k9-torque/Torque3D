/*
 * Copyright (c) 2006, Creative Labs Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 * 	     the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * 	     and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of Creative Labs Inc. nor the names of its contributors may be used to endorse or
 * 	     promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>
#include "sfx/openal/LoadOAL.h"

HINSTANCE g_hOpenALDLL = NULL;

ALboolean LoadOAL10Library(char *szOALFullPathName, LPOPENALFNTABLE lpOALFnTable)
{

   if (!lpOALFnTable)
      return AL_FALSE;

   if (szOALFullPathName)
      g_hOpenALDLL = LoadLibraryA(szOALFullPathName);
   else
      g_hOpenALDLL = LoadLibraryA("openal32.dll");

   if (!g_hOpenALDLL)
      return AL_FALSE;

   memset(lpOALFnTable, 0, sizeof(OPENALFNTABLE));

   int err = 0;
#define LOAD_PROC(x) do {                                                                    \
    lpOALFnTable->x = reinterpret_cast<decltype(lpOALFnTable->x)>(reinterpret_cast<void*>(   \
        GetProcAddress(g_hOpenALDLL, #x)));                                                  \
    if(!lpOALFnTable->x)                                                                     \
    {                                                                                        \
        OutputDebugStringA("Failed to find entry point\n");                                  \
        err = 1;                                                                             \
    }                                                                                        \
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
   LOAD_PROC(alGenFilters);
   LOAD_PROC(alDeleteFilters);
   LOAD_PROC(alIsFilter);
   LOAD_PROC(alFilterf);
   LOAD_PROC(alFilterfv);
   LOAD_PROC(alFilteri);
   LOAD_PROC(alFilteriv);
   LOAD_PROC(alGetFilterf);
   LOAD_PROC(alGetFilterfv);
   LOAD_PROC(alGetFilteri);
   LOAD_PROC(alGetFilteriv);
   LOAD_PROC(alGenEffects);
   LOAD_PROC(alDeleteEffects);
   LOAD_PROC(alIsEffect);
   LOAD_PROC(alEffectf);
   LOAD_PROC(alEffectfv);
   LOAD_PROC(alEffecti);
   LOAD_PROC(alEffectiv);
   LOAD_PROC(alGetEffectf);
   LOAD_PROC(alGetEffectfv);
   LOAD_PROC(alGetEffecti);
   LOAD_PROC(alGetEffectiv);
   LOAD_PROC(alGenAuxiliaryEffectSlots);
   LOAD_PROC(alDeleteAuxiliaryEffectSlots);
   LOAD_PROC(alIsAuxiliaryEffectSlot);
   LOAD_PROC(alAuxiliaryEffectSlotf);
   LOAD_PROC(alAuxiliaryEffectSlotfv);
   LOAD_PROC(alAuxiliaryEffectSloti);
   LOAD_PROC(alAuxiliaryEffectSlotiv);
   LOAD_PROC(alGetAuxiliaryEffectSlotf);
   LOAD_PROC(alGetAuxiliaryEffectSlotfv);
   LOAD_PROC(alGetAuxiliaryEffectSloti);
   LOAD_PROC(alGetAuxiliaryEffectSlotiv);
   if (!err)
   {
      ALCint alc_ver[2] = { 0, 0 };
      lpOALFnTable->alcGetIntegerv(nullptr, ALC_MAJOR_VERSION, 1, &alc_ver[0]);
      lpOALFnTable->alcGetIntegerv(nullptr, ALC_MINOR_VERSION, 1, &alc_ver[1]);
      if (lpOALFnTable->alcGetError(nullptr) == ALC_NO_ERROR)
         lpOALFnTable->ALCVer = MAKE_ALC_VER(alc_ver[0], alc_ver[1]);
      else
      {
         OutputDebugStringA("Failed to query ALC, assuming 1.0\n");
         lpOALFnTable->ALCVer = MAKE_ALC_VER(1, 0);
      }
#undef LOAD_PROC
#define LOAD_PROC(x) do {                                                     \
    lpOALFnTable->x = reinterpret_cast<decltype(lpOALFnTable->x)>(            \
        lpOALFnTable->alcGetProcAddress(nullptr, #x));                        \
    if(!lpOALFnTable->x)                                                      \
    {                                                                         \
        OutputDebugStringA("Failed to find entry point\n");                   \
        err = 1;                                                              \
    }                                                                         \
} while(0)
      if (lpOALFnTable->alcIsExtensionPresent(nullptr, "ALC_EXT_thread_local_context"))
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

ALvoid UnloadOAL10Library()
{
   // Unload the dll
   if (g_hOpenALDLL)
   {
      FreeLibrary(g_hOpenALDLL);
      g_hOpenALDLL = NULL;
   }
}
