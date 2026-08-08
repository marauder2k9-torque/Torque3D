//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiTextEdit.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUITEXTEDIT_H_
#define _NEWGUITEXTEDIT_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#ifndef _NEWGUITEXT_H_
#include "gui_rev2/core/newGuiText.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif

/// An editable text field - single-line or multi-line, caret + selection, mouse click/drag to
/// place caret or select, full keyboard navigation (arrows, Home/End, PageUp/PageDown, word/
/// line/doc jump via SI_WORDJUMP/SI_LINEJUMP/SI_DOCJUMP), and clipboard copy/cut/paste
/// (SI_COPYPASTE). See gui_rev2's text-edit design note for the input-contract reasoning -
/// raw key vs decoded char are two distinct channels (NewGuiInputEvent::isCharInput), and
/// focus gain/loss suppresses the canvas's keyboard-accelerator table
/// (wantsRawKeyboardInput()) and the platform window's native OS accelerators
/// (setFirstResponder()).
///
/// Caret/selection are flat UTF-16 character indices into mBuffer, not byte offsets into the
/// authored UTF-8 string. A line/column view is derived on demand from the current NewGuiText
/// layout (mapIndexToLineColumn()/mapLineColumnToIndex()) rather than cached separately, so it
/// can't drift from what's actually wrapped on screen.
///
/// @code
/// new NewGuiTextEdit( NameField )
/// {
///    width = "200"; multiLine = "false"; maxLength = "64";
///    text = "default value";
/// };
/// @endcode
class NewGuiTextEdit : public NewGuiControl
{
public:

   typedef NewGuiControl Parent;

protected:

   NewGuiText mText;                   ///< Display/layout/measurement - same ownership pattern as NewGuiButton::mText.
   Resource<GFont> mFont;
   StringTableEntry mCachedFontFamily;
   F32 mCachedFontSize;

   /// Decoded UTF-16 working buffer - the actual editable content. mText's authored string is
   /// re-derived from this after every edit (see syncDisplayText()) rather than spliced
   /// incrementally.
   Vector<UTF16> mBuffer;

   bool mMultiLine;                    ///< Authored. False (default): single line, Enter fires onCommit, pasted newlines collapse to a space.
   S32 mMaxLength;                     ///< Authored. <= 0 means unlimited. Enforced identically for typed input and paste - see canInsert().

   U32 mCaretPos;                      ///< Flat UTF-16 index into mBuffer, [0, mBuffer.size()].
   U32 mSelectionAnchor;               ///< Flat index selection was extended from. mCaretPos == mSelectionAnchor means no selection.

   bool mCaretBlinkPhaseIsVisible() const;   ///< Computed from elapsed time at read time - never stored/toggled, so EmitDrawCommands() stays a pure consumer.
   U32 mCaretBlinkStartMS;                   ///< Sim time the current blink phase began - reset on focus gain and on caret movement.
   S32 mCaretBlinkIntervalMS;                ///< Blink half-period.

   /// True from a bounds-landing onMouseDown() until the matching onMouseUp() - while true,
   /// Move events extend the selection from mSelectionAnchor to whatever's under the pointer.
   bool mDragSelecting;

   static bool _setText(void* obj, const char* index, const char* data);
   static bool _setMultiLine(void* obj, const char* index, const char* data);
   static bool _setMaxLength(void* obj, const char* index, const char* data);

   /// Loads/caches the font matching the resolved style - identical shape to NewGuiButton::resolveFont().
   void resolveFont();

   /// Configures mText (wrap/box extent, from this control's padding-inset bounds) and returns
   /// the current layout(). The one place that happens - every hit-test/render call site
   /// funnels through here so they can't disagree about what box the layout was computed
   /// against.
   /// @param outClientRect Receives the padding-inset rect, in local space (origin at
   /// padding.left/top). Callers must offset any input localPoint by this before hit-testing.
   const NewGuiTextLayoutResult& layoutClientText(RectI& outClientRect);

   /// Re-derives mText's authored string from mBuffer and marks content/arrangement dirty.
   void syncDisplayText();

   /// @return True if inserting additionalChars more characters would still fit mMaxLength
   /// (accounting for any currently-selected range, which insertion would first replace).
   /// mMaxLength <= 0 always returns true.
   bool canInsert(U32 additionalChars) const;

   /// @return True if mText's current layout (against this control's actual box) doesn't fit -
   /// NewGuiTextLayoutResult::didOverflow, checked fresh via layoutClientText() rather than
   /// cached, since it depends on whatever mBuffer/mBounds currently are. mMaxLength guards
   /// character count; this guards vertical space in a fixed-height box, which mMaxLength alone
   /// doesn't cover (a short buffer of many blank lines can still overflow a box's height).
   bool wouldOverflowVertically() const;

   /// Deletes the current selection (no-op if none) and collapses the caret to the deletion point.
   /// @return True if a selection was actually deleted.
   bool deleteSelection();

   /// Splices already-decoded, newline-policy-resolved text into mBuffer at mCaretPos,
   /// replacing any active selection first, then rolls the whole edit back if the result would
   /// overflow the box vertically (wouldOverflowVertically()) - caller must have already
   /// checked canInsert() for the character-count limit, but a box can also run out of vertical
   /// room independent of mMaxLength, and unlike the count-based limit, there's no meaningful
   /// per-character invariant to check ahead of time (wrap/newline placement isn't known until
   /// after the text is actually in the buffer).
   /// @return True if the insertion was kept; false if it was rolled back for overflowing.
   bool insertAt(const UTF16* chars, U32 count);

   /// Inserts one decoded character at the caret, subject to canInsert(). A stray '\n'/'\r'
   /// follows the same mMultiLine policy as pasteFromClipboard()'s embedded-newline handling.
   /// @param ascii Decoded character from NewGuiInputEvent::ascii.
   void insertCharacter(U16 ascii);

   /// @return The previous/next word-boundary index from fromIndex, via dIsspace()/dIsalnum()
   /// (ASCII/Latin-1 range only). Used by SI_WORDJUMP handling.
   U32 findWordBoundary(U32 fromIndex, bool forward) const;

   /// Maps a flat mBuffer index to a (line, column) pair against the current NewGuiText layout.
   /// Accounts for NewGuiTextLine::consumedCharsBeforeThisLine - a wrap-consumed space or a
   /// real '\n' is removed from the source but never stored in any line's chars array, so line
   /// length alone under-counts at every line boundary.
   /// @return False if there's no current layout (e.g. never measured yet).
   bool mapIndexToLineColumn(U32 index, S32& outLine, S32& outColumn) const;

   /// Inverse of mapIndexToLineColumn() - same consumedCharsBeforeThisLine accounting, clamps
   /// column to the target line's actual length.
   U32 mapLineColumnToIndex(S32 line, S32 column) const;

   /// Maps a click point (already in text-local space) to the nearest character boundary: finds
   /// the line by Y, the column within it by X (via NewGuiTextMetrics::fitCount()), then
   /// resolves the flat index via mapLineColumnToIndex() rather than re-deriving line-length
   /// accounting independently.
   U32 hitTestTextPosition(const NewGuiTextLayoutResult& layout, const Point2I& textLocalPoint);

   /// Copies the current selection to the platform clipboard. No-op if there is no selection.
   /// Never registered as a canvas accelerator - handled locally so it isn't stolen by an
   /// unrelated control's accelerator binding while this control has focus.
   void copySelectionToClipboard();

   /// copySelectionToClipboard() then deleteSelection(). No-op if there is no selection.
   void cutSelectionToClipboard();

   /// Reads the platform clipboard, collapses embedded newlines to a space in single-line mode,
   /// truncates to fit canInsert()'s character-count budget, then progressively shortens further
   /// (one character at a time, from the end) until insertAt() accepts it without vertically
   /// overflowing the box - so a paste too long to fully display still lands as much of itself
   /// as actually fits, rather than being rejected outright.
   void pasteFromClipboard();

public:

   NewGuiTextEdit();
   virtual ~NewGuiTextEdit();

   DECLARE_CONOBJECT(NewGuiTextEdit);

   static void initPersistFields();

   /// True while this control is the first responder - lets the canvas's accelerator table
   /// skip over this control's own keystrokes (see NewGuiCanvas::checkAccelerators()), which is
   /// what makes this control's own Ctrl+C/X/V reachable at all.
   bool wantsRawKeyboardInput() const override { return isFirstResponder(); }

   /// Toggles native OS keyboard-accelerator suppression and IME translation on this control's
   /// owning canvas/window on focus gain/loss. Also resets the caret blink phase to visible on
   /// gain.
   void setFirstResponder(bool responder) override;

   /// Sets the full text content, replacing mBuffer and re-syncing display. Clears any
   /// selection and moves the caret to the end.
   /// @param text New content, UTF-8.
   void setText(const char* text);

   /// @return The current text content, UTF-8 (re-encoded from mBuffer).
   const char* getText() const;

   void setMultiLine(bool multiLine) { mMultiLine = multiLine; }
   bool getMultiLine() const { return mMultiLine; }

   /// @param maxLength <= 0 means unlimited. Does not retroactively truncate existing content
   /// if lowered - only enforced going forward, at the next edit.
   void setMaxLength(S32 maxLength) { mMaxLength = maxLength; }
   S32 getMaxLength() const { return mMaxLength; }

   /// Measures the current content (wrapped, if mMultiLine) and adds padding.
   Point2I ComputePreferredSize() override;

   /// Draws background/border (via Parent), the selection highlight, the text, then the caret
   /// (if focused and the current blink phase is visible).
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override;

   /// Places the caret at the clicked glyph (single click), selects the word under it (double
   /// click), or selects the current line (triple click). Starts mDragSelecting.
   void onMouseDown(NewGuiInputEvent& event) override;

   /// Ends mDragSelecting. Doesn't itself move the caret - onMouseDown() already placed it.
   void onMouseUp(NewGuiInputEvent& event) override;

   /// Mouse-drag selection extension and all keyboard handling - both the raw-key channel
   /// (navigation, Backspace/Delete, SI_COPYPASTE, SI_WORDJUMP/SI_LINEJUMP/SI_DOCJUMP) and the
   /// decoded-char channel (literal insertion, via NewGuiInputEvent::isCharInput).
   void onInputEvent(NewGuiInputEvent& event) override;

   /// Fired when Enter is pressed while NOT mMultiLine (in multi-line mode, Enter inserts a
   /// newline instead and this never fires).
   DECLARE_CALLBACK(void, onCommit, ());

   /// Fired whenever mBuffer's content changes as a result of user input (typing, paste, cut,
   /// Backspace/Delete) - not from a script-driven setText() call.
   DECLARE_CALLBACK(void, onTextChanged, ());
};

#endif // _NEWGUITEXTEDIT_H_
