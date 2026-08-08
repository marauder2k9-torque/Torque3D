//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiCursor.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUICURSOR_H_
#define _NEWGUICURSOR_H_

#ifndef _SIMOBJECT_H_
#include "sim/simObject.h"
#endif
#ifndef _MPOINT2_H_
#include "math/mPoint2.h"
#endif
#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class NewGuiRenderBatch;

/// Cursor shapes, mirroring PlatformCursorController's own constants
/// one-to-one so a control's pushCursor() and a NewGuiCursor's image
/// lookup always refer to the same shape.
enum class NewGuiCursorShape : U8
{
   Arrow = 0,
   Wait,
   Crosshair,
   ResizeVertical,
   ResizeHorizontal,
   ResizeAll,
   Text,
   ResizeDiagonalNESW,
   ResizeDiagonalNWSE,
   Pointer,
   WaitArrow,
   NotAllowed,

   NumShapes   ///< Not a real shape - array bound.
};

/// Parses a cursor shape name.
/// @param str Shape name, e.g. "arrow", "wait".
/// @param outShape Receives the parsed shape.
/// @return True if the name was recognized.
bool NewGuiCursorShapeFromString(const char* str, NewGuiCursorShape& outShape);

/// Maps a shape onto PlatformCursorController's own integer constant.
/// @param shape Shape to map.
/// @return The platform cursor ID.
S32 NewGuiCursorShapeToPlatformId(NewGuiCursorShape shape);

/// A set of per-shape cursor images, bound to a NewGuiCanvas via
/// setCursor(). For any shape with a resolved image, the canvas hides
/// the platform cursor and draws that image at its hotspot instead; for
/// any shape without one, the real platform cursor is used unchanged.
class NewGuiCursor : public SimObject
{
public:

   typedef SimObject Parent;

protected:

   StringTableEntry mImagePath[(U32)NewGuiCursorShape::NumShapes];        ///< Authored image path per shape; empty means no override.
   GFXTexHandle mResolvedTexture[(U32)NewGuiCursorShape::NumShapes];      ///< Loaded texture per shape, kept in step with mImagePath by resolveImages().
   Point2I mHotspot[(U32)NewGuiCursorShape::NumShapes];                   ///< Click point per shape, in device pixels from the image's top-left.

#define DECLARE_CURSOR_SHAPE_SETTERS( name ) \
         static bool _set##name##Image( void* obj, const char* index, const char* data ); \
         static bool _set##name##Hotspot( void* obj, const char* index, const char* data );

   DECLARE_CURSOR_SHAPE_SETTERS(Arrow)
   DECLARE_CURSOR_SHAPE_SETTERS(Wait)
   DECLARE_CURSOR_SHAPE_SETTERS(Crosshair)
   DECLARE_CURSOR_SHAPE_SETTERS(ResizeVertical)
   DECLARE_CURSOR_SHAPE_SETTERS(ResizeHorizontal)
   DECLARE_CURSOR_SHAPE_SETTERS(ResizeAll)
   DECLARE_CURSOR_SHAPE_SETTERS(Text)
   DECLARE_CURSOR_SHAPE_SETTERS(ResizeDiagonalNESW)
   DECLARE_CURSOR_SHAPE_SETTERS(ResizeDiagonalNWSE)
   DECLARE_CURSOR_SHAPE_SETTERS(Pointer)
   DECLARE_CURSOR_SHAPE_SETTERS(WaitArrow)
   DECLARE_CURSOR_SHAPE_SETTERS(NotAllowed)

#undef DECLARE_CURSOR_SHAPE_SETTERS

public:

   NewGuiCursor();
   virtual ~NewGuiCursor();

   DECLARE_CONOBJECT(NewGuiCursor);

   static void initPersistFields();

   /// Loads every non-empty mImagePath[] entry into mResolvedTexture[].
   /// @note Currently a stub - no texture-from-path loading call was
   /// available to build against; this clears every resolved texture
   /// to null without loading anything.
   void resolveImages();

   /// @param shape Shape to check.
   /// @return True if this cursor has a resolved image for the given shape.
   bool hasImageForShape(NewGuiCursorShape shape) const;

   /// @param shape Shape to look up.
   /// @return The resolved texture for the given shape (may be null).
   const GFXTexHandle& getTexture(NewGuiCursorShape shape) const { return mResolvedTexture[(U32)shape]; }

   /// @param shape Shape to look up.
   /// @return The hotspot for the given shape.
   const Point2I& getHotspot(NewGuiCursorShape shape) const { return mHotspot[(U32)shape]; }

   /// Draws this shape's cursor image at screenPoint minus its hotspot. No-op if hasImageForShape(shape) is false.
   /// @param batch Render batch to draw into.
   /// @param shape Shape to draw.
   /// @param screenPoint Cursor position in device pixels.
   void submitDrawnCursor(NewGuiRenderBatch* batch, NewGuiCursorShape shape, const Point2I& screenPoint) const;
};

#endif // _NEWGUICURSOR_H_
