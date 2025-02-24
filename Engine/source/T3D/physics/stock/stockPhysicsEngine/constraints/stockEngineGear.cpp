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

StockEngineGear::StockEngineGear(StockBody* bodyA, StockBody* bodyB, F32 gearRatio)
{
   mBodyA = bodyA;
   mBodyB = bodyB;
   mGearRatio = gearRatio;
}

void StockEngineGear::setGearRatio(F32 gearRatio)
{
   mGearRatio = gearRatio;
}

void StockEngineGear::solve()
{
   // Get the angular velocities of both bodies
   VectorF angVelA = mBodyA->getAngVelocity();
   VectorF angVelB = mBodyB->getAngVelocity();

   // Calculate the target angular velocity for body B based on body A's angular velocity and the gear ratio
   VectorF targetAngVelB = angVelA * mGearRatio;

   // Calculate the angular velocity error (how much the actual angular velocity of body B deviates from the target)
   VectorF angularVelocityError = angVelB - targetAngVelB;

   // Apply corrective torque to both bodies to align their angular velocities based on the gear ratio
   VectorF correctiveTorque = -angularVelocityError;

   // Apply corrective torque to body A (to slow it down) and body B (to speed it up) to match the gear ratio
   mBodyA->applyTorque(-correctiveTorque);
   mBodyB->applyTorque(correctiveTorque);
}

void StockEngineGear::destroy()
{
   mBodyA = nullptr;
   mBodyB = nullptr;
}

