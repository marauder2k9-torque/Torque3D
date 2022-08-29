//
//  sfxAppleDevice.m
//  TestProject
//
//  Created by ctrlintelligence on 29/08/2022.
//

#import <Foundation/Foundation.h>
#include "sfxAppleDevice.h"
#include "platform/async/asyncUpdate.h"



SFXAppleDevice::SFXAppleDevice(SFXProvider *provider,
                               String name,
                               bool useHardware,
                               S32 maxBuffers)
:  Parent(name,provider,useHardware,maxBuffers),
   AudioGraph(NULL),
   Output(0),
   OutputUnit(0),
   mDistanceModel(SFXDistanceModelLinear),
   mDistanceFactor(1.0f),
   mRolloffFactor(1.0f),
   mUserRolloffFactor(1.0f)
{
   OSStatus status = NewAUGraph(&AudioGraph);
   if(status != noErr)
   {
      Con::errorf("SFXAppleDevice - Failed to create audio unit.");
      return;
   }
   AudioComponentDescription desc;
   desc.componentType = kAudioUnitType_Output;
   desc.componentManufacturer = kAudioUnitManufacturer_Apple;
   
   status = AUGraphAddNode(AudioGraph, &desc, &Output);
   if(status != noErr)
   {
      Con::errorf("SFXAppleDevice - Failed to create output node.");
      return;
   }
   
}

SFXAppleDevice::~SFXAppleDevice()
{
   if(AudioGraph)
   {
      AUGraphStop(AudioGraph);
      DisposeAUGraph(AudioGraph);
      
      AudioGraph = NULL;
      Output = -1;
      OutputUnit = NULL;
      
   }
}

SFXBuffer *SFXAppleDevice::createBuffer(const ThreadSafeRef<SFXStream>& stream,
                                SFXDescription *desc)
{
   return NULL;
}

SFXVoice *SFXAppleDevice::createVoice(bool is3D, SFXBuffer *buffer)
{
   return NULL;
}

void SFXAppleDevice::setListener(U32 idx, const SFXListenerProperties& listener)
{
   
}

void SFXAppleDevice::setDistanceModel(SFXDistanceModel model)
{
   
}

void SFXAppleDevice::setDopplerFactor(F32 factor)
{
   
}

void SFXAppleDevice::setRolloffFactor(F32 factor)
{
   
}

void SFXAppleDevice::setReverb(const SFXReverbProperties& reverb)
{
   
}
