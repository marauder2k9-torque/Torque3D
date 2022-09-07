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
#include "sfxAppleAVSystem.h"
#include "console/console.h"
#include "console/engineAPI.h"

SFXAVDevice::SFXAVDevice(String name, bool captureDevice)
: Parent(name, captureDevice)
{}

void SFXAVDevice::init()
{
   // no idea if this is right.
   mAudioEngine = [[AVAudioEngine alloc] init];
   
   // sure why not
   mOutputNode = mAudioEngine.outputNode;
   
   // node that all sources play through.
   mMixerNode = mAudioEngine.mainMixerNode;
   
   mOutputFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channels:2]
   
   [mAudioEngine connect:mMixerNode to:mOutputNode format:mOutputFormat]
   
   NSError *error = nil;
   [mAudioEngine prepare]
   if([mAudioEngine startAndReturnError:&error] == NO)
   {
      Con::printf("SFXAVDevice - Failed to start AudioEngine %s!", (char)error.description.UTF8String);
      return;
   }
   
}
