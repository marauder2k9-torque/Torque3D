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

#import <AVFoundation/AVFoundation.h>

struct SFXAPPLEBuffer::AVAudioBuffer{
   AVAudioPCMBuffer *pcmBuffer;
   AVAudioFormat *format;
   
   AVAudioBuffer()
   : pcmBuffer(nullptr), format(nullptr){}
   
   ~AVAudioBuffer(){
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

SFXAPPLEBuffer *SFXAPPLEBuffer::create(const ThreadSafeRef<SFXStream> &stream, SFXDescription *desc, bool useHardware) {
   SFXAPPLEBuffer* buffer = new SFXAPPLEBuffer(stream, desc, useHardware);
   
   return buffer;
}


SFXAPPLEBuffer::SFXAPPLEBuffer(const ThreadSafeRef<SFXStream> &stream, SFXDescription *desc, bool useHardware) : Parent(stream, desc)
{
   this->mAVBuffer = new AVAudioBuffer();
   
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




