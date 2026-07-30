//-----------------------------------------------------------------------------
// stbPlatformFont.cc
//
// See stbPlatformFont.h for the design rationale (bitmap vs SDF mode, why
// this exists alongside the per-OS PlatformFont implementations rather than
// replacing them).
//-----------------------------------------------------------------------------

#include "platform/stbPlatformFont.h"

#include "core/stream/fileStream.h"
#include "console/console.h"
#include "core/util/safeDelete.h"
#include "math/mMathFn.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

//-----------------------------------------------------------------------------
// Tuning constants
//-----------------------------------------------------------------------------

namespace
{
   /// Reference pixel size SDF glyphs are rasterized at internally,
   /// independent of whatever pixel size was requested at construction.
   const U32 kSDFReferencePixelSize = 48;
   /// Spread of the distance field, in reference-rasterization pixels either
   /// side of the glyph edge.
   const F32 kSDFPixelRange = 4.0f;
   /// Padding, in reference pixels, stbtt_GetGlyphSDF applies around the
   /// glyph bitmap so the distance field has room to fall off cleanly.
   const S32 kSDFPadding = 4;
   /// stbtt's mid-value for "exactly on the glyph edge" in its 0-255 SDF
   /// output convention.
   const U8  kSDFOnEdgeValue = 128;
}

//-----------------------------------------------------------------------------

namespace
{
   /// Minimal, self-contained UTF-8 -> single-codepoint decoder for the
   /// UTF8-string overloads below. 
   U32 decodeFirstUTF8Codepoint(const UTF8 *str)
   {
      if (!str || !*str)
         return 0;

      const U8 b0 = (U8)str[0];

      if (b0 < 0x80)
         return b0;

      if ((b0 & 0xE0) == 0xC0 && str[1])
         return ((b0 & 0x1F) << 6) | ((U8)str[1] & 0x3F);

      if ((b0 & 0xF0) == 0xE0 && str[1] && str[2])
         return ((b0 & 0x0F) << 12) | (((U8)str[1] & 0x3F) << 6) | ((U8)str[2] & 0x3F);

      if ((b0 & 0xF8) == 0xF0 && str[1] && str[2] && str[3])
         return ((b0 & 0x07) << 18) | (((U8)str[1] & 0x3F) << 12) | (((U8)str[2] & 0x3F) << 6) | ((U8)str[3] & 0x3F);

      return b0; // malformed lead byte -- treat as Latin-1 fallback rather than fail outright
   }
}

//-----------------------------------------------------------------------------

StbPlatformFont::StbPlatformFont()
   : mFontInfo(nullptr),
     mFontFileData(nullptr),
     mFontFileSize(0),
     mScale(0.0f),
     mRefPixelSize(0),
     mFontHeight(0),
     mBaseline(0),
     mAscentPx(0),
     mDescentPx(0),
     mLineGapPx(0),
     mSDFMode(false),
     mSDFPixelRange(0.0f)
{
   VECTOR_SET_ASSOCIATION(mCharInfoCache);
   VECTOR_SET_ASSOCIATION(mCodepointToCacheIndex);
}

StbPlatformFont::~StbPlatformFont()
{
   // bitmapData is deliberately NOT freed here.
   // it should be freed from gfont
   SAFE_DELETE_ARRAY(mFontFileData);

   if (mFontInfo)
      delete mFontInfo;
}

//-----------------------------------------------------------------------------

bool StbPlatformFont::create(const char *name, dsize_t size, U32 charset)
{
   // This overload exists only to satisfy the abstract PlatformFont
   // interface (createPlatformFont-style construction by face NAME). 
   return createFromFile(name, size, false);
}

//-----------------------------------------------------------------------------

bool StbPlatformFont::createFromFile(const char *fontFilePath, dsize_t size, bool sdfMode)
{
   FileStream stream;
   if (!stream.open(fontFilePath, Torque::FS::File::Read))
   {
      Con::errorf("StbPlatformFont::createFromFile - could not open font file '%s'", fontFilePath);
      return false;
   }

   const U32 fileSize = (U32)stream.getStreamSize();
   if (fileSize == 0)
   {
      Con::errorf("StbPlatformFont::createFromFile - font file '%s' is empty", fontFilePath);
      return false;
   }

   U8 *data = new U8[fileSize];
   if (!stream.read(fileSize, data))
   {
      Con::errorf("StbPlatformFont::createFromFile - failed reading font file '%s'", fontFilePath);
      delete[] data;
      return false;
   }
   stream.close();

   mFontFileData = data;
   mFontFileSize = fileSize;

   mFontInfo = new stbtt_fontinfo;
   dMemset(mFontInfo, 0, sizeof(stbtt_fontinfo)); // stbtt_fontinfo is a plain C struct with no constructor; stbtt_InitFont expects clean state, not whatever `new` happened to leave behind
   const int offset = stbtt_GetFontOffsetForIndex(mFontFileData, 0);
   if (offset < 0 || !stbtt_InitFont(mFontInfo, mFontFileData, offset))
   {
      Con::errorf("StbPlatformFont::createFromFile - stbtt_InitFont failed for '%s' (not a valid ttf/otf?)", fontFilePath);
      delete mFontInfo;
      mFontInfo = nullptr;
      SAFE_DELETE_ARRAY(mFontFileData);
      return false;
   }

   mSDFMode = sdfMode;
   mSDFPixelRange = sdfMode ? kSDFPixelRange : 0.0f;

   // Reference pixel size: in bitmap mode this IS the exact size every
   // glyph is baked at (equivalent to the OS-backed rasterizers' behavior).
   mRefPixelSize = sdfMode ? kSDFReferencePixelSize : (U32)size;

   mScale = stbtt_ScaleForPixelHeight(mFontInfo, (F32)mRefPixelSize);

   int ascent, descent, lineGap;
   stbtt_GetFontVMetrics(mFontInfo, &ascent, &descent, &lineGap);

   mAscentPx  = (S32)mRoundToNearest(ascent  * mScale);
   mDescentPx = (S32)mRoundToNearest(-descent * mScale); // stb's descent is negative; PlatformFont convention elsewhere treats descent as a positive extent below baseline
   mLineGapPx = (S32)mRoundToNearest(lineGap * mScale);

   mBaseline   = (U32)mAscentPx;
   mFontHeight = (U32)(mAscentPx + mDescentPx + mLineGapPx);

   return true;
}

//-----------------------------------------------------------------------------

bool StbPlatformFont::isValidChar(const UTF16 ch) const
{
   if (!mFontInfo)
      return false;

   // \n, \r, and similar control characters are intentionally excluded --
   // matches the -1 bitmapIndex convention documented on CharInfo.
   if (ch < 0x20)
      return false;

   return stbtt_FindGlyphIndex(mFontInfo, (int)ch) != 0;
}

bool StbPlatformFont::isValidChar(const UTF8 *str) const
{
   if (!str || !*str)
      return false;

   U32 codepoint = decodeFirstUTF8Codepoint(str);
   return isValidChar((UTF16)codepoint);
}

//-----------------------------------------------------------------------------

PlatformFont::CharInfo &StbPlatformFont::getCharInfo(const UTF16 ch) const
{
   return getOrRasterize((U32)ch);
}

PlatformFont::CharInfo &StbPlatformFont::getCharInfo(const UTF8 *str) const
{
   U32 codepoint = decodeFirstUTF8Codepoint(str);
   return getOrRasterize(codepoint);
}

//-----------------------------------------------------------------------------

PlatformFont::CharInfo &StbPlatformFont::getOrRasterize(U32 codepoint) const
{
   static CharInfo sInvalidCharInfo; // bitmapIndex defaults appropriately via CharInfo's own ctor

   if (codepoint >= mCodepointToCacheIndex.size())
   {
      const U32 oldSize = mCodepointToCacheIndex.size();
      mCodepointToCacheIndex.setSize(codepoint + 1);
      for (U32 i = oldSize; i <= codepoint; i++)
         mCodepointToCacheIndex[i] = -1;
   }

   S32 cacheIdx = mCodepointToCacheIndex[codepoint];
   if (cacheIdx != -1)
      return mCharInfoCache[cacheIdx];

   CharInfo info;
   const bool ok = rasterizeGlyph(codepoint, info);

   if (!ok)
   {
      // Cache the failure too, so we don't retry rasterizing an invalid
      // codepoint every single time it's requested (matches the spirit of
      // the OS-backed rasterizers' remap-table-of--1 behavior in GFont).
      mCodepointToCacheIndex[codepoint] = -2;
      return sInvalidCharInfo;
   }

   mCharInfoCache.push_back(info);
   cacheIdx = mCharInfoCache.size() - 1;
   mCodepointToCacheIndex[codepoint] = cacheIdx;

   return mCharInfoCache[cacheIdx];
}

//-----------------------------------------------------------------------------

bool StbPlatformFont::rasterizeGlyph(U32 codepoint, CharInfo &outInfo) const
{
   return mSDFMode ? rasterizeGlyphSDF(codepoint, outInfo)
                    : rasterizeGlyphBitmap(codepoint, outInfo);
}

//-----------------------------------------------------------------------------

bool StbPlatformFont::rasterizeGlyphBitmap(U32 codepoint, CharInfo &outInfo) const
{
   int glyphIndex = stbtt_FindGlyphIndex(mFontInfo, (int)codepoint);
   if (glyphIndex == 0)
      return false;

   int width, height, xoff, yoff;
   U8 *bitmap = stbtt_GetGlyphBitmap(mFontInfo, mScale, mScale, glyphIndex, &width, &height, &xoff, &yoff);

   int advanceWidth, leftSideBearing;
   stbtt_GetGlyphHMetrics(mFontInfo, glyphIndex, &advanceWidth, &leftSideBearing);
   (void)leftSideBearing; // unused: horizontal placement comes from xOrigin (bitmap-space offset), not lsb

   outInfo.width       = (U32)width;
   outInfo.height      = (U32)height;
   outInfo.xOrigin     = xoff;
   outInfo.yOrigin     = -yoff; // stb's yoff is the top-left offset (negative = above baseline);
   outInfo.xIncrement  = (S32)mRoundToNearest(advanceWidth * mScale);
   outInfo.rasterMode  = PlatformFont::GlyphRasterMode::Bitmap;
   outInfo.sdfPixelRange = 0.0f;

   if (width > 0 && height > 0)
   {
      // GFont::addBitmap copies these bytes into its own texture-sheet
      U8 *ownedCopy = new U8[width * height];
      dMemcpy(ownedCopy, bitmap, width * height);
      outInfo.bitmapData = ownedCopy;
      stbtt_FreeBitmap(bitmap, nullptr);
   }
   else
   {
      outInfo.bitmapData = nullptr;
      if (bitmap)
         stbtt_FreeBitmap(bitmap, nullptr);
   }

   outInfo.bitmapIndex = 0; // real sheet index is assigned later, by GFont::addBitmap, same as every other PlatformFont implementation

   return true;
}

//-----------------------------------------------------------------------------

bool StbPlatformFont::rasterizeGlyphSDF(U32 codepoint, CharInfo &outInfo) const
{
   int glyphIndex = stbtt_FindGlyphIndex(mFontInfo, (int)codepoint);
   if (glyphIndex == 0)
      return false;

   int width, height, xoff, yoff;
   U8 *sdfBitmap = stbtt_GetGlyphSDF(
      mFontInfo, mScale, glyphIndex,
      kSDFPadding, kSDFOnEdgeValue, kSDFPixelRange,
      &width, &height, &xoff, &yoff);

   int advanceWidth, leftSideBearing;
   stbtt_GetGlyphHMetrics(mFontInfo, glyphIndex, &advanceWidth, &leftSideBearing);
   (void)leftSideBearing;

   outInfo.width       = (U32)width;
   outInfo.height       = (U32)height;
   outInfo.xOrigin      = xoff;
   outInfo.yOrigin      = -yoff; // see rasterizeGlyphBitmap for sign-convention note
   outInfo.xIncrement   = (S32)mRoundToNearest(advanceWidth * mScale);
   outInfo.rasterMode   = PlatformFont::GlyphRasterMode::SDF;
   outInfo.sdfPixelRange = kSDFPixelRange;

   if (width > 0 && height > 0)
   {
      U8 *ownedCopy = new U8[width * height];
      dMemcpy(ownedCopy, sdfBitmap, width * height);
      outInfo.bitmapData = ownedCopy;
      stbtt_FreeSDF(sdfBitmap, nullptr);
   }
   else
   {
      outInfo.bitmapData = nullptr;
      if (sdfBitmap)
         stbtt_FreeSDF(sdfBitmap, nullptr);
   }

   outInfo.bitmapIndex = 0;

   return true;
}

//-----------------------------------------------------------------------------
// Factory
//-----------------------------------------------------------------------------

PlatformFont *createStbPlatformFont(const char *fontFilePath, dsize_t size, U32 charset, bool sdfMode)
{
   StbPlatformFont *font = new StbPlatformFont;
   if (!font->createFromFile(fontFilePath, size, sdfMode))
   {
      delete font;
      return nullptr;
   }
   return font;
}
