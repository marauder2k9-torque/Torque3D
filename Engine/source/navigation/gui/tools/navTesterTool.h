#pragma once
#ifndef _NAVTESTERTOOL_H_
#define _NAVTESTERTOOL_H_

#ifndef _NAVEDITORTOOL_H_
#include "navigation/gui/tools/navEditorTool.h"
#endif

class NavMeshTesterTool : public NavEditorTool
{
   typedef NavEditorTool Parent;

public:

   enum TesterMode
   {
      TOOLMODE_PATHFIND_FOLLOW,
      TOOLMODE_PATHFIND_STRAIGHT,
      TOOLMODE_PATHFIND_SLICED,
      TOOLMODE_RAYCAST,
      TOOLMODE_DISTANCE_TO_WALL,
      TOOLMODE_FIND_POLYS_IN_CIRCLE,
      TOOLMODE_FIND_POLYS_IN_SHAPE,
      TOOLMODE_FIND_LOCAL_NEIGHBOURHOOD
   };

protected:
   dtNavMesh* mNavMesh;
   dtNavMeshQuery* mNavQuery;
   dtQueryFilter     mQueryFilter;
   dtStatus          mPathFindStatus;
   TesterMode mToolMode;
   S32 mStraightOptions;
   static const S32 MAX_POLYS = 256;
   static const S32 MAX_SMOOTH = 2048;
   static const S32 MAX_RAND_POINTS = 64;

   String mSpawnClass;
   String mSpawnDatablock;
   SimObjectPtr<AIPlayer> mPlayer;
   SimObjectPtr<AIPlayer> mCurPlayer;

public:
   NavMeshTesterTool();
   virtual ~NavMeshTesterTool();

   // SimObject
   DECLARE_CONOBJECT(NavMeshTesterTool);
   static void initPersistFields();
   bool onAdd() override;
   void onRemove() override;
   void inspectPostApply() override;

   // NavEditorTool
   S32 type() override { return NavigationTool::NavmeshTesterTool; }
   void setActiveNavMesh(NavMesh* navMesh) override;

   void onActivated(const Gui3DMouseEvent& lastEvent) override;
   void onDeactivated() override;
   void spawnPlayer(const Point3F& pos);
   void on3DMouseDown(const Gui3DMouseEvent& evt) override;
   void on3DMouseUp(const Gui3DMouseEvent& evt) override;
   void onRender3D() override;
   void onRender2D() override;
   bool updateGuiInfo() override;

   // NavTesterTool 
   void recalc();
   void drawAgent(const Point3F& pos);
};

typedef NavMeshTesterTool::TesterMode navmeshtest_mode;
DefineEnumType(navmeshtest_mode);

#endif // !_NAVTESTERTOOL_H_
