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

#ifndef _T3D_PHYSICS_STOCKWORLD_H_
#define _T3D_PHYSICS_STOCKWORLD_H_

#ifndef _STOCK_PHYSICS_ENGINE_H_
#include "T3D/physics/stock/stockPhysicsEngine/stockPhysicsEngine.h"
#endif
#ifndef _T3D_PHYSICS_PHYSICSWORLD_H_
#include "T3D/physics/physicsWorld.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _TOCTREE_H_
#include "core/util/tOctree.h"
#endif
#ifndef _T3D_PHYSICS_STOCKBODY_H_
#include "T3D/physics/stock/stockBody.h"
#endif

class StockBody;

class StockWorld : public PhysicsWorld
{
protected:
   F32   mEditorTimeScale;
   bool	mIsEnabled;
   bool  mIsSimulating;
   bool  mIsServer;
   ProcessList* mProcessList;

   typedef Octree<StockBody*> StockWorldOctree;
public:

   StockWorld();
   virtual ~StockWorld();

   // Physics World overrides
   void onDebugDraw(const SceneRenderState* state) override;
   bool initWorld(bool isServer, ProcessList* processList) override;
   void destroyWorld() override;
   void reset() override;
   bool isEnabled() const override;
   bool castRay(const Point3F& startPnt, const Point3F& endPnt, RayInfo* ri, const Point3F& impulse) override;
   PhysicsBody* castRay(const Point3F& start, const Point3F& end, U32 bodyTypes = BT_All) override;
   void explosion(const Point3F& pos, F32 radius, F32 forceMagnitude) override;
   // Physics World overrides end

   void setTimeScale(F32 timeScale) { mEditorTimeScale = timeScale; }
   const F32 getTimeScale() const { return mEditorTimeScale; }

   void setEnabled(bool enabled);
   bool getEnabled() const { return mIsEnabled; }
};

#endif // !_T3D_PHYSICS_STOCKWORLD_H_
