//
//  sfxAppleAVSystem.h
//  TestProject
//
//  Created by ctrlintelligence on 01/09/2022.
//

#ifndef _SFXAPPLEAVSYSTEM_H_
#define _SFXAPPLEAVSYSTEM_H_

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#ifndef _SFXSYSTEM2_H_
#import "sfx2/sfxSystem.h"
#endif

class SFXAVDevice : public SFXDevice
{
public:
   void init();
private:
   AVAudioEngine mAudioEngine;
};

#endif /* _SFXAPPLEAVSYSTEM_H_ */
