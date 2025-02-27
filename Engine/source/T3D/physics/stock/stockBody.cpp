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
#include "T3D/physics/stock/stockBody.h"

#include "scene/sceneObject.h"
#include "console/console.h"
#include "console/consoleTypes.h"

StockBody::StockBody()
   : mColShape(NULL)
{
   mMass = 1.0f;
   mInvMass = 1.0f;
   mInertiaTensor.identity();
   mInvInertiaTensor.identity();
   mCMassPosition = Point3F::Zero;

   mLinVelocity = Point3F::Zero;
   mAngVelocity = Point3F::Zero;

   mLinearDamping = 0.0f;
   mAngularDamping = 0.0f;
   mLinearThreshold = 0.0f;
   mAngularThreshold = 0.0f;

   mForceAccum = Point3F::Zero;
   mTorqueAccum = Point3F::Zero;

   mIsEnabled = false;
   mIsStatic = false;
}

StockBody::~StockBody()
{
   SAFE_DELETE(mColShape);
}

void StockBody::stepVelocities(F32 dt)
{
   if (!isDynamic())
      return;

   // Setup lin velocity to include force accumulated during step.
   // * by dt 
   mLinVelocity += mForceAccum * (mInvMass * dt);

   // Apply damping.
   mLinVelocity *= (1.0f - mLinearDamping * dt);

   // Setup ang velocity to include force accumulated during step.
   // * by dt 
   Point3F angVel;
   mInvInertiaTensor.mulV(mTorqueAccum, &angVel);
   angVel *= dt;
   mAngVelocity += angVel;

   // Apply damping
   mAngVelocity *= (1.0f - mAngularDamping * dt);
}

void StockBody::predictTransform(F32 dt, MatrixF& predictedMatrix)
{
   if (!isDynamic())
      return;

   // Get the world space cmass position and add the vel to it (* delta)
   Point3F newPos = getCMassPosition() + mLinVelocity * dt;

   QuatF newRot;
   newRot.set(mWorldTransform);

   F32 ang = mAngVelocity.len();
   if (ang != 0.00f)
   {
      // Compute rotation quaternion from angular velocity
      F32 halfAngle = ang * dt * 0.5f;
      F32 sinHalfAngle = mSin(halfAngle);
      F32 cosHalfAngle = mCos(halfAngle);

      QuatF dq(
         sinHalfAngle * (mAngVelocity.x / ang),
         sinHalfAngle * (mAngVelocity.y / ang),
         sinHalfAngle * (mAngVelocity.z / ang),
         cosHalfAngle
      );

      newRot.mul(dq, newRot);
      newRot.normalize();
   }

   MatrixF newTransform(true);
   newRot.setMatrix(&newTransform);
   newTransform.setPosition(newPos);

   predictedMatrix = newTransform;
}

void StockBody::updateInertiaTensor()
{
   // Get the rotation matrix from the world transform (exclude translation)
   MatrixF rotMatrix = mWorldTransform;
   rotMatrix.setPosition(Point3F(0, 0, 0));
   rotMatrix.inverse();
   // Update inertia tensor in local space by rotating it
   mInertiaTensor = rotMatrix * mInertiaTensor * rotMatrix;

   // Update inverse inertia tensor in local space
   mInvInertiaTensor = rotMatrix * mInvInertiaTensor * rotMatrix;
}

void StockBody::clearAccum()
{
   mForceAccum = Point3F::Zero;
   mTorqueAccum = Point3F::Zero;
}

//-----------------------------------------------------------------------------
// PHYSICS OBJECT OVERRIDES
//-----------------------------------------------------------------------------

PhysicsWorld* StockBody::getWorld()
{
   return nullptr;
}

void StockBody::setTransform(const MatrixF& xfm)
{
   mWorldTransform = xfm;
   // Update our velocities and inertia.
   mLinVelocity = getLinVelocity();
   mAngVelocity = getAngVelocity();
   updateInertiaTensor();
}

MatrixF& StockBody::getTransform(MatrixF* outMatrix)
{
   *outMatrix = mWorldTransform;
   return *outMatrix;
}

Box3F StockBody::getWorldBounds()
{
   if (mUserData.getObject())
   {
      return mUserData.getObject()->getWorldBox();
   }

   return Box3F();
}

void StockBody::setSimulationEnabled(bool enabled)
{
   mIsEnabled = enabled;
}

//-----------------------------------------------------------------------------
// PHYSICS OBJECT OVERRIDES END
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PHYSICS BODY OVERRIDES
//-----------------------------------------------------------------------------

bool StockBody::init(PhysicsCollision* shape, F32 mass, U32 bodyFlags, SceneObject* obj, PhysicsWorld* world)
{
   AssertFatal(obj, "StockBody::init - Got a null scene object!");
   AssertFatal(world, "StockBody::init - Got a null world!");
   AssertFatal(dynamic_cast<StockWorld*>(world), "StockBody::init - The world is the wrong type!");
   AssertFatal(shape, "StockBody::init - Got a null collision shape!");
   AssertFatal(dynamic_cast<StockCollision*>(shape), "StockBody::init - The collision shape is the wrong type!");

   mColShape = (StockCollision*)shape;
   mWorldTransform = obj->getWorldTransform();
   mUserData.setObject(obj);
   mUserData.setBody(this);

   mMass = mass;
   mInvMass = (mass > 0.0001f) ? (1.0f / mass) : 0.0f;

   if (mass < 0.0001f)
   {
      mIsStatic = true;
      mInertiaTensor.identity();
      mInvInertiaTensor.identity();
      return true;
   }

   return false;
}

bool StockBody::isDynamic() const
{
   return !mIsStatic;
}

PhysicsCollision* StockBody::getColShape()
{
   if (mColShape)
      return mColShape;

   return nullptr;
}

void StockBody::setSleepThreshold(F32 linear, F32 angular)
{
   mLinearThreshold = linear;
   mAngularThreshold = angular;
}

void StockBody::setDamping(F32 linear, F32 angular)
{
   mLinearDamping = linear;
   mAngularDamping = angular;
}

void StockBody::getState(PhysicsState* outState)
{
}

F32 StockBody::getMass() const
{
   return mMass;
}

Point3F StockBody::getCMassPosition() const
{
   if (!mWorldTransform)
      return Point3F::Zero;

   Point3F worldCMass;
   mWorldTransform.mulP(mCMassPosition, &worldCMass);

   return worldCMass;
}

void StockBody::setLinVelocity(const Point3F& vel)
{
   if (!isDynamic())
      return;

   mLinVelocity = vel;
}

void StockBody::setAngVelocity(const Point3F& vel)
{
   if (!isDynamic())
      return;

   mAngVelocity = vel;
}

Point3F StockBody::getLinVelocity() const
{
   return mLinVelocity;
}

Point3F StockBody::getAngVelocity() const
{
   return mAngVelocity;
}

void StockBody::setSleeping(bool sleeping)
{
}

void StockBody::setMaterial(F32 restitution, F32 friction, F32 staticFriction)
{
}

void StockBody::applyCorrection(const MatrixF& xfm)
{
}

void StockBody::applyImpulse(const Point3F& origin, const Point3F& impulse)
{
   if (!isDynamic())
      return;

   if (mInvMass > 0.0001f)
   {
      mLinVelocity += impulse * mInvMass;

      // Compute torque impulse (r × impulse), where r is the offset from center of mass
      Point3F r = origin - getCMassPosition();
      applyTorqueImpulse(mCross(r, impulse));
   }
}

void StockBody::applyTorqueImpulse(const Point3F& torque)
{
   if (!isDynamic())
      return;

   Point3F angVel;
   mInvInertiaTensor.mulV(torque, &angVel);
   mAngVelocity += angVel;
}

void StockBody::applyTorque(const Point3F& torque)
{
   if (!isDynamic())
      return;

   mTorqueAccum += torque;
}

void StockBody::applyForce(const Point3F& force)
{
   if (!isDynamic())
      return;

   mForceAccum += force;
}

void StockBody::findContact(SceneObject** contactObject, VectorF* contactNormal, Vector<SceneObject*>* outOverlapObjects) const
{
}

void StockBody::moveKinematicTo(const MatrixF& xfm)
{
}

bool StockBody::isValid()
{
   if (mColShape)
      return true;

   return false;
}

//-----------------------------------------------------------------------------
// PHYSICS BODY OVERRIDES END
//-----------------------------------------------------------------------------
