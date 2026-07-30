//-----------------------------------------------------------------------------
// stbPlatformFont.h
//
// stb_truetype-backed PlatformFont implementation.
//
// This is a NEW PlatformFont implementation, added alongside the existing
// per-OS ones (win32/mac/linux), not a replacement for them. It differs from
// those in two ways:
//
//   1. It loads a .ttf/.otf file directly rather than asking the OS to
//      rasterize an installed face by name, so font behavior is identical
//      across platforms for any font shipped as an engine asset.
//
//   2. It supports two rasterization modes, selected at construction:
//        - Bitmap mode: behaves like the existing rasterizers. One glyph
//          bitmap is baked per exact pixel size; re-run this per DPI bucket,
//          same cost profile as today.
//        - SDF mode: each glyph is rasterized ONCE, as a signed distance
//          field, at a moderate reference pixel size. The renderer then
//          samples that single SDF bitmap at any device pixel size via a
//          threshold/smoothstep shader, so the same glyph data serves every
//          DPI scale without re-rasterizing. This is what makes fonts
//          resize "for free" per the design doc.
//-----------------------------------------------------------------------------

#ifndef _STBPLATFORMFONT_H_
#define _STBPLATFORMFONT_H_

#ifndef _PLATFORMFONT_H_
#include "platform/platformFont.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

// Forward-declared here rather than including <stb_truetype.h> in the
// header, so nothing that merely includes stbPlatformFont.h needs the stb
// include directory on its path — only stbPlatformFont.cc does.
struct stbtt_fontinfo;

/// stb_truetype-backed PlatformFont. See file header above for the two
/// rasterization modes this supports.
class StbPlatformFont : public PlatformFont
{
public:
   StbPlatformFont();
   virtual ~StbPlatformFont();

   // PlatformFont interface
   bool isValidChar(const UTF16 ch) const override;
   bool isValidChar(const UTF8 *str) const override;

   U32 getFontHeight() const override { return mFontHeight; }
   U32 getFontBaseLine() const override { return mBaseline; }

   PlatformFont::CharInfo &getCharInfo(const UTF16 ch) const override;
   PlatformFont::CharInfo &getCharInfo(const UTF8 *str) const override;

   bool create(const char *name, dsize_t size, U32 charset = TGE_ANSI_CHARSET) override;

   bool isSDFProvider() const override { return mSDFMode; }

   /// Loads a specific .ttf/.otf file and prepares rasterization at the
   /// given reference pixel size.
   bool createFromFile(const char *fontFilePath, dsize_t size, bool sdfMode);

private:
   /// Rasterizes one glyph (bitmap or SDF depending on mSDFMode)
   bool rasterizeGlyph(U32 codepoint, CharInfo &outInfo) const;

   /// Signed-distance-field rasterization. Produces an 8-bit SDF bitmap
   /// where 128 is "on the glyph edge," >128 is inside, <128 is outside
   bool rasterizeGlyphSDF(U32 codepoint, CharInfo &outInfo) const;

   /// Plain 8-bit coverage rasterization, equivalent in spirit to what the
   /// OS-backed rasterizers already produce, just sourced from stb_truetype
   /// instead of GDI/CoreText/FreeType.
   bool rasterizeGlyphBitmap(U32 codepoint, CharInfo &outInfo) const;

   stbtt_fontinfo  *mFontInfo;      ///< opaque to this header, defined in the .cc where <stb_truetype.h> is included
   U8              *mFontFileData;  ///< owned copy of the whole font file's bytes; stb_truetype reads directly from this buffer for the fontinfo's lifetime
   U32              mFontFileSize;

   F32   mScale;         ///< stbtt scale-for-pixel-height for mRefPixelSize
   U32   mRefPixelSize;  ///< reference pixel size rasterization was baked at (bitmap mode: the exact draw size; SDF mode: an internal reference only)
   U32   mFontHeight;
   U32   mBaseline;
   S32   mAscentPx;
   S32   mDescentPx;
   S32   mLineGapPx;

   bool  mSDFMode;
   F32   mSDFPixelRange;   ///< spread (in reference-rasterization pixels) baked into every glyph's distance field; passed through to CharInfo::sdfPixelRange so the render/shader side can derive a scale-correct AA band

   /// Mutable because getCharInfo() is const (matches the existing
   /// PlatformFont interface, which lazily rasterizes on first request) but
   /// needs to populate/extend this cache.
   mutable Vector<CharInfo> mCharInfoCache;
   mutable Vector<S32>      mCodepointToCacheIndex; // sparse map, -1 = not yet rasterized; sized to the highest codepoint requested so far

   CharInfo &getOrRasterize(U32 codepoint) const;
};

#endif // _STBPLATFORMFONT_H_
