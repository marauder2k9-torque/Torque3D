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

#ifndef _STOCK_PHYSICS_CONSTRAINTS_H_
#define _STOCK_PHYSICS_CONSTRAINTS_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _T3D_PHYSICS_STOCKBODY_H_
#include "T3D/physics/stock/stockBody.h"
#endif

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

class StockBody;

class StockEngineConstraint
{
protected:
   StockBody* mBodyA = nullptr;
   StockBody* mBodyB = nullptr;
   Point3F mAnchor;
   F32 mSpringStiffness = 0.0f;
   F32 mSpringDamping = 0.0f;
   bool mIsBreakable = false;
   F32 mBreakForce = F32_MAX;
public:
   virtual ~StockEngineConstraint() {}

   void setBodyA(StockBody* body) { mBodyA = body; }
   void setBodyB(StockBody* body) { mBodyB = body; }
   void setAnchor(const Point3F& anchor) { mAnchor = anchor; }
   void setSpring(F32 stiffness, F32 damping) { mSpringStiffness = stiffness; mSpringDamping = damping; }
   void setBreakable(bool breakable, F32 force) { mIsBreakable = breakable; mBreakForce = force; }

   virtual void solve() = 0;
   virtual void destroy() = 0;

   // Getters
   StockBody* getBodyA() { return mBodyA; }
   StockBody* getBodyB() { return mBodyB; }
};

/**
* @class StockEngineDistance
* @brief A distance or rope constraint that maintains a fixed distance between two bodies.
*
* This constraint can act as a rope (no compression) or a spring-based connector
* that applies forces to maintain a defined distance.
* @see BaseType
* @see StockEngineConstraint
*/
class StockEngineDistance : public StockEngineConstraint
{
protected:
   F32 mMaxDistance;
   F32 mMinDistance;
public:

   StockEngineDistance(StockBody* bodyA, StockBody* bodyB, F32 minDistance = -1.0f, F32 maxDistance = 1.0f);

   /// <summary>
   /// Set the limits for this constraint.
   /// </summary>
   /// <param name="minDistance">Minimum distance float.</param>
   /// <param name="maxDistance">Maximum distance float.</param>
   void setLimits(F32 minDistance, F32 maxDistance);

   /// <summary>
   /// Set the world space anchor position.
   /// </summary>
   void setAnchor(const Point3F& anchor);

   /// <summary>
   /// Set the minimum distance.
   /// </summary>
   /// <param name="minDistance">Minimum distance float.</param>
   void setMinDistance(F32 minDistance);

   /// <summary>
   /// Set the maximum distance.
   /// </summary>
   /// <param name="maxDistance">Maximum distance float.</param>
   void setMaxDistance(F32 maxDistance);

   void solve() override;
   void destroy() override;
};

/**
* @class StockEngineSlider
* @brief A prismatic joint that allows linear movement along a single axis.
*
* This constraint restricts movement to a given axis while preventing rotation.
* It supports limits, spring forces, damping, and breakable conditions.
*/
class StockEngineSlider : public StockEngineConstraint
{
protected:
   VectorF mSlideAxis;
   F32 mLowerLimit;
   F32 mUpperLimit;

public:

   /// <summary>
   /// Construct a Slider Constraint.
   /// </summary>
   /// <param name="bodyA">Body A.</param>
   /// <param name="bodyB">Body B.</param>
   /// <param name="anchor">The Anchor Point.</param>
   /// <param name="axis">Unit vector representing the axis.</param>
   StockEngineSlider(StockBody* bodyA, StockBody* bodyB, const Point3F& anchor, const VectorF& axis);

   /// <summary>
   /// Set the axis.
   /// </summary>
   /// <param name="axis">Unit vector representing the axis.</param>
   void setAxis(const VectorF& axis);

   /// <summary>
   /// Set the limits for this constraint.
   /// </summary>
   /// <param name="lower">Upper limit float.</param>
   /// <param name="upper">Lower limit float.</param>
   void setLimits(F32 lower, F32 upper) { mLowerLimit = lower; mUpperLimit = upper; }

   void solve() override;
   void destroy() override;
};

/**
* @class StockEngineHinge
* @brief A revolute joint that allows rotation around a single axis.
*
* This constraint behaves like a door hinge, allowing limited rotation around an axis.
* It supports limits, spring forces, damping, and breakable conditions.
*/
class StockEngineHinge : public StockEngineConstraint
{
protected:
   VectorF mHingeAxis;
   F32 mLowerLimit;
   F32 mUpperLimit;
public:

   /// <summary>
   /// Construct a Hinge Constraint.
   /// </summary>
   /// <param name="bodyA">Body A.</param>
   /// <param name="bodyB">Body B.</param>
   /// <param name="anchor">The Anchor Point.</param>
   /// <param name="axis">Unit vector representing the axis.</param>
   StockEngineHinge(StockBody* bodyA, StockBody* bodyB, const Point3F& anchor, const VectorF& axis);

   /// <summary>
   /// Set the axis.
   /// </summary>
   /// <param name="axis">Unit vector representing the axis.</param>
   void setAxis(const VectorF& axis);

   /// <summary>
   /// Set the limits for this constraint.
   /// </summary>
   /// <param name="lower">Upper limit in radians.</param>
   /// <param name="upper">Lower limit in radians.</param>
   void setLimits(F32 lower, F32 upper) { mLowerLimit = lower; mUpperLimit = upper; }

   void solve() override;
   void destroy() override;
};

/**
* @class StockEngineUniversal
* @brief A universal joint that allows rotation around two perpendicular axes.
*
* This constraint prevents translation while allowing controlled rotation on two axes.
* It supports rotation limits, spring forces, damping, and breakable conditions.
*/
class StockEngineUniversal : public StockEngineConstraint
{
protected:
   VectorF mAxis1;
   VectorF mAxis2;
   F32 mLowerLimit1, mLowerLimit2;
   F32 mUpperLimit1, mUpperLimit2;
   bool mUseLimits;
public:
   /// <summary>
   /// Construct a Universal Constraint.
   /// </summary>
   /// <param name="bodyA">Body A.</param>
   /// <param name="bodyB">Body B.</param>
   /// <param name="anchor">The Anchor Point.</param>
   /// <param name="axis1">Unit vector representing the primary axis.</param>
   /// <param name="axis2">Unit vector representing the secondary axis.</param>
   StockEngineUniversal(StockBody* bodyA, StockBody* bodyB, const Point3F& anchor, const VectorF& axis1, const VectorF& axis2);

   /// <summary>
   /// Set the limits for the universal joint
   /// </summary>
   /// <param name="lowerLimit1">The primary axis lower limit.</param>
   /// <param name="lowerLimit2">The secondary axis lower limit.</param>
   /// <param name="upperLimit1">The primary axis upper limit.</param>
   /// <param name="upperLimit2">The secondary axis upper limit.</param>
   /// <param name="enableLimits">Use the limits.</param>
   void setLimits(F32 lowerLimit1, F32 lowerLimit2, F32 upperLimit1, F32 upperLimit2, bool enableLimits = true);

   void solve() override;
   void destroy() override;
};

/**
* @class StockEngineGear
* @brief A gear joint that links the rotation of two bodies with a fixed ratio.
*
* This constraint ensures that one body rotates proportionally to the other
* based on `mGearRatio`. It does not use spring or damping.
*/
class StockEngineGear : public StockEngineConstraint
{
protected:
   F32 mGearRatio;

public:
   /// <summary>
   /// Construct a Gear Constraint.
   /// </summary>
   /// <param name="bodyA">Body A.</param>
   /// <param name="bodyB">Body B.</param>
   /// <param name="gearRatio">The gear ratio.</param>
   StockEngineGear(StockBody* bodyA, StockBody* bodyB, F32 gearRatio = 1.0f);

   /// <summary>
   /// Sets the gear ratio.
   /// </summary>
   /// <param name="gearRatio">The gear ratio.</param>
   void setGearRatio(F32 gearRatio);

   void solve() override;
   void destroy() override;
};

#endif // !_STOCK_PHYSICS_CONSTRAINTS_H_
