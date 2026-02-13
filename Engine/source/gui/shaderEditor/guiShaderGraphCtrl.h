#pragma once
#ifndef _GUISHADERGRAPHCTRL_H_
#define _GUISHADERGRAPHCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#ifndef _UNDO_H_
#include "util/undo.h"
#endif

#ifndef _GFX_GFXDRAWER_H_
#include "gfx/gfxDrawUtil.h"
#endif

class GuiShaderGraphCtrl : public GuiControl
{
   typedef GuiControl Parent;

public:

   GuiShaderGraphCtrl();

   DECLARE_CONOBJECT(GuiShaderGraphCtrl);
   DECLARE_CATEGORY("Shader Editor");
   DECLARE_DESCRIPTION("Implements a shader node graph used for creating node shaders.");

   bool onWake() override;
   void onSleep() override;
   static void initPersistFields();
   bool onAdd() override;
   void onRemove() override;

   void onPreRender() override;
   void onRender(Point2I offset, const RectI& updateRect) override;

   // interaction
   bool onKeyDown(const GuiEvent& event) override;
   void onMouseDown(const GuiEvent& event) override;
   void onMouseUp(const GuiEvent& event) override;
   void onMouseDragged(const GuiEvent& event) override;
   void onMiddleMouseDown(const GuiEvent& event) override;
   void onMiddleMouseUp(const GuiEvent& event) override;
   void onMiddleMouseDragged(const GuiEvent& event) override;
   bool onMouseWheelUp(const GuiEvent& event) override;
   bool onMouseWheelDown(const GuiEvent& event) override;


};

#endif // !_GUISHADERGRAPHCTRL_H_
