//
//  sfxAppleDevice.h
//  TestProject
//
//  Created by ctrlintelligence on 29/08/2022.
//

#ifndef _SFXAPPLEDEVICE_H_
#define _SFXAPPLEDEVICE_H_

class SFXProvider;

// apple includes for av.
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <AVFoundation/AVFoundation.h>
#include <AudioUnit/AudioUnit.h>
//

#ifndef _SFXDEVICE_H_
#include "sfx/sfxDevice.h"
#endif

#ifndef _SFXPROVIDER_H_
#include "sfx/sfxProvider.h"
#endif

class SFXAppleDevice final : public SFXDevice
{
protected:
   AUGraph     AudioGraph;
   AUNode      Output;
   AudioUnit   OutputUnit;
   
   SFXDistanceModel mDistanceModel;
   F32 mDistanceFactor;
   F32 mRolloffFactor;
   F32 mUserRolloffFactor;
   
public:
   typedef SFXDevice Parent;
   
   SFXAppleDevice(SFXProvider *provider,
                  String name,
                  bool useHardware,
                  S32 maxBuffers);
   virtual ~SFXAppleDevice();
   
   virtual SFXBuffer *createBuffer(const ThreadSafeRef<SFXStream>& stream,
                                   SFXDescription *desc) final;
   
   virtual SFXVoice *createVoice(bool is3D, SFXBuffer *buffer) final;
   virtual void setListener(U32 idx, const SFXListenerProperties& listener) final;
   virtual void setDistanceModel(SFXDistanceModel model) final;
   virtual void setDopplerFactor(F32 factor) final;
   virtual void setRolloffFactor(F32 factor) final;
   virtual void setReverb(const SFXReverbProperties& reverb) final;
   virtual void resetReverb() final {};
};

class SFXAppleProvider final : public SFXProvider
{
   friend class SFXProvider;
public:
   
   SFXAppleProvider() : SFXProvider("Apple"){}
   virtual ~SFXAppleProvider();
   
protected:
   struct AppleDeviceInfo : SFXDeviceInfo
   {
      
   };
   
   AppleDeviceInfo *mAppDI;
   
   void init();
   
public:
   SFXDevice *createDevice(const String& deviceName, bool useHardware, S32 maxBuffers);
};


#endif /* _SFXAPPLEDEVICE_H_ */
