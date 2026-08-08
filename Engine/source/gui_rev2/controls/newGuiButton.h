//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiButton.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIBUTTON_H_
#define _NEWGUIBUTTON_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#ifndef _NEWGUITEXT_H_
#include "gui_rev2/core/newGuiText.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif

/// A clickable text-label control supporting push, toggle, and radio
/// behavior via a single class (see ButtonType). Owns a NewGuiText for
/// its label, following the same measure/draw pattern as NewGuiLabel.
///
/// @code
/// new NewGuiButton( MyButton )
/// {
///    buttonType = "radio";
///    groupNum = 1;
///    text = "Option A";
/// };
/// @endcode
class NewGuiButton : public NewGuiControl
{
public:

   typedef NewGuiControl Parent;

   /// Behavior mode for a NewGuiButton.
   enum ButtonType : U8
   {
      ButtonType_Push = 0,     ///< Momentary; never latches mChecked.
      ButtonType_Toggle,       ///< Latches mChecked on each click.
      ButtonType_Radio,        ///< Latches mChecked and clears every other Radio-type sibling sharing groupNum.
   };

protected:

   NewGuiText mText;                   ///< Label text/measurement/drawing.
   Resource<GFont> mFont;
   StringTableEntry mCachedFontFamily;
   F32 mCachedFontSize;

   ButtonType mButtonType;
   S32 mGroupNum;                      ///< Mutual-exclusion tag for ButtonType_Radio.
   bool mPressArmed;                   ///< True from a bounds-landing onMouseDown() until the matching onMouseUp().

   /// When true, EmitDrawCommands() draws a checkbox glyph (box + check mark) to the label's
   /// left instead of a bordered button with centered text - a rendering mode, not a distinct
   /// behavior. Only meaningful combined with ButtonType_Toggle (see createCheckbox()); does not
   /// change hit-testing, click handling, or mChecked latching in any way.
   bool mCheckboxStyle;

   StringTableEntry mAcceleratorString;   ///< Authored form (e.g. "ctrl s"), retained for getAccelerator()/re-registration.
   U16 mAcceleratorKeyCode;               ///< Parsed from mAcceleratorString; 0 (KEY_NULL) if unset/unparsed.
   U32 mAcceleratorModifier;              ///< Parsed from mAcceleratorString; raw (un-normalized) bits, as NewGuiCanvas::registerAccelerator() expects.

   static bool _setButtonType(void* obj, const char* index, const char* data);
   static bool _setGroupNum(void* obj, const char* index, const char* data);
   static bool _setText(void* obj, const char* index, const char* data);
   static bool _setAccelerator(void* obj, const char* index, const char* data);
   static bool _setCheckboxStyle(void* obj, const char* index, const char* data);

   /// Parses an authored accelerator string (e.g. "ctrl-shift s", ActionMap's own
   /// modifier-then-space-then-key grammar - see ActionMap::createEventDescriptor()) into a
   /// keyCode + modifier mask. Thin wrapper around that existing engine parser rather than a
   /// second, competing grammar. Unparseable/empty input yields (KEY_NULL, 0).
   /// @param text Authored accelerator string.
   /// @param outKeyCode Receives the parsed key code.
   /// @param outModifier Receives the parsed (raw, un-normalized) modifier mask.
   static void ParseAcceleratorString(const char* text, U16& outKeyCode, U32& outModifier);

   /// Re-registers this button's accelerator with its (possibly new) owning canvas. No-op if
   /// mAcceleratorKeyCode is unset (KEY_NULL) or there's no owning canvas yet.
   void updateAcceleratorRegistration();

   /// Loads/caches the font matching the resolved style.
   void resolveFont();

   /// Un-checks every other Radio-type sibling under the same parent sharing mGroupNum.
   void clearGroupSiblings();

public:

   NewGuiButton();
   virtual ~NewGuiButton();

   DECLARE_CONOBJECT(NewGuiButton);

   static void initPersistFields();

   /// Registers this button's accelerator (if any) with its new owning canvas.
   void onGroupAdd() override;

   /// Removes this button's accelerator registration from its (former) owning canvas.
   void onGroupRemove() override;

   /// Sets the button's label text.
   /// @param text New label text.
   void setText(const char* text);

   /// @return The button's current label text.
   const char* getText() const { return mText.getText().c_str(); }

   /// @return The button's behavior mode (push/toggle/radio).
   ButtonType getButtonType() const { return mButtonType; }

   /// Sets the button's behavior mode.
   /// @param type New behavior mode.
   void setButtonType(ButtonType type) { mButtonType = type; }

   /// @return The radio mutual-exclusion group tag.
   S32 getGroupNum() const { return mGroupNum; }

   /// Sets the radio mutual-exclusion group tag.
   /// @param group New group tag.
   void setGroupNum(S32 group) { mGroupNum = group; }

   /// Sets this button's accelerator key (e.g. "ctrl s", "ctrl-shift s"). Empty/NULL clears it. Re-registers
   /// with the owning canvas immediately if already attached to one.
   /// @param accelerator Authored accelerator string, or NULL/"" to clear.
   void setAccelerator(const char* accelerator);

   /// @return This button's authored accelerator string, or "" if unset.
   const char* getAccelerator() const { return mAcceleratorString ? mAcceleratorString : ""; }

   /// True to draw a checkbox glyph (box + check mark) beside the label instead of a bordered
   /// button with centered text. Purely a rendering-mode switch - see mCheckboxStyle's doc comment.
   /// @param checkbox New rendering mode.
   void setCheckboxStyle(bool checkbox) { mCheckboxStyle = checkbox; setContentDirty(); setArrangementDirty(); }
   bool getCheckboxStyle() const { return mCheckboxStyle; }


   /// Runs the same click path (group-clear, latch, callbacks) a real
   /// mouse click would, without needing an input event.
   /// @code
   /// %button.performClick();
   /// @endcode
   virtual void performClick();

   /// Measures the label unbounded and adds padding.
   /// @return Preferred size in device pixels.
   Point2I ComputePreferredSize() override;

   /// Draws background/border, then centered label text.
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override;

   /// Arms the press.
   void onMouseDown(NewGuiInputEvent& event) override;

   /// Completes the click if release lands within bounds and the press was armed.
   void onMouseUp(NewGuiInputEvent& event) override;

   void onMouseLeave(NewGuiInputEvent& event) override;

   /// Handles keyboard/gamepad activation (Enter/Space/confirm) by running performClick().
   void onActivate(NewGuiInputEvent& event) override;

   /// Fired when this button's registered accelerator (if any) is pressed - funnels straight
   /// into performClick(), so an accelerator-triggered Save is indistinguishable downstream
   /// from a real mouse click.
   void onAccelerator() override;

   /// Fired whenever the button is clicked, for every button type.
   DECLARE_CALLBACK(void, onClick, ());

   /// Fired whenever a Toggle/Radio button's checked state changes as a result of a click.
   /// @param newState The button's new checked state.
   DECLARE_CALLBACK(void, onToggle, (bool newState));
};

#endif // _NEWGUIBUTTON_H_
