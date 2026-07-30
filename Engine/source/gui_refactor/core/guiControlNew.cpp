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

#include "platform/platform.h"
#include "gui_refactor/core/guiControlNew.h"

#include "console/consoleTypes.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "console/script.h"
#include "gfx/bitmap/gBitmap.h"
#include "sim/actionMap.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "gui/editor/guiEditCtrl.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "console/typeValidators.h"

#include <algorithm>


//#define DEBUG_SPEW


IMPLEMENT_CONOBJECT_CHILDREN(GuiControlNew);

ConsoleDocClass(GuiControlNew,
   "@brief Base class for all Gui control objects.\n\n"

   "GuiControlNew is the basis for the Gui system.  It represents an individual control that can be placed on the canvas and with which "
   "the mouse and keyboard can potentially interact with.\n\n"

   "@section GuiControl_Hierarchy Control Hierarchies\n"

   "GuiControls are arranged in a hierarchy.  All children of a control are placed in their parent's coordinate space, i.e. their "
   "coordinates are relative to the upper left corner of their immediate parent.  When a control is moved, all its child controls "
   "are moved along with it.\n\n"

   "Since GuiControlNew's are SimGroups, hierarchy also implies ownership.  This means that if a control is destroyed, all its children "
   "are destroyed along with it.  It also means that a given control can only be part of a single GuiControlNew hierarchy.  When adding a "
   "control to another control, it will automatically be reparented from another control it may have previously been parented to.\n\n"

   "@section GuiControl_Layout Layout System\n"

   "GuiControls have a two-dimensional position and are rectangular in shape.\n\n"

   "@section GuiControl_Events Event System\n"

   "@section GuiControl_Profiles Control Profiles\n"

   "Common data accessed by GuiControls is stored in so-called \"Control Profiles.\"  This includes font, color, and texture information. "
   "By pooling this data in shared objects, the appearance of any number of controls can be changed quickly and easily by modifying "
   "only the shared profile object.\n\n"

   "If not explicitly assigned a profile, a control will by default look for a profile object that matches its class name.  This means "
   "that the class GuiMyCtrl, for example, will look for a profile called 'GuiMyProfile'.  If this profile cannot be found, the control "
   "will fall back to GuiDefaultProfile which must be defined in any case for the Gui system to work.\n\n"

   "In addition to its primary profile, a control may be assigned a second profile called 'tooltipProfile' that will be used to render "
   "tooltip popups for the control.\n\n"

   "@section GuiControl_Actions Triggered Actions\n"

   "@section GuiControl_FirstResponders First Responders\n"

   "At any time, a single control can be what is called the \"first responder\" on the GuiCanvasNew is placed on.  This control "
   "will be the first control to receive keyboard events not bound in the global ActionMap.  If the first responder choses to "
   "handle a particular keyboard event, \n\n"

   "@section GuiControl_Waking Waking and Sleeping\n"

   "@section GuiControl_VisibleActive Visibility and Activeness\n"
   "By default, a GuiControlNew is active which means that it\n\n"

   "@see GuiCanvasNew\n"
   "@see GuiStyle\n"
   "@ingroup GuiCore\n"
);

IMPLEMENT_CALLBACK(GuiControlNew, onAdd, void, (), (),
   "Called when the control object is registered with the system after the control has been created.");
IMPLEMENT_CALLBACK(GuiControlNew, onRemove, void, (), (),
   "Called when the control object is removed from the system before it is deleted.");
IMPLEMENT_CALLBACK(GuiControlNew, onWake, void, (), (),
   "Called when the control is woken up.\n"
   "@ref GuiControl_Waking");
IMPLEMENT_CALLBACK(GuiControlNew, onSleep, void, (), (),
   "Called when the control is put to sleep.\n"
   "@ref GuiControl_Waking");
IMPLEMENT_CALLBACK(GuiControlNew, onGainFirstResponder, void, (), (),
   "Called when the control gains first responder status on the GuiCanvasNew.\n"
   "@see setFirstResponder\n"
   "@see makeFirstResponder\n"
   "@see isFirstResponder\n"
   "@ref GuiControl_FirstResponders");
IMPLEMENT_CALLBACK(GuiControlNew, onLoseFirstResponder, void, (), (),
   "Called when the control loses first responder status on the GuiCanvasNew.\n"
   "@see setFirstResponder\n"
   "@see makeFirstResponder\n"
   "@see isFirstResponder\n"
   "@ref GuiControl_FirstResponders");
IMPLEMENT_CALLBACK(GuiControlNew, onAction, void, (), (),
   "Called when the control's associated action is triggered and no 'command' is defined for the control.\n"
   "@ref GuiControl_Actions");
IMPLEMENT_CALLBACK(GuiControlNew, onVisible, void, (bool state), (state),
   "Called when the control changes its visibility state, i.e. when going from visible to invisible or vice versa.\n"
   "@param state The new visibility state.\n"
   "@see isVisible\n"
   "@see setVisible\n"
   "@ref GuiControl_VisibleActive");
IMPLEMENT_CALLBACK(GuiControlNew, onActive, void, (bool state), (state),
   "Called when the control changes its activeness state, i.e. when going from active to inactive or vice versa.\n"
   "@param stat The new activeness state.\n"
   "@see isActive\n"
   "@see setActive\n"
   "@ref GuiControl_VisibleActive");
IMPLEMENT_CALLBACK(GuiControlNew, onDialogPush, void, (), (),
   "Called when the control is pushed as a dialog onto the canvas.\n"
   "@see GuiCanvasNew::pushDialog");
IMPLEMENT_CALLBACK(GuiControlNew, onDialogPop, void, (), (),
   "Called when the control is removed as a dialog from the canvas.\n"
   "@see GuiCanvasNew::popDialog");
IMPLEMENT_CALLBACK(GuiControlNew, onControlDragEnter, void, (GuiControlNew* control, const Point2I& dropPoint), (control, dropPoint),
   "Called when a drag&drop operation through GuiDragAndDropControl has entered the control.  This is only called for "
   "topmost visible controls as the GuiDragAndDropControl moves over them.\n\n"
   "@param control The payload of the drag operation.\n"
   "@param dropPoint The point at which the payload would be dropped if it were released now.  Relative to the canvas.");
IMPLEMENT_CALLBACK(GuiControlNew, onControlDragExit, void, (GuiControlNew* control, const Point2I& dropPoint), (control, dropPoint),
   "Called when a drag&drop operation through GuiDragAndDropControl has exited the control and moved over a different control.  This is only called for "
   "topmost visible controls as the GuiDragAndDropControl moves off of them.\n\n"
   "@param control The payload of the drag operation.\n"
   "@param dropPoint The point at which the payload would be dropped if it were released now.  Relative to the canvas.");
IMPLEMENT_CALLBACK(GuiControlNew, onControlDragged, void, (GuiControlNew* control, const Point2I& dropPoint), (control, dropPoint),
   "Called when a drag&drop operation through GuiDragAndDropControl is moving across the control after it has entered it.  This is only called for "
   "topmost visible controls as the GuiDragAndDropControl moves across them.\n\n"
   "@param control The payload of the drag operation.\n"
   "@param dropPoint The point at which the payload would be dropped if it were released now.  Relative to the canvas.");
IMPLEMENT_CALLBACK(GuiControlNew, onControlDropped, void, (GuiControlNew* control, const Point2I& dropPoint), (control, dropPoint),
   "Called when a drag&drop operation through GuiDragAndDropControl has completed and is dropping its payload onto the control.  "
   "This is only called for topmost visible controls as the GuiDragAndDropControl drops its payload on them.\n\n"
   "@param control The control that is being dropped onto this control.\n"
   "@param dropPoint The point at which the control is being dropped.  Relative to the canvas.");


GuiControlNew* GuiControlNew::smPrevResponder = NULL;
GuiControlNew* GuiControlNew::smCurResponder = NULL;
GuiEditCtrlNew* GuiControlNew::smEditorHandle = NULL;
bool        GuiControlNew::smDesignTime = false;
GuiControlNew* GuiControlNew::smThisControl;

IMPLEMENT_SCOPE(GuiAPINew, Gui, , "");

//-----------------------------------------------------------------------------
// GuiDimension struct field reflection, for engine introspection (inspector, serialization).
//-----------------------------------------------------------------------------
IMPLEMENT_STRUCT(GuiDimension,
   GuiDimension, GuiAPINew,
   "A CSS-style layout value: auto, a logical-pixel amount, or a percentage of the parent's size.")

   END_IMPLEMENT_STRUCT;

//-----------------------------------------------------------------------------
// GuiDimension console type -- parses "auto" / "120" / "120px" / "50%" into
// a GuiDimension once, at field-set time. See guiDimension.h.
//-----------------------------------------------------------------------------

ConsoleType(GuiDimension, TypeGuiDimension, GuiDimension, "")
ImplementConsoleTypeCasters(TypeGuiDimension, GuiDimension)

ConsoleGetType(TypeGuiDimension)
{
   GuiDimension* dim = (GuiDimension*)dptr;
   return dim->toString();
}

ConsoleSetType(TypeGuiDimension)
{
   if (argc == 1)
      *((GuiDimension*)dptr) = GuiDimension::parse(argv[0]);
   else
      Con::printf("GuiDimension must be set as a single string, e.g. \"auto\", \"120\", \"120px\", or \"50%%\"");
}

//-----------------------------------------------------------------------------

GuiControlNew::GuiControlNew() : mAddGroup(NULL),
mBounds(0, 0, 64, 64),
mStyle(NULL),
mTooltipStyle(NULL),
mHasError(false),
mTipHoverTime(1000),
mCachedTooltipStyleGeneration(NULL),
mCachedTooltipValid(false),
mVisible(true),
mActive(true),
mAwake(false),
mIsContainer(false),
mCanResize(true),
mCanHit(true),
mAllowOverflow(false),
mTabable(true),
mFocusable(true),
mCapturesInput(false),
mTabIndex(-1),
mLayer(0),
mRenderLayer(0),
mRenderLayerExplicit(false),
mLangTable(NULL),
mFirstResponder(NULL),
mWidth(GuiDimension::autoValue()),
mHeight(GuiDimension::autoValue()),
mMinWidth(GuiDimension::autoValue()),
mMaxWidth(GuiDimension::autoValue()),
mMinHeight(GuiDimension::autoValue()),
mMaxHeight(GuiDimension::autoValue()),
mLeft(GuiDimension::autoValue()),
mTop(GuiDimension::autoValue()),
mRight(GuiDimension::autoValue()),
mBottom(GuiDimension::autoValue()),
mCenterHorizontal(false),
mCenterVertical(false),
mPreserveAspectRatio(false),
mPendingPercentAxes(0),
mLayoutDirty(true),
mCategory(StringTable->EmptyString())
{
   mConsoleVariable = StringTable->EmptyString();
   mAcceleratorKey = StringTable->EmptyString();
   mLangTableName = StringTable->EmptyString();

   mTooltip = StringTable->EmptyString();
   mRenderTooltipDelegate.bind(this, &GuiControlNew::defaultTooltipRender);

   mCanSaveFieldDictionary = false;
   mNotifyChildrenResized = true;
   fade_amt = 1.0f;
}

//-----------------------------------------------------------------------------

GuiControlNew::~GuiControlNew()
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::consoleInit()
{
   Con::addVariable("$ThisControl", TYPEID< GuiControlNew >(), &smThisControl,
      "The control for which a command is currently being evaluated.  Only set during 'command' "
      "and altCommand callbacks to the control for which the command or altCommand is invoked.\n"
      "@ingroup GuiCore");
}

//-----------------------------------------------------------------------------

void GuiControlNew::initPersistFields()
{
   docsURL;
   addGroup("Layout");

   addProtectedField("width", TYPEID< GuiDimension >(), Offset(mWidth, GuiControlNew), &setWidthProt, &defaultProtectedGetFn,
      "Width of the control: \"auto\" (keep current size), a logical-pixel number (e.g. \"200\" or \"200px\"), "
      "or a percentage of the immediate parent's current width (e.g. \"50%\").");
   addProtectedField("height", TYPEID< GuiDimension >(), Offset(mHeight, GuiControlNew), &setHeightProt, &defaultProtectedGetFn,
      "Height of the control: \"auto\" (keep current size), a logical-pixel number, or a percentage of the "
      "immediate parent's current height (e.g. \"50%\").");
   addProtectedField("minWidth", TYPEID< GuiDimension >(), Offset(mMinWidth, GuiControlNew), &setMinWidthProt, &defaultProtectedGetFn,
      "Minimum resolved width, same value grammar as width. \"auto\" means unbounded.");
   addProtectedField("maxWidth", TYPEID< GuiDimension >(), Offset(mMaxWidth, GuiControlNew), &setMaxWidthProt, &defaultProtectedGetFn,
      "Maximum resolved width, same value grammar as width. \"auto\" means unbounded.");
   addProtectedField("minHeight", TYPEID< GuiDimension >(), Offset(mMinHeight, GuiControlNew), &setMinHeightProt, &defaultProtectedGetFn,
      "Minimum resolved height, same value grammar as height. \"auto\" means unbounded.");
   addProtectedField("maxHeight", TYPEID< GuiDimension >(), Offset(mMaxHeight, GuiControlNew), &setMaxHeightProt, &defaultProtectedGetFn,
      "Maximum resolved height, same value grammar as height. \"auto\" means unbounded.");

   addProtectedField("left", TYPEID< GuiDimension >(), Offset(mLeft, GuiControlNew), &setLeftProt, &defaultProtectedGetFn,
      "Position of the control's left edge, offset from the parent's left edge: \"auto\" (don't position on "
      "this axis, keep current position), a logical-pixel number, or a percentage of the parent's width. "
      "Ignored if centerHorizontal is set; if both left and right are set, left wins.");
   addProtectedField("top", TYPEID< GuiDimension >(), Offset(mTop, GuiControlNew), &setTopProt, &defaultProtectedGetFn,
      "Position of the control's top edge, offset from the parent's top edge. Same grammar as left. "
      "Ignored if centerVertical is set; if both top and bottom are set, top wins.");
   addProtectedField("right", TYPEID< GuiDimension >(), Offset(mRight, GuiControlNew), &setRightProt, &defaultProtectedGetFn,
      "Position of the control's right edge, offset from the parent's right edge (positive values move the "
      "control's right edge inward, away from the parent's right edge). Same grammar as left. Ignored if "
      "left or centerHorizontal is set.");
   addProtectedField("bottom", TYPEID< GuiDimension >(), Offset(mBottom, GuiControlNew), &setBottomProt, &defaultProtectedGetFn,
      "Position of the control's bottom edge, offset from the parent's bottom edge. Same grammar as right. "
      "Ignored if top or centerVertical is set.");

   addProtectedField("centerHorizontal", TypeBool, Offset(mCenterHorizontal, GuiControlNew), &setCenterHorizontalProt, &defaultProtectedGetFn,
      "Centers the control horizontally within its parent's client area, overriding left/right entirely.");
   addProtectedField("centerVertical", TypeBool, Offset(mCenterVertical, GuiControlNew), &setCenterVerticalProt, &defaultProtectedGetFn,
      "Centers the control vertically within its parent's client area, overriding top/bottom entirely.");
   addProtectedField("preserveAspectRatio", TypeBool, Offset(mPreserveAspectRatio, GuiControlNew), &setPreserveAspectRatioProt, &defaultProtectedGetFn,
      "After this control's box is otherwise fully resolved, shrinks it (never grows) so its own authored "
      "width:height ratio is preserved when it would otherwise be clamped to fit the parent -- a contain-style "
      "fit, useful for e.g. a bitmap that would otherwise be squashed/stretched. Position is left alone. "
      "Independent of GuiCanvasNew's lockAspectRatio, which governs the whole design's letterboxing into the "
      "window instead.");

   endGroup("Layout");

   addGroup("Rendering");

   addProtectedField("renderLayer", TypeS32, Offset(mRenderLayer, GuiControlNew), &setRenderLayerProt, &defaultProtectedGetFn,
      "Explicit override for this control's draw-order layer within GuiRenderBatch -- higher values paint "
      "later (on top). Defaults to the parent's renderLayer + 1 and is normally left alone; set this "
      "directly only to force a specific control (e.g. a scrollbar thumb that must always paint over "
      "scrolled content) to win against other controls at the same depth. See bringToFront()/sendToBack() "
      "for the common cases, which don't require picking a layer number by hand.");

   endGroup("Rendering");

   addGroup("Control");

   addProtectedField("style", TYPEID< GuiStyle >(), Offset(mStyle, GuiControlNew), &setStyleProt, &defaultProtectedGetFn,
      "The style that determines fill colors, borders, font settings, etc. Replaces the old GuiControlProfile "
      "'profile' field -- see GuiStyle.");

   endGroup("Control");

   addGroup("Inline Style Overrides");

   addProtectedField("backgroundColor", TypeColorI, Offset(mInlineStyleOverrides.backgroundColor.mValue, GuiControlNew), &setInlineBackgroundColorProt, &defaultProtectedGetFn,
      "Overrides this control's resolved backgroundColor, regardless of what 'style' or its own current "
      "interaction state would otherwise produce -- the same way inline style=\"\" beats a CSS class. Leave "
      "unset to let 'style' fully control this property.");
   addProtectedField("borderColor", TypeColorI, Offset(mInlineStyleOverrides.borderColor.mValue, GuiControlNew), &setInlineBorderColorProt, &defaultProtectedGetFn,
      "Overrides this control's resolved borderColor. See backgroundColor's doc comment.");
   addProtectedField("borderWidth", TypeS32, Offset(mInlineStyleOverrides.borderWidth.mValue, GuiControlNew), &setInlineBorderWidthProt, &defaultProtectedGetFn,
      "Overrides this control's resolved borderWidth. See backgroundColor's doc comment.");
   addProtectedField("textColor", TypeColorI, Offset(mInlineStyleOverrides.textColor.mValue, GuiControlNew), &setInlineTextColorProt, &defaultProtectedGetFn,
      "Overrides this control's resolved textColor. See backgroundColor's doc comment.");
   addProtectedField("fontFamily", TypeString, Offset(mInlineStyleOverrides.fontFamily.mValue, GuiControlNew), &setInlineFontFamilyProt, &defaultProtectedGetFn,
      "Overrides this control's resolved fontFamily. See backgroundColor's doc comment.");
   addProtectedField("fontSize", TypeS32, Offset(mInlineStyleOverrides.fontSize.mValue, GuiControlNew), &setInlineFontSizeProt, &defaultProtectedGetFn,
      "Overrides this control's resolved fontSize. See backgroundColor's doc comment.");
   addProtectedField("letterSpacing", TypeS32, Offset(mInlineStyleOverrides.letterSpacing.mValue, GuiControlNew), &setInlineLetterSpacingProt, &defaultProtectedGetFn,
      "Overrides this control's resolved letterSpacing. See backgroundColor's doc comment.");
   addProtectedField("wordSpacing", TypeS32, Offset(mInlineStyleOverrides.wordSpacing.mValue, GuiControlNew), &setInlineWordSpacingProt, &defaultProtectedGetFn,
      "Overrides this control's resolved wordSpacing. See backgroundColor's doc comment.");
   addProtectedField("textAlignHorizontal", TYPEID< GuiTextAlignHorizontal >(), Offset(mInlineStyleOverrides.textAlignHorizontal.mValue, GuiControlNew), &setInlineTextAlignHProt, &defaultProtectedGetFn,
      "Overrides this control's resolved horizontal text alignment: 0=left, 1=center, 2=right. See "
      "backgroundColor's doc comment.");
   addProtectedField("textAlignVertical", TYPEID< GuiTextAlignVertical >(), Offset(mInlineStyleOverrides.textAlignVertical.mValue, GuiControlNew), &setInlineTextAlignVProt, &defaultProtectedGetFn,
      "Overrides this control's resolved vertical text alignment: 0=top, 1=middle, 2=bottom. See "
      "backgroundColor's doc comment.");

   endGroup("Inline Style Overrides");

   addGroup("Control");

   addProtectedField("visible", TypeBool, Offset(mVisible, GuiControlNew), &_setVisible, &defaultProtectedGetFn,
      "Whether the control is visible or hidden.");
   addProtectedField("active", TypeBool, Offset(mActive, GuiControlNew), &_setActive, &defaultProtectedGetFn,
      "Whether the control is enabled for user interaction.");

   addField("capturesInput", TypeBool, Offset(mCapturesInput, GuiControlNew),
      "When true and this control is pushed as a dialog, mouse input is captured -- it will not reach "
      "controls behind this one. Replaces the old GuiControlProfile 'modal' field, renamed since it "
      "describes input capture, not visual modality.");
   addField("tabable", TypeBool, Offset(mTabable, GuiControlNew),
      "Whether this control participates in tab-key navigation. Defaults to true -- replaces the old "
      "GuiControlProfile 'tabable' field, which defaulted to false.");
   addField("focusable", TypeBool, Offset(mFocusable, GuiControlNew),
      "Whether this control can become the keyboard first responder. Replaces the old GuiControlProfile "
      "'canKeyFocus' field.");
   addFieldV("tabIndex", TypeS32, Offset(mTabIndex, GuiControlNew), &CommonValidators::NegDefaultInt,
      "Explicit tab-order override. -1 (the default) means 'use document order' (this control, then its "
      "children, before its next sibling). When >= 0, controls that set this are ordered by this number "
      "relative to each other instead, the same way HTML's tabindex works.");
   addDeprecatedField("setFirstResponder");

   addField("variable", TypeString, Offset(mConsoleVariable, GuiControlNew),
      "Name of the variable to which the value of this control will be synchronized.");
   addField("command", TypeCommand, Offset(mConsoleCommand, GuiControlNew),
      "Command to execute on the primary action of the control.\n\n"
      "@note Within this script snippet, the control on which the #command is being executed is bound to "
      "the global variable $ThisControl.");
   addField("altCommand", TypeCommand, Offset(mAltConsoleCommand, GuiControlNew),
      "Command to execute on the secondary action of the control.\n\n"
      "@note Within this script snippet, the control on which the #altCommand is being executed is bound to "
      "the global variable $ThisControl.");
   addField("accelerator", TypeString, Offset(mAcceleratorKey, GuiControlNew),
      "Key combination that triggers the control's primary action when the control is on the canvas.");

   addField("category", TypeString, Offset(mCategory, GuiControlNew),
      "Name of the category this gui control should be grouped into for organizational purposes. Primarily for tooling.");


   endGroup("Control");

   addGroup("ToolTip");
   addProtectedField("tooltipStyle", TYPEID< GuiStyle >(), Offset(mTooltipStyle, GuiControlNew), &setTooltipStyleProt, &defaultProtectedGetFn,
      "Style to use when rendering tooltips for this control.");
   addField("tooltip", TypeRealString, Offset(mTooltip, GuiControlNew),
      "String to show in tooltip for this control.");
   addFieldV("hovertime", TypeRangedS32, Offset(mTipHoverTime, GuiControlNew), &CommonValidators::PositiveInt,
      "Time for mouse to hover over control until tooltip is shown (in milliseconds).");
   endGroup("ToolTip");

   addGroup("Editing");
   addField("isContainer", TypeBool, Offset(mIsContainer, GuiControlNew),
      "If true, the control may contain child controls.");
   endGroup("Editing");

   addGroup("Localization");
   addField("langTableMod", TypeString, Offset(mLangTableName, GuiControlNew),
      "Name of string table to use for lookup of internationalized text.");
   endGroup("Localization");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool GuiControlNew::processArguments(S32 argc, ConsoleValue* argv)
{
   // argv[0]: optional GuiGroup (by internal name) to add this control to on creation.
   if (argc == 1)
   {
      StringTableEntry steIntName = StringTable->insert(argv[0]);
      mAddGroup = dynamic_cast<SimGroup*>(Sim::getGuiGroup()->findObjectByInternalName(steIntName));
      if (mAddGroup == NULL)
      {
         mAddGroup = new SimGroup();
         if (mAddGroup->registerObject())
         {
            mAddGroup->setInternalName(steIntName);
            Sim::getGuiGroup()->addObject(mAddGroup);
         }
         else
         {
            SAFE_DELETE(mAddGroup);
            return false;
         }
      }
      mAddGroup->addObject(this);
   }
   return true;
}

//-----------------------------------------------------------------------------

void GuiControlNew::awaken()
{
   PROFILE_SCOPE(GuiControl_awaken);

#ifdef DEBUG_SPEW
   Platform::outputDebugString("[GuiControlNew] Waking up %i:%s (%s:%s) awake=%s",
      getId(), getClassName(), getName(), getInternalName(),
      mAwake ? "true" : "false");
#endif

   if (mAwake)
      return;

   // Wake visible children first.
   for (GuiControlNew::iterator iter = begin(); iter != end(); ++iter)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*iter);

      if (ctrl->isVisible() && !ctrl->isAwake())
         ctrl->awaken();
   }

   if (!mAwake && !onWake())
   {
      Con::errorf(ConsoleLogEntry::General, "GuiControlNew::awaken: failed onWake for obj: %i:%s (%s)",
         getId(), getClassName(), getName());
      mAwake = false;
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::sleep()
{
#ifdef DEBUG_SPEW
   Platform::outputDebugString("[GuiControlNew] Putting to sleep %i:%s (%s:%s) awake=%s",
      getId(), getClassName(), getName(), getInternalName(),
      mAwake ? "true" : "false");
#endif

   if (!mAwake)
      return;

   iterator i;
   for (i = begin(); i != end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl->isAwake())
         ctrl->sleep();
   }

   if (mAwake)
      onSleep();
}

//=============================================================================
//    Rendering.
//=============================================================================
// MARK: ---- Rendering ----

//-----------------------------------------------------------------------------

void GuiControlNew::preRender()
{
   if (!mAwake)
      return;

   iterator i;
   for (i = begin(); i != end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      ctrl->preRender();
   }
   onPreRender();
}

//-----------------------------------------------------------------------------

void GuiControlNew::onPreRender()
{
   // do nothing.
}

//-----------------------------------------------------------------------------

void GuiControlNew::onRender(Point2I offset, const RectI& updateRect)
{
   // offset is this control's device-pixel top-left corner.
   RectI ctrlRect(offset, getDeviceBounds().extent);

   const GuiStyleProperties style = resolveStyle();

   // Submit into the canvas's per-frame GuiRenderBatch rather than calling GFXDrawUtil directly.
   GuiCanvasNew* root = getRoot();
   if (!root)
      return;
   GuiRenderBatch& batch = root->getRenderBatch();

   // An unset backgroundColor means "don't fill."
   if (style.backgroundColor.isSet())
      batch.pushQuad(ctrlRect, style.backgroundColor.mValue, getRenderLayer());

   // Simple flat border only for now.
   if (style.borderWidth.isSet() && style.borderWidth.mValue > 0 && style.borderColor.isSet())
   {
      const S32 bw = style.borderWidth.mValue;
      const ColorI& bc = style.borderColor.mValue;
      batch.pushQuad(RectI(ctrlRect.point, Point2I(ctrlRect.extent.x, bw)), bc, getRenderLayer()); // top
      batch.pushQuad(RectI(Point2I(ctrlRect.point.x, ctrlRect.point.y + ctrlRect.extent.y - bw), Point2I(ctrlRect.extent.x, bw)), bc, getRenderLayer()); // bottom
      batch.pushQuad(RectI(ctrlRect.point, Point2I(bw, ctrlRect.extent.y)), bc, getRenderLayer()); // left
      batch.pushQuad(RectI(Point2I(ctrlRect.point.x + ctrlRect.extent.x - bw, ctrlRect.point.y), Point2I(bw, ctrlRect.extent.y)), bc, getRenderLayer()); // right
   }

   // Render Children
   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------

bool GuiControlNew::defaultTooltipRender(const Point2I& hoverPos, const Point2I& cursorPos, const char* tipText)
{
   // Short Circuit.
   if (!mAwake)
      return false;

   if (dStrlen(mTooltip) == 0 && (tipText == NULL || dStrlen(tipText) == 0))
      return false;

   String renderTip(mTooltip);
   if (tipText != NULL)
      renderTip = tipText;

   GuiCanvasNew* root = getRoot();
   if (!root)
      return false;

   if (!mTooltipStyle)
      return false;

   // Re-resolve style/font and re-run text layout only when something
   // that would actually change the result has changed since the last
   // call -- this function is called every frame for the ENTIRE time a
   // tooltip is visible (see GuiCanvasNew::renderFrame()'s hover-time
   // check, not a one-shot "just started hovering" event), so without
   // this cache, GuiStyle::getResolvedFont() (and therefore
   // GFont::create()) and GuiText::layout() were both being re-run
   // continuously -- dozens of times a second -- for as long as a
   // tooltip stayed on screen, which traced back to unbounded memory
   // growth while a tooltip was rendering. The cache key is
   // (mTooltipStyle pointer identity, renderTip content): neither the
   // resolved style/font nor the wrapped layout can differ between two
   // calls that share both, since resolve(0)/getResolvedFont(0) always
   // use the same fixed state mask (0) for tooltips (see the comment
   // below) and layout only depends on text + font + wrap box, and the
   // wrap box here (S32_MAX, S32_MAX) never changes either.
   const bool cacheValid = mCachedTooltipValid
      && mCachedTooltipStyleGeneration == mTooltipStyle
      && String::compare(mCachedTooltipText.c_str(), renderTip.c_str()) == 0;

   if (!cacheValid)
   {
      // Resolve at base state only -- the tooltip box has no interaction state of its own.
      mCachedTooltipStyleProps = mTooltipStyle->resolve(0);
      mCachedTooltipFont = mTooltipStyle->getResolvedFont(0);

      if (!mCachedTooltipFont)
      {
         mCachedTooltipValid = false;
         return false;
      }

      mCachedTooltipGuiText.setFont(mCachedTooltipFont);
      mCachedTooltipGuiText.setWrap(true);
      mCachedTooltipGuiText.setAlignHorizontal(GuiTextAlignHorizontal::GuiTextAlignHorizontal_Left);
      mCachedTooltipGuiText.setAlignVertical(GuiTextAlignVertical::GuiTextAlignVertical_Top);
      mCachedTooltipGuiText.setBoxExtent(Point2I(S32_MAX, S32_MAX));
      mCachedTooltipGuiText.setText(renderTip);

      // layout() is what actually does the wrap/measure work (and
      // caches ITS OWN result internally -- see GuiText::mLayoutDirty --
      // but every setter call above already marks that dirty again, so
      // calling it here, once, right after configuring, is what avoids
      // redoing that work on every render call too, not just the
      // style/font resolve above it).
      const GuiTextLayoutResult& layoutResult = mCachedTooltipGuiText.layout();
      if (layoutResult.lines.empty())
      {
         mCachedTooltipValid = false;
         return false;
      }

      mCachedTooltipStyleGeneration = mTooltipStyle;
      mCachedTooltipText = renderTip;
      mCachedTooltipValid = true;
   }

   const GuiTextLayoutResult& layoutResult = mCachedTooltipGuiText.layout();
   if (layoutResult.lines.empty())
      return false;

   const U32 tipWidth = (U32)layoutResult.blockBounds.extent.x;
   const U32 tipHeight = (U32)layoutResult.blockBounds.extent.y;

   Point2I screensize = getRoot()->getWindowSize();
   Point2I offset = hoverPos;
   Point2I textBounds;

   offset.y += 20; // offset below cursor image

   const U32 vMargin = 2; // pixels above/below the text
   const U32 hMargin = 4; // pixels left/right of the text

   textBounds.x = tipWidth + hMargin * 2;
   textBounds.y = tipHeight + vMargin * 2;

   // Keep the tooltip fully on-screen, with a 5px edge buffer.
   if (screensize.x < offset.x + textBounds.x + 5)
      offset.x = screensize.x - textBounds.x - 5;
   if (screensize.y < offset.y + textBounds.y + 5)
      offset.y = hoverPos.y - textBounds.y - 5;

   RectI rect(offset, textBounds);

   GuiRenderBatch& batch = root->getRenderBatch();

   // Tooltips draw above everything else.
   const S32 tooltipLayer = getRenderLayer() + 100;

   if (mCachedTooltipStyleProps.backgroundColor.isSet())
      batch.pushQuad(rect, mCachedTooltipStyleProps.backgroundColor.mValue, tooltipLayer);
   if (mCachedTooltipStyleProps.borderColor.isSet())
   {
      // Flat border as four thin quads.
      const S32 bw = 1;
      const ColorI& bc = mCachedTooltipStyleProps.borderColor.mValue;
      batch.pushQuad(RectI(rect.point, Point2I(rect.extent.x, bw)), bc, tooltipLayer);
      batch.pushQuad(RectI(Point2I(rect.point.x, rect.point.y + rect.extent.y - bw), Point2I(rect.extent.x, bw)), bc, tooltipLayer);
      batch.pushQuad(RectI(rect.point, Point2I(bw, rect.extent.y)), bc, tooltipLayer);
      batch.pushQuad(RectI(Point2I(rect.point.x + rect.extent.x - bw, rect.point.y), Point2I(bw, rect.extent.y)), bc, tooltipLayer);
   }

   const ColorI textColor = mCachedTooltipStyleProps.textColor.isSet() ? mCachedTooltipStyleProps.textColor.mValue : ColorI(0, 0, 0);
   const Point2I textOrigin(rect.point.x + hMargin, rect.point.y + vMargin);
   mCachedTooltipGuiText.submit(batch, textOrigin, textColor, 0, 0, tooltipLayer);

   return true;
}

//-----------------------------------------------------------------------------

void GuiControlNew::renderChildControls(Point2I offset, const RectI& updateRect)
{
   // NOTE ON CLIPPING: GFX->setClipRect() below is legacy/cosmetic only --
   // it no longer clips anything the render batch actually draws. Every
   // onRender() call in this loop (transitively, all the way down) only
   // QUEUES primitives into GuiRenderBatch; nothing hits the GPU until
   // GuiCanvasNew::renderFrame()'s single end-of-frame flush(), by which
   // point every GFX->setClipRect() call made during the tree walk has
   // long since been overwritten by whatever ran after it. A primitive's
   // actual clip comes from GuiRenderBatch's OWN clip stack at the moment
   // it was pushed (see GuiBatchQuad::clip and GuiRenderBatch::
   // pushClipRect()'s doc comment) -- this per-child intersection here
   // does NOT feed that stack, and doesn't need to: updateRect is already
   // whatever clip an ancestor established (ultimately bottoming out at
   // GuiRenderBatch::begin()'s deviceViewport), and ordinary children
   // don't shrink it further -- only a container that wants its OWN
   // children visually confined to a sub-region (e.g. GuiScrollCtrlNew's
   // viewport) needs to call batch.pushClipRect()/popClipRect() around
   // its own renderChildControls() call, narrowing what THOSE children's
   // primitives get stamped with. See GuiScrollCtrlNew::onRender() for
   // that pattern.
   RectI savedClipRect = GFX->getClipRect();

   // offset is this control's device-pixel top-left corner; updateRect is
   // the device-space intersection rect, usable directly as the clip rect.
   RectI clipRect = updateRect;

   // This control's own device-pixel origin, used to re-express each
   // child's absolute device bounds relative to 'offset'.
   const Point2I thisDeviceOrigin = getDeviceBounds().point;

   iterator i;
   for (i = begin(); i != end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl->mVisible)
      {
         const RectI childDeviceBounds = ctrl->getDeviceBounds();

         // Re-express the child's absolute device position relative to our own device origin.
         Point2I childPosition = offset + (childDeviceBounds.point - thisDeviceOrigin);
         RectI childClip(childPosition, childDeviceBounds.extent + Point2I(1, 1));

         if (childClip.intersect(clipRect))
         {
            GFX->setClipRect(childClip);
            GFX->setStateBlock(mDefaultGuiSB);
            ctrl->onRender(childPosition, childClip);
         }
      }
   }

   GFX->setClipRect(savedClipRect);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setUpdateRegion(Point2I pos, Point2I ext)
{
   GuiCanvasNew* root = getRoot();
   if (!root)
      return;

   // pos/ext are logical, relative to this control -- project into device space via the canvas.
   const Point2I globalLogicalPos = localToGlobalCoord(pos);
   const Point2I upos = root->localToGlobal(globalLogicalPos);

   const Point2I uext(
      (S32)(ext.x * root->getEffectiveScaleX()),
      (S32)(ext.y * root->getEffectiveScaleY()));

   root->addUpdateRegion(upos, uext);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setUpdate()
{
   setUpdateRegion(Point2I(0, 0), getExtent());
}

//-----------------------------------------------------------------------------

void GuiControlNew::renderJustifiedText(Point2I offset, Point2I extent, const char* text)
{
   // Configures/submits mJustifiedTextCache directly -- deliberately NOT
   // routed through renderText(GuiText&, ...), even though that also
   // ends up calling GuiText::submit(): renderText() sources alignment
   // from mInlineStyleOverrides (the control's own sparse, un-cascaded
   // overrides -- see getInlineStyleOverrides()'s doc comment) and
   // unconditionally overwrites whatever alignment was set on its GuiText
   // argument beforehand, which is correct for renderText()'s own
   // contract but wrong for this function's ORIGINAL semantics: full
   // resolveStyle() cascade, defaulting to Left/Middle if unset anywhere
   // in that cascade (not GuiStyleValue's own default-construction
   // values, Left(0)/Top(0), which is what an unset mInlineStyleOverrides
   // would silently fall back to if this called through renderText()
   // instead). This still goes through GuiText -- see
   // mJustifiedTextCache's doc comment (guiControlNew.h) for why that
   // matters (a persistent, reused GuiText, not a fresh one or a raw
   // GFont measure/align each call) -- just not through renderText()'s
   // narrower alignment contract.
   if (!mStyle)
      return;

   GuiCanvasNew* root = getRoot();
   if (!root)
      return;

   const GuiStyleProperties style = resolveStyle();

   Resource<GFont> fontRes = mStyle->getResolvedFont(getCurrentStyleStateMask());
   if (!fontRes)
      return;

   const GuiTextAlignHorizontal alignH = style.textAlignHorizontal.isSet()
      ? style.textAlignHorizontal.mValue : GuiTextAlignHorizontal::GuiTextAlignHorizontal_Left;
   const GuiTextAlignVertical alignV = style.textAlignVertical.isSet()
      ? style.textAlignVertical.mValue : GuiTextAlignVertical::GuiTextAlignVertical_Middle;
   const ColorI textColor = style.textColor.isSet() ? style.textColor.mValue : ColorI(255, 255, 255);
   const S32 letterSpacing = style.letterSpacing.isSet() ? style.letterSpacing.mValue : 0;
   const S32 wordSpacing = style.wordSpacing.isSet() ? style.wordSpacing.mValue : 0;

   mJustifiedTextCache.setFont(fontRes);
   mJustifiedTextCache.setText(text);
   mJustifiedTextCache.setBoxExtent(extent);
   mJustifiedTextCache.setAlignHorizontal(alignH);
   mJustifiedTextCache.setAlignVertical(alignV);

   mJustifiedTextCache.submit(root->getRenderBatch(), offset, textColor, letterSpacing, wordSpacing, getRenderLayer());
}

//-----------------------------------------------------------------------------

void GuiControlNew::renderText(GuiText& text, Point2I offset, Point2I extent)
{
   if (!mStyle)
      return;

   GuiCanvasNew* root = getRoot();
   if (!root)
      return;

   const U32 activeStateMask = getCurrentStyleStateMask();
   const GuiStyleProperties style = resolveStyle();

   Resource<GFont> fontRes = mStyle->getResolvedFont(activeStateMask);
   if (!fontRes)
      return;

   text.setFont(fontRes);
   text.setBoxExtent(extent);

   text.setAlignHorizontal(mInlineStyleOverrides.textAlignHorizontal.mValue);
   text.setAlignVertical(mInlineStyleOverrides.textAlignVertical.mValue);

   // Stash state for the delegate below (Delegate<> binds a member function, not a lambda).
   mRenderTextStateMask = activeStateMask;
   GuiTextFontAtSizeDelegate fontAtSizeDelegate;
   fontAtSizeDelegate.bind(this, &GuiControlNew::_resolveFontAtSizeForRenderText);
   text.setFontAtSizeDelegate(fontAtSizeDelegate);

   const ColorI textColor = style.textColor.isSet() ? style.textColor.mValue : ColorI(255, 255, 255);
   const S32 letterSpacing = style.letterSpacing.isSet() ? style.letterSpacing.mValue : 0;
   const S32 wordSpacing = style.wordSpacing.isSet() ? style.wordSpacing.mValue : 0;

   text.submit(root->getRenderBatch(), offset, textColor, letterSpacing, wordSpacing, getRenderLayer());
}

//=============================================================================
//    Events.
//=============================================================================
// MARK: ---- Events ----

//-----------------------------------------------------------------------------

bool GuiControlNew::onAdd()
{
   if (!Parent::onAdd())
      return false;

   const char* cName = getClassName();

   // A pure GuiControlNew is a container by default.
   if (String::compare("GuiControlNew", cName) == 0)
      mIsContainer = true;

   if (mAddGroup == NULL)
      mAddGroup = Sim::getGuiGroup();
   mAddGroup->addObject(this);

   // Assign a style if none is set yet, most to least specific:
   // explicit style="" (already set) -> nearest ancestor's style ->
   // class-name match ("GuiMyCtrl" -> "GuiMyStyle") -> GuiDefaultStyle.
   if (!mStyle)
   {
      GuiControlNew* parent = getParent();
      if (parent && parent->mStyle)
         setStyle(parent->mStyle);
   }

   if (!mStyle)
   {
      String name = getClassName();

      if (name.isNotEmpty())
      {
         U32 pos = name.find("Ctrl");

         if (pos != -1)
            name.replace(pos, 4, "Style");
         else
            name += "Style";

         GuiStyle* style = NULL;
         if (Sim::findObject(name, style))
            setStyle(style);
      }
   }

   if (!mStyle)
   {
      GuiStyle* style = NULL;
      Sim::findObject("GuiDefaultStyle", style);

      AssertISV(style != NULL, avar("GuiControlNew::onAdd() unable to find specified style and GuiDefaultStyle does not exist!"));

      setStyle(style);
   }

   if (!mTooltipStyle)
   {
      GuiStyle* style = NULL;
      Sim::findObject("GuiTooltipStyle", style);

      AssertISV(style != NULL, avar("GuiControlNew::onAdd() unable to find specified tooltip style and GuiTooltipStyle does not exist!"));

      setTooltipStyle(style);
   }

   onAdd_callback();

   GFXStateBlockDesc d;

   d.cullDefined = true;
   d.cullMode = GFXCullNone;
   d.zDefined = true;
   d.zEnable = false;

   mDefaultGuiSB = GFX->createStateBlock(d);

   return true;
}

//-----------------------------------------------------------------------------

void GuiControlNew::onRemove()
{
   // Sleep first, or Parent::onRemove() will trigger onSleep() when removed from our parent.
   if (mAwake)
      sleep();

   onRemove_callback();

   mStyle = NULL;
   mTooltipStyle = NULL;

   clearFirstResponder();

   Parent::onRemove();
}

//-----------------------------------------------------------------------------

void GuiControlNew::onDeleteNotify(SimObject* object)
{
   if (object == mStyle)
   {
      GuiStyle* style;
      Sim::findObject("GuiDefaultStyle", style);

      if (style == mStyle)
         mStyle = NULL;
      else
         setStyle(style);
   }
   if (object == mTooltipStyle)
   {
      GuiStyle* style;
      Sim::findObject("GuiDefaultStyle", style);

      if (style == mTooltipStyle)
         mTooltipStyle = NULL;
      else
         setTooltipStyle(style);
   }
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onWake()
{
   PROFILE_SCOPE(GuiControl_onWake);

   AssertFatal(!mAwake, "GuiControlNew::onWake() - control is already awake");
   if (mAwake)
      return true;

   // [tom, 4/18/2005] Cause mLangTable to be refreshed in case it was changed
   mLangTable = NULL;

   mAwake = true;

   GuiCanvasNew* root = getRoot();
   GuiControlNew* parent = getParent();
   if (parent && parent != root)
      mLayer = parent->mLayer;

   // mRenderLayer is NOT computed here -- awaken() wakes children before
   // this control's own onWake(), so the parent's value isn't final yet.
   // It's established from addObject() instead, top-down.

   if (!mFirstResponder)
      mFirstResponder = findFirstTabable();

   if (mStyle)
      mStyle->incLoadCount();
   if (mTooltipStyle)
      mTooltipStyle->incLoadCount();

   onWake_callback();

   return true;
}

//-----------------------------------------------------------------------------

void GuiControlNew::onSleep()
{
   AssertFatal(mAwake, "GuiControlNew::onSleep() - control is already asleep");
   if (!mAwake)
      return;

   clearFirstResponder();
   mouseUnlock();

   onSleep_callback();

   if (mStyle)
      mStyle->decLoadCount();
   if (mTooltipStyle)
      mTooltipStyle->decLoadCount();

   mAwake = false;
}

//-----------------------------------------------------------------------------

void GuiControlNew::onChildAdded(GuiControlNew* child)
{
   // No-op in the base class.
}

//-----------------------------------------------------------------------------

void GuiControlNew::onChildRemoved(GuiControlNew* child)
{
   // No-op in the base class.
}

//-----------------------------------------------------------------------------

void GuiControlNew::onGroupRemove()
{
   // Clear any first responder in our hierarchy.
   if (mFirstResponder)
      mFirstResponder->clearFirstResponder();
   else
   {
      GuiCanvasNew* root = getRoot();
      if (root)
      {
         GuiControlNew* firstResponder = root->getFirstResponder();
         if (firstResponder && firstResponder->isChildOfGroup(this))
            firstResponder->clearFirstResponder();
      }
   }

   if (isAwake())
      sleep();
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onInputEvent(const InputEventInfo& event)
{
   return(false);
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onKeyDown(const GuiEvent& event)
{
   // Pass unhandled key events up to the parent.
   GuiControlNew* parent = getParent();
   if (parent)
      return parent->onKeyDown(event);
   else
      return false;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onKeyRepeat(const GuiEvent& event)
{
   return onKeyDown(event);
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onKeyUp(const GuiEvent& event)
{
   GuiControlNew* parent = getParent();
   if (parent)
      return parent->onKeyUp(event);
   else
      return false;
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMouseUp(const GuiEvent& event)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMouseDown(const GuiEvent& event)
{
   if (!mVisible || !mAwake)
      return;

   execConsoleCallback();
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMouseMove(const GuiEvent& event)
{
   if (!mVisible || !mAwake)
      return;

   GuiControlNew* parent = getParent();
   if (parent)
      parent->onMouseMove(event);
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMouseDragged(const GuiEvent& event)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMouseEnter(const GuiEvent& event)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMouseLeave(const GuiEvent& event)
{
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onMouseWheelUp(const GuiEvent& event)
{
   if (!mVisible || !mAwake)
      return true;

   GuiControlNew* parent = getParent();
   if (parent)
      return parent->onMouseWheelUp(event);
   else
      return false;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onMouseWheelDown(const GuiEvent& event)
{
   if (!mVisible || !mAwake)
      return true;

   GuiControlNew* parent = getParent();
   if (parent)
      return parent->onMouseWheelDown(event);
   else
      return false;
}

//-----------------------------------------------------------------------------

void GuiControlNew::onRightMouseDown(const GuiEvent&)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onRightMouseUp(const GuiEvent&)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onRightMouseDragged(const GuiEvent&)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMiddleMouseDown(const GuiEvent&)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMiddleMouseUp(const GuiEvent&)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMiddleMouseDragged(const GuiEvent&)
{
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onGamepadButtonDown(const GuiEvent& event)
{
   return onKeyDown(event);
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onGamepadButtonUp(const GuiEvent& event)
{
   return onKeyUp(event);
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onGamepadAxisUp(const GuiEvent& event)
{
   GuiControlNew* parent = getParent();
   if (parent)
   {
      return parent->onGamepadAxisUp(event);
   }
   else
   {
      return false;
   }
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onGamepadAxisDown(const GuiEvent& event)
{
   GuiControlNew* parent = getParent();
   if (parent)
   {
      return parent->onGamepadAxisDown(event);
   }
   else
   {
      return false;
   }
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onGamepadAxisLeft(const GuiEvent& event)
{
   GuiControlNew* parent = getParent();
   if (parent)
   {
      return parent->onGamepadAxisLeft(event);
   }
   else
   {
      return false;
   }
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onGamepadAxisRight(const GuiEvent& event)
{
   GuiControlNew* parent = getParent();
   if (parent)
   {
      return parent->onGamepadAxisRight(event);
   }
   else
   {
      return false;
   }
}

//-----------------------------------------------------------------------------

bool GuiControlNew::onGamepadTrigger(const GuiEvent& event)
{
   GuiControlNew* parent = getParent();
   if (parent)
   {
      return parent->onGamepadTrigger(event);
   }
   else
   {
      return false;
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::onAction()
{
   if (!mActive)
      return;

   if (mConsoleCommand.isNotEmpty())
   {
      execConsoleCallback();
   }
   else
      onAction_callback();
}

//-----------------------------------------------------------------------------

void GuiControlNew::onMessage(GuiControlNew*, S32)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::onDialogPush()
{
   onDialogPush_callback();
}

//-----------------------------------------------------------------------------

void GuiControlNew::onDialogPop()
{
   onDialogPop_callback();
}

//-----------------------------------------------------------------------------

void GuiControlNew::inspectPreApply()
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::inspectPostApply()
{
   // Fake a sleep/wake cycle so property changes take effect without
   // every control needing its own inspectPostApply hook. Skip the canvas.
   if (mAwake && !dynamic_cast<GuiCanvasNew*>(this))
   {
      bool isContainer = mIsContainer;

      onSleep();
      onWake();

      mIsContainer = isContainer;
   }
}

//=============================================================================
//    Layout.
//=============================================================================
// MARK: ---- Layout ----

//-----------------------------------------------------------------------------

Point2I GuiControlNew::getMinExtent() const
{
   // mMinWidth/mMinHeight are the single authored source for a
   // structural floor now (see this method's own doc comment,
   // guiControlNew.h) -- resolve them the same way any other
   // percent-capable dimension resolves against its parent's CLIENT
   // extent (see _getParentReferenceLength()/getClientExtent()). auto on
   // either axis means "no floor," i.e. 0, same as auto's meaning
   // everywhere else min/max clamps don't have a defined auto behavior
   // (see _resolveAutoDimension()'s own doc comment).
   Point2I result(0, 0);

   const GuiControlNew* parent = getParent();
   const Point2I parentExtent = parent ? parent->getClientExtent() : Point2I(0, 0);

   if (!mMinWidth.isAuto())
      result.x = (S32)mMinWidth.resolve((F32)parentExtent.x);
   if (!mMinHeight.isAuto())
      result.y = (S32)mMinHeight.resolve((F32)parentExtent.y);

   return result;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::resize(const Point2I& newPosition, const Point2I& newExtent)
{
   const Point2I minExtent = getMinExtent();
   Point2I actualNewExtent = Point2I(getMax(minExtent.x, newExtent.x),
      getMax(minExtent.y, newExtent.y));

   // Only child-control resizing needs to run when something actually changed.
   const RectI bounds = getBounds();

   bool extentChanged = (actualNewExtent != bounds.extent);
   bool positionChanged = (newPosition != bounds.point);
   if (!extentChanged && !positionChanged)
      return false;

   if (positionChanged)
      mBounds.point = newPosition;

   if (extentChanged)
   {
      setUpdate();

      mBounds.extent = actualNewExtent;

      // Children no longer resolve off a parentResized() ratio callback --
      // mark every child's layout dirty instead so each recomputes against this parent's new size.
      if (mNotifyChildrenResized)
      {
         iterator i;
         for (i = begin(); i != end(); i++)
         {
            GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
            ctrl->markLayoutDirty();
         }
      }

      GuiControlNew* parent = getParent();
      if (parent)
         parent->childResized(this);
      setUpdate();
   }

   // A repositioning alone doesn't count as sizing -- parents don't need
   // to know when a child moves, since child bounds are parent-relative.
   if (extentChanged)
      return true;

   return false;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setPosition(const Point2I& newPosition)
{
   return resize(newPosition, mBounds.extent);
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setExtent(const Point2I& newExtent)
{
   return resize(mBounds.point, newExtent);
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setBounds(const RectI& newBounds)
{
   return resize(newBounds.point, newBounds.extent);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setLeft(S32 newLeft)
{
   resize(Point2I(newLeft, mBounds.point.y), mBounds.extent);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setTop(S32 newTop)
{
   resize(Point2I(mBounds.point.x, newTop), mBounds.extent);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setWidth(S32 newWidth)
{
   resize(mBounds.point, Point2I(newWidth, mBounds.extent.y));
}

//-----------------------------------------------------------------------------

void GuiControlNew::setHeight(S32 newHeight)
{
   resize(mBounds.point, Point2I(mBounds.extent.x, newHeight));
}

//-----------------------------------------------------------------------------

void GuiControlNew::childResized(GuiControlNew*)
{
}

//-----------------------------------------------------------------------------
// Protected field setters for width/height/minWidth/maxWidth/minHeight/
// maxHeight/left/top/right/bottom.
//-----------------------------------------------------------------------------

bool GuiControlNew::_getParentReferenceLength(bool horizontalAxis, F32& outLength) const
{
   const GuiControlNew* parent = getParent();
   if (!parent)
      return false;

   // getClientExtent() (not getExtent()) -- see that virtual's own doc
   // comment. Ordinary containers return the same value either way;
   // this only differs for a container like GuiScrollCtrlNew that
   // reserves part of its own extent for chrome (a scrollbar gutter) a
   // percent-sized child should be confined inside, not laid out
   // underneath.
   const Point2I parentExtent = parent->getClientExtent();
   outLength = horizontalAxis ? (F32)parentExtent.x : (F32)parentExtent.y;
   return true;
}

//-----------------------------------------------------------------------------

void GuiControlNew::_resolveAndApplyDimension(const GuiDimension& dim, PendingDimensionAxis axis, bool isWidthAxis, bool isPositionAxis)
{
   if (dim.isAuto())
   {
      mPendingPercentAxes &= ~axis;
      _resolveAutoDimension(axis, isWidthAxis, isPositionAxis);
      return;
   }

   F32 referenceLength = 0.0f;
   const bool haveParent = (dim.isPercent())
      ? _getParentReferenceLength(isWidthAxis, referenceLength)
      : true; // pixels never need a parent.

   if (!haveParent)
   {
      mPendingPercentAxes |= axis;
      markLayoutDirty();
      return;
   }

   mPendingPercentAxes &= ~axis;

   if (!isPositionAxis)
   {
      // Sizing field: min/max are clamps against mBounds.extent.
      Point2I newExtent = mBounds.extent;
      const S32 resolved = (S32)dim.resolve(referenceLength);

      switch (axis)
      {
      case PendingWidth:     newExtent.x = resolved; break;
      case PendingHeight:    newExtent.y = resolved; break;
      case PendingMinWidth:  newExtent.x = getMax(newExtent.x, resolved); break;
      case PendingMaxWidth:  newExtent.x = getMin(newExtent.x, resolved); break;
      case PendingMinHeight: newExtent.y = getMax(newExtent.y, resolved); break;
      case PendingMaxHeight: newExtent.y = getMin(newExtent.y, resolved); break;
      default: break;
      }

      resize(mBounds.point, newExtent);
   }
   else
   {
      // Positioning field: right/bottom need this control's own current extent.
      Point2I newPosition = mBounds.point;
      const S32 offset = (S32)dim.resolve(referenceLength);

      switch (axis)
      {
      case PendingLeft:   newPosition.x = offset; break;
      case PendingTop:    newPosition.y = offset; break;
      case PendingRight:  newPosition.x = (S32)referenceLength - offset - mBounds.extent.x; break;
      case PendingBottom: newPosition.y = (S32)referenceLength - offset - mBounds.extent.y; break;
      default: break;
      }

      resize(newPosition, mBounds.extent);
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::_resolveAutoDimension(PendingDimensionAxis axis, bool isWidthAxis, bool isPositionAxis)
{
   // Only width/height/left/top have a defined auto behavior beyond
   // "leave this axis alone" -- see this method's own doc comment
   // (guiControlNew.h) for why minWidth/maxWidth/minHeight/maxHeight/
   // right/bottom are deliberately excluded (an auto CLAMP or an auto
   // right/bottom offset has no equivalent CSS-block-flow meaning the
   // way auto width/height/left/top do).
   if (axis != PendingWidth && axis != PendingHeight && axis != PendingLeft && axis != PendingTop)
      return;

   GuiControlNew* parent = getParent();
   if (!parent)
      return; // no reference frame yet -- _resolvePendingPercentAxes()'s "once this control has a live parent" re-trigger covers this the same way it does for percent values

   // getClientExtent() (not getExtent()) -- see that virtual's own doc
   // comment. "Auto width fills the parent" (below) and the auto-left
   // same-row fit check both need the region actually available to lay
   // children out in, not the parent's raw outer size -- a container
   // like GuiScrollCtrlNew that reserves a scrollbar gutter overrides
   // getClientExtent() to reflect that; an ordinary container returns
   // the same value getExtent() would anyway.
   const Point2I parentExtent = parent->getClientExtent();

   if (!isPositionAxis)
   {
      Point2I newExtent = mBounds.extent;

      if (axis == PendingWidth)
      {
         // Auto width fills the parent's client width
         newExtent.x = parentExtent.x;
      }
      else // PendingHeight
      {
         // Auto height sizes to content if this control has an opinion
         // (see getPreferredContentExtent()'s doc comment)
         Point2I preferredExtent;
         if (getPreferredContentExtent(preferredExtent))
            newExtent.y = preferredExtent.y;
      }

      resize(mBounds.point, newExtent);
   }
   else
   {
      Point2I newPosition = mBounds.point;

      if (axis == PendingLeft)
      {
         // Auto left tries to sit immediately to the right of the
         // previous sibling, on the SAME row
         GuiControlNew* prevSibling = _getPreviousSibling();
         S32 candidateLeft = 0;

         if (prevSibling)
         {
            const Point2I prevPos = prevSibling->getPosition();
            const Point2I prevExtent = prevSibling->getExtent();

            if (prevPos.y == mBounds.point.y)
            {
               const S32 rightOfPrev = prevPos.x + prevExtent.x;
               if (rightOfPrev + mBounds.extent.x <= parentExtent.x)
                  candidateLeft = rightOfPrev;
            }
         }

         newPosition.x = candidateLeft;
      }
      else // PendingTop
      {
         // Auto top flows directly below the previous sibling's current
         // bottom edge (0 if this is the first child, or there's no
         // previous sibling)
         GuiControlNew* prevSibling = _getPreviousSibling();
         newPosition.y = prevSibling
            ? (prevSibling->getPosition().y + prevSibling->getExtent().y)
            : 0;
      }

      resize(newPosition, mBounds.extent);
   }
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiControlNew::_getPreviousSibling() const
{
   GuiControlNew* parent = getParent();
   if (!parent)
      return NULL;

   GuiControlNew* previous = NULL;
   iterator i;
   for (i = parent->begin(); i != parent->end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl == this)
         return previous;
      previous = ctrl;
   }

   return NULL;
}

//-----------------------------------------------------------------------------

void GuiControlNew::_resolvePendingPercentAxes()
{
   // Called from addObject() once this control has a live parent; every axis is guarded since this may run more than once.
   if (mPendingPercentAxes & PendingWidth)
      _resolveAndApplyDimension(mWidth, PendingWidth, /*isWidthAxis*/ true, /*isPositionAxis*/ false);
   if (mPendingPercentAxes & PendingHeight)
      _resolveAndApplyDimension(mHeight, PendingHeight, /*isWidthAxis*/ false, /*isPositionAxis*/ false);
   if (mPendingPercentAxes & PendingMinWidth)
      _resolveAndApplyDimension(mMinWidth, PendingMinWidth, /*isWidthAxis*/ true, /*isPositionAxis*/ false);
   if (mPendingPercentAxes & PendingMaxWidth)
      _resolveAndApplyDimension(mMaxWidth, PendingMaxWidth, /*isWidthAxis*/ true, /*isPositionAxis*/ false);
   if (mPendingPercentAxes & PendingMinHeight)
      _resolveAndApplyDimension(mMinHeight, PendingMinHeight, /*isWidthAxis*/ false, /*isPositionAxis*/ false);
   if (mPendingPercentAxes & PendingMaxHeight)
      _resolveAndApplyDimension(mMaxHeight, PendingMaxHeight, /*isWidthAxis*/ false, /*isPositionAxis*/ false);

   // Positioning: centerHorizontal/centerVertical take priority over left/right and top/bottom.
   if (mPendingPercentAxes & PendingCenterHorizontal)
   {
      F32 parentW;
      if (_getParentReferenceLength(true, parentW))
      {
         Point2I newPosition = mBounds.point;
         newPosition.x = ((S32)parentW - mBounds.extent.x) / 2;
         resize(newPosition, mBounds.extent);
      }
      mPendingPercentAxes &= ~PendingCenterHorizontal;
   }
   else if (mPendingPercentAxes & PendingLeft)
      _resolveAndApplyDimension(mLeft, PendingLeft, /*isWidthAxis*/ true, /*isPositionAxis*/ true);
   else if (mPendingPercentAxes & PendingRight)
      _resolveAndApplyDimension(mRight, PendingRight, /*isWidthAxis*/ true, /*isPositionAxis*/ true);

   if (mPendingPercentAxes & PendingCenterVertical)
   {
      F32 parentH;
      if (_getParentReferenceLength(false, parentH))
      {
         Point2I newPosition = mBounds.point;
         newPosition.y = ((S32)parentH - mBounds.extent.y) / 2;
         resize(newPosition, mBounds.extent);
      }
      mPendingPercentAxes &= ~PendingCenterVertical;
   }
   else if (mPendingPercentAxes & PendingTop)
      _resolveAndApplyDimension(mTop, PendingTop, /*isWidthAxis*/ false, /*isPositionAxis*/ true);
   else if (mPendingPercentAxes & PendingBottom)
      _resolveAndApplyDimension(mBottom, PendingBottom, /*isWidthAxis*/ false, /*isPositionAxis*/ true);

   // Once this control's own percent fields resolve, re-trigger any child
   // that addObject() deferred because this control was still pending.
   // Auto width/height/left/top don't need an equivalent re-trigger here
   // -- see resolveLayout()'s doc comment: those four now re-resolve on
   // EVERY resolveLayout() pass (not a one-shot fired from here or from
   // addObject()), so a child that couldn't resolve them yet because
   // this parent wasn't settled will simply pick up real values the next
   // time anything reads that child's position/extent, the same as any
   // other still-dirty control.
   if (mPendingPercentAxes == 0)
   {
      iterator i;
      for (i = begin(); i != end(); i++)
      {
         GuiControlNew* child = static_cast<GuiControlNew*>(*i);
         if (child->mPendingPercentAxes != 0)
            child->_resolvePendingPercentAxes();
      }
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::_recomputeRenderLayer()
{
   if (!mRenderLayerExplicit)
   {
      GuiControlNew* parent = getParent();
      const S32 newLayer = parent ? (parent->mRenderLayer + 1) : 0;

      if (newLayer == mRenderLayer)
         return; // nothing changed -- no need to touch children either.

      mRenderLayer = newLayer;
   }

   // else: explicit override
   for (iterator i = begin(); i != end(); i++)
   {
      GuiControlNew* child = static_cast<GuiControlNew*>(*i);
      child->_recomputeRenderLayer();
   }
}

//-----------------------------------------------------------------------------

S32 GuiControlNew::_findMaxRenderLayerInSubtree(GuiControlNew* ctrl)
{
   if (!ctrl)
      return 0;

   S32 maxLayer = ctrl->getRenderLayer();

   for (iterator i = ctrl->begin(); i != ctrl->end(); ++i)
   {
      GuiControlNew* child = static_cast<GuiControlNew*>(*i);
      const S32 childMax = _findMaxRenderLayerInSubtree(child);
      if (childMax > maxLayer)
         maxLayer = childMax;
   }

   return maxLayer;
}

//-----------------------------------------------------------------------------

void GuiControlNew::bringToFront()
{
   GuiControlNew* parent = getParent();
   if (!parent)
      return;

   // Move to the end of the parent's child list so renderChildControls() paints this last (on top).
   parent->reOrder(this, NULL);

   // Highest render layer anywhere among the OTHER siblings' subtrees --
   // deliberately the true max via _findMaxRenderLayerInSubtree(), not
   // "sibling's own layer + sibling's immediate child count" (the
   // previous approach here). That estimate only accounted for one
   // level of nesting below each sibling; a sibling with real depth
   // below it (a GuiScrollCtrlNew's children, a GuiWindowCtrlNew's
   // content area, several nested containers, etc.) has descendants
   // whose actual mRenderLayer -- which cascades as parent+1 per level
   // of REAL nesting, not per immediate-child count -- could exceed
   // that estimate, silently leaving this control drawing BELOW some
   // deeply-nested sibling content despite calling bringToFront().
   S32 maxSiblingLayer = -1;
   for (iterator i = parent->begin(); i != parent->end(); ++i)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl == this)
         continue;

      const S32 subtreeMax = _findMaxRenderLayerInSubtree(ctrl);
      if (subtreeMax > maxSiblingLayer)
         maxSiblingLayer = subtreeMax;
   }

   // One past the true max -- guaranteed to out-rank every sibling's
   // entire subtree, at any nesting depth, not just its immediate
   // children.
   setRenderLayer(maxSiblingLayer + 1);
}

//-----------------------------------------------------------------------------

void GuiControlNew::sendToBack()
{
   GuiControlNew* parent = getParent();
   if (!parent)
      return;

   // Move to the start of the parent's child list.
   SimObject* currentFront = parent->front();
   if (currentFront != this)
      parent->reOrder(this, currentFront);

   // No reason to keep out-ranking deeper controls elsewhere in the tree.
   clearRenderLayerOverride();
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setRenderLayerProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->setRenderLayer(dAtoi(data));
   return false;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setPreserveAspectRatioProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->setPreserveAspectRatio(dAtob(data));
   return false;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setWidthProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mWidth = GuiDimension::parse(data);
   ctrl->_resolveAndApplyDimension(ctrl->mWidth, PendingWidth, /*isWidthAxis*/ true, /*isPositionAxis*/ false);
   return false;
}

bool GuiControlNew::setHeightProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mHeight = GuiDimension::parse(data);
   ctrl->_resolveAndApplyDimension(ctrl->mHeight, PendingHeight, /*isWidthAxis*/ false, /*isPositionAxis*/ false);
   return false;
}

bool GuiControlNew::setMinWidthProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mMinWidth = GuiDimension::parse(data);
   ctrl->_resolveAndApplyDimension(ctrl->mMinWidth, PendingMinWidth, /*isWidthAxis*/ true, /*isPositionAxis*/ false);
   return false;
}

bool GuiControlNew::setMaxWidthProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mMaxWidth = GuiDimension::parse(data);
   ctrl->_resolveAndApplyDimension(ctrl->mMaxWidth, PendingMaxWidth, /*isWidthAxis*/ true, /*isPositionAxis*/ false);
   return false;
}

bool GuiControlNew::setMinHeightProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mMinHeight = GuiDimension::parse(data);
   ctrl->_resolveAndApplyDimension(ctrl->mMinHeight, PendingMinHeight, /*isWidthAxis*/ false, /*isPositionAxis*/ false);
   return false;
}

bool GuiControlNew::setMaxHeightProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mMaxHeight = GuiDimension::parse(data);
   ctrl->_resolveAndApplyDimension(ctrl->mMaxHeight, PendingMaxHeight, /*isWidthAxis*/ false, /*isPositionAxis*/ false);
   return false;
}

bool GuiControlNew::setLeftProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mLeft = GuiDimension::parse(data);
   if (!ctrl->mCenterHorizontal)
      ctrl->_resolveAndApplyDimension(ctrl->mLeft, PendingLeft, /*isWidthAxis*/ true, /*isPositionAxis*/ true);
   else
      // centerHorizontal is authoritative for this axis
      ctrl->mPendingPercentAxes &= ~PendingLeft;
   return false;
}

bool GuiControlNew::setTopProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mTop = GuiDimension::parse(data);
   if (!ctrl->mCenterVertical)
      ctrl->_resolveAndApplyDimension(ctrl->mTop, PendingTop, /*isWidthAxis*/ false, /*isPositionAxis*/ true);
   else
      ctrl->mPendingPercentAxes &= ~PendingTop;
   return false;
}

bool GuiControlNew::setRightProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mRight = GuiDimension::parse(data);
   // Left wins over right when both are set. Still stored above for get/serialize.
   if (!ctrl->mCenterHorizontal && ctrl->mLeft.isAuto())
      ctrl->_resolveAndApplyDimension(ctrl->mRight, PendingRight, /*isWidthAxis*/ true, /*isPositionAxis*/ true);
   else
      ctrl->mPendingPercentAxes &= ~PendingRight;
   return false;
}

bool GuiControlNew::setBottomProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mBottom = GuiDimension::parse(data);
   // Top wins over bottom when both are set -- same as left/right.
   if (!ctrl->mCenterVertical && ctrl->mTop.isAuto())
      ctrl->_resolveAndApplyDimension(ctrl->mBottom, PendingBottom, /*isWidthAxis*/ false, /*isPositionAxis*/ true);
   else
      ctrl->mPendingPercentAxes &= ~PendingBottom;
   return false;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setCenterHorizontalProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mCenterHorizontal = dAtob(data);

   if (ctrl->mCenterHorizontal)
   {
      F32 parentW;
      if (ctrl->_getParentReferenceLength(true, parentW))
      {
         Point2I newPosition = ctrl->mBounds.point;
         newPosition.x = ((S32)parentW - ctrl->mBounds.extent.x) / 2;
         ctrl->resize(newPosition, ctrl->mBounds.extent);
      }
      else
      {
         ctrl->mPendingPercentAxes |= PendingCenterHorizontal;
      }
   }
   else
   {
      ctrl->mPendingPercentAxes &= ~PendingCenterHorizontal;
   }

   ctrl->markLayoutDirty();
   return false;
}

bool GuiControlNew::setCenterVerticalProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mCenterVertical = dAtob(data);

   if (ctrl->mCenterVertical)
   {
      F32 parentH;
      if (ctrl->_getParentReferenceLength(false, parentH))
      {
         Point2I newPosition = ctrl->mBounds.point;
         newPosition.y = ((S32)parentH - ctrl->mBounds.extent.y) / 2;
         ctrl->resize(newPosition, ctrl->mBounds.extent);
      }
      else
      {
         ctrl->mPendingPercentAxes |= PendingCenterVertical;
      }
   }
   else
   {
      ctrl->mPendingPercentAxes &= ~PendingCenterVertical;
   }

   ctrl->markLayoutDirty();
   return false;
}

void GuiControlNew::resolveLayout()
{
   GuiControlNew* parent = getParent();

   // No parent (canvas, or not yet added to a tree) means no reference frame to clamp/fit against.
   if (!parent)
   {
      mLayoutDirty = false;
      return;
   }

   mLayoutDirty = false;

   // The canvas's content control is exempt from containment: its bounds
   // are owned by GuiCanvasNew::maintainSizing(), and the canvas's raw
   // device-pixel extent is the wrong reference frame to clamp against.
   GuiCanvasNew* parentCanvas = dynamic_cast<GuiCanvasNew*>(parent);
   if (parentCanvas && parentCanvas->getContentControl() == this)
      return;

   // Auto width/height/left/top re-resolve on EVERY pass here (not just
   // once, at add-time) -- see _resolveAutoDimension()'s doc comment for
   // why this now lives in resolveLayout() rather than being a one-shot
   // fired only from addObject()/_resolvePendingPercentAxes(): a parent
   // resize (window rescale, etc.) marks every child mLayoutDirty (see
   // resize()'s mNotifyChildrenResized cascade), and re-running fill-
   // width/content-height/flow-position here is what makes those axes
   // actually track that resize instead of freezing at whatever they
   // resolved to the first time. This intentionally applies uniformly to
   // every control, including a GuiScrollCtrlNew's content children --
   // GuiScrollCtrlNew::_updateScrollRanges() re-derives its own
   // mChildAnchors from each child's freshly-reflowed position on every
   // call (temporarily zeroing scroll offset while it does), rather than
   // this method trying to special-case scroll children itself; see that
   // function's own doc comment for why a universal rule here plus an
   // explicit re-derive there was chosen over carving out an exception
   // in this shared, non-scroll-specific code path.
   //
   // mLeft/mTop flow only when genuinely unused for positioning --
   // mRight/mBottom (this axis's other positioning field) still wins
   // over flow if IT'S the one actually set, same precedence
   // setRightProt()/setBottomProt() already give right/bottom over an
   // auto left/top for the one-shot percent-pending case; flow is only
   // the fallback when NEITHER left/right (or top/bottom) says anything,
   // not a hard override of a control that's genuinely being positioned
   // from its opposite edge.
   if (mWidth.isAuto())
      _resolveAutoDimension(PendingWidth, /*isWidthAxis*/ true, /*isPositionAxis*/ false);
   if (mHeight.isAuto())
      _resolveAutoDimension(PendingHeight, /*isWidthAxis*/ false, /*isPositionAxis*/ false);
   // Top before left -- left's "sit beside the previous sibling" check
   // (see _resolveAutoDimension()'s PendingLeft branch) needs to compare
   // against THIS control's own already-resolved top, and top's own
   // flow rule (stack below the previous sibling's bottom) has no
   // dependency on left at all, so resolving top first is always safe.
   if (mTop.isAuto() && mBottom.isAuto() && !mCenterVertical)
      _resolveAutoDimension(PendingTop, /*isWidthAxis*/ false, /*isPositionAxis*/ true);
   if (mLeft.isAuto() && mRight.isAuto() && !mCenterHorizontal)
      _resolveAutoDimension(PendingLeft, /*isWidthAxis*/ true, /*isPositionAxis*/ true);

   const Point2I parentExtent = parent->getExtent();
   const F32 parentW = (F32)parentExtent.x;
   const F32 parentH = (F32)parentExtent.y;

   // Re-derived fresh from mWidth/mHeight (the permanent authored
   // fields) via _getAuthoredExtent()
   Point2I newExtent = _getAuthoredExtent(parentW, parentH);
   Point2I newPosition = mBounds.point;

   // Snapshot, not a reference
   const Point2I authoredExtent = newExtent;
   const F32 authoredRatio = (mPreserveAspectRatio && authoredExtent.y > 0)
      ? (F32)authoredExtent.x / (F32)authoredExtent.y
      : 0.0f;

   // Structural floor
   const Point2I minExtent = getMinExtent();
   newExtent.x = getMax(newExtent.x, minExtent.x);
   newExtent.y = getMax(newExtent.y, minExtent.y);

   if (authoredRatio > 0.0f && !mAllowOverflow)
   {
      // Explicitly re-seat from authoredExtent (the pre-min-extent-floor
      // snapshot)
      newExtent = authoredExtent;
      if (newExtent.x > (S32)parentW)
      {
         newExtent.x = (S32)parentW;
         newExtent.y = (S32)(newExtent.x / authoredRatio);
      }
      if (newExtent.y > (S32)parentH)
      {
         newExtent.y = (S32)parentH;
         newExtent.x = (S32)(newExtent.y * authoredRatio);
      }
   }

   if (!mAllowOverflow)
   {
      newExtent.x = getMin(newExtent.x, (S32)parentW);
      newExtent.y = getMin(newExtent.y, (S32)parentH);
   }

   // Re-derived fresh from mLeft/mRight/mTop/mBottom (the permanent
   // authored fields)
   if (!mCenterHorizontal)
   {
      if (!mLeft.isAuto())
         newPosition.x = (S32)mLeft.resolve(parentW);
      else if (!mRight.isAuto())
         newPosition.x = (S32)parentW - (S32)mRight.resolve(parentW) - newExtent.x;
      // else: auto/auto flow case already wrote mBounds.point above,
      // and newPosition was seeded from that -- nothing to re-derive.
   }

   if (!mCenterVertical)
   {
      if (!mTop.isAuto())
         newPosition.y = (S32)mTop.resolve(parentH);
      else if (!mBottom.isAuto())
         newPosition.y = (S32)parentH - (S32)mBottom.resolve(parentH) - newExtent.y;
   }

   if (!mAllowOverflow)
   {
      // Clamp position to stay in bounds -- same as the non-aspect-ratio
      // case, so a shrink never yanks the control to a new anchor.
      newPosition.x = mClamp(newPosition.x, 0, (S32)parentW - newExtent.x);
      newPosition.y = mClamp(newPosition.y, 0, (S32)parentH - newExtent.y);
   }

   // Apply via resize() so setUpdate()/childResized()/dirty-marking all still happen.
   resize(newPosition, newExtent);
}

//-----------------------------------------------------------------------------

F32 GuiControlNew::getEffectiveScaleX() const
{
   GuiCanvasNew* canvas = getRoot();
   return canvas ? canvas->getEffectiveScaleX() : 1.0f;
}

//-----------------------------------------------------------------------------

F32 GuiControlNew::getEffectiveScaleY() const
{
   GuiCanvasNew* canvas = getRoot();
   return canvas ? canvas->getEffectiveScaleY() : 1.0f;
}

//-----------------------------------------------------------------------------

RectI GuiControlNew::getDeviceBounds() const
{
   if (mLayoutDirty)
      const_cast<GuiControlNew*>(this)->resolveLayout();

   GuiCanvasNew* canvas = getRoot();

   // The canvas itself has no ancestor to project against -- its own
   // bounds are already device pixels.
   if (dynamic_cast<const GuiCanvasNew*>(this) || !canvas)
      return mBounds;

   const Point2I deviceOrigin = canvas->localToGlobal(localToGlobalCoord(Point2I(0, 0)));

   // A control with mPreserveAspectRatio set has already been resolved to
   // the correct logical ratio by resolveLayout() -- projecting x and y
   // through the canvas's independent per-axis scales (see
   // GuiCanvasNew::mLockAspectRatio) would silently distort it again
   // here, even though mBounds itself is correct. Use one uniform scale
   // for this control instead, same "never overflow either axis" choice
   // mLockAspectRatio's own letterboxing makes at the canvas level.
   F32 scaleX = canvas->getEffectiveScaleX();
   F32 scaleY = canvas->getEffectiveScaleY();
   if (mPreserveAspectRatio)
      scaleX = scaleY = getMin(scaleX, scaleY);

   const Point2I deviceExtent(
      (S32)(mBounds.extent.x * scaleX),
      (S32)(mBounds.extent.y * scaleY));

   return RectI(deviceOrigin, deviceExtent);
}

//-----------------------------------------------------------------------------

Point2I GuiControlNew::localToGlobalCoord(const Point2I& src) const
{
   Point2I ret = src;
   ret += getPosition();
   GuiControlNew* walk = getParent();
   while (walk)
   {
      ret += walk->getPosition();
      walk = walk->getParent();
   }
   return ret;
}

//-----------------------------------------------------------------------------

Point2I GuiControlNew::globalToLocalCoord(const Point2I& src) const
{
   Point2I ret = src;
   ret -= getPosition();
   GuiControlNew* walk = getParent();
   while (walk)
   {
      ret -= walk->getPosition();
      walk = walk->getParent();
   }
   return ret;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::cursorInControl() const
{
   GuiCanvasNew* root = getRoot();
   if (!root) return false;

   Point2I pt = root->getCursorPos();
   pt = root->getPlatformWindow() ? root->getPlatformWindow()->screenToClient(pt) : pt;
   pt = root->deviceToLogicalPoint(pt); // pt is now logical
   Point2I extent = getExtent();
   Point2I offset = localToGlobalCoord(Point2I(0, 0));
   if (pt.x >= offset.x && pt.y >= offset.y &&
      pt.x < offset.x + extent.x && pt.y < offset.y + extent.y)
   {
      return true;
   }
   else
   {
      return false;
   }
}

//-----------------------------------------------------------------------------

bool GuiControlNew::pointInControl(const Point2I& parentCoordPoint) const
{
   const RectI bounds = getBounds();
   S32 xt = parentCoordPoint.x - bounds.point.x;
   S32 yt = parentCoordPoint.y - bounds.point.y;
   return xt >= 0 && yt >= 0 && xt < bounds.extent.x && yt < bounds.extent.y;
}

//=============================================================================
//    Properties.
//=============================================================================
// MARK: ---- Properties ----

//-----------------------------------------------------------------------------

void GuiControlNew::setTooltipStyle(GuiStyle* style)
{
   AssertFatal(style, "GuiControlNew::setTooltipStyle: invalid style");

   if (style == mTooltipStyle)
      return;

   bool skipAwaken = false;

   if (mTooltipStyle == NULL)
      skipAwaken = true;

   if (mAwake && mTooltipStyle)
      mTooltipStyle->decLoadCount();

   if (mTooltipStyle)
      clearNotify(mTooltipStyle);

   mTooltipStyle = style;
   if (mAwake)
      mTooltipStyle->incLoadCount();

   if (mTooltipStyle)
      deleteNotify(mTooltipStyle);

   if (mAwake && !skipAwaken)
   {
      sleep();

      if (!Sim::isShuttingDown())
         awaken();
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::setStyle(GuiStyle* style)
{
   AssertFatal(style, "GuiControlNew::setStyle: invalid style");

   if (style == mStyle)
      return;

   bool skipAwaken = false;

   if (mStyle == NULL)
      skipAwaken = true;

   if (mAwake && mStyle)
      mStyle->decLoadCount();

   if (mStyle)
      clearNotify(mStyle);

   mStyle = style;
   if (mAwake)
      mStyle->incLoadCount();

   if (mStyle)
      deleteNotify(mStyle);

   if (mAwake && !skipAwaken)
   {
      sleep();

      if (!Sim::isShuttingDown())
         awaken();
   }
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setStyleProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   GuiStyle* style = dynamic_cast<GuiStyle*>(Sim::findObject(data));
   if (style == NULL)
      return false;

   ctrl->setStyle(style);
   return false; // tell the console not to also set the data
}

//-----------------------------------------------------------------------------

bool GuiControlNew::setTooltipStyleProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   GuiStyle* style = dynamic_cast<GuiStyle*>(Sim::findObject(data));
   if (style == NULL)
      return false;

   ctrl->setTooltipStyle(style);
   return false; // tell the console not to also set the data
}

//-----------------------------------------------------------------------------
// Inline style override setters
//-----------------------------------------------------------------------------

bool GuiControlNew::setInlineBackgroundColorProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.backgroundColor.set(GuiStyleParseColor(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineBorderColorProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.borderColor.set(GuiStyleParseColor(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineBorderWidthProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.borderWidth.set(dAtoi(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineTextColorProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.textColor.set(GuiStyleParseColor(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineFontFamilyProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.fontFamily.set(StringTable->insert(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineFontSizeProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.fontSize.set(dAtoi(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineLetterSpacingProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.letterSpacing.set(dAtoi(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineWordSpacingProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   ctrl->mInlineStyleOverrides.wordSpacing.set(dAtoi(data));
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineTextAlignHProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);

   // data is the RAW script string ("left"/"center"/"right")
   GuiTextAlignHorizontal res;
   castConsoleTypeFromString(res, data);

   ctrl->mInlineStyleOverrides.textAlignHorizontal.set(res);
   ctrl->setUpdate();
   return false;
}

bool GuiControlNew::setInlineTextAlignVProt(void* object, const char* index, const char* data)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);

   // See setInlineTextAlignHProt()'s doc comment -- same reasoning,
   // vertical axis's own table (TYPEID<GuiTextAlignVertical>()).
   GuiTextAlignVertical res;
   castConsoleTypeFromString(res, data);

   ctrl->mInlineStyleOverrides.textAlignVertical.set(res);
   ctrl->setUpdate();
   return false;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::isHovered() const
{
   // Derived from the canvas's centrally-tracked mouse state rather than a
   // per-control bool, since subclasses often override the mouse virtuals without calling Parent::.
   GuiCanvasNew* root = getRoot();
   if (!root)
      return false;

   return root->getMouseControl() == this;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::isPressed() const
{
   GuiCanvasNew* root = getRoot();
   if (!root)
      return false;

   if (!root->mouseButtonDown())
      return false;

   if (root->getMouseLockedControl() == this)
      return true;

   return !root->getMouseLockedControl() && root->getMouseControl() == this;
}

//-----------------------------------------------------------------------------

U32 GuiControlNew::getCurrentStyleStateMask() const
{
   U32 mask = 0;

   if (isHovered())
      mask |= GuiStyle::bit(GuiStyleState::Hover);
   if (isPressed())
      mask |= GuiStyle::bit(GuiStyleState::Active);
   if (isFirstResponder())
      mask |= GuiStyle::bit(GuiStyleState::Focus);
   if (!mActive)
      mask |= GuiStyle::bit(GuiStyleState::Disabled);
   if (isChecked())
      mask |= GuiStyle::bit(GuiStyleState::Checked);
   if (mHasError)
      mask |= GuiStyle::bit(GuiStyleState::Error);

   return mask;
}

//-----------------------------------------------------------------------------

GuiStyleProperties GuiControlNew::resolveStyle() const
{
   GuiStyleProperties result;

   if (mStyle)
      result = mStyle->resolve(getCurrentStyleStateMask());

   // This control's own inline overrides win over anything mStyle
   // resolved to.
   mInlineStyleOverrides.cascadeOnto(result);

   return result;
}

//-----------------------------------------------------------------------------


const char* GuiControlNew::getScriptValue()
{
   return NULL;
}

//-----------------------------------------------------------------------------

void GuiControlNew::setScriptValue(const char*)
{
}

//-----------------------------------------------------------------------------

void GuiControlNew::setConsoleVariable(const char* variable)
{
   if (variable)
   {
      mConsoleVariable = StringTable->insert(variable);
   }
   else
   {
      mConsoleVariable = StringTable->EmptyString();
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::setConsoleCommand(const String& newCmd)
{
   mConsoleCommand = newCmd;
}

//-----------------------------------------------------------------------------

const char* GuiControlNew::getConsoleCommand() const
{
   return mConsoleCommand;
}

//-----------------------------------------------------------------------------

void GuiControlNew::setVariable(const char* value)
{
   if (mConsoleVariable[0])
      Con::setVariable(mConsoleVariable, value);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setIntVariable(S32 value)
{
   if (mConsoleVariable[0])
      Con::setIntVariable(mConsoleVariable, value);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setFloatVariable(F32 value)
{
   if (mConsoleVariable[0])
      Con::setFloatVariable(mConsoleVariable, value);
}

//-----------------------------------------------------------------------------

const char* GuiControlNew::getVariable() const
{
   if (mConsoleVariable[0])
      return Con::getVariable(mConsoleVariable);
   else return NULL;
}

//-----------------------------------------------------------------------------

S32 GuiControlNew::getIntVariable() const
{
   if (mConsoleVariable[0])
      return Con::getIntVariable(mConsoleVariable);
   else return 0;
}

//-----------------------------------------------------------------------------

F32 GuiControlNew::getFloatVariable() const
{
   if (mConsoleVariable[0])
      return Con::getFloatVariable(mConsoleVariable);
   else return 0.0f;
}

//-----------------------------------------------------------------------------

void GuiControlNew::setVisible(bool value)
{
   mVisible = value;

   setUpdate();

   for (iterator i = begin(); i != end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      ctrl->clearFirstResponder();
   }

   GuiControlNew* parent = getParent();
   if (parent)
   {
      parent->childResized(this);

      // If newly visible while sleeping, and parent is visible/awake, wake up now.
      if (parent->isVisible() && parent->isAwake()
         && this->isVisible() && !this->isAwake())
         awaken();
   }

   if (getNamespace()) // May be called during construction.
      onVisible_callback(value);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setActive(bool value)
{
   if (mActive == value)
      return;

   mActive = value;

   if (!mActive)
      clearFirstResponder();

   if (mVisible && mAwake)
      setUpdate();

   if (getNamespace()) // May be called during construction.
      onActive_callback(value);

   // Pass activation on to children.
   for (iterator iter = begin(); iter != end(); ++iter)
   {
      GuiControlNew* child = dynamic_cast<GuiControlNew*>(*iter);
      if (child)
         child->setActive(value);
   }
}

//=============================================================================
//    Persistence.
//=============================================================================
// MARK: ---- Persistence ----

//-----------------------------------------------------------------------------

bool GuiControlNew::getCanSaveParent() const
{
   const GuiControlNew* walk = this;
   while (walk)
   {
      if (!walk->getCanSave())
         return false;

      walk = walk->getParent();
   }

   return true;
}

//-----------------------------------------------------------------------------

void GuiControlNew::write(Stream& stream, U32 tabStop, U32 flags)
{
   // False if we or any parent is a non-save control.
   bool bCanSave = (flags & IgnoreCanSave) || (flags & NoCheckParentCanSave && getCanSave()) || getCanSaveParent();

   if (bCanSave && mAddGroup)
   {
      StringTableEntry steName = mAddGroup->getInternalName();

      if ((steName != NULL) && (steName != StringTable->insert("null")) && getName())
      {
         MutexHandle handle;
         handle.lock(mMutex);

         // export selected only?
         if ((flags & SelectedOnly) && !isSelected())
         {
            for (U32 i = 0; i < size(); i++)
               (*this)[i]->write(stream, tabStop, flags);

            return;

         }

         stream.writeTabs(tabStop);
         char buffer[1024];
         dSprintf(buffer, sizeof(buffer), "new %s(%s,%s) {\r\n", getClassName(), getName() ? getName() : "", mAddGroup->getInternalName());
         stream.write(dStrlen(buffer), buffer);
         writeFields(stream, tabStop + 1);

         if (size())
         {
            stream.write(2, "\r\n");
            for (U32 i = 0; i < size(); i++)
               (*this)[i]->write(stream, tabStop + 1, flags);
         }

         stream.writeTabs(tabStop);
         stream.write(4, "};\r\n");

         return;
      }
   }

   if (bCanSave)
      Parent::write(stream, tabStop, flags);
}

//=============================================================================
//    Hierarchies.
//=============================================================================
// MARK: ---- Hierarchies ----

//-----------------------------------------------------------------------------

void GuiControlNew::addObject(SimObject* object)
{
   GuiControlNew* ctrl = dynamic_cast<GuiControlNew*>(object);
   if (object->getGroup() == this)
      return;

   AssertFatal(ctrl, "GuiControlNew::addObject() - cannot add non-GuiControlNew as child of GuiControlNew");

   Parent::addObject(object);

   // Only resolve now if this control (the parent) is itself already
   // fully resolved -- otherwise leave ctrl pending; this control's own
   // _resolvePendingPercentAxes() cascade will re-trigger it once settled.
   if (ctrl->mPendingPercentAxes != 0 && mPendingPercentAxes == 0)
      ctrl->_resolvePendingPercentAxes();

   ctrl->_recomputeRenderLayer();

   AssertFatal(!ctrl->isAwake(), "GuiControlNew::addObject: object is already awake before add");
   if (mAwake)
      ctrl->awaken();

   GuiControlNew* parent = ctrl->getParent();
   if (parent)
      parent->onChildAdded(ctrl);
}

//-----------------------------------------------------------------------------

void GuiControlNew::removeObject(SimObject* object)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(object);
   if (mAwake && ctrl->isAwake())
      ctrl->sleep();

   onChildRemoved(ctrl);

   Parent::removeObject(object);
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiControlNew::getParent() const
{
   SimObject* obj = getGroup();
   if (GuiControlNew* gui = dynamic_cast<GuiControlNew*>(obj))
      return gui;
   return 0;
}

//-----------------------------------------------------------------------------

GuiCanvasNew* GuiControlNew::getRoot() const
{
   GuiControlNew* root = NULL;
   GuiControlNew* parent = getParent();
   while (parent)
   {
      root = parent;
      parent = parent->getParent();
   }
   if (root)
      return dynamic_cast<GuiCanvasNew*>(root);
   else
      return NULL;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::acceptsAsChild(SimObject* object) const
{
   return (dynamic_cast<GuiControlNew*>(object) != NULL);
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiControlNew::findHitControl(const Point2I& pt, S32 initialLayer)
{
   iterator i = end(); // find in z order (last to first)

   while (i != begin())
   {
      i--;
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (initialLayer >= 0 && ctrl->mLayer > initialLayer)
      {
         continue;
      }

      else if (ctrl->mVisible && ctrl->mCanHit && ctrl->pointInControl(pt))
      {
         Point2I ptemp = pt - ctrl->getPosition();
         GuiControlNew* hitCtrl = ctrl->findHitControl(ptemp);

         if (hitCtrl->getCapturesInput())
            return hitCtrl;
      }
   }

   if (mCanHit)
      return this;
   return NULL;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::findHitControls(const RectI& rect, Vector< GuiControlNew* >& outResult, U32 flags, S32 initialLayer, U32 depth)
{
   if (!mVisible)
      return false;
   else if (!mCanHit && flags & HIT_NoCanHitNoRecurse)
      return false;

   // A hit counts if not full-box, or the rect fully contains our bounds.
   bool isHit = mVisible;
   if (flags & HIT_FullBoxOnly)
   {
      RectI rectInParentSpace = rect;
      rectInParentSpace.point += getPosition();

      isHit &= rectInParentSpace.contains(getBounds());
   }
   else
      isHit &= mCanHit;

   // A hit that prevents child hits returns immediately.
   if (isHit && flags & HIT_ParentPreventsChildHit && depth > 0)
   {
      outResult.push_back(this);
      return true;
   }

   // Check child controls.
   bool haveFoundChild = false;
   iterator i = end();

   while (i != begin())
   {
      i--;

      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (initialLayer >= 0 && ctrl->mLayer > initialLayer)
         continue;

      if (ctrl->getBounds().overlaps(rect))
      {
         RectI transposedRect = rect;
         transposedRect.point -= ctrl->getPosition();

         if (ctrl->findHitControls(transposedRect, outResult, flags, -1, depth + 1))
            haveFoundChild = true;
      }
   }

   if ((!haveFoundChild || flags & HIT_AddParentHits) && isHit)
   {
      outResult.push_back(this);
      return true;
   }

   return haveFoundChild;
}

//-----------------------------------------------------------------------------

void GuiControlNew::_getChildrenInTabOrder(Vector<GuiControlNew*>& outChildren)
{
   outChildren.clear();

   Vector<GuiControlNew*> indexed;
   Vector<GuiControlNew*> unindexed;

   for (iterator i = begin(); i != end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl->mTabIndex >= 0)
         indexed.push_back(ctrl);
      else
         unindexed.push_back(ctrl);
   }

   // Stable sort so ties (equal tabIndex, which is a valid if unusual
   // authoring choice) keep their original document-order relative
   // position, same as every other tie-breaking rule in this system.
   std::stable_sort(indexed.begin(), indexed.end(),
      [](GuiControlNew* a, GuiControlNew* b) { return a->mTabIndex < b->mTabIndex; });

   outChildren.reserve(indexed.size() + unindexed.size());
   for (U32 i = 0; i < indexed.size(); ++i)
      outChildren.push_back(indexed[i]);
   for (U32 i = 0; i < unindexed.size(); ++i)
      outChildren.push_back(unindexed[i]);
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiControlNew::findFirstTabable()
{
   // No tabbing if the control is disabled or hidden.
   if (!mAwake || !mVisible)
      return NULL;

   Vector<GuiControlNew*> orderedChildren;
   _getChildrenInTabOrder(orderedChildren);

   GuiControlNew* tabCtrl = NULL;
   for (U32 i = 0; i < orderedChildren.size(); ++i)
   {
      tabCtrl = orderedChildren[i]->findFirstTabable();
      if (tabCtrl)
      {
         mFirstResponder = tabCtrl;
         return tabCtrl;
      }
   }

   //nothing was found, therefore, see if this ctrl is tabable
   return (mTabable && mAwake && mVisible) ? this : NULL;
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiControlNew::findLastTabable(bool firstCall)
{
   if (!mAwake || !mVisible)
      return NULL;

   if (firstCall)
      smPrevResponder = NULL;

   if (mTabable)
      smPrevResponder = this;

   Vector<GuiControlNew*> orderedChildren;
   _getChildrenInTabOrder(orderedChildren);

   for (U32 i = 0; i < orderedChildren.size(); ++i)
      orderedChildren[i]->findLastTabable(false);

   // Once the whole tree has been traversed, return the last responder found.
   mFirstResponder = smPrevResponder;
   return smPrevResponder;
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiControlNew::findNextTabable(GuiControlNew* curResponder, bool firstCall)
{
   if (!mAwake || !mVisible)
      return NULL;

   if (firstCall)
      smCurResponder = NULL;

   if (curResponder == this)
      smCurResponder = this;
   else if (smCurResponder && mTabable && mAwake && mVisible && mActive)
      return(this);

   // Find the child (in tab order) that follows the current responder.
   Vector<GuiControlNew*> orderedChildren;
   _getChildrenInTabOrder(orderedChildren);

   GuiControlNew* tabCtrl = NULL;
   for (U32 i = 0; i < orderedChildren.size(); ++i)
   {
      tabCtrl = orderedChildren[i]->findNextTabable(curResponder, false);
      if (tabCtrl) break;
   }
   mFirstResponder = tabCtrl;
   return tabCtrl;
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiControlNew::findPrevTabable(GuiControlNew* curResponder, bool firstCall)
{
   if (!mAwake || !mVisible)
      return NULL;

   if (firstCall)
      smPrevResponder = NULL;

   if (curResponder == this)
      return smPrevResponder;
   else if (mTabable && mAwake && mVisible && mActive)
      smPrevResponder = this; // store in case the next found is the current responder

   Vector<GuiControlNew*> orderedChildren;
   _getChildrenInTabOrder(orderedChildren);

   GuiControlNew* tabCtrl = NULL;
   for (U32 i = 0; i < orderedChildren.size(); ++i)
   {
      tabCtrl = orderedChildren[i]->findPrevTabable(curResponder, false);
      if (tabCtrl) break;
   }
   mFirstResponder = tabCtrl;
   return tabCtrl;
}

//-----------------------------------------------------------------------------

bool GuiControlNew::controlIsChild(GuiControlNew* child)
{
   for (iterator i = begin(); i != end(); ++i)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl == child || ctrl->controlIsChild(child))
         return true;
   }

   return false;
}

//=============================================================================
//    Event Handling.
//=============================================================================
// MARK: ---- Event Handling ----

//-----------------------------------------------------------------------------

bool GuiControlNew::isFirstResponder() const
{
   GuiCanvasNew* root = getRoot();
   return root && root->getFirstResponder() == this;
}

//-----------------------------------------------------------------------------

void GuiControlNew::makeFirstResponder(bool value)
{
   if (value)
      setFirstResponder();
   else
      clearFirstResponder();
}

//-----------------------------------------------------------------------------

void GuiControlNew::setFirstResponder(GuiControlNew* firstResponder)
{
   // Refuse if the control can't have keyboard focus.
   if (firstResponder && !firstResponder->isFocusable())
      return;

   mFirstResponder = firstResponder;

   if (getParent())
      getParent()->setFirstResponder(firstResponder);
}

//-----------------------------------------------------------------------------

void GuiControlNew::setFirstResponder()
{
   if (mAwake && mVisible && isProperlyAdded())
   {
      GuiControlNew* parent = getParent();
      if (mFocusable && parent && parent->isProperlyAdded())
         parent->setFirstResponder(this);
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::clearFirstResponder()
{
   if (!getParent())
      return;

   if (isFirstResponder())
      getParent()->setFirstResponder(NULL);
   else
      for (GuiControlNew* parent = this; parent != NULL; parent = parent->getParent())
         if (parent->mFirstResponder == this)
            parent->mFirstResponder = NULL;
}

//-----------------------------------------------------------------------------

void GuiControlNew::onLoseFirstResponder()
{
   setUpdate(); // Many controls have a visual cue for first-responder status.

   onLoseFirstResponder_callback();
}

//-----------------------------------------------------------------------------

void GuiControlNew::onGainFirstResponder()
{
   this->setUpdate(); // Many controls have a visual cue for first-responder status.

   onGainFirstResponder_callback();
}

//-----------------------------------------------------------------------------

void GuiControlNew::buildAcceleratorMap()
{
   addAcceleratorKey();

   iterator i;
   for (i = begin(); i != end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      ctrl->buildAcceleratorMap();
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::addAcceleratorKey()
{
   if (mAcceleratorKey == StringTable->EmptyString())
      return;

   EventDescriptor accelEvent;
   ActionMap::createEventDescriptor(mAcceleratorKey, &accelEvent);

   GuiCanvasNew* root = getRoot();
   if (root)
      root->addAcceleratorKey(this, 0, accelEvent.eventCode, accelEvent.flags);
}

//-----------------------------------------------------------------------------

void GuiControlNew::acceleratorKeyPress(U32)
{
   onAction();
}

//-----------------------------------------------------------------------------

void GuiControlNew::acceleratorKeyRelease(U32)
{
}

//-----------------------------------------------------------------------------

bool GuiControlNew::isMouseLocked() const
{
   GuiCanvasNew* root = getRoot();
   return root ? root->getMouseLockedControl() == this : false;
}

//-----------------------------------------------------------------------------

void GuiControlNew::mouseLock(GuiControlNew* lockingControl)
{
   GuiCanvasNew* root = getRoot();
   if (root)
      root->mouseLock(lockingControl);
}

//-----------------------------------------------------------------------------

void GuiControlNew::mouseLock()
{
   GuiCanvasNew* root = getRoot();
   if (root)
      root->mouseLock(this);
}

//-----------------------------------------------------------------------------

void GuiControlNew::mouseUnlock()
{
   GuiCanvasNew* root = getRoot();
   if (root)
      root->mouseUnlock(this);
}

//=============================================================================
//    Misc.
//=============================================================================
// MARK: ---- Misc ----

//-----------------------------------------------------------------------------

LangTable* GuiControlNew::getGUILangTable()
{
   if (mLangTable)
      return mLangTable;

   if (mLangTableName && *mLangTableName)
   {
      mLangTable = (LangTable*)getModLangTable((const UTF8*)mLangTableName);
      return mLangTable;
   }

   GuiControlNew* parent = getParent();
   if (parent)
      return parent->getGUILangTable();

   return NULL;
}

//-----------------------------------------------------------------------------

const UTF8* GuiControlNew::getGUIString(S32 id)
{
   LangTable* lt = getGUILangTable();
   if (lt)
      return lt->getString(id);

   return NULL;
}

//-----------------------------------------------------------------------------

void GuiControlNew::messageSiblings(S32 message)
{
   GuiControlNew* parent = getParent();
   if (!parent) return;
   GuiControlNew::iterator i;
   for (i = parent->begin(); i != parent->end(); i++)
   {
      GuiControlNew* ctrl = dynamic_cast<GuiControlNew*>(*i);
      if (ctrl != this)
         ctrl->onMessage(this, message);
   }
}

//-----------------------------------------------------------------------------

void GuiControlNew::getScrollLineSizes(U32* rowHeight, U32* columnWidth)
{
   // Default to 30 pixels each way.
   *columnWidth = 30;
   *rowHeight = 30;
}

//-----------------------------------------------------------------------------

U32 GuiControlNew::clipText(String& text, U32 clipWidth) const
{
   PROFILE_SCOPE(GuiControl_clipText);

   Resource<GFont> fontRes = mStyle ? mStyle->getResolvedFont(getCurrentStyleStateMask()) : Resource<GFont>();
   GFont* font = fontRes;
   if (!font)
      return 0;

   U32 textWidth = font->getStrWidthPrecise(text);

   if (textWidth <= clipWidth)
      return textWidth;

   // Remove characters from the end until text + "..." fits within clipWidth.
   String temp;

   while (text.isNotEmpty())
   {
      text.erase(text.length() - 1, 1);
      temp = text;
      temp += "...";
      textWidth = font->getStrWidthPrecise(temp);

      if (textWidth <= clipWidth)
      {
         text = temp;
         return textWidth;
      }
   }

   // Not even the ellipsis fits; text is now just "...".
   return 0;
}

//-----------------------------------------------------------------------------

void GuiControlNew::getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent)
{
#ifdef _XBOX
   return;
#endif

   TORQUE_UNUSED(lastGuiEvent);

   if (!getRoot())
      return;

   if (getRoot()->mCursorChanged != -1 && !isMouseLocked())
   {
      // Already changed the cursor -- pop it back before changing again.
      PlatformWindow* pWindow = static_cast<GuiCanvasNew*>(getRoot())->getPlatformWindow();
      if (!pWindow)
         return;
      PlatformCursorController* pController = pWindow->getCursorController();
      AssertFatal(pController != NULL, "PlatformWindow without an owned CursorController!");

      pController->popCursor();

      getRoot()->mCursorChanged = -1;
   }
}

//-----------------------------------------------------------------------------

const char* GuiControlNew::evaluate(const char* str)
{
   smThisControl = this;
   StringTableEntry objectName = getName();
   if (objectName != NULL)
      objectName = getIdString();

   StringTableEntry groupName = getGroup() ? getGroup()->getName() : NULL;
   if (groupName != NULL)
      groupName = getGroup()->getIdString();

   String context = String::ToString("%s\nGroup: %s, Object: %s", getFilename(), groupName, objectName);
   const char* result = Con::evaluate(str, false, context.c_str()).value;
   smThisControl = NULL;

   return result;
}

//-----------------------------------------------------------------------------

const char* GuiControlNew::execConsoleCallback()
{
   if (mConsoleCommand.isNotEmpty())
      return evaluate(mConsoleCommand);

   return "";
}

//-----------------------------------------------------------------------------

const char* GuiControlNew::execAltConsoleCallback()
{
   if (mAltConsoleCommand.isNotEmpty())
      return evaluate(mAltConsoleCommand);

   return "";
}

//=============================================================================
//    Console Methods.
//=============================================================================
// MARK: ---- Console Methods ----

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, findHitControl, GuiControlNew*, (S32 x, S32 y), ,
   "Find the topmost child control located at the given coordinates.\n"
   "@note Only children that are both visible and have the 'capturesInput' flag set will be considered in the search."
   "@param x The X coordinate in the control's own coordinate space.\n"
   "@param y The Y coordinate in the control's own coordinate space.\n"
   "@return The topmost child control at the given coordintes or the control on which the method was called if no matching child could be found.\n"
   "@see GuiControlNew::capturesInput\n"
   "@see findHitControls")
{
   return object->findHitControl(Point2I(x, y));
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, findHitControls, const char*, (S32 x, S32 y, S32 width, S32 height), ,
   "Find all visible child controls that intersect with the given rectangle.\n"
   "@note Invisible child controls will not be included in the search.\n"
   "@param x The X coordinate of the rectangle's upper left corner in the control's own coordinate space.\n"
   "@param y The Y coordinate of the rectangle's upper left corner in the control's own coordinate space.\n"
   "@param width The width of the search rectangle in pixels.\n"
   "@param height The height of the search rectangle in pixels.\n"
   "@return A space-separated list of the IDs of all visible control objects intersecting the given rectangle.\n\n"
   "@tsexample\n"
   "// Lock all controls in the rectangle at x=10 and y=10 and the extent width=100 and height=100.\n"
   "foreach$( %ctrl in %this.findHitControls( 10, 10, 100, 100 ) )\n"
   "   %ctrl.setLocked( true );\n"
   "@endtsexample\n"
   "@see findHitControl")
{
   RectI bounds(x, y, width, height);
   Vector< GuiControlNew* > controls;

   if (!object->findHitControls(bounds, controls))
      return "";

   bool isFirst = true;
   StringBuilder str;
   for (U32 i = 0, num = controls.size(); i < num; ++i)
   {
      if (!isFirst)
         str.append(' ');

      str.append(controls[i]->getIdString());
      isFirst = false;
   }
   String s = str.end();

   if (s.compare(object->getIdString()) == 0)
      return "";

   char* buffer = Con::getReturnBuffer(s.size());
   dStrcpy(buffer, s.c_str(), s.size());

   return buffer;
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, controlIsChild, bool, (GuiControlNew* control), ,
   "Test whether the given control is a direct or indirect child to this control.\n"
   "@param control The potential child control.\n"
   "@return True if the given control is a direct or indirect child to this control.")
{
   if (!control)
      return false;

   return object->controlIsChild(control);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, isFirstResponder, bool, (), ,
   "Test whether the control is the current first responder.\n"
   "@return True if the control is the current first responder.\n"
   "@see makeFirstResponder\n"
   "@see setFirstResponder\n"
   "@ref GuiControl_FirstResponders")
{
   return object->isFirstResponder();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, setFirstResponder, void, (), ,
   "Make this control the current first responder.\n"
   "@note Only controls with 'focusable' enabled are able to become first responders.\n"
   "@see GuiControlNew::focusable\n"
   "@see isFirstResponder\n"
   "@ref GuiControl_FirstResponders")
{
   object->setFirstResponder();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getFirstResponder, GuiControlNew*, (), ,
   "Get the first responder set on this GuiControlNew tree.\n"
   "@return The first responder set on the control's subtree.\n"
   "@see isFirstResponder\n"
   "@see makeFirstResponder\n"
   "@see setFirstResponder\n"
   "@ref GuiControl_FirstResponders")
{
   return object->getFirstResponder();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, clearFirstResponder, void, (bool ignored), (false),
   "Clear this control from being the first responder in its hierarchy chain.\n"
   "@param ignored Ignored.  Supported for backwards-compatibility.\n")
{
   object->clearFirstResponder();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, pointInControl, bool, (S32 x, S32 y), ,
   "Test whether the given point lies within the rectangle of the control.\n"
   "@param x X coordinate of the point in parent-relative coordinates.\n"
   "@param y Y coordinate of the point in parent-relative coordinates.\n"
   "@return True if the point is within the control, false if not.\n"
   "@see getExtent\n"
   "@see getPosition\n")
{
   return object->pointInControl(Point2I(x, y));
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, addGuiControl, void, (GuiControlNew* control), ,
   "Add the given control as a child to this control.\n"
   "This is synonymous to calling SimGroup::addObject.\n"
   "@param control The control to add as a child.\n"
   "@note The control will retain its current position and size.\n"
   "@see SimGroup::addObject\n"
   "@ref GuiControl_Hierarchy\n")
{
   if (control)
      object->addObject(control);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getRoot, GuiCanvasNew*, (), ,
   "Get the canvas on which the control is placed.\n"
   "@return The canvas on which the control's hierarchy is currently placed or 0 if the control is not currently placed on a GuiCanvasNew.\n"
   "@see GuiControl_Hierarchy\n")
{
   return object->getRoot();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getParent, GuiControlNew*, (), ,
   "Get the immediate parent control of the control.\n"
   "@return The immediate parent GuiControlNew or 0 if the control is not parented to a GuiControlNew.\n")
{
   return object->getParent();
}

//-----------------------------------------------------------------------------
DefineEngineMethod(GuiControlNew, isMouseLocked, bool, (), ,
   "Indicates if the mouse is locked in this control.\n"
   "@return True if the mouse is currently locked.\n")
{
   return object->isMouseLocked();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, setValue, void, (const char* value), ,
   "Set the value associated with the control.\n"
   "@param value The new value for the control.\n")
{
   object->setScriptValue(value);
}

DefineEngineMethod(GuiControlNew, getValue, const char*, (), ,
   "Get the value associated with the control.\n"
   "@return value for the control.\n")
{
   return object->getScriptValue();
}

DefineEngineMethod(GuiControlNew, makeFirstResponder, void, (bool isFirst), ,
   "Make this control the first responder.\n"
   "@param isFirst True to make first responder, false to not.\n")
{
   object->makeFirstResponder(isFirst);
}

DefineEngineMethod(GuiControlNew, isActive, bool, (), ,
   "Check if this control is active or not.\n"
   "@return True if it's active, false if not.\n")
{
   return object->isActive();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, setActive, void, (bool state), (true),
   "Set the control as active or inactive."
   "@param state True to set the control as active, false to set it as inactive.")
{
   object->setActive(state);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, isVisible, bool, (), ,
   "Test whether the control is currently set to be visible.\n"
   "@return True if the control is currently set to be visible."
   "@note This method does not tell anything about whether the control is actually visible to "
   "the user at the moment.\n\n"
   "@ref GuiControl_VisibleActive")
{
   return object->isVisible();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, setVisible, void, (bool state), (true),
   "Set whether the control is visible or not.\n"
   "@param state The new visiblity flag state for the control.\n"
   "@ref GuiControl_VisibleActive")
{
   object->setVisible(state);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, isAwake, bool, (), ,
   "Test whether the control is currently awake.\n"
   "If a control is awake it means that it is part of the GuiControlNew hierarchy of a GuiCanvasNew.\n"
   "@return True if the control is awake."
   "@ref GuiControl_Waking")
{
   return object->isAwake();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, setStyle, void, (GuiStyle* style), ,
   "Set the style for the control to use.\n"
   "The style used by a control determines a great part of its appearance.\n"
   "@param style The new style the control should use.\n"
   "@ref GuiControl_Styles")
{
   if (!style)
      return;

   object->setStyle(style);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getPosition, Point2I, (), ,
   "Get the control's current RESOLVED position relative to its parent, "
   "in logical units. This is a snapshot of the last layout resolve, not "
   "an authored value -- see setLeft()/setTop() (or the left/top fields "
   "directly) to actually change positioning.\n"
   "@return The coordinate of the control in its parent's coordinate space.")
{
   return object->getPosition();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getCenter, Point2I, (), ,
   "Get the coordinate of the control's RESOLVED center point relative to its parent.\n"
   "@return The coordinate of the control's center point in parent-relative coordinates.")
{
   const Point2I pos = object->getPosition();
   const Point2I ext = object->getExtent();
   Point2I center(pos.x + ext.x / 2, pos.y + ext.y / 2);

   return center;
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getGlobalCenter, Point2I, (), ,
   "Get the coordinate of the control's RESOLVED center point in coordinates relative to the root control in its control hierarchy.\n"
   "@Return the center coordinate of the control in root-relative coordinates.\n")
{
   const Point2I tl(0, 0);
   Point2I pos = object->localToGlobalCoord(tl);
   const Point2I ext = object->getExtent();
   Point2I center(pos.x + ext.x / 2, pos.y + ext.y / 2);

   return center;
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getGlobalPosition, Point2I, (), ,
   "Get the RESOLVED position of the control relative to the root of the GuiControlNew hierarchy it is contained in.\n"
   "@return The control's current position in root-relative coordinates.")
{
   const Point2I pos(0, 0);
   return object->localToGlobalCoord(pos);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getExtent, Point2I, (), ,
   "Get the RESOLVED width and height of the control, in logical units.\n"
   "@return A point structure containing the width of the control in x and the height in y.")
{
   return object->getExtent();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getMinExtent, Point2I, (), ,
   "Get the minimum allowed size of the control, resolved from minWidth/minHeight "
   "against the current parent's client extent.\n"
   "@return The minimum size to which the control can be shrunk.\n"
   "@see minWidth\n"
   "@see minHeight")
{
   return object->getMinExtent();
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, getAspect, F32, (), ,
   "Get the aspect ratio of the control's RESOLVED extents.\n"
   "@return The width of the control divided by its height.\n"
   "@see getExtent")
{
   const Point2I& ext = object->getExtent();
   return (F32)ext.x / (F32)ext.y;
}

//-----------------------------------------------------------------------------
// Layout field setters -- script-side equivalents of assigning left/top/
// right/bottom/width/height directly (%ctrl.left = "50%"; and
// %ctrl.setLeft("50%"); do exactly the same thing)
//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, setLeft, void, (const char* left), ,
   "Set the control's left position: \"auto\", a logical-pixel number (e.g. \"120\" or \"120px\"), "
   "or a percentage of the immediate parent's current width (e.g. \"50%\"). Equivalent to assigning "
   "the left field directly.\n"
   "@param left The new left value.\n"
   "@see left")
{
   object->setDataField(StringTable->insert("left"), NULL, left);
}

DefineEngineMethod(GuiControlNew, setTop, void, (const char* top), ,
   "Set the control's top position: \"auto\", a logical-pixel number, or a percentage of the "
   "immediate parent's current height (e.g. \"50%\"). Equivalent to assigning the top field directly.\n"
   "@param top The new top value.\n"
   "@see top")
{
   object->setDataField(StringTable->insert("top"), NULL, top);
}

DefineEngineMethod(GuiControlNew, setRight, void, (const char* right), ,
   "Set the control's right position (distance from the parent's right edge): \"auto\", a "
   "logical-pixel number, or a percentage of the immediate parent's current width. Loses to left "
   "if both are set -- see this class's own doc comment (guiControlNew.h). Equivalent to assigning "
   "the right field directly.\n"
   "@param right The new right value.\n"
   "@see right")
{
   object->setDataField(StringTable->insert("right"), NULL, right);
}

DefineEngineMethod(GuiControlNew, setBottom, void, (const char* bottom), ,
   "Set the control's bottom position (distance from the parent's bottom edge): \"auto\", a "
   "logical-pixel number, or a percentage of the immediate parent's current height. Loses to top "
   "if both are set. Equivalent to assigning the bottom field directly.\n"
   "@param bottom The new bottom value.\n"
   "@see bottom")
{
   object->setDataField(StringTable->insert("bottom"), NULL, bottom);
}

DefineEngineMethod(GuiControlNew, setWidth, void, (const char* width), ,
   "Set the control's width: \"auto\" (fills the parent's client width), a logical-pixel number, "
   "or a percentage of the immediate parent's current width. Equivalent to assigning the width "
   "field directly.\n"
   "@param width The new width value.\n"
   "@see width")
{
   object->setDataField(StringTable->insert("width"), NULL, width);
}

DefineEngineMethod(GuiControlNew, setHeight, void, (const char* height), ,
   "Set the control's height: \"auto\" (content-sized if the control overrides "
   "getPreferredContentExtent(), otherwise leaves the current height alone), a logical-pixel "
   "number, or a percentage of the immediate parent's current height. Equivalent to assigning the "
   "height field directly.\n"
   "@param height The new height value.\n"
   "@see height")
{
   object->setDataField(StringTable->insert("height"), NULL, height);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiControlNew, execCommand, const char*, (), ,
   "Forcefully executes the command field value(if any) on this guiControlNew.\n"
   "@return The results of the evaluation of the command.")
{
   return object->execConsoleCallback();
}

DefineEngineMethod(GuiControlNew, execAltCommand, const char*, (), ,
   "Forcefully executes the altCommand field value(if any) on this guiControlNew.\n"
   "@return The results of the evaluation of the altCommand.")
{
   return object->execAltConsoleCallback();
}
