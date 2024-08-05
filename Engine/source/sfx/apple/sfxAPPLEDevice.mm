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

#include "platform/platform.h"

#include "sfx/sfxProvider.h"
#include "console/console.h"
#include "core/module.h"

#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#import <AVFoundation/AVFoundation.h>

namespace
{
   struct AVAudio{
      AVAudioEngine* audioEngine;
      AVAudioEnvironmentNode* listenerNode;
      
      AVAudio(){
         audioEngine = [[AVAudioEngine alloc] init];
         listenerNode = [[AVAudioEnvironmentNode alloc] init];
         
         listenerNode.listenerVectorOrientation = AVAudioMake3DVectorOrientation(
                                                                                 AVAudioMake3DVector(0,0,-1), // forward
                                                                                 AVAudioMake3DVector(0, 1, 0)); // up
         
         [audioEngine attachNode:listenerNode];
         [audioEngine connect:listenerNode to:[audioEngine mainMixerNode] format:nil];
         
         [audioEngine startAndReturnError:nil];
      }
      
      ~AVAudio(){
         [audioEngine stop];
      }
   };

   struct AVVoice{
      AVAudioEngine* audioEngine;
      AVAudioPlayerNode* playerNode;
      AVAudioPCMBuffer* pcmBuffer; // Assuming PCM buffer is used
      
      AVVoice(){
      }
      
      ~AVVoice(){
         [audioEngine stop];
      }
   };

   struct AVBuffer{
      AVAudioPCMBuffer *pcmBuffer;
      AVAudioFormat *format;
      
      AVBuffer()
      : pcmBuffer(nullptr), format(nullptr){}
      
      ~AVBuffer(){
         if(pcmBuffer)
         {
            [pcmBuffer release];
            pcmBuffer = nullptr;
         }
         
         if(format)
         {
            [format release];
            format = nullptr;
         }
      }
      
   };
}

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
   
   AVAudio* mAVAudio;
};

class SFXAPPLEBuffer : public SFXBuffer
{
public:
   typedef SFXBuffer Parent;
   
protected:
   
   AVBuffer* mAVBuffer;
   
   SFXAPPLEBuffer(const ThreadSafeRef<SFXStream>& stream,
                  SFXDescription* desc,
                  bool useHardware);
   
   void write( SFXInternal::SFXStreamPacket* const* packets, U32 num) override;
   void _flush() override;
   
public:
   static SFXAPPLEBuffer* create(const ThreadSafeRef<SFXStream>& stream,
                                 SFXDescription* desc,
                                 bool useHardware);
   
   virtual ~SFXAPPLEBuffer();
};

class SFXAPPLEVoice : public SFXVoice
{
public:
   typedef SFXVoice Parent;
   friend class SFXAPPLEDevice;
   friend class SFXAPPLEBuffer;
private:
   
   AVVoice* mAVVoice;
   
protected:
   SFXAPPLEVoice(AVVoice* avVoice,
                 SFXAPPLEBuffer* buffer);
   
   SFXAPPLEBuffer* _getBuffer() const
   {
      return (SFXAPPLEBuffer*) mBuffer.getPointer();
   }
   
   
   // SFXVoice.
   SFXStatus _status() const override;
   void _play() override;
   void _pause() override;
   void _stop() override;
   void _seek( U32 sample ) override;
   U32 _tell() const override;
   
public:
   static SFXAPPLEVoice* create(SFXAPPLEDevice* device,
                                SFXAPPLEBuffer* buffer);
   
   virtual ~SFXAPPLEVoice();
   
   /// SFXVoice
   void setMinMaxDistance( F32 min, F32 max ) override;
   void play( bool looping ) override;
   void setVelocity( const VectorF& velocity ) override;
   void setTransform( const MatrixF& transform ) override;
   void setVolume( F32 volume ) override;
   void setPitch( F32 pitch ) override;
   void setCone( F32 innerAngle, F32 outerAngle, F32 outerVolume ) override;
   void setRolloffFactor( F32 factor ) override;
};

class SFXAPPLEProvider : public SFXProvider
{
public:
   SFXAPPLEProvider() : SFXProvider( "Apple" )
   {}
   
   virtual ~SFXAPPLEProvider() {}
   
protected:
   void init() override;
   String getDefaultDeviceName();
   
public:
   SFXDevice *createDevice(const String& deviceName, bool useHardware, S32 maxBuffers) override;
};

MODULE_BEGIN(SFXAPPLE)

   MODULE_INIT_BEFORE( SFX )
   MODULE_SHUTDOWN_AFTER( SFX )

   SFXAPPLEProvider* mProvider = NULL;
   
   MODULE_INIT
   {
      mProvider = new SFXAPPLEProvider;
   }

   MODULE_SHUTDOWN
   {
      delete mProvider;
   }

MODULE_END;

String SFXAPPLEProvider::getDefaultDeviceName(){
   AudioDeviceID devid = kAudioObjectUnknown;
   
   U32 size = sizeof(devid);
   AudioObjectPropertyAddress addr = {
      kAudioHardwarePropertyDefaultOutputDevice,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
   };
   
   OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, NULL, &size, &devid);
   if(status != noErr)
   {
      Con::errorf("SFXAppleProvider: Could not find default device!");
      return String::EmptyString;
   }
   
   CFStringRef devName = NULL;
   size = sizeof(devName);
   AudioObjectPropertyAddress nameAddress = {
      kAudioDevicePropertyDeviceNameCFString,
      kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster
   };
   
   status = AudioObjectGetPropertyData(devid, &nameAddress, 0, NULL, &size, &devName);
   if(status != noErr)
   {
      Con::errorf("SFXAppleProvider: Could not find default device!");
      return String::EmptyString;
   }
   
   char name[256];
   CFStringGetCString(devName, name, sizeof(name), kCFStringEncodingUTF8);
   
   return String::ToString(name);
}

void SFXAPPLEProvider::init()
{
   String deviceName = getDefaultDeviceName();
   
   SFXDeviceInfo* info = new SFXDeviceInfo;
   
   info->internalName = deviceName;
   info->name = deviceName;
   
   mDeviceInfo.push_back(info);
   
   regProvider(this);
}

SFXDevice *SFXAPPLEProvider::createDevice(const String &deviceName, bool useHardware, int maxBuffers)
{
   SFXDeviceInfo* info = _findDeviceInfo(deviceName);
   return new SFXAPPLEDevice(this, info->internalName, useHardware, maxBuffers);
}


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


SFXAPPLEBuffer *SFXAPPLEBuffer::create(const ThreadSafeRef<SFXStream> &stream, SFXDescription *desc, bool useHardware) {
   SFXAPPLEBuffer* buffer = new SFXAPPLEBuffer(stream, desc, useHardware);
   
   return buffer;
}


SFXAPPLEBuffer::SFXAPPLEBuffer(const ThreadSafeRef<SFXStream> &stream, SFXDescription *desc, bool useHardware) : Parent(stream, desc)
{
   this->mAVBuffer = new AVBuffer();
   
   const SFXFormat format = stream->getFormat();
   
   
   
   mAVBuffer->format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:format.getSamplesPerSecond() channels:format.getChannels()];
   
   U32 sampleCount = stream->getSampleCount();
   
   mAVBuffer->pcmBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:mAVBuffer->format frameCapacity:sampleCount];
}


void SFXAPPLEBuffer::write(SFXInternal::SFXStreamPacket *const *packets, U32 num) {
   if (!num || !mAVBuffer)
   {
      return;
   }

   // If this is not a streaming buffer, load the data into our single static buffer.
   if (!isStreaming())
   {
      SFXInternal::SFXStreamPacket* packet = packets[num - 1];

      // Ensure format is valid and create AVAudioPCMBuffer.
      AVAudioFormat* format = mAVBuffer->format;
      if (!format)
      {
         return;
      }

      if (mAVBuffer->pcmBuffer)
      {
         [mAVBuffer->pcmBuffer release];
      }
      
      mAVBuffer->pcmBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format frameCapacity:packet->mSizeActual / format.streamDescription->mBytesPerFrame];
      mAVBuffer->pcmBuffer.frameLength = mAVBuffer->pcmBuffer.frameCapacity;

      // Copy data to AVAudioPCMBuffer
      memcpy(mAVBuffer->pcmBuffer.audioBufferList->mBuffers[0].mData, packet->data, packet->mSizeActual);

      // Clean up packet
      destructSingle(packet);
      return;
   }

   // Unqueue processed packets if necessary (done in the playernode (voice)

   // Queue new buffers
   for (U32 i = 0; i < num; ++i)
   {
      SFXInternal::SFXStreamPacket* packet = packets[i];
      AVAudioFormat* format = mAVBuffer->format;

      // Ensure format is valid and create AVAudioPCMBuffer
      if (!format) {
         continue;
      }

      mAVBuffer->pcmBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format       frameCapacity:packet->mSizeActual / format.streamDescription->mBytesPerFrame];
      mAVBuffer->pcmBuffer.frameLength = mAVBuffer->pcmBuffer.frameCapacity;

      // Copy data to AVAudioPCMBuffer
      memcpy(mAVBuffer->pcmBuffer.audioBufferList->mBuffers[0].mData, packet->data, packet->mSizeActual);

      // Clean up packet
      destructSingle(packet);
   }
}

void SFXAPPLEBuffer::_flush() {

}

SFXAPPLEBuffer::~SFXAPPLEBuffer() {
   delete mAVBuffer;
}


SFXAPPLEVoice* SFXAPPLEVoice::create(SFXAPPLEDevice *device, SFXAPPLEBuffer *buffer)
{
   AssertFatal(buffer, "SFXAppleVoice::create: - Got a null buffer");
   
   AVVoice* voice = new AVVoice();
   
   return new SFXAPPLEVoice(voice, buffer);
}

SFXAPPLEVoice::SFXAPPLEVoice(AVVoice* avVoice, SFXAPPLEBuffer *buffer)
: Parent(buffer)
{
   
}

