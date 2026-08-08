//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiRenderBatch.cpp
//-----------------------------------------------------------------------------
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gfx/gfxEnums.h"
#include <algorithm>   // std::stable_sort

// Every batched primitive here (quad, textured quad, glyph) is 6 raw,
// unindexed vertices (two triangles, no shared indexing - see
// _writeQuadVerts()'s own comment).
static const U32 kMaxItemsPerChunk = GFX_MAX_DYNAMIC_VERTS / 6;

NewGuiRenderBatch::NewGuiRenderBatch()
   : mStateBlocksReady(false)
{
}

NewGuiRenderBatch::~NewGuiRenderBatch()
{
}

void NewGuiRenderBatch::_ensureStateBlocks(GFXDevice* device)
{
   if (mStateBlocksReady)
      return;

   // Untextured solid fill.
   GFXStateBlockDesc solidDesc;
   solidDesc.setCullMode(GFXCullNone);
   solidDesc.setZReadWrite(false);
   solidDesc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
   mSolidSB = device->createStateBlock(solidDesc);

   // Textured glyph sheets - point-filtered, clamped, alpha-blended, no color-channel alpha write.
   GFXStateBlockDesc textDesc;
   textDesc.zDefined = true;
   textDesc.zEnable = false;
   textDesc.zWriteEnable = false;
   textDesc.cullDefined = true;
   textDesc.cullMode = GFXCullNone;
   textDesc.blendDefined = true;
   textDesc.blendEnable = true;
   textDesc.blendSrc = GFXBlendSrcAlpha;
   textDesc.blendDest = GFXBlendInvSrcAlpha;
   textDesc.samplersDefined = true;
   textDesc.samplers[0].magFilter = GFXTextureFilterPoint;
   textDesc.samplers[0].minFilter = GFXTextureFilterPoint;
   textDesc.samplers[0].mipFilter = GFXTextureFilterPoint;
   textDesc.samplers[0].addressModeU = GFXAddressClamp;
   textDesc.samplers[0].addressModeV = GFXAddressClamp;
   textDesc.setColorWrites(true, true, true, false);
   mTextSB = device->createStateBlock(textDesc);

   mStateBlocksReady = true;
}

// Textured icons/images/skin parts - bilinear filtered, alpha-blended, addressing per the
// requested (addressU, addressV) pair. mTextureSBCache entries persist for the life of the
// batch (not just one frame), same lifetime as mSolidSB/mTextSB, since the small fixed set of
// address-mode combinations a running app actually uses stabilizes almost immediately.
GFXStateBlockRef NewGuiRenderBatch::_getTextureStateBlock(GFXDevice* device, GFXTextureAddressMode addressU, GFXTextureAddressMode addressV)
{
   const TextureSBKey key{ addressU, addressV };

   for (U32 i = 0; i < mTextureSBCache.size(); i++)
   {
      if (mTextureSBCache[i].key == key)
         return mTextureSBCache[i].sb;
   }

   GFXStateBlockDesc textureDesc;
   textureDesc.zDefined = true;
   textureDesc.zEnable = false;
   textureDesc.zWriteEnable = false;
   textureDesc.cullDefined = true;
   textureDesc.cullMode = GFXCullNone;
   textureDesc.blendDefined = true;
   textureDesc.blendEnable = true;
   textureDesc.blendSrc = GFXBlendSrcAlpha;
   textureDesc.blendDest = GFXBlendInvSrcAlpha;
   textureDesc.samplersDefined = true;
   textureDesc.samplers[0].magFilter = GFXTextureFilterPoint;
   textureDesc.samplers[0].minFilter = GFXTextureFilterPoint;
   textureDesc.samplers[0].mipFilter = GFXTextureFilterPoint;
   textureDesc.samplers[0].addressModeU = addressU;
   textureDesc.samplers[0].addressModeV = addressV;

   TextureSBEntry entry;
   entry.key = key;
   entry.sb = device->createStateBlock(textureDesc);
   mTextureSBCache.push_back(entry);

   return entry.sb;
}

void NewGuiRenderBatch::begin(const RectI& deviceViewport)
{
   mQuads.clear();
   mTextRuns.clear();
   mTexturedRuns.clear();

   mClipStack.clear();
   mClipStack.push_back(deviceViewport);
}

void NewGuiRenderBatch::pushClipRect(const RectI& deviceRect)
{
   RectI narrowed = deviceRect;
   if (!narrowed.intersect(mClipStack.last()))
      narrowed = RectI(mClipStack.last().point, Point2I(0, 0));   // No overlap with the current clip.

   mClipStack.push_back(narrowed);
}

void NewGuiRenderBatch::popClipRect()
{
   AssertFatal(mClipStack.size() > 1, "NewGuiRenderBatch::popClipRect() - unbalanced pop");

   if (mClipStack.size() > 1)
      mClipStack.pop_back();
}

void NewGuiRenderBatch::pushQuad(const RectI& deviceRect, const ColorI& color, S32 layer)
{
   NewGuiBatchQuad q;

   const F32 left = (F32)deviceRect.point.x;
   const F32 top = (F32)deviceRect.point.y;
   const F32 right = (F32)(deviceRect.point.x + deviceRect.extent.x);
   const F32 bottom = (F32)(deviceRect.point.y + deviceRect.extent.y);

   q.p0.set(left, top);
   q.p1.set(right, top);
   q.p2.set(right, bottom);
   q.p3.set(left, bottom);
   q.color = color;
   q.layer = layer;
   q.clip = mClipStack.last();

   mQuads.push_back(q);
}

void NewGuiRenderBatch::pushLine(const Point2I& start, const Point2I& end, const ColorI& color, F32 thickness, S32 layer)
{
   // Submitted as a thin quad so it shares a draw call with pushQuad() output.
   Point2F a((F32)start.x, (F32)start.y);
   Point2F b((F32)end.x, (F32)end.y);

   Point2F dir = b - a;
   F32 len = dir.len();

   Point2F perp;
   if (len > 0.0001f)
   {
      dir *= (1.0f / len);
      perp = dir.getPerpendicular();
   }
   else
   {
      perp.set(1.0f, 0.0f);   // Zero-length segment - draw a small dot rather than dropping it.
   }

   const F32 half = thickness * 0.5f;
   Point2F offset(perp.x * half, perp.y * half);

   NewGuiBatchQuad q;
   q.p0 = a - offset;
   q.p1 = b - offset;
   q.p2 = b + offset;
   q.p3 = a + offset;
   q.color = color;
   q.layer = layer;
   q.clip = mClipStack.last();

   mQuads.push_back(q);
}

void NewGuiRenderBatch::pushText(const Resource<GFont>& font, const Point2I& basePos, const char* text, const ColorI& color, S32 letterSpacing, S32 wordSpacing, S32 layer)
{
   if (font == NULL || !text || !text[0])
      return;

   NewGuiBatchTextRun run;
   run.font = font;
   run.basePos = basePos;
   run.color = color;
   run.text = text;
   run.letterSpacing = letterSpacing;
   run.wordSpacing = wordSpacing;
   run.layer = layer;
   run.clip = mClipStack.last();

   // Decode UTF-8 to UTF-16 once here; _flushText() walks the decoded buffer.
   const U32 byteLen = dStrlen(text);
   run.utf16Text.setSize(byteLen + 1);
   convertUTF8toUTF16N(text, run.utf16Text.address(), byteLen + 1);

   mTextRuns.push_back(run);
}

void NewGuiRenderBatch::pushTextRun(const Resource<GFont>& font, const Point2I& basePos, const UTF16* chars, U32 charCount, const ColorI& color, S32 letterSpacing, S32 wordSpacing, S32 layer)
{
   if (font == NULL || !chars || charCount == 0)
      return;

   NewGuiBatchTextRun run;
   run.font = font;
   run.basePos = basePos;
   run.color = color;
   run.letterSpacing = letterSpacing;
   run.wordSpacing = wordSpacing;
   run.layer = layer;
   run.clip = mClipStack.last();

   run.utf16Text.setSize(charCount + 1);
   dMemcpy(run.utf16Text.address(), chars, charCount * sizeof(UTF16));
   run.utf16Text[charCount] = 0;

   mTextRuns.push_back(run);
}

void NewGuiRenderBatch::pushTexturedQuad(const RectI& deviceRect, GFXTexHandle texture, const Point2F& texelLower, const Point2F& texelUpper, const Point2I& framePixelSize, const ColorI& color, S32 layer, GFXTextureAddressMode addressModeU, GFXTextureAddressMode addressModeV)
{
   if (texture.isNull())
      return;

   NewGuiBatchTextureDraw draw;
   draw.deviceRect = deviceRect;
   draw.texture = texture;
   draw.texelLower = texelLower;
   draw.texelUpper = texelUpper;
   draw.framePixelSize = framePixelSize;
   draw.color = color;
   draw.addressModeU = addressModeU;
   draw.addressModeV = addressModeV;
   draw.layer = layer;
   draw.clip = mClipStack.last();

   mTexturedRuns.push_back(draw);
}

// Two triangles, six independent vertices per quad (no shared indexing across quads).
void NewGuiRenderBatch::_writeQuadVerts(GFXVertexBufferHandle<GFXVertexPCT>& verts, U32 startIndex, const NewGuiBatchQuad& quad) const
{
   GFXVertexColor vcolor = quad.color;

   verts[startIndex + 0].point.set(quad.p0.x, quad.p0.y, 0.f);
   verts[startIndex + 1].point.set(quad.p1.x, quad.p1.y, 0.f);
   verts[startIndex + 2].point.set(quad.p2.x, quad.p2.y, 0.f);

   verts[startIndex + 3].point.set(quad.p2.x, quad.p2.y, 0.f);
   verts[startIndex + 4].point.set(quad.p3.x, quad.p3.y, 0.f);
   verts[startIndex + 5].point.set(quad.p0.x, quad.p0.y, 0.f);

   for (U32 i = 0; i < 6; i++)
   {
      verts[startIndex + i].color = vcolor;
      verts[startIndex + i].texCoord.set(0.0f, 0.0f);
   }
}

void NewGuiRenderBatch::_flushQuads(GFXDevice* device, U32 startIndex, U32 endIndex)
{
   if (startIndex >= endIndex)
      return;

   // Sub-batch against kMaxItemsPerChunk - see this file's top-of-file comment.
   for (U32 chunkStart = startIndex; chunkStart < endIndex; chunkStart += kMaxItemsPerChunk)
   {
      const U32 chunkEnd = getMin(endIndex, chunkStart + kMaxItemsPerChunk);
      const U32 count = chunkEnd - chunkStart;
      const U32 vertCount = count * 6;

      GFXVertexBufferHandle<GFXVertexPCT> verts(device, vertCount, GFXBufferTypeVolatile);
      verts.lock();

      // Clip is applied here, in CPU-side geometry - exact for axis-aligned quads against an axis-aligned clip.
      for (U32 i = 0; i < count; i++)
      {
         NewGuiBatchQuad clipped = mQuads[chunkStart + i];

         const F32 clipLeft = (F32)clipped.clip.point.x;
         const F32 clipTop = (F32)clipped.clip.point.y;
         const F32 clipRight = (F32)(clipped.clip.point.x + clipped.clip.extent.x);
         const F32 clipBottom = (F32)(clipped.clip.point.y + clipped.clip.extent.y);

         clipped.p0.x = mClampF(clipped.p0.x, clipLeft, clipRight);
         clipped.p0.y = mClampF(clipped.p0.y, clipTop, clipBottom);
         clipped.p1.x = mClampF(clipped.p1.x, clipLeft, clipRight);
         clipped.p1.y = mClampF(clipped.p1.y, clipTop, clipBottom);
         clipped.p2.x = mClampF(clipped.p2.x, clipLeft, clipRight);
         clipped.p2.y = mClampF(clipped.p2.y, clipTop, clipBottom);
         clipped.p3.x = mClampF(clipped.p3.x, clipLeft, clipRight);
         clipped.p3.y = mClampF(clipped.p3.y, clipTop, clipBottom);

         _writeQuadVerts(verts, i * 6, clipped);
      }

      verts.unlock();

      device->setVertexBuffer(verts);
      device->setStateBlock(mSolidSB);
      device->setupGenericShaders();

      device->drawPrimitive(GFXTriangleList, 0, count * 2);
   }
}

// Groups by (texture, addressModeU, addressModeV) - draws against a different texture or a
// different sampler addressing mode can't share one drawPrimitive() call.
void NewGuiRenderBatch::_flushTextured(GFXDevice* device, U32 startIndex, U32 endIndex)
{
   if (startIndex >= endIndex)
      return;

   // Sub-batch against kMaxItemsPerChunk - see this file's top-of-file comment. Grouping
   // (by texture/addressing) is recomputed fresh per chunk - it's entirely local to the
   // [chunkStart, chunkEnd) range already, so this is just running the same per-call logic
   // over a smaller slice each time, not a structural change to how grouping works.
   for (U32 chunkStart = startIndex; chunkStart < endIndex; chunkStart += kMaxItemsPerChunk)
   {
      const U32 chunkEnd = getMin(endIndex, chunkStart + kMaxItemsPerChunk);
      const U32 count = chunkEnd - chunkStart;

      GFXVertexBufferHandle<GFXVertexPCT> verts(device, count * 6, GFXBufferTypeVolatile);
      verts.lock();

      // Grouped by (texture, addressModeU, addressModeV) - a draw against different addressing
      // needs its own state block bound, same as it already needed its own texture bound.
      struct TextureGroup
      {
         GFXTextureObject* texture;
         GFXTextureAddressMode addressU;
         GFXTextureAddressMode addressV;
         U32 startVertex;
         U32 numQuads;
      };
      Vector< TextureGroup > groups;
      Vector< U32 > groupForEntry;   // Parallel to [chunkStart, chunkEnd).
      groupForEntry.setSize(count);

      for (U32 i = 0; i < count; i++)
      {
         const NewGuiBatchTextureDraw& srcDraw = mTexturedRuns[chunkStart + i];
         GFXTextureObject* tex = srcDraw.texture.getPointer();

         U32 g = 0;
         for (; g < groups.size(); g++)
         {
            if (groups[g].texture == tex && groups[g].addressU == srcDraw.addressModeU && groups[g].addressV == srcDraw.addressModeV)
               break;
         }

         if (g == groups.size())
         {
            TextureGroup newGroup;
            newGroup.texture = tex;
            newGroup.addressU = srcDraw.addressModeU;
            newGroup.addressV = srcDraw.addressModeV;
            newGroup.startVertex = 0;   // Fixed up below.
            newGroup.numQuads = 0;
            groups.push_back(newGroup);
         }

         groups[g].numQuads++;
         groupForEntry[i] = g;
      }

      U32 runningVertex = 0;
      for (U32 g = 0; g < groups.size(); g++)
      {
         groups[g].startVertex = runningVertex;
         runningVertex += groups[g].numQuads * 6;
      }

      Vector< U32 > writeCursor;
      writeCursor.setSize(groups.size());
      for (U32 g = 0; g < groups.size(); g++)
         writeCursor[g] = groups[g].startVertex;

      for (U32 i = 0; i < count; i++)
      {
         const NewGuiBatchTextureDraw& draw = mTexturedRuns[chunkStart + i];

         const F32 clipLeft = (F32)draw.clip.point.x;
         const F32 clipTop = (F32)draw.clip.point.y;
         const F32 clipRight = (F32)(draw.clip.point.x + draw.clip.extent.x);
         const F32 clipBottom = (F32)(draw.clip.point.y + draw.clip.extent.y);

         F32 left = (F32)draw.deviceRect.point.x;
         F32 top = (F32)draw.deviceRect.point.y;
         F32 right = (F32)(draw.deviceRect.point.x + draw.deviceRect.extent.x);
         F32 bottom = (F32)(draw.deviceRect.point.y + draw.deviceRect.extent.y);

         // Position-only clamp - UVs aren't re-derived, so a clipped edge holds whatever texel was already there.
         left = mClampF(left, clipLeft, clipRight);
         right = mClampF(right, clipLeft, clipRight);
         top = mClampF(top, clipTop, clipBottom);
         bottom = mClampF(bottom, clipTop, clipBottom);

         GFXVertexColor vcolor = draw.color;
         const U32 g = groupForEntry[i];
         U32 vi = writeCursor[g];

         verts[vi + 0].point.set(left, top, 0.f);     verts[vi + 0].color = vcolor; verts[vi + 0].texCoord.set(draw.texelLower.x, draw.texelLower.y);
         verts[vi + 1].point.set(right, top, 0.f);    verts[vi + 1].color = vcolor; verts[vi + 1].texCoord.set(draw.texelUpper.x, draw.texelLower.y);
         verts[vi + 2].point.set(right, bottom, 0.f); verts[vi + 2].color = vcolor; verts[vi + 2].texCoord.set(draw.texelUpper.x, draw.texelUpper.y);

         verts[vi + 3].point.set(right, bottom, 0.f); verts[vi + 3].color = vcolor; verts[vi + 3].texCoord.set(draw.texelUpper.x, draw.texelUpper.y);
         verts[vi + 4].point.set(left, bottom, 0.f);  verts[vi + 4].color = vcolor; verts[vi + 4].texCoord.set(draw.texelLower.x, draw.texelUpper.y);
         verts[vi + 5].point.set(left, top, 0.f);     verts[vi + 5].color = vcolor; verts[vi + 5].texCoord.set(draw.texelLower.x, draw.texelLower.y);

         writeCursor[g] += 6;
      }

      verts.unlock();

      device->setVertexBuffer(verts);
      device->setupGenericShaders(GFXDevice::GSModColorTexture);

      for (U32 g = 0; g < groups.size(); g++)
      {
         if (groups[g].numQuads == 0)
            continue;

         device->setStateBlock(_getTextureStateBlock(device, groups[g].addressU, groups[g].addressV));
         device->setTexture(0, groups[g].texture);
         device->drawPrimitive(GFXTriangleList, groups[g].startVertex, groups[g].numQuads * 2);
      }
   }
}

// Clip is per-glyph: a glyph is either fully written or fully skipped, never partially clipped.
void NewGuiRenderBatch::_flushText(GFXDevice* device, U32 startIndex, U32 endIndex)
{
   if (startIndex >= endIndex)
      return;

   // Flatten every surviving (post-clip) glyph in [startIndex, endIndex] into one flat list
   // first, before any GPU work - this is what makes chunking against kMaxItemsPerChunk (see
   // this file's top-of-file comment) straightforward below: a text run's glyphs aren't
   // naturally a flat, independent list the way mQuads/mTexturedRuns entries already are (one
   // run can itself contain far more glyphs than fit in a single chunk - the exact case a busy
   // NewGuiConsole full of long lines produces), so flattening first turns this into the same
   // "chunk a flat list" shape _flushQuads()/_flushTextured() already use, rather than needing
   // chunk boundaries that can split mid-run.
   struct FlatGlyph
   {
      GFont* font;
      U32 sheetIndex;
      F32 screenLeft, screenTop, screenRight, screenBottom;
      F32 texLeft, texTop, texRight, texBottom;
      GFXVertexColor color;
   };
   Vector<FlatGlyph> glyphs;

   for (U32 r = startIndex; r < endIndex; r++)
   {
      GFont* font = mTextRuns[r].font;
      const ColorI& color = mTextRuns[r].color;
      const Point2I& basePos = mTextRuns[r].basePos;
      const S32 letterSpacing = mTextRuns[r].letterSpacing;
      const S32 wordSpacing = mTextRuns[r].wordSpacing;
      const RectI& runClip = mTextRuns[r].clip;
      const UTF16* s = mTextRuns[r].utf16Text.address();

      S32 penX = basePos.x;
      GFXVertexColor vcolor = color;

      for (const UTF16* c = s; *c; c++)
      {
         const PlatformFont::CharInfo& ci = font->getCharInfo(*c);

         if (ci.width != 0 && ci.height != 0)
         {
            const F32 drawY = (F32)basePos.y + font->getBaseline() - ci.yOrigin;
            const F32 drawX = (F32)penX + ci.xOrigin;
            const RectI glyphRect(Point2I((S32)drawX, (S32)drawY), Point2I(ci.width, ci.height));

            if (runClip.contains(glyphRect))
            {
               const U32 sheetIndex = ci.bitmapIndex;
               const GFXTextureObject* tex = font->getTextureHandle(sheetIndex);

               const F32 texWidth = (F32)tex->getWidth();
               const F32 texHeight = (F32)tex->getHeight();

               FlatGlyph g;
               g.font = font;
               g.sheetIndex = sheetIndex;
               g.screenLeft = drawX;
               g.screenTop = drawY;
               g.screenRight = drawX + ci.width;
               g.screenBottom = drawY + ci.height;
               g.texLeft = (F32)(ci.xOffset) / texWidth;
               g.texRight = (F32)(ci.xOffset + ci.width) / texWidth;
               g.texTop = (F32)(ci.yOffset) / texHeight;
               g.texBottom = (F32)(ci.yOffset + ci.height) / texHeight;
               g.color = vcolor;
               glyphs.push_back(g);
            }
         }

         penX += ci.xIncrement + letterSpacing;
         if (*c == ' ')
            penX += wordSpacing;
      }
   }

   if (glyphs.empty())
      return;

   // Sub-batch against kMaxItemsPerChunk - see this file's top-of-file comment. Grouping (by
   // font/sheet) is recomputed fresh per chunk, same as _flushTextured()'s texture grouping.
   for (U32 chunkStart = 0; chunkStart < glyphs.size(); chunkStart += kMaxItemsPerChunk)
   {
      const U32 chunkEnd = getMin((U32)glyphs.size(), chunkStart + kMaxItemsPerChunk);
      const U32 count = chunkEnd - chunkStart;

      GFXVertexBufferHandle<GFXVertexPCT> verts(device, count * 6, GFXBufferTypeVolatile);
      verts.lock();

      // Group by (font, sheet index) so every sheet gets one drawPrimitive() call.
      struct SheetGroup
      {
         GFont* font;
         U32    sheetIndex;
         U32    startVertex;
         U32    numGlyphs;
      };
      Vector< SheetGroup > groups;
      Vector< U32 > groupForEntry;   // Parallel to [chunkStart, chunkEnd].
      groupForEntry.setSize(count);

      for (U32 i = 0; i < count; i++)
      {
         const FlatGlyph& glyph = glyphs[chunkStart + i];

         U32 g = 0;
         for (; g < groups.size(); g++)
         {
            if (groups[g].font == glyph.font && groups[g].sheetIndex == glyph.sheetIndex)
               break;
         }

         if (g == groups.size())
         {
            SheetGroup newGroup;
            newGroup.font = glyph.font;
            newGroup.sheetIndex = glyph.sheetIndex;
            newGroup.startVertex = 0;   // Fixed up below.
            newGroup.numGlyphs = 0;
            groups.push_back(newGroup);
         }

         groups[g].numGlyphs++;
         groupForEntry[i] = g;
      }

      U32 runningVertex = 0;
      for (U32 g = 0; g < groups.size(); g++)
      {
         groups[g].startVertex = runningVertex;
         runningVertex += groups[g].numGlyphs * 6;
      }

      Vector< U32 > writeCursor;
      writeCursor.setSize(groups.size());
      for (U32 g = 0; g < groups.size(); g++)
         writeCursor[g] = groups[g].startVertex;

      for (U32 i = 0; i < count; i++)
      {
         const FlatGlyph& glyph = glyphs[chunkStart + i];
         const U32 g = groupForEntry[i];
         U32 vi = writeCursor[g];

         verts[vi + 0].point.set(glyph.screenLeft, glyph.screenTop, 0.f);     verts[vi + 0].color = glyph.color; verts[vi + 0].texCoord.set(glyph.texLeft, glyph.texTop);
         verts[vi + 1].point.set(glyph.screenRight, glyph.screenTop, 0.f);    verts[vi + 1].color = glyph.color; verts[vi + 1].texCoord.set(glyph.texRight, glyph.texTop);
         verts[vi + 2].point.set(glyph.screenRight, glyph.screenBottom, 0.f); verts[vi + 2].color = glyph.color; verts[vi + 2].texCoord.set(glyph.texRight, glyph.texBottom);

         verts[vi + 3].point.set(glyph.screenRight, glyph.screenBottom, 0.f); verts[vi + 3].color = glyph.color; verts[vi + 3].texCoord.set(glyph.texRight, glyph.texBottom);
         verts[vi + 4].point.set(glyph.screenLeft, glyph.screenBottom, 0.f);  verts[vi + 4].color = glyph.color; verts[vi + 4].texCoord.set(glyph.texLeft, glyph.texBottom);
         verts[vi + 5].point.set(glyph.screenLeft, glyph.screenTop, 0.f);     verts[vi + 5].color = glyph.color; verts[vi + 5].texCoord.set(glyph.texLeft, glyph.texTop);

         writeCursor[g] += 6;
      }

      verts.unlock();

      device->setVertexBuffer(verts);
      device->setStateBlock(mTextSB);
      device->setupGenericShaders(GFXDevice::GSAddColorTexture);

      for (U32 g = 0; g < groups.size(); g++)
      {
         if (groups[g].numGlyphs == 0)
            continue;

         device->setTexture(0, groups[g].font->getTextureHandle(groups[g].sheetIndex));
         device->drawPrimitive(GFXTriangleList, groups[g].startVertex, groups[g].numGlyphs * 2);
      }
   }
}

void NewGuiRenderBatch::flush(GFXDevice* device)
{
   if (!device)
      return;

   _ensureStateBlocks(device);

   // Stable-sort each queue by layer; within a layer, submission order is preserved.
   std::stable_sort(mQuads.begin(), mQuads.end(),
      [](const NewGuiBatchQuad& a, const NewGuiBatchQuad& b) { return a.layer < b.layer; });
   std::stable_sort(mTextRuns.begin(), mTextRuns.end(),
      [](const NewGuiBatchTextRun& a, const NewGuiBatchTextRun& b) { return a.layer < b.layer; });
   std::stable_sort(mTexturedRuns.begin(), mTexturedRuns.end(),
      [](const NewGuiBatchTextureDraw& a, const NewGuiBatchTextureDraw& b) { return a.layer < b.layer; });

   // Walk all three sorted queues in lockstep, layer by layer: quads, then textured quads, then text.
   U32 qi = 0, xi = 0, ti = 0;
   while (qi < mQuads.size() || xi < mTexturedRuns.size() || ti < mTextRuns.size())
   {
      bool haveQuad = (qi < mQuads.size());
      bool haveTextured = (xi < mTexturedRuns.size());
      bool haveText = (ti < mTextRuns.size());

      S32 layer = 0;
      bool haveLayer = false;
      if (haveQuad) { layer = mQuads[qi].layer; haveLayer = true; }
      if (haveTextured) { layer = haveLayer ? getMin(layer, mTexturedRuns[xi].layer) : mTexturedRuns[xi].layer; haveLayer = true; }
      if (haveText) { layer = haveLayer ? getMin(layer, mTextRuns[ti].layer) : mTextRuns[ti].layer; haveLayer = true; }

      if (!haveLayer)
         break;

      const U32 quadStart = qi;
      while (qi < mQuads.size() && mQuads[qi].layer == layer)
         qi++;

      const U32 texturedStart = xi;
      while (xi < mTexturedRuns.size() && mTexturedRuns[xi].layer == layer)
         xi++;

      const U32 textStart = ti;
      while (ti < mTextRuns.size() && mTextRuns[ti].layer == layer)
         ti++;

      _flushQuads(device, quadStart, qi);
      _flushTextured(device, texturedStart, xi);
      _flushText(device, textStart, ti);
   }

   mQuads.clear();
   mTextRuns.clear();
   mTexturedRuns.clear();

   if (mClipStack.size() > 1)
      mClipStack.setSize(1);
}
