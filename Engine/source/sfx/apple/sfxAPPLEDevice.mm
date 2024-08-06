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
#include "sfx/apple/sfxAPPLEVoice.h"

SFXAPPLEDevice::SFXAPPLEDevice(SFXProvider *provider, String name, bool useHardware, S32 maxBuffers)
: Parent(name, provider, useHardware, maxBuffers)
{
   audioEngine = [[AVAudioEngine alloc] init];
   listenerNode = [[AVAudioEnvironmentNode alloc] init];
   
   listenerNode.renderingAlgorithm = AVAudio3DMixingRenderingAlgorithmAuto;
   
   listenerNode.listenerVectorOrientation = AVAudioMake3DVectorOrientation(
                                                AVAudioMake3DVector(0,0,-1), // forward
                                                AVAudioMake3DVector(0, 1, 0)); // up
   
   [audioEngine attachNode:listenerNode];
   
   [audioEngine connect:listenerNode
                     to:[audioEngine mainMixerNode] format:nil];
   
   [audioEngine connect:[audioEngine mainMixerNode]
                     to:[audioEngine outputNode]
                 format:nil];
   
   [audioEngine startAndReturnError:nil];
}

SFXAPPLEDevice::~SFXAPPLEDevice(){
   [audioEngine stop];
}

SFXBuffer* SFXAPPLEDevice::createBuffer(const ThreadSafeRef<SFXStream> &stream, SFXDescription *desc)
{
   AssertFatal( stream, "SFXALDevice::createBuffer() - Got null stream!" );
   AssertFatal( desc, "SFXALDevice::createBuffer() - Got null description!" );
   
   SFXAPPLEBuffer* buffer = SFXAPPLEBuffer::create(stream, desc, mUseHardware);
   
   if(!buffer)
      return NULL;
   
   _addBuffer(buffer);
   
   return buffer;
}

SFXVoice* SFXAPPLEDevice::createVoice(bool is3D, SFXBuffer *buffer)
{
   SFXAPPLEBuffer* buf = dynamic_cast<SFXAPPLEBuffer*>(buffer);
   AssertFatal(buf, "SFXAPPLEDevice::createVoice - bad buffer");
   
   SFXAPPLEVoice* voice = SFXAPPLEVoice::create(this, buf);
   if(!voice)
      return NULL;
   
   _addVoice(voice);
   
   return voice;
}

void SFXAPPLEDevice::setListener(U32 index, const SFXListenerProperties &listener)
{
   if(index != 0)
      return;
   
   const MatrixF &transform = listener.getTransform();
   
   VectorF up = transform.getUpVector();
   VectorF forward = transform.getForwardVector();
   
   Point3F pos = transform.getPosition();
   
   listenerNode.listenerPosition = AVAudioMake3DPoint(pos.x, pos.y, pos.z);
   listenerNode.listenerVectorOrientation = AVAudioMake3DVectorOrientation(
                                                AVAudioMake3DVector(forward.x, forward.y, forward.z), // forward
                                                AVAudioMake3DVector(up.x, up.y, up.z)); // up
}

void SFXAPPLEDevice::setDistanceModel(SFXDistanceModel model)
{
   listenerNode.distanceAttenuationParameters.distanceAttenuationModel = AVAudioEnvironmentDistanceAttenuationModelLinear;
   return;
}

void SFXAPPLEDevice::setDopplerFactor(F32 fac)
{
   
}

void SFXAPPLEDevice::setRolloffFactor(F32 fac)
{
   listenerNode.distanceAttenuationParameters.rolloffFactor = fac;
}


