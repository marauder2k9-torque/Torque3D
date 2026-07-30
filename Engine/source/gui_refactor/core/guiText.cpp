//-----------------------------------------------------------------------------
// guiText.cpp
//
// See guiText.h. Layout is: decode/mask -> wrap (optional) -> clip/ellipsis
// (optional) -> measure each line -> justify. ShrinkToFit re-runs the whole
// pass at successively smaller candidate sizes via mFontAtSizeDelegate.
//-----------------------------------------------------------------------------

#include "gui_refactor/core/guiText.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "gui_refactor/core/guiStyle.h"
#include "core/strings/unicode.h"

//-----------------------------------------------------------------------------
//    Console enum registration -- see GuiTextOverflowConsole (guiText.h).
//-----------------------------------------------------------------------------

ImplementEnumType(GuiTextOverflowConsole,
   "How a GuiText/GuiLabelCtrlNew handles text wider than its configured box.\n\n")
{
   GuiTextOverflowConsole_Overflow, "overflow",
      "Text is measured/positioned as if the box were unbounded; nothing is wrapped, clipped, or shortened."
},
{ GuiTextOverflowConsole_Clip, "clip",
   "Text is truncated (no wrap) to fit the box width, with no \"...\" appended." },
{ GuiTextOverflowConsole_Ellipsis, "ellipsis",
   "Text is truncated (no wrap) to fit the box width, with a trailing \"...\" appended if anything was cut." },
   EndImplementEnumType;

//=============================================================================
//    Construction / configuration.
//=============================================================================

GuiText::GuiText()
   : mBoxExtent(0, 0),
   mAlignH(GuiTextAlignHorizontal::GuiTextAlignHorizontal_Left),
   mAlignV(GuiTextAlignVertical::GuiTextAlignVertical_Middle),
   mOverflow(GuiTextOverflowConsole::GuiTextOverflowConsole_Overflow),
   mWrap(false),
   mMasked(false),
   mMaskChar(0x2022),
   mFitMode(GuiTextFitMode::Fixed),
   mDynamicFontSizeMin(6),
   mDynamicFontSizeStep(1),
   mLayoutDirty(true)
{
}

//-----------------------------------------------------------------------------

void GuiText::setText(const String& text)
{
   if (String::compare(mText.c_str(), text.c_str()) == 0)
      return;

   mText = text;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setFont(const Resource<GFont>& font)
{
   if (mFont == font)
      return;

   mFont = font;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setBoxExtent(const Point2I& extent)
{
   if (mBoxExtent == extent)
      return;

   mBoxExtent = extent;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setAlignHorizontal(GuiTextAlignHorizontal align)
{
   if (mAlignH == align)
      return;

   mAlignH = align;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setAlignVertical(GuiTextAlignVertical align)
{
   if (mAlignV == align)
      return;

   mAlignV = align;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setOverflow(GuiTextOverflowConsole overflow)
{
   if (mOverflow == overflow)
      return;

   mOverflow = overflow;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setWrap(bool wrap)
{
   if (mWrap == wrap)
      return;

   mWrap = wrap;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setMasked(bool masked, UTF16 maskChar)
{
   if (mMasked == masked && mMaskChar == maskChar)
      return;

   mMasked = masked;
   mMaskChar = maskChar;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setFitMode(GuiTextFitMode mode)
{
   if (mFitMode == mode)
      return;

   mFitMode = mode;
   mLayoutDirty = true;
}

//-----------------------------------------------------------------------------

void GuiText::setDynamicFontSizeRange(S32 minSize, S32 step)
{
   const S32 clampedStep = getMax(step, 1); // avoid an infinite search loop

   if (mDynamicFontSizeMin == minSize && mDynamicFontSizeStep == clampedStep)
      return;

   mDynamicFontSizeMin = minSize;
   mDynamicFontSizeStep = clampedStep;
   mLayoutDirty = true;
}

//=============================================================================
//    Decoding / masking.
//=============================================================================

// Decodes mText to UTF-16 once; if masked, substitutes mMaskChar per decoded
// character (not per raw byte, so multi-byte UTF-8 doesn't leak into glyph count).
void GuiText::_decodeSourceChars(Vector<UTF16>& outChars) const
{
   outChars.clear();

   if (!mText.isNotEmpty())
      return;

   const U32 byteLen = mText.length();
   Vector<UTF16> decoded;
   decoded.setSize(byteLen + 1);
   convertUTF8toUTF16N(mText.c_str(), decoded.address(), byteLen + 1);

   if (!mMasked)
   {
      for (U32 i = 0; i < byteLen + 1 && decoded[i] != 0; i++)
         outChars.push_back(decoded[i]);
      return;
   }

   for (U32 i = 0; i < byteLen + 1 && decoded[i] != 0; i++)
      outChars.push_back(mMaskChar);
}

//=============================================================================
//    Line breaking.
//=============================================================================

// Splits sourceChars at every embedded '\n' into hard line segments. This is
// NOT width-driven wrapping (see _wrapLines() below for that) -- an explicit
// newline in the source text is always honored as a line break, regardless
// of mWrap/box width, the same way any real text widget treats an
// author-inserted line break as distinct from where a renderer happens to
// wrap for width. The '\n' characters themselves are consumed (not included
// in any output segment) -- GFont has no real glyph for a raw control
// character (see getCharInfo()'s "no remap info for this character"
// contract), so handing '\n' through to width-based measuring/drawing the
// way every other character is treated produces whatever the font's
// missing-glyph fallback looks like (a filled box), rather than an actual
// line break -- which was invisible before this function existed, since
// nothing anywhere in this class ever looked for '\n' at all.
void GuiText::_splitHardLines(const Vector<UTF16>& sourceChars, Vector< Vector<UTF16> >& outSegments) const
{
   outSegments.clear();

   const U32 total = sourceChars.size();
   U32 segStart = 0;

   for (U32 i = 0; i < total; i++)
   {
      if (sourceChars[i] == (UTF16)'\n')
      {
         Vector<UTF16> segment;
         const U32 segLen = i - segStart;
         segment.setSize(segLen);
         if (segLen > 0)
            dMemcpy(segment.address(), sourceChars.address() + segStart, segLen * sizeof(UTF16));
         outSegments.push_back(segment);

         segStart = i + 1;
      }
   }

   // Final segment (everything after the last '\n', or the whole string if
   // there was none at all).
   Vector<UTF16> lastSegment;
   const U32 lastLen = total - segStart;
   lastSegment.setSize(lastLen);
   if (lastLen > 0)
      dMemcpy(lastSegment.address(), sourceChars.address() + segStart, lastLen * sizeof(UTF16));
   outSegments.push_back(lastSegment);
}

//-----------------------------------------------------------------------------

// Splits sourceChars into lines no wider than maxWidth, breaking at the last
// space that fits (hard-breaking an over-wide word with no space at all).
// Assumes sourceChars has ALREADY been split on '\n' by _splitHardLines()
// (i.e. contains no embedded newlines itself) -- called once per hard-line
// segment from _layoutAtSize(), not on the raw decoded source directly.
void GuiText::_wrapLines(GFont* font, const Vector<UTF16>& sourceChars, S32 maxWidth, S32 letterSpacing, S32 wordSpacing, Vector< Vector<UTF16> >& outLines, Vector<U32>& outConsumedBefore) const
{
   outLines.clear();
   outConsumedBefore.clear();

   if (sourceChars.empty())
      return;

   const U32 total = sourceChars.size();
   U32 lineStart = 0;
   U32 consumedBeforeNextLine = 0; // nothing consumed before the very first line

   while (lineStart < total)
   {
      const UTF16* remaining = sourceChars.address() + lineStart;
      const U32 remainingCount = total - lineStart;

      S32 fitWidth = 0;
      U32 fitCount = GuiTextMetrics::fitCount(font, remaining, remainingCount, maxWidth, letterSpacing, wordSpacing, fitWidth);

      if (fitCount >= remainingCount)
      {
         Vector<UTF16> line;
         line.setSize(remainingCount);
         dMemcpy(line.address(), remaining, remainingCount * sizeof(UTF16));
         outLines.push_back(line);
         outConsumedBefore.push_back(consumedBeforeNextLine);
         break;
      }

      // Find the last space at-or-before fitCount to break on a word boundary.
      U32 breakAt = fitCount;
      bool foundSpace = false;
      if (fitCount > 0)
      {
         for (U32 i = fitCount; i > 0; i--)
         {
            if (remaining[i - 1] == (UTF16)' ')
            {
               breakAt = i - 1;
               foundSpace = true;
               break;
            }
         }
      }

      if (!foundSpace)
      {
         // No space in range -- hard-break an over-wide word. Nothing
         // consumed at this boundary (the break falls mid-word, not on
         // whitespace), so the NEXT line's consumedBefore is 0.
         breakAt = getMax(fitCount, (U32)1);
         breakAt = getMin(breakAt, remainingCount);

         Vector<UTF16> line;
         line.setSize(breakAt);
         dMemcpy(line.address(), remaining, breakAt * sizeof(UTF16));
         outLines.push_back(line);
         outConsumedBefore.push_back(consumedBeforeNextLine);

         lineStart += breakAt;
         consumedBeforeNextLine = 0;
         continue;
      }

      Vector<UTF16> line;
      line.setSize(breakAt);
      if (breakAt > 0)
         dMemcpy(line.address(), remaining, breakAt * sizeof(UTF16));
      outLines.push_back(line);
      outConsumedBefore.push_back(consumedBeforeNextLine);

      // Consume the space broken on, plus any further consecutive spaces
      // -- COUNTED this time (see this function's own doc comment in
      // guiText.h on why silently discarding this count was the actual
      // bug), so the line about to be produced next reports accurately
      // how many source characters were dropped immediately before it.
      const U32 preConsumeLineStart = lineStart;
      lineStart += breakAt;
      while (lineStart < total && sourceChars[lineStart] == (UTF16)' ')
         lineStart++;
      consumedBeforeNextLine = lineStart - (preConsumeLineStart + breakAt);
   }
}

//-----------------------------------------------------------------------------

// Truncates a single line to fit maxWidth; appends "..." under Ellipsis mode.
void GuiText::_clipLine(GFont* font, const Vector<UTF16>& line, S32 maxWidth, S32 letterSpacing, S32 wordSpacing, Vector<UTF16>& outLine, bool& outTruncated) const
{
   outTruncated = false;

   S32 fullWidth = 0;
   U32 fullFit = GuiTextMetrics::fitCount(font, line.address(), line.size(), maxWidth, letterSpacing, wordSpacing, fullWidth);

   if (fullFit >= line.size())
   {
      outLine = line;
      return;
   }

   outTruncated = true;

   if (mOverflow != GuiTextOverflowConsole::GuiTextOverflowConsole_Ellipsis)
   {
      outLine.setSize(fullFit);
      if (fullFit > 0)
         dMemcpy(outLine.address(), line.address(), fullFit * sizeof(UTF16));
      return;
   }

   const UTF16 kEllipsis[3] = { (UTF16)'.', (UTF16)'.', (UTF16)'.' };
   S32 ellipsisWidth = GuiTextMetrics::measure(font, kEllipsis, 3, letterSpacing, wordSpacing);

   const S32 availableForText = maxWidth - ellipsisWidth;

   U32 keepCount = 0;
   if (availableForText > 0)
   {
      S32 keptWidth = 0;
      keepCount = GuiTextMetrics::fitCount(font, line.address(), line.size(), availableForText, letterSpacing, wordSpacing, keptWidth);
   }

   outLine.setSize(keepCount + 3);
   if (keepCount > 0)
      dMemcpy(outLine.address(), line.address(), keepCount * sizeof(UTF16));
   outLine[keepCount + 0] = kEllipsis[0];
   outLine[keepCount + 1] = kEllipsis[1];
   outLine[keepCount + 2] = kEllipsis[2];
}

//=============================================================================
//    Layout.
//=============================================================================

// Core layout pass at a specific font/size: wrap (optional) -> clip/ellipsis
// (optional) -> measure -> justify. See GuiTextLayoutResult::font for why
// font is stashed into the result rather than assumed to be GuiText::mFont.
GuiTextLayoutResult GuiText::_layoutAtSize(Resource<GFont> font, S32 fontSize, S32 letterSpacing, S32 wordSpacing) const
{
   GuiTextLayoutResult result;
   result.fontSize = fontSize;
   result.font = font;

   GFont* rawFont = font;
   if (!rawFont)
      return result;

   Vector<UTF16> sourceChars;
   _decodeSourceChars(sourceChars);

   if (sourceChars.empty())
      return result;

   const S32 lineHeight = (S32)rawFont->getHeight();
   const bool hasBox = (mBoxExtent.x > 0 && mBoxExtent.y > 0);

   // Step 1: split into raw (un-clipped) lines. ALWAYS split on embedded
   // '\n' first (see _splitHardLines()'s doc comment on why this must
   // happen regardless of mWrap -- an explicit newline is a hard break
   // even in non-wrapping/single-line mode, not just when width-wrapping
   // is also active), then apply width-based wrapping to EACH resulting
   // segment independently when mWrap is on. A segment with no '\n' at
   // all degrades to exactly the old single-segment behavior.
   Vector< Vector<UTF16> > hardSegments;
   _splitHardLines(sourceChars, hardSegments);

   Vector< Vector<UTF16> > rawLines;
   Vector<U32> rawLineConsumedBefore; // parallel to rawLines -- see GuiTextLine::consumedCharsBeforeThisLine's doc comment

   for (U32 s = 0; s < hardSegments.size(); s++)
   {
      // Every segment after the first was preceded by a consumed '\n'
      // (see _splitHardLines()) -- exactly 1 character, always, unlike
      // the wrap case below which can consume more than one space.
      const U32 hardBreakConsumed = (s > 0) ? 1 : 0;

      if (mWrap && hasBox)
      {
         Vector< Vector<UTF16> > wrappedSegment;
         Vector<U32> wrappedConsumedBefore;
         _wrapLines(rawFont, hardSegments[s], mBoxExtent.x, letterSpacing, wordSpacing, wrappedSegment, wrappedConsumedBefore);

         // An empty segment (two consecutive '\n's, or a leading/trailing
         // one) still needs to occupy a line -- _wrapLines() returns
         // nothing for an empty input (see its own early-out), so that
         // case is added back explicitly rather than silently
         // collapsing a blank line out of the layout.
         if (wrappedSegment.empty())
         {
            rawLines.push_back(hardSegments[s]); // empty Vector<UTF16> -- a blank line
            rawLineConsumedBefore.push_back(hardBreakConsumed);
         }
         else
         {
            for (U32 w = 0; w < wrappedSegment.size(); w++)
            {
               rawLines.push_back(wrappedSegment[w]);
               // The FIRST wrapped line of this segment is preceded by
               // the hard break (if any) PLUS whatever _wrapLines()
               // itself reports for that same position (always 0 for
               // w==0, since nothing is consumed before the start of a
               // segment -- but added rather than assumed, in case that
               // ever changes). Subsequent lines within the segment are
               // pure width-wrap continuations -- only wrappedConsumedBefore
               // applies, no hard-break component.
               rawLineConsumedBefore.push_back((w == 0 ? hardBreakConsumed : 0) + wrappedConsumedBefore[w]);
            }
         }
      }
      else
      {
         rawLines.push_back(hardSegments[s]); // no width-wrap -- this hard segment is one line
         rawLineConsumedBefore.push_back(hardBreakConsumed);
      }
   }

   // Step 2: Clip/Ellipsis per-line, and a vertical cutoff on line count.
   U32 maxLines = rawLines.size();
   if (hasBox && mOverflow != GuiTextOverflowConsole::GuiTextOverflowConsole_Overflow)
   {
      const U32 linesThatFitHeight = (lineHeight > 0) ? getMax((U32)1, (U32)(mBoxExtent.y / lineHeight)) : rawLines.size();
      maxLines = getMin((U32)rawLines.size(), linesThatFitHeight);
   }

   for (U32 i = 0; i < maxLines; i++)
   {
      GuiTextLine outLine;

      if (hasBox && mOverflow != GuiTextOverflowConsole::GuiTextOverflowConsole_Overflow)
      {
         const bool isLastVisibleLine = (i == maxLines - 1) && (maxLines < rawLines.size());

         Vector<UTF16> clipped;
         bool truncated = false;

         if (isLastVisibleLine)
         {
            // Cut off by the vertical limit -- always mark truncated.
            _clipLine(rawFont, rawLines[i], mBoxExtent.x, letterSpacing, wordSpacing, clipped, truncated);
            if (!truncated && mOverflow == GuiTextOverflowConsole::GuiTextOverflowConsole_Ellipsis)
            {
               // Fit horizontally, but more content follows -- force ellipsis.
               Vector<UTF16> withEllipsis;
               const UTF16 kEllipsis[3] = { (UTF16)'.', (UTF16)'.', (UTF16)'.' };
               S32 ellipsisWidth = GuiTextMetrics::measure(rawFont, kEllipsis, 3, letterSpacing, wordSpacing);
               S32 avail = mBoxExtent.x - ellipsisWidth;
               S32 keptWidth = 0;
               U32 keepCount = (avail > 0) ? GuiTextMetrics::fitCount(rawFont, clipped.address(), clipped.size(), avail, letterSpacing, wordSpacing, keptWidth) : 0;
               withEllipsis.setSize(keepCount + 3);
               if (keepCount > 0)
                  dMemcpy(withEllipsis.address(), clipped.address(), keepCount * sizeof(UTF16));
               withEllipsis[keepCount + 0] = kEllipsis[0];
               withEllipsis[keepCount + 1] = kEllipsis[1];
               withEllipsis[keepCount + 2] = kEllipsis[2];
               clipped = withEllipsis;
               truncated = true;
            }
         }
         else
         {
            _clipLine(rawFont, rawLines[i], mBoxExtent.x, letterSpacing, wordSpacing, clipped, truncated);
         }

         outLine.chars = clipped;
         outLine.wasTruncated = truncated;
      }
      else
      {
         outLine.chars = rawLines[i];
         outLine.wasTruncated = false;
      }

      outLine.width = GuiTextMetrics::measure(rawFont, outLine.chars.address(), outLine.chars.size(), letterSpacing, wordSpacing);
      outLine.height = lineHeight;
      outLine.consumedCharsBeforeThisLine = rawLineConsumedBefore[i];

      result.lines.push_back(outLine);
   }

   result.didOverflow = (maxLines < rawLines.size());
   if (!result.didOverflow && hasBox)
   {
      // Overflow mode never clips, so a too-wide line still counts as overflow.
      for (U32 i = 0; i < result.lines.size(); i++)
      {
         if (result.lines[i].width > mBoxExtent.x || result.lines[i].wasTruncated)
         {
            result.didOverflow = true;
            break;
         }
      }
      if (!result.didOverflow && (S32)(result.lines.size() * lineHeight) > mBoxExtent.y)
         result.didOverflow = true;
   }

   // Step 3: justify.
   const S32 blockHeight = (S32)result.lines.size() * lineHeight;

   S32 blockStartY = 0;
   if (hasBox)
   {
      switch (mAlignV)
      {
      case GuiTextAlignVertical::GuiTextAlignVertical_Top:
         blockStartY = 0;
         break;
      case GuiTextAlignVertical::GuiTextAlignVertical_Bottom:
         blockStartY = mBoxExtent.y - blockHeight;
         break;
      case GuiTextAlignVertical::GuiTextAlignVertical_Middle:
      default:
         blockStartY = (mBoxExtent.y - blockHeight) / 2;
         break;
      }
   }

   S32 minX = S32_MAX, minY = S32_MAX, maxX = S32_MIN, maxY = S32_MIN;

   for (U32 i = 0; i < result.lines.size(); i++)
   {
      GuiTextLine& line = result.lines[i];

      S32 lineStartX = 0;
      if (hasBox && line.width <= mBoxExtent.x)
      {
         switch (mAlignH)
         {
         case GuiTextAlignHorizontal::GuiTextAlignHorizontal_Right:
            lineStartX = mBoxExtent.x - line.width;
            break;
         case GuiTextAlignHorizontal::GuiTextAlignHorizontal_Center:
            lineStartX = (mBoxExtent.x - line.width) / 2;
            break;
         case GuiTextAlignHorizontal::GuiTextAlignHorizontal_Left:
         default:
            lineStartX = 0;
            break;
         }
      }
      // else: wider than the box -- always left-justify.

      line.origin = Point2I(lineStartX, blockStartY + (S32)i * lineHeight);

      minX = getMin(minX, line.origin.x);
      minY = getMin(minY, line.origin.y);
      maxX = getMax(maxX, line.origin.x + line.width);
      maxY = getMax(maxY, line.origin.y + line.height);
   }

   if (!result.lines.empty())
      result.blockBounds = RectI(Point2I(minX, minY), Point2I(maxX - minX, maxY - minY));

   return result;
}

//-----------------------------------------------------------------------------

// Fixed mode: one layout pass at the authored size. ShrinkToFit: repeats the
// pass at successively smaller candidate sizes (via mFontAtSizeDelegate)
// until it fits or mDynamicFontSizeMin is reached.
const GuiTextLayoutResult& GuiText::layout()
{
   if (!mLayoutDirty)
      return mCachedResult;

   GFont* initialFont = mFont;

   if (!initialFont)
   {
      mCachedResult = GuiTextLayoutResult();
      mLayoutDirty = false;
      return mCachedResult;
   }

   const S32 letterSpacing = 0; // applied later, at submit() time
   const S32 wordSpacing = 0;

   if (mFitMode != GuiTextFitMode::ShrinkToFit || !mFontAtSizeDelegate)
   {
      mCachedResult = _layoutAtSize(mFont, initialFont->getFontSize(), letterSpacing, wordSpacing);
      mLayoutDirty = false;
      return mCachedResult;
   }

   S32 candidateSize = initialFont->getFontSize();
   GuiTextLayoutResult best = _layoutAtSize(mFont, candidateSize, letterSpacing, wordSpacing);

   const bool hasBox = (mBoxExtent.x > 0 && mBoxExtent.y > 0);

   while (hasBox && best.didOverflow && candidateSize > mDynamicFontSizeMin)
   {
      candidateSize = getMax(mDynamicFontSizeMin, candidateSize - mDynamicFontSizeStep);

      Resource<GFont> candidateFont = mFontAtSizeDelegate(candidateSize);
      GFont* rawCandidate = candidateFont;
      if (!rawCandidate)
         break; // delegate couldn't produce a font at this size

      best = _layoutAtSize(candidateFont, candidateSize, letterSpacing, wordSpacing);

      if (candidateSize <= mDynamicFontSizeMin)
         break;
   }

   mCachedResult = best;
   mLayoutDirty = false;
   return mCachedResult;
}

//=============================================================================
//    Submission.
//=============================================================================

// Pushes every layout() line into batch, drawing with result.font (the
// actual font the layout was measured against -- may differ from mFont
// under ShrinkToFit).
void GuiText::submit(GuiRenderBatch& batch, const Point2I& basePos, const ColorI& color, S32 letterSpacing, S32 wordSpacing, S32 layer)
{
   const GuiTextLayoutResult& result = layout();

   if (!result.font)
      return;

   // NOTE: letterSpacing/wordSpacing here must match whatever a caller uses
   // for its own measurement (e.g. cursor hit-testing), since layout()
   // itself measures with 0/0 -- see GuiStyleProperties::letterSpacing.
   for (U32 i = 0; i < result.lines.size(); i++)
   {
      const GuiTextLine& line = result.lines[i];
      if (line.chars.empty())
         continue;

      batch.pushTextRun(result.font, basePos + line.origin, line.chars.address(), line.chars.size(), color, letterSpacing, wordSpacing, layer);
   }
}

//-----------------------------------------------------------------------------

// One-shot convenience wrapper -- see guiText.h.
void GuiText::renderSimple(GuiRenderBatch& batch, const Resource<GFont>& font, const Point2I& basePos, const Point2I& boxExtent, const char* text, const ColorI& color, GuiTextAlignHorizontal alignH, GuiTextAlignVertical alignV, S32 letterSpacing, S32 wordSpacing, S32 layer)
{
   if (!font || !text || !text[0])
      return;

   GuiText t;
   t.setFont(font);
   t.setText(text);
   t.setBoxExtent(boxExtent);
   t.setAlignHorizontal(alignH);
   t.setAlignVertical(alignV);
   t.submit(batch, basePos, color, letterSpacing, wordSpacing, layer);
}
