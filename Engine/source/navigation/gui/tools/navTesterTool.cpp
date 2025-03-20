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

#include "navigation/gui/tools/navTesterTool.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"

#include "math/mMath.h"
#include "math/mathUtils.h"


IMPLEMENT_CONOBJECT(NavMeshTesterTool);

ConsoleDocClass(NavMeshTesterTool,
   "@brief Defines the tool properties when testing navmeshes in the Navigation Editor\n\n"
   "Editor use only.\n\n"
   "@internal"
);

ImplementEnumType(navmeshtest_mode, "NavMesh tester tool mode\n\n")
{ navmeshtest_mode::TOOLMODE_PATHFIND_FOLLOW,            "Pathfind Follow", "..."  },
{ navmeshtest_mode::TOOLMODE_PATHFIND_STRAIGHT,          "Pathfind Straight", "..."  },
{ navmeshtest_mode::TOOLMODE_PATHFIND_SLICED,            "Pathfind Sliced", "..."  },
{ navmeshtest_mode::TOOLMODE_RAYCAST,                    "Raycast", "..."  },
{ navmeshtest_mode::TOOLMODE_DISTANCE_TO_WALL,           "Distance to wall", "..."  },
{ navmeshtest_mode::TOOLMODE_FIND_POLYS_IN_CIRCLE,       "Find polys in circle", "..."  },
{ navmeshtest_mode::TOOLMODE_FIND_POLYS_IN_SHAPE,        "Find polys in shape", "..."  },
{ navmeshtest_mode::TOOLMODE_FIND_LOCAL_NEIGHBOURHOOD,   "Find local neighbourhood", "..."  },
EndImplementEnumType;

NavMeshTesterTool::NavMeshTesterTool()
   :  mNavMesh(NULL),
      mNavQuery(NULL),
      mPathFindStatus(DT_FAILURE),
      mStraightOptions(0)
{
   mToolMode = TOOLMODE_PATHFIND_FOLLOW;
   mQueryFilter.setIncludeFlags(PolyFlags::AllFlags ^ PolyFlags::DisabledFlag);
   mQueryFilter.setExcludeFlags(0);

   mSpawnClass = mSpawnDatablock = "";
   mPlayer = mCurPlayer = NULL;
}

NavMeshTesterTool::~NavMeshTesterTool()
{
}

void NavMeshTesterTool::initPersistFields()
{
}

bool NavMeshTesterTool::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void NavMeshTesterTool::onRemove()
{
   Parent::onRemove();
}

void NavMeshTesterTool::inspectPostApply()
{
   Parent::inspectPostApply();

   // all settings point to a recalc.
   recalc();
}

void NavMeshTesterTool::setActiveNavMesh(NavMesh* navMesh)
{
   mNavMeshParent = navMesh;
   mNavMesh = navMesh->getNavMesh();
   mNavQuery = navMesh->getNavMeshQuery();

   recalc();

   if (mNavQuery)
   {
      mQueryFilter.setAreaCost(PolyAreas::GroundArea,  1.0f);
      mQueryFilter.setAreaCost(PolyAreas::WaterArea,  10.0f);
      mQueryFilter.setAreaCost(PolyAreas::RoadArea,    1.0f);
      mQueryFilter.setAreaCost(PolyAreas::DoorArea,    1.0f);
      mQueryFilter.setAreaCost(PolyAreas::GrassArea,   2.0f);
      mQueryFilter.setAreaCost(PolyAreas::OffMeshArea, 1.5f); // Jump
   }

}

void NavMeshTesterTool::onActivated(const Gui3DMouseEvent& lastEvent)
{
}

void NavMeshTesterTool::onDeactivated()
{
}

void NavMeshTesterTool::spawnPlayer(const Point3F& pos)
{
   SceneObject* obj = (SceneObject*)Sim::spawnObject(mSpawnClass, mSpawnDatablock);
   if (obj)
   {
      MatrixF mat(true);
      mat.setPosition(pos);
      obj->setTransform(mat);
      SimObject* cleanup = Sim::findObject("MissionCleanup");
      if (cleanup)
      {
         SimGroup* missionCleanup = dynamic_cast<SimGroup*>(cleanup);
         missionCleanup->addObject(obj);
      }
      mPlayer = static_cast<AIPlayer*>(obj);
      Con::executef(this, "onPlayerSelected", Con::getIntArg(mPlayer->mLinkTypes.getFlags()));
   }
}

void NavMeshTesterTool::on3DMouseDown(const Gui3DMouseEvent& evt)
{
   Point3F startPnt = evt.pos;
   Point3F endPnt = evt.pos + evt.vec * 1000.0f;

   RayInfo ri;

   U8 keys = Input::getModifierKeys();
   bool shift = keys & SI_LSHIFT;
   bool ctrl = keys & SI_LCTRL;

   // Spawn new character
   if (ctrl)
   {
      if (gServerContainer.castRay(startPnt, endPnt, StaticObjectType, &ri))
         spawnPlayer(ri.point);
   }
   // Deselect character
   else if (shift)
   {
      mPlayer = NULL;
      Con::executef(this, "onPlayerDeselected");
   }
   // Select/move character
   else
   {
      if (gServerContainer.castRay(startPnt, endPnt, PlayerObjectType, &ri))
      {
         if (dynamic_cast<AIPlayer*>(ri.object))
         {
            mPlayer = dynamic_cast<AIPlayer*>(ri.object);
            Con::executef(this, "onPlayerSelected", Con::getIntArg(mPlayer->mLinkTypes.getFlags()));
         }
      }
      else if (!mPlayer.isNull() && gServerContainer.castRay(startPnt, endPnt, StaticObjectType, &ri))
         mPlayer->setPathDestination(ri.point);
   }


}

void NavMeshTesterTool::on3DMouseUp(const Gui3DMouseEvent& evt)
{
}

void NavMeshTesterTool::onRender3D()
{
}

void NavMeshTesterTool::onRender2D()
{
}

bool NavMeshTesterTool::updateGuiInfo()
{
   SimObject* statusbar;
   if (!Sim::findObject("EditorGuiStatusBar", statusbar))
      return false;

   String text;



   return true;
}

void NavMeshTesterTool::recalc()
{
}

void NavMeshTesterTool::drawAgent(const Point3F& pos)
{
}
