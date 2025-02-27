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
#include "T3D/physics/stock/stockPlugin.h"

#include "T3D/physics/physicsShape.h"
#include "T3D/physics/stock/stockWorld.h"
#include "T3D/physics/stock/stockBody.h"
#include "T3D/physics/stock/stockCollision.h"

#include "T3D/gameBase/gameProcess.h"
#include "core/util/tNamedFactory.h"

AFTER_MODULE_INIT(Sim)
{
   NamedFactory < PhysicsPlugin>::add("Stock", &StockPlugin::create);
}

PhysicsPlugin* StockPlugin::create()
{
   return new StockPlugin();
}

StockPlugin::StockPlugin()
{
}

StockPlugin::~StockPlugin()
{
}

void StockPlugin::destroyPlugin()
{
   // Cleanup any worlds that are still kicking.
   Map<StringNoCase, PhysicsWorld*>::Iterator iter = mPhysicsWorldLookup.begin();
   for (; iter != mPhysicsWorldLookup.end(); iter++)
   {
      iter->value->destroyWorld();
      delete iter->value;
   }
   mPhysicsWorldLookup.clear();

   delete this;
}

void StockPlugin::reset()
{
}

PhysicsCollision* StockPlugin::createCollision()
{
   return new StockCollision();
}

PhysicsBody* StockPlugin::createBody()
{
   return new StockBody();
}

PhysicsPlayer* StockPlugin::createPlayer()
{
   return nullptr;
}

PhysicsConstraint* StockPlugin::createConstraint()
{
   return nullptr;
}

bool StockPlugin::isSimulationEnabled() const
{
   bool ret = false;
   StockWorld* world = static_cast<StockWorld*>(getWorld(smClientWorldName));
   if (world)
   {
      ret = world->getEnabled();
      return ret;
   }

   world = static_cast<StockWorld*>(getWorld(smServerWorldName));
   if (world)
   {
      ret = world->getEnabled();
      return ret;
   }

   return ret;
}

void StockPlugin::enableSimulation(const String& worldName, bool enable)
{
   StockWorld* world = static_cast<StockWorld*>(getWorld(worldName));
   if (world)
      world->setEnabled(enable);
}

void StockPlugin::setTimeScale(const F32 timeScale)
{
   // Set both worlds timescale
   StockWorld* world = static_cast<StockWorld*>(getWorld(smClientWorldName));
   if (world)
      world->setTimeScale(timeScale);

   world = static_cast<StockWorld*>(getWorld(smServerWorldName));
   if (world)
      world->setTimeScale(timeScale);
}

const F32 StockPlugin::getTimeScale() const
{
   // both worlds timescales must match, make sure we have both.
   StockWorld* world = static_cast<StockWorld*>(getWorld(smClientWorldName));
   if (!world)
   {
      world = static_cast<StockWorld*>(getWorld(smServerWorldName));
      if (!world)
         return 0.0f;
   }

   return world->getTimeScale();
}

bool StockPlugin::createWorld(const String& worldName)
{
   Map<StringNoCase, PhysicsWorld*>::Iterator iter = mPhysicsWorldLookup.find(worldName);
   if (iter != mPhysicsWorldLookup.end())
   {
      Con::errorf("StockPlugin::createWorld - %s world already exists!", worldName.c_str());
      return false;
   }

   PhysicsWorld* world = new StockWorld();

   if (worldName.equal(smClientWorldName, String::NoCase))
      world->initWorld(false, ClientProcessList::get());
   else
      world->initWorld(true, ServerProcessList::get());

   mPhysicsWorldLookup.insert(worldName, world);

   return world != NULL;
}

void StockPlugin::destroyWorld(const String& worldName)
{
   Map<StringNoCase, PhysicsWorld*>::Iterator iter = mPhysicsWorldLookup.find(worldName);
   if (iter == mPhysicsWorldLookup.end())
      return;

   PhysicsWorld* world = (*iter).value;
   world->destroyWorld();
   delete world;

   mPhysicsWorldLookup.erase(iter);
}

PhysicsWorld* StockPlugin::getWorld(const String& worldName) const
{
   if (mPhysicsWorldLookup.isEmpty())
      return NULL;

   Map<StringNoCase, PhysicsWorld*>::ConstIterator iter = mPhysicsWorldLookup.find(worldName);

   return iter != mPhysicsWorldLookup.end() ? (*iter).value : NULL;
}


