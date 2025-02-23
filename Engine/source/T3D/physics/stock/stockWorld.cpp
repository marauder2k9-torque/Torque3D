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
#include "T3D/physics/stock/stockWorld.h"

#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "platform/profiler.h"
#include "sim/netConnection.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "collision/collision.h"
#include "T3D/gameBase/gameProcess.h"
#include "gfx/sim/debugDraw.h"
#include "gfx/primBuilder.h"

StockWorld::StockWorld()
   :  mIsEnabled(false),
      mIsSimulating(false),
      mIsServer(false),
      mEditorTimeScale(1.0f),
      mProcessList(NULL)
{
}

StockWorld::~StockWorld()
{
   // Release the tick processing signals.
   if (mProcessList)
   {
      //mProcessList->preTickSignal().remove(this, &BtWorld::getPhysicsResults);
      //mProcessList->postTickSignal().remove(this, &BtWorld::tickPhysics);
      mProcessList = NULL;
   }
}

void StockWorld::onDebugDraw(const SceneRenderState* state)
{
}

bool StockWorld::initWorld(bool isServer, ProcessList* processList)
{
   AssertFatal(processList, "StockWorld::initWorld() - We need a process list to create the world!");
   mProcessList = processList;
   //mProcessList->preTickSignal().notify(this, &BtWorld::getPhysicsResults);
   //mProcessList->postTickSignal().notify(this, &BtWorld::tickPhysics);


   mIsServer = isServer;


   return true;
}

void StockWorld::destroyWorld()
{
}

void StockWorld::reset()
{
}

bool StockWorld::isEnabled() const
{
   return false;
}

bool StockWorld::castRay(const Point3F& startPnt, const Point3F& endPnt, RayInfo* ri, const Point3F& impulse)
{
   return false;
}

PhysicsBody* StockWorld::castRay(const Point3F& start, const Point3F& end, U32 bodyTypes)
{
   return nullptr;
}

void StockWorld::explosion(const Point3F& pos, F32 radius, F32 forceMagnitude)
{
}

void StockWorld::setEnabled(bool enabled)
{
}
