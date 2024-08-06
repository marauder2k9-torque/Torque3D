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

#ifndef _SFXAPPLEBUFFER_H_
#define _SFXAPPLEBUFFER_H_

#ifndef _SFXAPPLEDEVICE_H_
#include "sfx/apple/sfxAPPLEDevice.h"
#endif

#ifndef _SFXINTERNAL_H_
   #include "sfx/sfxInternal.h"
#endif

class SFXAPPLEVoice;

class SFXAPPLEBuffer : public SFXBuffer
{
public:
   typedef SFXBuffer Parent;
   friend class SFXAPPLEVoice;
   friend class SFXAPPLEDevice;
   
protected:
   
   AVAudioPCMBuffer *mPCMBuffer;
   AVAudioFormat *mFormat;
   bool mIs3d;
   
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

#endif /* _SFXAPPLEBUFFER_H_ */
