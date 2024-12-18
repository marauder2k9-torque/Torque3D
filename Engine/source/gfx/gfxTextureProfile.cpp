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
#include "gfx/gfxTextureProfile.h"

#include "gfx/gfxTextureObject.h"
#include "gfx/bitmap/gBitmap.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "console/engineAPI.h"


// Set up defaults...

GFX_ImplementTextureProfile(GFXRenderTargetProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::PreserveSize | GFXTextureProfile::NoMipmap | GFXTextureProfile::RenderTarget,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXRenderTargetSRGBProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::PreserveSize | GFXTextureProfile::NoMipmap | GFXTextureProfile::RenderTarget | GFXTextureProfile::SRGB,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXStaticTextureProfile, GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::Static,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXStaticTextureSRGBProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::Static | GFXTextureProfile::SRGB,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXTexturePersistentProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::PreserveSize | GFXTextureProfile::Static | GFXTextureProfile::KeepBitmap,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXTexturePersistentSRGBProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::PreserveSize | GFXTextureProfile::Static | GFXTextureProfile::KeepBitmap | GFXTextureProfile::SRGB,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXSystemMemTextureProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::PreserveSize | GFXTextureProfile::NoMipmap | GFXTextureProfile::SystemMemory,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXNormalMapProfile,
                            GFXTextureProfile::NormalMap,
                            GFXTextureProfile::Static,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXNormalMapBC3Profile,
                            GFXTextureProfile::NormalMap,
                            GFXTextureProfile::Static,
                            GFXTextureProfile::BC3);
GFX_ImplementTextureProfile(GFXNormalMapBC5Profile,
                            GFXTextureProfile::NormalMap,
                            GFXTextureProfile::Static,
                            GFXTextureProfile::BC5);
GFX_ImplementTextureProfile(GFXZTargetProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::PreserveSize | GFXTextureProfile::NoMipmap | GFXTextureProfile::ZTarget | GFXTextureProfile::NoDiscard,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXDynamicTextureProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::Dynamic,
                            GFXTextureProfile::NONE);
GFX_ImplementTextureProfile(GFXDynamicTextureSRGBProfile,
                            GFXTextureProfile::DiffuseMap,
                            GFXTextureProfile::Dynamic | GFXTextureProfile::SRGB,
                            GFXTextureProfile::NONE);

//-----------------------------------------------------------------------------

GFXTextureProfile *GFXTextureProfile::smHead = NULL;
U32 GFXTextureProfile::smProfileCount = 0;

GFXTextureProfile::GFXTextureProfile(const String &name, Types type, U32 flag, Compression compression)
{
   mName = generateName(type, flag, compression);
   // Take type, flag, and compression and produce a munged profile word.
   mProfile = (type & (BIT(TypeBits + 1) - 1)) |
             ((flag & (BIT(FlagBits + 1) - 1)) << TypeBits) | 
             ((compression & (BIT(CompressionBits + 1) - 1)) << (FlagBits + TypeBits));   

   // Stick us on the linked list.
   mNext = smHead;
   smHead = this;
   ++smProfileCount;

   // Now do some sanity checking. (Ben is not proud of this code.)
   AssertFatal( (testFlag(Dynamic) && !testFlag(Static)) 
                  || (!testFlag(Dynamic) && !testFlag(Static)) 
                  || (!testFlag(Dynamic) &&  testFlag(Static)), 
                  "GFXTextureProfile::GFXTextureProfile - Cannot have a texture profile be both static and dynamic!");
   mDownscale = 0;
}

String GFXTextureProfile::generateName(Types type, U32 flag, Compression compression)
{
   StringBuilder str;
   str.append("GFX");

   // Add type to name.
   switch (type)
   {
   case GFXTextureProfile::DiffuseMap:
      str.append("Diffuse");
      break;
   case GFXTextureProfile::NormalMap:
      str.append("Normal");
      break;
   case GFXTextureProfile::AlphaMap:
      str.append("Alpha");
      break;
   case GFXTextureProfile::LuminanceMap:
      str.append("Luminance");
      break;
   default:
      str.append("NoType");
      break;
   }

   // Add flags to the name
   if (flag & PreserveSize)   str.append("_PreserveSize");
   if (flag & NoMipmap)       str.append("_NoMipmap");
   if (flag & SystemMemory)   str.append("_SystemMemory");
   if (flag & RenderTarget)   str.append("_RenderTarget");
   if (flag & Dynamic)        str.append("_Dynamic");
   if (flag & Static)         str.append("_Static");
   if (flag & NoPadding)      str.append("_NoPadding");
   if (flag & KeepBitmap)     str.append("_KeepBitmap");
   if (flag & ZTarget)        str.append("_ZTarget");
   if (flag & SRGB)           str.append("_SRGB");
   if (flag & Pooled)         str.append("_Pooled");
   if (flag & NoDiscard)      str.append("_NoDiscard");
   if (flag & NoModify)       str.append("_NoModify");

   // Add compression to the name
   switch (compression)
   {
      case NONE: str.append("_NONE"); break;
      case BC1:  str.append("_BC1");  break;
      case BC2:  str.append("_BC2");  break;
      case BC3:  str.append("_BC3");  break;
      case BC4:  str.append("_BC4");  break;
      case BC5:  str.append("_BC5");  break;
   }

   return str.end();
}

void GFXTextureProfile::init()
{
   // Do something, anything?
}

GFXTextureProfile * GFXTextureProfile::find(const String &name)
{
   // Not really necessary at this time.
   return NULL;
}

void GFXTextureProfile::collectStats( Flags flags, GFXTextureProfileStats *stats )
{
   // Walk the profile list.
   GFXTextureProfile *curr = smHead;
   while ( curr )
   {
      if ( curr->testFlag( flags ) )
         (*stats) += curr->getStats();

      curr = curr->mNext;
   }
}

void GFXTextureProfile::updateStatsForCreation(GFXTextureObject *t)
{
   if(t->mProfile)
   {
      t->mProfile->incActiveCopies();
      t->mProfile->mStats.allocatedTextures++;
      
      U32 texSize = t->getHeight() * t->getWidth();
      U32 byteSize = t->getEstimatedSizeInBytes();

      t->mProfile->mStats.allocatedTexels += texSize;
      t->mProfile->mStats.allocatedBytes  += byteSize;

      t->mProfile->mStats.activeTexels += texSize;
      t->mProfile->mStats.activeBytes += byteSize; 
   }
}

void GFXTextureProfile::updateStatsForDeletion(GFXTextureObject *t)
{
   if(t->mProfile)
   {
      t->mProfile->decActiveCopies();
      
      U32 texSize = t->getHeight() * t->getWidth();
      U32 byteSize = t->getEstimatedSizeInBytes();

      t->mProfile->mStats.activeTexels -= texSize;
      t->mProfile->mStats.activeBytes -= byteSize; 
   }
}

DefineEngineFunction( getTextureProfileStats, String, (),,
   "Returns a list of texture profiles in the format: ProfileName TextureCount TextureMB\n"
   "@ingroup GFX\n" )
{
   StringBuilder result;

   GFXTextureProfile *profile = GFXTextureProfile::getHead();
   while ( profile )
   {
      const GFXTextureProfileStats &stats = profile->getStats();

      F32 mb = ( stats.activeBytes / 1024.0f ) / 1024.0f;

      result.format( "%s %d %0.2f\n",
         profile->getName().c_str(),
         stats.activeCount,
         mb );

      profile = profile->getNext();
   }

   return result.end();
}

