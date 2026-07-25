//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "gui/buttons/guiButtonBaseCtrl.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiDefaultControlRender.h"
#include "i18n/lang.h"
#include "sfx/sfxSystem.h"
#include "sfx/sfxTrack.h"


IMPLEMENT_CONOBJECT(GuiButtonBaseCtrl);

ConsoleDocClass(GuiButtonBaseCtrl,
   "@brief The button control. Covers push buttons, checkboxes, radio buttons, and "
   "border-only buttons in a single class.\n\n"

   "Behavior is selected by #buttonType:\n"
   "- PushButton: triggers an action when clicked.\n"
   "- ToggleButton: toggles between on/off state (checkbox-style).\n"
   "- RadioButton: toggles on/off in concert with sibling radio buttons sharing the same #groupNum.\n\n"

   "Independently, #renderStyle selects how the button is drawn:\n"
   "- Filled: filled/themed push-button look.\n"
   "- CheckBox: bitmap-array glyph plus text label (the usual look for ToggleButton/RadioButton).\n"
   "- Border: draws only a state-colored border, useful for wrapping child controls.\n\n"

   "Other, more specialized button controls (bitmapped buttons, color swatches, etc.) remain "
   "separate subclasses where their behavior genuinely differs, such as:\n"

   "- GuiBitmapButtonCtrl (bitmapped buttons)\n"
   "- GuiBitmapButtonTextCtrl (bitmapped buttons with a text label)\n"
   "- GuiSwatchButtonCtrl (color swatch buttons)\n\n"

   "@ingroup GuiButtons"
);

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onMouseDown, void, (), (),
   "If #useMouseEvents is true, this is called when the left mouse button is pressed on an (active) "
   "button.");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onMouseUp, void, (), (),
   "If #useMouseEvents is true, this is called when the left mouse button is release over an (active) "
   "button.\n\n"
   "@note To trigger actions, better use onClick() since onMouseUp() will also be called when the mouse was "
   "not originally pressed on the button.");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onClick, void, (), (),
   "Called when the primary action of the button is triggered (e.g. by a left mouse click).");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onDoubleClick, void, (), (),
   "Called when the left mouse button is double-clicked on the button.");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onRightClick, void, (), (),
   "Called when the right mouse button is clicked on the button.");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onMouseEnter, void, (), (),
   "If #useMouseEvents is true, this is called when the mouse cursor moves over the button (only if the button "
   "is the front-most visible control, though).");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onMouseLeave, void, (), (),
   "If #useMouseEvents is true, this is called when the mouse cursor moves off the button (only if the button "
   "had previously received an onMouseEvent() event).");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onMouseDragged, void, (), (),
   "If #useMouseEvents is true, this is called when a left mouse button drag is detected, i.e. when the user "
   "pressed the left mouse button on the control and then moves the mouse over a certain distance threshold with "
   "the mouse button still pressed.");

IMPLEMENT_CALLBACK(GuiButtonBaseCtrl, onHighlighted, void, (bool highlighted), (highlighted),
   "This is called when the highlighted state of the button is changed.");


ImplementEnumType(GuiButtonType,
   "Type of button control.\n\n"
   "@ingroup GuiButtons")
{
   GuiButtonBaseCtrl::ButtonTypePush, "PushButton", "A button that triggers an action when clicked."
},
{ GuiButtonBaseCtrl::ButtonTypeCheck, "ToggleButton", "A button that is toggled between on and off state." },
{ GuiButtonBaseCtrl::ButtonTypeRadio, "RadioButton", "A button placed in groups for presenting choices." },
EndImplementEnumType;

ImplementEnumType(GuiButtonRenderStyle,
   "Draw path used by a button control, independent of its ButtonType.\n\n"
   "@ingroup GuiButtons")
{
   GuiButtonBaseCtrl::RenderStyleFilled, "Filled", "Filled/themed rendering (push button look)."
},
{ GuiButtonBaseCtrl::RenderStyleCheckBox, "CheckBox", "Bitmap-array glyph and label (checkbox/radio look)." },
{ GuiButtonBaseCtrl::RenderStyleBorder, "Border", "Border-only rendering, for surrounding child controls." },
EndImplementEnumType;


//-----------------------------------------------------------------------------

GuiButtonBaseCtrl::GuiButtonBaseCtrl()
{
   mDepressed = false;
   mHighlighted = false;
   mActive = true;
   mButtonText = StringTable->EmptyString();
   mButtonTextID = StringTable->EmptyString();
   mStateOn = false;
   mRadioGroup = -1;
   mButtonType = ButtonTypePush;
   mUseMouseEvents = false;
   mMouseDragged = false;

   mRenderStyle = RenderStyleFilled;
   mHasTheme = false;
   mIndent = 0;

   setExtent(140, 30);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::initPersistFields()
{
   docsURL;
   addGroup("Button");

   addField("text", TypeCaseString, Offset(mButtonText, GuiButtonBaseCtrl),
      "Text label to display on button (if button class supports text labels).");
   addField("textID", TypeString, Offset(mButtonTextID, GuiButtonBaseCtrl),
      "ID of string in string table to use for text label on button.\n\n"
      "@see setTextID\n"
      "@see GuiControl::langTableMod\n"
      "@see LangTable\n\n");
   addField("groupNum", TypeS32, Offset(mRadioGroup, GuiButtonBaseCtrl),
      "Radio button toggle group number.  All radio buttons that are assigned the same #groupNum and that "
      "are parented to the same control will synchronize their toggle state, i.e. if one radio button is toggled on "
      "all other radio buttons in its group will be toggled off.\n\n"
      "The default group is -1.");
   addField("buttonType", TYPEID< ButtonType >(), Offset(mButtonType, GuiButtonBaseCtrl),
      "Button behavior type.\n");
   addField("renderStyle", TYPEID< RenderStyle >(), Offset(mRenderStyle, GuiButtonBaseCtrl),
      "Button draw path, independent of buttonType. 'Filled' for a push-button look, "
      "'CheckBox' for a checkbox/radio glyph + label look, 'Border' to draw only a "
      "state-colored border around child controls.\n");
   addField("useMouseEvents", TypeBool, Offset(mUseMouseEvents, GuiButtonBaseCtrl),
      "If true, mouse events will be passed on to script.  Default is false.\n");
   addField("indent", TypeS32, Offset(mIndent, GuiButtonBaseCtrl),
      "CheckBox render style only: extra space, in pixels, before the glyph.\n");

   endGroup("Button");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool GuiButtonBaseCtrl::onWake()
{
   if (!Parent::onWake())
      return false;

   // is we have a script variable, make sure we're in sync
   if (mConsoleVariable[0])
      mStateOn = Con::getBoolVariable(mConsoleVariable);
   if (mButtonTextID && *mButtonTextID != 0)
      setTextID(mButtonTextID);

   switch (mRenderStyle)
   {
   case RenderStyleFilled:
      // Button Theme? (36+ frame bitmap array means the profile supplies a
      // themed/sizable border set rather than a plain fill+border look.)
      mHasTheme = (mProfile->constructBitmapArray() >= 36);
      break;

   case RenderStyleCheckBox:
      // Make sure there is a bitmap array for this control type, if it is
      // declared as such in the profile.
      if (mProfile->mBitmapArrayRects.empty() && !mProfile->constructBitmapArray())
      {
         Con::errorf("GuiButtonBaseCtrl::onWake - failed to create bitmap array from profile '%s'", mProfile->getName());
         return false;
      }
      break;

   case RenderStyleBorder:
   default:
      break;
   }

   return true;
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::setText(const char* text)
{
   mButtonText = StringTable->insert(text, true);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::setTextID(const char* id)
{
   S32 n = Con::getIntVariable(id, -1);
   if (n != -1)
   {
      mButtonTextID = StringTable->insert(id);
      setTextID(n);
   }
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::setTextID(S32 id)
{
   const UTF8* str = getGUIString(id);
   if (str)
      setText((const char*)str);
   //mButtonTextID = id;
}

//-----------------------------------------------------------------------------

const char* GuiButtonBaseCtrl::getText()
{
   return mButtonText;
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::setStateOn(bool bStateOn)
{
   if (!mActive)
      return;

   if (mButtonType == ButtonTypeCheck)
   {
      mStateOn = bStateOn;
   }
   else if (mButtonType == ButtonTypeRadio)
   {
      messageSiblings(mRadioGroup);
      mStateOn = bStateOn;
   }
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::acceleratorKeyPress(U32)
{
   if (!mActive)
      return;

   //set the bool
   mDepressed = true;

   if (mProfile->mTabable)
      setFirstResponder();
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::acceleratorKeyRelease(U32)
{
   if (!mActive)
      return;

   if (mDepressed)
   {
      //set the bool
      mDepressed = false;
      //perform the action
      onAction();
   }

   //update
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onMouseDown(const GuiEvent& event)
{
   if (!mActive)
      return;

   if (mProfile->mCanKeyFocus)
      setFirstResponder();

   if (mProfile->isSoundButtonDownValid())
      SFX->playOnce(mProfile->getSoundButtonDownProfile());

   mMouseDownPoint = event.mousePoint;
   mMouseDragged = false;

   if (mUseMouseEvents)
      onMouseDown_callback();

   //lock the mouse
   mouseLock();
   mDepressed = true;

   // If we have a double click then execute the alt command.
   if (event.mouseClickCount == 2)
   {
      onDoubleClick_callback();
      execAltConsoleCallback();
   }

   //update
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onMouseEnter(const GuiEvent& event)
{
   setUpdate();

   if (mUseMouseEvents)
      onMouseEnter_callback();

   if (isMouseLocked())
   {
      mDepressed = true;
      mHighlighted = true;
      onHighlighted_callback(mHighlighted);
   }
   else
   {
      if (mProfile->isSoundButtonOverValid())
         SFX->playOnce(mProfile->getSoundButtonOverProfile());

      mHighlighted = true;

      if (mButtonType != ButtonTypeRadio)
         messageSiblings(mRadioGroup);

      onHighlighted_callback(mHighlighted);
   }
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onMouseLeave(const GuiEvent&)
{
   setUpdate();

   if (mUseMouseEvents)
      onMouseLeave_callback();
   if (isMouseLocked())
      mDepressed = false;
   mHighlighted = false;
   onHighlighted_callback(mHighlighted);

   if (mButtonType != ButtonTypeRadio)
      messageSiblings(mRadioGroup);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onMouseUp(const GuiEvent& event)
{
   mouseUnlock();

   if (!mActive)
      return;

   setUpdate();

   if (mUseMouseEvents)
      onMouseUp_callback();

   //if we released the mouse within this control, perform the action
   if (mDepressed)
      onAction();

   mDepressed = false;
   mMouseDragged = false;
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onRightMouseUp(const GuiEvent& event)
{
   onRightClick_callback();
   Parent::onRightMouseUp(event);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onMouseDragged(const GuiEvent& event)
{
   if (mUseMouseEvents)
   {
      // If we haven't started a drag yet, find whether we have moved past
      // the tolerance value.

      if (!mMouseDragged)
      {
         Point2I delta = mMouseDownPoint - event.mousePoint;
         if (mAbs(delta.x) > 2 || mAbs(delta.y) > 2)
            mMouseDragged = true;
      }

      if (mMouseDragged)
         onMouseDragged_callback();
   }

   Parent::onMouseDragged(event);
}

//-----------------------------------------------------------------------------

bool GuiButtonBaseCtrl::onKeyDown(const GuiEvent& event)
{
   //if the control is a dead end, kill the event
   if (!mActive)
      return true;

   //see if the key down is a return or space or not
   if ((event.keyCode == KEY_RETURN || event.keyCode == KEY_SPACE)
      && event.modifier == 0)
   {
      if (mProfile->isSoundButtonDownValid())
         SFX->playOnce(mProfile->getSoundButtonDownProfile());

      return true;
   }
   //otherwise, pass the event to it's parent
   return Parent::onKeyDown(event);
}

//-----------------------------------------------------------------------------

bool GuiButtonBaseCtrl::onKeyUp(const GuiEvent& event)
{
   //if the control is a dead end, kill the event
   if (!mActive)
      return true;

   //see if the key down is a return or space or not
   if (mDepressed &&
      (event.keyCode == KEY_RETURN || event.keyCode == KEY_SPACE) &&
      event.modifier == 0)
   {
      onAction();
      return true;
   }

   //otherwise, pass the event to it's parent
   return Parent::onKeyUp(event);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::setScriptValue(const char* value)
{
   mStateOn = dAtob(value);

   // Update the console variable:
   if (mConsoleVariable[0])
      Con::setBoolVariable(mConsoleVariable, mStateOn);

   setUpdate();
}

//-----------------------------------------------------------------------------

const char* GuiButtonBaseCtrl::getScriptValue()
{
   return mStateOn ? "1" : "0";
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onAction()
{
   if (!mActive)
      return;

   if (mButtonType == ButtonTypeCheck)
   {
      mStateOn = mStateOn ? false : true;
   }
   else if (mButtonType == ButtonTypeRadio)
   {
      mStateOn = true;
      messageSiblings(mRadioGroup);
   }
   setUpdate();

   // Update the console variable:
   if (mConsoleVariable[0])
      Con::setBoolVariable(mConsoleVariable, mStateOn);

   onClick_callback();
   Parent::onAction();
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onMessage(GuiControl* sender, S32 msg)
{
   Parent::onMessage(sender, msg);
   if (mRadioGroup == msg)
   {
      if (mButtonType == ButtonTypeRadio)
      {
         setUpdate();
         mStateOn = (sender == this);

         // Update the console variable:
         if (mConsoleVariable[0])
            Con::setBoolVariable(mConsoleVariable, mStateOn);
      }
      else if (mButtonType == ButtonTypePush)
      {
         mHighlighted = (sender == this);
         onHighlighted_callback(mHighlighted);
      }
   }
}

void GuiButtonBaseCtrl::setHighlighted(bool highlighted)
{
   mHighlighted = highlighted;
   onHighlighted_callback(mHighlighted);

   if (mRadioGroup != -1)
   {
      messageSiblings(mRadioGroup);
   }
}

//-----------------------------------------------------------------------------
//    Rendering.
//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::onPreRender()
{
   Parent::onPreRender();

   // Keep mStateOn in sync with a bound console variable every frame, so an
   // external change to the variable (e.g. from another control or from script)
   // is reflected visually without requiring a click on this control.
   // Formerly done in GuiToggleButtonCtrl::onPreRender() (Filled/push-style) and
   // in GuiCheckBoxCtrl::onRender() (CheckBox-style); centralized here so both
   // render styles -- and RenderStyleBorder, which never had this before -- get
   // the same behavior consistently.
   if (mConsoleVariable[0])
   {
      bool stateOn = Con::getBoolVariable(mConsoleVariable);
      if (stateOn != mStateOn)
         mStateOn = stateOn;
   }
}

void GuiButtonBaseCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   switch (mRenderStyle)
   {
   case RenderStyleCheckBox:
      renderCheckBoxButton(offset, updateRect);
      break;

   case RenderStyleBorder:
      renderBorderButton(offset, updateRect);
      break;

   case RenderStyleFilled:
   default:
      renderFilledButton(offset, updateRect);
      break;
   }
}

//-----------------------------------------------------------------------------

GuiState GuiButtonBaseCtrl::resolveState() const
{
   // Precedence matches renderFilledButton's/renderBorderButton's original
   // behavior specifically, where a checked-on button (mStateOn) and an
   // actively-depressed one (mDepressed) intentionally use the same visual:
   // disabled beats everything, then depressed-or-checked-on, then
   // highlighted.
   //
   // NOT universal: renderCheckBoxButton's glyph selection does NOT use this.
   // There, mStateOn (checked) and mDepressed (actively clicked) select
   // different, independent things -- checked picks the off/on frame, while
   // only an actual mouse-press adds the "+2 depressed" offset on top of
   // whichever frame is already selected. Folding mStateOn into Depressed
   // here would make a merely-checked-and-hovered checkbox render as if it
   // were being actively clicked. See renderCheckBoxButton for its own,
   // separate state resolution.
   if (!mActive)
      return GuiState::Disabled;
   if (mDepressed || mStateOn)
      return GuiState::Depressed;
   if (mHighlighted)
      return GuiState::Highlighted;
   return GuiState::Normal;
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::renderFilledButton(Point2I offset, const RectI& updateRect)
{
   // Formerly GuiButtonCtrl::onRender / GuiToggleButtonCtrl::onRender (identical).
   // All fill/border/theme drawing now routes through guiDefaultControlRender's
   // state-aware helpers -- this function only resolves state and lays out text.

   GuiState state = resolveState();

   RectI boundsRect(offset, getExtent());

   if (!mHasTheme)
      renderStateFill(boundsRect, state, mProfile);
   else
      renderStateBitmapBorders(boundsRect, state, mProfile);

   ColorI fontColor = mActive ? (mHighlighted ? mProfile->mFontColorHL : mProfile->mFontColor) : mProfile->mFontColorNA;

   Point2I textPos = offset;
   if (mDepressed)
      textPos += Point2I(1, 1);

   GFX->getDrawUtil()->setBitmapModulation(fontColor);
   renderJustifiedText(textPos, getExtent(), mButtonText);

   //render the children
   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::renderCheckBoxButton(Point2I offset, const RectI& updateRect)
{
   // Formerly GuiCheckBoxCtrl::onRender (also used, unchanged, for GuiRadioCtrl).
   // Console-variable sync now happens once per frame in onPreRender().
   // Glyph drawing now routes through guiDefaultControlRender's
   // renderStateGlyph(); this function only resolves state and lays out text.
   //
   // NOTE: this deliberately does NOT use resolveState(). That helper folds
   // mStateOn into GuiState::Depressed to match renderFilledButton's original
   // behavior (where a checked-on push button uses the same themed frame as
   // an actively-depressed one). The checkbox glyph never worked that way:
   // mStateOn only ever selected the off/on frame (0/1), and only an actual
   // mouse-press (mDepressed) added the +2 depressed offset -- a checked,
   // merely-hovered checkbox must stay on frame 0/1, not jump to 2/3.

   ColorI fontColor = mActive ? (mHighlighted ? mProfile->mFontColorHL : mProfile->mFontColor) : mProfile->mFontColorNA;

   GuiState glyphState = GuiState::Normal;
   if (!mActive)
      glyphState = GuiState::Disabled;
   else if (mDepressed)
      glyphState = GuiState::Depressed;

   S32 xOffset = 0;
   GFX->getDrawUtil()->clearBitmapModulation();
   if (mProfile->mBitmapArrayRects.size() >= 4)
   {
      S32 y = (getHeight() - mProfile->mBitmapArrayRects[0].extent.y) / 2;
      renderStateGlyph(offset + Point2I(mIndent, y), glyphState, mStateOn, mProfile);
      // xOffset is measured from frame 0's extent (not the drawn frame's
      // extent) to match the original layout exactly, since different
      // state frames can vary slightly in size.
      xOffset = mProfile->mBitmapArrayRects[0].extent.x + 2 + mIndent;
   }

   if (mButtonText[0] != '\0')
   {
      GFX->getDrawUtil()->setBitmapModulation(fontColor);
      renderJustifiedText(Point2I(offset.x + xOffset, offset.y),
         Point2I(getWidth() - getHeight(), getHeight()),
         mButtonText);
   }
   //render the children
   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::renderBorderButton(Point2I offset, const RectI& updateRect)
{
   // Formerly (the standalone class) GuiBorderButtonCtrl::onRender.
   //
   // NOTE: the original's depressed/on and highlighted overlays are drawn from
   // two independent `if`s, not mutually exclusive -- onMouseEnter() can set
   // both mDepressed and mHighlighted at once (e.g. dragging back onto a
   // locked button), in which case both overlays are drawn stacked on top of
   // the base border. Preserved here rather than simplified to if/else, which
   // would silently drop the highlighted overlay in that case.

   // NOTE: only the always-on base border was gated by mBorder > 0 in the
   // original; the depressed/highlighted overlays below drew unconditionally
   // whenever active, regardless of mBorder.
   if (mProfile->mBorder > 0)
      renderStateBorderOnly(RectI(offset, getExtent()), GuiState::Normal, mProfile);

   if (mActive)
   {
      if (mStateOn || mDepressed)
         renderStateBorderOnly(RectI(offset, getExtent()), GuiState::Depressed, mProfile);

      if (mHighlighted)
         renderStateBorderOnly(RectI(offset, getExtent()), GuiState::Highlighted, mProfile);
   }

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------

void GuiButtonBaseCtrl::autoSize()
{
   // Formerly GuiCheckBoxCtrl::autoSize. Only meaningful for the CheckBox render
   // style, which sizes itself around the glyph bitmap plus the text label.
   if (mRenderStyle != RenderStyleCheckBox)
      return;

   U32 width, height;
   U32 bmpArrayRect0Width = 0;

   if (!mAwake)
   {
      mProfile->incLoadCount();

      if (mProfile->mBitmapArrayRects.empty())
         mProfile->constructBitmapArray();
      if (!mProfile->mBitmapArrayRects.empty())
         bmpArrayRect0Width = mProfile->mBitmapArrayRects[0].extent.x;
   }

   U32 bmpWidth = bmpArrayRect0Width + 2 + mIndent;
   U32 strWidth = mProfile->mFont->getStrWidthPrecise(mButtonText);

   width = bmpWidth + strWidth + 2;

   U32 bmpHeight = mProfile->mBitmapArrayRects[0].extent.y;
   U32 fontHeight = mProfile->mFont->getHeight();

   height = getMax(bmpHeight, fontHeight) + 4;

   setExtent(width, height);

   if (!mAwake)
      mProfile->decLoadCount();
}

//=============================================================================
//    Console Methods.
//=============================================================================
// MARK: ---- Console Methods ----

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, performClick, void, (), ,
   "Simulate a click on the button.\n"
   "This method will trigger the button's action just as if the button had been pressed by the "
   "user.\n\n")
{
   object->onAction();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, setText, void, (const char* text), ,
   "Set the text displayed on the button's label.\n"
   "@param text The text to display as the button's text label.\n"
   "@note Not all buttons render text labels.\n\n"
   "@see getText\n"
   "@see setTextID\n")
{
   object->setText(text);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, setTextID, void, (const char* id), ,
   "Set the text displayed on the button's label using a string from the string table "
   "assigned to the control.\n\n"
   "@param id Name of the variable that contains the integer string ID.  Used to look up "
   "string in table.\n\n"
   "@note Not all buttons render text labels.\n\n"
   "@see setText\n"
   "@see getText\n"
   "@see GuiControl::langTableMod\n"
   "@see LangTable\n\n"
   "@ref Gui_i18n")
{
   object->setTextID(id);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, getText, const char*, (), ,
   "Get the text display on the button's label (if any).\n\n"
   "@return The button's label.")
{
   return object->getText();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, setStateOn, void, (bool isOn), (true),
   "For toggle or radio buttons, set whether the button is currently activated or not.  For radio buttons, "
   "toggling a button on will toggle all other radio buttons in its group to off.\n\n"
   "@param isOn If true, the button will be toggled on (if not already); if false, it will be toggled off.\n\n"
   "@note Toggling the state of a button with this method will <em>not</em> not trigger the action associated with the "
   "button.  To do that, use performClick().")
{
   object->setStateOn(isOn);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, resetState, void, (), ,
   "Reset the mousing state of the button.\n\n"
   "This method should not generally be called.")
{
   object->resetState();
}

DefineEngineMethod(GuiButtonBaseCtrl, setHighlighted, void, (bool highlighted), (false),
   "Reset the mousing state of the button.\n\n"
   "This method should not generally be called.")
{
   object->setHighlighted(highlighted);
}

DefineEngineMethod(GuiButtonBaseCtrl, isHighlighted, bool, (), ,
   "Reset the mousing state of the button.\n\n"
   "This method should not generally be called.")
{
   return object->isHighlighted();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, isStateOn, bool, (), ,
   "Test whether the button is currently in its 'on' state (checked, for a "
   "CheckBox render style, or selected, for a radio button).\n"
   "@return True if the button is currently toggled on.\n")
{
   return object->getStateOn();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiButtonBaseCtrl, autoSize, void, (), ,
   "Resize the control to fit its glyph and text label (CheckBox render style only).\n")
{
   object->autoSize();
}
