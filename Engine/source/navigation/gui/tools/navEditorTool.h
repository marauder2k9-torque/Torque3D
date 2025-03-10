#pragma once
#ifndef _NAVEDITORTOOL_H_
#define _NAVEDITORTOOL_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif

#ifndef _GUITYPES_H_
#include "gui/core/guiTypes.h"
#endif

#ifndef _TORQUE_RECAST_H_
#include "navigation/torqueRecast.h"
#endif

#ifndef _NAVMESH_H_
#include "navigation/navMesh.h"
#endif

#ifndef _GUINAVEDITORCTRL_H_
#include "navigation/gui/guiNavEditorCtrl.h"
#endif

/// <summary>
/// A base tool for all tools that will interact with the navigation editor.
/// </summary>
class NavEditorTool : public SimObject
{
   typedef SimObject Parent;
protected:
   void _submitUndo(UndoAction* action);
   SimObjectPtr<NavEditorTool> mNavMesh;
public:
   NavEditorTool();
   virtual ~NavEditorTool();

   DECLARE_CONOBJECT(NavEditorTool);

   virtual int type() = 0;
   virtual void init(class NavMesh* navMesh) = 0;

   virtual void onActivated(const Gui3DMouseEvent& lastEvent) {}
   virtual void onDeactivated() {}

   virtual void on3DMouseDown(const Gui3DMouseEvent& evt) {}
   virtual void on3DMouseUp(const Gui3DMouseEvent& evt) {}
   virtual void on3DMouseMove(const Gui3DMouseEvent& evt) {}
   virtual void on3DMouseDragged(const Gui3DMouseEvent& evt) {}
   virtual void on3DMouseEnter(const Gui3DMouseEvent& evt) {}
   virtual void on3DMouseLeave(const Gui3DMouseEvent& evt) {}
   virtual bool onMouseWheel(const GuiEvent& evt) { return false; }
   virtual void onRender3D() {}
   virtual void onRender2D() {}
   virtual void updateGizmo() {}
   virtual bool updateGuiInfo() { return false; }
   virtual void onUndoAction() {}
};

#endif // !_NAVEDITORTOOL_H_
