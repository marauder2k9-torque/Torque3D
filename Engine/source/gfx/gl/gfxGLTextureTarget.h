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

#ifndef _GFXGLTEXTURETARGET_H_
#define _GFXGLTEXTURETARGET_H_

#include "gfx/gfxTarget.h"
#ifndef _GFXGLTEXTUREOBJECT_H
#include "gfx/gl/gfxGLTextureObject.h"
#endif
#include "core/util/autoPtr.h"

class _GFXGLTargetDesc;
class _GFXGLTextureTargetImpl;

/// Render to texture support for OpenGL.
/// This class needs to make a number of assumptions due to the requirements
/// and complexity of render to texture in OpenGL.
/// 1) This class is only guaranteed to work with 2D textures or cubemaps.  3D textures
/// may or may not work.
/// 2) This class does not currently support multiple texture targets.  Regardless
/// of how many targets you bind, only Color0 will be used.
/// 3) This class requires that the DepthStencil and Color0 targets have identical
/// dimensions.
/// 4) If the DepthStencil target is GFXTextureTarget::sDefaultStencil, then the
/// Color0 target should be the same size as the current backbuffer and should also
/// be the same format (typically R8G8B8A8)
class GFXGLTextureTarget : public GFXTextureTarget
{
public:
   GFXGLTextureTarget(bool genMips);
   virtual ~GFXGLTextureTarget();

   const Point2I getSize() override { return mTargetSize; }
   GFXFormat getFormat() override { return mTargetFormat; }
   void attachTexture(RenderSlot slot, GFXTextureObject *tex, U32 mipLevel=0, U32 zOffset = 0, U32 faceIndex = 0) override;
   void attachTexture(RenderSlot slot, GFXCubemap *tex, U32 face, U32 mipLevel=0) override;

   void activate() override;
   void deactivate() override;
   void zombify() override;
   void resurrect() override;
   
   void resolve() override;
   void resolveTo(GFXTextureObject* obj) override;
   
protected:

   friend class GFXGLDevice;

   GLuint mFBO;
   GLuint mCopyFboSrc;
   GLuint mCopyFboDst;
   GLuint mTargets[MaxRenderSlotId];       // OpenGL texture IDs
   GLuint mTargetFace[MaxRenderSlotId];    // Face index for cubemaps
   bool mTargetIsCube[MaxRenderSlotId];
   Point2I mTargetSize;
   GFXFormat mTargetFormat;
   bool mGenMips;

};

#endif
