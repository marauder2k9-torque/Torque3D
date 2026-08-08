//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiRenderBatch.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIRENDERBATCH_H_
#define _NEWGUIRENDERBATCH_H_

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

/// One queued solid-color quad.
struct NewGuiBatchQuad
{
   Point2F p0, p1, p2, p3;   ///< Four corners in device-pixel screen space.
   ColorI  color;
   S32     layer;            ///< Submitting control's render layer at push time.
   RectI   clip;             ///< Clip rect in effect at push time.
};

/// One queued text run against a specific font.
struct NewGuiBatchTextRun
{
   Resource<GFont> font;

   Point2I      basePos;   ///< Device-pixel draw origin.
   ColorI       color;
   String       text;      ///< Original UTF-8 source, kept for reference only.

   Vector<UTF16> utf16Text;

   S32          letterSpacing;
   S32          wordSpacing;
   S32          layer;
   RectI        clip;

   NewGuiBatchTextRun() : letterSpacing(0), wordSpacing(0), layer(0) {}
};

/// One queued textured quad (icon, image control, skin frame).
struct NewGuiBatchTextureDraw
{
   RectI deviceRect;               ///< Destination rect in device-pixel space.
   GFXTexHandle texture;
   Point2F texelLower, texelUpper; ///< UV rect. Normalized (0..1) for a clamped draw; may extend
   ///< past 0..1 for a wrapped draw, where the excess tiles the
   ///< bound texture's full UV space that many times (see
   ///< pushTexturedQuad()'s own doc comment on the atlas caveat).
   Point2I framePixelSize;         ///< Pixel-space size of the source frame.
   ColorI color;                   ///< Tint; (255,255,255,255) for untinted.
   GFXTextureAddressMode addressModeU;   ///< Sampler wrap/clamp behavior along U. Independent of V so a caller can wrap one axis and clamp the other.
   GFXTextureAddressMode addressModeV;
   S32 layer;
   RectI clip;

   NewGuiBatchTextureDraw() : color(255, 255, 255, 255), addressModeU(GFXAddressClamp), addressModeV(GFXAddressClamp), layer(0) {}
};

/// Collects every control's draw commands for one frame and flushes them
/// in (layer, submission order). Owned by NewGuiCanvas: begin() at the
/// start of each frame, flush() at the end.
class NewGuiRenderBatch
{
protected:

   Vector< NewGuiBatchQuad >         mQuads;
   Vector< NewGuiBatchTextRun >      mTextRuns;
   Vector< NewGuiBatchTextureDraw >  mTexturedRuns;

   GFXStateBlockRef mSolidSB;      ///< Untextured solid-fill state block.
   GFXStateBlockRef mTextSB;       ///< Alpha-blended, point-filtered state block for glyph sheets.

   /// Alpha-blended, bilinear-filtered state blocks for icons/images/skin parts, one per distinct
   /// (addressModeU, addressModeV) pair actually requested this run - built lazily so the common
   /// clamp/clamp case costs exactly what it always did, and a skin using wrap tiling only pays
   /// for the extra state block(s) it actually needs.
   struct TextureSBKey
   {
      GFXTextureAddressMode addressU;
      GFXTextureAddressMode addressV;

      bool operator==(const TextureSBKey& o) const { return addressU == o.addressU && addressV == o.addressV; }
   };
   struct TextureSBEntry
   {
      TextureSBKey key;
      GFXStateBlockRef sb;
   };
   Vector<TextureSBEntry> mTextureSBCache;

   /// @return The cached (creating if needed) textured-quad state block for this address mode pair.
   GFXStateBlockRef _getTextureStateBlock(GFXDevice* device, GFXTextureAddressMode addressU, GFXTextureAddressMode addressV);

   bool mStateBlocksReady;   ///< Guards mSolidSB/mTextSB only - mTextureSBCache entries are built on demand regardless.

   Vector< RectI > mClipStack;     ///< Clip rect stack; index 0 is always the full device viewport after begin().

   void _ensureStateBlocks(GFXDevice* device);

   /// Appends one quad's six vertices (two triangles) into a locked vertex buffer.
   void _writeQuadVerts(GFXVertexBufferHandle<GFXVertexPCT>& verts, U32 startIndex, const NewGuiBatchQuad& quad) const;

   void _flushQuads(GFXDevice* device, U32 startIndex, U32 endIndex);
   void _flushTextured(GFXDevice* device, U32 startIndex, U32 endIndex);
   void _flushText(GFXDevice* device, U32 startIndex, U32 endIndex);

public:

   NewGuiRenderBatch();
   ~NewGuiRenderBatch();

   /// Clears all queued primitives for a new frame.
   /// @param deviceViewport Full device-pixel viewport rect.
   void begin(const RectI& deviceViewport);

   /// Pushes a clip rect, intersected against the current top of stack.
   /// @param deviceRect Rect to clip to, in device pixels.
   void pushClipRect(const RectI& deviceRect);

   /// Pops the clip rect pushed by the matching pushClipRect().
   void popClipRect();

   /// @return The clip rect that would apply to a primitive pushed right now.
   const RectI& getCurrentClipRect() const { return mClipStack.last(); }

   /// Queues a solid-color quad.
   /// @param deviceRect Destination rect in device pixels.
   /// @param color Fill color.
   /// @param layer Paint-order layer.
   void pushQuad(const RectI& deviceRect, const ColorI& color, S32 layer = 0);

   /// Queues a 1px-equivalent line as a thin quad.
   /// @param start Line start, in device pixels.
   /// @param end Line end, in device pixels.
   /// @param color Line color.
   /// @param thickness Line thickness in device pixels.
   /// @param layer Paint-order layer.
   void pushLine(const Point2I& start, const Point2I& end, const ColorI& color, F32 thickness = 1.0f, S32 layer = 0);

   /// Queues a UTF-8 text run.
   /// @param font Already-resolved/loaded font.
   /// @param basePos Draw origin, in device pixels.
   /// @param text UTF-8 source text.
   /// @param color Text color.
   /// @param letterSpacing Extra pixels between characters.
   /// @param wordSpacing Extra pixels between words.
   /// @param layer Paint-order layer.
   void pushText(const Resource<GFont>& font, const Point2I& basePos, const char* text, const ColorI& color, S32 letterSpacing = 0, S32 wordSpacing = 0, S32 layer = 0);

   /// Same as pushText(), but takes already-decoded UTF-16.
   void pushTextRun(const Resource<GFont>& font, const Point2I& basePos, const UTF16* chars, U32 charCount, const ColorI& color, S32 letterSpacing = 0, S32 wordSpacing = 0, S32 layer = 0);

   /// Queues a textured quad.
   /// @param deviceRect Destination rect in device pixels.
   /// @param texture Source texture.
   /// @param texelLower UV lower bound. Normalized (0..1) for Clamp addressing.
   /// @param texelUpper UV upper bound. Normalized (0..1) for Clamp addressing. With Wrap
   /// addressing on the corresponding axis, values beyond 0..1 tile the WHOLE texture (not just
   /// this UV sub-rect) that many times across deviceRect via the hardware sampler - correct only
   /// when texture isn't a shared atlas, since wrap addresses the texture's full 0..1 space
   /// regardless of what sub-rect texelLower/texelUpper describe. NewGuiStyleDrawSkinImage()'s own
   /// tiling (skin parts, which are commonly atlased) deliberately does NOT use this - it pushes
   /// one clamped quad per repeat instead. This addressMode-driven wrap path is for a caller who
   /// explicitly wants a single draw call tiled against its own full, non-atlased texture.
   /// @param framePixelSize Pixel-space size of the source frame.
   /// @param color Tint color.
   /// @param layer Paint-order layer.
   /// @param addressModeU Sampler address mode along U (default Clamp, matching pre-skin behavior).
   /// @param addressModeV Sampler address mode along V (default Clamp).
   void pushTexturedQuad(const RectI& deviceRect, GFXTexHandle texture, const Point2F& texelLower, const Point2F& texelUpper, const Point2I& framePixelSize, const ColorI& color = ColorI(255, 255, 255, 255), S32 layer = 0, GFXTextureAddressMode addressModeU = GFXAddressClamp, GFXTextureAddressMode addressModeV = GFXAddressClamp);

   /// Renders every queued primitive, in (layer, submission order), and clears the queues.
   /// @param device Device to render through.
   void flush(GFXDevice* device);

   /// @return True if nothing has been queued since the last begin()/flush().
   bool isEmpty() const { return mQuads.empty() && mTextRuns.empty() && mTexturedRuns.empty(); }
};

#endif // _NEWGUIRENDERBATCH_H_
