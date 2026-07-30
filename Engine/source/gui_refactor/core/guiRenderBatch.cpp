//-----------------------------------------------------------------------------
// guiRenderBatch.cpp
// See guiRenderBatch.h for full design notes and the confirmed call sites
// this is modeled on (GFXDrawUtil::drawRect(), FontRenderBatcher).
//-----------------------------------------------------------------------------

#include "gui_refactor/core/guiRenderBatch.h"
#include <algorithm> // std::stable_sort -- see flush()'s layer-grouping sort

//-----------------------------------------------------------------------------

GuiRenderBatch::GuiRenderBatch()
   : mStateBlocksReady(false)
{
}

//-----------------------------------------------------------------------------

GuiRenderBatch::~GuiRenderBatch()
{
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::_ensureStateBlocks(GFXDevice* device)
{
   if (mStateBlocksReady)
      return;

   // Untextured solid fill -- same blend/cull/z settings as
   // GFXDrawUtil's own mRectFillSB (gfxDrawUtil.cpp's _setupStateBlocks()),
   // since this is drawing exactly the same kind of primitive (a flat-
   // colored 2D quad), just batched across many quads per buffer instead
   // of one buffer per call.
   GFXStateBlockDesc solidDesc;
   solidDesc.setCullMode(GFXCullNone);
   solidDesc.setZReadWrite(false);
   solidDesc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
   mSolidSB = device->createStateBlock(solidDesc);

   // Textured glyph-sheet pass -- same settings as FontRenderBatcher's own
   // mFontSB (gfxFontRenderBatcher.cpp's constructor): point-filtered,
   // clamped, alpha-blended, no depth, no alpha channel write.
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
   textDesc.samplers[0].addressModeU = GFXAddressClamp;
   textDesc.samplers[0].addressModeV = GFXAddressClamp;
   textDesc.setColorWrites(true, true, true, false);
   mTextSB = device->createStateBlock(textDesc);

   mStateBlocksReady = true;
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::begin(const RectI& deviceViewport)
{
   mQuads.clear();
   mTextRuns.clear();
   mTexturedRuns.clear();

   // Reset the clip stack to just the base viewport
   mClipStack.clear();
   mClipStack.push_back(deviceViewport);
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::pushClipRect(const RectI& deviceRect)
{
   RectI narrowed = deviceRect;
   if (!narrowed.intersect(mClipStack.last()))
   {
      // No overlap with the current clip at all
      narrowed = RectI(mClipStack.last().point, Point2I(0, 0));
   }

   mClipStack.push_back(narrowed);
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::popClipRect()
{
   AssertFatal(mClipStack.size() > 1, "GuiRenderBatch::popClipRect() - unbalanced pop (no matching pushClipRect(), or already at the base viewport) -- see pushClipRect()'s doc comment");

   if (mClipStack.size() > 1)
      mClipStack.pop_back();
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::pushQuad(const RectI& deviceRect, const ColorI& color, S32 layer)
{
   GuiBatchQuad q;

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

//-----------------------------------------------------------------------------

void GuiRenderBatch::pushLine(const Point2I& start, const Point2I& end, const ColorI& color, F32 thickness, S32 layer)
{
   // A line is submitted as a thin quad so it shares the same vertex
   // buffer/draw call as every pushQuad() this frame -- see file header.
   // Build a quad of `thickness` device pixels, centered on the
   // start->end segment, by offsetting perpendicular to its direction.
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
      // Zero-length segment -- degenerate, but still draw *something*
      // (a small dot) rather than silently dropping it.
      perp.set(1.0f, 0.0f);
   }

   const F32 half = thickness * 0.5f;
   Point2F offset(perp.x * half, perp.y * half);

   GuiBatchQuad q;
   q.p0 = a - offset;
   q.p1 = b - offset;
   q.p2 = b + offset;
   q.p3 = a + offset;
   q.color = color;
   q.layer = layer;
   q.clip = mClipStack.last();

   mQuads.push_back(q);
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::pushText(const Resource<GFont>& font, const Point2I& basePos, const char* text, const ColorI& color, S32 letterSpacing, S32 wordSpacing, S32 layer)
{
   // font != NULL relies on Resource<GFont>'s own operator==(NULL)-style
   // validity check (same pattern every getResolvedFont() call site
   // already uses, e.g. guiCheckBoxCtrlNew.cpp: "GFont *font = fontRes;
   // if (font)") -- an empty/unresolved Resource<GFont> is just as much
   // a "nothing to draw" case as a null raw pointer used to be.
   if (font == NULL || !text || !text[0])
      return;

   GuiBatchTextRun run;
   run.font = font; // copies the Resource<GFont> handle itself (ref-counted), not just a raw pointer out of it -- see GuiBatchTextRun::font's doc comment
   run.basePos = basePos;
   run.color = color;
   run.text = text;
   run.letterSpacing = letterSpacing;
   run.wordSpacing = wordSpacing;
   run.layer = layer;
   run.clip = mClipStack.last();

   // Decode the raw UTF-8 bytes into real UTF-16 code units ONCE, here,
   // the same way GFXDrawUtil::drawTextN(..., const UTF8*, ...) already
   // does via convertUTF8toUTF16N() before it ever calls
   // GFont::getCharInfo()/FontRenderBatcher::queueChar() (both of which
   // take a single UTF16 code unit, not a UTF-8 byte -- see gFont.h/
   // gfxFontRenderBatcher.h). _flushText() below walks this decoded
   // buffer, never run.text's raw bytes directly -- casting a raw UTF-8
   // byte straight to UTF16 only happens to work for pure ASCII; any
   // real multi-byte character would otherwise be split into several
   // bogus per-byte glyph lookups.
   //
   // Sized dStrlen(text)+1: decoded UTF-16 length is always <= the UTF-8
   // source's byte length (each UTF-8 sequence collapses to one, or in
   // the surrogate-pair case at most two, UTF-16 units per 1-4 source
   // bytes), +1 for the null terminator -- same sizing convention as
   // gfxDrawUtil.cpp's own FrameTemp<UTF16> ubuf(n + 1).
   const U32 byteLen = dStrlen(text);
   run.utf16Text.setSize(byteLen + 1);
   convertUTF8toUTF16N(text, run.utf16Text.address(), byteLen + 1);

   mTextRuns.push_back(run);
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::pushTextRun(const Resource<GFont>& font, const Point2I& basePos, const UTF16* chars, U32 charCount, const ColorI& color, S32 letterSpacing, S32 wordSpacing, S32 layer)
{
   if (font == NULL || !chars || charCount == 0)
      return;

   GuiBatchTextRun run;
   run.font = font;
   run.basePos = basePos;
   run.color = color;
   // run.text (UTF-8 debug copy) left empty -- no UTF-8 form exists here.
   run.letterSpacing = letterSpacing;
   run.wordSpacing = wordSpacing;
   run.layer = layer;
   run.clip = mClipStack.last();

   // Copy directly (already UTF-16); null-terminate since _flushText()
   // walks the buffer as null-terminated, same as pushText()'s own decode.
   run.utf16Text.setSize(charCount + 1);
   dMemcpy(run.utf16Text.address(), chars, charCount * sizeof(UTF16));
   run.utf16Text[charCount] = 0;

   mTextRuns.push_back(run);
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::_writeQuadVerts(GFXVertexBufferHandle<GFXVertexPCT>& verts, U32 startIndex, const GuiBatchQuad& quad) const
{
   // Two triangles, six vertices, no shared indexing -- see file header's
   // note on why GFXTriangleList (not Strip) is used for the whole-frame
   // buffer: unrelated quads can't share strip continuity with each
   // other, so each quad contributes its own independent 6 verts.
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
      verts[startIndex + i].texCoord.set(0.0f, 0.0f); // unused by the untextured solid-fill shader path, but GFXVertexPCT always carries the field
   }
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::_flushQuads(GFXDevice* device, U32 startIndex, U32 endIndex)
{
   if (startIndex >= endIndex)
      return;

   const U32 count = endIndex - startIndex;
   const U32 vertCount = count * 6;

   GFXVertexBufferHandle<GFXVertexPCT> verts(device, vertCount, GFXBufferTypeVolatile);
   verts.lock();

   // Clip is applied here, in CPU-side geometry, rather than via
   // GFX->setClipRect()
   for (U32 i = 0; i < count; i++)
   {
      GuiBatchQuad clipped = mQuads[startIndex + i];

      const F32 clipLeft = (F32)clipped.clip.point.x;
      const F32 clipTop = (F32)clipped.clip.point.y;
      const F32 clipRight = (F32)(clipped.clip.point.x + clipped.clip.extent.x);
      const F32 clipBottom = (F32)(clipped.clip.point.y + clipped.clip.extent.y);

      // Quads are always axis-aligned rects at this point (pushQuad()'s
      // four corners, or pushLine()'s thin quad) -- clamping each corner
      // independently to the clip bounds is exact for an axis-aligned
      // quad against an axis-aligned rect; it would NOT be exact for an
      // arbitrarily rotated quad, but nothing in this batch produces one.
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
   device->setupGenericShaders(); // no-arg: plain untextured/solid-color path -- see GFXDrawUtil::drawRect()

   // One draw call for every quad in this layer -- clip is already baked
   // into the vertex positions above, so this needs no further splitting
   // and no GFX->setClipRect() call at all.
   device->drawPrimitive(GFXTriangleList, 0, count * 2);
}

//-----------------------------------------------------------------------------

void GuiRenderBatch::_flushText(GFXDevice* device, U32 startIndex, U32 endIndex)
{
   if (startIndex >= endIndex)
      return;

   // Clip is applied per-glyph, in CPU-side geometry (a glyph is either
   // fully written or fully skipped, based on whether its screen rect
   // overlaps its own run's stored clip rect)
   U32 totalGlyphs = 0;
   for (U32 r = startIndex; r < endIndex; r++)
   {
      GFont* font = mTextRuns[r].font;
      const Point2I& basePos = mTextRuns[r].basePos;
      const S32 letterSpacing = mTextRuns[r].letterSpacing;
      const S32 wordSpacing = mTextRuns[r].wordSpacing;
      const RectI& runClip = mTextRuns[r].clip;
      const UTF16* s = mTextRuns[r].utf16Text.address();

      S32 penX = basePos.x;

      for (const UTF16* c = s; *c; c++)
      {
         const PlatformFont::CharInfo& ci = font->getCharInfo(*c);
         if (ci.width != 0 && ci.height != 0)
         {
            const F32 drawY = (F32)basePos.y + font->getBaseline() - ci.yOrigin;
            const F32 drawX = (F32)penX + ci.xOrigin;
            const RectI glyphRect(Point2I((S32)drawX, (S32)drawY), Point2I(ci.width, ci.height));

            if (runClip.contains(glyphRect))
               totalGlyphs++;
         }

         penX += ci.xIncrement + letterSpacing;
         if (*c == ' ')
            penX += wordSpacing;
      }
   }

   if (totalGlyphs == 0)
      return;

   GFXVertexBufferHandle<GFXVertexPCT> verts(device, totalGlyphs * 6, GFXBufferTypeVolatile);
   verts.lock();

   // Group by (font, sheet index) so every sheet touched, across every
   // run IN THIS RANGE, gets exactly one drawPrimitive() call --
   // generalizes FontRenderBatcher's per-string SheetMarker grouping (see
   // gfxFontRenderBatcher.cpp) to this range's scope. Sheet index alone
   // isn't unique across different fonts, so key on (font pointer, sheet)
   // pairs, flushed in the order first encountered. Clip is NOT part of
   // this key -- clip-culled glyphs are simply never counted/written
   // below, so every remaining glyph in a group is visible and belongs in
   // one shared draw call same as before clip support existed.
   struct SheetGroup
   {
      GFont* font;
      U32    sheetIndex;
      U32    startVertex;
      U32    numGlyphs;
   };
   Vector< SheetGroup > groups;

   for (U32 r = startIndex; r < endIndex; r++)
   {
      GFont* font = mTextRuns[r].font;
      const Point2I& basePos = mTextRuns[r].basePos;
      const S32 letterSpacing = mTextRuns[r].letterSpacing;
      const S32 wordSpacing = mTextRuns[r].wordSpacing;
      const RectI& runClip = mTextRuns[r].clip;
      const UTF16* s = mTextRuns[r].utf16Text.address();

      S32 penX = basePos.x;

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

               // Find (or start) this (font, sheet)'s group. Groups are
               // only ever appended to contiguously as we walk runs in
               // order, so a simple linear scan of the (small) existing
               // group list is fine here -- this isn't a hot path
               // relative to the actual draw calls it's saving.
               SheetGroup* group = NULL;
               for (U32 g = 0; g < groups.size(); g++)
               {
                  if (groups[g].font == font && groups[g].sheetIndex == sheetIndex)
                  {
                     group = &groups[g];
                     break;
                  }
               }

               if (!group)
               {
                  SheetGroup newGroup;
                  newGroup.font = font;
                  newGroup.sheetIndex = sheetIndex;
                  newGroup.startVertex = 0; // fixed up in the second pass below
                  newGroup.numGlyphs = 0;
                  groups.push_back(newGroup);
                  group = &groups.last();
               }

               group->numGlyphs++;
            }
         }

         penX += ci.xIncrement + letterSpacing;
         if (*c == ' ')
            penX += wordSpacing;
      }
   }

   // Second pass: now that every group's total glyph count is known,
   // assign each group a contiguous starting vertex and actually write
   // glyph quads into place -- this keeps every sheet's glyphs contiguous
   // in the buffer regardless of draw-call interleaving, so _each_ group
   // can still be flushed with exactly one drawPrimitive() call.
   U32 runningVertex = 0;
   for (U32 g = 0; g < groups.size(); g++)
   {
      groups[g].startVertex = runningVertex;
      runningVertex += groups[g].numGlyphs * 6;
   }

   Vector< U32 > writeCursor; // per-group next-write-offset, parallel to groups
   writeCursor.setSize(groups.size());
   for (U32 g = 0; g < groups.size(); g++)
      writeCursor[g] = groups[g].startVertex;

   for (U32 r = startIndex; r < endIndex; r++)
   {
      GFont* font = mTextRuns[r].font;
      const ColorI& color = mTextRuns[r].color;
      const Point2I& basePos = mTextRuns[r].basePos;
      const S32 letterSpacing = mTextRuns[r].letterSpacing;
      const S32 wordSpacing = mTextRuns[r].wordSpacing;
      const RectI& runClip = mTextRuns[r].clip;
      const UTF16* u = mTextRuns[r].utf16Text.address();

      S32 penX = basePos.x;
      GFXVertexColor vcolor = color;

      for (const UTF16* c = u; *c; c++)
      {
         const PlatformFont::CharInfo& ci = font->getCharInfo(*c);

         if (ci.width != 0 && ci.height != 0)
         {
            const F32 drawY = (F32)basePos.y + font->getBaseline() - ci.yOrigin;
            const F32 drawX = (F32)penX + ci.xOrigin;

            const F32 screenLeft = drawX;
            const F32 screenRight = drawX + ci.width;
            const F32 screenTop = drawY;
            const F32 screenBottom = drawY + ci.height;

            const RectI glyphRect(Point2I((S32)drawX, (S32)drawY), Point2I(ci.width, ci.height));

            if (runClip.contains(glyphRect))
            {
               const U32 sheetIndex = ci.bitmapIndex;

               U32 groupIdx = 0;
               for (; groupIdx < groups.size(); groupIdx++)
                  if (groups[groupIdx].font == font && groups[groupIdx].sheetIndex == sheetIndex)
                     break;

               const GFXTextureObject* tex = font->getTextureHandle(sheetIndex);

               const F32 texWidth = (F32)tex->getWidth();
               const F32 texHeight = (F32)tex->getHeight();
               const F32 texLeft = (F32)(ci.xOffset) / texWidth;
               const F32 texRight = (F32)(ci.xOffset + ci.width) / texWidth;
               const F32 texTop = (F32)(ci.yOffset) / texHeight;
               const F32 texBottom = (F32)(ci.yOffset + ci.height) / texHeight;

               U32 vi = writeCursor[groupIdx];

               verts[vi + 0].point.set(screenLeft, screenTop, 0.f); verts[vi + 0].color = vcolor; verts[vi + 0].texCoord.set(texLeft, texTop);
               verts[vi + 1].point.set(screenRight, screenTop, 0.f); verts[vi + 1].color = vcolor; verts[vi + 1].texCoord.set(texRight, texTop);
               verts[vi + 2].point.set(screenRight, screenBottom, 0.f); verts[vi + 2].color = vcolor; verts[vi + 2].texCoord.set(texRight, texBottom);

               verts[vi + 3].point.set(screenRight, screenBottom, 0.f); verts[vi + 3].color = vcolor; verts[vi + 3].texCoord.set(texRight, texBottom);
               verts[vi + 4].point.set(screenLeft, screenBottom, 0.f); verts[vi + 4].color = vcolor; verts[vi + 4].texCoord.set(texLeft, texBottom);
               verts[vi + 5].point.set(screenLeft, screenTop, 0.f); verts[vi + 5].color = vcolor; verts[vi + 5].texCoord.set(texLeft, texTop);

               writeCursor[groupIdx] += 6;
            }
         }

         // Advance formula documented on _flushText()'s own declaration
         // (guiRenderBatch.h) -- letterSpacing applies after every
         // character, wordSpacing additionally after a space, matching
         // GuiStyleProperties::letterSpacing/wordSpacing's semantics.
         penX += ci.xIncrement + letterSpacing;
         if (*c == ' ')
            penX += wordSpacing;
      }
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

//-----------------------------------------------------------------------------

void GuiRenderBatch::flush(GFXDevice* device)
{
   if (!device)
      return;

   _ensureStateBlocks(device);

   // Stable-sort each queue by layer, ascending. Stable so that within one
   // layer, primitives keep the relative order they were pushed in
   // (submission order, which for the common case of a control's own
   // onRender() is background-quad-then-its-own-text) -- though per this
   // design's own contract (see GuiControlNew::mRenderLayer's doc
   // comment), no ordering GUARANTEE is made or needed WITHIN a layer;
   // stability here is just "don't gratuitously reorder things that
   // didn't need to move," not a promise callers should rely on.
   std::stable_sort(mQuads.begin(), mQuads.end(),
      [](const GuiBatchQuad& a, const GuiBatchQuad& b) { return a.layer < b.layer; });
   std::stable_sort(mTextRuns.begin(), mTextRuns.end(),
      [](const GuiBatchTextRun& a, const GuiBatchTextRun& b) { return a.layer < b.layer; });

   // Walk both now-sorted queues in lockstep, layer by layer, ascending --
   // for each distinct layer value present in EITHER queue, flush that
   // layer's quads (if any) then that SAME layer's text (if any) before
   // moving to the next layer. This is what actually fixes cross-control
   // draw order: a shallower/earlier-drawn control's quad can no longer
   // block in front of every layer's text just because "all quads" used
   // to be one undifferentiated category ahead of "all text" -- see this
   // method's header doc comment (guiRenderBatch.h) for the full
   // rationale.
   U32 qi = 0, ti = 0;
   while (qi < mQuads.size() || ti < mTextRuns.size())
   {
      // The next layer to process is the smaller of whichever queue still
      // has unflushed entries (or the only one that does, if the other is
      // already exhausted).
      bool haveQuad = (qi < mQuads.size());
      bool haveText = (ti < mTextRuns.size());
      S32 layer;
      if (haveQuad && haveText)
         layer = getMin(mQuads[qi].layer, mTextRuns[ti].layer);
      else if (haveQuad)
         layer = mQuads[qi].layer;
      else
         layer = mTextRuns[ti].layer;

      const U32 quadStart = qi;
      while (qi < mQuads.size() && mQuads[qi].layer == layer)
         qi++;

      const U32 textStart = ti;
      while (ti < mTextRuns.size() && mTextRuns[ti].layer == layer)
         ti++;

      // Quads before text WITHIN this one layer -- same "background then
      // text" convention as before, just now scoped per-layer instead of
      // per-frame.
      _flushQuads(device, quadStart, qi);
      _flushText(device, textStart, ti);
   }

   mQuads.clear();
   mTextRuns.clear();
   mTexturedRuns.clear();

   // Not strictly required (begin() resets this unconditionally next
   // frame)
   if (mClipStack.size() > 1)
      mClipStack.setSize(1);
}
