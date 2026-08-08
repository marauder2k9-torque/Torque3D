//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiTextEdit.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiTextEdit.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gui_rev2/core/newGuiCanvas.h"
#include "sim/sim.h"

IMPLEMENT_CONOBJECT(NewGuiTextEdit);

IMPLEMENT_CALLBACK(NewGuiTextEdit, onCommit, void, (), (),
   "Called when Enter is pressed while multiLine == false. Never fires in multi-line mode - "
   "Enter inserts a newline there instead.");

IMPLEMENT_CALLBACK(NewGuiTextEdit, onTextChanged, void, (), (),
   "Called whenever this control's content changes as a result of user input (typing, paste, "
   "cut, Backspace/Delete). Does NOT fire from a script-driven setText() call.");

static const S32 kDefaultCaretBlinkIntervalMS = 500;

NewGuiTextEdit::NewGuiTextEdit()
   : mCachedFontFamily(NULL),
   mCachedFontSize(0.0f),
   mMultiLine(false),
   mMaxLength(0),
   mCaretPos(0),
   mSelectionAnchor(0),
   mCaretBlinkStartMS(0),
   mCaretBlinkIntervalMS(kDefaultCaretBlinkIntervalMS),
   mDragSelecting(false)
{
   mText.setAlignHorizontal(NewGuiTextAlignHorizontal::Left);
   mText.setAlignVertical(NewGuiTextAlignVertical::Top);
   mText.setOverflow(NewGuiTextOverflow_Clip);
}

NewGuiTextEdit::~NewGuiTextEdit()
{
}

// --- Field setters / initPersistFields ----------------------------------------------------

bool NewGuiTextEdit::_setText(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTextEdit*>(obj)->setText(data);
   return false;
}

bool NewGuiTextEdit::_setMultiLine(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTextEdit*>(obj)->setMultiLine(dAtob(data));
   return false;
}

bool NewGuiTextEdit::_setMaxLength(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTextEdit*>(obj)->setMaxLength(dAtoi(data));
   return false;
}

void NewGuiTextEdit::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("TextEdit");

   ADD_FIELD("text", TypeString, 0)
      .onSet(_setText)
      .doc("This field's current text content.");

   ADD_FIELD("multiLine", TypeBool, Offset(mMultiLine, NewGuiTextEdit))
      .onSet(_setMultiLine)
      .doc("False (default): single line, Enter fires onCommit, pasted newlines collapse to a space. True: multiple lines, Enter inserts a newline, no onCommit.");

   ADD_FIELD("maxLength", TypeS32, Offset(mMaxLength, NewGuiTextEdit))
      .onSet(_setMaxLength)
      .doc("Maximum character count. <= 0 (default) means unlimited. Enforced for both typed input and paste; does not retroactively truncate existing content if lowered.");

   GROUP_END("TextEdit");
}

// --- Font ------------------------------------------------------------------------------------

// Identical shape to NewGuiButton::resolveFont().
void NewGuiTextEdit::resolveFont()
{
   const NewGuiResolvedStyle& style = getResolvedStyle();

   if (mFont != NULL && style.fontFamily == mCachedFontFamily && style.fontSize == mCachedFontSize)
      return;

   const char* faceName = style.fontFamily ? style.fontFamily : "Arial";
   U32 size = (U32)(style.fontSize > 0.0f ? style.fontSize : 14.0f);

   mFont = GFont::create(faceName, size);
   mCachedFontFamily = style.fontFamily;
   mCachedFontSize = style.fontSize;

   mText.setFont(mFont);
}

// The one place mText's box extent is configured and layout() is invoked outside
// ComputePreferredSize()'s own measurement pass (see that method's comment on why it's
// separate).
const NewGuiTextLayoutResult& NewGuiTextEdit::layoutClientText(RectI& outClientRect)
{
   const NewGuiResolvedStyle& style = getResolvedStyle();

   outClientRect = RectI(
      Point2I((S32)style.padding.left, (S32)style.padding.top),
      Point2I(getMax(0, mBounds.extent.x - (S32)style.padding.horizontal()),
         getMax(0, mBounds.extent.y - (S32)style.padding.vertical())));

   mText.setWrap(mMultiLine);
   mText.setBoxExtent(outClientRect.extent);
   return mText.layout();
}

// --- Buffer <-> display text sync -------------------------------------------------------------

// UTF-16 -> UTF-8 encode (standard RFC 3629 byte-length rules), written self-contained since no
// confirmed engine primitive covers this direction (convertUTF8toUTF16N() only decodes the
// other way). BMP-range only, matching that function's own apparent scope.
static void EncodeUTF16ToUTF8(const Vector<UTF16>& chars, String& outText)
{
   Vector<char> bytes;
   bytes.reserve(chars.size() * 3 + 1);

   for (U32 i = 0; i < chars.size(); i++)
   {
      UTF16 c = chars[i];

      if (c < 0x80)
      {
         bytes.push_back((char)c);
      }
      else if (c < 0x800)
      {
         bytes.push_back((char)(0xC0 | (c >> 6)));
         bytes.push_back((char)(0x80 | (c & 0x3F)));
      }
      else
      {
         bytes.push_back((char)(0xE0 | (c >> 12)));
         bytes.push_back((char)(0x80 | ((c >> 6) & 0x3F)));
         bytes.push_back((char)(0x80 | (c & 0x3F)));
      }
   }

   bytes.push_back(0);
   outText = String(bytes.address());
}

void NewGuiTextEdit::syncDisplayText()
{
   String encoded;
   EncodeUTF16ToUTF8(mBuffer, encoded);
   mText.setText(encoded);

   setContentDirty();
   setArrangementDirty();
}

void NewGuiTextEdit::setText(const char* text)
{
   mBuffer.clear();

   if (text && text[0])
   {
      const U32 byteLen = dStrlen(text);
      Vector<UTF16> decoded;
      decoded.setSize(byteLen + 1);
      convertUTF8toUTF16N(text, decoded.address(), byteLen + 1);

      for (U32 i = 0; i < byteLen + 1 && decoded[i] != 0; i++)
         mBuffer.push_back(decoded[i]);
   }

   mCaretPos = mBuffer.size();
   mSelectionAnchor = mCaretPos;

   syncDisplayText();
}

const char* NewGuiTextEdit::getText() const
{
   return mText.getText().c_str();
}

// --- Editing primitives -----------------------------------------------------------------------

bool NewGuiTextEdit::canInsert(U32 additionalChars) const
{
   if (mMaxLength <= 0)
      return true;

   U32 selectionLen = (mCaretPos > mSelectionAnchor) ? (mCaretPos - mSelectionAnchor) : (mSelectionAnchor - mCaretPos);
   U32 lengthAfterDelete = mBuffer.size() - selectionLen;

   return (lengthAfterDelete + additionalChars) <= (U32)mMaxLength;
}

bool NewGuiTextEdit::wouldOverflowVertically() const
{
   RectI unusedClientRect;
   const NewGuiTextLayoutResult& layout = const_cast<NewGuiTextEdit*>(this)->layoutClientText(unusedClientRect);
   return layout.didOverflow;
}

bool NewGuiTextEdit::deleteSelection()
{
   if (mCaretPos == mSelectionAnchor)
      return false;

   U32 start = getMin(mCaretPos, mSelectionAnchor);
   U32 end = getMax(mCaretPos, mSelectionAnchor);

   // Single-index erase(i), repeated - Vector::erase() elsewhere in this codebase is
   // single-index only. Erasing from 'start' repeatedly is correct because each erase shifts
   // everything after it left by one.
   for (U32 i = start; i < end; i++)
      mBuffer.erase(start);

   mCaretPos = start;
   mSelectionAnchor = start;
   return true;
}

bool NewGuiTextEdit::insertAt(const UTF16* chars, U32 count)
{
   if (count == 0)
      return true;

   // Snapshot for rollback - wrap/newline placement isn't known until the text is actually in
   // mBuffer and re-laid-out, so overflow can only be checked after the fact.
   Vector<UTF16> bufferBefore = mBuffer;
   U32 caretBefore = mCaretPos;
   U32 anchorBefore = mSelectionAnchor;

   deleteSelection();

   for (U32 i = 0; i < count; i++)
      mBuffer.insert(mCaretPos + i, chars[i]);

   mCaretPos += count;
   mSelectionAnchor = mCaretPos;

   syncDisplayText();

   if (wouldOverflowVertically())
   {
      mBuffer = bufferBefore;
      mCaretPos = caretBefore;
      mSelectionAnchor = anchorBefore;
      syncDisplayText();
      return false;
   }

   onTextChanged_callback();
   return true;
}

void NewGuiTextEdit::insertCharacter(U16 ascii)
{
   if (ascii == 0)
      return;

   if (ascii == '\n' || ascii == '\r')
   {
      if (!mMultiLine)
         return;
   }

   if (!canInsert(1))
      return;

   UTF16 c = (UTF16)ascii;
   insertAt(&c, 1);
}

// --- Word boundary / line-column mapping --------------------------------------------------

U32 NewGuiTextEdit::findWordBoundary(U32 fromIndex, bool forward) const
{
   U32 count = mBuffer.size();

   if (forward)
   {
      U32 i = fromIndex;
      while (i < count && dIsspace((char)mBuffer[i]))
         i++;
      while (i < count && !dIsspace((char)mBuffer[i]))
         i++;
      return i;
   }
   else
   {
      if (fromIndex == 0)
         return 0;

      U32 i = fromIndex;
      while (i > 0 && dIsspace((char)mBuffer[i - 1]))
         i--;
      while (i > 0 && !dIsspace((char)mBuffer[i - 1]))
         i--;
      return i;
   }
}

// The upper-bound test is inclusive (index <= consumed+lineLen) - the index that equals "end of
// this line's real characters" (a wrap-consumed space, or a consumed '\n') must resolve to end
// of THIS line, not the start of the next one.
bool NewGuiTextEdit::mapIndexToLineColumn(U32 index, S32& outLine, S32& outColumn) const
{
   const NewGuiTextLayoutResult& layout = const_cast<NewGuiText&>(mText).layout();

   if (layout.lines.empty())
      return false;

   U32 consumed = 0;
   for (U32 line = 0; line < layout.lines.size(); line++)
   {
      consumed += layout.lines[line].consumedCharsBeforeThisLine;

      U32 lineLen = layout.lines[line].chars.size();
      bool isLastLine = (line == layout.lines.size() - 1);
      U32 upperBound = consumed + lineLen;

      if (index <= upperBound || isLastLine)
      {
         outLine = (S32)line;
         outColumn = (S32)mClamp((S32)(index - consumed), 0, (S32)lineLen);
         return true;
      }

      consumed += lineLen;
   }

   return false;
}

U32 NewGuiTextEdit::mapLineColumnToIndex(S32 line, S32 column) const
{
   const NewGuiTextLayoutResult& layout = const_cast<NewGuiText&>(mText).layout();

   if (layout.lines.empty())
      return 0;

   line = mClamp(line, 0, (S32)layout.lines.size() - 1);

   U32 consumed = 0;
   for (S32 i = 0; i <= line; i++)
      consumed += layout.lines[i].consumedCharsBeforeThisLine;
   for (S32 i = 0; i < line; i++)
      consumed += layout.lines[i].chars.size();

   U32 lineLen = layout.lines[line].chars.size();
   column = mClamp(column, 0, (S32)lineLen);

   return consumed + (U32)column;
}

// --- Clipboard ---------------------------------------------------------------------------------

void NewGuiTextEdit::copySelectionToClipboard()
{
   if (mCaretPos == mSelectionAnchor)
      return;

   U32 start = getMin(mCaretPos, mSelectionAnchor);
   U32 end = getMax(mCaretPos, mSelectionAnchor);

   Vector<UTF16> selected;
   for (U32 i = start; i < end; i++)
      selected.push_back(mBuffer[i]);

   String encoded;
   EncodeUTF16ToUTF8(selected, encoded);
   Platform::setClipboard(encoded.c_str());
}

void NewGuiTextEdit::cutSelectionToClipboard()
{
   if (mCaretPos == mSelectionAnchor)
      return;

   copySelectionToClipboard();
   deleteSelection();
   syncDisplayText();
   onTextChanged_callback();
}

void NewGuiTextEdit::pasteFromClipboard()
{
   const char* clipboard = Platform::getClipboard();
   if (!clipboard || !clipboard[0])
      return;

   const U32 byteLen = dStrlen(clipboard);
   Vector<UTF16> decoded;
   decoded.setSize(byteLen + 1);
   convertUTF8toUTF16N(clipboard, decoded.address(), byteLen + 1);

   Vector<UTF16> filtered;
   for (U32 i = 0; i < byteLen + 1 && decoded[i] != 0; i++)
   {
      UTF16 c = decoded[i];

      if (c == '\r')
         continue;   // Collapse CRLF to LF before applying the mMultiLine policy below.

      if (c == '\n' && !mMultiLine)
         c = ' ';

      filtered.push_back(c);
   }

   if (filtered.empty())
      return;

   if (mMaxLength > 0)
   {
      U32 selectionLen = (mCaretPos > mSelectionAnchor) ? (mCaretPos - mSelectionAnchor) : (mSelectionAnchor - mCaretPos);
      U32 remaining = (U32)mMaxLength - getMin((U32)mMaxLength, mBuffer.size() - selectionLen);
      if (filtered.size() > remaining)
         filtered.setSize(remaining);
   }

   if (filtered.empty())
      return;

   // insertAt() rolls back the whole edit if the box would overflow vertically. Rather than
   // accept an all-or-nothing result, progressively shorten filtered (from the end) and retry
   // until it fits - so a paste too long for the box still lands as much as actually fits.
   // Character-by-character is deliberately simple (not a binary search) since paste content is
   // small relative to a UI text field and this only runs once per paste, not per keystroke.
   while (!filtered.empty() && !insertAt(filtered.address(), filtered.size()))
      filtered.setSize(filtered.size() - 1);
}

// Computed fresh from elapsed sim time every call, never stored/toggled - keeps
// EmitDrawCommands() a pure consumer of already-resolved state.
bool NewGuiTextEdit::mCaretBlinkPhaseIsVisible() const
{
   if (mCaretBlinkIntervalMS <= 0)
      return true;

   U32 elapsed = (U32)Sim::getCurrentTime() - mCaretBlinkStartMS;
   return (elapsed / (U32)mCaretBlinkIntervalMS) % 2 == 0;
}

// --- Sizing / rendering -----------------------------------------------------------------------

Point2I NewGuiTextEdit::ComputePreferredSize()
{
   resolveFont();

   const NewGuiResolvedStyle& style = getResolvedStyle();

   // Doesn't go through layoutClientText() - mBounds isn't resolved yet during MeasurePass.
   // Single-line measures unbounded; multi-line measures against the authored width (if fixed)
   // so wrap is accounted for in the height.
   Point2I measureExtent(0, 0);
   if (mMultiLine && getAuthoredWidth().mode != NewGuiDimension::Auto)
      measureExtent.x = getMax(0, mBounds.extent.x - (S32)style.padding.horizontal());

   mText.setWrap(mMultiLine);
   mText.setBoxExtent(measureExtent);
   const NewGuiTextLayoutResult& result = mText.layout();

   S32 textWidth = 0;
   S32 textHeight = (mFont != NULL) ? (S32)mFont->getHeight() : 0;

   if (!result.lines.empty())
   {
      textWidth = result.blockBounds.extent.x;
      textHeight = getMax(textHeight, result.blockBounds.extent.y);
   }

   S32 width = textWidth + (S32)style.padding.horizontal();
   S32 height = textHeight + (S32)style.padding.vertical();

   return Point2I(width, height);
}

void NewGuiTextEdit::EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer)
{
   Parent::EmitDrawCommands(batch, bounds, style, layer);

   if (!batch || mFont == NULL || style.opacity <= 0.0f)
      return;

   // NewGuiTextMetrics::measure()/fitCount() take GFont*, not Resource<GFont>.
   GFont* rawFont = mFont;

   // layoutClientText() returns a local-space rect; add bounds.point to get device space.
   RectI localClientRect;
   const NewGuiTextLayoutResult& layoutResult = layoutClientText(localClientRect);
   const RectI clientRect(bounds.point + localClientRect.point, localClientRect.extent);

   // Selection highlight - one quad per selected line-span.
   if (mCaretPos != mSelectionAnchor && mFont != NULL)
   {
      U32 selStart = getMin(mCaretPos, mSelectionAnchor);
      U32 selEnd = getMax(mCaretPos, mSelectionAnchor);

      S32 startLine, startCol, endLine, endCol;
      if (mapIndexToLineColumn(selStart, startLine, startCol) && mapIndexToLineColumn(selEnd, endLine, endCol))
      {
         for (S32 line = startLine; line <= endLine; line++)
         {
            const NewGuiTextLine& lineInfo = layoutResult.lines[line];

            S32 colFrom = (line == startLine) ? startCol : 0;
            S32 colTo = (line == endLine) ? endCol : (S32)lineInfo.chars.size();

            S32 xFrom = NewGuiTextMetrics::measure(rawFont, lineInfo.chars.address(), colFrom, 0, 0);
            S32 xTo = NewGuiTextMetrics::measure(rawFont, lineInfo.chars.address(), colTo, 0, 0);

            RectI highlightRect(
               Point2I(clientRect.point.x + lineInfo.origin.x + xFrom, clientRect.point.y + lineInfo.origin.y),
               Point2I(getMax(0, xTo - xFrom), lineInfo.height));

            batch->pushQuad(highlightRect, style.borderColor, layer);
         }
      }
   }

   ColorI textColor(
      style.textColor.red,
      style.textColor.green,
      style.textColor.blue,
      (U8)((F32)style.textColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));

   mText.submit(*batch, clientRect.point, textColor, layer);

   if (isFirstResponder() && mCaretBlinkPhaseIsVisible() && mFont != NULL)
   {
      S32 caretLine, caretCol;
      if (mapIndexToLineColumn(mCaretPos, caretLine, caretCol))
      {
         const NewGuiTextLine& lineInfo = layoutResult.lines[caretLine];
         S32 caretX = NewGuiTextMetrics::measure(rawFont, lineInfo.chars.address(), caretCol, 0, 0);

         Point2I caretTop(clientRect.point.x + lineInfo.origin.x + caretX, clientRect.point.y + lineInfo.origin.y);
         Point2I caretBottom(caretTop.x, caretTop.y + lineInfo.height);

         batch->pushLine(caretTop, caretBottom, textColor, 1.0f, layer);
      }
   }
}

// --- Mouse ---------------------------------------------------------------------------------

U32 NewGuiTextEdit::hitTestTextPosition(const NewGuiTextLayoutResult& layout, const Point2I& textLocalPoint)
{
   if (layout.lines.empty() || mFont == NULL)
      return 0;

   GFont* rawFont = mFont;

   S32 targetLine = 0;
   for (U32 i = 0; i < layout.lines.size(); i++)
   {
      targetLine = (S32)i;
      if (textLocalPoint.y < layout.lines[i].origin.y + layout.lines[i].height)
         break;
   }

   const NewGuiTextLine& lineInfo = layout.lines[targetLine];
   S32 relativeX = textLocalPoint.x - lineInfo.origin.x;

   S32 column;
   if (relativeX <= 0)
   {
      column = 0;
   }
   else
   {
      S32 fitWidth = 0;
      column = (S32)NewGuiTextMetrics::fitCount(rawFont, lineInfo.chars.address(), lineInfo.chars.size(), relativeX, 0, 0, fitWidth);
   }

   return mapLineColumnToIndex(targetLine, column);
}

void NewGuiTextEdit::onMouseDown(NewGuiInputEvent& event)
{
   // Claim keyboard focus - nothing in the base control tree does this on click by default
   // (setFirstResponderControl() is otherwise only called from Tab navigation).
   NewGuiCanvas* canvas = getOwningCanvas();
   if (canvas)
      canvas->setFirstResponderControl(this);

   RectI clientRect;
   const NewGuiTextLayoutResult& layoutResult = layoutClientText(clientRect);
   Point2I textLocalPoint(event.localPoint.x - clientRect.point.x, event.localPoint.y - clientRect.point.y);

   U32 hitIndex = hitTestTextPosition(layoutResult, textLocalPoint);

   if (event.clickCount >= 3)
   {
      S32 line, column;
      if (mapIndexToLineColumn(hitIndex, line, column))
      {
         mSelectionAnchor = mapLineColumnToIndex(line, 0);
         mCaretPos = mapLineColumnToIndex(line, (S32)layoutResult.lines[line].chars.size());
      }
   }
   else if (event.clickCount == 2)
   {
      mSelectionAnchor = findWordBoundary(hitIndex, false);
      mCaretPos = findWordBoundary(hitIndex, true);
   }
   else
   {
      mCaretPos = hitIndex;
      mSelectionAnchor = hitIndex;
   }

   mDragSelecting = true;
   mCaretBlinkStartMS = (U32)Sim::getCurrentTime();

   setStyleDirty();
   event.handled = true;
}

void NewGuiTextEdit::onMouseUp(NewGuiInputEvent& event)
{
   mDragSelecting = false;
   event.handled = true;
}

// --- Keyboard / char input ------------------------------------------------------------------

void NewGuiTextEdit::onInputEvent(NewGuiInputEvent& event)
{
   if (event.deviceKind == NewGuiDeviceKind::Mouse && event.action == NewGuiInputAction::Move && mDragSelecting)
   {
      RectI clientRect;
      const NewGuiTextLayoutResult& layoutResult = layoutClientText(clientRect);
      Point2I textLocalPoint(event.localPoint.x - clientRect.point.x, event.localPoint.y - clientRect.point.y);

      mCaretPos = hitTestTextPosition(layoutResult, textLocalPoint);
      setStyleDirty();
      event.handled = true;
      return;
   }

   if (event.deviceKind != NewGuiDeviceKind::Keyboard)
      return;

   if (event.isCharInput)
   {
      insertCharacter(event.ascii);
      mCaretBlinkStartMS = (U32)Sim::getCurrentTime();
      setStyleDirty();
      event.handled = true;
      return;
   }

   if (event.action != NewGuiInputAction::Down && event.action != NewGuiInputAction::Repeat)
      return;

   // Ensures mText is configured against this control's current bounds before any
   // mapIndexToLineColumn()/mapLineColumnToIndex() call below runs.
   RectI unusedClientRect;
   layoutClientText(unusedClientRect);

   bool extendSelection = (event.modifier & SI_RANGESELECT) != 0;
   bool wordJump = (event.modifier & SI_WORDJUMP) != 0;
   bool docJump = (event.modifier & SI_DOCJUMP) != 0;

   U32 newCaretPos = mCaretPos;
   bool moved = false;
   bool handled = true;

   switch (event.keyCode)
   {
   case KEY_LEFT:
      newCaretPos = wordJump ? findWordBoundary(mCaretPos, false) : (mCaretPos > 0 ? mCaretPos - 1 : 0);
      moved = true;
      break;

   case KEY_RIGHT:
      newCaretPos = wordJump ? findWordBoundary(mCaretPos, true) : getMin(mCaretPos + 1, mBuffer.size());
      moved = true;
      break;

   case KEY_UP:
   case KEY_DOWN:
   {
      S32 line, column;
      if (mapIndexToLineColumn(mCaretPos, line, column))
      {
         line += (event.keyCode == KEY_UP) ? -1 : 1;
         newCaretPos = mapLineColumnToIndex(line, column);
         moved = true;
      }
      break;
   }

   case KEY_HOME:
   {
      if (docJump)
      {
         newCaretPos = 0;
      }
      else
      {
         S32 line, column;
         if (mapIndexToLineColumn(mCaretPos, line, column))
            newCaretPos = mapLineColumnToIndex(line, 0);
      }
      moved = true;
      break;
   }

   case KEY_END:
   {
      if (docJump)
      {
         newCaretPos = mBuffer.size();
      }
      else
      {
         S32 line, column;
         if (mapIndexToLineColumn(mCaretPos, line, column))
         {
            const NewGuiTextLayoutResult& layoutResult = const_cast<NewGuiText&>(mText).layout();
            if (line >= 0 && line < (S32)layoutResult.lines.size())
               newCaretPos = mapLineColumnToIndex(line, (S32)layoutResult.lines[line].chars.size());
         }
      }
      moved = true;
      break;
   }

   case KEY_PAGE_UP:
   case KEY_PAGE_DOWN:
   {
      // Pages by the control's own visible line count, at least one line.
      S32 line, column;
      if (mapIndexToLineColumn(mCaretPos, line, column) && mFont != NULL)
      {
         S32 lineHeight = (S32)mFont->getHeight();
         S32 visibleLines = (lineHeight > 0) ? getMax(1, mBounds.extent.y / lineHeight) : 1;
         line += (event.keyCode == KEY_PAGE_UP) ? -visibleLines : visibleLines;
         newCaretPos = mapLineColumnToIndex(line, column);
         moved = true;
      }
      break;
   }

   case KEY_BACKSPACE:
      if (mCaretPos != mSelectionAnchor)
      {
         deleteSelection();
      }
      else if (mCaretPos > 0)
      {
         mBuffer.erase(mCaretPos - 1);
         mCaretPos--;
         mSelectionAnchor = mCaretPos;
      }
      syncDisplayText();
      onTextChanged_callback();
      break;

   case KEY_DELETE:
      if (mCaretPos != mSelectionAnchor)
      {
         deleteSelection();
      }
      else if (mCaretPos < mBuffer.size())
      {
         mBuffer.erase(mCaretPos);
      }
      syncDisplayText();
      onTextChanged_callback();
      break;

   case KEY_RETURN:
   case KEY_NUMPADENTER:
      if (mMultiLine)
      {
         UTF16 newline = '\n';
         if (canInsert(1))
            insertAt(&newline, 1);
      }
      else
      {
         onCommit_callback();
         notifyNativeChange();
      }
      break;

   default:
      // SI_COPYPASTE (Ctrl normally, Cmd/Alt on Mac) + C/X/V - checked by keyCode since this is
      // the raw-key channel. Never registered as a canvas accelerator; handled locally here.
      if ((event.modifier & SI_COPYPASTE) != 0)
      {
         if (event.keyCode == KEY_C)
            copySelectionToClipboard();
         else if (event.keyCode == KEY_X)
            cutSelectionToClipboard();
         else if (event.keyCode == KEY_V)
            pasteFromClipboard();
         else
            handled = false;
      }
      else
      {
         handled = false;
      }
      break;
   }

   if (moved)
   {
      mCaretPos = newCaretPos;
      if (!extendSelection)
         mSelectionAnchor = mCaretPos;

      mCaretBlinkStartMS = (U32)Sim::getCurrentTime();
      setStyleDirty();
   }

   event.handled = handled;
}

// --- Focus lifecycle -------------------------------------------------------------------------

void NewGuiTextEdit::setFirstResponder(bool responder)
{
   Parent::setFirstResponder(responder);

   NewGuiCanvas* canvas = getOwningCanvas();
   if (canvas)
   {
      canvas->enableKeyboardAccelerators(!responder);
      canvas->enableKeyboardTranslation(responder);
   }

   if (responder)
      mCaretBlinkStartMS = (U32)Sim::getCurrentTime();

   setStyleDirty();
}

//-----------------------------------------------------------------------------
// Script (console) API
//-----------------------------------------------------------------------------
// Same gap as NewGuiLabel (see that file's own comment on this section) - "text" is an
// authorable field via ADD_FIELD(...).onSet(_setText), but setText()/getText() had no
// script-callable method form of their own. Particularly relevant here since NewGuiTreeRow's
// inline-rename edit control is exactly this class - a test/tool script driving a rename
// programmatically (rather than through the mouse) needs setText()/getText() as real methods.

DefineEngineMethod(NewGuiTextEdit, setText, void, (const char* text), ,
   "Sets the full text content, replacing any current buffer/selection and moving the caret to "
   "the end.\n"
   "@ingroup GuiCore")
{
   object->setText(text);
}

DefineEngineMethod(NewGuiTextEdit, getText, const char*, (), ,
   "@return The current text content, UTF-8.\n"
   "@ingroup GuiCore")
{
   return object->getText();
}
