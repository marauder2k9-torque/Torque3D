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

StockEngineSlider::StockEngineSlider(StockBody* bodyA, StockBody* bodyB, const Point3F& anchor, const VectorF& axis)
{
   mBodyA = bodyA;
   mBodyB = bodyB;
   mAnchor = anchor;
   mSlideAxis = axis;
   mSlideAxis.normalizeSafe();

   mLowerLimit = -10.0f;
   mUpperLimit = 10.0f;
}

void StockEngineSlider::setAxis(const VectorF& axis)
{
   mSlideAxis = axis;
   mSlideAxis.normalizeSafe();
}

void StockEngineSlider::solve()
{
   MatrixF bodyA, bodyB;
   mBodyB->getTransform(&bodyB);
   mBodyA->getTransform(&bodyA);

   // Calculate the position of the bodies relative to the anchor point
   VectorF deltaA = bodyA.getPosition() - mAnchor;
   VectorF deltaB = bodyB.getPosition() - mAnchor;

   // Calculate the slide distance along the hinge axis (projection onto the axis)
   F32 slideDistance = mDot(deltaB, mSlideAxis) - mDot(deltaA, mSlideAxis);

   // Apply corrective forces only if slideDistance is outside the limits
   VectorF force(0.0f, 0.0f, 0.0f);

   if (slideDistance < mLowerLimit || slideDistance > mUpperLimit) {
      // If slideDistance is outside the limits, apply corrective spring forces
      if (mSpringStiffness > 0.0f) {
         F32 correctionForce = -mSpringStiffness * (slideDistance - mUpperLimit);
         if (slideDistance < mLowerLimit) correctionForce = -mSpringStiffness * (slideDistance - mLowerLimit);
         force += correctionForce * mSlideAxis;
      }

      // Apply damping force
      VectorF dampingForce = -mSpringDamping * mBodyB->getLinVelocity();
      force += dampingForce;

   }
   else {
      // If slideDistance is within the limits, only apply damping force (no spring force)
      VectorF dampingForce = -mSpringDamping * mBodyB->getLinVelocity();
      force += dampingForce;
   }

   // Check if the force exceeds the max allowed force (break the constraint)
   if (mIsBreakable && force.len() > mBreakForce)
   {
      destroy();
      return;
   }

   // Apply the corrective force to both bodies
   mBodyA->applyForce(-force);
   mBodyB->applyForce(force);
}

void StockEngineSlider::destroy()
{
   mBodyA = nullptr;
   mBodyB = nullptr;
}
