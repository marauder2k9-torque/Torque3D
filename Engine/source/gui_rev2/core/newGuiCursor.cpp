//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiCursor.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "windowManager/platformCursorController.h"
#include "gui_rev2/core/newGuiCursor.h"
#include "gui_rev2/core/newGuiRenderBatch.h"

IMPLEMENT_CONOBJECT(NewGuiCursor);

// Shape name + PlatformCursorController constant, one row per shape.
struct NewGuiCursorShapeInfo
{
   NewGuiCursorShape shape;
   const char* name;
   S32 platformId;
};

static const NewGuiCursorShapeInfo sShapeInfo[] =
{
   { NewGuiCursorShape::Arrow,               "arrow",              PlatformCursorController::curArrow },
   { NewGuiCursorShape::Wait,                "wait",               PlatformCursorController::curWait },
   { NewGuiCursorShape::Crosshair,           "crosshair",          PlatformCursorController::curPlus },
   { NewGuiCursorShape::ResizeVertical,      "resizeVertical",     PlatformCursorController::curResizeVert },
   { NewGuiCursorShape::ResizeHorizontal,    "resizeHorizontal",   PlatformCursorController::curResizeHorz },
   { NewGuiCursorShape::ResizeAll,           "resizeAll",          PlatformCursorController::curResizeAll },
   { NewGuiCursorShape::Text,                "text",               PlatformCursorController::curIBeam },
   { NewGuiCursorShape::ResizeDiagonalNESW,  "resizeDiagonalNESW", PlatformCursorController::curResizeNESW },
   { NewGuiCursorShape::ResizeDiagonalNWSE,  "resizeDiagonalNWSE", PlatformCursorController::curResizeNWSE },
   { NewGuiCursorShape::Pointer,             "pointer",            PlatformCursorController::curHand },
   { NewGuiCursorShape::WaitArrow,           "waitArrow",          PlatformCursorController::curWaitArrow },
   { NewGuiCursorShape::NotAllowed,          "notAllowed",         PlatformCursorController::curNoNo },
};
static const U32 sShapeCount = sizeof(sShapeInfo) / sizeof(sShapeInfo[0]);

bool NewGuiCursorShapeFromString(const char* str, NewGuiCursorShape& outShape)
{
   if (!str)
      return false;

   for (U32 i = 0; i < sShapeCount; i++)
   {
      if (dStricmp(str, sShapeInfo[i].name) == 0)
      {
         outShape = sShapeInfo[i].shape;
         return true;
      }
   }

   return false;
}

S32 NewGuiCursorShapeToPlatformId(NewGuiCursorShape shape)
{
   const U32 index = (U32)shape;
   if (index < sShapeCount)
      return sShapeInfo[index].platformId;

   return PlatformCursorController::curArrow;
}

NewGuiCursor::NewGuiCursor()
{
}

NewGuiCursor::~NewGuiCursor()
{
}

// One setter pair per shape, generated to avoid 12 hand-duplicated bodies.
#define DEFINE_CURSOR_SHAPE_SETTERS( name, shapeEnum ) \
   bool NewGuiCursor::_set##name##Image( void* obj, const char* index, const char* data ) \
   { \
      NewGuiCursor* cursor = static_cast<NewGuiCursor*>( obj ); \
      cursor->mImagePath[ (U32)( shapeEnum ) ] = StringTable->insert( data ); \
      return false; \
   } \
   bool NewGuiCursor::_set##name##Hotspot( void* obj, const char* index, const char* data ) \
   { \
      NewGuiCursor* cursor = static_cast<NewGuiCursor*>( obj ); \
      Point2I hotspot( 0, 0 ); \
      dSscanf( data, "%d %d", &hotspot.x, &hotspot.y ); \
      cursor->mHotspot[ (U32)( shapeEnum ) ] = hotspot; \
      return false; \
   }

DEFINE_CURSOR_SHAPE_SETTERS(Arrow, NewGuiCursorShape::Arrow)
DEFINE_CURSOR_SHAPE_SETTERS(Wait, NewGuiCursorShape::Wait)
DEFINE_CURSOR_SHAPE_SETTERS(Crosshair, NewGuiCursorShape::Crosshair)
DEFINE_CURSOR_SHAPE_SETTERS(ResizeVertical, NewGuiCursorShape::ResizeVertical)
DEFINE_CURSOR_SHAPE_SETTERS(ResizeHorizontal, NewGuiCursorShape::ResizeHorizontal)
DEFINE_CURSOR_SHAPE_SETTERS(ResizeAll, NewGuiCursorShape::ResizeAll)
DEFINE_CURSOR_SHAPE_SETTERS(Text, NewGuiCursorShape::Text)
DEFINE_CURSOR_SHAPE_SETTERS(ResizeDiagonalNESW, NewGuiCursorShape::ResizeDiagonalNESW)
DEFINE_CURSOR_SHAPE_SETTERS(ResizeDiagonalNWSE, NewGuiCursorShape::ResizeDiagonalNWSE)
DEFINE_CURSOR_SHAPE_SETTERS(Pointer, NewGuiCursorShape::Pointer)
DEFINE_CURSOR_SHAPE_SETTERS(WaitArrow, NewGuiCursorShape::WaitArrow)
DEFINE_CURSOR_SHAPE_SETTERS(NotAllowed, NewGuiCursorShape::NotAllowed)

#undef DEFINE_CURSOR_SHAPE_SETTERS

void NewGuiCursor::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Cursor Images");

   ADD_FIELD("arrowImage", TypeString, 0).onSet(_setArrowImage)
      .doc("Image file used as the drawn cursor for the arrow shape. Empty (default) falls back to the real platform cursor.");
   ADD_FIELD("arrowHotspot", TypeString, 0).onSet(_setArrowHotspot)
      .doc("Click/hotspot point within arrowImage, as \"x y\" device pixels from its top-left. Defaults to \"0 0\".");

   ADD_FIELD("waitImage", TypeString, 0).onSet(_setWaitImage)
      .doc("Image file used as the drawn cursor for the wait shape.");
   ADD_FIELD("waitHotspot", TypeString, 0).onSet(_setWaitHotspot)
      .doc("Click/hotspot point within waitImage, as \"x y\".");

   ADD_FIELD("crosshairImage", TypeString, 0).onSet(_setCrosshairImage)
      .doc("Image file used as the drawn cursor for the crosshair shape.");
   ADD_FIELD("crosshairHotspot", TypeString, 0).onSet(_setCrosshairHotspot)
      .doc("Click/hotspot point within crosshairImage, as \"x y\".");

   ADD_FIELD("resizeVerticalImage", TypeString, 0).onSet(_setResizeVerticalImage)
      .doc("Image file used as the drawn cursor for the vertical-resize shape.");
   ADD_FIELD("resizeVerticalHotspot", TypeString, 0).onSet(_setResizeVerticalHotspot)
      .doc("Click/hotspot point within resizeVerticalImage, as \"x y\".");

   ADD_FIELD("resizeHorizontalImage", TypeString, 0).onSet(_setResizeHorizontalImage)
      .doc("Image file used as the drawn cursor for the horizontal-resize shape.");
   ADD_FIELD("resizeHorizontalHotspot", TypeString, 0).onSet(_setResizeHorizontalHotspot)
      .doc("Click/hotspot point within resizeHorizontalImage, as \"x y\".");

   ADD_FIELD("resizeAllImage", TypeString, 0).onSet(_setResizeAllImage)
      .doc("Image file used as the drawn cursor for the resize-all shape.");
   ADD_FIELD("resizeAllHotspot", TypeString, 0).onSet(_setResizeAllHotspot)
      .doc("Click/hotspot point within resizeAllImage, as \"x y\".");

   ADD_FIELD("textImage", TypeString, 0).onSet(_setTextImage)
      .doc("Image file used as the drawn cursor for the text/I-beam shape.");
   ADD_FIELD("textHotspot", TypeString, 0).onSet(_setTextHotspot)
      .doc("Click/hotspot point within textImage, as \"x y\".");

   ADD_FIELD("resizeDiagonalNESWImage", TypeString, 0).onSet(_setResizeDiagonalNESWImage)
      .doc("Image file used as the drawn cursor for the NE-SW diagonal resize shape.");
   ADD_FIELD("resizeDiagonalNESWHotspot", TypeString, 0).onSet(_setResizeDiagonalNESWHotspot)
      .doc("Click/hotspot point within resizeDiagonalNESWImage, as \"x y\".");

   ADD_FIELD("resizeDiagonalNWSEImage", TypeString, 0).onSet(_setResizeDiagonalNWSEImage)
      .doc("Image file used as the drawn cursor for the NW-SE diagonal resize shape.");
   ADD_FIELD("resizeDiagonalNWSEHotspot", TypeString, 0).onSet(_setResizeDiagonalNWSEHotspot)
      .doc("Click/hotspot point within resizeDiagonalNWSEImage, as \"x y\".");

   ADD_FIELD("pointerImage", TypeString, 0).onSet(_setPointerImage)
      .doc("Image file used as the drawn cursor for the pointer/hand shape.");
   ADD_FIELD("pointerHotspot", TypeString, 0).onSet(_setPointerHotspot)
      .doc("Click/hotspot point within pointerImage, as \"x y\".");

   ADD_FIELD("waitArrowImage", TypeString, 0).onSet(_setWaitArrowImage)
      .doc("Image file used as the drawn cursor for the wait-arrow shape.");
   ADD_FIELD("waitArrowHotspot", TypeString, 0).onSet(_setWaitArrowHotspot)
      .doc("Click/hotspot point within waitArrowImage, as \"x y\".");

   ADD_FIELD("notAllowedImage", TypeString, 0).onSet(_setNotAllowedImage)
      .doc("Image file used as the drawn cursor for the not-allowed shape.");
   ADD_FIELD("notAllowedHotspot", TypeString, 0).onSet(_setNotAllowedHotspot)
      .doc("Click/hotspot point within notAllowedImage, as \"x y\".");

   GROUP_END("Cursor Images");
}

// Stub - no confirmed texture-from-path loading call was available. Clears
// resolved textures rather than leaving stale handles from a prior call.
void NewGuiCursor::resolveImages()
{
   for (U32 i = 0; i < sShapeCount; i++)
      mResolvedTexture[i] = GFXTexHandle();
}

bool NewGuiCursor::hasImageForShape(NewGuiCursorShape shape) const
{
   const U32 index = (U32)shape;
   if (index >= sShapeCount)
      return false;

   return !mResolvedTexture[index].isNull();
}

void NewGuiCursor::submitDrawnCursor(NewGuiRenderBatch* batch, NewGuiCursorShape shape, const Point2I& screenPoint) const
{
   if (!batch || !hasImageForShape(shape))
      return;

   const U32 index = (U32)shape;

   // High enough that no real control's layer (always parentLayer + 1 from the canvas) could reach it.
   static const S32 kCursorLayer = 100000;

   GFXTextureObject* texObj = mResolvedTexture[index].getPointer();
   if (!texObj)
      return;

   Point2I frameSize((S32)texObj->getWidth(), (S32)texObj->getHeight());

   RectI deviceRect(screenPoint - mHotspot[index], frameSize);

   batch->pushTexturedQuad(
      deviceRect,
      mResolvedTexture[index],
      Point2F(0.0f, 0.0f),
      Point2F(1.0f, 1.0f),
      frameSize,
      ColorI(255, 255, 255, 255),
      kCursorLayer);
}
