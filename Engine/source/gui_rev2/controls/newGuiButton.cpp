//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiButton.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiButton.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gui_rev2/core/newGuiCanvas.h"
#include "sim/actionMap.h"

IMPLEMENT_CONOBJECT(NewGuiButton);

IMPLEMENT_CALLBACK(NewGuiButton, onClick, void, (), (),
   "Called whenever this button is clicked - a genuine bounds-landing "
   "press-then-release, or a script-driven performClick() call. Fires "
   "for every button type, including Push.");

IMPLEMENT_CALLBACK(NewGuiButton, onToggle, void, (bool newState), (newState),
   "Called whenever a Toggle- or Radio-type button's checked state "
   "changes as a result of a click. Never fires for ButtonType_Push.");

NewGuiButton::NewGuiButton()
   : mCachedFontFamily(NULL),
   mCachedFontSize(0.0f),
   mButtonType(ButtonType_Push),
   mGroupNum(0),
   mPressArmed(false),
   mCheckboxStyle(false),
   mAcceleratorString(NULL),
   mAcceleratorKeyCode(0),
   mAcceleratorModifier(0)
{
   // Centered by default, unlike NewGuiLabel's left-aligned default.
   mText.setAlignHorizontal(NewGuiTextAlignHorizontal::Center);
}

NewGuiButton::~NewGuiButton()
{
}

void NewGuiButton::onGroupAdd()
{
   Parent::onGroupAdd();
   updateAcceleratorRegistration();
}

void NewGuiButton::onGroupRemove()
{
   NewGuiCanvas* canvas = getOwningCanvas();
   if (canvas)
      canvas->removeAccelerators(this);

   Parent::onGroupRemove();
}

void NewGuiButton::setText(const char* text)
{
   mText.setText(text ? text : "");
   setContentDirty();
   setArrangementDirty();
}

bool NewGuiButton::_setText(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiButton*>(obj)->setText(data);
   return false;
}

bool NewGuiButton::_setButtonType(void* obj, const char* index, const char* data)
{
   NewGuiButton* button = static_cast<NewGuiButton*>(obj);

   ButtonType type = ButtonType_Push;
   if (dStricmp(data, "toggle") == 0)
      type = ButtonType_Toggle;
   else if (dStricmp(data, "radio") == 0)
      type = ButtonType_Radio;

   button->setButtonType(type);
   // Doesn't change layout/measured size - no dirty flag needed. Leaves any latched mChecked
   // untouched rather than clearing it, since switching type at runtime is rare/authoring-time.
   return false;
}

bool NewGuiButton::_setGroupNum(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiButton*>(obj)->setGroupNum(dAtoi(data));
   return false;
}

bool NewGuiButton::_setAccelerator(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiButton*>(obj)->setAccelerator(data);
   return false;
}

bool NewGuiButton::_setCheckboxStyle(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiButton*>(obj)->setCheckboxStyle(dAtob(data));
   return false;
}

// Thin wrapper around ActionMap::createEventDescriptor()
void NewGuiButton::ParseAcceleratorString(const char* text, U16& outKeyCode, U32& outModifier)
{
   outKeyCode = KEY_NULL;
   outModifier = 0;

   if (!text || !text[0])
      return;

   EventDescriptor descriptor;
   if (!ActionMap::createEventDescriptor(text, &descriptor))
      return;

   if (descriptor.eventType != SI_KEY)
      return;

   outKeyCode = descriptor.eventCode;
   outModifier = descriptor.flags;   // Already SI_SHIFT/SI_CTRL/SI_ALT-flavored - matches what NewGuiCanvas::NormalizeModifiers() collapses live events down to.
}

void NewGuiButton::setAccelerator(const char* accelerator)
{
   mAcceleratorString = (accelerator && accelerator[0]) ? StringTable->insert(accelerator) : NULL;
   ParseAcceleratorString(mAcceleratorString, mAcceleratorKeyCode, mAcceleratorModifier);

   // Re-register immediately if already attached - clears any previous binding first so
   // changing the accelerator at runtime never leaves a stale one registered alongside it.
   NewGuiCanvas* canvas = getOwningCanvas();
   if (canvas)
      canvas->removeAccelerators(this);

   updateAcceleratorRegistration();
}

void NewGuiButton::updateAcceleratorRegistration()
{
   if (mAcceleratorKeyCode == KEY_NULL)
      return;

   NewGuiCanvas* canvas = getOwningCanvas();
   if (!canvas)
      return;

   canvas->registerAccelerator(this, mAcceleratorKeyCode, mAcceleratorModifier);
}

void NewGuiButton::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Button");

   ADD_FIELD("text", TypeString, 0)
      .onSet(_setText)
      .doc("The label text this button displays.");

   ADD_FIELD("buttonType", TypeString, 0)
      .onSet(_setButtonType)
      .doc("push (default, momentary - onClick only), toggle (latches mChecked, onClick + onToggle), or radio (latches mChecked and clears every other radio-type sibling sharing groupNum).");

   ADD_FIELD("groupNum", TypeS32, Offset(mGroupNum, NewGuiButton))
      .onSet(_setGroupNum)
      .doc("Mutual-exclusion tag for buttonType == radio. Only siblings under the same parent, sharing this same groupNum, are cleared against each other.");

   ADD_FIELD("accelerator", TypeString, 0)
      .onSet(_setAccelerator)
      .doc("Keyboard accelerator, using ActionMap's own grammar (e.g. \"ctrl s\", \"ctrl-shift s\" - modifiers joined by '-', a space, then the key). Fires performClick() regardless of what currently has keyboard focus, unless the focused control wants raw keyboard input itself (see NewGuiCanvas::checkAccelerators()). Empty/unset means no accelerator.");

   ADD_FIELD("checkboxStyle", TypeBool, Offset(mCheckboxStyle, NewGuiButton))
      .onSet(_setCheckboxStyle)
      .doc("True draws a checkbox glyph beside the label instead of a bordered button with centered text. Purely a rendering-mode switch - does not change click/toggle behavior. See createCheckbox().");

   GROUP_END("Button");
}

void NewGuiButton::resolveFont()
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

// mCheckboxStyle reserves a square glyph box (side length == text line height) to the left of
// the label, same general "glyph box + label" shape whether or not the box has room to draw a
// check mark yet (EmitDrawCommands() decides that from mChecked, not this).
Point2I NewGuiButton::ComputePreferredSize()
{
   resolveFont();

   const NewGuiResolvedStyle& style = getResolvedStyle();

   mText.setBoxExtent(Point2I(0, 0));
   const NewGuiTextLayoutResult& result = mText.layout();

   S32 textWidth = 0;
   S32 textHeight = (mFont != NULL) ? (S32)mFont->getHeight() : 0;

   if (!result.lines.empty())
   {
      textWidth = result.blockBounds.extent.x;
      textHeight = getMax(textHeight, result.blockBounds.extent.y);
   }

   S32 glyphBoxSide = mCheckboxStyle ? textHeight : 0;
   S32 glyphBoxGap = mCheckboxStyle ? 4 : 0;   // Fixed gap between glyph box and label - not style-driven; same tier of detail as NewGuiScroll's fixed scrollbar thickness.

   S32 width = glyphBoxSide + glyphBoxGap + textWidth + (S32)style.padding.horizontal();
   S32 height = getMax(glyphBoxSide, textHeight) + (S32)style.padding.vertical();

   return Point2I(width, height);
}

// Background/border already reflects current interaction state via the style cascade - no
// button-specific "pressed" recoloring drawn here; that's what an authored hover|active rule is for.

void NewGuiButton::EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer)
{
   Parent::EmitDrawCommands(batch, bounds, style, layer);

   if (!batch || mFont == NULL || style.opacity <= 0.0f)
      return;

   RectI clientRect(
      Point2I(bounds.point.x + (S32)style.padding.left, bounds.point.y + (S32)style.padding.top),
      Point2I(getMax(0, bounds.extent.x - (S32)style.padding.horizontal()),
         getMax(0, bounds.extent.y - (S32)style.padding.vertical())));

   ColorI textColor(
      style.textColor.red,
      style.textColor.green,
      style.textColor.blue,
      (U8)((F32)style.textColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));

   if (mCheckboxStyle)
   {
      S32 glyphBoxSide = (mFont != NULL) ? (S32)mFont->getHeight() : clientRect.extent.y;

      // getText() never returns NULL (see NewGuiButton::getText()) - "" means no label
      // authored, which is the ordinary case for a bare checkbox (e.g. NewGuiInspector's bool
      // binding, which sets no text at all).
      bool hasLabel = getText()[0] != '\0';
      S32 glyphBoxGap = hasLabel ? 4 : 0;

      RectI glyphBox(
         Point2I(clientRect.point.x, clientRect.point.y + (clientRect.extent.y - glyphBoxSide) / 2),
         Point2I(glyphBoxSide, glyphBoxSide));

      // If this style has a skin image for "background" (or "border"), Parent::EmitDrawCommands()
      // above already drew it across the whole control's bounds - which, with no label, IS
      // glyphBox (see the double-border fix's own comment on why the two rects coincide in that
      // case). Drawing the flat-color box fill/border/checkmark on top of an already-drawn skin
      // image would layer solid quads/lines over it for no reason - a themed checkbox (see
      // gui_rev2/test's checkbox_image sprite sheet: 4 frames, normal/hover/checked/disabled,
      // selected per state via stateMask-scoped NewGuiSkinImageDef children on the style) is
      // meant to fully replace the flat glyph drawing, not sit underneath it.
      const NewGuiSkinImage* backgroundSkin = style.findSkinImage(StringTable->insert("background"));
      const NewGuiSkinImage* borderSkin = style.findSkinImage(StringTable->insert("border"));
      bool hasSkin = (backgroundSkin && backgroundSkin->hasImage()) || (borderSkin && borderSkin->hasImage());

      if (!hasSkin)
      {
         // With no label, this control's own ComputePreferredSize() resolves to exactly
         // glyphBoxSide (plus padding) on both axes - i.e. bounds (the whole control, already
         // painted by Parent::EmitDrawCommands() above) and glyphBox end up the SAME rect. Drawing
         // a second background fill + border here on top of that would double-paint it (this is
         // what produced the visible double-border/gap artifact previously) for no visual benefit,
         // so only draw the glyph box's own fill/border when there's a label making the two rects
         // genuinely different shapes.
         if (hasLabel)
         {
            // Box background fill, then a 1px border traced with four lines - pushQuad()/pushLine()
            // are the only solid-color primitives NewGuiRenderBatch exposes (no combined
            // filled-rect-with-border call), same primitives every other control's background/border
            // drawing already goes through.
            batch->pushQuad(glyphBox, style.backgroundColor, layer);
            Point2I tl = glyphBox.point;
            Point2I tr(glyphBox.point.x + glyphBox.extent.x, glyphBox.point.y);
            Point2I bl(glyphBox.point.x, glyphBox.point.y + glyphBox.extent.y);
            Point2I br(glyphBox.point.x + glyphBox.extent.x, glyphBox.point.y + glyphBox.extent.y);
            batch->pushLine(tl, tr, style.borderColor, 1.0f, layer);
            batch->pushLine(tr, br, style.borderColor, 1.0f, layer);
            batch->pushLine(br, bl, style.borderColor, 1.0f, layer);
            batch->pushLine(bl, tl, style.borderColor, 1.0f, layer);
         }

         if (isChecked())
         {
            // Simple inset filled square as the check mark, rather than a drawn glyph - matches
            // this batch's rect/line-based primitives instead of reaching for a font glyph.
            // secondaryColor (not textColor) - matches the accent-fill convention NewGuiScroll's
            // thumb and NewGuiTree's selection highlight/icon already use; defaults to textColor
            // when unauthored (see NewGuiStyle::Cascade()), so existing styles that never set
            // secondaryColor draw identically to before, but a style CAN now distinguish "text
            // color" from "check-mark accent color" if it wants to.
            RectI checkMark(
               Point2I(glyphBox.point.x + 3, glyphBox.point.y + 3),
               Point2I(getMax(0, glyphBox.extent.x - 6), getMax(0, glyphBox.extent.y - 6)));
            batch->pushQuad(checkMark, style.secondaryColor, layer);
         }
      }
      // else: skinned - the frame Parent::EmitDrawCommands() already drew

      clientRect.point.x += glyphBoxSide + glyphBoxGap;
      clientRect.extent.x = getMax(0, clientRect.extent.x - glyphBoxSide - glyphBoxGap);
   }

   mText.setBoxExtent(clientRect.extent);
   mText.submit(*batch, clientRect.point, textColor, layer);
}

// Siblings-only (this->getGroup()'s immediate children), not a whole-tree search - avoids two
// unrelated radio groups elsewhere in the UI colliding over the same small groupNum.
void NewGuiButton::clearGroupSiblings()
{
   SimGroup* parent = getGroup();
   if (!parent)
      return;

   for (SimSet::iterator itr = parent->begin(); itr != parent->end(); ++itr)
   {
      if (*itr == this)
         continue;

      NewGuiButton* sibling = dynamic_cast<NewGuiButton*>(*itr);
      if (!sibling)
         continue;

      if (sibling->mButtonType == ButtonType_Radio && sibling->mGroupNum == mGroupNum && sibling->isChecked())
         sibling->setChecked(false);
   }
}

// The one place Push/Toggle/Radio behavior branches and onClick()/onToggle() fire. Both the real
// input path (onMouseUp()) and script funnel through here so the two can never disagree.
void NewGuiButton::performClick()
{
   switch (mButtonType)
   {
   case ButtonType_Push:
      break;   // Momentary - no latched state.

   case ButtonType_Toggle:
   {
      bool newState = !isChecked();
      setChecked(newState);
      onToggle_callback(newState);
      break;
   }

   case ButtonType_Radio:
   {
      // Re-clicking an already-checked radio button is a no-op state-wise - a radio group
      // always keeps exactly one member checked once any press has occurred.
      if (!isChecked())
      {
         clearGroupSiblings();
         setChecked(true);
         onToggle_callback(true);
      }
      break;
   }
   }

   onClick_callback();
   notifyNativeChange();
}

// Deliberately does not call Parent:: - Parent's own onMouseDown()/onMouseUp() only toggle
// mMouseActive and set event.handled, both of which are done directly here alongside the arming logic.
void NewGuiButton::onMouseDown(NewGuiInputEvent& event)
{
   setMouseActive(true);
   mPressArmed = true;
   event.handled = true;
}

void NewGuiButton::onMouseUp(NewGuiInputEvent& event)
{
   setMouseActive(false);

   // A valid click needs both: the press that started it landed here (mPressArmed), and the
   // release point is still within bounds - a press dragged off and released elsewhere is cancelled.
   bool wasArmed = mPressArmed;
   mPressArmed = false;

   const RectI localBounds(Point2I(0, 0), mBounds.extent);
   if (wasArmed && localBounds.pointInRect(event.localPoint))
      performClick();

   event.handled = true;
}

void NewGuiButton::onMouseLeave(NewGuiInputEvent& event)
{
   Parent::onMouseLeave(event);

   // Deliberately does not clear mPressArmed - a press that drags off and comes back before
   // release still counts as a click; only the release point decides (checked in onMouseUp()).
}

// Down (not Up) triggers - no drag-cancel concept exists for keyboard/gamepad activation the way it does for a mouse click.
void NewGuiButton::onActivate(NewGuiInputEvent& event)
{
   if (event.action != NewGuiInputAction::Down)
      return;

   performClick();
   event.handled = true;
}

// Funnels straight into the same click path a real mouse click or performClick() call already
// uses - an accelerator-triggered Save is indistinguishable downstream from a clicked Save.
void NewGuiButton::onAccelerator()
{
   performClick();
}

//-----------------------------------------------------------------------------
// Script (console) API
//-----------------------------------------------------------------------------
// Same gap as NewGuiLabel (see that file's own comment on this section) - "text" is an
// authorable field via ADD_FIELD(...).onSet(_setText), but setText()/getText() had no
// script-callable method form of their own.

DefineEngineMethod(NewGuiButton, setText, void, (const char* text), ,
   "Sets the button's label text.\n"
   "@ingroup GuiCore")
{
   object->setText(text);
}

DefineEngineMethod(NewGuiButton, getText, const char*, (), ,
   "@return The button's current label text.\n"
   "@ingroup GuiCore")
{
   return object->getText();
}
