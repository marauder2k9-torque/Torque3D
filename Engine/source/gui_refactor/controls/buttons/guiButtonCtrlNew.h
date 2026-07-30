//-----------------------------------------------------------------------------
// guiButtonCtrlNew.h
//
// GuiButtonCtrlNew -- a clickable/pressable button supporting three modes:
//   Push   -- fires onAction() once per completed click; no persistent state.
//   Toggle -- flips a checked state on click; stays checked until clicked again.
//   Check  -- same checked-state model as Toggle, kept as a distinct mode
//             purely for clarity/console-authoring (a "checkbox" reads
//             differently than a "toggle button" even though the underlying
//             behavior is identical) and as the seam for a future radio-group
//             behavior (mutually-exclusive checked siblings) without a
//             breaking mode-enum change.
//
// Handles mouse press/release (with press-and-drag-off-then-release
// correctly NOT firing, matching standard button feel), keyboard activation
// (Enter/Return while focused), and accelerator keys (via the existing
// GuiControlNew::mAcceleratorKey/addAcceleratorKey() -- already wired to
// call onAction() through acceleratorKeyPress(), no extra work needed here).
//
// Draws its own background/border from style state (Hover/Active/Checked/
// Disabled all already resolve automatically via
// GuiControlNew::getCurrentStyleStateMask()), then its caption via GuiText/
// GuiControlNew::renderText() -- same pattern as GuiLabelCtrlNew.
//-----------------------------------------------------------------------------

#ifndef _GUIBUTTONCTRLNEW_H_
#define _GUIBUTTONCTRLNEW_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _GUITEXT_H_
#include "gui_refactor/core/guiText.h"
#endif

/// See file header. Plain enum (not enum class) for the same reason
/// GuiTextOverflowConsole is -- DefineEnumType/ImplementEnumType only bind
/// to plain enums in this codebase (see GuiCanvasNew::KeyTranslationMode).
enum GuiButtonMode : U8
{
   GuiButtonMode_Push = 0,
   GuiButtonMode_Toggle,
   GuiButtonMode_Check
};

DefineEnumType(GuiButtonMode);

class GuiButtonCtrlNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

protected:

   String mText;
   GuiButtonMode mMode;

   /// Toggle/Check state -- see isChecked(). Meaningless for Push mode.
   bool mChecked;

   /// True from a valid press (inside the button) until the matching
   /// release, drag-off, or loss of mouse lock -- drives isPressed()-
   /// style visuals via mHasError... no, via the normal Active style
   /// state (see GuiControlNew::isPressed(), which already derives
   /// from the canvas's centrally-tracked mouse-down/locked-control
   /// state, so THIS flag is only needed to know whether a press that
   /// started on this button is still "live" for click-detection
   /// purposes, not for the Active style bit itself).
   bool mPressStarted;

   GuiText mGuiText; ///< Persistent instance for the caption; same pattern as GuiLabelCtrlNew.

   static bool setTextProt(void* object, const char* index, const char* data);

   /// Fires the click: Push mode calls onAction() directly; Toggle/Check
   /// flip mChecked first (see setChecked()), then call onAction() so a
   /// bound console command / onAction callback can inspect the new
   /// state via isChecked().
   void _fireClick();

   /// Draws the Check-mode indicator box: an outer quad (border/frame)
   /// plus, when checked, a smaller inset quad standing in for the
   /// checkmark -- a temporary two-quad stand-in until skinned Gui
   /// classes (see gui-rewrite-design.md) can draw a real checkbox
   /// glyph from a style-provided bitmap/icon. Push/Toggle modes still
   /// render as a plain button (no indicator) -- Toggle's checked
   /// state is communicated purely through the style's Checked state
   /// variant (background/border/text color), same as before this
   /// existed.
   /// @param ctrlRect this control's own device-pixel rect (see onRender())
   /// @param style resolved style for the current frame
   /// @return the indicator's device-pixel rect, so onRender() can
   /// offset the caption text out of the way; an empty (zero-extent)
   /// rect at ctrlRect's origin if nothing was drawn (not Check mode)
   RectI _renderCheckboxIndicator(const RectI& ctrlRect, const GuiStyleProperties& style);

public:

   GuiButtonCtrlNew();

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiButtonCtrlNew);
   DECLARE_CATEGORY("Gui Core");
   DECLARE_DESCRIPTION("A clickable button supporting push, toggle, and check modes.");

   bool onWake() override;

   void setText(const String& text);
   const String& getText() const { return mText; }

   void setMode(GuiButtonMode mode) { mMode = mode; }
   GuiButtonMode getMode() const { return mMode; }

   /// Only meaningful for Toggle/Check modes -- see isChecked(). Setting
   /// this directly (e.g. from script, to initialize a toggle's default
   /// state) does NOT fire onAction(); only a real click does.
   void setChecked(bool checked);
   bool isChecked() const override { return mChecked; }

   void onMouseDown(const GuiEvent& event) override;
   void onMouseUp(const GuiEvent& event) override;
   void onMouseLeave(const GuiEvent& event) override;
   bool onKeyDown(const GuiEvent& event) override;

   void onRender(Point2I offset, const RectI& updateRect) override;
};

#endif // _GUIBUTTONCTRLNEW_H_
