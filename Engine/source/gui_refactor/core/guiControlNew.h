//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _GUICONTROLNEW_H_
#define _GUICONTROLNEW_H_

#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _SIMBASE_H_
#include "sim/simBase.h"
#endif
#ifndef _GUITYPES_H_ 
#include "gui/core/guiTypes.h"
#endif
#ifndef _GUISTYLE_H_
#include "gui_refactor/core/guiStyle.h"
#endif
#ifndef _GUITEXT_H_
#include "gui_refactor/core/guiText.h"
#endif
#ifndef _GUIDIMENSION_H_
#include "gui_refactor/core/guiDimension.h"
#endif
#ifndef _UTIL_DELEGATE_H_
#include "core/util/delegate.h"
#endif
#ifndef _LANG_H_
#include "i18n/lang.h"
#endif

class GuiCanvasNew;
class GuiEditCtrlNew;
class GuiWindowCtrlNew;


DECLARE_SCOPE(GuiAPINew);


/// Delegate for rendering a tooltip.
/// @param hoverPos position to display the tip near
/// @param cursorPos current cursor position
/// @param tipText optional alternate tip text
/// @return true if the tooltip was rendered
/// @see GuiControlNew::mRenderTooltipDelegate
typedef Delegate<bool(const Point2I& hoverPos, const Point2I& cursorPos, const char* tipText)> RenderTooltipDelegate;

/// @defgroup gui_group Gui System
/// Torque's GUI system: controls, layout, and rendering for
/// WYSIWYG game/application interfaces.

/// @addtogroup gui_core_group Core
/// @section GuiControl_Intro Introduction
/// GuiControlNew is the base class for GUI controls. It provides child
/// containment (via SimGroup), a GuiStyle, mouse/keyboard event hooks,
/// layout/resize, and coordinate conversion (localToGlobalCoord()/
/// globalToLocalCoord()).
///
/// @section GuiControl_Layout Layout model
/// CSS/HTML-box-model-style layout, per axis:
///   - Sizing: #mWidth/#mHeight (each a GuiDimension: auto/pixels/percent),
///     clamped by #mMinWidth/#mMaxWidth/#mMinHeight/#mMaxHeight. auto
///     WIDTH fills the parent's client width (same result as "100%").
///     auto HEIGHT sizes to content via the getPreferredContentExtent()
///     virtual (see its own doc comment) if the control overrides it;
///     controls that don't override it (the base-class default) keep
///     auto height as "leave the current extent alone," same as before
///     content-based sizing existed.
///   - Positioning: #mLeft wins over #mRight if both are set (#mTop/#mBottom
///     likewise); #mCenterHorizontal/#mCenterVertical override their axis
///     entirely. auto LEFT/TOP is auto-FLOW positioning, resolved TOP
///     first: auto top goes directly below the previous sibling's
///     bottom edge (0 if this is the first child, or no previous
///     sibling). Auto left then tries to sit immediately to the right
///     of the previous sibling, on the SAME row -- but ONLY if this
///     control's own (already-resolved) top exactly matches the
///     previous sibling's top, AND the previous sibling's right edge
///     plus this control's own width still fits within the parent's
///     client width; otherwise auto left falls back to flush with the
///     parent's left edge (0), which also covers the plain top-to-
///     bottom stacking case (different tops never share a row) -- see
///     _getPreviousSibling() and resolveLayout(). An axis with nothing
///     set (right/bottom also unset) falls through to this flow
///     behavior, same as an explicit "auto".
///   - Containment: resolved bounds are always clamped inside the parent's
///     client area (see resolveLayout()), unless mAllowOverflow is set.
///
/// All of the above is in LOGICAL units. mBounds is the only stored
/// representation of a control's position/size; getDeviceBounds() projects
/// it into device pixels on demand via GuiCanvasNew::localToGlobal().
///
/// @ref GUI has an overview of the GUI system.
/// @ingroup gui_group Gui System
/// @{
class GuiControlNew : public SimGroup
{
public:

   typedef SimGroup Parent;
   typedef GuiControlNew Children;

   friend class GuiWindowCtrlNew;
   friend class GuiCanvasNew;
   friend class GuiEditCtrlNew;
   friend class GuiDragAndDropControl; // drag callbacks

   /// Additional write flags for GuiControls.
   enum
   {
      NoCheckParentCanSave = BIT(31),   ///< Don't inherit mCanSave=false from parents.
   };

private:

   SimGroup* mAddGroup;   ///< SimGroup child of the global GuiGroup to organize this gui under on creation.
   RectI                   mBounds;     ///< This control's bounds, in logical units. Sole source of truth for position/size.

protected:

   GuiStyle* mStyle;         ///< Style for this control.
   GuiStyle* mTooltipStyle;  ///< Style for this control's tooltip.

   /// @name Interaction state
   /// Drives which GuiStyle state variant resolves (see GuiStyleState).
   /// Hover/active are derived from the canvas's centrally-tracked
   /// mouse state rather than per-control booleans, since subclasses
   /// commonly override the mouse virtuals without calling Parent::.
   /// @{

   /// Whether this control has a "checked" state. Default false;
   /// meaningful only for toggle-style subclasses that override it.
   virtual bool isChecked() const { return false; }

   /// General-purpose validation-failure flag any control can raise.
   bool mHasError;

   /// This control's own sparse style overrides.
   /// @see getInlineStyleOverrides()
   GuiStyleProperties mInlineStyleOverrides;

   /// Computes the current GuiStyleState bitmask for resolving mStyle.
   /// @return combined hover/active/focus/disabled/checked/error bits
   U32 getCurrentStyleStateMask() const;

   /// Interaction-state mask stashed by renderText() for
   /// _resolveFontAtSizeForRenderText()'s delegate binding.
   U32 mRenderTextStateMask;

   /// ShrinkToFit font-at-size delegate bound from renderText().
   /// @return font resolved from mStyle at the given size/state
   Resource<GFont> _resolveFontAtSizeForRenderText(S32 size)
   {
      if (!mStyle)
         return Resource<GFont>();
      return mStyle->getResolvedFontAtSize(mRenderTextStateMask, size);
   }

   /// Persistent GuiText owned by renderJustifiedText(), reused across
   /// calls rather than reconfigured/measured against raw GFont calls
   /// each time -- renderJustifiedText() USED to call
   /// GFont::getStrWidthPrecise() and hand-roll its own alignment math
   /// directly every call, which is the exact bug class already hit
   /// twice elsewhere in this codebase: once in the original
   /// defaultTooltipRender() (see its own cache and doc comment,
   /// further down this file) and once in GuiEditCtrlNew's palette
   /// label rendering (which had called GuiText::renderSimple() --
   /// itself unsafe for a per-frame onRender() caller, since it
   /// constructs a fresh, throwaway GuiText -- and thus mLayoutDirty
   /// starting true -- on every single call, discarding all of
   /// GuiText's own layout caching every time). A member here, reused
   /// call to call the same way mCachedTooltipGuiText is, is what
   /// actually lets GuiText's built-in dirty-tracking do its job.
   GuiText mJustifiedTextCache;

   /// @}

   /// @name Control State
   /// @{   

   static bool setStyleProt(void* object, const char* index, const char* data);
   static bool setTooltipStyleProt(void* object, const char* index, const char* data);

   /// @name Inline style override setters
   /// One protected setter per GuiStyleProperties property, writing
   /// into mInlineStyleOverrides (script/.gui-authored, no per-state
   /// variants -- see getInlineStyleOverrides()).
   /// @{
   static bool setInlineBackgroundColorProt(void* object, const char* index, const char* data);
   static bool setInlineBorderColorProt(void* object, const char* index, const char* data);
   static bool setInlineBorderWidthProt(void* object, const char* index, const char* data);
   static bool setInlineTextColorProt(void* object, const char* index, const char* data);
   static bool setInlineFontFamilyProt(void* object, const char* index, const char* data);
   static bool setInlineFontSizeProt(void* object, const char* index, const char* data);
   static bool setInlineLetterSpacingProt(void* object, const char* index, const char* data);
   static bool setInlineWordSpacingProt(void* object, const char* index, const char* data);
   static bool setInlineTextAlignHProt(void* object, const char* index, const char* data);
   static bool setInlineTextAlignVProt(void* object, const char* index, const char* data);
   /// @}

   S32      mTipHoverTime;

   /// Delegate used to render this control's tooltip. Defaults to defaultTooltipRender.
   RenderTooltipDelegate mRenderTooltipDelegate;

   /// Per-hover cache for defaultTooltipRender() -- see that method's own
   /// doc comment for why this exists. mCachedTooltipStyle/mCachedTooltipFont
   /// are only re-resolved (mTooltipStyle->resolve(0)/getResolvedFont(0))
   /// when mTooltipStyle or the tip text being shown has actually
   /// changed since the last call, rather than on every call -- without
   /// this, a tooltip visible for several seconds was calling
   /// GuiStyle::getResolvedFont() (and therefore GFont::create()) every
   /// single frame for as long as it stayed on screen, which is where
   /// the memory growth traced back to. mCachedTooltipText tracks which
   /// tip string the cache below was built for (a control can render
   /// different tip text across calls via the tipText override
   /// parameter, not just its own fixed mTooltip), and
   /// mCachedTooltipStyleGeneration tracks which GuiStyle instance the
   /// cache was resolved against (compared by pointer -- if mTooltipStyle
   /// is reassigned to a different style object mid-hover, the cache
   /// must be treated as invalid even though the CONTROL didn't change).
   /// @{
   GuiStyle* mCachedTooltipStyleGeneration;
   String mCachedTooltipText;
   GuiStyleProperties mCachedTooltipStyleProps;
   Resource<GFont> mCachedTooltipFont;
   GuiText mCachedTooltipGuiText;
   bool mCachedTooltipValid;
   /// @}

   /// Default tooltip rendering function.
   /// @see RenderTooltipDelegate
   bool defaultTooltipRender(const Point2I& hoverPos, const Point2I& cursorPos, const char* tipText = NULL);

   bool    mVisible;
   bool    mActive;
   bool    mAwake;
   bool    mSetFirstResponder;
   bool    mIsContainer; ///< If true, the GuiEditor can drag other controls into this one.
   bool    mCanResize;
   bool    mCanHit;

   /// If true, resolveLayout()'s containment clamp is skipped.
   bool    mAllowOverflow;

   /// @name Tab navigation / focus behavior
   /// @{

   /// Whether this control participates in tab-key navigation. Default true.
   bool mTabable;

   /// Whether this control can become the keyboard first responder.
   bool mFocusable;

   /// Whether this control, when pushed as a dialog, blocks mouse input to controls behind it.
   bool mCapturesInput;

   /// Explicit tab-order override; -1 means document order.
   S32 mTabIndex;

   /// @}

   S32     mLayer;

   /// @name Render layer
   /// Cross-control draw-order sort key within GuiRenderBatch, distinct
   /// from mLayer (the canvas dialog-stack layer). Lets primitives from
   /// different controls' onRender() calls sort back into correct
   /// paint order once batched together for the frame.
   /// @{

   /// Defaults to parent's mRenderLayer + 1; fixed once mRenderLayerExplicit is set.
   S32 mRenderLayer;

   /// True once mRenderLayer has been set directly rather than left automatic.
   bool mRenderLayerExplicit;

   /// Recomputes mRenderLayer from the current parent and cascades to children if changed.
   void _recomputeRenderLayer();

   /// Recursively finds the highest getRenderLayer() value anywhere in
   /// ctrl's subtree (inclusive of ctrl itself). GuiRenderBatch::flush()
   /// draws strictly in ascending-layer order GLOBALLY across the whole
   /// frame (see guiRenderBatch.cpp) -- layer values are NOT scoped
   /// per-branch of the control tree -- so anything that needs to
   /// guarantee it draws above a given subtree, AT ANY NESTING DEPTH,
   /// needs the true max, not an estimate. mRenderLayer cascades as
   /// parent+1 per level of actual nesting (see _recomputeRenderLayer()),
   /// NOT per immediate-child count, so a proxy like "deepest sibling's
   /// child count" (as bringToFront() used before this helper existed)
   /// systematically undercounts any subtree with real nesting -- a
   /// GuiScrollCtrlNew's children or a GuiWindowCtrlNew's content area,
   /// for instance -- and produces a control that LOOKS like it should
   /// be on top but isn't. See bringToFront()/sendToBack() and
   /// GuiEditCtrlNew's own selection/handle chrome layering for the two
   /// current uses.
   static S32 _findMaxRenderLayerInSubtree(GuiControlNew* ctrl);
   /// @}

   StringTableEntry mLangTableName;
   LangTable* mLangTable;

   bool mNotifyChildrenResized;

   static bool smDesignTime; ///< True if the GUI Editor is active.
   /// @}

   /// @name Design Time Editor Access
   /// @{
   static GuiEditCtrlNew* smEditorHandle; ///< Editor handle controls can access; NULL if editor is closed.
   /// @}

   /// @name Keyboard Input
   /// @{
   GuiControlNew* mFirstResponder;
   static GuiControlNew* smPrevResponder;
   static GuiControlNew* smCurResponder;
   /// @}

   /// @name Layout (CSS box-model style)
   /// Logical units, parsed once into GuiDimension at field-set time.
   /// @{

   /// Authored layout fields. Each resolves into mBounds immediately
   /// when set (see _resolveAndApplyDimension()); kept as members only
   /// so script/.gui serialization has a last-authored value to read.
   /// A percent value set before a live parent exists is deferred --
   /// see mPendingPercentAxes.
   GuiDimension mWidth;
   GuiDimension mHeight;
   GuiDimension mMinWidth;
   GuiDimension mMaxWidth;
   GuiDimension mMinHeight;
   GuiDimension mMaxHeight;

   GuiDimension mLeft;
   GuiDimension mTop;
   GuiDimension mRight;
   GuiDimension mBottom;

   bool mCenterHorizontal;
   bool mCenterVertical;

   /// Bitmask of axes with a pending percent value waiting on a live parent.
   enum PendingDimensionAxis
   {
      PendingWidth = BIT(0),
      PendingHeight = BIT(1),
      PendingMinWidth = BIT(2),
      PendingMaxWidth = BIT(3),
      PendingMinHeight = BIT(4),
      PendingMaxHeight = BIT(5),
      PendingLeft = BIT(6),
      PendingTop = BIT(7),
      PendingRight = BIT(8),
      PendingBottom = BIT(9),

      /// Separate from PendingLeft/PendingTop so a pending center and a
      /// pending percent offset on the same axis can both be tracked.
      PendingCenterHorizontal = BIT(10),
      PendingCenterVertical = BIT(11),
   };
   U32 mPendingPercentAxes;

   /// Looks up the live parent's current extent on one axis, for resolving a percent value against.
   /// @param horizontalAxis true = width, false = height
   /// @param outLength filled with the parent's extent on that axis
   /// @return false if there's no live parent yet
   bool _getParentReferenceLength(bool horizontalAxis, F32& outLength) const;

   /// Resolves one GuiDimension field against the current parent and applies it to mBounds via resize().
   /// auto is a no-op; pixels always resolve; percent with no live
   /// parent yet is recorded in mPendingPercentAxes instead.
   /// @param dim the field's parsed value
   /// @param axis which PendingDimensionAxis bit this field maps to
   /// @param isWidthAxis true = horizontal, false = vertical
   /// @param isPositionAxis true = writes mBounds.point, false = mBounds.extent
   void _resolveAndApplyDimension(const GuiDimension& dim, PendingDimensionAxis axis, bool isWidthAxis, bool isPositionAxis);

   /// Applies auto's field-specific meaning for one axis -- called by
   /// _resolveAndApplyDimension() when the field's parsed value is
   /// Auto, instead of that function's normal pixel/percent path, AND by
   /// resolveLayout() on every dirty-layout pass (not just once) -- see
   /// resolveLayout()'s own doc comment for why continuous re-resolution
   /// is what makes fill-width/content-height/flow-position actually
   /// track a resizing parent, and for how GuiScrollCtrlNew stays
   /// correct despite that (it re-syncs its own child anchors after
   /// calling the base resolveLayout(), rather than this method special-
   /// casing scroll children). A no-op for any axis besides width/
   /// height/left/top (see this method's own .cpp doc comment) and for
   /// any axis without a live parent yet.
   /// @param axis which PendingDimensionAxis bit this field maps to
   /// @param isWidthAxis true = horizontal, false = vertical
   /// @param isPositionAxis true = writes mBounds.point, false = mBounds.extent
   void _resolveAutoDimension(PendingDimensionAxis axis, bool isWidthAxis, bool isPositionAxis);

   /// Re-resolves every axis still marked in mPendingPercentAxes, called once this control gets a live parent.
   void _resolvePendingPercentAxes();

   /// Opt-in per-control content measurement, used when mHeight is
   /// "auto" -- see _resolveAutoDimension()'s doc comment on what auto
   /// now means for width/height. Base implementation returns false ("no
   /// opinion"), which is what makes auto height on a control with no
   /// override fall back to the original "leave mBounds.extent alone"
   /// behavior rather than collapsing to zero -- e.g. a plain
   /// GuiControlNew container with auto height and no override still
   /// keeps whatever height it was constructed/resized to. Override in a
   /// leaf control (GuiLabelCtrlNew measuring its own wrapped text,
   /// GuiButtonCtrlNew measuring its label + padding, etc.) to report a
   /// real preferred size: return true and fill outExtent.y (only the
   /// axis actually being resolved is read by the caller -- see
   /// _resolveAutoDimension()'s PendingHeight branch). Measurement
   /// should be done at the control's CURRENT width (already resolved by
   /// the time height's auto pass runs -- see addObject()'s width-then-
   /// height ordering), since wrap-dependent content (e.g. label text)
   /// needs a width to measure against.
   virtual bool getPreferredContentExtent(Point2I& outExtent) const { return false; }

   /// Opt-in: after layout resolves, shrink (never grow) this control's
   /// extent to fit inside its parent while preserving its authored
   /// width:height ratio -- CSS object-fit: contain, applied to the
   /// control's own bounds. Position is left alone (same containment
   /// clamp as any other control), not re-centered.
   ///
   /// The ratio is derived fresh every resolveLayout() pass from
   /// mWidth/mHeight directly (see _getAuthoredExtent()) rather than
   /// from mBounds.extent -- mWidth/mHeight are the permanent authored
   /// fields, untouched by resolveLayout()'s own containment/aspect
   /// clamping, so re-resolving them each pass naturally reflects both
   /// a fixed pixel size and a percent-of-parent size that changes as
   /// the parent does, without needing a separately cached value that
   /// could drift or get stuck at a previously shrunk size.
   bool mPreserveAspectRatio;

   /// Resolves mWidth/mHeight directly against the given parent extent
   /// (independent of mBounds, and without applying via resize()) to
   /// get this control's authored extent for the current pass -- see
   /// mPreserveAspectRatio's doc comment for why resolveLayout() needs
   /// this rather than reading mBounds.extent. auto on either axis
   /// falls back to that axis's current mBounds.extent (the "leave
   /// this axis alone" contract auto already has everywhere else).
   Point2I _getAuthoredExtent(F32 parentW, F32 parentH) const
   {
      Point2I result = mBounds.extent;
      if (!mWidth.isAuto())
         result.x = (S32)mWidth.resolve(parentW);
      if (!mHeight.isAuto())
         result.y = (S32)mHeight.resolve(parentH);
      return result;
   }

   /// True whenever this control's bounds need resolveLayout() to run again.
   bool mLayoutDirty;

   /// Finds this control's immediate previous sibling in the parent's
   /// child list (the child added just before this one), or NULL if this
   /// is the first child or there's no live parent. Used by
   /// resolveLayout()'s auto-flow positioning (see mLeft/mTop's updated
   /// semantics) -- walks the parent's iterator once per call rather
   /// than maintaining a cached prev/next pointer pair, since layout
   /// resolution is not a per-frame hot path (only re-runs when
   /// markLayoutDirty() actually fires) and a SimGroup child list is
   /// typically small.
   GuiControlNew* _getPreviousSibling() const;

   /// Recomputes mBounds from the layout fields and the parent's current resolved size.
   /// Applies sizing, then position, then the containment clamp.
   /// Called automatically when needed (see markLayoutDirty()).
   /// Virtual so composite controls (e.g. a vertical stack) can resolve
   /// children relative to each other after the base resolve.
   virtual void resolveLayout();

   static bool setWidthProt(void* object, const char* index, const char* data);
   static bool setHeightProt(void* object, const char* index, const char* data);
   static bool setMinWidthProt(void* object, const char* index, const char* data);
   static bool setMaxWidthProt(void* object, const char* index, const char* data);
   static bool setMinHeightProt(void* object, const char* index, const char* data);
   static bool setMaxHeightProt(void* object, const char* index, const char* data);
   static bool setLeftProt(void* object, const char* index, const char* data);
   static bool setTopProt(void* object, const char* index, const char* data);
   static bool setRightProt(void* object, const char* index, const char* data);
   static bool setBottomProt(void* object, const char* index, const char* data);

   /// Applies centering immediately against mBounds.extent via resize(), same as any positioning field.
   static bool setCenterHorizontalProt(void* object, const char* index, const char* data);
   static bool setCenterVerticalProt(void* object, const char* index, const char* data);

   /// Routes script/.gui-file "renderLayer" assignment through setRenderLayer().
   static bool setRenderLayerProt(void* object, const char* index, const char* data);

   /// Routes script/.gui-file "preserveAspectRatio" assignment through setPreserveAspectRatio(),
   /// so enabling it correctly captures the current extent as authored -- see that method's doc comment.
   static bool setPreserveAspectRatioProt(void* object, const char* index, const char* data);

   /// @}

   StringTableEntry mAcceleratorKey;
   StringTableEntry mConsoleVariable;

   String mConsoleCommand;
   String mAltConsoleCommand;

   String mTooltip;

   StringTableEntry mCategory;

   /// @}

   /// @name Console
   /// Binds a console variable to this control (e.g. an edit field to $foo).
   /// @{

   /// $ThisControl variable for callback execution.
   static GuiControlNew* smThisControl;

   /// Sets $ThisControl and evaluates the given script code.
   const char* evaluate(const char* str);

   /// Sets the bound console variable to a string value.
   void setVariable(const char* value);

   /// Sets the bound console variable to an integer value.
   void setIntVariable(S32 value);

   /// Sets the bound console variable to a float value.
   void setFloatVariable(F32 value);

   const char* getVariable() const; ///< @return bound console variable's value as a string
   S32 getIntVariable() const;      ///< @return bound console variable's value as an integer
   F32 getFloatVariable() const;    ///< @return bound console variable's value as a float

   GFXStateBlockRef mDefaultGuiSB;

   /// @name Callbacks
   /// @{

   DECLARE_CALLBACK(void, onAdd, ());
   DECLARE_CALLBACK(void, onRemove, ());

   DECLARE_CALLBACK(void, onWake, ());
   DECLARE_CALLBACK(void, onSleep, ());

   DECLARE_CALLBACK(void, onLoseFirstResponder, ());
   DECLARE_CALLBACK(void, onGainFirstResponder, ());

   DECLARE_CALLBACK(void, onAction, ());
   DECLARE_CALLBACK(void, onVisible, (bool state));
   DECLARE_CALLBACK(void, onActive, (bool state));

   DECLARE_CALLBACK(void, onDialogPush, ());
   DECLARE_CALLBACK(void, onDialogPop, ());

   DECLARE_CALLBACK(void, onControlDragEnter, (GuiControlNew* control, const Point2I& dropPoint));
   DECLARE_CALLBACK(void, onControlDragExit, (GuiControlNew* control, const Point2I& dropPoint));
   DECLARE_CALLBACK(void, onControlDragged, (GuiControlNew* control, const Point2I& dropPoint));
   DECLARE_CALLBACK(void, onControlDropped, (GuiControlNew* control, const Point2I& dropPoint));

   /// @}

public:

   /// Sets the name of the console variable this control is bound to.
   void setConsoleVariable(const char* variable);

   /// Sets the console function bound to this control (e.g. a button's onClick).
   void setConsoleCommand(const String& newCmd);
   const char* getConsoleCommand() const; ///< @return the function name bound to this control
   LangTable* getGUILangTable(void);
   const UTF8* getGUIString(S32 id);

   inline String& getTooltip() { return mTooltip; } ///< @return this control's tooltip text

   /// @}

   /// @name Callbacks
   /// Sets $ThisControl to this control's id before executing (not thread-safe).
   /// @{

   /// Executes mConsoleCommand.
   /// @return the command's result
   const char* execConsoleCallback();
   /// Executes mAltConsoleCommand.
   /// @return the command's result
   const char* execAltConsoleCallback();
   /// @}

   static bool _setVisible(void* object, const char* index, const char* data) { static_cast<GuiControlNew*>(object)->setVisible(dAtob(data)); return false; };
   static bool _setActive(void* object, const char* index, const char* data) { static_cast<GuiControlNew*>(object)->setActive(dAtob(data)); return false; };

   /// @name Editor
   /// Used by the GUI Editor.
   /// @{

   /// Overrides Parent serialization to allow specific controls to not be saved.
   void write(Stream& stream, U32 tabStop, U32 flags) override;

   /// @return true if no parent (including self) has the 'no serialization' flag set
   bool getCanSaveParent() const;

   /// @}

   /// @name Initialization
   /// @{

   DECLARE_CONOBJECT(GuiControlNew);
   DECLARE_CATEGORY("Gui Core");
   DECLARE_DESCRIPTION("Base class for GUI controls. Can also be used as a generic container.");

   GuiControlNew();
   virtual ~GuiControlNew();
   bool processArguments(S32 argc, ConsoleValue* argv) override;

   static void initPersistFields();
   static void consoleInit();

   /// @}

   /// @name Accessors
   /// @{

   /// @return this control's position, in logical units (resolves a dirty layout first)
   inline const Point2I& getPosition() const
   {
      if (mLayoutDirty)
         const_cast<GuiControlNew*>(this)->resolveLayout();
      return mBounds.point;
   }

   /// @return this control's extent, in logical units (resolves a dirty layout first)
   inline const Point2I& getExtent() const
   {
      if (mLayoutDirty)
         const_cast<GuiControlNew*>(this)->resolveLayout();
      return mBounds.extent;
   }

   /// @return this control's bounds, in logical units (resolves a dirty layout first)
   inline const RectI     getBounds() const
   {
      if (mLayoutDirty)
         const_cast<GuiControlNew*>(this)->resolveLayout();
      return mBounds;
   }

   /// @return this control's bounds without resolving a pending dirty layout first
   inline const RectI     getRawBounds() const { return mBounds; }

   /// @return this control's bounds projected into device (physical
   /// screen) pixels, computed fresh each call. If mPreserveAspectRatio
   /// is set, uses a single uniform scale for this control's extent
   /// (never the canvas's independent per-axis scale) so its ratio
   /// stays correct even when the canvas itself is stretching non-
   /// uniformly (GuiCanvasNew::mLockAspectRatio == false).
   RectI getDeviceBounds() const;

   /// The extent CHILDREN should resolve their own auto/percent width or
   /// height against -- i.e. this control's CLIENT area, as opposed to
   /// getExtent()'s full outer size. Defaults to getExtent() itself (no
   /// difference for an ordinary container, where the client area IS the
   /// full extent). Exists specifically for a container that reserves
   /// part of its own extent for its own chrome -- e.g. GuiScrollCtrlNew
   /// reserving a scrollbar gutter -- so a child left at the ordinary
   /// "auto width fills the parent" or "50%" default is actually
   /// confined to the region a user could see/reach, not laid out
   /// underneath chrome the parent draws on top of it. Deliberately NOT
   /// used by getExtent() itself, hit-testing, rendering, or scroll-range
   /// math elsewhere in this class -- those all still need this
   /// control's OWN true full size; only _getParentReferenceLength() and
   /// _resolveAutoDimension() (both of which read a CHILD's parent's
   /// extent specifically to size that child) call this instead of
   /// getExtent() directly.
   virtual Point2I getClientExtent() const { return getExtent(); }

   /// @return this canvas's current logical->device scale factor for this axis (1.0 if not attached to a canvas)
   F32 getEffectiveScaleX() const;
   F32 getEffectiveScaleY() const;

   /// @return minimum size this control can be, resolved from
   /// #mMinWidth/#mMinHeight against the current parent's client extent
   /// (0 on an axis with no live parent yet). Superseded the old plain
   /// mMinExtent member/settable "minExtent" script field -- minWidth/
   /// minHeight are now the single authored source for a structural
   /// floor, same as every other sizing axis (see mMinWidth's own doc
   /// comment).
   virtual Point2I getMinExtent() const;
   /// @return this control's left/top/width/height, in logical units
   inline const S32        getLeft() const { return getPosition().x; }
   inline const S32        getTop() const { return getPosition().y; }
   inline const S32        getWidth() const { return getExtent().x; }
   inline const S32        getHeight() const { return getExtent().y; }

   /// @}

   /// @name Flags
   /// @{

   /// Sets the visibility of the control.
   virtual void setVisible(bool value);
   inline bool isVisible() const { return mVisible; } ///< @return true if this control is visible
   bool isHidden() const override { return !isVisible(); }
   void setHidden(bool state) override { setVisible(!state); }

   void setCanHit(bool value) { mCanHit = value; }

   /// Sets whether this control is active and responding to input.
   virtual void setActive(bool value);
   bool isActive() const { return mActive; } ///< @return true if this control is active

   bool isAwake() const { return mAwake; } ///< @return true if this control is awake

   /// @name Tab navigation / focus behavior
   /// @{
   bool isTabable() const { return mTabable; }
   void setTabable(bool value) { mTabable = value; }

   bool isFocusable() const { return mFocusable; }
   void setFocusable(bool value) { mFocusable = value; }

   bool getCapturesInput() const { return mCapturesInput; }
   void setCapturesInput(bool value) { mCapturesInput = value; }

   S32 getTabIndex() const { return mTabIndex; }
   void setTabIndex(S32 value) { mTabIndex = value; }
   /// @}

   /// @name Render layer / draw ordering
   /// Cross-control paint order within GuiRenderBatch, not the canvas dialog-stack mLayer.
   /// @{

   S32 getRenderLayer() const { return mRenderLayer; }

   /// Directly overrides this control's render layer and cascades to children immediately.
   void setRenderLayer(S32 value)
   {
      mRenderLayer = value;
      mRenderLayerExplicit = true;
      _recomputeRenderLayer();
   }

   /// Clears any explicit override and reverts to the automatic parent-plus-one default.
   void clearRenderLayerOverride()
   {
      mRenderLayerExplicit = false;
      _recomputeRenderLayer();
   }

   bool isRenderLayerExplicit() const { return mRenderLayerExplicit; }

   /// Moves this control to paint above everything else at its current depth.
   void bringToFront();

   /// Visual opposite of bringToFront(): paints behind everything else at this depth.
   void sendToBack();
   /// @}

   /// @}

   /// Gets the size of a scroll line.
   /// @param rowHeight filled with row height in pixels
   /// @param columnWidth filled with column width in pixels
   virtual void getScrollLineSizes(U32* rowHeight, U32* columnWidth);

   /// Gets cursor info for this control.
   /// @param cursor filled with cursor info
   /// @param showCursor filled with whether the cursor is visible
   /// @param lastGuiEvent cursor position and modifier keys
   virtual void getCursor(GuiCursor*& cursor, bool& showCursor, const GuiEvent& lastGuiEvent);

   /// @name Children
   /// @{

   /// Adds an object as a child of this control.
   void addObject(SimObject* obj) override;

   /// Removes a child object from this control.
   void removeObject(SimObject* obj) override;

   GuiControlNew* getParent()const;  ///< @return the control which owns this one
   GuiCanvasNew* getRoot() const;     ///< @return the root canvas of this control

   bool acceptsAsChild(SimObject* object) const override;

   void onGroupRemove() override;

   /// @}

   /// @name Coordinates
   /// @{

   /// Translates local coordinates (relative to this control) into global coordinates.
   Point2I localToGlobalCoord(const Point2I& src) const;

   /// Translates global coordinates into local coordinates (relative to this control).
   Point2I globalToLocalCoord(const Point2I& src) const;
   /// @}

   /// @name Resizing
   /// @{

   /// Directly changes size/position in logical units, bypassing the layout fields.
   /// Used for interactive moves (e.g. the GUI editor dragging a control).
   virtual bool resize(const Point2I& newPosition, const Point2I& newExtent);

   /// Changes the position of this control.
   virtual bool setPosition(const Point2I& newPosition);
   inline  void setPosition(const S32 x, const S32 y) { setPosition(Point2I(x, y)); }

   /// Changes the size of this control.
   virtual bool setExtent(const Point2I& newExtent);
   inline  void setExtent(const S32 width, const S32 height) { setExtent(Point2I(width, height)); }

   /// Changes the bounds of this control.
   virtual bool setBounds(const RectI& newBounds);
   inline  void setBounds(const S32 left, const S32 top,
      const S32 width, const S32 height) {
      setBounds(RectI(left, top, width, height));
   }

   /// Changes the X position of this control.
   virtual void setLeft(S32 newLeft);

   /// Changes the Y position of this control.
   virtual void setTop(S32 newTop);

   /// Changes the width of this control.
   virtual void setWidth(S32 newWidth);

   /// Changes the height of this control.
   virtual void setHeight(S32 newHeight);

   /// Called when a child control is resized.
   virtual void childResized(GuiControlNew* child);

   /// Marks this control's layout as needing to be resolved again.
   void markLayoutDirty() { mLayoutDirty = true; }
   bool isLayoutDirty() const { return mLayoutDirty; }

   /// @see mAllowOverflow
   void setAllowOverflow(bool value) { mAllowOverflow = value; markLayoutDirty(); }
   bool getAllowOverflow() const { return mAllowOverflow; }

   /// @see mPreserveAspectRatio
   bool getPreserveAspectRatio() const { return mPreserveAspectRatio; }
   void setPreserveAspectRatio(bool value) { mPreserveAspectRatio = value; markLayoutDirty(); }
   /// @}

   /// @name Rendering
   /// @{

   /// Called when this control is to render itself.
   /// @param offset where this control should begin rendering
   /// @param updateRect the screen area this control has drawing access to
   virtual void onRender(Point2I offset, const RectI& updateRect);

   /// Called when this control should render its children.
   /// @param offset where rendering should begin
   /// @param updateRect the screen area available for drawing
   virtual void renderChildControls(Point2I offset, const RectI& updateRect);

   /// Sets the area (local coordinates) this control wants refreshed each frame.
   void setUpdateRegion(Point2I pos, Point2I ext);

   /// Sets the update area to encompass the whole control.
   virtual void setUpdate();
   /// @}

   //child hierarchy calls
   void awaken();          ///< Called when this control and its children have been wired up.
   void sleep();           ///< Called when this control is no longer active.
   void preRender();       ///< Pre-renders this control and all its children.

   /// @name Events
   /// Subclasses overriding these should call the Parent:: version.
   /// @{

   /// Called when this control wakes. @return true if actually awake by the end
   virtual bool onWake();

   /// Called when this control is asked to sleep.
   virtual void onSleep();

   /// Special pre-render processing.
   virtual void onPreRender();

   /// Called when this object is removed.
   void onRemove() override;

   /// Called when a child of this object is removed.
   virtual void onChildRemoved(GuiControlNew* child);

   /// Called when this object is added to the scene.
   bool onAdd() override;

   /// Called when mStyle or mTooltipStyle is deleted.
   void onDeleteNotify(SimObject* object) override;

   /// Called when this object gains a new child.
   virtual void onChildAdded(GuiControlNew* child);

   /// @}

   /// @name Console
   /// @{

   /// @return the value of the variable bound to this object
   virtual const char* getScriptValue();

   /// Sets the value of the variable bound to this object.
   virtual void setScriptValue(const char* value);
   /// @}

   /// @name Input (Keyboard/Mouse)
   /// @{

   /// @param parentCoordPoint coordinates to test, relative to the parent
   /// @return true if the point is within this control's bounds
   virtual bool pointInControl(const Point2I& parentCoordPoint) const;

   /// @return true if the global cursor is inside this control
   bool cursorInControl() const;

   /// @param pt point to test
   /// @param initialLayer layer of gui objects to begin the search
   /// @return the control under the given point, layering-aware
   /// @note not const: recurses via begin()/end(), which SimSet doesn't expose a const overload for
   virtual GuiControlNew* findHitControl(const Point2I& pt, S32 initialLayer = -1);

   enum EHitTestFlags
   {
      HIT_FullBoxOnly = BIT(0),    ///< Hit only counts if all of a control's bounds are within the hit rectangle.
      HIT_ParentPreventsChildHit = BIT(1),    ///< A positive hit test on a parent control will prevent hit tests on children.
      HIT_AddParentHits = BIT(2),    ///< Parent's that get hit should be added regardless of whether any of their children get hit, too.
      HIT_NoCanHitNoRecurse = BIT(3),    ///< A hit-disabled control will not recurse into children.
   };

   /// @note same const constraint as findHitControl()
   virtual bool findHitControls(const RectI& rect, Vector< GuiControlNew* >& outResult, U32 flags = 0, S32 initialLayer = -1, U32 depth = 0);

   /// Locks the mouse within the provided control.
   void mouseLock(GuiControlNew* lockingControl);

   /// Turns on mouse locking with the last used lock control.
   void mouseLock();

   /// Unlocks the mouse.
   void mouseUnlock();

   /// @return true if the mouse is locked
   bool isMouseLocked() const;
   /// @}


   /// General input handler.
   virtual bool onInputEvent(const InputEventInfo& event);

   /// @name Mouse Events
   /// @{
   virtual void onMouseUp(const GuiEvent& event);
   virtual void onMouseDown(const GuiEvent& event);
   virtual void onMouseMove(const GuiEvent& event);
   virtual void onMouseDragged(const GuiEvent& event);
   virtual void onMouseEnter(const GuiEvent& event);
   virtual void onMouseLeave(const GuiEvent& event);

   virtual bool onMouseWheelUp(const GuiEvent& event);
   virtual bool onMouseWheelDown(const GuiEvent& event);

   virtual void onRightMouseDown(const GuiEvent& event);
   virtual void onRightMouseUp(const GuiEvent& event);
   virtual void onRightMouseDragged(const GuiEvent& event);

   virtual void onMiddleMouseDown(const GuiEvent& event);
   virtual void onMiddleMouseUp(const GuiEvent& event);
   virtual void onMiddleMouseDragged(const GuiEvent& event);
   /// @}

   /// @name Gamepad Events
   /// @{
   virtual bool onGamepadButtonDown(const GuiEvent& event);  ///< Default behavior is call-through to onKeyDown
   virtual bool onGamepadButtonUp(const GuiEvent& event);    ///< Default behavior is call-through to onKeyUp
   virtual bool onGamepadAxisUp(const GuiEvent& event);
   virtual bool onGamepadAxisDown(const GuiEvent& event);
   virtual bool onGamepadAxisLeft(const GuiEvent& event);
   virtual bool onGamepadAxisRight(const GuiEvent& event);
   virtual bool onGamepadTrigger(const GuiEvent& event);
   /// @}

   /// @name Editor Mouse Events
   /// Called when the GUI editor is active. Returning true suppresses
   /// the editor's own handling of the event.
   /// @{

   /// @param event the event that triggered this call
   /// @param offset the editor's on-screen unit offset
   virtual bool onMouseDownEditor(const GuiEvent& event, Point2I offset) { return false; };

   /// @param event the event that triggered this call
   /// @param offset the editor's on-screen unit offset
   virtual bool onMouseUpEditor(const GuiEvent& event, Point2I offset) { return false; };

   /// @param event the event that triggered this call
   /// @param offset the editor's on-screen unit offset
   virtual bool onRightMouseDownEditor(const GuiEvent& event, Point2I offset) { return false; };

   /// @param event the event that triggered this call
   /// @param offset the editor's on-screen unit offset
   virtual bool onMouseDraggedEditor(const GuiEvent& event, Point2I offset) { return false; };

   /// @}

   /// @name Tabs
   /// @{

   /// @return this control's immediate children in tab order (explicit tabIndex first, then document order)
   /// @note not const: uses SimSet::iterator, which has no const overload
   void _getChildrenInTabOrder(Vector<GuiControlNew*>& outChildren);

   /// @return the first tab-accessible child of this control
   virtual GuiControlNew* findFirstTabable();

   /// @param firstCall set true to clear the global previous responder
   /// @return the last tab-accessible child of this control
   virtual GuiControlNew* findLastTabable(bool firstCall = true);

   /// @param curResponder current control
   /// @param firstCall set true to clear the global previous responder
   /// @return the previous tab-accessible control relative to curResponder
   virtual GuiControlNew* findPrevTabable(GuiControlNew* curResponder, bool firstCall = true);

   /// @param curResponder current control
   /// @param firstCall set true to clear the global current responder
   /// @return the next tab-accessible control relative to curResponder
   virtual GuiControlNew* findNextTabable(GuiControlNew* curResponder, bool firstCall = true);
   /// @}

   /// @return true if the given control is a descendant of this one
   virtual bool controlIsChild(GuiControlNew* child);

   /// @name First Responder
   /// The control which reacts first, in its chain, to keyboard events.
   /// @{

   /// Sets the first responder for child controls.
   virtual void setFirstResponder(GuiControlNew* firstResponder);

   /// Makes this control the first in its group to respond to input.
   virtual void makeFirstResponder(bool value);

   /// @return true if this control is the first responder
   bool isFirstResponder() const;

   /// Sets this object as the first responder.
   virtual void setFirstResponder();

   /// Clears the first responder for this chain.
   void clearFirstResponder();

   /// @return the first responder for this chain
   GuiControlNew* getFirstResponder() { return mFirstResponder; }

   /// Called when this control gains first-responder status.
   virtual void onGainFirstResponder();

   /// Called when this control loses first-responder status.
   virtual void onLoseFirstResponder();
   /// @}

   /// @name Keyboard Events
   /// @{

   /// Adds this control's accelerator key to the canvas.
   void addAcceleratorKey();

   /// Adds this control's accelerator key to the accelerator map, recursing into children.
   virtual void buildAcceleratorMap();

   /// Called when this control's accelerator key is pressed.
   virtual void acceleratorKeyPress(U32 index);

   /// Called when this control's accelerator key is released.
   virtual void acceleratorKeyRelease(U32 index);

   /// Called when a key is pressed.
   virtual bool onKeyDown(const GuiEvent& event);

   /// Called when a key is released.
   virtual bool onKeyUp(const GuiEvent& event);

   /// Called on repeated keystrokes from a held key.
   virtual bool onKeyRepeat(const GuiEvent& event);
   /// @}

   /// @return the delegate used to render tooltips on this control
   RenderTooltipDelegate& getRenderTooltipDelegate() { return mRenderTooltipDelegate; }
   const RenderTooltipDelegate& getRenderTooltipDelegate() const { return mRenderTooltipDelegate; }

   /// @return this control's tooltip style, finding it if not yet set
   GuiStyle* getTooltipStyle() { return mTooltipStyle; }
   const GuiStyle* getTooltipStyle() const { return mTooltipStyle; }

   /// Sets the tooltip style for this control.
   /// @see GuiStyle
   void setTooltipStyle(GuiStyle* style);

   /// @return this control's style, finding it if not yet set
   GuiStyle* getStyle() { return mStyle; }
   const GuiStyle* getStyle() const { return mStyle; }

   /// Sets the style for this control.
   /// @see GuiStyle
   void setStyle(GuiStyle* style);

   /// @name Style resolution
   /// @{

   /// @return true if the canvas's mouse tracking has this control under the cursor
   bool isHovered() const;

   /// @return true if this control is hovered/mouse-locked and the mouse button is held down
   bool isPressed() const;

   /// @see mHasError
   bool hasError() const { return mHasError; }
   void setHasError(bool value) { mHasError = value; }

   /// @return this control's own inline style overrides (mutable)
   GuiStyleProperties& getInlineStyleOverrides() { return mInlineStyleOverrides; }
   const GuiStyleProperties& getInlineStyleOverrides() const { return mInlineStyleOverrides; }

   /// @return this control's fully cascaded style for the current frame (state + inline overrides)
   GuiStyleProperties resolveStyle() const;

   /// @}

   /// Called when this control performs its "action".
   virtual void onAction();

   /// @name Peer Messaging
   /// Sends a message to sibling controls (mostly used by radio controls).
   /// @{
   void messageSiblings(S32 message);                      ///< Sends a message to all siblings
   virtual void onMessage(GuiControlNew* sender, S32 msg);    ///< Receives a message from another control
   /// @}

   /// @name Canvas Events
   /// @{

   /// Called if this object is a dialog, when added to the visible layers.
   virtual void onDialogPush();

   /// Called if this object is a dialog, when removed from the visible layers.
   virtual void onDialogPop();
   /// @}

   /// Renders justified text using this control's style. Thin wrapper
   /// over renderText()/mJustifiedTextCache -- see that member's doc
   /// comment for why this does NOT measure/align against GFont
   /// directly the way it used to.
   void renderJustifiedText(Point2I offset, Point2I extent, const char* text);

   /// Renders a caller-configured GuiText using this control's resolved style/font/interaction-state.
   void renderText(GuiText& text, Point2I offset, Point2I extent);

   /// Clips text to fit within a pixel width, appending "..." if truncated.
   /// @return the final clipped text's width in pixels
   U32 clipText(String& inOutText, U32 width) const;

   void inspectPostApply() override;
   void inspectPreApply() override;
protected:
   F32 fade_amt;
public:
   void setFadeAmount(F32 amt) { fade_amt = amt; }
};

/// @}

#endif
