//-----------------------------------------------------------------------------
// guiButtonCtrlNew.cpp
//-----------------------------------------------------------------------------

#include "gui_refactor/controls/buttons/guiButtonCtrlNew.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"

IMPLEMENT_CONOBJECT(GuiButtonCtrlNew);

//-----------------------------------------------------------------------------

ImplementEnumType(GuiButtonMode,
   "How a GuiButtonCtrlNew responds to clicks.\n\n")
{
   GuiButtonMode_Push, "push",
      "Fires once per completed click; no persistent state."
},
{ GuiButtonMode_Toggle, "toggle",
   "Click flips a checked state, which stays until clicked again." },
{ GuiButtonMode_Check, "check",
   "Same checked-state behavior as toggle; distinct mode for clarity/authoring." },
   EndImplementEnumType;

//-----------------------------------------------------------------------------

GuiButtonCtrlNew::GuiButtonCtrlNew()
   : mMode(GuiButtonMode_Push),
   mChecked(false),
   mPressStarted(false)
{
   setCapturesInput(true);

   mGuiText.mAlignH = GuiTextAlignHorizontal_Center;
   mGuiText.mAlignV = GuiTextAlignVertical_Middle;
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::initPersistFields()
{
   docsURL;
   addGroup("Button");

   addProtectedField("text", TypeRealString, Offset(mText, GuiButtonCtrlNew), &setTextProt, &defaultProtectedGetFn,
      "The button's caption.");

   addField("mode", TYPEID< GuiButtonMode >(), Offset(mMode, GuiButtonCtrlNew),
      "\"push\" (default), \"toggle\", or \"check\" -- see GuiButtonMode.");

   addField("checked", TypeBool, Offset(mChecked, GuiButtonCtrlNew),
      "Initial checked state for toggle/check mode. Setting this directly does not fire onAction().");

   endGroup("Button");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool GuiButtonCtrlNew::onWake()
{
   if (!Parent::onWake())
      return false;

   mGuiText.setText(mText);

   return true;
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::setText(const String& text)
{
   if (String::compare(mText.c_str(), text.c_str()) == 0)
      return;

   mText = text;
   mGuiText.setText(mText);
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::setChecked(bool checked)
{
   if (mChecked == checked)
      return;

   mChecked = checked;
   setUpdate(); // style state (GuiStyleState::Checked) depends on this
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::_fireClick()
{
   if (mMode == GuiButtonMode_Toggle || mMode == GuiButtonMode_Check)
      setChecked(!mChecked);

   onAction();
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::onMouseDown(const GuiEvent& event)
{
   if (!mVisible || !mAwake || !mActive)
      return;

   mPressStarted = true;
   mouseLock();
   setFirstResponder();
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::onMouseUp(const GuiEvent& event)
{
   const bool wasPressStarted = mPressStarted;
   mPressStarted = false;

   if (isMouseLocked())
      mouseUnlock();

   if (isFirstResponder())
      clearFirstResponder();

   if (!mVisible || !mAwake || !mActive || !wasPressStarted)
      return;

   // Only a genuine click -- press started on this button AND release
   // landed back on it -- fires. A press that dragged off and released
   // elsewhere does not (standard button feel).
   //
   // event.mousePoint is canvas-global logical space; pointInControl()
   // expects a point in THIS control's PARENT's local space (see its own
   // doc comment/implementation in guiControlNew.cpp). Converting via
   // parent->globalToLocalCoord() -- which walks the FULL ancestor chain
   // (parent, parent's parent, ... up to the root) -- is what's actually
   // needed here, not a single "subtract this parent's own position"
   // step: that one-level subtraction only happens to produce the right
   // answer when parent is itself a direct child of the canvas, and
   // silently produces the WRONG point (and therefore a pointInControl()
   // that incorrectly reports false even for a click squarely on the
   // button) for any deeper nesting -- e.g. a button inside a row inside
   // a scrollable panel, two levels of accumulated parent offset above
   // the immediate parent alone. That's what let this button's mouse-
   // down correctly grab focus/lock (onMouseDown() has no such
   // conversion to get wrong) while every plain click silently failed to
   // fire at all.
   GuiControlNew* parent = getParent();
   const Point2I parentLocalPoint = parent ? parent->globalToLocalCoord(event.mousePoint) : event.mousePoint;

   if (pointInControl(parentLocalPoint))
      _fireClick();

   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::onMouseLeave(const GuiEvent& event)
{
   Parent::onMouseLeave(event);
   setUpdate(); // Hover style state changes even though nothing else does
}

//-----------------------------------------------------------------------------

bool GuiButtonCtrlNew::onKeyDown(const GuiEvent& event)
{
   if (!mActive)
      return Parent::onKeyDown(event);

   if (event.keyCode == KEY_RETURN || event.keyCode == KEY_NUMPADENTER)
   {
      // If the mouse button is ALSO currently held down over this
      // control (mPressStarted true -- set by onMouseDown(), only
      // cleared by the matching onMouseUp()), firing the click here via
      // Enter and leaving mPressStarted set means the eventual
      // onMouseUp() -- once the mouse is finally released, whenever that
      // happens -- fires _fireClick() a SECOND, independent time (see
      // onMouseUp(): it fires whenever wasPressStarted was true AND the
      // release point is still inside the control, with no awareness
      // that a click already happened via keyboard in between). For a
      // push-mode button that's a duplicate command invocation; for
      // toggle/check mode it silently flips the state right back to
      // where it started, which is exactly what let a checkbox-style
      // inspector row look like Enter "did nothing" -- the object's
      // field genuinely was written correctly by the first (keyboard)
      // fire, then immediately overwritten back by the second
      // (mouse-up) fire once the button was finally released. Clearing
      // mPressStarted here means that pending mouse-up sees
      // wasPressStarted == false and correctly treats the eventual
      // release as already accounted for, not a second genuine click.
      mPressStarted = false;
      _fireClick();
      return true;
   }

   return Parent::onKeyDown(event);
}

//-----------------------------------------------------------------------------

RectI GuiButtonCtrlNew::_renderCheckboxIndicator(const RectI& ctrlRect, const GuiStyleProperties& style)
{
   if (mMode != GuiButtonMode_Check)
      return RectI(ctrlRect.point, Point2I(0, 0));

   GuiCanvasNew* root = getRoot();
   if (!root)
      return RectI(ctrlRect.point, Point2I(0, 0));
   GuiRenderBatch& batch = root->getRenderBatch();

   // Sized off the control's own device height rather than the font's
   // line height -- no font-metrics dependency needed for a plain box,
   // and it keeps the indicator's size stable even if ShrinkToFit ever
   // kicks the caption down to a smaller point size.
   const S32 boxSize = getMax((S32)((F32)ctrlRect.extent.y * 0.6f), 4);
   const S32 margin = getMax((ctrlRect.extent.y - boxSize) / 2, 0);

   const Point2I boxPos(ctrlRect.point.x + margin, ctrlRect.point.y + margin);
   const RectI boxRect(boxPos, Point2I(boxSize, boxSize));

   // Quad 1: the outer box, flat-filled in the border color if the style
   // set one (falling back to text color) -- deliberately a single flat
   // quad rather than four thin edge quads; this whole indicator is a
   // placeholder for a real checkmark glyph once skinned Gui classes
   // exist (see gui-rewrite-design.md), so it isn't worth the extra draw
   // calls to hollow it out for now.
   const ColorI frameColor = style.borderColor.isSet() ? style.borderColor.mValue
      : (style.textColor.isSet() ? style.textColor.mValue : ColorI(255, 255, 255, 255));
   batch.pushQuad(boxRect, frameColor, getRenderLayer());

   // Quad 2: the checked-state fill -- a smaller inset quad, only drawn
   // when mChecked, in the background color (falling back to a plain
   // dark fill) so it reads against the frame quad above.
   if (mChecked)
   {
      const S32 inset = getMax(boxSize / 4, 2);
      const RectI fillRect(
         Point2I(boxRect.point.x + inset, boxRect.point.y + inset),
         Point2I(getMax(boxSize - inset * 2, 1), getMax(boxSize - inset * 2, 1)));

      const ColorI fillColor = style.backgroundColor.isSet() ? style.backgroundColor.mValue : ColorI(32, 32, 32, 255);
      batch.pushQuad(fillRect, fillColor, getRenderLayer());
   }

   // Reserve the box plus one box-width of breathing room before the caption.
   return RectI(boxRect.point, Point2I(boxSize + margin, boxSize));
}

//-----------------------------------------------------------------------------

void GuiButtonCtrlNew::onRender(Point2I offset, const RectI& updateRect)
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

   // Check mode reserves space on the left for the indicator box, same
   // as the two quads it draws being purely additive chrome -- Push/
   // Toggle draw nothing here and get the reservation's zero-extent
   // no-op, so their caption still fills/centers across the full rect
   // exactly as before this existed.
   const RectI indicatorRect = _renderCheckboxIndicator(ctrlRect, style);
   const S32 textOffsetX = (mMode == GuiButtonMode_Check) ? indicatorRect.extent.x : 0;

   const Point2I textOffset(offset.x + textOffsetX, offset.y);
   const Point2I textExtent(getMax(ctrlRect.extent.x - textOffsetX, 0), ctrlRect.extent.y);
   renderText(mGuiText, textOffset, textExtent);

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------

bool GuiButtonCtrlNew::setTextProt(void* object, const char* index, const char* data)
{
   static_cast<GuiButtonCtrlNew*>(object)->setText(data);
   return false;
}
