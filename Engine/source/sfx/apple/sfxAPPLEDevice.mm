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

#include "sfx/apple/sfxAPPLEDevice.h"
#include "sfx/apple/sfxAPPLEBuffer.h"

#import <AVFoundation/AVFoundation.h>

struct SFXAPPLEDevice::AVAudio{
   AVAudioEngine* audioEngine;
   AVAudioPlayerNode* listenerNode;
   
   AVAudio(){
      audioEngine = [[AVAudioEngine alloc] init];
      listenerNode = [[AVAudioPlayerNode alloc] init];
      
      [audioEngine attachNode:listenerNode];
      AVAudioFormat* format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100 channels:2];
      [audioEngine attachNode:listenerNode];
      [audioEngine connect:listenerNode to:[audioEngine mainMixerNode] format:format];
      
      [audioEngine startAndReturnError:nil];
   }
   
   ~AVAudio(){
      [audioEngine stop];
   }
};

SFXAPPLEDevice::SFXAPPLEDevice(SFXProvider *provider, String name, bool useHardware, S32 maxBuffers) 
: Parent(name, provider, useHardware, maxBuffers)
{
   mAVAudio = new AVAudio();
}

SFXAPPLEDevice::~SFXAPPLEDevice(){
   delete mAVAudio;
}

SFXBuffer* SFXAPPLEDevice::createBuffer(const ThreadSafeRef<SFXStream> &stream, SFXDescription *desc)
{
   
}

SFXVoice* SFXAPPLEDevice::createVoice(bool is3D, SFXBuffer *buffer)
{
   
}

void SFXAPPLEDevice::setListener(U32 index, const SFXListenerProperties &listener)
{
   
}

void SFXAPPLEDevice::setDistanceModel(SFXDistanceModel model)
{
   return;
}

void SFXAPPLEDevice::setDopplerFactor(F32 fac)
{
   
}

void SFXAPPLEDevice::setRolloffFactor(F32 fac)
{
   
}
