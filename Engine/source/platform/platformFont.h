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

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _PLATFORMFONT_H_
#define _PLATFORMFONT_H_

// Charsets for fonts

// [tom, 7/27/2005] These are intended to map to their Win32 equivalents. This
// enumeration may require changes to accommodate other platforms.
enum FontCharset
{
    TGE_ANSI_CHARSET = 0,
    TGE_SYMBOL_CHARSET,
    TGE_SHIFTJIS_CHARSET,
    TGE_HANGEUL_CHARSET,
    TGE_HANGUL_CHARSET,
    TGE_GB2312_CHARSET,
    TGE_CHINESEBIG5_CHARSET,
    TGE_OEM_CHARSET,
    TGE_JOHAB_CHARSET,
    TGE_HEBREW_CHARSET,
    TGE_ARABIC_CHARSET,
    TGE_GREEK_CHARSET,
    TGE_TURKISH_CHARSET,
    TGE_VIETNAMESE_CHARSET,
    TGE_THAI_CHARSET,
    TGE_EASTEUROPE_CHARSET,
    TGE_RUSSIAN_CHARSET,
    TGE_MAC_CHARSET,
    TGE_BALTIC_CHARSET
};

extern const char *getCharSetName(const U32 charSet);

class PlatformFont
{
public:
   /// Rasterization mode a CharInfo's bitmapData was produced in.
   /// Existing GDI/CoreText/FreeType-backed PlatformFont implementations
   /// only ever produce Bitmap; SDF is produced by StbPlatformFont when
   /// its font asset requests SDF mode (see StbPlatformFont.h).
   enum class GlyphRasterMode
   {
      Bitmap = 0,   ///< bitmapData is 8-bit coverage, baked at this exact pixel size
      SDF    = 1,   ///< bitmapData is 8-bit signed distance field, sampled at ANY size via threshold
   };

   struct CharInfo
   {
      S16 bitmapIndex;     ///< @note -1 indicates character is NOT to be
                           ///        rendered, i.e., \n, \r, etc.
      U32  xOffset;        ///< x offset into bitmap sheet
      U32  yOffset;        ///< y offset into bitmap sheet
      U32  width;          ///< width of character (pixels)
      U32  height;         ///< height of character (pixels)
      S32  xOrigin;
      S32  yOrigin;
      S32  xIncrement;
      U8  *bitmapData;     ///< temp storage for bitmap data

      /// @name SDF metadata
      /// Defaulted to Bitmap/0 so every existing PlatformFont implementation
      /// (win32/mac/linux GDI-CoreText-FreeType) is unaffected
      GlyphRasterMode rasterMode = GlyphRasterMode::Bitmap;
      F32 sdfPixelRange = 0.0f;  ///< distance-field spread, in source-rasterization pixels, used by the
                                  ///< threshold shader to derive a stable-width outline/AA band at any draw scale
      /// @}

      CharInfo()
         : bitmapIndex(0), xOffset(0), yOffset(0), width(0), height(0),
           xOrigin(0), yOrigin(0), xIncrement(0), bitmapData(nullptr),
           rasterMode(GlyphRasterMode::Bitmap), sdfPixelRange(0.0f) {}
   };
   
   virtual ~PlatformFont() {}
   
   /// Is the specified character valid for rendering?
   virtual bool isValidChar(const UTF16 ch) const = 0;
   virtual bool isValidChar(const UTF8 *str) const = 0;

   virtual U32 getFontHeight() const = 0;
   virtual U32 getFontBaseLine() const = 0;

   virtual PlatformFont::CharInfo &getCharInfo(const UTF16 ch) const = 0;
   virtual PlatformFont::CharInfo &getCharInfo(const UTF8 *str) const = 0;

   /// This is just for createPlatformFont to call.
   ///
   /// @todo Rethink this so we don't have a private public.
   virtual bool create( const char *name, dsize_t size, U32 charset = TGE_ANSI_CHARSET ) = 0;
   static void enumeratePlatformFonts( Vector<StringTableEntry>& fonts, UTF16* fontFamily = NULL );

   /// Does this rasterizer produce SDF glyphs?
   virtual bool isSDFProvider() const { return false; }
};

extern PlatformFont *createPlatformFont(const char *name, dsize_t size, U32 charset = TGE_ANSI_CHARSET);

/// Creates an stb_truetype-backed PlatformFont loading a specific .ttf/.otf
/// file directly (not an OS-installed face lookup like createPlatformFont).
/// @param  fontFilePath  path to a .ttf/.otf file, resolved the same way any
///                       other engine asset path is resolved
/// @param  size          requested pixel size. In SDF mode this only affects
///                       the *hinted-metrics reference size* the rasterizer
///                       bakes at — the resulting glyphs are then sampled at
///                       any device pixel size by the renderer, so re-calling
///                       this per DPI bucket is unnecessary (though harmless)
///                       for SDF fonts. In bitmap mode it behaves exactly
///                       like createPlatformFont: one exact baked size.
/// @param  charset       kept for signature parity with createPlatformFont;
///                       stb_truetype works from Unicode codepoints directly
///                       and does not need a Win32-style charset table, so
///                       this is accepted but unused.
/// @param  sdfMode       false = Bitmap rasterization (drop-in equivalent of
///                       today's behavior, sourced from the ttf file instead
///                       of the OS). true = SDF rasterization (see
///                       GlyphRasterMode::SDF above).
extern PlatformFont *createStbPlatformFont(const char *fontFilePath, dsize_t size, U32 charset = TGE_ANSI_CHARSET, bool sdfMode = false);

#endif // _PLATFORMFONT_H_
