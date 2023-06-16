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

#include "math/mAngAxis.h"
#include "math/mQuat.h"
#include "math/mMatrix.h"

AngAxisF& AngAxisF::set(const QuatF& q)
{
   F32 mag = mSqrt(q.x * q.x + q.y * q.y + q.z * q.z);
   if (mag > 0.0f) {
      F32 cosTheta = q.w;
      F32 twoTheta = 2.0f * (cosTheta < 0.0f ? mAtan2(-mag, -cosTheta) : mAtan2(mag, cosTheta));

      angle = twoTheta;
      axis.set(
         q.x / mag,
         q.y / mag,
         q.z / mag);

   }
   else
   {
      axis.set(1.0f, 0.0f, 0.0f);
      angle = 0.0f;
   }

   return *this;
}
#define qidx(r,c) (r*4 + c)

AngAxisF & AngAxisF::set( const MatrixF & mat )
{
   F32 const* m = mat;
   F32 epsi = 0.01f;
   F32 epsi2 = 0.1f;

   if ((mFabs(m[qidx(1, 0)] - m[qidx(0, 1)]) < epsi) &&
      (mFabs(m[qidx(2, 0)] - m[qidx(0, 2)]) < epsi) &&
      (mFabs(m[qidx(2, 1)] - m[qidx(1, 2)]) < epsi))
   {
      if ((mFabs(m[qidx(1, 0)] - m[qidx(0, 1)]) < epsi2) &&
         (mFabs(m[qidx(2, 0)] - m[qidx(0, 2)]) < epsi2) &&
         (mFabs(m[qidx(2, 1)] - m[qidx(1, 2)]) < epsi2) &&
         (mFabs(m[qidx(0, 0)] + m[qidx(1, 1)] + m[qidx(2, 2)] - 3.0f) < epsi2))
      {
         axis.set(1.0f, 0.0f, 0.0f);
         angle = 0.0f;
         return *this;
      }

      angle = M_PI_F;
      F32 xx = (m[qidx(0, 0)] + 1.0f) / 2.0f;
      F32 yy = (m[qidx(1, 1)] + 1.0f) / 2.0f;
      F32 zz = (m[qidx(2, 2)] + 1.0f) / 2.0f;
      F32 xy = (m[qidx(1, 0)] + m[qidx(0, 1)]) / 4.0f;
      F32 xz = (m[qidx(2, 0)] + m[qidx(0, 2)]) / 4.0f;
      F32 yz = (m[qidx(2, 1)] + m[qidx(1, 2)]) / 4.0f;

      if ((xx > yy) && (xx > zz))
      {
         if (xx < epsi)
         {
            axis.set(.0f, 0.7071f, 0.7071f);
         }
         else
         {
            axis.x = mSqrt(xx);
            axis.y = xy / axis.x;
            axis.z = xz / axis.x;
         }
      }
      else if (yy > zz)
      {
         if (yy < epsi)
         {
            axis.set(0.7071f, 0.0f, 0.7071f);
         }
         else
         {
            axis.y = mSqrt(yy);
            axis.x = xy / axis.y;
            axis.z = yz / axis.y;
         }
      }
      else
      {
         if (zz < epsi)
         {
            axis.set(0.7071f, 0.7071f, 0.0f);
         }
         else
         {
            axis.z = mSqrt(zz);
            axis.x = xz / axis.z;
            axis.y = yz / axis.z;
         }
      }

   }

   F32 s = mSqrt( (m[qidx(1, 2)] - m[qidx(2, 1)]) * (m[qidx(1, 2)] - m[qidx(2, 1)]) +
                  (m[qidx(2, 0)] - m[qidx(0, 2)]) * (m[qidx(2, 0)] - m[qidx(0, 2)]) +
                  (m[qidx(0, 1)] - m[qidx(1, 0)]) * (m[qidx(0, 1)] - m[qidx(1, 0)]));

   if (mFabs(s) < POINT_EPSILON)
      s = 1.0f;

   angle = mAcos( (m[qidx(0, 0)] + m[qidx(1, 1)] + m[qidx(2, 2)] - 1.0f) / 2.0f);
   axis.x = (m[qidx(1, 2)] - m[qidx(2, 1)]) / s;
   axis.y = (m[qidx(2, 0)] - m[qidx(0, 2)]) / s;
   axis.z = (m[qidx(0, 1)] - m[qidx(1, 0)]) / s;


   //QuatF q( mat );
   //set( q );

   return *this;
}

MatrixF * AngAxisF::setMatrix( MatrixF * mat ) const
{
   F32* m = *mat;

   F32 c = mCos(angle);
   F32 s = mSin(angle);
   F32 t = 1.0f - c;

   F32 tmp1 = axis.x * axis.y * t;
   F32 tmp2 = axis.z * s;

   m[qidx(0, 0)] = c + axis.x * axis.x * t;
   m[qidx(1, 1)] = c + axis.y * axis.y * t;
   m[qidx(2, 2)] = c + axis.z * axis.z * t;

   m[qidx(0, 1)] = tmp1 + tmp2;
   m[qidx(1, 0)] = tmp1 - tmp2;

   tmp1 = axis.x * axis.z * t;
   tmp2 = axis.z * s;

   m[qidx(0, 2)] = tmp1 - tmp2;
   m[qidx(2, 0)] = tmp1 + tmp2;

   tmp1 = axis.y * axis.z * t;
   tmp2 = axis.x * s;
   m[qidx(1, 2)] = tmp1 + tmp2;
   m[qidx(2, 1)] = tmp1 - tmp2;

   // intiialixe the rest to identity.
   m[qidx(3, 0)] = 0.0f;
   m[qidx(3, 1)] = 0.0f;
   m[qidx(3, 2)] = 0.0f;

   m[qidx(0, 3)] = 0.0f;
   m[qidx(1, 3)] = 0.0f;
   m[qidx(2, 3)] = 0.0f;
   m[qidx(3, 3)] = 1.0f;

   return mat;

   /*QuatF q( *this );
   return q.setMatrix( mat );*/
}

void AngAxisF::RotateX(F32 angle, MatrixF * mat)
{
   // for now...do it the easy way
   AngAxisF rotX(Point3F(1.0f,0.0f,0.0f),angle);
   rotX.setMatrix(mat);
}

void AngAxisF::RotateY(F32 angle, MatrixF * mat)
{
   // for now...do it the easy way
   AngAxisF rotY(Point3F(0.0f,1.0f,0.0f),angle);
   rotY.setMatrix(mat);
}

void AngAxisF::RotateZ(F32 angle, MatrixF * mat)
{
   // for now...do it the easy way
   AngAxisF rotZ(Point3F(0.0f,0.0f,1.0f),angle);
   rotZ.setMatrix(mat);
}

void AngAxisF::RotateX(F32 angle, const Point3F & from, Point3F * to)
{
   // for now...do it the easy way
   MatrixF mat;
   AngAxisF::RotateX(angle,&mat);
   mat.mulV(from,to);
}

void AngAxisF::RotateY(F32 angle, const Point3F & from, Point3F * to)
{
   // for now...do it the easy way
   MatrixF mat;
   AngAxisF::RotateY(angle,&mat);
   mat.mulV(from,to);
}

void AngAxisF::RotateZ(F32 angle, const Point3F & from, Point3F * to)
{
   // for now...do it the easy way
   MatrixF mat;
   AngAxisF::RotateZ(angle,&mat);
   mat.mulV(from,to);
}

