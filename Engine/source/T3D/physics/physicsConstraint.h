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

#ifndef _T3D_PHYSICS_PHYSICSCONSTRAINT_H_
#define _T3D_PHYSICS_PHYSICSCONSTRAINT_H_

#ifndef _T3D_PHYSICSCOMMON_H_
#include "T3D/physics/physicsCommon.h"
#endif

#ifndef _T3D_PHYSICS_PHYSICSOBJECT_H_
#include "T3D/physics/physicsObject.h"
#endif

class PhysicsBody;
class PhysicsWorld;

class PhysicsConstraint
{
public:
   virtual ~PhysicsConstraint() {}

   enum
   {
      CT_FIXED = 0,
      CT_SLIDER,
      CT_HINGE,
      CT_UNIVERSAL,
      CT_BALL_SOCKET,
      CT_6DOF,
      CT_CONE_TWIST,
      CT_GEAR,
      CT_ROPE,
      CT_PULLEY,
      CT_SPRING
   };

   virtual bool init(PhysicsBody* bodyA,
                     PhysicsBody* bodyB,
                     U32 type,
                     PhysicsWorld* world) = 0;

   /// <summary>
   /// Set this constraint to breakable.
   /// </summary>
   /// <param name="breakable">Bool for whether this joint should be breakable.</param>
   /// <param name="maxForce">The max force this joint can take before it breaks.</param>
   virtual void setBreakable(bool breakable, F32 maxForce) = 0;

   /// <summary>
   /// Set whether the bodies of this constraint can collide with one another.
   /// </summary>
   /// <param name="enabled">Bool for whether the bodies in the constraint should check for collisions.</param>
   virtual void setCollisionEnabled(bool enabled) = 0;

   /// <summary>
   /// Set the limit for the constraint.
   /// Used by: hinge, slider, 6DOF, cone-twist, ball-socket.
   /// </summary>
   /// <param name="minLimits">Minimum limit.</param>
   /// <param name="maxLimits">Maximum limit.</param>
   virtual void setLimits(const Point3F& minLimits, const Point3F& maxLimits) = 0;

   /// <summary>
   /// Apply damping to smooth movements.
   /// </summary>
   /// <param name="linearDamping">Linear damping.</param>
   /// <param name="angularDamping">Angular damping.</param>
   virtual void setDamping(F32 linearDamping, F32 angularDamping) = 0;

   /// <summary>
   /// Set the axis of rotation.
   /// Used by: Hinge, Universal.
   /// </summary>
   /// <param name="axis">Unit vector representing the axis.</param>
   virtual void setAxis(const VectorF& axis) = 0;

   /// <summary>
   /// Set the secondary axis of rotation.
   /// Used by: Universal, Gear.
   /// </summary>
   /// <param name="axis">Unit vector representing the axis.</param>
   virtual void setSecondaryAxis(const VectorF& axis) = 0;

   /// <summary>
   /// Set the motor target velocity and maximum force.
   /// </summary>
   /// <param name="velocity">The target velocity.</param>
   /// <param name="maxForce">The maximum force.</param>
   virtual void setMotor(F32 velocity, F32 maxForce) = 0;

   /// <summary>
   /// Enable the motor of this constraint.
   /// </summary>
   /// <param name="enabled">Bool for whether to enable this motor or not.</param>
   virtual void enableMotor(bool enabled) = 0;

   /// <summary>
   /// Set the rope joint maximum length.
   /// </summary>
   /// <param name="length">The max length of the rope.</param>
   virtual void setMaxLength(F32 length) = 0;

   /// <summary>
   /// Sets the gear ratio for a gear constraint.
   /// </summary>
   /// <param name="ratio">Float value representing a gear ratio.</param>
   virtual void setGearRatio(F32 ratio) = 0;

   /// <summary>
   /// Set the spring constraints stiffness and damping.
   /// </summary>
   /// <param name="stiffness">Float for the spring stiffness.</param>
   /// <param name="damping">Float for the spring damping.</param>
   virtual void setSpring(F32 stiffness, F32 damping) = 0;

   /// <summary>
   /// Destroy this constraint. This will happen automatically at breaking force.
   /// </summary>
   virtual void destroy() = 0;

};

#endif
