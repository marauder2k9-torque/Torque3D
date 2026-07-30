//-----------------------------------------------------------------------------
// guiText.h
//
// GuiText -- core class any control uses to render text: justification,
// word-wrap, password masking, and dynamic shrink-to-fit sizing. Measures
// entirely off GFont::getCharInfo() (never GFont::wrapString()/
// getStrWidthPrecise()/getStrNWidth()), and submits through GuiRenderBatch.
// Units are DEVICE pixels throughout, matching GuiRenderBatch/onRender().
//-----------------------------------------------------------------------------

#ifndef _GUITEXT_H_
#define _GUITEXT_H_

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
#ifndef _GUISTYLEPROPERTIES_H_
#include "gui_refactor/core/guiStyleProperties.h"
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

class GuiRenderBatch;

/// Resolves a font at a candidate point size for GuiTextFitMode::ShrinkToFit.
/// Bind via Delegate<>::bind(object, &Class::method) -- see
/// GuiControlNew::renderText()/_resolveFontAtSizeForRenderText() for the
/// real usage.
typedef Delegate< Resource<GFont>(S32 size) > GuiTextFontAtSizeDelegate;

/// How GuiText handles text wider than its box.
enum GuiTextOverflowConsole : U8
{
   GuiTextOverflowConsole_Overflow = 0,
   GuiTextOverflowConsole_Clip,
   GuiTextOverflowConsole_Ellipsis
};

DefineEnumType(GuiTextOverflowConsole);

/// How dynamic font sizing searches for a size that fits, when enabled.
enum class GuiTextFitMode : U8
{
   Fixed = 0,    ///< Use the resolved style's font size as-is; no search.
   ShrinkToFit   ///< Shrink (never grow) in steps until the text fits the box or the min size is reached.
};

/// One resolved, ready-to-draw line -- output of GuiText::layout().
struct GuiTextLine
{
   Vector<UTF16> chars;   ///< Decoded (and mask-substituted, if applicable) UTF-16 for this line.
   S32 width;             ///< Measured width at the font size used for this layout.
   S32 height;            ///< Font's line height at that size.
   Point2I origin;        ///< Top-left device-pixel draw origin, already justified.
   bool wasTruncated;     ///< True if this line was shortened to fit (Clip/Ellipsis).

   /// True if this line was immediately preceded by a real, author-
   /// inserted '\n' in the source text (consumed during layout -- see
   /// GuiText::_splitHardLines()) rather than being a continuation
   /// produced purely by width-based wrapping. False for the very first
   /// line and for any line that's just a width-wrap continuation of the
   /// previous one. A flat-character-index consumer (e.g. a text-edit
   /// control mapping its cursor position to a line -- see
   /// GuiTextEditCtrlNew::_findCaretLineAndOffset()) needs this to know
   /// whether to account for a consumed '\n' byte that still exists in
   /// Number of source characters that were CONSUMED (dropped from every
   /// GuiTextLine::chars buffer, but still present in the ORIGINAL
   /// source text's flat coordinate space) at the boundary immediately
   /// before this line started. Zero for the very first line. Two
   /// distinct things can consume characters at a line boundary, and
   /// both need to be counted here for a flat-character-index consumer
   /// (e.g. a text-edit control mapping its cursor position to a line --
   /// see GuiTextEditCtrlNew::_findCaretLineAndOffset()/
   /// _findFlatIndexFromLineAndOffset()) to correctly reconstruct flat
   /// indices from line/offset pairs:
   ///   - A real, author-inserted '\n' (see GuiText::_splitHardLines())
   ///     -- always exactly 1 character.
   ///   - Width-based wrapping consuming the space(s) it broke on (see
   ///     GuiText::_wrapLines()'s "consume the space broken on, plus any
   ///     further consecutive spaces" step) -- can be more than 1 if
   ///     multiple consecutive spaces preceded the wrap point. This case
   ///     was NOT tracked at all before this field existed (only the
   ///     '\n' case was, via a plain followsHardBreak bool), which
   ///     silently produced an off-by-however-many-spaces flat index for
   ///     any content after an auto-wrapped line -- e.g. Ctrl+Right word
   ///     jump landing one character early into the first word of a
   ///     wrapped (but not explicitly newline-broken) line.
   U32 consumedCharsBeforeThisLine;

   GuiTextLine() : width(0), height(0), wasTruncated(false), consumedCharsBeforeThisLine(0) {}
};

/// Full resolved result of a GuiText::layout() call.
struct GuiTextLayoutResult
{
   Vector<GuiTextLine> lines;
   RectI blockBounds;        ///< Bounding box of every line combined.
   S32 fontSize;             ///< Font size actually used (may be smaller than authored under ShrinkToFit).
   Resource<GFont> font;     ///< The ACTUAL font this layout was measured/drawn against -- submit() must use this, not GuiText::mFont, since ShrinkToFit may have picked a smaller candidate.
   bool didOverflow;         ///< True if the text still didn't fit even at the smallest tried size/wrap.

   GuiTextLayoutResult() : fontSize(0), didOverflow(false) {}
};

/// Self-contained glyph-metrics helper built directly on GFont::getCharInfo()
/// -- deliberately not GFont::wrapString()/getStrWidthPrecise()/getStrNWidth(),
/// so there is exactly one width/advance formula used for both measuring
/// (wrap/fit decisions) and drawing.
struct GuiTextMetrics
{
   /// Sums character advances for [chars, chars+count), including trailing
   /// advance past the last glyph (letterSpacing/wordSpacing applied per
   /// GuiRenderBatch::_flushText()'s own formula).
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

   /// Like measure(), but stops at the last character that still fits
   /// maxWidth (never splits a glyph); outFitWidth receives that width.
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

/// Core text-handling class -- see file header. Normally owned one-per-
/// control (persistent, reconfigured via the setters below); can also be
/// used transiently for a one-off measurement.
class GuiText
{
public:

   GuiText();

   /// @name Configuration
   /// Each setter marks the cached layout dirty if the value actually
   /// changed; layout() re-derives lazily on next use.
   /// @{

   /// The literal source string; getText() always returns this unmasked
   /// even when setMasked(true) -- masking is a draw-time substitution.
   void setText(const String& text);
   const String& getText() const { return mText; }

   /// Font must already be resolved/loaded -- GuiText does no loading.
   void setFont(const Resource<GFont>& font);

   /// Box to lay out within, in device pixels; only the extent matters
   /// (submit() supplies the actual draw position separately).
   void setBoxExtent(const Point2I& extent);
   const Point2I& getBoxExtent() const { return mBoxExtent; }

   void setAlignHorizontal(GuiTextAlignHorizontal align);
   void setAlignVertical(GuiTextAlignVertical align);

   /// See GuiTextOverflow. Default is Overflow.
   void setOverflow(GuiTextOverflowConsole overflow);

   /// Enables word-wrap across multiple lines within the box width.
   /// When off (default), text is always a single line.
   void setWrap(bool wrap);

   /// Password/masked display: every character is substituted with
   /// maskChar before any measuring/wrapping/drawing -- real characters
   /// are never measured or handed to the render batch.
   void setMasked(bool masked, UTF16 maskChar = 0x2022);

   /// See GuiTextFitMode. Fixed by default.
   void setFitMode(GuiTextFitMode mode);

   /// Only used by ShrinkToFit: smallest size to try, and step size per
   /// search iteration.
   void setDynamicFontSizeRange(S32 minSize, S32 step = 1);

   /// Required for ShrinkToFit to actually search; see
   /// GuiTextFontAtSizeDelegate. If unset, ShrinkToFit behaves as Fixed.
   void setFontAtSizeDelegate(const GuiTextFontAtSizeDelegate& deleg) { mFontAtSizeDelegate = deleg; }
   /// @}

   /// Forces the next layout()/submit() to recompute even if nothing
   /// tracked here changed (e.g. the underlying GFont reloaded in place).
   void markDirty() { mLayoutDirty = true; }

   /// Resolves current configuration into drawable lines (cached; see
   /// mLayoutDirty). Safe to call purely for measurement.
   const GuiTextLayoutResult& layout();

   /// Pushes every line from layout()'s result into batch. The only
   /// place GuiText touches GuiRenderBatch.
   void submit(GuiRenderBatch& batch, const Point2I& basePos, const ColorI& color, S32 letterSpacing = 0, S32 wordSpacing = 0, S32 layer = 0);

   /// One-shot convenience wrapper: configures a stack-local GuiText and
   /// submits immediately. Prefer a persistent GuiText (above) for
   /// anything needing wrap/masking/dynamic sizing or repeated draws.
   static void renderSimple(GuiRenderBatch& batch, const Resource<GFont>& font, const Point2I& basePos, const Point2I& boxExtent, const char* text, const ColorI& color, GuiTextAlignHorizontal alignH, GuiTextAlignVertical alignV, S32 letterSpacing = 0, S32 wordSpacing = 0, S32 layer = 0);
   GuiTextOverflowConsole mOverflow;
   GuiTextAlignHorizontal mAlignH;
   GuiTextAlignVertical   mAlignV;

private:

   String mText;
   Resource<GFont> mFont;
   Point2I mBoxExtent;

   bool                   mWrap;

   bool   mMasked;
   UTF16  mMaskChar;

   GuiTextFitMode mFitMode;
   S32 mDynamicFontSizeMin;
   S32 mDynamicFontSizeStep;
   GuiTextFontAtSizeDelegate mFontAtSizeDelegate;

   bool mLayoutDirty;                  ///< True if anything above changed since the last layout().
   GuiTextLayoutResult mCachedResult;  ///< Result of the last layout() call.

   /// Decodes mText (or mMaskChar repeated per character, if masked)
   /// into UTF-16 once; every other step below works from this buffer.
   void _decodeSourceChars(Vector<UTF16>& outChars) const;

   /// Splits sourceChars at every embedded '\n' into hard line segments
   /// (the '\n' characters themselves are consumed, not kept in any
   /// segment). Always applied, regardless of mWrap -- an explicit
   /// newline in the source text is a line break even when width-based
   /// wrapping is off. Called once from _layoutAtSize() before
   /// _wrapLines() runs on each resulting segment.
   void _splitHardLines(const Vector<UTF16>& sourceChars, Vector< Vector<UTF16> >& outSegments) const;

   /// Splits sourceChars into word-wrapped lines no wider than maxWidth,
   /// breaking at the last space that fits (hard-breaking an over-wide
   /// word with no space at all). Assumes sourceChars has already been
   /// split on '\n' by _splitHardLines() -- called per hard-line
   /// segment, not on the raw decoded source directly. outConsumedBefore
   /// is parallel to outLines: outConsumedBefore[i] is how many
   /// characters (always spaces, here -- see this function's own
   /// "consume the space(s) broken on" step) were dropped from the
   /// source immediately before outLines[i] started. 0 for the first
   /// line produced (nothing consumed before the very start of the
   /// segment); a real, possibly->1 count for every wrapped
   /// continuation after it -- see GuiTextLine::consumedCharsBeforeThisLine's
   /// doc comment for why this has to be tracked precisely rather than
   /// assumed to always be exactly 1.
   void _wrapLines(GFont* font, const Vector<UTF16>& sourceChars, S32 maxWidth, S32 letterSpacing, S32 wordSpacing, Vector< Vector<UTF16> >& outLines, Vector<U32>& outConsumedBefore) const;

   /// Truncates a single line to fit maxWidth, appending "..." under
   /// Ellipsis mode if anything was cut.
   void _clipLine(GFont* font, const Vector<UTF16>& line, S32 maxWidth, S32 letterSpacing, S32 wordSpacing, Vector<UTF16>& outLine, bool& outTruncated) const;

   /// Core layout pass at a specific font/size -- used directly for
   /// Fixed mode and repeatedly by layout()'s ShrinkToFit search. Takes
   /// font by value (Resource<GFont>'s pointer-conversion operator is
   /// non-const) so it can be stashed into the returned result.
   GuiTextLayoutResult _layoutAtSize(Resource<GFont> font, S32 fontSize, S32 letterSpacing, S32 wordSpacing) const;
};

#endif // _GUITEXT_H_
