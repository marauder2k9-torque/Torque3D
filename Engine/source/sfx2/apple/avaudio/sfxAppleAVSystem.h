//
//  sfxAppleAVSystem.h
//  TestProject
//
//  Created by ctrlintelligence on 01/09/2022.
//

#ifndef sfxAppleAVSystem_h
#define sfxAppleAVSystem_h

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#ifndef _SFXSYSTEM2_H_
#import "sfx2/sfxSystem.h"
#endif

class SFXCADevice : public SFXDevice
{
public:
   void init();
private:
   AVAudioEngine mAudioEngine;
};


#endif /* sfxAppleAVSystem_h */
