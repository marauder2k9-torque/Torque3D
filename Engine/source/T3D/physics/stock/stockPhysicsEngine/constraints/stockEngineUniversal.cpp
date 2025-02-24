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

StockEngineUniversal::StockEngineUniversal(StockBody* bodyA, StockBody* bodyB, const Point3F& anchor, const VectorF& axis1, const VectorF& axis2)
{
   mBodyA = bodyA;
   mBodyB = bodyB;
   mAnchor = anchor;

   mAxis1 = axis1;
   mAxis1.normalizeSafe();

   mAxis2 = axis2;
   mAxis2.normalizeSafe();

   mUseLimits = true;
   mLowerLimit1 = -0.785f; mUpperLimit1 = 0.785f; // 45 degrees in radians
   mLowerLimit2 = -0.785f; mUpperLimit2 = 0.785f; // 45 degrees in radians
}

void StockEngineUniversal::setLimits(F32 lowerLimit1, F32 lowerLimit2, F32 upperLimit1, F32 upperLimit2, bool enableLimits)
{
   mLowerLimit1 = lowerLimit1;
   mLowerLimit2 = lowerLimit2;

   mUpperLimit1 = upperLimit1;
   mUpperLimit2 = upperLimit2;

   mUseLimits = enableLimits;
}

void StockEngineUniversal::solve()
{
   // Get current angular velocities
   VectorF angVelA = mBodyA->getAngVelocity();
   VectorF angVelB = mBodyB->getAngVelocity();
   VectorF relAngVel = angVelB - angVelA; // Relative angular velocity

   // Compute torque needed to keep rotation limited
   VectorF torque1 = mDot(relAngVel, mAxis1) * mAxis1;
   VectorF torque2 = mDot(relAngVel, mAxis2) * mAxis2;

   VectorF totalTorque = torque1 + torque2; // Only allowing rotation on these two axes

   if (mUseLimits)
   {
      // Compute rotation angles along each axis
      F32 angle1 = mDot(angVelB, mAxis1);
      F32 angle2 = mDot(angVelB, mAxis2);

      if (angle1 < mLowerLimit1 || angle1 > mUpperLimit1)
      {
         F32 correction = (angle1 < mLowerLimit1) ? (mLowerLimit1 - angle1) : (mUpperLimit1 - angle1);
         torque1 += correction * mAxis1 * mSpringStiffness;
      }

      if (angle2 < mLowerLimit2 || angle2 > mUpperLimit2)
      {
         F32 correction = (angle2 < mLowerLimit2) ? (mLowerLimit2 - angle2) : (mUpperLimit2 - angle2);
         torque2 += correction * mAxis2 * mSpringStiffness;
      }

      totalTorque = torque1 + torque2;
   }

   // Apply damping if enabled
   if (mSpringDamping > 0.0f)
   {
      VectorF dampingTorque = -mSpringDamping * relAngVel;
      totalTorque += dampingTorque;
   }

   // Check break force condition
   if (mIsBreakable && totalTorque.len() > mBreakForce)
   {
      destroy();
      return;
   }

   // Apply equal and opposite torques to the two bodies
   mBodyA->applyTorque(-totalTorque);
   mBodyB->applyTorque(totalTorque);
}

void StockEngineUniversal::destroy()
{
   mBodyA = nullptr;
   mBodyB = nullptr;
}
