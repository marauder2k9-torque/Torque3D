//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiStyle.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/core/newGuiStyle.h"
#include "console/typeValidators.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "math/mRect.h"

IMPLEMENT_CONOBJECT(NewGuiStyle);
IMPLEMENT_CONOBJECT(NewGuiSkinImageDef);

//-----------------------------------------------------------------------------
// NewGuiSkinFillMode
//-----------------------------------------------------------------------------

bool NewGuiSkinFillModeFromString(const char* str, NewGuiSkinFillMode& out)
{
   if (dStricmp(str, "stretch") == 0) { out = NewGuiSkinFill_Stretch; return true; }
   if (dStricmp(str, "tile") == 0) { out = NewGuiSkinFill_Tile; return true; }
   if (dStricmp(str, "tileHorizontal") == 0) { out = NewGuiSkinFill_TileHorizontal; return true; }
   if (dStricmp(str, "tileVertical") == 0) { out = NewGuiSkinFill_TileVertical; return true; }
   return false;
}

const char* NewGuiSkinFillModeToString(NewGuiSkinFillMode mode)
{
   switch (mode)
   {
   case NewGuiSkinFill_Tile:            return "tile";
   case NewGuiSkinFill_TileHorizontal:  return "tileHorizontal";
   case NewGuiSkinFill_TileVertical:    return "tileVertical";
   case NewGuiSkinFill_Stretch:
   default:                             return "stretch";
   }
}

//-----------------------------------------------------------------------------
// NewGuiSkinImage
//-----------------------------------------------------------------------------

NewGuiSkinImage::NewGuiSkinImage()
   : frameName(NULL),
   centerFill(NewGuiSkinFill_Stretch),
   edgeFillHorizontal(NewGuiSkinFill_Stretch),
   edgeFillVertical(NewGuiSkinFill_Stretch),
   tint(255, 255, 255, 255),
   allowHardwareWrap(false)
{
}

//-----------------------------------------------------------------------------
// NewGuiResolvedStyle
//-----------------------------------------------------------------------------

NewGuiResolvedStyle::NewGuiResolvedStyle()
   : textColor(0, 0, 0, 255),
   backgroundColor(255, 255, 255, 255),
   borderColor(0, 0, 0, 255),
   secondaryColor(120, 120, 120, 255),
   secondaryColorAuthored(false),
   fontFamily(NULL),
   fontSize(14.0f),
   padding(),
   margin(),
   borderWidth(0.0f),
   opacity(1.0f),
   visible(true),
   generation(0)
{
}

const NewGuiSkinImage* NewGuiResolvedStyle::findSkinImage(StringTableEntry partName) const
{
   for (U32 i = 0; i < skinImageNames.size(); ++i)
   {
      if (skinImageNames[i] == partName)
         return &skinImages[i];
   }
   return NULL;
}

void NewGuiResolvedStyle::setSkinImage(StringTableEntry partName, const NewGuiSkinImage& img)
{
   for (U32 i = 0; i < skinImageNames.size(); ++i)
   {
      if (skinImageNames[i] == partName)
      {
         skinImages[i] = img;
         return;
      }
   }

   skinImageNames.push_back(partName);
   skinImages.push_back(img);
}

NewGuiStyle::NewGuiStyle()
   : mTextColor(0, 0, 0, 255),
   mBackgroundColor(255, 255, 255, 255),
   mBorderColor(0, 0, 0, 255),
   mSecondaryColor(120, 120, 120, 255),
   mFontFamily(NULL),
   mFontSize(14.0f),
   mPadding(),
   mMargin(),
   mBorderWidth(0.0f),
   mOpacity(1.0f),
   mVisible(true),
   mSetMask(0),
   mStateMask(NewGuiState_Normal),
   mPriority(0)
{
}

NewGuiStyle::~NewGuiStyle()
{
}

bool NewGuiStyle::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

// Each setter stamps mSetMask so Cascade() can tell "authored" apart from "default."
bool NewGuiStyle::_setTextColor(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   castConsoleTypeFromString(style->mTextColor, data);
   style->mSetMask |= Bit_TextColor;
   return false;
}

bool NewGuiStyle::_setBackgroundColor(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   castConsoleTypeFromString(style->mBackgroundColor, data);
   style->mSetMask |= Bit_BackgroundColor;
   return false;
}

bool NewGuiStyle::_setBorderColor(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   castConsoleTypeFromString(style->mBorderColor, data);
   style->mSetMask |= Bit_BorderColor;
   return false;
}

bool NewGuiStyle::_setSecondaryColor(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   castConsoleTypeFromString(style->mSecondaryColor, data);
   style->mSetMask |= Bit_SecondaryColor;
   return false;
}

bool NewGuiStyle::_setFontFamily(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   style->mFontFamily = StringTable->insert(data);
   style->mSetMask |= Bit_FontFamily;
   return false;
}

bool NewGuiStyle::_setFontSize(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   style->mFontSize = (F32)dAtof(data);
   style->mSetMask |= Bit_FontSize;
   return false;
}

bool NewGuiStyle::_setPadding(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   NewGuiEdgeInsets::setFromString(style->mPadding, data);
   style->mSetMask |= Bit_Padding;
   return false;
}

bool NewGuiStyle::_setMargin(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   NewGuiEdgeInsets::setFromString(style->mMargin, data);
   style->mSetMask |= Bit_Margin;
   return false;
}

bool NewGuiStyle::_setBorderWidth(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   style->mBorderWidth = (F32)dAtof(data);
   style->mSetMask |= Bit_BorderWidth;
   return false;
}

bool NewGuiStyle::_setOpacity(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   style->mOpacity = mClampF((F32)dAtof(data), 0.0f, 1.0f);
   style->mSetMask |= Bit_Opacity;
   return false;
}

bool NewGuiStyle::_setVisible(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   style->mVisible = dAtob(data);
   style->mSetMask |= Bit_Visible;
   return false;
}

// Parses pipe-delimited state names, e.g. "hover|active". Unrecognized tokens -> Normal.
static NewGuiStyleStateMask parseSingleStateToken(const char* token)
{
   if (dStricmp(token, "hover") == 0)     return NewGuiState_Hover;
   if (dStricmp(token, "active") == 0)    return NewGuiState_Active;
   if (dStricmp(token, "focus") == 0)     return NewGuiState_Focus;
   if (dStricmp(token, "disabled") == 0)  return NewGuiState_Disabled;
   if (dStricmp(token, "checked") == 0)   return NewGuiState_Checked;
   if (dStricmp(token, "error") == 0)     return NewGuiState_Error;
   if (dStricmp(token, "normal") == 0)    return NewGuiState_Normal;
   return NewGuiState_Normal;
}

bool NewGuiStyle::_setStateMask(void* obj, const char* index, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);

   NewGuiStyleStateMask mask = NewGuiState_Normal;

   char buffer[256];
   dStrncpy(buffer, data, sizeof(buffer) - 1);
   buffer[sizeof(buffer) - 1] = '\0';

   char* token = dStrtok(buffer, "|");
   while (token)
   {
      mask |= parseSingleStateToken(token);
      token = dStrtok(NULL, "|");
   }

   style->mStateMask = mask;
   return false;
}

const char* NewGuiStyle::_getStateMask(void* obj, const char* data)
{
   NewGuiStyle* style = static_cast<NewGuiStyle*>(obj);
   NewGuiStyleStateMask mask = style->mStateMask;

   if (mask == NewGuiState_Normal)
      return "normal";

   static const U32 bufSize = 128;
   char* buf = Con::getReturnBuffer(bufSize);
   buf[0] = '\0';

   bool first = true;
   auto append = [&](const char* name)
   {
      if (!first)
         dStrcat(buf, "|", bufSize);
      dStrcat(buf, name, bufSize);
      first = false;
   };

   if (mask & NewGuiState_Hover)     append("hover");
   if (mask & NewGuiState_Active)    append("active");
   if (mask & NewGuiState_Focus)     append("focus");
   if (mask & NewGuiState_Disabled)  append("disabled");
   if (mask & NewGuiState_Checked)   append("checked");
   if (mask & NewGuiState_Error)     append("error");

   return buf;
}

void NewGuiStyle::initPersistFields()
{
   Parent::initPersistFields();

   ADD_FIELD("textColor", TypeColorI, Offset(mTextColor, NewGuiStyle))
      .onSet(_setTextColor)
      .doc("Foreground/text color, as \"r g b [a]\" 0-255 integers (or a stock color name).");

   ADD_FIELD("backgroundColor", TypeColorI, Offset(mBackgroundColor, NewGuiStyle))
      .onSet(_setBackgroundColor)
      .doc("Fill color for this control's background, as \"r g b [a]\" 0-255 integers (or a stock color name).");

   ADD_FIELD("borderColor", TypeColorI, Offset(mBorderColor, NewGuiStyle))
      .onSet(_setBorderColor)
      .doc("Stroke color for this control's border, as \"r g b [a]\" 0-255 integers (or a stock color name).");

   ADD_FIELD("secondaryColor", TypeColorI, Offset(mSecondaryColor, NewGuiStyle))
      .onSet(_setSecondaryColor)
      .doc("General-purpose second color for a control's own extra parts (scrollbar thumb, checkbox check mark, slider fill, ...) that aren't background/border/text. "
         "As \"r g b [a]\" 0-255 integers (or a stock color name). If never authored anywhere in the cascade, defaults to the resolved textColor.");

   ADD_FIELD("fontFamily", TypeString, Offset(mFontFamily, NewGuiStyle))
      .onSet(_setFontFamily)
      .doc("Font family name used for text content.");

   ADD_FIELD("fontSize", TypeF32, Offset(mFontSize, NewGuiStyle))
      .onSet(_setFontSize)
      .doc("Font size in logical units.");

   ADD_FIELD("padding", TypeNewGuiEdgeInsets, Offset(mPadding, NewGuiStyle))
      .onSet(_setPadding)
      .doc("Inset between this control's border and its client content, as \"top right bottom left\" (CSS shorthand rules apply).");

   ADD_FIELD("margin", TypeNewGuiEdgeInsets, Offset(mMargin, NewGuiStyle))
      .onSet(_setMargin)
      .doc("Space reserved outside this control's border, same shorthand rules as padding.");

   ADD_FIELD("borderWidth", TypeF32, Offset(mBorderWidth, NewGuiStyle))
      .onSet(_setBorderWidth)
      .doc("Border stroke width in logical units.");

   ADD_FIELD("opacity", TypeF32, Offset(mOpacity, NewGuiStyle))
      .onSet(_setOpacity)
      .validate(new FRangeValidator(0.0f, 1.0f))
      .doc("Overall opacity multiplier, 0-1.");

   ADD_FIELD("visible", TypeBool, Offset(mVisible, NewGuiStyle))
      .onSet(_setVisible)
      .doc("Style-driven visibility.");

   ADD_FIELD("stateMask", TypeString, 0)
      .onSet(_setStateMask)
      .onGet(_getStateMask)
      .doc("Interaction state(s) this rule applies under: normal (default), hover, active, focus, disabled, checked, error. Combine with '|', e.g. \"hover|active\".");

   ADD_FIELD("priority", TypeS32, Offset(mPriority, NewGuiStyle))
      .doc("Tiebreaker among sibling rules of equal specificity; higher wins.");
}

// Overwrites only the properties this rule authored, leaving everything else in io untouched.
// Skin image defs are a slightly different shape than the flat mSetMask fields (there's no fixed
// bit for "did this rule set the background skin image" since part names are open-ended), so
// they're found the same way Cascade() finds nested state-rule children: walk this rule's direct
// children and dynamic_cast, applying every NewGuiSkinImageDef found. A rule with none is a no-op
// here, same as an unset flat field.
void NewGuiStyle::applyTo(NewGuiResolvedStyle& io) const
{
   if (mSetMask & Bit_TextColor)        io.textColor = mTextColor;
   if (mSetMask & Bit_BackgroundColor)  io.backgroundColor = mBackgroundColor;
   if (mSetMask & Bit_BorderColor)      io.borderColor = mBorderColor;
   if (mSetMask & Bit_SecondaryColor) { io.secondaryColor = mSecondaryColor; io.secondaryColorAuthored = true; }
   if (mSetMask & Bit_FontFamily)       io.fontFamily = mFontFamily;
   if (mSetMask & Bit_FontSize)         io.fontSize = mFontSize;
   if (mSetMask & Bit_Padding)          io.padding = mPadding;
   if (mSetMask & Bit_Margin)           io.margin = mMargin;
   if (mSetMask & Bit_BorderWidth)      io.borderWidth = mBorderWidth;
   if (mSetMask & Bit_Opacity)          io.opacity = mOpacity;
   if (mSetMask & Bit_Visible)          io.visible = mVisible;

   for (SimSet::const_iterator itr = begin(); itr != end(); ++itr)
   {
      const NewGuiSkinImageDef* def = dynamic_cast<const NewGuiSkinImageDef*>(*itr);
      if (def)
         io.setSkinImage(def->getPartName(), def->getImage());
   }
}

// A rule applies if every bit it sets is also set on the control's current state.
static bool stateMaskApplies(NewGuiStyleStateMask ruleMask, NewGuiStyleStateMask controlMask)
{
   return (ruleMask & controlMask) == ruleMask;
}

NewGuiResolvedStyle NewGuiStyle::Cascade(const NewGuiResolvedStyle& inherited, NewGuiStyle* ownStyle, NewGuiStyleStateMask stateMask)
{
   NewGuiResolvedStyle result = inherited;

   // Background/border/skin images are per-control paint, not inherited state - reset before layering.
   result.backgroundColor = ColorI(0, 0, 0, 0);
   result.borderColor = ColorI(0, 0, 0, 0);
   result.borderWidth = 0.0f;
   result.skinImageNames.clear();
   result.skinImages.clear();

   if (!ownStyle)
   {
      if (!result.secondaryColorAuthored)
         result.secondaryColor = result.textColor;
      return result;
   }

   // Base rule - this style's own directly-authored fields.
   ownStyle->applyTo(result);

   // Matching child (state-scoped) rules, sorted by (priority, tree order).
   Vector<const NewGuiStyle*> matching;
   for (SimSet::const_iterator itr = ownStyle->begin(); itr != ownStyle->end(); ++itr)
   {
      const NewGuiStyle* childRule = dynamic_cast<const NewGuiStyle*>(*itr);
      if (!childRule)
         continue;

      if (stateMaskApplies(childRule->getStateMask(), stateMask))
         matching.push_back(childRule);
   }

   // Stable insertion sort by priority - lists are expected to be small.
   for (U32 i = 1; i < matching.size(); ++i)
   {
      const NewGuiStyle* key = matching[i];
      S32 j = S32(i) - 1;
      while (j >= 0 && matching[j]->getPriority() > key->getPriority())
      {
         matching[j + 1] = matching[j];
         --j;
      }
      matching[j + 1] = key;
   }

   for (U32 i = 0; i < matching.size(); ++i)
      matching[i]->applyTo(result);

   // secondaryColor defaults to textColor whenever nothing in the whole cascade (inherited chain
   // included) ever authored it - keeps it "always something reasonable" without needing every
   // style to explicitly set it just to get a usable thumb/check-mark/etc color.
   if (!result.secondaryColorAuthored)
      result.secondaryColor = result.textColor;

   return result;
}

//-----------------------------------------------------------------------------
// NewGuiSkinImageDef
//-----------------------------------------------------------------------------

NewGuiSkinImageDef::NewGuiSkinImageDef()
   : mPartName(NULL)
{
}

NewGuiSkinImageDef::~NewGuiSkinImageDef()
{
}

bool NewGuiSkinImageDef::_setImage(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   def->mImage.image = data;
   return false;
}

bool NewGuiSkinImageDef::_setFrame(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   def->mImage.frameName = StringTable->insert(data);
   return false;
}

bool NewGuiSkinImageDef::_setInsets(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   NewGuiEdgeInsets::setFromString(def->mImage.insets, data);
   return false;
}

bool NewGuiSkinImageDef::_setCenterFill(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   NewGuiSkinFillMode mode;
   if (NewGuiSkinFillModeFromString(data, mode))
      def->mImage.centerFill = mode;
   return false;
}

bool NewGuiSkinImageDef::_setEdgeFillHorizontal(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   NewGuiSkinFillMode mode;
   if (NewGuiSkinFillModeFromString(data, mode))
      def->mImage.edgeFillHorizontal = mode;
   return false;
}

bool NewGuiSkinImageDef::_setEdgeFillVertical(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   NewGuiSkinFillMode mode;
   if (NewGuiSkinFillModeFromString(data, mode))
      def->mImage.edgeFillVertical = mode;
   return false;
}

bool NewGuiSkinImageDef::_setTint(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   castConsoleTypeFromString(def->mImage.tint, data);
   return false;
}

bool NewGuiSkinImageDef::_setAllowHardwareWrap(void* obj, const char* index, const char* data)
{
   NewGuiSkinImageDef* def = static_cast<NewGuiSkinImageDef*>(obj);
   def->mImage.allowHardwareWrap = dAtob(data);
   return false;
}

void NewGuiSkinImageDef::initPersistFields()
{
   Parent::initPersistFields();

   ADD_FIELD("partName", TypeString, Offset(mPartName, NewGuiSkinImageDef))
      .doc("Name of the part this def paints, e.g. \"background\", \"border\", or a name a specific control type defines (\"thumb\", \"titlebar\", ...).");

   ADD_FIELD("image", TypeImageAssetPtr, 0)
      .onSet(_setImage)
      .doc("ImageAsset this part's texture is sourced from.");

   ADD_FIELD("frame", TypeString, 0)
      .onSet(_setFrame)
      .doc("Named frame within image to use (see ImageAsset frames). Empty = frame 0 / whole image.");

   ADD_FIELD("insets", TypeNewGuiEdgeInsets, 0)
      .onSet(_setInsets)
      .doc("9-slice insets in source-image texels: \"top right bottom left\". All zero = no slicing, the whole frame is one stretched/tiled quad.");

   ADD_FIELD("centerFill", TypeString, 0)
      .onSet(_setCenterFill)
      .doc("Fill mode for the center segment: stretch (default), tile, tileHorizontal, tileVertical.");

   ADD_FIELD("edgeFillHorizontal", TypeString, 0)
      .onSet(_setEdgeFillHorizontal)
      .doc("Fill mode for the top/bottom edge segments: stretch (default) or tile.");

   ADD_FIELD("edgeFillVertical", TypeString, 0)
      .onSet(_setEdgeFillVertical)
      .doc("Fill mode for the left/right edge segments: stretch (default) or tile.");

   ADD_FIELD("tint", TypeColorI, 0)
      .onSet(_setTint)
      .doc("Tint multiplied with the control's resolved opacity at draw time. Default white/untinted.");

   ADD_FIELD("allowHardwareWrap", TypeBool, 0)
      .onSet(_setAllowHardwareWrap)
      .doc("Opts a Tile* fill mode on this image into hardware-wrapped tiling (GFXAddressWrap) instead of the default CPU-side multi-quad tiling, once that draw path exists - see NewGuiSkinImage::allowHardwareWrap's own doc comment. Only correct for a standalone, non-atlased image. Default false. Currently has no effect - NewGuiStyleDrawSkinImage() always tiles via CPU-side quads regardless of this flag.");
}

//-----------------------------------------------------------------------------
// NewGuiStyleDrawSkinImage
//-----------------------------------------------------------------------------

// Pushes one 9-slice segment. Stretch is the plain single-quad case (always GFXAddressClamp -
// this is deliberately CPU-side tiling, not hardware wrap: a skin frame is typically one region
// inside a shared atlas texture, and GFXAddressWrap wraps the WHOLE texture's 0..1 space, not
// just the frame's own sub-rect, which would sample into whatever's atlased next to it. Wrap
// addressing stays available on NewGuiRenderBatch::pushTexturedQuad() itself for a designer who
// explicitly wants it against a standalone, non-atlased texture - it's just not used here as an
// automatic substitute for tiling). A tiled axis instead pushes one quad per native-texel-sized
// repeat, each sampling its own correct in-frame UV sub-rect, with the final partial repeat (if
// the destination span isn't an exact multiple of the native size) narrowed on both its
// destination rect and its UV span together, so it neither stretches nor bleeds past the frame.
static void pushSkinSegment(
   NewGuiRenderBatch* batch,
   const RectI& deviceRect,
   GFXTexHandle texture,
   const Point2F& texelLower,
   const Point2F& texelSpan,       // texelUpper - texelLower for one untiled repeat, kept separate so it can be scaled per partial repeat.
   const Point2I& nativePixelSize,
   NewGuiSkinFillMode fillMode,
   const ColorI& color,
   S32 layer)
{
   if (deviceRect.extent.x <= 0 || deviceRect.extent.y <= 0)
      return;

   const bool tileX = (fillMode == NewGuiSkinFill_Tile || fillMode == NewGuiSkinFill_TileHorizontal) && nativePixelSize.x > 0;
   const bool tileY = (fillMode == NewGuiSkinFill_Tile || fillMode == NewGuiSkinFill_TileVertical) && nativePixelSize.y > 0;

   if (!tileX && !tileY)
   {
      // Straight stretch - single quad covers the whole segment.
      batch->pushTexturedQuad(deviceRect, texture, texelLower, Point2F(texelLower.x + texelSpan.x, texelLower.y + texelSpan.y), nativePixelSize, color, layer);
      return;
   }

   const S32 stepX = tileX ? nativePixelSize.x : deviceRect.extent.x;
   const S32 stepY = tileY ? nativePixelSize.y : deviceRect.extent.y;

   for (S32 y = 0; y < deviceRect.extent.y; y += stepY)
   {
      const S32 tileH = getMin(stepY, deviceRect.extent.y - y);
      for (S32 x = 0; x < deviceRect.extent.x; x += stepX)
      {
         const S32 tileW = getMin(stepX, deviceRect.extent.x - x);

         RectI tileRect(deviceRect.point + Point2I(x, y), Point2I(tileW, tileH));

         // A partial tile at the far edge samples only the covered fraction of the source
         // segment's own UV span, rather than stretching the full span into a smaller quad.
         Point2F thisSpan(
            texelSpan.x * (F32)tileW / (F32)stepX,
            texelSpan.y * (F32)tileH / (F32)stepY);

         batch->pushTexturedQuad(tileRect, texture, texelLower, Point2F(texelLower.x + thisSpan.x, texelLower.y + thisSpan.y), Point2I(tileW, tileH), color, layer);
      }
   }
}

void NewGuiStyleDrawSkinImage(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiSkinImage& img, F32 opacity, S32 layer)
{
   if (!batch || !img.hasImage())
      return;

   if (bounds.extent.x <= 0 || bounds.extent.y <= 0)
      return;

   ImageAsset* image = img.image;
   if (!image)
      return;

   // frame accepts either a named cell ("button_normal", from ExplicitCells - see ImageAsset)
   // or a plain numeric index ("0", "1", ...) into an implicit frameWidth/frameHeight grid, which
   // has no names of its own. Name lookup is tried first since it's unambiguous; a bare integer
   // string falls back to index lookup only if the name lookup found nothing.
   const ImageAsset::Frame* frame = NULL;
   if (img.frameName && img.frameName[0])
   {
      frame = image->findFrameByName(img.frameName);
      if (!frame && dIsdigit(img.frameName[0]))
         frame = image->getFrame(dAtoi(img.frameName));
   }
   else
   {
      frame = image->getFrame(0);
   }

   if (!frame)
      return;

   // GFXDefaultGUIProfile - PreserveSize/NoPadding (no forced power-of-two padding, which matters
   // for a non-square atlas like our 4-frame strip), SRGB, and no mip generation regardless of the
   // asset's own UseMips setting - the correct, purpose-built profile for GUI skin art, rather
   // than a generic color-texture profile.
   GFXTexHandle texture = image->getTexture(&GFXDefaultGUIProfile);
   if (texture.isNull())
      return;

   const ColorI tint(
      img.tint.red,
      img.tint.green,
      img.tint.blue,
      (U8)((F32)img.tint.alpha * mClampF(opacity, 0.0f, 1.0f)));

   if (tint.alpha == 0)
      return;

   const S32 srcW = frame->pixelSize.x;
   const S32 srcH = frame->pixelSize.y;

   const bool sliced = (img.insets.left > 0.0f || img.insets.right > 0.0f || img.insets.top > 0.0f || img.insets.bottom > 0.0f);

   const S32 srcInsetLeft = (S32)img.insets.left;
   const S32 srcInsetTop = (S32)img.insets.top;
   const S32 srcInsetRight = (S32)img.insets.right;
   const S32 srcInsetBottom = (S32)img.insets.bottom;
   const S32 srcCenterW = getMax(0, srcW - srcInsetLeft - srcInsetRight);
   const S32 srcCenterH = getMax(0, srcH - srcInsetTop - srcInsetBottom);

   // Fixed destination sizes = source texel counts, 1:1 - no bounds-derived scaling at all.
   const S32 insetLeft = getMin(srcInsetLeft, bounds.extent.x);
   const S32 insetRight = getMin(srcInsetRight, getMax(0, bounds.extent.x - insetLeft));
   const S32 insetTop = getMin(srcInsetTop, bounds.extent.y);
   const S32 insetBottom = getMin(srcInsetBottom, getMax(0, bounds.extent.y - insetTop));

   // Texel-space helper: maps a pixel-space sub-rect of the frame to normalized UVs.
   const Point2F texelPerPixel(
      (frame->texelUpper.x - frame->texelLower.x) / (F32)getMax(1, srcW),
      (frame->texelUpper.y - frame->texelLower.y) / (F32)getMax(1, srcH));

   auto uvFor = [&](S32 px, S32 py, S32 pw, S32 ph, Point2F& outLower, Point2F& outSpan)
   {
      outLower.x = frame->texelLower.x + (F32)px * texelPerPixel.x;
      outLower.y = frame->texelLower.y + (F32)py * texelPerPixel.y;
      outSpan.x = (F32)pw * texelPerPixel.x;
      outSpan.y = (F32)ph * texelPerPixel.y;
   };

   if (!sliced)
   {
      // No 9-slice insets authored - the whole frame is just the (possibly tiled) center segment.
      Point2F lower, span;
      uvFor(0, 0, srcW, srcH, lower, span);
      pushSkinSegment(batch, bounds, texture, lower, span, Point2I(srcW, srcH), img.centerFill, tint, layer);
      return;
   }

   const S32 left = bounds.point.x;
   const S32 top = bounds.point.y;
   const S32 right = bounds.point.x + bounds.extent.x;
   const S32 bottom = bounds.point.y + bounds.extent.y;

   const S32 centerLeft = left + insetLeft;
   const S32 centerTop = top + insetTop;
   const S32 centerRight = right - insetRight;
   const S32 centerBottom = bottom - insetBottom;

   Point2F lower, span;

   // Corners - drawn at their fixed native source-texel size (see above), unstretched on both
   // axes.
   if (insetLeft > 0 && insetTop > 0 && srcInsetLeft > 0 && srcInsetTop > 0)
   {
      uvFor(0, 0, srcInsetLeft, srcInsetTop, lower, span);
      pushSkinSegment(batch, RectI(Point2I(left, top), Point2I(insetLeft, insetTop)), texture, lower, span, Point2I(srcInsetLeft, srcInsetTop), NewGuiSkinFill_Stretch, tint, layer);
   }
   if (insetRight > 0 && insetTop > 0 && srcInsetRight > 0 && srcInsetTop > 0)
   {
      uvFor(srcW - srcInsetRight, 0, srcInsetRight, srcInsetTop, lower, span);
      pushSkinSegment(batch, RectI(Point2I(centerRight, top), Point2I(insetRight, insetTop)), texture, lower, span, Point2I(srcInsetRight, srcInsetTop), NewGuiSkinFill_Stretch, tint, layer);
   }
   if (insetLeft > 0 && insetBottom > 0 && srcInsetLeft > 0 && srcInsetBottom > 0)
   {
      uvFor(0, srcH - srcInsetBottom, srcInsetLeft, srcInsetBottom, lower, span);
      pushSkinSegment(batch, RectI(Point2I(left, centerBottom), Point2I(insetLeft, insetBottom)), texture, lower, span, Point2I(srcInsetLeft, srcInsetBottom), NewGuiSkinFill_Stretch, tint, layer);
   }
   if (insetRight > 0 && insetBottom > 0 && srcInsetRight > 0 && srcInsetBottom > 0)
   {
      uvFor(srcW - srcInsetRight, srcH - srcInsetBottom, srcInsetRight, srcInsetBottom, lower, span);
      pushSkinSegment(batch, RectI(Point2I(centerRight, centerBottom), Point2I(insetRight, insetBottom)), texture, lower, span, Point2I(srcInsetRight, srcInsetBottom), NewGuiSkinFill_Stretch, tint, layer);
   }

   // Top/bottom edges - stretch/tile along X only.
   if (insetTop > 0 && srcInsetTop > 0 && centerRight > centerLeft)
   {
      uvFor(srcInsetLeft, 0, srcCenterW, srcInsetTop, lower, span);
      pushSkinSegment(batch, RectI(Point2I(centerLeft, top), Point2I(centerRight - centerLeft, insetTop)),
         texture, lower, span, Point2I(srcCenterW, srcInsetTop), img.edgeFillHorizontal, tint, layer);
   }
   if (insetBottom > 0 && srcInsetBottom > 0 && centerRight > centerLeft)
   {
      uvFor(srcInsetLeft, srcH - srcInsetBottom, srcCenterW, srcInsetBottom, lower, span);
      pushSkinSegment(batch, RectI(Point2I(centerLeft, centerBottom), Point2I(centerRight - centerLeft, insetBottom)),
         texture, lower, span, Point2I(srcCenterW, srcInsetBottom), img.edgeFillHorizontal, tint, layer);
   }

   // Left/right edges - stretch/tile along Y only, destination-scaled width along X.
   if (insetLeft > 0 && srcInsetLeft > 0 && centerBottom > centerTop)
   {
      uvFor(0, srcInsetTop, srcInsetLeft, srcCenterH, lower, span);
      pushSkinSegment(batch, RectI(Point2I(left, centerTop), Point2I(insetLeft, centerBottom - centerTop)),
         texture, lower, span, Point2I(srcInsetLeft, srcCenterH), img.edgeFillVertical, tint, layer);
   }
   if (insetRight > 0 && srcInsetRight > 0 && centerBottom > centerTop)
   {
      uvFor(srcW - srcInsetRight, srcInsetTop, srcInsetRight, srcCenterH, lower, span);
      pushSkinSegment(batch, RectI(Point2I(centerRight, centerTop), Point2I(insetRight, centerBottom - centerTop)),
         texture, lower, span, Point2I(srcInsetRight, srcCenterH), img.edgeFillVertical, tint, layer);
   }

   // Center segment.
   if (centerRight > centerLeft && centerBottom > centerTop && srcCenterW > 0 && srcCenterH > 0)
   {
      uvFor(srcInsetLeft, srcInsetTop, srcCenterW, srcCenterH, lower, span);
      pushSkinSegment(batch, RectI(Point2I(centerLeft, centerTop), Point2I(centerRight - centerLeft, centerBottom - centerTop)),
         texture, lower, span, Point2I(srcCenterW, srcCenterH), img.centerFill, tint, layer);
   }
}
