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

#ifndef _MPOINT3_H_
#define _MPOINT3_H_

#ifndef _MMATHFN_H_
#include "math/mMathFn.h"
#endif
#ifndef _MPOINT2_H_
#include "math/mPoint2.h"
#endif

//------------------------------------------------------------------------------
/// 3D integer point
///
/// Uses S32 internally.
class Point3I
{
   //-------------------------------------- Public data
  public:
   S32 x;                                                   ///< X co-ordinate
   S32 y;                                                   ///< Y co-ordinate
   S32 z;                                                   ///< Z co-ordinate

   //-------------------------------------- Public interface
  public:
   Point3I();               ///< Create an uninitialized point.
   Point3I(const Point3I&); ///< Copy constructor.
   explicit Point3I(S32 xyz);        ///< Initializes all elements to the same value.
   Point3I(S32 in_x, S32 in_y, S32 in_z); ///< Create a point from co-ordinates.

   //-------------------------------------- Non-math mutators and misc functions
   void set(S32 xyz);           ///< Initializes all elements to the same value.
   void set(S32 in_x, S32 in_y, S32 in_z); ///< Set co-ordinates.
   void setMin(const Point3I&); ///< Store lesser co-ordinates in this point.
   void setMax(const Point3I&); ///< Store greater co-ordinates in this point.
   void zero();                 ///< Zero all values

   //-------------------------------------- Math mutators
   void neg();                      ///< Invert co-ordinate's signs.
   void convolve(const Point3I&);   ///< Convolve by parameter.

   //-------------------------------------- Queries
   bool isZero() const;             ///< Check for point at origin. (No epsilon.)
   F32  len() const;                ///< Get length.

   //-------------------------------------- Overloaded operators
  public:
   operator S32*() { return &x; }
   operator const S32*() const { return &x; }

   // Comparison operators
   bool operator==(const Point3I&) const;
   bool operator!=(const Point3I&) const;

   // Arithmetic w/ other points
   Point3I  operator+(const Point3I&) const;
   Point3I  operator-(const Point3I&) const;
   Point3I& operator+=(const Point3I&);
   Point3I& operator-=(const Point3I&);

   // Arithmetic w/ scalars
   Point3I  operator*(S32) const;
   Point3I& operator*=(S32);
   Point3I  operator/(S32) const;
   Point3I& operator/=(S32);

   // Unary operators
   Point3I operator-() const;

   //-------------------------------------- Public static constants
public:
   const static Point3I One;
   const static Point3I Zero;
};

class Point3D;

//------------------------------------------------------------------------------
class Point3F
{
   //-------------------------------------- Public data
  public:
   F32 x;
   F32 y;
   F32 z;

  public:
   Point3F();
   Point3F(const Point3F&);
   Point3F(F32 _x, F32 _y, F32 _z);
   explicit Point3F(F32 xyz);

   //-------------------------------------- Non-math mutators and misc functions
  public:
   void set(F32 xyz);
   void set(F32 _x, F32 _y, F32 _z);
   void set(const Point3F&);

   void setMin(const Point3F&);
   void setMax(const Point3F&);

   void interpolate(const Point3F&, const Point3F&, F32);
   void zero();

   /// Returns the smallest absolute value.
   F32 least() const;

   /// Returns the greatest absolute value.
   F32 most() const;

   operator F32*() { return &x; }
   operator const F32*() const { return &x; }

   /// Returns the x and y coords as a Point2F.
   Point2F asPoint2F() const { return Point2F( x, y ); }

   //-------------------------------------- Queries
  public:
   bool  isZero() const;
   bool  isUnitLength() const;
   F32   len()    const;
   F32   lenSquared() const;
   F32   magnitudeSafe() const;
   bool  equal( const Point3F &compare, F32 epsilon = POINT_EPSILON ) const;
   U32   getLeastComponentIndex() const;
   U32   getGreatestComponentIndex() const;

   //-------------------------------------- Mathematical mutators
  public:
   void neg();
   void normalize();
   void normalizeSafe();
   void normalize(F32 val);
   void convolve(const Point3F&);
   void convolveInverse(const Point3F&);

   //-------------------------------------- Overloaded operators
  public:
   // Comparison operators
   bool operator==(const Point3F&) const;
   bool operator!=(const Point3F&) const;

   // Arithmetic w/ other points
   Point3F  operator+(const Point3F&) const;
   Point3F  operator-(const Point3F&) const;
   Point3F& operator+=(const Point3F&);
   Point3F& operator-=(const Point3F&);

   // Arithmetic w/ scalars
   Point3F  operator*(F32) const;
   Point3F  operator/(F32) const;
   Point3F& operator*=(F32);
   Point3F& operator/=(F32);

   Point3F  operator*(const Point3F&) const;
   Point3F& operator*=(const Point3F&);
   Point3F  operator/(const Point3F&) const;
   Point3F& operator/=(const Point3F&);

   // Unary operators
   Point3F operator-() const;

   Point3F& operator=(const Point3D&);

   //-------------------------------------- Public static constants
public:
   const static Point3F One;
   const static Point3F Zero;
   const static Point3F Max;
   const static Point3F Min;
   const static Point3F UnitX;
   const static Point3F UnitY;
   const static Point3F UnitZ;
};

typedef Point3F VectorF;
typedef Point3F EulerF;


//------------------------------------------------------------------------------
class Point3D
{
   //-------------------------------------- Public data
  public:
   F64 x;
   F64 y;
   F64 z;

  public:
   Point3D();
   Point3D(const Point3D&);
   Point3D(const Point3F&);
   explicit Point3D(F64 xyz);
   Point3D(F64 _x, F64 _y, F64 _z);

   //-------------------------------------- Non-math mutators and misc functions
  public:
   void set(F64 xyz);
   void set(F64 _x, F64 _y, F64 _z);

   void setMin(const Point3D&);
   void setMax(const Point3D&);

   void interpolate(const Point3D&, const Point3D&, F64);
   void zero();

   operator F64*() { return (&x); }
   operator const F64*() const { return &x; }

   //-------------------------------------- Queries
  public:
   bool  isZero() const;
   F64 len()    const;
   F64 lenSquared() const;
   F64 magnitudeSafe() const;

   //-------------------------------------- Mathematical mutators
  public:
   void neg();
   void normalize();
   void normalizeSafe();
   void normalize(F64 val);
   void convolve(const Point3D&);
   void convolveInverse(const Point3D&);

   //-------------------------------------- Overloaded operators
  public:
   Point3F toPoint3F() const;
   // Comparison operators
   bool operator==(const Point3D&) const;
   bool operator!=(const Point3D&) const;

   // Arithmetic w/ other points
   Point3D  operator+(const Point3D&) const;
   Point3D  operator-(const Point3D&) const;
   Point3D& operator+=(const Point3D&);
   Point3D& operator-=(const Point3D&);

   // Arithmetic w/ scalars
   Point3D  operator*(F64) const;
   Point3D  operator/(F64) const;
   Point3D& operator*=(F64);
   Point3D& operator/=(F64);

   // Unary operators
   Point3D operator-() const;

   //-------------------------------------- Public static constants
public:
   const static Point3D One;
   const static Point3D Zero;
};

//------------------------------------------------------------------------------
//-------------------------------------- Point3I
//
inline Point3I::Point3I()
   : x(0), y(0), z(0)
{
}

inline Point3I::Point3I(const Point3I& _copy)
 : x(_copy.x), y(_copy.y), z(_copy.z)
{
   //
}

inline Point3I::Point3I(S32 xyz)
 : x(xyz), y(xyz), z(xyz)
{
   //
}

inline Point3I::Point3I(S32 _x, S32 _y, S32 _z)
 : x(_x), y(_y), z(_z)
{
   //
}

inline void Point3I::set(S32 xyz)
{
   x = y = z = xyz;
}

inline void Point3I::set(S32 _x, S32 _y, S32 _z)
{
   x = _x;
   y = _y;
   z = _z;
}

inline void Point3I::setMin(const Point3I& _test)
{
   x = (_test.x < x) ? _test.x : x;
   y = (_test.y < y) ? _test.y : y;
   z = (_test.z < z) ? _test.z : z;
}

inline void Point3I::setMax(const Point3I& _test)
{
   x = (_test.x > x) ? _test.x : x;
   y = (_test.y > y) ? _test.y : y;
   z = (_test.z > z) ? _test.z : z;
}

inline void Point3I::zero()
{
   x = y = z = 0;
}

inline void Point3I::neg()
{
   x = -x;
   y = -y;
   z = -z;
}

inline F32 Point3I::len() const
{
   return mSqrt(F32(x*x + y*y + z*z));
}

inline void Point3I::convolve(const Point3I& c)
{
   x *= c.x;
   y *= c.y;
   z *= c.z;
}

inline bool Point3I::isZero() const
{
   return ((x == 0) && (y == 0) && (z == 0));
}

inline bool Point3I::operator==(const Point3I& _test) const
{
   return ((x == _test.x) && (y == _test.y) && (z == _test.z));
}

inline bool Point3I::operator!=(const Point3I& _test) const
{
   return (operator==(_test) == false);
}

inline Point3I Point3I::operator+(const Point3I& _add) const
{
   return Point3I(x + _add.x, y + _add.y, z + _add.z);
}

inline Point3I Point3I::operator-(const Point3I& _rSub) const
{
   return Point3I(x - _rSub.x, y - _rSub.y, z - _rSub.z);
}

inline Point3I& Point3I::operator+=(const Point3I& _add)
{
   x += _add.x;
   y += _add.y;
   z += _add.z;

   return *this;
}

inline Point3I& Point3I::operator-=(const Point3I& _rSub)
{
   x -= _rSub.x;
   y -= _rSub.y;
   z -= _rSub.z;

   return *this;
}

inline Point3I Point3I::operator-() const
{
   return Point3I(-x, -y, -z);
}

inline Point3I Point3I::operator*(S32 mul) const
{
   return Point3I(x * mul, y * mul, z * mul);
}

inline Point3I Point3I::operator/(S32 div) const
{
   AssertFatal(div != 0, "Error, div by zero attempted");
   return Point3I(x/div, y/div, z/div);
}

inline Point3I& Point3I::operator*=(S32 mul)
{
   x *= mul;
   y *= mul;
   z *= mul;

   return *this;
}

inline Point3I& Point3I::operator/=(S32 div)
{
   AssertFatal(div != 0, "Error, div by zero attempted");

   x /= div;
   y /= div;
   z /= div;

   return *this;
}

//------------------------------------------------------------------------------
//-------------------------------------- Point3F
//
inline Point3F::Point3F()
 : x(0.0f), y(0.0f), z(0.0f)
{
// Uninitialized points are definitely a problem.
// Enable the following code to see how often they crop up.
#ifdef DEBUG_MATH
   *(U32 *)&x = 0x7FFFFFFA;
   *(U32 *)&y = 0x7FFFFFFB;
   *(U32 *)&z = 0x7FFFFFFC;
#endif
}


inline Point3F::Point3F(const Point3F& _copy)
 : x(_copy.x), y(_copy.y), z(_copy.z)
{
   //
}

inline Point3F::Point3F(F32 _x, F32 _y, F32 _z)
 : x(_x), y(_y), z(_z)
{
   //
}

inline Point3F::Point3F(F32 xyz)
 : x(xyz), y(xyz), z(xyz)
{
   //
}

inline void Point3F::set(F32 xyz)
{
   x = y = z = xyz;
}

inline void Point3F::set(F32 _x, F32 _y, F32 _z)
{
   x = _x;
   y = _y;
   z = _z;
}

inline void Point3F::set(const Point3F& copy)
{
   x = copy.x;
   y = copy.y;
   z = copy.z;
}

inline void Point3F::setMin(const Point3F& _test)
{
   x = (_test.x < x) ? _test.x : x;
   y = (_test.y < y) ? _test.y : y;
   z = (_test.z < z) ? _test.z : z;
}

inline void Point3F::setMax(const Point3F& _test)
{
   x = (_test.x > x) ? _test.x : x;
   y = (_test.y > y) ? _test.y : y;
   z = (_test.z > z) ? _test.z : z;
}

inline void Point3F::interpolate(const Point3F& _from, const Point3F& _to, F32 _factor)
{
   AssertFatal(_factor >= 0.0f && _factor <= 1.0f, "Out of bound interpolation factor");
   m_point3F_interpolate( _from, _to, _factor, *this);
}

inline void Point3F::zero()
{
   x = y = z = 0.0f;
}

inline bool Point3F::isZero() const
{
   return ((x*x) <= POINT_EPSILON) && ((y*y) <= POINT_EPSILON) && ((z*z) <= POINT_EPSILON );
}

inline bool Point3F::isUnitLength() const
{
   return ( mFabs( 1.0f - ( x*x + y*y + z*z ) ) < POINT_EPSILON );
}

inline bool Point3F::equal( const Point3F &compare, F32 epsilon ) const
{
   return( ( mFabs( x - compare.x ) < epsilon ) &&
           ( mFabs( y - compare.y ) < epsilon ) &&
           ( mFabs( z - compare.z ) < epsilon ) );
}

inline U32 Point3F::getLeastComponentIndex() const
{
   U32 idx;

   if ( mFabs( x ) < mFabs( y ) )
   {
      if ( mFabs( x ) < mFabs( z ) )
         idx = 0;
      else
         idx = 2;
   }
   else
   {
      if ( mFabs( y ) < mFabs( z ) )
         idx = 1;  
      else
         idx = 2;
   }

   return idx;
}

inline U32 Point3F::getGreatestComponentIndex() const
{
   U32 idx;

   if ( mFabs( x ) > mFabs( y ) )
   {
      if ( mFabs( x ) > mFabs( z ) )
         idx = 0;
      else
         idx = 2;
   }
   else
   {
      if ( mFabs( y ) > mFabs( z ) )
         idx = 1;  
      else
         idx = 2;
   }

   return idx;
}

inline F32 Point3F::least() const
{
   return getMin( mFabs( x ), getMin( mFabs( y ), mFabs( z ) ) );
}

inline F32 Point3F::most() const
{
   return getMax( mFabs( x ), getMax( mFabs( y ), mFabs( z ) ) );
}

inline void Point3F::neg()
{
   x = -x;
   y = -y;
   z = -z;
}

inline void Point3F::convolve(const Point3F& c)
{
   x *= c.x;
   y *= c.y;
   z *= c.z;
}

inline void Point3F::convolveInverse(const Point3F& c)
{
   x /= c.x;
   y /= c.y;
   z /= c.z;
}

inline F32 Point3F::lenSquared() const
{
   return (x * x) + (y * y) + (z * z);
}

inline F32 Point3F::len() const
{
   return mSqrt(x*x + y*y + z*z);
}

inline void Point3F::normalize()
{
   m_point3F_normalize(*this);
}

inline F32 Point3F::magnitudeSafe() const
{
   if( isZero() )
   {
      return 0.0f;
   }
   else
   {
      return len();
   }
}

inline void Point3F::normalizeSafe()
{
   F32 vmag = magnitudeSafe();

   if( vmag > POINT_EPSILON )
   {
      *this *= F32(1.0 / vmag);
   }
}


inline void Point3F::normalize(F32 val)
{
   m_point3F_normalize_f(*this, val);
}

inline bool Point3F::operator==(const Point3F& _test) const
{
   return (x == _test.x) && (y == _test.y) && (z == _test.z);
}

inline bool Point3F::operator!=(const Point3F& _test) const
{
   return operator==(_test) == false;
}

inline Point3F Point3F::operator+(const Point3F& _add) const
{
   return Point3F(x + _add.x, y + _add.y,  z + _add.z);
}

inline Point3F Point3F::operator-(const Point3F& _rSub) const
{
   return Point3F(x - _rSub.x, y - _rSub.y, z - _rSub.z);
}

inline Point3F& Point3F::operator+=(const Point3F& _add)
{
   x += _add.x;
   y += _add.y;
   z += _add.z;

   return *this;
}

inline Point3F& Point3F::operator-=(const Point3F& _rSub)
{
   x -= _rSub.x;
   y -= _rSub.y;
   z -= _rSub.z;

   return *this;
}

inline Point3F Point3F::operator*(F32 _mul) const
{
   return Point3F(x * _mul, y * _mul, z * _mul);
}

inline Point3F Point3F::operator/(F32 _div) const
{
   AssertFatal(_div != 0.0f, "Error, div by zero attempted");

   F32 inv = 1.0f / _div;

   return Point3F(x * inv, y * inv, z * inv);
}

inline Point3F& Point3F::operator*=(F32 _mul)
{
   x *= _mul;
   y *= _mul;
   z *= _mul;

   return *this;
}

inline Point3F& Point3F::operator/=(F32 _div)
{
   AssertFatal(_div != 0.0f, "Error, div by zero attempted");

   F32 inv = 1.0f / _div;
   x *= inv;
   y *= inv;
   z *= inv;

   return *this;
}

inline Point3F Point3F::operator*(const Point3F &_vec) const
{
   return Point3F(x * _vec.x, y * _vec.y, z * _vec.z);
}

inline Point3F& Point3F::operator*=(const Point3F &_vec)
{
   x *= _vec.x;
   y *= _vec.y;
   z *= _vec.z;
   return *this;
}

inline Point3F Point3F::operator/(const Point3F &_vec) const
{
   AssertFatal(_vec.x != 0.0f && _vec.y != 0.0f && _vec.z != 0.0f, "Error, div by zero attempted");
   return Point3F(x / _vec.x, y / _vec.y, z / _vec.z);
}

inline Point3F& Point3F::operator/=(const Point3F &_vec)
{
   AssertFatal(_vec.x != 0.0f && _vec.y != 0.0f && _vec.z != 0.0f, "Error, div by zero attempted");
   x /= _vec.x;
   y /= _vec.y;
   z /= _vec.z;
   return *this;
}

inline Point3F Point3F::operator-() const
{
   return Point3F(-x, -y, -z);
}


inline Point3F& Point3F::operator=(const Point3D &_vec)
{
   x = (F32)_vec.x;
   y = (F32)_vec.y;
   z = (F32)_vec.z;
   return *this;
}

//------------------------------------------------------------------------------
//-------------------------------------- Point3D
//
inline Point3D::Point3D()
   : x(0.0), y(0.0), z(0.0)
{
   //
}

inline Point3D::Point3D(const Point3D& _copy)
 : x(_copy.x), y(_copy.y), z(_copy.z)
{
   //
}

inline Point3D::Point3D(const Point3F& _copy)
 : x(_copy.x), y(_copy.y), z(_copy.z)
{
   //
}

inline Point3D::Point3D(F64 xyz)
 : x(xyz), y(xyz), z(xyz)
{
   //
}

inline Point3D::Point3D(F64 _x, F64 _y, F64 _z)
 : x(_x), y(_y), z(_z)
{
   //
}

inline void Point3D::set( F64 xyz )
{
   x = y = z = xyz;
}

inline void Point3D::set(F64 _x, F64 _y, F64 _z)
{
   x = _x;
   y = _y;
   z = _z;
}

inline void Point3D::setMin(const Point3D& _test)
{
   x = (_test.x < x) ? _test.x : x;
   y = (_test.y < y) ? _test.y : y;
   z = (_test.z < z) ? _test.z : z;
}

inline void Point3D::setMax(const Point3D& _test)
{
   x = (_test.x > x) ? _test.x : x;
   y = (_test.y > y) ? _test.y : y;
   z = (_test.z > z) ? _test.z : z;
}

inline void Point3D::interpolate(const Point3D& _from, const Point3D& _to, F64 _factor)
{
   AssertFatal(_factor >= 0.0f && _factor <= 1.0f, "Out of bound interpolation factor");
   m_point3D_interpolate( _from, _to, _factor, *this);
}

inline void Point3D::zero()
{
   x = y = z = 0.0;
}

inline bool Point3D::isZero() const
{
   return (x == 0.0f) && (y == 0.0f) && (z == 0.0f);
}

inline void Point3D::neg()
{
   x = -x;
   y = -y;
   z = -z;
}

inline void Point3D::convolve(const Point3D& c)
{
   x *= c.x;
   y *= c.y;
   z *= c.z;
}

inline void Point3D::convolveInverse(const Point3D& c)
{
   x /= c.x;
   y /= c.y;
   z /= c.z;
}

inline F64 Point3D::lenSquared() const
{
   return (x * x) + (y * y) + (z * z);
}

inline F64 Point3D::len() const
{
   F64 temp = x*x + y*y + z*z;
   return (temp > 0.0) ? mSqrtD(temp) : 0.0;
}

inline void Point3D::normalize()
{
   m_point3D_normalize(*this);
}

inline F64 Point3D::magnitudeSafe() const
{
   if( isZero() )
   {
      return 0.0;
   }
   else
   {
      return len();
   }
}

inline void Point3D::normalizeSafe()
{
   F64 vmag = magnitudeSafe();

   if( vmag > POINT_EPSILON )
   {
      *this *= F64(1.0 / vmag);
   }
}

inline void Point3D::normalize(F64 val)
{
   m_point3D_normalize_f(*this, val);
}

inline bool Point3D::operator==(const Point3D& _test) const
{
   return (x == _test.x) && (y == _test.y) && (z == _test.z);
}

inline bool Point3D::operator!=(const Point3D& _test) const
{
   return operator==(_test) == false;
}

inline Point3D Point3D::operator+(const Point3D& _add) const
{
   return Point3D(x + _add.x, y + _add.y,  z + _add.z);
}


inline Point3D Point3D::operator-(const Point3D& _rSub) const
{
   return Point3D(x - _rSub.x, y - _rSub.y, z - _rSub.z);
}

inline Point3D& Point3D::operator+=(const Point3D& _add)
{
   x += _add.x;
   y += _add.y;
   z += _add.z;

   return *this;
}

inline Point3D& Point3D::operator-=(const Point3D& _rSub)
{
   x -= _rSub.x;
   y -= _rSub.y;
   z -= _rSub.z;

   return *this;
}

inline Point3D Point3D::operator*(F64 _mul) const
{
   return Point3D(x * _mul, y * _mul, z * _mul);
}

inline Point3D Point3D::operator/(F64 _div) const
{
   AssertFatal(_div != 0.0f, "Error, div by zero attempted");

   F64 inv = 1.0f / _div;

   return Point3D(x * inv, y * inv, z * inv);
}

inline Point3D& Point3D::operator*=(F64 _mul)
{
   x *= _mul;
   y *= _mul;
   z *= _mul;

   return *this;
}

inline Point3D& Point3D::operator/=(F64 _div)
{
   AssertFatal(_div != 0.0f, "Error, div by zero attempted");

   F64 inv = 1.0f / _div;
   x *= inv;
   y *= inv;
   z *= inv;

   return *this;
}

inline Point3D Point3D::operator-() const
{
   return Point3D(-x, -y, -z);
}

inline Point3F Point3D::toPoint3F() const
{
   return Point3F((F32)x,(F32)y,(F32)z);
}

//-------------------------------------------------------------------
// Non-Member Operators
//-------------------------------------------------------------------

inline Point3I operator*(S32 mul, const Point3I& multiplicand)
{
   return multiplicand * mul;
}

inline Point3F operator*(F32 mul, const Point3F& multiplicand)
{
   return multiplicand * mul;
}

inline Point3D operator*(F64 mul, const Point3D& multiplicand)
{
   return multiplicand * mul;
}

inline F32 mDot(const Point3F &p1, const Point3F &p2)
{
   return (p1.x*p2.x + p1.y*p2.y + p1.z*p2.z);
}

inline F64 mDot(const Point3D &p1, const Point3D &p2)
{
   return (p1.x*p2.x + p1.y*p2.y + p1.z*p2.z);
}

inline void mCross(const Point3F &a, const Point3F &b, Point3F *res)
{
   res->x = (a.y * b.z) - (a.z * b.y);
   res->y = (a.z * b.x) - (a.x * b.z);
   res->z = (a.x * b.y) - (a.y * b.x);
}

inline void mCross(const Point3D &a, const Point3D &b, Point3D *res)
{
   res->x = (a.y * b.z) - (a.z * b.y);
   res->y = (a.z * b.x) - (a.x * b.z);
   res->z = (a.x * b.y) - (a.y * b.x);
}

inline Point3F mCross(const Point3F &a, const Point3F &b)
{
   Point3F r;
   mCross( a, b, &r );
   return r;
}

inline Point3D mCross(const Point3D &a, const Point3D &b)
{
   Point3D r;
   mCross( a, b, &r );
   return r;
}

/// Returns the vector normalized.
inline Point3F mNormalize( const Point3F &vec )
{
   Point3F out( vec );
   out.normalize();
   return out;
}

/// Returns true if the point is NaN.
inline bool mIsNaN( const Point3F &p )
{
   return mIsNaN_F( p.x ) || mIsNaN_F( p.y ) || mIsNaN_F( p.z );
}

/// Returns a copy of the vector reflected by a normal
inline Point3F mReflect( const Point3F &v, const Point3F &n )
{
   return v - 2 * n * mDot( v, n );
}

/// Returns a perpendicular vector to the unit length input vector.
extern Point3F mPerp( const Point3F &normal );


//------------------------------------------------------------------------------
// Templated Point class.
//------------------------------------------------------------------------------
#pragma warning(push) // nameless struct warning.
#pragma warning(disable: 4201)
template<typename DATA_TYPE, U32 size>
class PointT {
public:

   // union for conformity with previous classes accessing xyzw,
   // when this class replaces the old points, remove this and
   // access dimensions with x y z w functions below. using this
   // union means everything has the same size as a point4

   union {
      struct {
         DATA_TYPE x;
         DATA_TYPE y;
         DATA_TYPE z;
         DATA_TYPE w;
      };

      DATA_TYPE data[size];
   };
   

   static_assert(size >= 2, "PointT must have a size of at least 2, otherwise just use scalar.");

   // ------ Setters and initializers ------
   explicit PointT() {
      std::fill(data, data + size, DATA_TYPE(0));
   }

   explicit PointT(DATA_TYPE scalar) {
      std::fill(data, data + size, DATA_TYPE(scalar));
   }

   // Copy constructor
   PointT(const PointT& p){
      for (U32 i = 0; i < size; i++)
      {
         data[i] = p.data[i];
      }
   }

   template<typename... Args>
   PointT(Args... args) {
      AssertFatal(sizeof...(args) <= size, "Number of arguments must not exceed the size of the point.");
      DATA_TYPE values[] = { static_cast<DATA_TYPE>(args)... };

      U32 count = mMin(sizeof...(args), size);
      for (U32 i = 0; i < count; i++)
      {
         data[i] = values[i];
      }

      if (size > count)
      {
         std::fill(data + count, data + size, DATA_TYPE(0));
      }

   }

   template<U32 bSize>
   explicit PointT(const PointT<DATA_TYPE, bSize>& p) {
      AssertFatal(bSize >= 2, "Cannot initialize with a point with size less than 2.");

      U32 min_size = mMin(size, bSize);
      for (U32 i = 0; i < min_size; i++)
      {
         data[i] = p.data[i];
      }

      if (size > bSize) {
         std::fill(data + bSize, data + size, DATA_TYPE(0));
      }
      
   }

   // following set functions are pretty much the same as the above constructors.
   void set(DATA_TYPE scalar) {
      std::fill(data, data + size, DATA_TYPE(scalar));
   }

   template<typename... Args>
   void set(Args... args) {
      AssertFatal(sizeof...(args) <= size, "Number of arguments must not exceed the size of the point.");
      DATA_TYPE values[] = { static_cast<DATA_TYPE>(args)... };

      U32 count = mMin(sizeof...(args), size);
      for (U32 i = 0; i < count; i++)
      {
         data[i] = values[i];
      }

      if (size > count)
      {
         std::fill(data + count, data + size, DATA_TYPE(0));
      }
   }

   void set(const PointT& p) {
      for (U32 i = 0; i < size; i++)
      {
         data[i] = p.data[i];
      }
   }

   template<U32 bSize>
   void set(const PointT<DATA_TYPE, bSize>& p) {
      AssertFatal(bSize >= 2, "Cannot initialize with a point with size less than 2.");

      U32 min_size = mMin(size, bSize);
      for (U32 i = 0; i < min_size; i++)
      {
         data[i] = p.data[i];
      }

      if (size > bSize) {
         std::fill(data + bSize, data + size, DATA_TYPE(0));
      }
   }

   void zero() {
      std::fill(data, data + size, DATA_TYPE(0));
   }

   template<U32 bSize>
   void setMin(const PointT<DATA_TYPE, bSize>& p) {
      AssertFatal(bSize >= 2, "Cannot initialize with a point with size less than 2.");

      U32 min_size = mMin(size, bSize);
      for (U32 i = 0; i < min_size; i++)
      {
         data[i] = p.data[i] < data[i] ? p.data[i] : data[i];
      }
   }

   template<U32 bSize>
   void setMax(const PointT<DATA_TYPE, bSize>& p) {
      AssertFatal(bSize >= 2, "Cannot initialize with a point with size less than 2.");

      U32 min_size = mMin(size, bSize);
      for (U32 i = 0; i < min_size; i++)
      {
         data[i] = p.data[i] > data[i] ? p.data[i] : data[i];
      }
   }

   // Mathematical mutators

   void interpolate(const PointT& _from, const PointT& _to, DATA_TYPE _factor) {
      // should maybe check _factor is a float type?
      DATA_TYPE inverse = DATA_TYPE(1) - _factor;
      for (U32 i = 0; i < size; i++)
      {
         data[i] = inverse * _from[i] + _factor * _to[i];
      }
   }

   void neg() {
      for (U32 i = 0; i < size; i++) {
         data[i] = -data[i];
      }
   }

   void normalize() {
      DATA_TYPE fac = DATA_TYPE(1) / std::sqrt(lenSquared());
      *this *= fac;
   }

   void normalizeSafe() {
      DATA_TYPE mag = magnitudeSafe();

      if (mag > std::numeric_limits<DATA_TYPE>::epsilon())
      {
         DATA_TYPE fac = DATA_TYPE(1) / mag;
         *this *= fac;
      }

   }

   void normalize(DATA_TYPE val) {
      DATA_TYPE fac = val / std::sqrt(lenSquared());
      *this *= fac;
   }

   void convolve(const PointT& rhs) {
      for (U32 i = 0; i < size; i++) {
         data[i] *= rhs.data[i];
      }
   }

   void convolveInverse(const PointT& rhs) {
      for (U32 i = 0; i < size; i++) {
         data[i] /= rhs.data[i];
      }
   }

   // ------ Getters ------

   bool isZero() const {
      for (U32 i = 0; i < size; i++)
      {
         if (data[i] != DATA_TYPE(0))
            return false;
      }

      return true;
   }

   bool isUnitLength() const {
      return (std::abs(DATA_TYPE(1) - lenSquared()) < std::numeric_limits<DATA_TYPE>::epsilon());
   }

   DATA_TYPE len() const {
      DATA_TYPE temp = lenSquared();

      return temp > DATA_TYPE(0) ? std::sqrt(temp) : DATA_TYPE(0);
   }

   DATA_TYPE lenSquared() const {
      DATA_TYPE val = DATA_TYPE();

      for(U32 i = 0; i < size; i++)
      {
         val += data[i] * data[i];
      }
      return val;
   }

   DATA_TYPE magnitudeSafe() const
   {
      if (isZero())
      {
         return DATA_TYPE(0);
      }
      else
      {
         return len();
      }
   }

   bool equal(const PointT& compare, DATA_TYPE epsilon = std::numeric_limits<DATA_TYPE>::epsilon()) const {
      for (U32 i = 0; i < size; i++)
      {
         if (std::abs(data[i] - compare.data[i]) > epsilon)
            return false;
      }

      return true;
   }


   U32 getLeastComponentIndex() const {
      U32 leastId = 0;
      DATA_TYPE least_val = data[0];

      for (U32 i = 0; i < size; i++)
      {
         if (data[i] < least_val)
         {
            least_val = data[i];
            leastId = i;
         }
      }

      return leastId;
   }

   U32 getGreatestComponentIndex() const {
      U32 maxId = 0;
      DATA_TYPE max_val = data[0];

      for (U32 i = 0; i < size; i++)
      {
         if (data[i] > max_val)
         {
            max_val = data[i];
            maxId = i;
         }
      }

      return maxId;
   }

   DATA_TYPE least() const {
      DATA_TYPE least_val = data[0];

      for (U32 i = 0; i < size; i++)
      {
         if (data[i] < least_val)
         {
            least_val = data[i];
         }
      }

      return least_val;
   }

   DATA_TYPE most() const {
      DATA_TYPE max_val = data[0];

      for (U32 i = 0; i < size; i++)
      {
         if (data[i] > max_val)
         {
            max_val = data[i];
         }
      }

      return max_val;
   }

   // Vector functions not found in original point

   DATA_TYPE dot(const PointT& rhs) const {
      DATA_TYPE res = DATA_TYPE(0);

      for (U32 i = 0; i < size; i++)
      {
         res += data[i] * rhs.data[i];
      }

      return res;
   }

   PointT cross(const PointT& rhs) const {
      AssertFatal(size == 3, "Cross product is only for 3d points");

      return PointT(
         data[1] * rhs.data[2] - data[2] * rhs.data[1],
         data[2] * rhs.data[0] - data[0] * rhs.data[2],
         data[0] * rhs.data[1] - data[1] * rhs.data[0]
      );
   }

   DATA_TYPE distance(const PointT& rhs) const {
      DATA_TYPE sum = DATA_TYPE(0);
      for (U32 i = 0; i < size; i++) {
         DATA_TYPE diff = data[i] - rhs.data[i];
         sum += diff * diff;
      }
      return std::sqrt(sum);
   }

   PointT reflect(const PointT& normal) {
      AssertFatal(size >= 2, "Reflect requires at least 2 dimensions");

      DATA_TYPE dot = this->dot(normal);

      PointT res;
      for (U32 i = 0; i < size; i++)
      {
         res.data[i] = data[i] - 2 * dot * normal.data[i];
      }

      return res;
   }

   DATA_TYPE angle(const PointT& rhs) const {
      DATA_TYPE dotProd = dot(rhs);
      DATA_TYPE mags = magnitudeSafe() * rhs.magnitudeSafe();

      return std::acos(dotProd / mags);
   }

   PointT projectTo(const PointT& rhs) const {
      DATA_TYPE scalar = dot(rhs) / rhs.dot(rhs);
   }

   PointT clampMagnitude(DATA_TYPE maxMagnitude) const {
      DATA_TYPE mag = magnitudeSafe();

      if (mag > maxMagnitude)
      {
         return normalize() * maxMagnitude;
      }

      return *this;
   }
   
   // ------ Operators ------
   // for now all operators expect same size points for rhs
   // Comparison operators
   bool operator==(const PointT& rhs) const {
      for (U32 i = 0; i < size; i++)
      {
         if (data[i] != rhs.data[i])
            return false;
      }

      return true;
   }
   bool operator!=(const PointT& rhs) const {
      return !(*this == rhs);
   }

   // Arithmetic w/ other points
   PointT operator+(const PointT& rhs) const {
      PointT result;
      for (U32 i = 0; i < size; i++)
      {
         result.data[i] = data[i] + rhs.data[i];
      }
      return result;
   }

   PointT operator-(const PointT& rhs) const {
      PointT result;
      for (U32 i = 0; i < size; i++)
      {
         result.data[i] = data[i] - rhs.data[i];
      }
      return result;
   }

   PointT& operator+=(const PointT& rhs) {
      for (U32 i = 0; i < size; i++)
      {
         data[i] += rhs.data[i];
      }
      return *this;

   }

   PointT& operator-=(const PointT& rhs) {
      for (U32 i = 0; i < size; i++)
      {
         data[i] -= rhs.data[i];
      }
      return *this;

   }

   // Arithmetic w/ scalars
   PointT operator*(F32 scalar) const {
      PointT result;
      for (U32 i = 0; i < size; i++)
      {
         result.data[i] = data[i] * scalar;
      }
      return result;
   }

   PointT operator/(F32 scalar) const {
      AssertFatal(scalar != DATA_TYPE(0), "Error, div by zero attempted");

      PointT result;
      for (U32 i = 0; i < size; i++)
      {
         result.data[i] = data[i] / scalar;
      }
      return result;
   }

   PointT& operator*=(F32 scalar) {
      for (U32 i = 0; i < size; i++)
      {
         data[i] *= scalar;
      }
      return *this;

   }
   PointT& operator/=(F32 scalar) {
      AssertFatal(scalar != DATA_TYPE(0), "Error, div by zero attempted");
      for (U32 i = 0; i < size; i++)
      {
         data[i] /= scalar;
      }
      return *this;

   }

   PointT operator*(const PointT& rhs) const {
      PointT result;
      for (U32 i = 0; i < size; i++)
      {
         result.data[i] = data[i] * rhs.data[i];
      }
      return result;
   }

   PointT& operator*=(const PointT& rhs) {
      for (U32 i = 0; i < size; i++)
      {
         data[i] *= rhs.data[i];
      }
      return *this;

   }

   PointT operator/(const PointT& rhs) const {
      PointT result;
      for (U32 i = 0; i < size; i++)
      {
         if (data[i] == DATA_TYPE(0) || rhs.data[i] == DATA_TYPE(0))
            continue;

         result.data[i] = data[i] / rhs.data[i];
      }
      return result;
   }

   PointT& operator/=(const PointT& rhs) {
      for (U32 i = 0; i < size; i++)
      {
         if (data[i] == DATA_TYPE(0) || rhs.data[i] == DATA_TYPE(0))
            continue;

         data[i] /= rhs.data[i];
      }

      return *this;
   }

   // Unary operators
   PointT operator-() const {
      PointT result;
      for (std::size_t i = 0; i < size; ++i) {
         result.data[i] = -data[i];
      }
      return result;
   }

   // same size but different type.
   template<typename O_TYPE>
   PointT& operator=(const PointT<O_TYPE, size>& rhs) {
      for (U32 i = 0; i < size; i++)
      {
         data[i] = static_cast<DATA_TYPE>(rhs.data[i]);
      }
   }

   operator DATA_TYPE* () { return (data); }
   operator const DATA_TYPE* () const { return (DATA_TYPE*)(data); }

   // Uncomment and replace union when template replaces other classes.
   //// Accessors for x, y, z, w components, putting these in operators.
   //DATA_TYPE& x() {
   //   // not sure if needed since we have a hard set minimum of 2.
   //   AssertFatal(size >= 2, "No X component in point.");
   //   return data[0];
   //}

   //const DATA_TYPE& x() const {
   //   // not sure if needed since we have a hard set minimum of 2.
   //   AssertFatal(size >= 2, "No X component in point.");
   //   return data[0];
   //}

   //DATA_TYPE& y() {
   //   // not sure if needed since we have a hard set minimum of 2.
   //   AssertFatal(size >= 2, "No Y component in point.");
   //   return data[1];
   //}

   //const DATA_TYPE& y() const {
   //   // not sure if needed since we have a hard set minimum of 2.
   //   AssertFatal(size >= 2, "No Y component in point.");
   //   return data[1];
   //}

   //DATA_TYPE& z() {
   //   AssertFatal(size >= 3, "No Z component in point.");
   //   return data[2];
   //}

   //const DATA_TYPE& z() const {
   //   AssertFatal(size >= 3, "No Z component in point.");
   //   return data[2];
   //}

   //DATA_TYPE& w() {
   //   AssertFatal(size >= 4, "No W component in point.");
   //   return data[3];
   //}

   //const DATA_TYPE& w() const {
   //   AssertFatal(size >= 4, "No W component in point.");
   //   return data[3];
   //}
   
   // ------ Static constants ------
   const static PointT One;
   const static PointT Zero;
   const static PointT Max;
   const static PointT Min;
   const static PointT UnitX;
   const static PointT UnitY;
   const static PointT UnitZ;
};

#pragma warning(pop)


#endif // _MPOINT3_H_
