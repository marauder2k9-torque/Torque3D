#pragma once

#ifndef _GFXDRAWBATCH_H_
#define _GFXDRAWBATCH_H_

#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif

class GFXDrawBatch
{
   struct DrawBatch
   {
      GFXTextureObject* texture;
      GFXPrimitiveType primitiveType;
      U32 vertStart;
      U32 vertCount;
      U32 indexStart;
      U32 indexCount;
   };

public:
   GFXDrawBatch();
   ~GFXDrawBatch();

   void render();

private:
   Vector<GFXVertexPCT> mVerts;
   Vector<U32> mIndices;
   Vector<DrawBatch> mBatches;

   GFXVertexBufferHandle<GFXVertexPCT> mVertexBuffer;
   GFXPrimitiveBufferHandle mPrimitiveBuffer;
};

#endif // _GFXDRAWBATCH_H_
