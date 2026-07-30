//-----------------------------------------------------------------------------
// guiLabelCtrlNew.h
//
// GuiLabelCtrlNew -- a static, non-interactive text label, and the minimal
// worked example of wiring a GuiText into a control (own one persistent
// GuiText, reconfigure it as fields change, draw via
// GuiControlNew::renderText()).
//-----------------------------------------------------------------------------

#ifndef _GUILABELCTRLNEW_H_
#define _GUILABELCTRLNEW_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _GUITEXT_H_
#include "gui_refactor/core/guiText.h"
#endif

class GuiLabelCtrlNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

protected:

   String mText;
   bool mWrap;
   bool mDynamicFontSize;

   GuiText mGuiText; ///< Persistent instance this control owns and reconfigures.

   static bool setTextProt(void* object, const char* index, const char* data);
   static bool setWrapProt(void* object, const char* index, const char* data);
   static bool setDynamicFontSizeProt(void* object, const char* index, const char* data);

public:

   GuiLabelCtrlNew();

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiLabelCtrlNew);
   DECLARE_CATEGORY("Gui Core");
   DECLARE_DESCRIPTION("A static, non-interactive text label drawn via GuiText/GuiControlNew::renderText().");

   bool onWake() override;

   void setText(const String& text);
   const String& getText() const { return mText; }

   void setWrap(bool wrap);
   void setDynamicFontSize(bool dynamicFontSize);

   void setOverflow(GuiTextOverflowConsole overflow);
   GuiTextOverflowConsole getOverflow() const { return mGuiText.mOverflow; }

   void onRender(Point2I offset, const RectI& updateRect) override;
   bool getPreferredContentExtent(Point2I& outExtent) const override;
};

#endif // _GUILABELCTRLNEW_H_
