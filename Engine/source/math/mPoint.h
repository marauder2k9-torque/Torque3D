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
#ifndef _MPOINT_H_
#define _MPOINT_H_

#ifndef _MMATHFN_H_
#include "math/mMathFn.h"
#endif

template<typename TYPE, U32 size>
class Point
{
   static_assert(size >= 2, "Just use float if you dont want 2 or more dimensions");
public:

   TYPE dim[size];

   Point()
   {
      for (U32 i = 0; i < size; i++)
      {
         dim[i] = static_cast<TYPE>(0);
      }
   }

   // Point2
   Point(const TYPE& x, const TYPE& y)
   {
      AssertFatal(size >= 2, "Out of bounds for set");

      dim[0] = x;
      dim[1] = y;
   }

   // Point3
   Point(const TYPE& x, const TYPE& y, const TYPE& z)
   {
      AssertFatal(size >= 3, "Out of bounds for set");

      dim[0] = x;
      dim[1] = y;
      dim[2] = z;
   }

   // Point4
   Point(const TYPE& x, const TYPE& y, const TYPE& z, const TYPE& w)
   {
      AssertFatal(size == 4, "Out of bounds for set");

      dim[0] = x;
      dim[1] = y;
      dim[2] = z;
      dim[3] = w;
   }

   // copy
   Point(const Point<TYPE, size>& point)
   {
      this->set(point.getData());
   }

   inline void set(const TYPE* data)
   {
      for (U32 i = 0; i < size; i++)
      {
         getData()[i] = data[i];
      }
   }

   TYPE& operator [](const U32 i)
   {
      AssertFatal(i < size, "Out of bounds for point");
      return dim[i];
   }

   const TYPE& operator [](const U32 i) const
   {
      AssertFatal(i < size, "Out of bounds for point");
      return dim[i];
   }


   // Accessors to data
   const TYPE* getData() const { return &dim[0]; }
   TYPE* getData() { return &dim[0]; }

   //------------------- Operators ---------------------
   Point<TYPE, size>& operator=(const Point<TYPE, size>& other) {
     if (this != &other) {
         for (U32 i = 0; i < size; ++i)
             dim[i] = other.dim[i];
     }
     return *this;
   }

   Point<TYPE, size> operator+(const Point<TYPE, size>& other) const {
     Point<TYPE, size> result;
     for (U32 i = 0; i < size; ++i)
         result.dim[i] = dim[i] + other.dim[i];
     return result;
   }
   
   Point<TYPE, size>& operator+=(const Point<TYPE, size>& other) {
     for (U32 i = 0; i < size; ++i)
         dim[i] += other.dim[i];
     return *this;
   }

   Point<TYPE, size> operator-(const Point<TYPE, size>& other) const {
     Point<TYPE, size> result;
     for (U32 i = 0; i < size; ++i)
         result[i] = dim[i] - other.dim[i];
     return result;
   }
   
   Point<TYPE, size>& operator-=(const Point<TYPE, size>& other) {
     for (U32 i = 0; i < size; ++i)
         dim[i] -= other.dim[i];
     return *this;
   }

   Point<TYPE, size> operator*(TYPE scalar) const {
     Point<TYPE, size> result;
     for (U32 i = 0; i < size; ++i)
         result[i] = dim[i] * scalar;
     return result;
   }
   
   Point<TYPE, size>& operator*=(TYPE scalar) {
     for (U32 i = 0; i < size; ++i)
         dim[i] *= scalar;
     return *this;
   }
   
   Point<TYPE, size> operator/(TYPE scalar) const {
      Point<TYPE, size> result;
      // should we check for zero?
      for (U32 i = 0; i < size; ++i)
         result[i] = dim[i] / scalar;
      return result;
   }
   
   Point<TYPE, size>& operator/=(TYPE scalar) {
     for (unsigned int i = 0; i < size; ++i)
         dim[i] /= scalar;
     return *this;
   }

   /// For more control please use isEqual.
   bool operator==(const Point<TYPE, size>& other) const {
     for (U32 i = 0; i < size; ++i) {
         if (dim[i] != other.dim[i])
             return false;
     }
     return true;
   }

   bool operator!=(const Point<TYPE, size>& other) const {
        return !(*this == other);
   }

   // this will allow vec2 = vec3
   template<U32 oSize>
   typename std::enable_if<(oSize >= size), Point<TYPE, size>&>::type operator=(const Point<TYPE, oSize>& other) {
      for (U32 i = 0; i < size, ++i) {
         dim[i] = other[i];
      }
      return *this;
   }

};

typedef Point<F32, 3> Point3;

#endif // !_MPOINT_H_
