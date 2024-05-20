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

   TYPE dot(const Vec<TYPE, size>& other) const {
      TYPE res = TYPE();

      for (U32 i = 0; i < size; ++i) {
         res += this->dim[i] * other[i];
      }

      return res;
   }

   TYPE magnitude() const {
      return std::sqrt(dot(*this));
   }

   Vec<TYPE, size> normalize() const {
      TYPE mag = magnitude();
      AssertFatal(mag != 0, "Magnitude = 0");

      return *this * (1.0 / mag);
   }

   Vec<TYPE, size> direction() const {
      TYPE mag = magnitude();
      AssertFatal(mag != 0, "Magnitude = 0");

      return *this / mag;
   }

   static Vec<TYPE, size> interpolate(const Vec<TYPE, size>&v1, const Vec<TYPE, size>& v2, TYPE fac) {
      fac = std::max(static_cast<TYPE>(0), std::min(static_cast<TYPE>(1), fac));

      return v1 * (1 - fac) + v2 * fac;
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
