//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiText.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUITEXT_H_
#define _NEWGUITEXT_H_

#ifndef _MPOINT2_H_
#include "math/mPoint2.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif
#ifndef _UNICODE_H_
#include "core/strings/unicode.h"
#endif
#ifndef _UTIL_DELEGATE_H_
#include "core/util/delegate.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

class NewGuiRenderBatch;

/// Resolves a font at a candidate point size for NewGuiTextFitMode::ShrinkToFit.
/// Bind via Delegate<>::bind(object, &Class::method).
typedef Delegate< Resource<GFont>(S32 size) > NewGuiTextFontAtSizeDelegate;

/// How NewGuiText handles text wider than its box.
enum NewGuiTextOverflow : U8
{
   NewGuiTextOverflow_Overflow = 0,   ///< Text is never clipped or truncated.
   NewGuiTextOverflow_Clip,           ///< Text past the box edge is cut off.
   NewGuiTextOverflow_Ellipsis        ///< Text past the box edge is cut off and an ellipsis appended.
};
DefineEnumType(NewGuiTextOverflow);

/// How dynamic font sizing searches for a size that fits, when enabled.
enum class NewGuiTextFitMode : U8
{
   Fixed = 0,     ///< Use the configured font size as-is; no search.
   ShrinkToFit    ///< Shrink (never grow) in steps until the text fits the box or the min size is reached.
};

/// Horizontal text justification within the configured box.
enum class NewGuiTextAlignHorizontal : U8
{
   Left = 0,
   Center,
   Right,
};

/// Vertical text justification within the configured box.
enum class NewGuiTextAlignVertical : U8
{
   Top = 0,
   Middle,
   Bottom,
};

/// One resolved, ready-to-draw line - output of NewGuiText::layout().
struct NewGuiTextLine
{
   Vector<UTF16> chars;    ///< Decoded (and mask-substituted, if applicable) UTF-16 for this line.
   S32 width;              ///< Measured width, including configured spacing.
   S32 height;             ///< Font's line height at the size used.
   Point2I origin;         ///< Top-left device-pixel draw origin, already justified.
   bool wasTruncated;      ///< True if this line was shortened to fit (Clip/Ellipsis).
   U32 consumedCharsBeforeThisLine;   ///< True if this line followed a real, author-inserted '\n' rather than a wrap break.

   NewGuiTextLine() : width(0), height(0), wasTruncated(false), consumedCharsBeforeThisLine(0) {}
};

/// Full resolved result of a NewGuiText::layout() call.
struct NewGuiTextLayoutResult
{
   Vector<NewGuiTextLine> lines;
   RectI blockBounds;         ///< Bounding box of every line combined.
   S32 fontSize;              ///< Font size actually used (may be smaller than authored under ShrinkToFit).
   Resource<GFont> font;      ///< The actual font this layout was measured/drawn against - submit() must use this, not NewGuiText::mFont.
   bool didOverflow;          ///< True if the text still didn't fit even at the smallest tried size/wrap.

   NewGuiTextLayoutResult() : fontSize(0), didOverflow(false) {}
};

/// Glyph-metrics helper built on GFont::getCharInfo() - deliberately not
/// GFont::wrapString()/getStrWidthPrecise()/getStrNWidth(), so there is
/// exactly one width/advance formula used for both measuring and drawing.
struct NewGuiTextMetrics
{
   /// Sums character advances for [chars, chars+count), including letterSpacing/wordSpacing.
   static S32 measure(GFont* font, const UTF16* chars, U32 count, S32 letterSpacing, S32 wordSpacing)
   {
      if (!font || !chars || count == 0)
         return 0;

      S32 width = 0;
      for (U32 i = 0; i < count; i++)
      {
         const PlatformFont::CharInfo& ci = font->getCharInfo(chars[i]);
         width += ci.xIncrement + letterSpacing;
         if (chars[i] == (UTF16)' ')
            width += wordSpacing;
      }
      return width;
   }

   /// Like measure(), but stops at the last character that still fits maxWidth (never splits a glyph).
   /// @param outFitWidth Receives the width actually used.
   /// @return Number of characters that fit.
   static U32 fitCount(GFont* font, const UTF16* chars, U32 count, S32 maxWidth, S32 letterSpacing, S32 wordSpacing, S32& outFitWidth)
   {
      outFitWidth = 0;
      if (!font || !chars || count == 0 || maxWidth <= 0)
         return 0;

      S32 width = 0;
      for (U32 i = 0; i < count; i++)
      {
         const PlatformFont::CharInfo& ci = font->getCharInfo(chars[i]);
         S32 advance = ci.xIncrement + letterSpacing;
         if (chars[i] == (UTF16)' ')
            advance += wordSpacing;

         if (width + advance > maxWidth)
            return i;

         width += advance;
      }

      outFitWidth = width;
      return count;
   }
};

/// Text layout/measurement/drawing helper: justification, word-wrap,
/// overflow handling, password masking, and dynamic shrink-to-fit
/// sizing, all measured off GFont::getCharInfo() so layout and drawing
/// can never disagree. Normally owned one-per-control and reconfigured
/// via the setters below; can also be used transiently for a one-off
/// measurement. Units are device pixels throughout.
///
/// @code
/// NewGuiText text;
/// text.setFont( myFont );
/// text.setText( "Hello" );
/// text.setBoxExtent( Point2I( 200, 40 ) );
/// text.submit( batch, Point2I( 10, 10 ), ColorI( 255, 255, 255, 255 ) );
/// @endcode
class NewGuiText
{
public:

   NewGuiText();

   /// The literal source string; getText() always returns this unmasked - masking is a draw-time substitution.
   void setText(const String& text);
   const String& getText() const { return mText; }

   /// @param font Already-resolved/loaded font; NewGuiText does no loading.
   void setFont(const Resource<GFont>& font);

   /// Box to lay out within, in device pixels; only the extent matters.
   void setBoxExtent(const Point2I& extent);
   const Point2I& getBoxExtent() const { return mBoxExtent; }

   void setAlignHorizontal(NewGuiTextAlignHorizontal align);
   void setAlignVertical(NewGuiTextAlignVertical align);

   /// @param overflow How text wider than the box is handled. Default Overflow.
   void setOverflow(NewGuiTextOverflow overflow);

   /// Enables word-wrap across multiple lines within the box width. Off (single line) by default.
   void setWrap(bool wrap);
   bool getWrap() const { return mWrap; }

   /// Password/masked display: every character is substituted with maskChar before any measuring/wrapping/drawing.
   /// @param masked True to enable masking.
   /// @param maskChar Character to substitute.
   void setMasked(bool masked, UTF16 maskChar = 0x2022);

   /// @param mode Fit mode. Fixed by default.
   void setFitMode(NewGuiTextFitMode mode);

   /// Only used by ShrinkToFit.
   /// @param minSize Smallest size to try.
   /// @param step Step size per search iteration.
   void setDynamicFontSizeRange(S32 minSize, S32 step = 1);

   /// Required for ShrinkToFit to actually search; if unset, ShrinkToFit behaves as Fixed.
   void setFontAtSizeDelegate(const NewGuiTextFontAtSizeDelegate& deleg) { mFontAtSizeDelegate = deleg; }

   /// Extra pixels applied after every character (letterSpacing) and additionally after every space (wordSpacing).
   /// Configured state, not a submit()-time parameter, so layout()'s wrap/clip decisions and submit()'s
   /// actual glyph placement always agree.
   void setSpacing(S32 letterSpacing, S32 wordSpacing);

   /// Forces the next layout()/submit() to recompute even if nothing tracked here changed.
   void markDirty() { mLayoutDirty = true; }

   /// Resolves current configuration into drawable lines (cached). Safe to call purely for measurement.
   /// @return The resolved layout.
   const NewGuiTextLayoutResult& layout();

   /// Pushes every line from layout()'s result into batch - the only place NewGuiText touches NewGuiRenderBatch.
   /// @param batch Render batch to draw into.
   /// @param basePos Draw origin, in device pixels.
   /// @param color Text color.
   /// @param layer Paint-order layer.
   void submit(NewGuiRenderBatch& batch, const Point2I& basePos, const ColorI& color, S32 layer = 0);

   /// One-shot convenience wrapper: configures a stack-local NewGuiText and submits immediately.
   /// Prefer a persistent NewGuiText for anything needing wrap/masking/dynamic sizing or repeated draws.
   static void renderSimple(NewGuiRenderBatch& batch, const Resource<GFont>& font, const Point2I& basePos, const Point2I& boxExtent, const char* text, const ColorI& color, NewGuiTextAlignHorizontal alignH, NewGuiTextAlignVertical alignV, S32 letterSpacing = 0, S32 wordSpacing = 0, S32 layer = 0);

   NewGuiTextOverflow mOverflow;
   NewGuiTextAlignHorizontal mAlignH;
   NewGuiTextAlignVertical   mAlignV;

private:

   String mText;
   Resource<GFont> mFont;
   Point2I mBoxExtent;

   bool mWrap;

   bool  mMasked;
   UTF16 mMaskChar;

   S32 mLetterSpacing;
   S32 mWordSpacing;

   NewGuiTextFitMode mFitMode;
   S32 mDynamicFontSizeMin;
   S32 mDynamicFontSizeStep;
   NewGuiTextFontAtSizeDelegate mFontAtSizeDelegate;

   bool mLayoutDirty;
   NewGuiTextLayoutResult mCachedResult;

   /// Decodes mText (or mMaskChar repeated per character, if masked) into UTF-16 once.
   void _decodeSourceChars(Vector<UTF16>& outChars) const;

   /// Splits sourceChars at every embedded '\n' into hard line segments. Always applied, regardless of mWrap.
   void _splitHardLines(const Vector<UTF16>& sourceChars, Vector< Vector<UTF16> >& outSegments) const;

   /// Splits sourceChars into word-wrapped lines no wider than maxWidth, breaking at the last
   /// fitting space (hard-breaking an over-wide word with no space). Assumes sourceChars has
   /// already been split on '\n'. outConsumedBefore is parallel to outLines.
   void _wrapLines(GFont* font, const Vector<UTF16>& sourceChars, S32 maxWidth, S32 letterSpacing, S32 wordSpacing, Vector< Vector<UTF16> >& outLines, Vector<U32>& outConsumedBefore) const;

   /// Truncates a single line to fit maxWidth, appending "..." under Ellipsis mode if anything was cut.
   void _clipLine(GFont* font, const Vector<UTF16>& line, S32 maxWidth, S32 letterSpacing, S32 wordSpacing, Vector<UTF16>& outLine, bool& outTruncated) const;

   /// Core layout pass at a specific font/size - used directly for Fixed mode and repeatedly by
   /// layout()'s ShrinkToFit search. Always measures/wraps/clips with mLetterSpacing/mWordSpacing.
   NewGuiTextLayoutResult _layoutAtSize(Resource<GFont> font, S32 fontSize) const;
};

#endif // _NEWGUITEXT_H_
