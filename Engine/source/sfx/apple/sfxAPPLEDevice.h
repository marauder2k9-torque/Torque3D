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

#ifndef _SFXAPPLEDEVICE_H_
#define _SFXAPPLEDEVICE_H_

#ifndef _SFXDEVICE_H_
#include "sfx/sfxDevice.h"
#endif 

#ifndef _SFXPROVIDER_H_
#include "sfx/sfxProvider.h"
#endif

class SFXAPPLEDevice : public SFXDevice
{
public:
   typedef SFXDevice Parent;
   
   SFXAPPLEDevice( SFXProvider* provider,
                   String name,
                   bool useHardware,
                   S32 maxBuffers);
   
   virtual ~SFXAPPLEDevice();
   
   SFXBuffer* createBuffer(const ThreadSafeRef<SFXStream>& stream, SFXDescription* desc) override;
   SFXVoice* createVoice(bool is3D, SFXBuffer* buffer) override;
   void setListener(U32 index, const SFXListenerProperties& listener) override;
   void setDistanceModel(SFXDistanceModel model) override;
   void setDopplerFactor(F32 fac) override;
   void setRolloffFactor(F32 fac) override;
   
   void resetReverb() override {}
   
private:
   struct AVAudio;
   AVAudio* mAVAudio;
};

#endif /* _SFXAPPLEDEVICE_H_ */

