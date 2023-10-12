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

#include "core/stream/fileStream.h"
#include "core/stream/memStream.h"
#include "core/strings/stringFunctions.h"
#include "gfx/bitmap/gBitmap.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"
#endif


static bool sReadSTB(Stream& stream, GBitmap* bitmap);
static bool sReadSTB(const Torque::Path& path, GBitmap* bitmap);
static bool sWriteSTB(GBitmap* bitmap, Stream& stream, U32 compressionLevel);

static struct _privateRegisterSTB
{
   _privateRegisterSTB()
   {
      GBitmap::Registration reg;

      reg.extensions.push_back("png");
      reg.extensions.push_back("bmp");
      reg.extensions.push_back("jpg");
      reg.extensions.push_back("jpeg");
      reg.extensions.push_back("psd");
      reg.extensions.push_back("hdr");
      reg.extensions.push_back("tga");

      reg.readFileFunc = sReadSTB;
      reg.writeFunc = sWriteSTB;

      GBitmap::sRegisterFormat(reg);

   }
} sStaticRegisterSTB;

bool sReadSTB(Stream& stream, GBitmap* bitmap)
{
   return false;
}

bool sReadSTB(const Torque::Path& path, GBitmap* bitmap)
{

   PROFILE_SCOPE(sReadSTB);
  
   S32 x, y, n, channels;
   bool is16 = false;
   bool isHdr = false;

   U32 prevWaterMark = FrameAllocator::getWaterMark();

   if (!stbi_info(path.getFullPath().c_str(), &x, &y, &channels))
   {
      FrameAllocator::setWaterMark(prevWaterMark);
      return false;
   }

   // do this to map one channel to 3 and 2 channels to 4
   if (channels == 2)
      channels = 4;

   if (stbi_is_16_bit(path.getFullPath().c_str()))
   {
      is16 = true;
      channels = 4;
   }

   // stbi treats hdrs as 32f (all float are 32f from stbi so no 16f =/ )
   if (stbi_is_hdr(path.getFullPath().c_str()))
   {
      isHdr = true;
      is16 = false;
      channels = 4;
   }

   unsigned char* data = stbi_load(path.getFullPath().c_str(), &x, &y, &n, channels);

   bitmap->deleteImage();

   GFXFormat format;

   switch (n) {
   case  1:
      if (is16)
      {
         format = GFXFormatL16;
         break;
      }
      format = GFXFormatA8;
      break;
   case 2:
      format = GFXFormatA8L8;
      break;
   case 3:
      format = GFXFormatR8G8B8;
      break;
   case 4:
      if (isHdr)
      {
         format = GFXFormatR32G32B32A32F;
         break;
      }

      if (is16)
      {
         format = GFXFormatR16G16B16A16;
         break;
      }

      format = GFXFormatR8G8B8A8;
      break;
   default:
      FrameAllocator::setWaterMark(prevWaterMark);
      return false;
   }

   // actually allocate the bitmap space...
   bitmap->allocateBitmap(x, y,
      false,            // don't extrude miplevels...
      format);          // use determined format...

   U8* pBase = (U8*)bitmap->getBits();

   U32 rowBytes = y * x * n;

   dMemcpy(pBase, data, rowBytes);

   stbi_image_free(data);
   // Check this bitmap for transparency
   if(channels == 4 && !is16)
      bitmap->checkForTransparency();

   FrameAllocator::setWaterMark(prevWaterMark);

   return true;
}

bool sWriteSTB(GBitmap* bitmap, Stream& stream, U32 compressionLevel)
{
   return false;
}
