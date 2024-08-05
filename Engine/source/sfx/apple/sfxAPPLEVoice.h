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

#ifndef _SFXAPPLEVOICE_H_
#define _SFXAPPLEVOICE_H_

#ifndef _SFXAPPLEDEVICE_H_
#include "sfx/apple/sfxAPPLEDevice.h"
#endif

class SFXAPPLEBuffer;

class SFXAPPLEVoice : public SFXVoice
{
public:
   typedef SFXVoice Parent;
   friend class SFXAPPLEBuffer;
   friend class SFXAPPLEDevice;
   
public:
   /// Create a new voice for the given buffer.
   static SFXAPPLEVoice* create(SFXAPPLEDevice* device, SFXAPPLEBuffer* buffer);

   SFXAPPLEVoice(AVAudioEngine* audioEngine, SFXAPPLEBuffer* buffer);
   
   virtual ~SFXAPPLEVoice();

   /// @name SFXVoice
   /// @{
   virtual SFXStatus _status() const override;
   virtual void _play() override;
   virtual void _pause() override;
   virtual void _stop() override;
   virtual void _seek(U32 sample) override;
   virtual U32 _tell() const override;
   virtual void setVolume(F32 volume) override;
   virtual void setPitch(F32 pitch) override;
   virtual void setMinMaxDistance(F32 min, F32 max) override;
   virtual void play(bool looping) override;
   virtual void setVelocity(const VectorF& velocity) override;
   virtual void setTransform(const MatrixF& transform) override;
   virtual void setCone(F32 innerAngle, F32 outerAngle, F32 outerVolume) override;
   virtual void setRolloffFactor(F32 factor) override;
   /// @}

protected:
   
   AVAudioEngine* mAudioEngine;
   AVAudioPlayerNode* mPlayerNode;

};

#endif // _SFXAPPLEVOICE_H_
