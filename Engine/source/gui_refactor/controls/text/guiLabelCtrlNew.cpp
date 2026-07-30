//-----------------------------------------------------------------------------
// guiLabelCtrlNew.cpp
//-----------------------------------------------------------------------------

#include "gui_refactor/controls/text/guiLabelCtrlNew.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"

IMPLEMENT_CONOBJECT(GuiLabelCtrlNew);

//-----------------------------------------------------------------------------

GuiLabelCtrlNew::GuiLabelCtrlNew()
   : mWrap(false),
   mDynamicFontSize(false)
{
   setCanHit(false); // static labels don't intercept mouse events by default
}

//-----------------------------------------------------------------------------

void GuiLabelCtrlNew::initPersistFields()
{
   docsURL;
   addGroup("Label");

   addProtectedField("text", TypeRealString, Offset(mText, GuiLabelCtrlNew), &setTextProt, &defaultProtectedGetFn,
      "The text this label displays.");

   addProtectedField("wrap", TypeBool, Offset(mWrap, GuiLabelCtrlNew), &setWrapProt, &defaultProtectedGetFn,
      "If true, wraps across multiple lines within this control's width.");

   addProtectedField("dynamicFontSize", TypeBool, Offset(mDynamicFontSize, GuiLabelCtrlNew), &setDynamicFontSizeProt, &defaultProtectedGetFn,
      "If true, shrinks the font size (never grows it) until the text fits this control's box.");

   addField("overflow", TYPEID< GuiTextOverflowConsole >(), Offset(mGuiText.mOverflow, GuiLabelCtrlNew),
      "How to handle text wider than the box: \"overflow\" (default), \"clip\", or \"ellipsis\".");

   endGroup("Label");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool GuiLabelCtrlNew::onWake()
{
   if (!Parent::onWake())
      return false;

   mGuiText.setText(mText);
   mGuiText.setWrap(mWrap);
   mGuiText.setFitMode(mDynamicFontSize ? GuiTextFitMode::ShrinkToFit : GuiTextFitMode::Fixed);

   return true;
}

//-----------------------------------------------------------------------------

void GuiLabelCtrlNew::setText(const String& text)
{
   if (String::compare(mText.c_str(), text.c_str()) == 0)
      return;

   mText = text;
   mGuiText.setText(mText);
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiLabelCtrlNew::setWrap(bool wrap)
{
   if (mWrap == wrap)
      return;

   mWrap = wrap;
   mGuiText.setWrap(mWrap);
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiLabelCtrlNew::setDynamicFontSize(bool dynamicFontSize)
{
   if (mDynamicFontSize == dynamicFontSize)
      return;

   mDynamicFontSize = dynamicFontSize;
   mGuiText.setFitMode(mDynamicFontSize ? GuiTextFitMode::ShrinkToFit : GuiTextFitMode::Fixed);
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiLabelCtrlNew::onRender(Point2I offset, const RectI& updateRect)
{
   // Background/border fill, same as GuiControlNew's own default onRender().
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

   // Alignment from the resolved style, then hand off to renderText().
   mGuiText.setAlignHorizontal(style.textAlignHorizontal.isSet() ? style.textAlignHorizontal.mValue : GuiTextAlignHorizontal::GuiTextAlignHorizontal_Left);
   mGuiText.setAlignVertical(style.textAlignVertical.isSet() ? style.textAlignVertical.mValue : GuiTextAlignVertical::GuiTextAlignVertical_Middle);

   renderText(mGuiText, offset, ctrlRect.extent);

   renderChildControls(offset, updateRect);
}

//-----------------------------------------------------------------------------

bool GuiLabelCtrlNew::getPreferredContentExtent(Point2I& outExtent) const
{
   // No style/font resolved yet (e.g. measured before onWake() -- see
   // GuiControlNew::addObject()'s auto-resolve pass, which can run
   // before this control has woken if its parent isn't awake yet
   // either) means no real measurement is possible; fall back to "no
   // opinion" rather than guessing, same as the base class's own
   // default. Not const_cast + calling into mStyle from here -- a local
   // stack GuiText (see guiText.h's own "can also be used transiently
   // for a one-off measurement" doc comment) is used instead of mGuiText
   // specifically so this stays side-effect-free against this control's
   // OWN persistent GuiText/render state.
   if (!mStyle)
      return false;

   const U32 activeStateMask = getCurrentStyleStateMask();
   Resource<GFont> fontRes = mStyle->getResolvedFont(activeStateMask);
   if (!fontRes)
      return false;

   GuiCanvasNew* root = getRoot();
   const F32 scaleX = root ? getEffectiveScaleX() : 1.0f;
   const F32 scaleY = root ? getEffectiveScaleY() : 1.0f;

   // Measure at this control's CURRENT logical width, converted to
   // device pixels the same way renderText() converts ctrlRect.extent --
   // see this method's own header doc comment (guiLabelCtrlNew.h) for
   // why width is guaranteed to already be resolved by the time this
   // runs. Height itself is irrelevant to the measurement (that's the
   // whole point -- it's what we're computing), so the box is set with
   // an arbitrary large height; only mWrap's effect on LINE WIDTH
   // depends on box width, wrapping never depends on box height.
   const S32 deviceWidth = (S32)((F32)getRawBounds().extent.x * scaleX);

   GuiText measureText;
   measureText.setFont(fontRes);
   measureText.setText(mText);
   measureText.setWrap(mWrap);
   measureText.setBoxExtent(Point2I(deviceWidth, 0x7fffffff));

   const GuiTextLayoutResult& result = measureText.layout();

   // blockBounds is in device pixels (see guiText.h's file header) --
   // convert back to logical units, matching how mBounds/getExtent()
   // work everywhere else in GuiControlNew.
   outExtent = getRawBounds().extent;
   outExtent.y = (S32)((F32)result.blockBounds.extent.y / getMax(scaleY, 0.0001f));
   return true;
}

//-----------------------------------------------------------------------------

bool GuiLabelCtrlNew::setTextProt(void* object, const char* index, const char* data)
{
   static_cast<GuiLabelCtrlNew*>(object)->setText(data);
   return false;
}

//-----------------------------------------------------------------------------

bool GuiLabelCtrlNew::setWrapProt(void* object, const char* index, const char* data)
{
   static_cast<GuiLabelCtrlNew*>(object)->setWrap(dAtob(data));
   return false;
}

//-----------------------------------------------------------------------------

bool GuiLabelCtrlNew::setDynamicFontSizeProt(void* object, const char* index, const char* data)
{
   static_cast<GuiLabelCtrlNew*>(object)->setDynamicFontSize(dAtob(data));
   return false;
}
