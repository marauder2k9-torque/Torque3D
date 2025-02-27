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

#ifndef _T3D_PHYSICS_STOCKPLUGIN_H_
#define _T3D_PHYSICS_STOCKPLUGIN_H_

#ifndef _STOCK_PHYSICS_ENGINE_H_
#include "T3D/physics/stock/stockPhysicsEngine/stockPhysicsEngine.h"
#endif
#ifndef _T3D_PHYSICS_PHYSICSPLUGIN_H_
#include "T3D/physics/physicsPlugin.h"
#endif

class StockPlugin : public PhysicsPlugin
{
public:

   StockPlugin();
   ~StockPlugin();

   /// Create function for factory.
   static PhysicsPlugin* create();

   // PhysicsPlugin
   void destroyPlugin() override;
   void reset() override;
   PhysicsCollision* createCollision() override;
   PhysicsBody* createBody() override;
   PhysicsPlayer* createPlayer() override;
   PhysicsConstraint* createConstraint() override;
   bool isSimulationEnabled() const override;
   void enableSimulation(const String& worldName, bool enable) override;
   void setTimeScale(const F32 timeScale) override;
   const F32 getTimeScale() const override;
   bool createWorld(const String& worldName) override;
   void destroyWorld(const String& worldName) override;
   PhysicsWorld* getWorld(const String& worldName) const override;
};


#endif
