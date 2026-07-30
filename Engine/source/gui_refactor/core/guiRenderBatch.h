//-----------------------------------------------------------------------------
// guiRenderBatch.h
//
// The per-frame batch collector from gui-rewrite-design.md §7 -- built now,
// ahead of the originally-planned sequencing (§7 was slated to land after
// the full style/skin system, see gui-migration-plan.md's Stage E), because
// GuiPopupMenu needs a real, non-immediate-mode rendering path today rather
// than falling back to GFXDrawUtil's one-draw-call-per-primitive model.
//
// SCOPE NOTE: this first cut only implements the two primitive kinds a
// plain solid-color-and-text control actually needs:
//
//   - pushQuad()  -- solid-color filled rectangle (box fills, borders when
//                    drawn as four thin quads, hover highlights)
//   - pushLine()  -- a thin solid-color quad standing in for a 1px line
//                    (separators) -- deliberately submitted as a quad, not
//                    a GFXLineList primitive, so it shares the SAME vertex
//                    buffer and draw call as pushQuad() output instead of
//                    forcing a second primitive-type draw call per frame.
//   - pushText()  -- a text run against a loaded GFont, batched per texture
//                    sheet exactly the way the existing (per-string)
//                    FontRenderBatcher already does it (see
//                    gfxFontRenderBatcher.cpp) -- this class generalizes
//                    that same technique across an entire frame's worth of
//                    text instead of scoping it to one string at a time.
//-----------------------------------------------------------------------------

#ifndef _GUIRENDERBATCH_H_
#define _GUIRENDERBATCH_H_

#ifndef _MPOINT2_H_
#include "math/mPoint2.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif
#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _UNICODE_H_
#include "core/strings/unicode.h"
#endif

/// One queued solid-color quad -- see pushQuad()/pushLine().
struct GuiBatchQuad
{
   Point2F p0, p1, p2, p3; // four corners, already in device-pixel screen space, wound consistently (see _pushQuadVerts())
   ColorI  color;

   /// The submitting control's GuiControlNew::getRenderLayer() at push
   /// time
   S32 layer;

   /// The clip rect in effect
   RectI clip;
};

/// One queued text run against a specific font
struct GuiBatchTextRun
{
   Resource<GFont> font;

   Point2I      basePos;  // device-pixel draw origin, same meaning as GFXDrawUtil::drawText()'s ptDraw
   ColorI       color;
   String       text;     // original UTF-8 source, kept for reference/debugging -- NOT what glyph lookup walks, see utf16Text below


   Vector<UTF16> utf16Text;

   S32          letterSpacing;
   S32          wordSpacing;
   S32          layer;

   /// Same meaning/semantics as GuiBatchQuad::clip -- captured at push time.
   RectI        clip;

   GuiBatchTextRun() : letterSpacing(0), wordSpacing(0), layer(0) {}
};

struct GuiBatchTextureDraw
{
   /// Destination rect in device-pixel space, same convention as
   /// pushQuad()'s deviceRect.
   RectI deviceRect;

   /// Which texture this frame's UVs are relative to.
   GFXTexHandle texture;

   /// Texel-space UV rect
   Point2F texelLower, texelUpper;

   /// Pixel-space size of the SOURCE frame
   Point2I framePixelSize;

   ColorI color; ///< tint; ColorI(255,255,255,255) for "untinted, draw the frame as-is"

   S32 layer; ///< same meaning as GuiBatchQuad::layer

   RectI clip; ///< same meaning as GuiBatchQuad::clip

   GuiBatchTextureDraw() : color(255, 255, 255, 255), layer(0) {}
};

/// See file header. Owned by GuiCanvas
class GuiRenderBatch
{
protected:

   Vector< GuiBatchQuad >    mQuads;
   Vector< GuiBatchTextRun > mTextRuns;
   Vector< GuiBatchTextureDraw > mTexturedRuns;

   GFXStateBlockRef mSolidSB;   ///< Untextured solid-fill state block -- see _ensureStateBlocks().
   GFXStateBlockRef mTextSB;    ///< Alpha-blended, point-filtered, textured state block for glyph sheets.
   GFXStateBlockRef mTextureSB; ///< Textured solid-fill state block, needs to also allow for wrapping. 

   bool mStateBlocksReady;

   /// Clip-rect stack -- see pushClipRect()/popClipRect(). The bottom
   /// of the stack (index 0) is always present after begin() and holds
   /// the full device viewport, so mClipStack is never empty while a
   /// frame is in progress and every primitive can unconditionally
   /// stamp mClipStack.last().
   Vector< RectI > mClipStack;

   void _ensureStateBlocks(GFXDevice* device);

   /// Appends a solid-color quad's six vertices (two triangles) into an
   /// already-locked vertex buffer at Index startIndex
   void _writeQuadVerts(GFXVertexBufferHandle<GFXVertexPCT>& verts, U32 startIndex, const GuiBatchQuad& quad) const;

   /// Flushes mQuads[startIndex, endIndex]
   void _flushQuads(GFXDevice* device, U32 startIndex, U32 endIndex);

   ///  Flushes mTexturedRuns[startIndex, endIndex]
   void _flushTextured(GFXDevice* device, U32 startIndex, U32 endIndex);

   /// Flushes mTextRuns[startIndex, endIndex]
   void _flushText(GFXDevice* device, U32 startIndex, U32 endIndex);

public:

   GuiRenderBatch();
   ~GuiRenderBatch();

   /// Clears all queued primitives -- called once at the start of each
   /// frame's render pass
   void begin(const RectI& deviceViewport);

   /// adds a cliprect to the top of the stack, any push after this call will
   /// clip to it.
   void pushClipRect(const RectI& deviceRect);

   /// Pops the clip rect pushed
   void popClipRect();

   /// The clip rect that would currently be stamped onto a primitive
   /// pushed right now
   const RectI& getCurrentClipRect() const { return mClipStack.last(); }

   /// Queues a solid-color quad.
   void pushQuad(const RectI& deviceRect, const ColorI& color, S32 layer = 0);

   /// Queues a thin quad standing in for a 1px line
   void pushLine(const Point2I& start, const Point2I& end, const ColorI& color, F32 thickness = 1.0f, S32 layer = 0);

   /// Queues a text run. font must already be resolved/loaded 
   void pushText(const Resource<GFont>& font, const Point2I& basePos, const char* text, const ColorI& color, S32 letterSpacing = 0, S32 wordSpacing = 0, S32 layer = 0);

   /// Same as pushText(), but takes already-decoded UTF-16 directly
   void pushTextRun(const Resource<GFont>& font, const Point2I& basePos, const UTF16* chars, U32 charCount, const ColorI& color, S32 letterSpacing = 0, S32 wordSpacing = 0, S32 layer = 0);

   /// Queues a textured quad with the resolved image asset and frame.
   // void pushSkinnedQuad(const RectI& destRect, const GuiResolvedSkinFrame& resolved, S32 layer = 0, const ColorI& color = ColorI(255, 255, 255, 255));

   /// Renders every queued primitive and clears the queues -- called
   /// once per frame, after the full control tree's onRender() calls
   /// have all run.
   void flush(GFXDevice* device);

   /// True if nothing has been queued since the last begin()/flush().
   bool isEmpty() const { return mQuads.empty() && mTextRuns.empty() && mTexturedRuns.empty(); }
};

#endif // _GUIRENDERBATCH_H_
