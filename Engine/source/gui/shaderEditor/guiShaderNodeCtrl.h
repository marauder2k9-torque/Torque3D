#pragma once
#ifndef _GUISHADERNODECTRL_H_
#define _GUISHADERNODECTRL_H_

#ifndef _SHADERGENNODES_H_
#include "shaderGen/NODE/shaderGenNodes.h"
#endif // !_SHADERGENNODES_H_

#ifndef _GUISHADERGRAPHCTRL_H_
#include "gui/shaderEditor/guiShaderGraphCtrl.h"
#endif

// forward decl.
class GuiShaderNodeCtrl;

class GuiShaderPortCtrl : public GuiControl
{
   typedef GuiControl Parent;

public:
   enum PortDir
   {
      Input,
      Output
   };

protected:
   PortDir mDirection;
   bool    mAllowMultiple;
   GFXShaderConstType mDataType;

public:
   DECLARE_CONOBJECT(GuiShaderPortCtrl);

   PortDir getDirection() const { return mDirection; }
   bool isInput() const { return mDirection == Input; }
   bool isOutput() const { return mDirection == Output; }

   const GFXShaderConstType& getDataType() const { return mDataType; }
   bool allowsMultiple() const { return mAllowMultiple; }

   GuiShaderNodeCtrl* getNode()
   {
      return static_cast<GuiShaderNodeCtrl*>(getParent());
   }

   void onMouseDown(const GuiEvent& event) override;
};

class GuiShaderNodeCtrl : public GuiControl
{
   typedef GuiControl Parent;

protected:
   ShaderNodeFeature_enum mNodeType;

public:
   DECLARE_CONOBJECT(GuiShaderNodeCtrl);
};


#endif // !_GUISHADERNODECTRL_H_
