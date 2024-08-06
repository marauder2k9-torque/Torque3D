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
#include "sfx/apple/sfxAPPLEBuffer.h"
#include "sfx/apple/sfxAPPLEDevice.h"
#include "sfx/apple/sfxAPPLEVoice.h"


SFXAPPLEVoice *SFXAPPLEVoice::create(SFXAPPLEDevice *device, SFXAPPLEBuffer *buffer) {
   return new SFXAPPLEVoice(device, buffer);
}

SFXAPPLEVoice::SFXAPPLEVoice(SFXAPPLEDevice *device, SFXAPPLEBuffer *buffer)
:  Parent( buffer),
   mAudioEngine(device->audioEngine),
   mEnvironmentNode(device->listenerNode),
   mPitchControl([[AVAudioUnitTimePitch alloc] init]),
   mPlayerNode([[AVAudioPlayerNode alloc] init])
{
   [mAudioEngine attachNode:mPitchControl];
   [mAudioEngine attachNode:mPlayerNode];
   
   [mAudioEngine connect:mPlayerNode
                      to:mPitchControl
                  format:buffer->mFormat];
   
   if(buffer->mIs3d){
      [mAudioEngine connect:mPitchControl
                         to:mEnvironmentNode
                     format:buffer->mFormat];
      mPlayerNode.sourceMode = AVAudio3DMixingSourceModePointSource;
   }
   else
   {
      [mAudioEngine connect:mPitchControl
                         to:mAudioEngine.mainMixerNode
                     format:buffer->mFormat];
   }
   
   [mPlayerNode scheduleBuffer:buffer->mPCMBuffer
             completionHandler:nil];
   
   mPlayerNode.position = AVAudioMake3DPoint(0, 0, 0);
   
}

SFXAPPLEVoice::~SFXAPPLEVoice() {
   [mPlayerNode stop];
   [mAudioEngine detachNode:mPlayerNode];
   [mPlayerNode release];
   
   [mAudioEngine detachNode:mPitchControl];
   [mPitchControl release];
}

SFXStatus SFXAPPLEVoice::_status() const { 
   if(mPlayerNode.isPlaying){
      return SFXStatus::SFXStatusPlaying;
   }
   
   if(!mPlayerNode.isPlaying)
      return SFXStatus::SFXStatusStopped;
}

void SFXAPPLEVoice::_play() { 
   [mPlayerNode play];
}

void SFXAPPLEVoice::_pause() { 
   [mPlayerNode pause];
}

void SFXAPPLEVoice::_stop() { 
   [mPlayerNode stop];
   [mPlayerNode reset];
}

void SFXAPPLEVoice::_seek(U32 sample) { 
   [mPlayerNode stop];
}

U32 SFXAPPLEVoice::_tell() const { 
   
}

void SFXAPPLEVoice::setVolume(F32 volume) { 
   mPlayerNode.volume = volume;
}

void SFXAPPLEVoice::setPitch(F32 pitch) { 
   mPitchControl.pitch = pitch * 1200;
}

void SFXAPPLEVoice::setMinMaxDistance(F32 min, F32 max) {
}

void SFXAPPLEVoice::play(bool looping) { 
   [mPlayerNode play];
}

void SFXAPPLEVoice::setVelocity(const VectorF &velocity) { 
   
}

void SFXAPPLEVoice::setTransform(const MatrixF &transform) {
   Point3D pos = transform.getPosition();
   
   mPlayerNode.position = AVAudioMake3DPoint(pos.x, pos.y, pos.z);
}

void SFXAPPLEVoice::setCone(F32 innerAngle, F32 outerAngle, F32 outerVolume) {
}

void SFXAPPLEVoice::setRolloffFactor(F32 factor) { 
   
}














