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

#include "stockEngineConstraints.h"

StockEngineDistance::StockEngineDistance(StockBody* bodyA, StockBody* bodyB, F32 minDistance, F32 maxDistance)
{
   mBodyA = bodyA;
   mBodyB = bodyB;
   mMinDistance = minDistance;
   mMaxDistance = maxDistance;
}

void StockEngineDistance::setLimits(F32 minDistance, F32 maxDistance)
{
   mMinDistance = minDistance;
   mMaxDistance = maxDistance;
}

void StockEngineDistance::setMinDistance(F32 minDistance)
{
   mMinDistance = minDistance;
}

void StockEngineDistance::setMaxDistance(F32 maxDistance)
{
   mMaxDistance = maxDistance;
}

void StockEngineDistance::solve()
{
   MatrixF bodyA, bodyB;
   mBodyB->getTransform(&bodyB);
   mBodyA->getTransform(&bodyA);

   // Get body positions
   VectorF posA = bodyA.getPosition();
   VectorF posB = bodyB.getPosition();

   // Compute the vector between the two bodies
   VectorF delta = posB - posA;
   F32 currentDistance = delta.len();

   // Avoid division by zero and normalize delta
   if (currentDistance < 0.0001f)
      return;

   VectorF direction = delta / currentDistance;  // Normalized direction

   // Check constraint limits
   F32 displacement = 0.0f;
   bool applyForce = false;

   if (currentDistance > mMaxDistance)
   {
      displacement = currentDistance - mMaxDistance;
      applyForce = true;
   }
   else if (currentDistance < mMinDistance)
   {
      displacement = currentDistance - mMinDistance;
      applyForce = true;
   }

   if (!applyForce)
      return;

   VectorF force = VectorF(0, 0, 0);
   if (mSpringStiffness > 0.0f) // Spring-like behavior
   {
      // Hooke's Law
      F32 springForce = -mSpringStiffness * displacement;

      // Add damping force
      VectorF relativeVelocity = mBodyB->getLinVelocity() - mBodyA->getLinVelocity();
      F32 dampingForce = -mSpringDamping * mDot(relativeVelocity, direction);

      // Total force
      force = (springForce + dampingForce) * direction;
   }
   else
   {
      force = -displacement * direction;
   }

   // Check if force exceeds break threshold
   if (mIsBreakable && force.len() > mBreakForce)
   {
      destroy();
      return;
   }

   // Apply equal and opposite forces to the two bodies
   mBodyA->applyForce(-force);
   mBodyB->applyForce(force);

}

void StockEngineDistance::destroy()
{
   mBodyA = nullptr;
   mBodyB = nullptr;
}
