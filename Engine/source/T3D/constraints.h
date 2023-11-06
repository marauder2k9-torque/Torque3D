#pragma once
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

#ifndef _CONSTRAINTS_H_
#define _CONSTRAINTS_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif
#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif
#ifndef _MQUAT_H_
#include "math/mQuat.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _RIGID_H_
#include "T3D/rigid.h"
#endif
// default constraint class. for now we will only use springs for soft bodies, but this will be updated
// later to account for more varied types of constraints and adding the jacobian for quicker resolution.

class Constraint
{
public:
   Constraint() {};

   virtual void update() = 0;
};

// Should we make springs between different types, separate classes, or roll the functionality all
// into one spring class?
class Spring : public Constraint
{
private:
   Resource<TSShape> shape;
   S32 boneA;
   S32 boneB;

   F32 stiffness;
   F32 restLength;
   S32 anchor;

   // we use rigids to apply forces and update the transforms.
   Rigid bodyA;
   Rigid bodyB;

public:
   // if we want a spring between bones.
   Spring(Resource<TSShape> inShape, S32 inBoneA, S32 inBoneB);
   Spring(Resource<TSShape> inShape, S32 inBoneA, S32 inBoneB, F32 inStiffness, F32 inRestlength);

   /// <summary>
   /// Set the stiffness of this spring.
   /// </summary>
   /// <param name="inStiff">Spring stiffness. its K constant.</param>
   void setStiffness(F32 inStiff);

   /// <summary>
   /// Set achor, if set to -1 there will be no anchor for this srping.
   /// Setting a bone to an anchor means it will not be moved by the spring at all.
   /// </summary>
   /// <param name="boneId">the id of the bone to use as an anchor.</param>
   void setAnchor(S32 boneId);

   /// <summary>
   /// update function.
   /// </summary>
   virtual void update(F32 delta);
};

#endif
