//-----------------------------------------------------------------------------
// guiTextEditCtrlNew.h
//
// GuiTextEditCtrlNew -- an editable text field. Single-line by default
// (Enter fires onAction()/a bound console command, no newline insertion);
// set multiLine=true for wrapped, multi-line editing (Enter inserts a
// newline; commit some other way, e.g. losing focus or a separate button --
// this control itself never treats Enter as "done" while multiLine).
//
// Built directly on GuiText for measurement/layout/drawing (see guiText.h's
// own file header on why: this rewrite deliberately never measures/draws
// text via raw GFont calls -- GFont::getStrWidthPrecise()/wrapString()/
// getStrNWidth() -- outside GuiText's own internals). The editable buffer
// itself (mText, character insert/delete, cursor/selection indices) is
// owned here, NOT by GuiText -- GuiText is reconfigured with the current
// mText and asked to lay out/submit each render, the same "persistent,
// reused, reconfigure-don't-reconstruct" pattern already established for
// every other GuiText consumer in this codebase (GuiButtonCtrlNew's
// caption, the tooltip cache, GuiEditCtrlNew's palette labels) -- see
// GuiText::renderSimple()'s doc comment for why a FRESH GuiText per call
// is the wrong shape for anything called from onRender().
//
// SINGLE-LINE vs MULTI-LINE: cursor/selection are always tracked as a flat
// character index into mText (0..mText.length()). Single-line mode derives
// the caret's draw position directly via GuiTextMetrics::measure() against
// the one line GuiText produces. Multi-line mode has to first figure out
// WHICH line a given flat character index falls on (by walking
// GuiText::layout()'s result and summing each line's character count) --
// see _findCaretLineAndOffset(). This is real, separate logic specific to
// wrapped text; single-line mode does not go through it at all, so
// multi-line's extra complexity doesn't cost single-line callers (the
// common case, and what the inspector needs first) anything.
//
// UNVERIFIED KEY CONSTANTS: KEY_LEFT/KEY_RIGHT/KEY_HOME/KEY_END/KEY_ESCAPE
// are used below on the strength of the SAME naming convention already
// confirmed elsewhere in this project for KEY_TAB, KEY_DELETE,
// KEY_BACKSPACE, KEY_RETURN, KEY_NUMPADENTER, KEY_UP, KEY_DOWN -- but,
// unlike those, these five specific names were not found via grep against
// this project's actual files. If any of them doesn't match this
// platform's real key-constant header, it's a one-line name fix, not a
// design problem.
//
// UNICODE: GuiEvent::ascii is a single U16 code unit (confirmed against
// guiTypes.h), and character insertion converts it via the confirmed
// convertUTF16toUTF8N() (core/strings/unicode.h) -- NOT a guessed
// signature; both were verified against real project headers before
// this file was finished.
//-----------------------------------------------------------------------------

#ifndef _GUITEXTEDITCTRLNEW_H_
#define _GUITEXTEDITCTRLNEW_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _GUITEXT_H_
#include "gui_refactor/core/guiText.h"
#endif

class GuiTextEditCtrlNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

protected:

   /// The editable content. Always the single source of truth; mGuiText
   /// is reconfigured from this every render, never edited directly.
   String mText;

   /// Optional cap on mText's length in characters; 0 = unlimited.
   S32 mMaxLength;

   /// See file header. false (default) = single line, Enter fires
   /// onAction()/the bound console command. true = wrapped multi-line,
   /// Enter inserts a newline character instead.
   bool mMultiLine;

   /// Password-style masking -- see GuiText::setMasked(). Real characters
   /// are still what's stored/edited/measured against; only the GLYPHS
   /// drawn are substituted (GuiText's own masking already guarantees
   /// the real text is never handed to the render batch when masked).
   bool mPasswordMask;

   /// @name Cursor / selection
   /// Both are flat character indices into mText, NOT line/column pairs
   /// -- see file header on why multi-line mode has to derive a line
   /// from this separately rather than storing one directly. Range is
   /// always normalized so mSelectionAnchor <= mCursorPos is NOT
   /// assumed -- see _getSelectionRange().
   /// @{
   S32 mCursorPos;

   /// -1 when there is no active selection; otherwise the far end of the
   /// selection range (mCursorPos is always the "live" end that moves).
   S32 mSelectionAnchor;

   /// True between onMouseDown() and onMouseUp() while dragging out a
   /// mouse selection -- lets onMouseDragged() know it should keep
   /// extending mSelectionAnchor->mCursorPos rather than doing nothing
   /// (this control has no other drag behavior to distinguish from).
   bool mMouseSelecting;

   /// Remembers the device-pixel x-coordinate of the caret across
   /// consecutive Up/Down presses, so moving through a short line and
   /// back onto a longer one restores the original column instead of
   /// snapping to wherever the short line happened to end -- standard
   /// text-editor caret behavior. Reset (to -1, "unset") by any
   /// horizontal movement (Left/Right/Home/End/click/typing), which is
   /// what onKeyDown()'s Up/Down branch itself does NOT do, since a
   /// currently-Up/Down-ing caret should keep the same target column.
   S32 mDesiredCaretDeviceX;
   /// @}

   /// Text as of the last onGainFirstResponder() -- what KEY_ESCAPE
   /// reverts to. Doc comment on onKeyDown()'s Escape handling explains
   /// why this exists (useful for exactly the kind of "cancel an edit"
   /// interaction an inspector field needs).
   String mTextOnFocusGain;

   /// @name Cursor blink
   /// Driven by onPreRender()'s real elapsed-time check (see
   /// Platform::getVirtualMilliseconds(), already used elsewhere in this
   /// codebase -- e.g. GuiCanvasNew::rootMouseDown()) rather than a
   /// frame counter, so blink rate is correct regardless of frame rate.
   /// @{
   static const U32 smBlinkIntervalMs = 500;
   U32 mLastBlinkToggleMs;
   bool mCursorBlinkOn;
   /// @}

   GuiText mGuiText; ///< Persistent -- see file header.

   static bool setTextProt(void* object, const char* index, const char* data);

   /// Clamps to [0, mText.length()].
   S32 _clampCursor(S32 pos) const;

   /// @return true if there's an active selection (mSelectionAnchor != -1
   /// AND it differs from mCursorPos)
   bool _hasSelection() const;

   /// Normalized [start, end) of the current selection, regardless of
   /// which end mCursorPos/mSelectionAnchor each are. Only meaningful
   /// when _hasSelection() is true.
   void _getSelectionRange(S32& outStart, S32& outEnd) const;

   /// Deletes the current selection (no-op if none) and collapses the
   /// cursor to the deletion point. Does NOT mark the control's own
   /// update/dirty state -- callers already do that as part of the
   /// larger edit they're performing.
   void _deleteSelection();

   /// Inserts text at mCursorPos, replacing the selection first if one
   /// exists, respecting mMaxLength (silently truncates/refuses whatever
   /// portion would exceed it, matching common text-field behavior
   /// rather than erroring).
   void _insertText(const String& insertion);

   /// Multi-line only -- see file header. Walks mGuiText's current
   /// layout() result to find which line flatCharIndex falls on, and
   /// that line's own local character offset within it. NOT const:
   /// mGuiText.layout() itself is non-const (it lazily recomputes and
   /// caches -- see GuiText::mLayoutDirty/mCachedResult in guiText.h),
   /// so nothing that calls it transitively can be const either.
   /// @return false if layout() has no lines yet (e.g. empty text)
   bool _findCaretLineAndOffset(S32 flatCharIndex, U32& outLineIndex, U32& outLineLocalOffset);

   /// Inverse of _findCaretLineAndOffset(): given a line index and a
   /// local character offset within it, returns the corresponding flat
   /// index into mText. Accounts for consumed characters (real '\n's and
   /// wrap-consumed spaces) the same way _findCaretLineAndOffset() does
   /// -- see GuiTextLine::consumedCharsBeforeThisLine's doc comment
   /// (guiText.h) for why that accounting is needed at all.
   /// Clamps lineIndex/lineLocalOffset to valid ranges. NOT const -- see
   /// _findCaretLineAndOffset()'s doc comment.
   S32 _findFlatIndexFromLineAndOffset(U32 lineIndex, U32 lineLocalOffset);

   /// Intersects the flat selection range [selStart, selEnd) against
   /// lineIndex's own flat-index span (derived via two
   /// _findFlatIndexFromLineAndOffset() calls -- the line's first and
   /// one-past-its-last character), producing that line's own LOCAL
   /// selection range [outLocalStart, outLocalEnd) for highlight drawing.
   /// @return false if the selection doesn't touch this line at all
   /// (outLocalStart/End are untouched in that case).
   bool _getSelectionRangeForLine(U32 lineIndex, S32 selStart, S32 selEnd, U32& outLocalStart, U32& outLocalEnd);

   /// @return the flat mText character index closest to devicePt (a
   /// DEVICE-pixel point, e.g. from event.mousePoint after conversion --
   /// see onMouseDown()/onMouseDragged()), given the current layout.
   /// Single-line: finds the closest character boundary on the one line.
   /// Multi-line: first finds the closest LINE by y, then the closest
   /// character boundary on it by x. offset is this control's own
   /// device-pixel draw origin (same convention as onRender()'s offset
   /// parameter), needed since GuiTextLine::origin is control-relative.
   /// Takes font as a caller-supplied, non-const Resource<GFont> rather
   /// than reading GuiTextLayoutResult::font itself, because layout()
   /// returns a CONST GuiTextLayoutResult& (see GuiText::layout()'s own
   /// signature) -- result.font is therefore only reachable through
   /// Resource<T>'s const-qualified conversion operator (operator const
   /// T*() const), which yields a const GFont*, and GFont::getCharInfo()
   /// (needed below) is NOT const (it can lazily bake a new glyph bitmap
   /// on first use -- see GFont::loadCharInfo()), so a const GFont* can't
   /// call it at all. Callers already have a live, non-const
   /// Resource<GFont> from _resolveFont() for exactly this reason -- see
   /// onMouseDown()/onMouseDragged() and onKeyDown()'s Up/Down handling.
   S32 _hitTestCharAt(const Point2I& devicePt, const Point2I& offset, Resource<GFont> font);

   /// Resolves this control's current font via mStyle (or an invalid
   /// Resource<GFont> if mStyle is unset) -- shared by onRender() and
   /// anything needing an up-to-date layout OUTSIDE a render call (mouse
   /// hit-testing, word-jump navigation measuring), where onRender()'s
   /// own font resolution hasn't run yet this frame.
   Resource<GFont> _resolveFont() const;

   /// Converts event.mousePoint (canvas-global LOGICAL, per GuiEvent's
   /// own convention -- see this file's onKeyDown() doc comment for the
   /// analogous ascii/keyCode note) into a point in THIS control's own
   /// DEVICE-pixel local space, i.e. directly comparable against
   /// GuiTextLine::origin (already control-relative device pixels -- see
   /// onRender()) with no further offset needed. Used by onMouseDown()/
   /// onMouseDragged() before calling _hitTestCharAt().
   Point2I _deviceLocalPointFromEvent(const GuiEvent& event) const;

   /// @return the index of the start of the word flatCharIndex is
   /// within/after (Ctrl+Left semantics) or the start of the NEXT word
   /// (Ctrl+Right semantics -- see isForward). "Word" boundaries are
   /// whitespace-delimited, matching common text-field behavior; walks
   /// mText directly, no font/layout dependency.
   S32 _findWordBoundary(S32 flatCharIndex, bool isForward) const;

   /// Reconfigures mGuiText from current mText/mMultiLine/mPasswordMask/
   /// style and returns its up-to-date layout() result. Called once per
   /// onRender() (mGuiText's own setters no-op when nothing actually
   /// changed, so this is cheap on frames where the text hasn't
   /// changed -- see GuiText::setText()'s doc comment), and also from
   /// onMouseDown()/onMouseDragged()/onKeyDown()'s Up/Down handling,
   /// which need an up-to-date layout to hit-test/navigate against
   /// before this frame's onRender() has necessarily run yet.
   const GuiTextLayoutResult& _refreshLayout(const Resource<GFont>& font, const Point2I& boxExtent);

public:

   GuiTextEditCtrlNew();

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiTextEditCtrlNew);
   DECLARE_CATEGORY("Gui Core");
   DECLARE_DESCRIPTION("An editable single- or multi-line text field.");

   bool onWake() override;

   void setText(const String& text);
   const String& getText() const { return mText; }

   void setMaxLength(S32 length) { mMaxLength = getMax(0, length); }
   S32 getMaxLength() const { return mMaxLength; }

   void setMultiLine(bool value);
   bool isMultiLine() const { return mMultiLine; }

   void setPasswordMask(bool value);
   bool isPasswordMask() const { return mPasswordMask; }

   /// Selects [start, end) and places the cursor at end. Clamped to
   /// valid range; pass start == end to just move the cursor with no
   /// selection.
   void setSelection(S32 start, S32 end);
   void selectAll();
   void clearSelection();

   S32 getCursorPos() const { return mCursorPos; }

   void onMouseDown(const GuiEvent& event) override;
   void onMouseUp(const GuiEvent& event) override;
   void onMouseDragged(const GuiEvent& event) override;
   bool onKeyDown(const GuiEvent& event) override;

   void onGainFirstResponder() override;
   void onLoseFirstResponder() override;

   void onPreRender() override;
   void onRender(Point2I offset, const RectI& updateRect) override;

   static bool setMultiLineProt(void* object, const char* index, const char* data);
   static bool setPasswordMaskProt(void* object, const char* index, const char* data);
};

#endif // _GUITEXTEDITCTRLNEW_H_
