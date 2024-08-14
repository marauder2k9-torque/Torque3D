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

#include "sfx/apple/sfxAPPLEBuffer.h"
#include "sfx/sfxDescription.h"
#include "console/console.h"

SFXAPPLEBuffer *SFXAPPLEBuffer::create(const ThreadSafeRef<SFXStream> &stream, SFXDescription *desc, bool useHardware) {
   SFXAPPLEBuffer* buffer = new SFXAPPLEBuffer(stream, desc, useHardware);
   
   return buffer;
}


SFXAPPLEBuffer::SFXAPPLEBuffer(const ThreadSafeRef<SFXStream> &stream, 
                               SFXDescription *desc,
                               bool useHardware)
   :  Parent(stream, desc),
      mIs3d(desc->mIs3D),
      mPCMBuffer(nullptr),
      mFormat(nullptr)
{
   const SFXFormat sfxFormat = stream->getFormat();
   
   // Standard initialization with sample rate and channels
   float sampleRate = sfxFormat.getSamplesPerSecond();
   AVAudioChannelCount channels = sfxFormat.getChannels();
   
   mFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:sampleRate
                                                            channels:channels];
   
   U32 sampleCount = stream->getSampleCount();
   
   mPCMBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:mFormat
                                              frameCapacity:sampleCount];
   
}


void SFXAPPLEBuffer::write(SFXInternal::SFXStreamPacket *const *packets, U32 num) {
   if (!num || !mFormat) {
      return;
   }

   auto convertShortToFloat32 = [](const int16_t *inputData, Float32 *outputData, U32 frameCount, U32 channelCount) {
      for (U32 frame = 0; frame < frameCount; ++frame) {
         for (U32 channel = 0; channel < channelCount; ++channel) {
            outputData[frame * channelCount + channel] = inputData[frame * channelCount + channel] / 32768.0f;
         }
      }
   };

   // Process non-streaming buffer
   if (!isStreaming()) {
      SFXInternal::SFXStreamPacket* packet = packets[num - 1];

      if (!mFormat) {
         return;
      }
      
      U32 numFrames = packet->mFormat.getFrames();

      if (!mPCMBuffer || mPCMBuffer.frameCapacity < numFrames) {
         if (mPCMBuffer) {
            [mPCMBuffer release];
         }
         mPCMBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:mFormat 
                                                    frameCapacity:numFrames];
      }
      
      U32 numChannels = packet->mFormat.getChannels();
      mPCMBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:mFormat
                                                 frameCapacity:numFrames];
      
      // Convert the int16 data to Float32 and fill the buffer
      int16_t *sourceData = (int16_t *)packet->data;
      Float32 *floatData = (Float32 *)mPCMBuffer.mutableAudioBufferList->mBuffers->mData;

      for (U32 i = 0; i < numFrames * numChannels; i++) {
          floatData[i] = (Float32)((sourceData[i]) / 32768.0f);
      }

      mPCMBuffer.frameLength = numFrames;

      destructSingle(packet);
      return;
   }

   // Process streaming buffers
   for (U32 i = 0; i < num; ++i) {
      SFXInternal::SFXStreamPacket* packet = packets[i];
      AVAudioFormat* format = mFormat;

      if (!format) {
         continue;
      }

      U32 numFrames = packet->mFormat.getFrames();
      mPCMBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format
      frameCapacity:numFrames];
      mPCMBuffer.frameLength = numFrames;

      // Allocate and convert data
      Float32 *floatData = (Float32 *)malloc(numFrames * sizeof(Float32));
      convertShortToFloat32((const int16_t *)packet->data, floatData, numFrames, packet->mFormat.getChannels());

      // Copy data to buffer
      memcpy(mPCMBuffer.audioBufferList->mBuffers[0].mData, floatData, numFrames * sizeof(Float32));
      free(floatData);

      destructSingle(packet);
   }
}

void SFXAPPLEBuffer::_flush() {
   [mPCMBuffer release];
}

SFXAPPLEBuffer::~SFXAPPLEBuffer() {
   if(mPCMBuffer)
   {
      [mPCMBuffer release];
      mPCMBuffer = nullptr;
   }
   
   if(mFormat)
   {
      [mFormat release];
      mFormat = nullptr;
   }
}
