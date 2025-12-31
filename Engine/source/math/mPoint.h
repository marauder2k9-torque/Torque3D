#pragma once

#ifndef _MPOINT_H_
#define _MPOINT_H_

#ifndef _CONSOLE_H_
#include "console/console.h"
#endif

#ifndef _ENGINETYPEINFO_H_
#include "console/engineTypeInfo.h"
#endif

template<typename T, size_t S>
struct Storage; // primary template left undefined

template<typename T>
struct Storage<T, 2> {
   union {
      T data[2];
      struct { T x, y; };  // anonymous struct
      struct { T r, g; };  // anonymous struct
      struct { T u, v; };  // anonymous struct
   };
};

template<typename T>
struct Storage<T, 3> {
   union {
      T data[3];
      struct { T x, y, z; };  // anonymous struct
      struct { T r, g, b; };  // anonymous struct
      struct { T u, v, s; };  // anonymous struct
   };
};

template<typename T>
struct Storage<T, 4> {
   union {
      T data[4];
      struct { T x, y, z, w; };  // anonymous struct
      struct { T r, g, b, a; };  // anonymous struct
      struct { T u, v, s, t; };  // anonymous struct
   };
};

// For all other sizes, no extra members
template<typename T, size_t S>
struct Storage {
   union {
      T data[S];
   };
};

template<typename DATA_TYPE, size_t size>
class PointBase : public Storage<DATA_TYPE, size> {
public:

   static_assert(size >= 2, "Point type must have larger size than 1.....");

   explicit PointBase()
   {
      std::fill(data, data + size, DATA_TYPE(0));
   }

   PointBase(std::initializer_list<DATA_TYPE> init)
   {
      U32 i = 0;

      // Fill the size we have.
      for (DATA_TYPE v : init)
      {
         if (i < size)
            data[i++] = v;
      }

      // Fill the rest with 0.
      for (; i < size; i++)
      {
         data[i] = DATA_TYPE(0);
      }
   }

   // Copy constructor
   PointBase(const PointBase& other)
   {
      std::copy(other.data, other.data + size, data);
   }

   // Move constructor
   PointBase(PointBase&& other) noexcept
   {
      std::move(other.data, other.data + size, data);
   }

   // Copy assignment
   PointBase& operator=(const PointBase& other)
   {
      if (this != &other)
      {
         std::copy(other.data, other.data + size, data);
      }
      return *this;
   }

   // Move assignment
   PointBase& operator=(PointBase&& other) noexcept
   {
      if (this != &other)
      {
         std::move(other.data, other.data + size, data);
      }
      return *this;
   }

   inline DATA_TYPE& operator[](U32 i) { return data[i]; }
   inline const DATA_TYPE& operator[](U32 i) const { return data[i]; }
   operator DATA_TYPE* () { return data; }
   operator const DATA_TYPE* () const { return data; }

   //-----------------------------------------------------
   // Convenience functions
   //-----------------------------------------------------

   bool isUnitLength() const
   {
      DATA_TYPE sum = DATA_TYPE(0);
      for (U32 i = 1; i < size; i++)
      {
         sum += data[i] * data[i];
      }

      return std::abs(DATA_TYPE(1) - sum);
   }

   DATA_TYPE least() const
   {
      DATA_TYPE res = data[0];
      for (U32 i = 1; i < size; i++)
      {
         res = std::min(data[i], res);
      }
      return res;
   }

   DATA_TYPE most() const
   {
      DATA_TYPE res = data[0];
      for (U32 i = 1; i < size; i++)
      {
         res = std::max(data[i], res);
      }
      return res;
   }

   U32 getLeastComponentIndex() const
   {
      U32 idx = 0;
      for (U32 i = 1; i < size; i++)
      {
         if (data[i] < data[idx])
            idx = i;
      }
      return idx;
   }

   U32 getGreatestComponentIndex() const
   {
      U32 idx = 0;
      for (U32 i = 1; i < size; i++)
      {
         if (data[i] > data[idx])
            idx = i;
      }
      return idx;
   }

   DATA_TYPE lengthSquared() const
   {
      DATA_TYPE sum = DATA_TYPE(0);
      for (U32 i = 0; i < size; i++)
      {
         sum += data[i] * data[i];
      }

      return sum;
   }

   DATA_TYPE length() const
   {
      DATA_TYPE sum = lengthSquared();
      return std::sqrt(sum);
   }

   void normalize()
   {
      DATA_TYPE len = length();
      if (len == DATA_TYPE(0)) return;

      DATA_TYPE denom = DATA_TYPE(1) / len;
      for (U32 i = 0; i < size; i++)
      {
         data[i] = data[i] * denom;
      }
   }

   void normalize(DATA_TYPE fac)
   {
      DATA_TYPE len = length();
      if (len == DATA_TYPE(0)) return;

      DATA_TYPE denom = fac / len;
      for (U32 i = 0; i < size; i++)
      {
         data[i] = data[i] * denom;
      }
   }

   DATA_TYPE magnitudeSafe() const
   {
      if (isZero())
      {
         return DATA_TYPE(0);
      }
      else
      {
         return length();
      }
   }

   PointBase clamp(const PointBase& p, DATA_TYPE min, DATA_TYPE max)
   {
      PointBase res;
      for (U32 i = 0; i < size; i++)
      {
         res[i] = std::max(min, std::min(max, p[i]));
      }
      return res;
   }

   PointBase lerp(const PointBase& a, const PointBase& b, DATA_TYPE factor)
   {
      if (factor == DATA_TYPE(0))
      {
         return PointBase();
      }

      DATA_TYPE invFac = DATA_TYPE(1) / factor;
      PointBase res;
      for (U32 i = 0; i < size; i++)
      {
         res[i] = a[i] * invFac + b[i] * factor;
      }

      return res;
   }

   void zero()
   {
      for (U32 i = 0; i < size; i++)
      {
         data[i] = DATA_TYPE(0);
      }
   }

   bool isZero() const
   {
      for (U32 i = 0; i < size; i++)
      {
         if ((data[i] * data[i]) > DATA_TYPE(0))
            return false;
      }

      return true;
   }

   void interpolate(const PointBase& a, const PointBase& b, DATA_TYPE factor)
   {
      if (factor == DATA_TYPE(0) || factor > DATA_TYPE(1))
      {
         return;
      }

      DATA_TYPE invFac = DATA_TYPE(1) / factor;
      PointBase res;
      for (U32 i = 0; i < size; i++)
      {
         data[i] = a[i] * invFac + b[i] * factor;
      }
   }

   void neg()
   {
      for (U32 i = 0; i < size; i++)
      {
         data[i] = -data[i];
      }
   }

   void convolve(const PointBase& a)
   {
      for (U32 i = 0; i < size; i++)
      {
         data[i] *= a[i];
      }
   }

   void convolveInverse(const PointBase& a)
   {
      for (U32 i = 0; i < size; i++)
      {
         data[i] /= a[i];
      }
   }

   //-----------------------------------------------------
   // END: Convenience functions
   //-----------------------------------------------------

   //-----------------------------------------------------
   // Basic arithmetic shared across all point types.
   //-----------------------------------------------------
   PointBase operator+(const PointBase& rhs) const
   {
      PointBase r;
      for (U32 i = 0; i < size; i++)
      {
         r.data[i] = data[i] + rhs.data[i];
      }
      return r;
   }

   PointBase& operator+=(const PointBase& rhs)
   {
      for (U32 i = 0; i < size; i++)
      {
         data[i] += rhs.data[i];
      }
      return *this;
   }

   PointBase operator-(const PointBase& rhs) const
   {
      PointBase r;
      for (U32 i = 0; i < size; i++)
      {
         r.data[i] = data[i] - rhs.data[i];
      }
      return r;
   }

   PointBase& operator-=(const PointBase& rhs)
   {
      for (U32 i = 0; i < size; i++)
      {
         data[i] -= rhs.data[i];
      }
      return *this;
   }

   PointBase operator*(DATA_TYPE scalar) const
   {
      PointBase r;
      for (U32 i = 0; i < size; i++)
      {
         r.data[i] = data[i] * scalar;
      }
      return r;
   }

   PointBase& operator*=(DATA_TYPE scalar)
   {
      for (U32 i = 0; i < size; i++)
      {
         data[i] *= scalar;
      }
      return *this;
   }

   PointBase operator*(const PointBase& rhs) const
   {
      PointBase r;
      for (U32 i = 0; i < size; ++i)
      {
         r[i] = data[i] * rhs.data[i];
      }
      return r;
   }

   PointBase& operator*=(const PointBase& rhs)
   {
      for (U32 i = 0; i < size; ++i)
      {
         data[i] *= rhs.data[i];
      }
      return *this;
   }

   PointBase operator/(DATA_TYPE scalar) const
   {
      PointBase r;
      DATA_TYPE denom = DATA_TYPE(1) / scalar;
      for (U32 i = 0; i < size; i++)
      {
         r.data[i] = data[i] * denom;
      }
      return r;
   }

   PointBase& operator/=(DATA_TYPE scalar)
   {
      DATA_TYPE denom = DATA_TYPE(1) / scalar;
      for (U32 i = 0; i < size; i++)
      {
         data[i] *= denom;
      }
      return *this;
   }

   bool operator==(const PointBase& rhs) const
   {
      for (U32 i = 0; i < size; ++i)
         if (data[i] != rhs.data[i])
            return false;
      return true;
   }

   bool operator!=(const PointBase& rhs) const
   {
      return !(*this == rhs);
   }

   //-----------------------------------------------------
   // END: Basic arithmetic shared across all point types.
   //-----------------------------------------------------

   //-----------------------------------------------------
   // Vector arithmetic
   //-----------------------------------------------------

   DATA_TYPE dot(const PointBase& rhs) const
   {
      DATA_TYPE sum(0);
      for (U32 i = 0; i < size; i++)
      {
         sum += data[i] * rhs.data[i];
      }
      return sum;
   }

   /// <summary>
   /// Cross product should only be run on 3D points. If you run this on a
   /// non 3d point, you are bad and you should feel bad.
   /// </summary>
   template<U32 S = size>
   typename std::enable_if<S >= 3, PointBase>::type cross(const PointBase& rhs)
   {
      PointBase r;
      r[0] = data[1] * rhs.data[2] - data[2] * rhs.data[1];
      r[1] = data[2] * rhs.data[0] - data[0] * rhs.data[2];
      r[2] = data[0] * rhs.data[1] - data[1] * rhs.data[0];

      return r;
   }

   //-----------------------------------------------------
   // END: Vector arithmetic
   //-----------------------------------------------------
};

//-----------------------------------------------------
// Arithmetic to handle different point sizes.
//-----------------------------------------------------

template<typename DATA_TYPE, U32 A, U32 B>
inline auto operator+(const PointBase<DATA_TYPE, A>& lhs, const PointBase<DATA_TYPE, B>& rhs)->PointBase<DATA_TYPE, (A > B ? A : B)>
{
   constexpr U32 maxSize = (A > B) ? A : B;
   PointBase<DATA_TYPE, maxSize> result;

   for (U32 i = 0; i < maxSize; i++)
   {
      DATA_TYPE l = (i < A) ? lhs[i] : DATA_TYPE(0);
      DATA_TYPE r = (i < B) ? rhs[i] : DATA_TYPE(0);
      result[i] = l + r;
   }

   return result;
}

template<typename DATA_TYPE, U32 A, U32 B>
inline auto operator-(const PointBase<DATA_TYPE, A>& lhs, const PointBase<DATA_TYPE, B>& rhs)->PointBase<DATA_TYPE, (A > B ? A : B)>
{
   constexpr U32 maxSize = (A > B) ? A : B;
   PointBase<DATA_TYPE, maxSize> result;

   for (U32 i = 0; i < maxSize; i++)
   {
      DATA_TYPE l = (i < A) ? lhs[i] : DATA_TYPE(0);
      DATA_TYPE r = (i < B) ? rhs[i] : DATA_TYPE(0);
      result[i] = l - r;
   }

   return result;
}

template<typename DATA_TYPE, U32 A, U32 B>
inline auto operator*(const PointBase<DATA_TYPE, A>& lhs, const PointBase<DATA_TYPE, B>& rhs)->PointBase<DATA_TYPE, (A > B ? A : B)>
{
   constexpr U32 maxSize = (A > B) ? A : B;
   PointBase<DATA_TYPE, maxSize> result;
   for (U32 i = 0; i < maxSize; i++)
   {
      DATA_TYPE l = (i < A) ? lhs[i] : DATA_TYPE(0);
      DATA_TYPE r = (i < B) ? rhs[i] : DATA_TYPE(0);
      result[i] = l * r;
   }
   return result;
}

template<typename DATA_TYPE, U32 A, U32 B>
inline auto operator/(const PointBase<DATA_TYPE, A>& lhs, const PointBase<DATA_TYPE, B>& rhs)->PointBase<DATA_TYPE, (A > B ? A : B)>
{
   constexpr U32 maxSize = (A > B) ? A : B;
   PointBase<DATA_TYPE, maxSize> result;
   for (U32 i = 0; i < maxSize; i++)
   {
      DATA_TYPE l = (i < A) ? lhs[i] : DATA_TYPE(0);
      DATA_TYPE r = (i < B) ? rhs[i] : DATA_TYPE(0);
      result[i] = lhs[i] / rhs[i];
   }
   return result;
}

// Compare different sized points.
template<typename DATA_TYPE, U32 A, U32 B>
inline bool operator==(const PointBase<DATA_TYPE, A>& lhs, const PointBase<DATA_TYPE, B>& rhs)
{
   constexpr U32 minSize = (A < B) ? A : B;

   for (U32 i = 0; i < minSize; i++)
   {
      if (lhs[i] != rhs[i])
         return false;
   }

   // If extra dimensions exist, ensure they are zero
   if constexpr (A < B)
   {
      for (U32 i = A; i < B; i++)
      {
         if (rhs[i] != DATA_TYPE(0))
            return false;
      }
   }
   else if constexpr (B < A)
   {
      for (U32 i = B; i < A; i++)
      {
         if (lhs[i] != DATA_TYPE(0))
            return false;
      }
   }

   return true;
}

//-----------------------------------------------------
// END: Arithmetic to handle different point sizes.
//-----------------------------------------------------

typedef PointBase<F64, 3> Point3Double;
typedef PointBase<F32, 3> Point3Float;
typedef PointBase<S32, 3> Point3Int;
typedef PointBase<U32, 3> Point3UInt;

#endif // !_MPOINT_H_
