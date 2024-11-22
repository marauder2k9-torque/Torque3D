#include "platform/platform.h"
#include "gfx/gfxDrawBatch.h"

GFXDrawBatch::GFXDrawBatch()
{
}

GFXDrawBatch::~GFXDrawBatch()
{
}

void GFXDrawBatch::render()
{
   // Ensure we have something to render
   if (mVerts.empty() || mIndices.empty() || mBatches.empty())
      return;

   mVertexBuffer.set(GFX, mVerts.size(), GFXBufferTypeVolatile);
   mPrimitiveBuffer.set(GFX, mIndices.size(), mIndices.size() / 3, GFXBufferTypeVolatile);

   mVertexBuffer.lock();
   mPrimitiveBuffer.lock(NULL);

   dMemcpy(mVertexBuffer.getPointer(), mVerts.address(), mVerts.size() * sizeof(GFXVertexPCT));
   dMemcpy(mPrimitiveBuffer.getPointer(), mIndices.address(), mIndices.size() * sizeof(U32));

   mVertexBuffer.unlock();
   mPrimitiveBuffer.unlock();

   GFX->setVertexBuffer(mVertexBuffer);
   GFX->setPrimitiveBuffer(mPrimitiveBuffer);

   for (const DrawBatch& batch : mBatches)
   {
      GFX->setTexture(0, batch.texture);
      GFX->drawIndexedPrimitive(batch.primitiveType, batch.vertStart, 0, batch.vertCount, batch.indexStart, batch.indexCount / 3);
   }
}
