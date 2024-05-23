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
#ifndef _MVECTOR_H_
#define _MVECTOR_H_

#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

#ifndef _MMATHFN_H_
#include "math/mMathFn.h"
#endif

template<typename TYPE, U32 size>
class Vec : public Point<TYPE, size> {
public:
   using Point<TYPE, size>::Point; // inherit constructors.

   // the dot product
   TYPE dot(const Vec<TYPE, size>& other) const {
      TYPE res = TYPE();

      for (U32 i = 0; i < size; ++i) {
         res += this->dim[i] * other[i];
      }

      return res;
   }

   // the length of the vector
   TYPE magnitude() const {
      return std::sqrt(dot(*this));
   }

   // normalize the vector
   Vec<TYPE, size> normalize() const {
      TYPE mag = magnitude();
      AssertFatal(mag != 0, "Magnitude = 0");

      return *this * (1.0 / mag);
   }

   // interpolate between 2 vectors
   static Vec<TYPE, size> interpolate(const Vec<TYPE, size>&v1, const Vec<TYPE, size>& v2, TYPE fac) {
      fac = std::max(static_cast<TYPE>(0), std::min(static_cast<TYPE>(1), fac));

      return v1 * (1 - fac) + v2 * fac;
   }

   // reflect the vector off a surface
   Vec<TYPE, size> reflect(const Vec<TYPE, size>& normal) {
      Vec<TYPE, size> unitNormal = normal.normalize();

      return *this - (unitNormal * (2.0 * dot(unitNormal)));
   }

   template <typename oTYPE = TYPE, U32 oSize>
   typename std::enable_if<(size >= 3 && oSize >= 3), Vec<oTYPE, oSize>&>::type cross(const Vec<oTYPE, oSize>& other) const {
      Vec<oTYPE, oSize> res;

      res[0] = this->dim[1] * other[2] - this->dim[2] * other[1];
      res[1] = this->dim[2] * other[0] - this->dim[0] * other[2];
      res[2] = this->dim[0] * other[1] - this->dim[1] * other[0];

      return res;
   }

   template <typename oTYPE = TYPE, U32 oSize>
   typename std::enable_if<(size >= 3 && oSize >= 3), void>::type cross(const Vec<oTYPE, oSize>& other, Vec<oTYPE, oSize>& result) const {
      result[0] = this->dim[1] * other[2] - this->dim[2] * other[1];
      result[1] = this->dim[2] * other[0] - this->dim[0] * other[2];
      result[2] = this->dim[0] * other[1] - this->dim[1] * other[0];
   }

};

typedef Vec<F32, 3> Vec3F;

#endif
