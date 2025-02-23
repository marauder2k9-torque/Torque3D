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

#include "T3D/physics/stock/stockWorld.h"
#include "T3D/physics/stock/stockCollision.h"

#include "console/console.h"
#include "console/consoleTypes.h"

StockBody::StockBody()
   :mIsEnabled(true),
   mIsStatic(false)
{
}

StockBody::~StockBody()
{
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
}

MatrixF& StockBody::getTransform(MatrixF* outMatrix)
{
   *outMatrix = MatrixF::Identity;
   return *outMatrix;
}

Box3F StockBody::getWorldBounds()
{
   return Box3F();
}

void StockBody::setSimulationEnabled(bool enabled)
{
}

//-----------------------------------------------------------------------------
// PHYSICS OBJECT OVERRIDES END
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PHYSICS BODY OVERRIDES
//-----------------------------------------------------------------------------

bool StockBody::init(PhysicsCollision* shape, F32 mass, U32 bodyFlags, SceneObject* obj, PhysicsWorld* world)
{
   mUserData.setObject(obj);
   mUserData.setBody(this);

   return false;
}

bool StockBody::isDynamic() const
{
   return false;
}

PhysicsCollision* StockBody::getColShape()
{
   return nullptr;
}

void StockBody::setSleepThreshold(F32 linear, F32 angular)
{
}

void StockBody::setDamping(F32 linear, F32 angular)
{
}

void StockBody::getState(PhysicsState* outState)
{
}

F32 StockBody::getMass() const
{
   return F32();
}

Point3F StockBody::getCMassPosition() const
{
   return Point3F();
}

void StockBody::setLinVelocity(const Point3F& vel)
{
}

void StockBody::setAngVelocity(const Point3F& vel)
{
}

Point3F StockBody::getLinVelocity() const
{
   return Point3F();
}

Point3F StockBody::getAngVelocity() const
{
   return Point3F();
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

void StockBody::applyImpulse(const Point3F& origin, const Point3F& force)
{
   if (!isDynamic())
      return;
}

void StockBody::applyTorque(const Point3F& torque)
{
   if (!isDynamic())
      return;
}

void StockBody::applyForce(const Point3F& force)
{
   if (!isDynamic())
      return;
}

void StockBody::findContact(SceneObject** contactObject, VectorF* contactNormal, Vector<SceneObject*>* outOverlapObjects) const
{
}

void StockBody::moveKinematicTo(const MatrixF& xfm)
{
}

bool StockBody::isValid()
{
   return false;
}

//-----------------------------------------------------------------------------
// PHYSICS BODY OVERRIDES END
//-----------------------------------------------------------------------------
