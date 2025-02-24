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

StockEngineHinge::StockEngineHinge(StockBody* bodyA, StockBody* bodyB, const Point3F& anchor, const VectorF& axis)
{
   mBodyA = bodyA;
   mBodyB = bodyB;
   mAnchor = anchor;
   mHingeAxis = axis;
   mHingeAxis.normalizeSafe();

   mLowerLimit = -0.785f;
   mUpperLimit = 0.785f;
}

void StockEngineHinge::setAxis(const VectorF& axis)
{
   mHingeAxis = axis;
   mHingeAxis.normalizeSafe();
}

void StockEngineHinge::solve()
{
   MatrixF bodyA, bodyB;
   mBodyB->getTransform(&bodyB);
   mBodyA->getTransform(&bodyA);

   // Calculate the relative rotation between the two bodies based on the anchor point
   VectorF deltaA = bodyA.getPosition() - mAnchor;
   VectorF deltaB = bodyB.getPosition() - mAnchor;

   // Get angular velocities for both bodies
   VectorF angVelA = mBodyA->getAngVelocity();
   VectorF angVelB = mBodyB->getAngVelocity();

   // Calculate the angular displacement (relative rotation between the two bodies around the hinge axis)
   F32 angularDisplacement = mDot(angVelB, mHingeAxis) - mDot(angVelA, mHingeAxis);

   VectorF torque(0.0f, 0.0f, 0.0f);

   // Apply torque if angular displacement is outside the limits
   if (angularDisplacement < mLowerLimit || angularDisplacement > mUpperLimit) {
      // Apply spring force to restore angular displacement within the limits
      if (mSpringStiffness > 0.0f) {
         F32 correctionTorque = -mSpringStiffness * (angularDisplacement - mUpperLimit);
         if (angularDisplacement < mLowerLimit) correctionTorque = -mSpringStiffness * (angularDisplacement - mLowerLimit);
         torque += correctionTorque * mHingeAxis;
      }

      // Apply damping torque to reduce angular velocity along the hinge axis
      VectorF dampingTorque = -mSpringDamping * (angVelB - angVelA);
      torque += dampingTorque;

   }
   else {
      // If angular displacement is within the limits, apply only damping torque
      VectorF dampingTorque = -mSpringDamping * (angVelB - angVelA);
      torque += dampingTorque;
   }

   // Check if the torque exceeds the max allowed torque (break the constraint)
   if (mIsBreakable && torque.len() > mBreakForce)
   {
      destroy();
      return;
   }

   // Apply the corrective torque to both bodies (using the cross product with the position vector)
   mBodyA->applyTorque(-torque);
   mBodyB->applyTorque(torque);
}

void StockEngineHinge::destroy()
{
   mBodyA = nullptr;
   mBodyB = nullptr;
}
