// Copyright (c) 2026 WrenchSoft Ltd.
//
// This file is not licensed under the MIT License.
//
// Permission is granted to use, copy, modify, and distribute this file solely
// as part of the official TorqueGameEngines/Torque3D source repository and derivative
// works of that repository.
//
// No permission is granted to copy, use, distribute, sublicense, or incorporate
// this file independently or as part of any other software project without
// prior written permission from WrenchSoft Ltd.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

#pragma once

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

namespace KorkScript
{
   class KorkArena
   {
   public:
      // Initial capacity in bytes, doubles each time it is exhausted.
      static constexpr U32 kInitCapacity = 65536; // 64kb

      KorkArena()
         : mBuffer(nullptr),
         mCapacity(0),
         mUsed(0)
      {
         grow(kInitCapacity);
      }

      ~KorkArena()
      {
         dFree(mBuffer);
      }

      // Non-copyable — owns the buffer.
      KorkArena(const KorkArena&) = delete;
      KorkArena& operator=(const KorkArena&) = delete;

      void* alloc(U32 size)
      {
         U32 aligned = (size + 7u) & ~7u;

         if(mUsed + aligned > mCapacity)
         {
            grow(mCapacity * 2 > mUsed + aligned ? mCapacity * 2 : mUsed + aligned + kInitCapacity);
         }

         void* ptr = mBuffer + mUsed;
         mUsed += aligned;
         return ptr;
      }

      void* allocZeroed(U32 size)
      {
         void* ptr = alloc(size);
         dMemset(ptr, 0, size);
         return ptr;
      }

      const char* allocString(const char* str, U32 len)
      {
         char* dst = (char*)alloc(len + 1);
         dMemcpy(dst, str, len);
         dst[len] = '\0';
         return dst;
      }

      const char* allocString(const char* str)
      {
         U32 len = 0;
         while (str[len]) ++len;
         return allocString(str, len);
      }

      template<typename T, typename... Args>
      T* construct(Args&&... args)
      {
         void* mem = allocZeroed(sizeof(T));
         return new (mem) T(std::forward<Args>(args)...);
      }

      void reset()
      {
         mUsed = 0;
      }

      U32 used()     const { return mUsed; }
      U32 capacity() const { return mCapacity; }

   private:
      unsigned char* mBuffer;
      U32 mCapacity;
      U32 mUsed;

      void grow(U32 newCapacity)
      {
         AssertFatal(newCapacity > mCapacity, "KorkArena::grow - Buffer capacity trying to shrink.");
         unsigned char* newbuf = (unsigned char*)dRealloc(mBuffer, newCapacity);
         mBuffer = newbuf;
         mCapacity = newCapacity;
      }
   };
}
