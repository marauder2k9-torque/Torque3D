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

#ifndef _TORQUE_PLATFORM_PLATFORMINTRINSICS_GCC_H_
#define _TORQUE_PLATFORM_PLATFORMINTRINSICS_GCC_H_

/// @file
/// Compiler intrinsics for GCC and Clang.

// Fetch-And-Add
//
// NOTE: These do not return the pre-add value because not all platforms
// (historically, OSX's OSAtomic API) could do that — kept as void to
// preserve the existing call-site contract even though the underlying
// __atomic_fetch_add does have the old value available.
//
inline void dFetchAndAdd(volatile U32& ref, U32 val)
{
   __atomic_fetch_add(&ref, val, __ATOMIC_SEQ_CST);
}

inline void dFetchAndAdd(volatile S32& ref, S32 val)
{
   __atomic_fetch_add(&ref, val, __ATOMIC_SEQ_CST);
}

// Compare-And-Swap

inline bool dCompareAndSwap(volatile U32& ref, U32 oldVal, U32 newVal)
{
   return __atomic_compare_exchange_n(&ref, &oldVal, newVal, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

inline bool dCompareAndSwap(volatile U64& ref, U64 oldVal, U64 newVal)
{
   return __atomic_compare_exchange_n(&ref, &oldVal, newVal, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

/// Performs an atomic read operation.
inline U32 dAtomicRead(volatile U32& ref)
{
   return __atomic_load_n(&ref, __ATOMIC_SEQ_CST);
}

#endif // _TORQUE_PLATFORM_PLATFORMINTRINSICS_GCC_H_
