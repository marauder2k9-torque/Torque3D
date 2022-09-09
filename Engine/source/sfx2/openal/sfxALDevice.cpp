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

#include "sfx2/openal/sfxALSystem.h"

SFXALDevice::SFXALDevice(String name, String openalName, bool sysDefault, bool captureDevice)
   :Parent(name, openalName, sysDefault, captureDevice)
{

}

bool SFXALDevice::init()
{
   mDevice = alcOpenDevice(mAPIDeviceName);

   mContext = alcCreateContext(mDevice, NULL);

   String name = String::EmptyString;
   if (alcIsExtensionPresent(mDevice, "ALC_ENUMERATE_ALL_EXT"))
      name = alcGetString(mDevice, ALC_ALL_DEVICES_SPECIFIER);
   if (!name || alcGetError(mDevice) != AL_NO_ERROR)
      name = alcGetString(mDevice, ALC_DEVICE_SPECIFIER);

   Con::printf("SFXALDevice - Initialized %s", name.c_str());

   if (!_loadExtendedApi())
   {
      Con::printf("SFXALDevice - Extended EFX not supported by device %s", name);
      return;
   }
}

bool SFXALDevice::_loadExtendedApi()
{
   bool success = false;
#define LOAD_PROC(x) do {                                                     \
    x = reinterpret_cast<decltype(x)>(alGetProcAddress(#x));                  \
    if(x)                                                                     \
        Con::printf("SFXALDevice - Failed to find entry point for %s", #x);   \
} while(0)
   if (alcIsExtensionPresent(mDevice, "ALC_EXT_EFX"))
   {
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
      success = true;
   }
#undef LOAD_PROC

   return success;
}
