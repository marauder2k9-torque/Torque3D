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

#ifndef _SFXAPPLECASYSTEM_H_
#define _SFXAPPLECASYSTEM_H_

#ifndef _SFXSYSTEM2_H_
#include "sfx2/sfxSystem.h"
#endif

// Apple Core Audio Includes
#include <CoreAudio/AudioHardware.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

// forward declaration.
// SFXCADevice talks with all these other classes through the top level
// SFXSystem. But SFXCABuffer, SFXCASource and SFXCAProvider need access
// to SFXCADevice.
class SFXCADevice;

class SFXCABuffer : public SFXBuffer
{

};

class SFXCASource : public SFXSource
{

};

class SFXCAProvider : public SFXProvider
{
public:
   SFXCAProvider()
   : SFXProvider("Apple Core Audio"){}
   virtual ~SFXCAProvider();
   
   void init();
   
   // initalize the device linked to this provider, with apple its only 1 device for now.
   void initDevice(SFXCADevice* device);
};

class SFXCADevice : public SFXDevice
{
public:
   
   // initialize this device as our active device.
   void init();
   
   // we are changing device or shutting down deInit.
   void deInit();
private:
   AUGraph     AudioGraph;
   AUNode      Output;
   AudioUnit   OutputUnit;
};

class SFXCAMixer : public SFXMixer
{

};

class SFXAEffectManager : public SFXEffectManager
{

};

#endif // !_SFXAPPLECASYSTEM_H_
