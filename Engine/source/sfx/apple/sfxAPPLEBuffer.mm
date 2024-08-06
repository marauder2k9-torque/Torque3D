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
   
   mFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:sfxFormat.getSamplesPerSecond()
                                                           channels:sfxFormat.getChannels()];
   
   U32 sampleCount = stream->getSampleCount();
   
   mPCMBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:mFormat
                                             frameCapacity:sampleCount];
}


void SFXAPPLEBuffer::write(SFXInternal::SFXStreamPacket *const *packets, U32 num) {
    if (!num || !mFormat) {
        return;
    }

    // Helper function to convert uint8_t data to Float32
    auto convertUInt8ToFloat32 = [](const uint8_t *inputData, Float32 *outputData, U32 sampleCount) {
        for (U32 i = 0; i < sampleCount; ++i) {
            outputData[i] = (inputData[i] - 128) / 128.0f;
        }
    };

    // Process non-streaming buffer
    if (!isStreaming()) {
        SFXInternal::SFXStreamPacket* packet = packets[num - 1];
        AVAudioFormat* format = mFormat;

        if (!format) {
            return;
        }

        if (mPCMBuffer) {
            [mPCMBuffer release];
        }

        U32 numFrames = num * getFormat().getBytesPerSample();
        mPCMBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format
                                                  frameCapacity:packet->mSizeActual];
        mPCMBuffer.frameLength = format.streamDescription->mBytesPerFrame;
       
        // Allocate and convert data
        Float32 *floatData = (Float32 *)malloc(numFrames * sizeof(Float32));
        convertUInt8ToFloat32((const uint8_t *)packet->data, floatData, numFrames);

        // Copy data to buffer
        memcpy(mPCMBuffer.audioBufferList->mBuffers[0].mData, floatData, numFrames * sizeof(Float32));
        free(floatData);

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

        U32 numFrames = packet->mSizeActual / format.streamDescription->mBytesPerFrame;
        mPCMBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:format
                                                  frameCapacity:numFrames];
        mPCMBuffer.frameLength = numFrames;

        // Allocate and convert data
        Float32 *floatData = (Float32 *)malloc(numFrames * sizeof(Float32));
        convertUInt8ToFloat32((const uint8_t *)packet->data, floatData, numFrames);

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
