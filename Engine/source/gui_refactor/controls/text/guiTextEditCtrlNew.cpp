//-----------------------------------------------------------------------------
// guiTextEditCtrlNew.cpp
// See guiTextEditCtrlNew.h for scope/design notes.
//-----------------------------------------------------------------------------

#include "gui_refactor/controls/text/guiTextEditCtrlNew.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiTextEditCtrlNew);

//-----------------------------------------------------------------------------

namespace
{
   const ColorI kDefaultCaretColor(255, 255, 255, 255);
   const ColorI kDefaultSelectionColor(60, 110, 200, 120);

   // String::insert()/substr() are not confirmed to exist against this
   // project's actual String header (not in the mounted tree -- see
   // this file's header). Everything below is built only from
   // operations already confirmed elsewhere in this codebase:
   // .c_str(), .length(), .erase(pos,count), String::ToString(),
   // plain C-string indexing, and String's char*-constructor/operator=.
   // A bit more verbose than a one-line substr()/insert() call would
   // be, but every piece here is something this project's code already
   // does somewhere else.

   // Returns the [start, start+count) slice of s as a new String.
   // count is clamped to what's actually available.
   String sliceString(const String& s, S32 start, S32 count)
   {
      const S32 len = (S32)s.length();
      start = getMax(0, getMin(start, len));
      count = getMax(0, getMin(count, len - start));

      if (count == 0)
         return String();

      // +1 for the null terminator sprintf-style construction needs.
      FrameTemp<char> buf(count + 1);
      dMemcpy((char*)buf, s.c_str() + start, count);
      ((char*)buf)[count] = '\0';
      return String((const char*)buf);
   }

   // Returns a new String with insertion spliced in at position pos
   // (characters, clamped to [0, s.length()]).
   String spliceInsert(const String& s, S32 pos, const String& insertion)
   {
      const S32 len = (S32)s.length();
      pos = getMax(0, getMin(pos, len));

      const String before = sliceString(s, 0, pos);
      const String after = sliceString(s, pos, len - pos);

      String result = before;
      result += insertion;
      result += after;
      return result;
   }

   // Returns a new String with [start, start+count) removed.
   String spliceErase(const String& s, S32 start, S32 count)
   {
      String result = s;
      const S32 len = (S32)result.length();
      start = getMax(0, getMin(start, len));
      count = getMax(0, getMin(count, len - start));
      if (count > 0)
         result.erase(start, count);
      return result;
   }
}

//-----------------------------------------------------------------------------

GuiTextEditCtrlNew::GuiTextEditCtrlNew()
   : mMaxLength(0),
   mMultiLine(false),
   mPasswordMask(false),
   mCursorPos(0),
   mSelectionAnchor(-1),
   mMouseSelecting(false),
   mDesiredCaretDeviceX(-1),
   mLastBlinkToggleMs(0),
   mCursorBlinkOn(true)
{
   setCapturesInput(true);
   mFocusable = true;

   mGuiText.setAlignHorizontal(GuiTextAlignHorizontal_Left);
   mGuiText.setAlignVertical(GuiTextAlignVertical_Top);
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::initPersistFields()
{
   addGroup("TextEdit");

   addProtectedField("text", TypeRealString, Offset(mText, GuiTextEditCtrlNew), &setTextProt, &defaultProtectedGetFn,
      "The current text content.");

   addField("maxLength", TypeS32, Offset(mMaxLength, GuiTextEditCtrlNew),
      "Maximum character count; 0 (default) means unlimited.");

   addProtectedField("multiLine", TypeBool, Offset(mMultiLine, GuiTextEditCtrlNew), &setMultiLineProt, &defaultProtectedGetFn,
      "false (default): single line, Enter fires onAction()/the bound console command. "
      "true: wrapped multi-line editing, Enter inserts a newline instead.");

   addProtectedField("passwordMask", TypeBool, Offset(mPasswordMask, GuiTextEditCtrlNew), &setPasswordMaskProt, &defaultProtectedGetFn,
      "When true, displays every character as a mask glyph -- see GuiText::setMasked(). "
      "The real text is still what's stored/edited, only the drawn glyphs are substituted.");

   endGroup("TextEdit");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::onWake()
{
   if (!Parent::onWake())
      return false;

   mGuiText.setText(mText);
   mCursorPos = _clampCursor(mCursorPos);

   return true;
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::setText(const String& text)
{
   const String newText = mMaxLength > 0 ? sliceString(text, 0, mMaxLength) : text;

   if (String::compare(mText.c_str(), newText.c_str()) == 0)
      return;

   mText = newText;
   mCursorPos = _clampCursor(mCursorPos);
   mSelectionAnchor = -1;
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::setMultiLine(bool value)
{
   if (mMultiLine == value)
      return;

   mMultiLine = value;
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::setPasswordMask(bool value)
{
   if (mPasswordMask == value)
      return;

   mPasswordMask = value;
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::setSelection(S32 start, S32 end)
{
   start = _clampCursor(start);
   end = _clampCursor(end);

   mSelectionAnchor = (start == end) ? -1 : start;
   mCursorPos = end;
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::selectAll()
{
   setSelection(0, (S32)mText.length());
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::clearSelection()
{
   mSelectionAnchor = -1;
   setUpdate();
}

//-----------------------------------------------------------------------------

S32 GuiTextEditCtrlNew::_clampCursor(S32 pos) const
{
   return getMax(0, getMin(pos, (S32)mText.length()));
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::_hasSelection() const
{
   return mSelectionAnchor >= 0 && mSelectionAnchor != mCursorPos;
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::_getSelectionRange(S32& outStart, S32& outEnd) const
{
   outStart = getMin(mSelectionAnchor, mCursorPos);
   outEnd = getMax(mSelectionAnchor, mCursorPos);
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::_deleteSelection()
{
   if (!_hasSelection())
      return;

   S32 start, end;
   _getSelectionRange(start, end);

   mText = spliceErase(mText, start, end - start);
   mCursorPos = start;
   mSelectionAnchor = -1;
}

//-----------------------------------------------------------------------------

S32 GuiTextEditCtrlNew::_findWordBoundary(S32 flatCharIndex, bool isForward) const
{
   // Plain ' '/'\t'/'\n' check against raw bytes, not a confirmed
   // library isspace()-style helper -- this project doesn't have one
   // verified anywhere (see this file's header on the general policy of
   // not guessing unconfirmed signatures). Byte-wise, matching every
   // other index in this class (mCursorPos/mSelectionAnchor are byte
   // offsets into mText throughout, not true Unicode codepoint indices
   // -- consistent with the rest of this file's existing convention,
   // not a new one introduced here).
   auto isWhitespace = [this](S32 i) -> bool
   {
      const char c = mText.c_str()[i];
      return c == ' ' || c == '\t' || c == '\n';
   };

   const S32 len = (S32)mText.length();
   flatCharIndex = getMax(0, getMin(flatCharIndex, len));

   if (isForward)
   {
      S32 i = flatCharIndex;

      // Skip any whitespace right at the cursor first, so Ctrl+Right
      // from mid-whitespace lands at the start of the NEXT word, not
      // immediately after the whitespace run began.
      while (i < len && isWhitespace(i))
         i++;

      // Skip the rest of the current word.
      while (i < len && !isWhitespace(i))
         i++;

      return i;
   }
   else
   {
      S32 i = flatCharIndex;

      // Skip whitespace immediately to the left first.
      while (i > 0 && isWhitespace(i - 1))
         i--;

      // Skip back through the word itself.
      while (i > 0 && !isWhitespace(i - 1))
         i--;

      return i;
   }
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::_insertText(const String& insertion)
{
   if (_hasSelection())
      _deleteSelection();

   String toInsert = insertion;

   if (mMaxLength > 0)
   {
      const S32 roomLeft = mMaxLength - (S32)mText.length();
      if (roomLeft <= 0)
         return; // already at cap, and nothing selected to make room
      if ((S32)toInsert.length() > roomLeft)
         toInsert = sliceString(toInsert, 0, roomLeft);
   }

   mText = spliceInsert(mText, mCursorPos, toInsert);
   mCursorPos = _clampCursor(mCursorPos + (S32)toInsert.length());
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::_findCaretLineAndOffset(S32 flatCharIndex, U32& outLineIndex, U32& outLineLocalOffset)
{
   const GuiTextLayoutResult& result = mGuiText.layout();

   if (result.lines.empty())
      return false;

   S32 remaining = flatCharIndex;

   for (U32 i = 0; i < result.lines.size(); ++i)
   {
      const U32 lineLen = result.lines[i].chars.size();

      // Last line (or the running index lands within/at the end of this
      // one) -- clamp here rather than falling off the end, so a cursor
      // at the very end of the text still resolves to a valid position
      // on the last line instead of returning false.
      if ((U32)remaining <= lineLen || i == result.lines.size() - 1)
      {
         outLineIndex = i;
         outLineLocalOffset = (U32)getMax(0, getMin(remaining, (S32)lineLen));
         return true;
      }

      remaining -= (S32)lineLen;

      // The line boundary just crossed consumed
      // result.lines[i+1].consumedCharsBeforeThisLine source characters
      // that are NOT in any GuiTextLine::chars (a real '\n' -- always
      // exactly 1 -- and/or spaces a width-wrap broke on, which can be
      // more than 1) but ARE still present in mText's flat coordinate
      // space, since mCursorPos/flatCharIndex index into the original
      // mText, not the post-layout line buffers. Getting this wrong (or
      // only accounting for the '\n' case, as an earlier version of this
      // field did) is exactly what silently misplaced the caret/word-jump
      // target after a line boundary -- correctly for an explicit '\n'
      // (which was tracked), incorrectly after an auto-wrap (which
      // wasn't, until this field started reporting a real count instead
      // of a hard-break-only bool).
      if (i + 1 < result.lines.size())
         remaining -= (S32)result.lines[i + 1].consumedCharsBeforeThisLine;
   }

   return false;
}

//-----------------------------------------------------------------------------

S32 GuiTextEditCtrlNew::_findFlatIndexFromLineAndOffset(U32 lineIndex, U32 lineLocalOffset)
{
   const GuiTextLayoutResult& result = mGuiText.layout();

   if (result.lines.empty())
      return 0;

   lineIndex = getMin(lineIndex, (U32)result.lines.size() - 1);
   lineLocalOffset = getMin(lineLocalOffset, (U32)result.lines[lineIndex].chars.size());

   S32 flatIndex = 0;
   for (U32 i = 0; i < lineIndex; ++i)
   {
      flatIndex += (S32)result.lines[i].chars.size();

      // Same accounting as _findCaretLineAndOffset()'s own doc comment.
      if (i + 1 < result.lines.size())
         flatIndex += (S32)result.lines[i + 1].consumedCharsBeforeThisLine;
   }

   return flatIndex + (S32)lineLocalOffset;
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::_getSelectionRangeForLine(U32 lineIndex, S32 selStart, S32 selEnd, U32& outLocalStart, U32& outLocalEnd)
{
   const GuiTextLayoutResult& result = mGuiText.layout();

   if (lineIndex >= result.lines.size())
      return false;

   const U32 lineLen = result.lines[lineIndex].chars.size();

   // This line's own flat-index span: [lineFlatStart, lineFlatEnd). The
   // end is one past the line's last character, which is exactly what
   // _findFlatIndexFromLineAndOffset(lineIndex, lineLen) already computes
   // (it clamps lineLocalOffset to chars.size(), so passing the full
   // length directly gives "one past the end of this line" -- the same
   // flat index a caret sitting at the very end of the line would have).
   const S32 lineFlatStart = _findFlatIndexFromLineAndOffset(lineIndex, 0);
   const S32 lineFlatEnd = _findFlatIndexFromLineAndOffset(lineIndex, lineLen);

   // Intersect [selStart, selEnd) with [lineFlatStart, lineFlatEnd).
   const S32 clippedStart = getMax(selStart, lineFlatStart);
   const S32 clippedEnd = getMin(selEnd, lineFlatEnd);

   if (clippedStart >= clippedEnd)
      return false; // selection doesn't touch this line at all

   outLocalStart = (U32)getMax(0, clippedStart - lineFlatStart);
   outLocalEnd = (U32)getMin((S32)lineLen, clippedEnd - lineFlatStart);
   return true;
}

//-----------------------------------------------------------------------------

Resource<GFont> GuiTextEditCtrlNew::_resolveFont() const
{
   if (!mStyle)
      return Resource<GFont>();

   return mStyle->getResolvedFont(getCurrentStyleStateMask());
}

//-----------------------------------------------------------------------------

Point2I GuiTextEditCtrlNew::_deviceLocalPointFromEvent(const GuiEvent& event) const
{
   GuiCanvasNew* root = getRoot();
   if (!root)
      return Point2I(0, 0);

   const Point2I logicalLocal = globalToLocalCoord(event.mousePoint);
   return Point2I(
      (S32)(logicalLocal.x * root->getEffectiveScaleX()),
      (S32)(logicalLocal.y * root->getEffectiveScaleY()));
}

//-----------------------------------------------------------------------------

S32 GuiTextEditCtrlNew::_hitTestCharAt(const Point2I& devicePt, const Point2I& offset, Resource<GFont> font)
{
   const GuiTextLayoutResult& result = mGuiText.layout();

   if (result.lines.empty() || !font)
      return 0;

   // Step 1: which LINE. Single-line always has exactly one line, so
   // this is a no-op there -- the loop below still runs correctly (one
   // iteration, always "closest"). Picks the line whose vertical span
   // [origin.y, origin.y + height) contains devicePt.y, or the nearest
   // one if the click is above the first line or below the last.
   U32 lineIndex = 0;
   for (U32 i = 0; i < result.lines.size(); ++i)
   {
      const GuiTextLine& line = result.lines[i];
      const S32 lineTop = offset.y + line.origin.y;
      const S32 lineBottom = lineTop + line.height;

      lineIndex = i; // last line seen so far -- correct fallback if the click is below every line

      if (devicePt.y < lineBottom)
         break; // this is the first line whose bottom is at-or-below the click -- it's the one, whether the click is above its top (click above all text -> first line, handled by i==0 already being current) or within it
   }

   const GuiTextLine& line = result.lines[lineIndex];
   const S32 lineStartX = offset.x + line.origin.x;

   // Step 2: which CHARACTER on that line. Walks character advances
   // (same GuiTextMetrics formula used everywhere else in this class,
   // for consistency with how the caret itself is positioned) looking
   // for the first boundary whose midpoint is past devicePt.x -- i.e.
   // clicking closer to a character's right half places the cursor
   // after it, closer to its left half places the cursor before it,
   // which is the usual click-to-position feel.
   S32 x = lineStartX;
   U32 localOffset = line.chars.size(); // default: click past the end of the line -> end of line

   for (U32 c = 0; c < line.chars.size(); ++c)
   {
      const PlatformFont::CharInfo& ci = font->getCharInfo(line.chars[c]);
      const S32 advance = ci.xIncrement;

      if (devicePt.x < x + advance / 2)
      {
         localOffset = c;
         break;
      }

      x += advance;
   }

   return _findFlatIndexFromLineAndOffset(lineIndex, localOffset);
}

//-----------------------------------------------------------------------------

const GuiTextLayoutResult& GuiTextEditCtrlNew::_refreshLayout(const Resource<GFont>& font, const Point2I& boxExtent)
{
   mGuiText.setFont(font);
   mGuiText.setText(mText);
   mGuiText.setBoxExtent(boxExtent);
   mGuiText.setWrap(mMultiLine);
   mGuiText.setMasked(mPasswordMask);
   mGuiText.setOverflow(mMultiLine ? GuiTextOverflowConsole::GuiTextOverflowConsole_Overflow : GuiTextOverflowConsole::GuiTextOverflowConsole_Clip);

   return mGuiText.layout();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::onGainFirstResponder()
{
   Parent::onGainFirstResponder();

   // Tells the platform layer to start generating real character events
   // (SDL_TEXTINPUT on the SDL backend -- see GuiCanvasNew::
   // enableKeyboardTranslation()'s doc comment and PlatformWindowSDL::
   // setKeyboardTranslation(), which gates SDL_StartTextInput()/
   // SDL_StopTextInput() via mInputState). Without this, nothing in the
   // engine ever asks SDL to start sending SDL_TEXTINPUT events at all
   // -- WindowInputGenerator::handleCharInput() and everything
   // downstream of it (onKeyDown()'s KEY_NULL/ascii branch, above) are
   // correctly wired but permanently starved with no caller ever having
   // turned text input mode on. A text-input control gaining focus is
   // exactly the "usually the [request]" case enableKeyboardTranslation()'s
   // own doc comment describes.
   //
   // KNOWN LIMITATION: PlatformWindowManagerSDL::_process() deliberately
   // defers the actual SDL_StartTextInput() call to AFTER the current
   // event-loop pass finishes (see KeyboardInputState's own doc comment
   // -- this is intentional, not a bug: calling SDL_StartTextInput()
   // synchronously mid-event-loop caused the SAME keystroke that
   // triggered a mode change to also generate a spurious text event,
   // e.g. the console-toggle key appearing in the console's own text
   // buffer). That means a keystroke arriving in the SAME event batch as
   // the click that focused this field can still be processed before
   // text input mode has actually taken effect, and its character is
   // lost -- this is a genuine one-event-loop-pass race in the platform
   // layer, not something fixable from here without reintroducing the
   // bug the deferral exists to prevent. Calling this again on every
   // focus-gain (rather than only once, ever) is a real, if partial,
   // mitigation: it's cheap/idempotent (mInputState is reset to NONE
   // after each _process() pass regardless, so re-requesting is a
   // harmless no-op once text input is already on) and keeps the
   // request as fresh/early as this control's own code can make it --
   // but it cannot retroactively recover a character already dropped by
   // a race that happened before this call even ran.
   GuiCanvasNew* root = getRoot();
   if (root)
      root->enableKeyboardTranslation();

   mTextOnFocusGain = mText;
   mCursorBlinkOn = true;
   mLastBlinkToggleMs = Platform::getVirtualMilliseconds();
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::onLoseFirstResponder()
{
   Parent::onLoseFirstResponder();

   // Symmetric with onGainFirstResponder() above -- turns text input mode
   // back off so normal key-only controls (buttons, accelerators, etc.)
   // aren't affected by it once this field is no longer focused.
   GuiCanvasNew* root = getRoot();
   if (root)
      root->disableKeyboardTranslation();

   mSelectionAnchor = -1;
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::onPreRender()
{
   Parent::onPreRender();

   if (!isFirstResponder())
      return;

   const U32 now = Platform::getVirtualMilliseconds();
   if (now - mLastBlinkToggleMs >= smBlinkIntervalMs)
   {
      mCursorBlinkOn = !mCursorBlinkOn;
      mLastBlinkToggleMs = now;
      setUpdate();
   }
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::onMouseDown(const GuiEvent& event)
{
   if (!mVisible || !mAwake || !mActive)
      return;

   setFirstResponder();
   mouseLock();

   // _hitTestCharAt() reads mGuiText.layout()'s CURRENT result -- refresh
   // it against this control's live bounds/font first, since onRender()
   // hasn't necessarily run yet this frame (a click can be the very
   // first thing that happens after the control appears/resizes).
   const Resource<GFont> font = _resolveFont();
   if (font != NULL)
      _refreshLayout(font, getDeviceBounds().extent);

   const S32 clickIndex = _hitTestCharAt(_deviceLocalPointFromEvent(event), Point2I(0, 0), font);

   const bool shiftHeld = (event.modifier & SI_SHIFT) != 0;
   if (shiftHeld && mSelectionAnchor < 0)
      mSelectionAnchor = mCursorPos; // extend the existing cursor position into a selection, shift-click style
   else if (!shiftHeld)
      mSelectionAnchor = clickIndex; // anchor here -- onMouseDragged() extends from this point if the mouse moves before release

   mCursorPos = clickIndex;
   mMouseSelecting = true;
   mDesiredCaretDeviceX = -1; // any horizontal movement resets the Up/Down column memory -- see its own doc comment

   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::onMouseUp(const GuiEvent& event)
{
   if (isMouseLocked())
      mouseUnlock();

   mMouseSelecting = false;
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::onMouseDragged(const GuiEvent& event)
{
   if (!mMouseSelecting)
      return;

   // Layout doesn't need refreshing here the way onMouseDown() does --
   // it's already current from either onMouseDown() (drag started this
   // same interaction) or the frame's own onRender() (which has run at
   // least once by the time a drag is in progress). Still needs a live
   // Resource<GFont> to pass through to _hitTestCharAt() (see its own
   // doc comment on why it can't just read GuiTextLayoutResult::font
   // itself) -- resolving it here is cheap, a plain mStyle lookup
   // against an already-cached resource, not a fresh load.
   mCursorPos = _hitTestCharAt(_deviceLocalPointFromEvent(event), Point2I(0, 0), _resolveFont());
   setUpdate();
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::onKeyDown(const GuiEvent& event)
{
   if (!mActive || !isFirstResponder())
      return Parent::onKeyDown(event);

   // Character input arrives as its OWN, SEPARATE onKeyDown() call, not
   // bundled into the same event as a real key press. This engine's
   // WindowInputGenerator has two distinct keyboard entry points:
   //   - handleKeyboard() -- fires for every physical key with a real
   //     objInst/keyCode, but ALWAYS sets event.ascii = 0 (confirmed
   //     directly in WindowInputGenerator::handleKeyboard's source --
   //     it never assigns ascii from anything). This is what arrow
   //     keys/Backspace/Delete/Home/End/Enter/Escape all come through,
   //     and is exactly why those already worked correctly before this
   //     fix -- none of them depend on ascii being populated.
   //   - handleCharInput() -- fires SEPARATELY, once per actual decoded
   //     character (shift/caps/layout already resolved by the OS), with
   //     event.objInst == KEY_NULL and a REAL event.ascii. It generates
   //     its own SI_MAKE/SI_BREAK pair through the exact same
   //     generateInputEvent()->processKeyboardEvent()->onKeyDown() path
   //     as any other key -- see WindowInputGenerator::handleCharInput's
   //     source. So checking event.ascii inside the SAME branch as
   //     keyCode comparisons (the original version of this function)
   //     could never work: a character-carrying event's keyCode is
   //     always KEY_NULL, so it would silently fail every keyCode == ...
   //     check above it and only reach an ascii check at the very
   //     bottom, AFTER already being caught (or not) by earlier
   //     branches that don't apply to it at all. Checking this FIRST
   //     and unconditionally (before any keyCode branching) is what
   //     actually matches how these two event kinds are meant to be
   //     told apart.
   if (event.keyCode == KEY_NULL && event.ascii != 0)
   {
      // >= 32 excludes control characters (e.g. a stray 13/10 if some
      // platform ever routes Enter through this path too, rather than
      // solely through handleKeyboard's KEY_RETURN) -- everything
      // meaningful below 32 is already handled via the keyCode-based
      // branches further down, which this event will never reach.
      if (event.ascii >= 32)
      {
         const UTF16 utf16Buf[2] = { event.ascii, 0 };

         // 3 bytes per code point + 1 for the null terminator, per
         // convertUTF8toUTF16N()/convertUTF16toUTF8N()'s own doc
         // comment in unicode.h ("UTF-8 output is clamped to 3 code
         // units per code point").
         char utf8Buf[4];
         const U32 utf8Len = convertUTF16toUTF8N(utf16Buf, (UTF8*)utf8Buf, sizeof(utf8Buf));
         utf8Buf[getMin(utf8Len, (U32)sizeof(utf8Buf) - 1)] = '\0';

         _insertText(utf8Buf);
         mDesiredCaretDeviceX = -1; // horizontal movement -- see its own doc comment
         mCursorBlinkOn = true;
         mLastBlinkToggleMs = Platform::getVirtualMilliseconds();
         setUpdate();
      }

      return true;
   }

   const bool shiftHeld = (event.modifier & SI_SHIFT) != 0;
   const bool ctrlHeld = (event.modifier & SI_CTRL) != 0;

   // --- Navigation -----------------------------------------------------
   if (event.keyCode == KEY_LEFT || event.keyCode == KEY_RIGHT)
   {
      const bool isForward = (event.keyCode == KEY_RIGHT);

      if (!shiftHeld && _hasSelection())
      {
         // Collapse an existing selection to whichever edge the arrow
         // points toward, matching common text-field behavior, rather
         // than moving relative to mCursorPos and potentially jumping
         // past the selection entirely. Applies whether or not Ctrl is
         // also held -- a Ctrl+arrow on an existing selection still
         // just collapses it first, same as a plain arrow would; the
         // word-jump only kicks in once there's no selection left to
         // collapse (i.e. on the FOLLOWING press).
         S32 start, end;
         _getSelectionRange(start, end);
         mCursorPos = isForward ? end : start;
         mSelectionAnchor = -1;
      }
      else
      {
         if (shiftHeld && mSelectionAnchor < 0)
            mSelectionAnchor = mCursorPos;

         mCursorPos = ctrlHeld
            ? _findWordBoundary(mCursorPos, isForward)
            : _clampCursor(mCursorPos + (isForward ? 1 : -1));

         if (!shiftHeld)
            mSelectionAnchor = -1;
      }

      mDesiredCaretDeviceX = -1; // horizontal movement -- see its own doc comment
      mCursorBlinkOn = true;
      mLastBlinkToggleMs = Platform::getVirtualMilliseconds();
      setUpdate();
      return true;
   }

   // Multi-line only -- single-line has nothing above/below to move to,
   // and this project's confirmed KEY_UP/KEY_DOWN usage elsewhere
   // (guiScrollCtrlNew.cpp) is the only precedent for these constants,
   // same as this file's other KEY_* names -- see file header.
   if (mMultiLine && (event.keyCode == KEY_UP || event.keyCode == KEY_DOWN))
   {
      // _hitTestCharAt()/_findCaretLineAndOffset() both need a current
      // layout -- refresh it against this control's live bounds/font
      // first, the same reasoning as onMouseDown()'s own refresh (a
      // key press can arrive before onRender() has run this frame).
      // NOT declared const: GFont* rawFont = font; below needs
      // Resource<GFont>'s non-const operator T*() (getCharInfo() is
      // itself non-const -- see _hitTestCharAt()'s doc comment in the
      // header for the full reasoning), which a const Resource<GFont>
      // cannot reach.
      Resource<GFont> font = _resolveFont();
      if (font != NULL)
         _refreshLayout(font, getDeviceBounds().extent);

      const GuiTextLayoutResult& layoutResult = mGuiText.layout();
      GFont* rawFont = font;

      if (!layoutResult.lines.empty() && rawFont)
      {
         U32 curLine, curLocalOffset;
         if (_findCaretLineAndOffset(mCursorPos, curLine, curLocalOffset))
         {
            // Remember (or reuse) the target column BEFORE moving, so
            // consecutive Up/Down presses stay on the same visual
            // column even after passing through a shorter line -- see
            // mDesiredCaretDeviceX's own doc comment. Measured from the
            // CURRENT line/offset, matching how the caret itself is
            // drawn (GuiTextMetrics::measure() against line.chars up to
            // the offset), so this is self-consistent with onRender()'s
            // caret position rather than a separately-derived value
            // that could disagree with it.
            if (mDesiredCaretDeviceX < 0)
            {
               const GuiTextLine& curLineData = layoutResult.lines[curLine];
               mDesiredCaretDeviceX = curLineData.origin.x +
                  GuiTextMetrics::measure(rawFont, curLineData.chars.address(), curLocalOffset, 0, 0);
            }

            const S32 targetLineIndex = (event.keyCode == KEY_UP)
               ? (S32)curLine - 1
               : (S32)curLine + 1;

            if (targetLineIndex >= 0 && targetLineIndex < (S32)layoutResult.lines.size())
            {
               // Find the character on the target line closest to
               // mDesiredCaretDeviceX -- same per-character advance walk
               // as _hitTestCharAt()'s Step 2, reused here since it's
               // exactly the same "closest boundary to an x position on
               // a known line" problem.
               const GuiTextLine& targetLineData = layoutResult.lines[targetLineIndex];
               S32 x = targetLineData.origin.x;
               U32 targetLocalOffset = targetLineData.chars.size();

               for (U32 c = 0; c < targetLineData.chars.size(); ++c)
               {
                  const PlatformFont::CharInfo& ci = rawFont->getCharInfo(targetLineData.chars[c]);
                  const S32 advance = ci.xIncrement;

                  if (mDesiredCaretDeviceX < x + advance / 2)
                  {
                     targetLocalOffset = c;
                     break;
                  }

                  x += advance;
               }

               if (shiftHeld && mSelectionAnchor < 0)
                  mSelectionAnchor = mCursorPos;

               mCursorPos = _findFlatIndexFromLineAndOffset((U32)targetLineIndex, targetLocalOffset);

               if (!shiftHeld)
                  mSelectionAnchor = -1;

               mCursorBlinkOn = true;
               mLastBlinkToggleMs = Platform::getVirtualMilliseconds();
               setUpdate();
            }
            // else: already at the first/last line -- no-op, same as a
            // real text editor (Up on line 1 doesn't do anything).
         }
      }

      return true;
   }

   if (event.keyCode == KEY_HOME || event.keyCode == KEY_END)
   {
      if (shiftHeld && mSelectionAnchor < 0)
         mSelectionAnchor = mCursorPos;

      mCursorPos = (event.keyCode == KEY_HOME) ? 0 : (S32)mText.length();

      if (!shiftHeld)
         mSelectionAnchor = -1;

      mDesiredCaretDeviceX = -1; // horizontal movement -- see its own doc comment
      mCursorBlinkOn = true;
      mLastBlinkToggleMs = Platform::getVirtualMilliseconds();
      setUpdate();
      return true;
   }

   // --- Deletion ---------------------------------------------------------
   if (event.keyCode == KEY_BACKSPACE)
   {
      if (_hasSelection())
         _deleteSelection();
      else if (mCursorPos > 0)
      {
         mText = spliceErase(mText, mCursorPos - 1, 1);
         mCursorPos -= 1;
      }
      mDesiredCaretDeviceX = -1; // horizontal movement -- see its own doc comment
      setUpdate();
      return true;
   }

   if (event.keyCode == KEY_DELETE)
   {
      if (_hasSelection())
         _deleteSelection();
      else if (mCursorPos < (S32)mText.length())
         mText = spliceErase(mText, mCursorPos, 1);

      mDesiredCaretDeviceX = -1; // horizontal movement -- see its own doc comment
      setUpdate();
      return true;
   }

   // --- Commit / cancel ----------------------------------------------
   if (!mMultiLine && (event.keyCode == KEY_RETURN || event.keyCode == KEY_NUMPADENTER))
   {
      onAction();
      return true;
   }

   if (event.keyCode == KEY_ESCAPE)
   {
      // Revert to whatever the text was when this control last gained
      // focus -- useful for exactly the "cancel an in-progress edit"
      // interaction an inspector field needs, without requiring a
      // separate undo stack.
      setText(mTextOnFocusGain);
      clearFirstResponder();
      return true;
   }

   // --- Character insertion (newline only here; printable characters
   // are handled at the very top of this function via the SEPARATE
   // KEY_NULL/handleCharInput event -- see this function's opening
   // comment) --------------------------------------------------------
   if (mMultiLine && (event.keyCode == KEY_RETURN || event.keyCode == KEY_NUMPADENTER))
   {
      _insertText("\n");
      mDesiredCaretDeviceX = -1; // horizontal (and vertical) movement -- see its own doc comment
      setUpdate();
      return true;
   }

   return Parent::onKeyDown(event);
}

//-----------------------------------------------------------------------------

void GuiTextEditCtrlNew::onRender(Point2I offset, const RectI& updateRect)
{
   const RectI ctrlRect(offset, getDeviceBounds().extent);
   const GuiStyleProperties style = resolveStyle();

   GuiCanvasNew* root = getRoot();
   if (!root)
      return;
   GuiRenderBatch& batch = root->getRenderBatch();

   if (style.backgroundColor.isSet())
      batch.pushQuad(ctrlRect, style.backgroundColor.mValue, getRenderLayer());

   if (style.borderWidth.isSet() && style.borderWidth.mValue > 0 && style.borderColor.isSet())
   {
      const S32 bw = style.borderWidth.mValue;
      const ColorI& bc = style.borderColor.mValue;
      batch.pushQuad(RectI(ctrlRect.point, Point2I(ctrlRect.extent.x, bw)), bc, getRenderLayer());
      batch.pushQuad(RectI(Point2I(ctrlRect.point.x, ctrlRect.point.y + ctrlRect.extent.y - bw), Point2I(ctrlRect.extent.x, bw)), bc, getRenderLayer());
      batch.pushQuad(RectI(ctrlRect.point, Point2I(bw, ctrlRect.extent.y)), bc, getRenderLayer());
      batch.pushQuad(RectI(Point2I(ctrlRect.point.x + ctrlRect.extent.x - bw, ctrlRect.point.y), Point2I(bw, ctrlRect.extent.y)), bc, getRenderLayer());
   }

   if (!mStyle)
      return;

   Resource<GFont> fontRes = mStyle->getResolvedFont(getCurrentStyleStateMask());
   if (!fontRes)
      return;

   const GuiTextLayoutResult& layoutResult = _refreshLayout(fontRes, ctrlRect.extent);

   GFont* font = fontRes;

   // Selection highlight, drawn BEHIND the text (lower layer).
   if (_hasSelection() && font)
   {
      S32 selStart, selEnd;
      _getSelectionRange(selStart, selEnd);

      if (!mMultiLine)
      {
         // Single line: exactly one GuiTextLine, so the selection rect
         // is just [measure(0..selStart), measure(0..selEnd)] on that
         // one line.
         if (!layoutResult.lines.empty())
         {
            const GuiTextLine& line = layoutResult.lines[0];
            const S32 xStart = GuiTextMetrics::measure(font, line.chars.address(), (U32)getMin((S32)line.chars.size(), selStart), 0, 0);
            const S32 xEnd = GuiTextMetrics::measure(font, line.chars.address(), (U32)getMin((S32)line.chars.size(), selEnd), 0, 0);

            const RectI selRect(
               Point2I(offset.x + line.origin.x + xStart, offset.y + line.origin.y),
               Point2I(xEnd - xStart, line.height));
            batch.pushQuad(selRect, kDefaultSelectionColor, getRenderLayer());
         }
      }
      else
      {
         // Multi-line: one rect PER LINE the selection touches, via
         // _getSelectionRangeForLine() intersecting the flat
         // [selStart, selEnd) range against each line's own flat-index
         // span. A line the selection only partially covers gets a
         // partial-width rect (measured the same GuiTextMetrics way as
         // single-line, just against that line's own local range); a
         // line the selection fully spans gets a full-width rect,
         // covering the (invisible) consumed '\n' at its end too --
         // matching how a real text editor highlights a fully-selected
         // line, trailing newline included, not just its visible glyphs.
         for (U32 i = 0; i < layoutResult.lines.size(); ++i)
         {
            U32 localStart, localEnd;
            if (!_getSelectionRangeForLine(i, selStart, selEnd, localStart, localEnd))
               continue; // selection doesn't touch this line

            const GuiTextLine& line = layoutResult.lines[i];
            const S32 xStart = GuiTextMetrics::measure(font, line.chars.address(), localStart, 0, 0);

            // A line fully covered by the selection (both its local
            // range hits the line's own end AND the selection continues
            // past this line into the next) draws full-width rather
            // than stopping at the last glyph's right edge -- otherwise
            // a selected blank line, or a selected line ending mid-word
            // wrap, would show no highlight/a short highlight that
            // doesn't visually read as "this whole line is selected".
            const bool coversRestOfLine = (localEnd >= (U32)line.chars.size()) && (selEnd > _findFlatIndexFromLineAndOffset(i, line.chars.size()));

            S32 xEnd;
            if (coversRestOfLine)
               xEnd = ctrlRect.extent.x - line.origin.x; // fill to the box's right edge, in line-local x
            else
               xEnd = GuiTextMetrics::measure(font, line.chars.address(), localEnd, 0, 0);

            const RectI selRect(
               Point2I(offset.x + line.origin.x + xStart, offset.y + line.origin.y),
               Point2I(getMax(0, xEnd - xStart), line.height));
            batch.pushQuad(selRect, kDefaultSelectionColor, getRenderLayer());
         }
      }
   }

   renderText(mGuiText, offset, ctrlRect.extent);

   // Caret, drawn ON TOP of the text.
   if (isFirstResponder() && mCursorBlinkOn && font)
   {
      Point2I caretPosLocal(0, 0);

      if (!mMultiLine)
      {
         if (!layoutResult.lines.empty())
         {
            const GuiTextLine& line = layoutResult.lines[0];
            const S32 x = GuiTextMetrics::measure(font, line.chars.address(), (U32)getMin((S32)line.chars.size(), mCursorPos), 0, 0);
            caretPosLocal = Point2I(line.origin.x + x, line.origin.y);
         }
      }
      else
      {
         U32 lineIndex, lineLocalOffset;
         if (_findCaretLineAndOffset(mCursorPos, lineIndex, lineLocalOffset) && lineIndex < layoutResult.lines.size())
         {
            const GuiTextLine& line = layoutResult.lines[lineIndex];
            const S32 x = GuiTextMetrics::measure(font, line.chars.address(), lineLocalOffset, 0, 0);
            caretPosLocal = Point2I(line.origin.x + x, line.origin.y);
         }
      }

      const S32 caretHeight = (S32)font->getHeight();
      const Point2I caretTop = offset + caretPosLocal;
      batch.pushLine(caretTop, Point2I(caretTop.x, caretTop.y + caretHeight), kDefaultCaretColor, 1.0f, getRenderLayer() + 1);
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::setTextProt(void* object, const char* index, const char* data)
{
   static_cast<GuiTextEditCtrlNew*>(object)->setText(data);
   return false;
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::setMultiLineProt(void* object, const char* index, const char* data)
{
   static_cast<GuiTextEditCtrlNew*>(object)->setMultiLine(dAtob(data));
   return false;
}

//-----------------------------------------------------------------------------

bool GuiTextEditCtrlNew::setPasswordMaskProt(void* object, const char* index, const char* data)
{
   static_cast<GuiTextEditCtrlNew*>(object)->setPasswordMask(dAtob(data));
   return false;
}
