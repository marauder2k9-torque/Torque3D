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

#ifndef _T3D_PHYSICS_STOCKBODY_H_
#define _T3D_PHYSICS_STOCKBODY_H_

#ifndef _STOCK_PHYSICS_ENGINE_H_
#include "T3D/physics/stock/stockPhysicsEngine/stockPhysicsEngine.h"
#endif
#ifndef _T3D_PHYSICS_PHYSICSBODY_H_
#include "T3D/physics/physicsBody.h"
#endif
#ifndef _PHYSICS_PHYSICSUSERDATA_H_
#include "T3D/physics/physicsUserData.h"
#endif
#ifndef _T3D_PHYSICS_STOCKCOLLISION_H_
#include "T3D/physics/stock/stockCollision.h"
#endif // !_T3D_PHYSICS_STOCKCOLLISION_H_
#ifndef _T3D_PHYSICS_STOCKWORLD_H_
#include "T3D/physics/stock/stockWorld.h"
#endif // !_T3D_PHYSICS_STOCKWORLD_H_
#ifndef _REFBASE_H_
#include "core/util/refBase.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

class StockWorld;
class StockCollision;

class StockBody : public PhysicsBody // derived from physicsObject.
{
protected:
   /// Holder to stock types.
   StrongRefPtr<StockCollision> mColShape;

   /// Body Properties
   MatrixF mWorldTransform;      /// World space transform
   F32 mMass;                    /// Mass of the body
   F32 mInvMass;                 /// Inverse mass (1/m)
   MatrixF mInertiaTensor;       /// Local space inertia tensor
   MatrixF mInvInertiaTensor;    /// Inverse inertia tensor (1/I)
   Point3F mCMassPosition;       /// Center of mass position

   /// Motion Properties
   Point3F mLinVelocity;         /// Linear velocity
   Point3F mAngVelocity;         /// Angular velocity
   F32 mLinearDamping;           /// Reduces velocity over time
   F32 mAngularDamping;          /// Reduces rotation over time
   F32 mLinearThreshold;         /// Reduces rotation over time
   F32 mAngularThreshold;        /// Reduces rotation over time

   Point3F mForceAccum;          /// Accumulated forces
   Point3F mTorqueAccum;         /// Accumulated torque

   /// Material Properties
   F32 mRestitution;             /// Bounciness
   F32 mStaticFriction;          /// Static friction
   F32 mFriction;                /// Dynamic friction

   bool mIsEnabled;
   bool mIsStatic;

public:
   StockBody();
   virtual ~StockBody();

   // update functions.
   void stepVelocities(F32 dt);
   void stepTransform(F32 dt);
   void updateInertiaTensor();

   // PhysicsObject overrides
   PhysicsWorld* getWorld() override;
   void setTransform(const MatrixF& xfm) override;
   MatrixF& getTransform(MatrixF* outMatrix) override;
   Box3F getWorldBounds() override;
   void setSimulationEnabled(bool enabled) override;
   bool isSimulationEnabled() override { return mIsEnabled; }

   // PhysicsBody overrides
   bool init(  PhysicsCollision* shape,
               F32 mass,
               U32 bodyFlags,
               SceneObject* obj,
               PhysicsWorld* world) override;
   bool isDynamic() const override;
   PhysicsCollision* getColShape() override;
   void setSleepThreshold(F32 linear, F32 angular) override;
   void setDamping(F32 linear, F32 angular) override;
   void getState(PhysicsState* outState) override;
   F32 getMass() const override;
   Point3F getCMassPosition() const override;
   void setLinVelocity(const Point3F& vel) override;
   void setAngVelocity(const Point3F& vel) override;
   Point3F getLinVelocity() const override;
   Point3F getAngVelocity() const override;
   void setSleeping(bool sleeping) override;
   void setMaterial( F32 restitution,
                     F32 friction,
                     F32 staticFriction) override;
   void applyCorrection(const MatrixF& xfm) override;
   void applyImpulse(const Point3F& origin, const Point3F& impulse) override;
   void applyTorqueImpulse(const Point3F& torque);

   void applyTorque(const Point3F& torque) override;
   void applyForce(const Point3F& force) override;
   void findContact( SceneObject** contactObject,
                     VectorF* contactNormal,
                     Vector<SceneObject*>* outOverlapObjects) const override;
   void moveKinematicTo(const MatrixF& xfm) override;
   bool isValid() override;

};


#endif // !_T3D_PHYSICS_STOCKBODY_H_
