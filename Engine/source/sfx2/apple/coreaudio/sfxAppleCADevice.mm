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

#include "console/console.h"
#include "console/engineAPI.h"

SFXCADevice::SFXCADevice(String name, bool isCapture)
:Parent(name, isCapture)
{
   
}

void SFXCADevice::init()
{
   // setup our device info holders.
   AudioDeviceID devID;
   U32 size - sizeof(AudioDeviceID);
   AudioObjectPropertyAddress propAddress;
   propAddress.mSelector   = kAudioHardwarePropertyDefaultOutputDevice;
   propAddress.mScope      = kAudioObjectPropertyScopeGlobal;
   propAddress.mElement    = kAudioObjectPropertyElementMaster;
   
   // get our audio device system information.
   OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propAddress, 0, NULL, size, &devID);
   
   if(status != noErr)
   {
      Con::printf("SFXCADevice - No Core Audio device found!");
      return;
   }
   
   // create a new audio graph.
   status = NewAUGraph(&AudioGraph);
   if(status != noErr)
   {
      Con::printf("SFXCADevice - Failed to create audio graph!");
      return;
   }
   
   AudioComponentDescription acDesc;
   acDesc.componentType = kAudioUnitType_Output;
   acDesc.componentSubType = kAudioUnitSubType_DefaultOutput;
   acDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
   acDesc.componentFlags = 0;
   acDesc.componentFlagsMask = 0;
   
   // create our output node.
   status = AUGraphAddNode(AudioGraph, &acDesc, &Output);
   if(status != noErr)
   {
      Con::printf("SFXCADevice - Failed to create Output Node!");
      return;
   }
   
   // open our audio graph.
   status = AUGraphOpen(AudioGraph);
   if(status != noErr)
   {
      Con::printf("SFXCADevice - Failed to open audio graph!");
      return;
   }
   
   // tie the output unit to the audio graph.
   status = AUGraphNodeInfo(AudioGraph, Output, NULL, &OutputUnit);
   if(status != noErr)
   {
      Con::printf("SFXCADevice - Failed to set audio info!");
      return;
   }
   
   // initialize the audio unit.
   status = AudioUnitInitialize(OutputUnit);
   if(status != noErr)
   {
      Con::printf("SFXCADevice - Failed to initialize audio unit!");
      return;
   }
   
}
