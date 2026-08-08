//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiStyle.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUISTYLE_H_
#define _NEWGUISTYLE_H_

#ifndef _SIMSET_H_
#include "sim/simSet.h"
#endif
#ifndef _NEWGUITYPES_H_
#include "gui_rev2/core/newGuiTypes.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif

#include "T3D/assets/ImageAsset.h"


/// How a skin image's center and edge 9-slice segments are painted across their destination
/// span once the corners (always drawn at native source size) are accounted for.
enum NewGuiSkinFillMode : U8
{
   NewGuiSkinFill_Stretch = 0,     ///< Segment is stretched to fill its destination span.
   NewGuiSkinFill_Tile,            ///< Segment repeats at native texel size along both axes it spans.
   NewGuiSkinFill_TileHorizontal,  ///< Repeats along X, stretched along Y.
   NewGuiSkinFill_TileVertical,    ///< Repeats along Y, stretched along X.
};

bool NewGuiSkinFillModeFromString(const char* str, NewGuiSkinFillMode& out);
const char* NewGuiSkinFillModeToString(NewGuiSkinFillMode mode);

/// One named, image-backed visual part of a control's resolved style (e.g. "background",
/// "border", or a part name a specific control type defines, like "thumb"/"titlebar"). Slices
/// its source frame into a 3x3 grid via insets - corners are drawn at native size, edges
/// stretch/tile along their one free axis, and the center stretches/tiles across both, the same
/// convention as CSS border-image. This is a resolved OUTPUT struct (part of NewGuiResolvedStyle),
/// not something authored directly - see NewGuiSkinImageDef for the authoring side.
struct NewGuiSkinImage
{
   AssetPtr<ImageAsset> image;         ///< Unset (isNull()) means no skin image for this part - the owning control falls back to flat-color painting for it.
   StringTableEntry     frameName;     ///< Named frame within image, or NULL/empty for frame 0.
   NewGuiEdgeInsets      insets;        ///< 9-slice insets in source-image texel space. All zero = unsliced, whole frame is the (possibly tiled) center segment.
   NewGuiSkinFillMode    centerFill;
   NewGuiSkinFillMode    edgeFillHorizontal;   ///< Fill mode for the top/bottom edge segments.
   NewGuiSkinFillMode    edgeFillVertical;     ///< Fill mode for the left/right edge segments.
   ColorI                tint;          ///< Multiplied with the control's resolved opacity at draw time.

   /// Designer opt-in for hardware-wrapped tiling (GFXAddressWrap) instead
   bool                   allowHardwareWrap;

   NewGuiSkinImage();

   bool hasImage() const { return image.notNull(); }
};

/// The fully-resolved, cascaded style output for one control, for one frame.
struct NewGuiResolvedStyle
{
   ColorI   textColor;
   ColorI   backgroundColor;
   ColorI   borderColor;

   /// General-purpose second color, for a control's own extra parts that aren't background/border/
   /// text - e.g. a scrollbar thumb, a checkbox's check mark, a slider's fill. Exists so those
   /// parts have a real style-driven color of their own instead of reusing textColor/backgroundColor
   /// as a stand-in. A control's skin images still use their own descriptive part names ("thumb",
   /// "track", "check", ...) via findSkinImage() - this is only the flat-color fallback layer.
   /// If never authored anywhere in the cascade, NewGuiStyle::Cascade() defaults this to the
   /// resolved textColor (see secondaryColorAuthored) so there's always something reasonable to draw.
   ColorI   secondaryColor;
   bool     secondaryColorAuthored;   ///< True once any rule in the cascade has actually set secondaryColor; false still means "use textColor".

   StringTableEntry fontFamily;
   F32            fontSize;

   NewGuiEdgeInsets padding;
   NewGuiEdgeInsets margin;
   F32            borderWidth;

   F32            opacity;
   bool           visible;      ///< Style-driven visibility, distinct from NewGuiControl::mVisible.

   /// Resolved skin images, keyed by part name ("background", "border", or a control-defined
   /// name). Small - a control typically skins a handful of parts - so linear scan by name is
   /// simpler and cheaper here than a hash map. Reset to empty at the top of every Cascade() (skin
   /// images are per-control paint, not inherited state, same as backgroundColor/borderColor).
   Vector<StringTableEntry> skinImageNames;
   Vector<NewGuiSkinImage>  skinImages;

   U32            generation;   ///< Cascade generation this result was produced at, for cache comparisons.

   NewGuiResolvedStyle();

   /// @return The resolved skin image for partName, or NULL if this style has no entry for it -
   /// the caller should fall back to flat-color painting (backgroundColor/borderColor/etc) in that case.
   const NewGuiSkinImage* findSkinImage(StringTableEntry partName) const;

   /// Sets (adding or overwriting) the skin image entry for partName.
   void setSkinImage(StringTableEntry partName, const NewGuiSkinImage& img);
};

/// One cascadable style rule. Holds authored visual properties, an
/// optional interaction-state scope, and child rules that override it -
/// assign one to NewGuiControl::style; StylePass() cascades it into a
/// NewGuiResolvedStyle. A rule's children are a mix of two kinds, told
/// apart by dynamic_cast in applyTo()/Cascade(): NewGuiSkinImageDef leaves,
/// which are just more of THIS rule's own authored properties (applied
/// unconditionally, same pass as backgroundColor etc), and nested NewGuiStyle
/// rules, which are separate state-scoped overrides (applied only when their
/// own stateMask matches). Skin images are deliberately just another
/// authorable style property, not a parallel system with its own cascade -
/// a skin image for "background" IS this control's background for this
/// state, the same way backgroundColor is when no skin image is set.
///
/// @code
/// new NewGuiStyle( MyButtonStyle )
/// {
///    backgroundColor = "40 40 44 255";
///    new NewGuiSkinImageDef() { partName = "background"; image = ButtonAtlas; frame = "button_normal"; insets = "6 6 6 6"; };
///
///    new NewGuiStyle() { stateMask = "hover";
///       backgroundColor = "60 60 66 255";
///       new NewGuiSkinImageDef() { partName = "background"; image = ButtonAtlas; frame = "button_hover"; insets = "6 6 6 6"; };
///    };
/// };
/// @endcode
class NewGuiStyle : public SimSet
{
public:

   typedef SimSet Parent;

protected:

   ColorI   mTextColor;
   ColorI   mBackgroundColor;
   ColorI   mBorderColor;
   ColorI   mSecondaryColor;

   StringTableEntry mFontFamily;
   F32            mFontSize;

   NewGuiEdgeInsets mPadding;
   NewGuiEdgeInsets mMargin;
   F32            mBorderWidth;

   F32            mOpacity;
   bool           mVisible;

   U32 mSetMask;   ///< Bit per property (see PropertyBit) marking which fields were actually authored.

   /// Bit flags for mSetMask, one per authorable property.
   enum PropertyBit : U32
   {
      Bit_TextColor = BIT(0),
      Bit_BackgroundColor = BIT(1),
      Bit_BorderColor = BIT(2),
      Bit_FontFamily = BIT(3),
      Bit_FontSize = BIT(4),
      Bit_Padding = BIT(5),
      Bit_Margin = BIT(6),
      Bit_BorderWidth = BIT(7),
      Bit_Opacity = BIT(8),
      Bit_Visible = BIT(9),
      Bit_SecondaryColor = BIT(10),
   };

   NewGuiStyleStateMask mStateMask;   ///< Interaction state this rule applies to; 0 always applies.
   S32 mPriority;                     ///< Tiebreaker among same-specificity sibling rules; higher wins.

   static bool _setTextColor(void* obj, const char* index, const char* data);
   static bool _setBackgroundColor(void* obj, const char* index, const char* data);
   static bool _setBorderColor(void* obj, const char* index, const char* data);
   static bool _setSecondaryColor(void* obj, const char* index, const char* data);
   static bool _setFontFamily(void* obj, const char* index, const char* data);
   static bool _setFontSize(void* obj, const char* index, const char* data);
   static bool _setPadding(void* obj, const char* index, const char* data);
   static bool _setMargin(void* obj, const char* index, const char* data);
   static bool _setBorderWidth(void* obj, const char* index, const char* data);
   static bool _setOpacity(void* obj, const char* index, const char* data);
   static bool _setVisible(void* obj, const char* index, const char* data);

   /// Parses a stateMask field, e.g. "hover", "hover|active".
   static bool _setStateMask(void* obj, const char* index, const char* data);
   static const char* _getStateMask(void* obj, const char* data);

public:

   NewGuiStyle();
   virtual ~NewGuiStyle();

   DECLARE_CONOBJECT(NewGuiStyle);

   static void initPersistFields();

   bool onAdd() override;

   /// @return The interaction state mask this rule applies to.
   NewGuiStyleStateMask getStateMask() const { return mStateMask; }

   /// @return This rule's tiebreaker priority.
   S32 getPriority() const { return mPriority; }

   /// @return Bitmask of which properties were actually authored on this rule.
   U32 getSetMask() const { return mSetMask; }

   /// Applies this rule's authored properties onto an existing resolved style, without state or
   /// nested-NewGuiStyle-child checks: overwrites every mSetMask-flagged flat field, and writes
   /// every direct NewGuiSkinImageDef child's part into io by name (a later-applied rule's part
   /// def for the same name simply overwrites the earlier one, same as a flat field).
   /// @param io Resolved style to update in place.
   void applyTo(NewGuiResolvedStyle& io) const;

   /// Resolves a control's final style: starts from the inherited
   /// result, applies ownStyle's base rule, then every child rule
   /// whose state mask matches, in priority/tree order.
   /// @param inherited Parent's already-resolved style.
   /// @param ownStyle The style assigned to the control, or NULL.
   /// @param stateMask The control's current interaction state.
   /// @return The resolved style for this control.
   static NewGuiResolvedStyle Cascade(const NewGuiResolvedStyle& inherited, NewGuiStyle* ownStyle, NewGuiStyleStateMask stateMask);
};

/// One named skin-image entry, authored as a direct child of a NewGuiStyle rule - see that
/// class's own doc comment for why this is just another authored property of its parent rule
/// rather than a separate cascadable object. Not itself a SimSet/cascade participant.
///
/// @code
/// new NewGuiSkinImageDef()
/// {
///    partName = "background";
///    image = ButtonAtlas;
///    frame = "button_normal";
///    insets = "6 6 6 6";
///    centerFill = "stretch";
/// };
/// @endcode
class NewGuiSkinImageDef : public SimObject
{
public:

   typedef SimObject Parent;

protected:

   StringTableEntry mPartName;
   NewGuiSkinImage  mImage;

   static bool _setImage(void* obj, const char* index, const char* data);
   static bool _setFrame(void* obj, const char* index, const char* data);
   static bool _setInsets(void* obj, const char* index, const char* data);
   static bool _setCenterFill(void* obj, const char* index, const char* data);
   static bool _setEdgeFillHorizontal(void* obj, const char* index, const char* data);
   static bool _setEdgeFillVertical(void* obj, const char* index, const char* data);
   static bool _setTint(void* obj, const char* index, const char* data);
   static bool _setAllowHardwareWrap(void* obj, const char* index, const char* data);

public:

   NewGuiSkinImageDef();
   virtual ~NewGuiSkinImageDef();

   DECLARE_CONOBJECT(NewGuiSkinImageDef);

   static void initPersistFields();

   StringTableEntry getPartName() const { return mPartName; }
   const NewGuiSkinImage& getImage() const { return mImage; }
};

class NewGuiRenderBatch;

/// Draws one resolved skin image as a 9-slice: corners and edges are each sized as the same
/// fraction of bounds that their authored inset is of the source frame
/// @param batch Render batch to draw into.
/// @param bounds Destination rect in device pixels.
/// @param img Resolved skin image to draw; caller should have already checked img.hasImage().
/// @param opacity Control's resolved style.opacity, combined with img.tint's own alpha.
/// @param layer Paint-order layer.
void NewGuiStyleDrawSkinImage(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiSkinImage& img, F32 opacity, S32 layer);

#endif // _NEWGUISTYLE_H_
