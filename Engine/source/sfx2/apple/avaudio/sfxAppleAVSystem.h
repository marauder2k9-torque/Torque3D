//
//  sfxAppleAVSystem.h
//  TestProject
//
//  Created by ctrlintelligence on 01/09/2022.
//

#ifndef _SFXAPPLEAVSYSTEM_H_
#define _SFXAPPLEAVSYSTEM_H_

#include <Foundation/Foundation.h>
#include <AVFoundation/AVFoundation.h>

#ifndef _SFXSYSTEM2_H_
#include "sfx2/sfxSystem.h"
#endif

class SFXAVDevice : public SFXDevice
{
public:
   virtual ~SFXAVDevice();
protected:
   void init();
};


#endif /* _SFXAPPLEAVSYSTEM_H_ */
