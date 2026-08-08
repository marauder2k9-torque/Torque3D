//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiControl.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUICONTROL_H_
#define _NEWGUICONTROL_H_

#ifndef _SIMSET_H_
#include "sim/simSet.h"
#endif
#ifndef _NEWGUITYPES_H_
#include "gui_rev2/core/newGuiTypes.h"
#endif
#ifndef _NEWGUISTYLE_H_
#include "gui_rev2/core/newGuiStyle.h" 
#endif
#ifndef _NEWGUIINPUTEVENT_H_
#include "gui_rev2/core/newGuiInputEvent.h"
#endif
#ifndef _NEWGUICURSOR_H_
#include "gui_rev2/core/newGuiCursor.h"
#endif
#ifndef _EVENT_H_
#include "platform/input/event.h"
#endif
#include <functional>

class NewGuiRenderBatch;

/// Base class for every control in the GUI system. Every control (leaf
/// or container) is a SimGroup, so any control can hold children -
/// "container behavior" is just an override of GetChildSlot(). Derive
/// from this to build a new control type.
///
/// @code
/// new NewGuiControl( MyPanel )
/// {
///    width = "100%"; height = "auto";
///    style = MyPanelStyle;
/// };
/// @endcode
class NewGuiControl : public SimGroup
{
public:
   friend class NewGuiCanvas;
   friend class NewGuiPopup;
   typedef SimGroup Parent;

protected:

   // Dirty flags - one per pass that can independently go stale.
   bool mStyleDirty;         ///< Propagates down to children.
   bool mContentDirty;       ///< Propagates up to ancestors.
   bool mArrangementDirty;   ///< Propagates down to children.

   U32 mCachedInheritedGeneration;   ///< Style cascade generation this control's mResolvedStyle was last computed against.

   // Authored layout state, per axis.
   NewGuiDimension mWidth;
   NewGuiDimension mHeight;
   NewGuiDimension mLeft;
   NewGuiDimension mTop;
   NewGuiDimension mRight;
   NewGuiDimension mBottom;

   /// When true, this control's own Pixels-mode width/height resolve
   /// using a single uniform scale (min of the two axes) instead of
   /// each axis independently, preserving aspect ratio. Positioning
   /// (left/top/right/bottom) always uses each axis's own scale
   /// regardless. Has no effect on Percent or Auto sizing.
   bool mPreserveAspect;

   // Resolved state - written only by the driver's passes.
   Point2I  mPreferredSize;     ///< Written by MeasurePass.
   RectI    mBounds;            ///< Written by ArrangePass.
   S32      mRenderLayer;       ///< Written by ArrangePass.

   F32      mResolvedUIScaleX;  ///< Scale ArrangePass() was last called with, cached for use outside that call chain.
   F32      mResolvedUIScaleY;

   S32 mRenderLayerOverride;    ///< Authored paint-order override; -1 = use parent.mRenderLayer + 1.

   /// Shared across every NewGuiControl instance - the next value elevateToFront() will hand
   /// out. Ever-incrementing, never reused, so the most recently elevated control always has
   /// the strictly highest override in play, the same "last one wins" guarantee tree-order
   /// reordering already gives findHitControl()/paint order, but for controls (like a
   /// NewGuiWindow) that need to jump an entire subtree above OTHER subtrees with their own
   /// fixed/overridden layers, not just above ordinary parent+1 siblings.
   static S32 smNextElevationLayer;

   /// kElevationBase..40000 is reserved for elevateToFront() - comfortably above ordinary
   /// content (which stays in the low hundreds even in a deeply nested tree) but below
   /// NewGuiPopup::kPopupLayer (40000) and NewGuiCanvas's tooltip layer (50000), so a popup or
   /// tooltip opened from inside an elevated control still paints above it, matching how those
   /// two already assume they're the top of the stack.
   static const S32 kElevationBase = 10000;

   // Tab order - see isTabbable()/getTabIndex() for the full contract.
   bool mIsTabbable;   ///< Whether this control is a tab stop. Children are still walked even if false.
   S32  mTabIndex;     ///< Explicit tab-order override; -1 = auto (natural tree-walk position).

   SimObjectPtr<NewGuiStyle> mStyle;         ///< Style rule(s) this control was authored with; may be NULL.
   NewGuiResolvedStyle mResolvedStyle;       ///< Fully-resolved cascaded style, valid after StylePass.

   // Tooltip authoring - mTooltipContent takes priority over mTooltipText when both are set.
   StringTableEntry mTooltipText;             ///< Plain-text tooltip; the canvas wraps it in a default styled box.
   SimObjectPtr<NewGuiControl> mTooltipContent;   ///< Author-built subtree shown as-is instead of the default box.
   S32 mTooltipDelayMS;                       ///< Hover delay before the tooltip appears; -1 = use the canvas default.

   /// Fired by a control's own commit path (e.g. NewGuiTextEdit::onCommit_callback,
   /// NewGuiButton::performClick()) after a user-driven value commit, so native code (e.g.
   /// NewGuiInspectorField's write-back path) can react without a script/console round-trip.
   /// Write-side only - nothing pushes a value INTO a control through this.
   std::function<void(NewGuiControl*)> mNativeChangeNotify;
   // Tree-owned interaction state.
   bool mMouseOver;
   bool mMouseActive;      ///< Left mouse currently down on this control.
   bool mFirstResponder;   ///< Keyboard focus.
   bool mDisabled;
   bool mChecked;
   bool mHasError;

   bool mVisible;
   bool mActive;            ///< Participates in layout/input at all.

   /// True (default) if this control can be the result of
   /// findHitControl() - false lets pointer events pass through to
   /// whatever's behind it. Children are still searched regardless.
   bool mHitTestable;

   static bool _setWidth(void* obj, const char* index, const char* data);
   static bool _setHeight(void* obj, const char* index, const char* data);
   static bool _setLeft(void* obj, const char* index, const char* data);
   static bool _setTop(void* obj, const char* index, const char* data);
   static bool _setRight(void* obj, const char* index, const char* data);
   static bool _setBottom(void* obj, const char* index, const char* data);
   static bool _setVisible(void* obj, const char* index, const char* data);
   static bool _setActive(void* obj, const char* index, const char* data);
   static bool _setDisabled(void* obj, const char* index, const char* data);
   static bool _setChecked(void* obj, const char* index, const char* data);
   static bool _setStyle(void* obj, const char* index, const char* data);
   static bool _setTooltipText(void* obj, const char* index, const char* data);
   static bool _setTooltipContent(void* obj, const char* index, const char* data);
   static bool _setRenderLayerOverride(void* obj, const char* index, const char* data);
   static bool _setPreserveAspect(void* obj, const char* index, const char* data);

   /// Marks this control's style dirty and propagates to every child.
   void setStyleDirty();

   /// Marks this control's content dirty and propagates to every ancestor.
   void setContentDirty();

   /// Marks this control's arrangement dirty and propagates to every child.
   void setArrangementDirty();

   /// Raw recursive field write shared by elevateToFront()/clearElevation() - see those methods'
   /// own doc comments. No dirty stamp of its own; both callers stamp once at the top.
   void setRenderLayerOverrideRecursive(S32 layer);

   /// @return The current interaction state mask, recomputed from this control's own bool flags.
   NewGuiStyleStateMask computeStateMask() const;

   void notifyNativeChange() { if (mNativeChangeNotify) mNativeChangeNotify(this); }

public:

   NewGuiControl();
   virtual ~NewGuiControl();

   DECLARE_CONOBJECT(NewGuiControl);

   static void initPersistFields();

   bool onAdd() override;
   void onRemove() override;

   // Re-declared (not just inherited) so add/remove also stamp content/arrangement dirty on this control.
   void addObject(SimObject* object) override;
   void removeObject(SimObject* object) override;

   /// Cascades this control's style (if dirty) then recurses into children.
   /// @param inherited Parent's already-resolved style; pass a default-constructed value for the root.
   /// @param inheritedGeneration Parent's cascade generation, for cache comparison.
   virtual void StylePass(const NewGuiResolvedStyle& inherited, U32 inheritedGeneration);

   /// @return This control's fully-resolved style (valid after StylePass has visited it this frame).
   const NewGuiResolvedStyle& getResolvedStyle() const { return mResolvedStyle; }

   /// Measures children bottom-up, then this control, via ComputePreferredSize().
   /// @return This control's preferred size.
   virtual Point2I MeasurePass();

   /// Computes this control's preferred size. Base default: union of
   /// children's preferred sizes (width = max, height = sum). Leaf
   /// controls override this to measure their own content instead.
   /// @return Preferred size in device pixels.
   virtual Point2I ComputePreferredSize();

   /// Resolves this control's own extent/position within slotRect,
   /// then hands each child a slot via GetChildSlot() and recurses.
   /// @param slotRect Slot offered by the parent.
   /// @param parentRenderLayer Parent's render layer.
   /// @param uiScaleX Horizontal design-to-device scale.
   /// @param uiScaleY Vertical design-to-device scale.
   virtual void ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY);

   /// Like ArrangePass(), but for a parent that has already fully
   /// decided this control's final size/position itself (e.g.
   /// NewGuiStack), rather than offering a slot to resolve against.
   /// finalBounds is used as-is; left/top/right/bottom are not honored.
   /// @param finalBounds This control's final bounds, already resolved by the caller.
   /// @param parentRenderLayer Parent's render layer.
   /// @param uiScaleX Horizontal design-to-device scale.
   /// @param uiScaleY Vertical design-to-device scale.
   virtual void ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY);

   /// The seam a layout-manager subclass overrides for different child placement. Base default:
   /// top-to-bottom flow, full width - see GetChildSlot()'s own definition for the reasoning.
   ///
   /// @param child Child being placed.
   /// @param clientRect This control's own client rect - fixed for the whole loop.
   /// @param remainingRect What's left of clientRect for a flow-participating sibling to use -
   /// see ArrangePass()'s child loop for how/when this is updated (only by flow-participating
   /// children; a self-positioning child never shrinks it).
   /// @return The slot offered to child.
   virtual RectI GetChildSlot(NewGuiControl* child, const RectI& clientRect, const RectI& remainingRect);

   /// Shared by ArrangePass()/ArrangePassWithFixedExtent()'s child loops: shrinks remainingRect by
   /// however much of it childBounds actually occupies, from whichever edge childBounds is flush
   /// against (top-anchored consumes from the top, bottom-anchored consumes from the bottom, a
   /// child spanning both leaves nothing, a child touching neither edge is left as a no-op - see
   /// the call site's own comment for the full reasoning).
   /// @param remainingRect Current remaining rect; updated in place.
   /// @param childBounds The child's own real resolved bounds (mBounds after ArrangePass()).
   static void ShrinkRemainingRect(RectI& remainingRect, const RectI& childBounds);

   /// @return This control's resolved extent, inset by padding and any chrome (e.g. a scrollbar gutter).
   virtual RectI GetClientRect() const;

   /// Resolves one authored dimension into a device-pixel length.
   /// @param dimension Authored value (Auto/Pixels/Percent).
   /// @param referenceLength Parent's resolved extent along this axis, for Percent.
   /// @param preferredLength This control's own preferred length, for Auto.
   /// @param pixelScale Design-to-device scale, for Pixels.
   /// @return Resolved length in device pixels.
   static S32 resolveAxis(const NewGuiDimension& dimension, S32 referenceLength, S32 preferredLength, F32 pixelScale);

   /// @return This control's preferred size, as computed by the last MeasurePass.
   const Point2I& getPreferredSize() const { return mPreferredSize; }

   /// @return This control's resolved bounds, as computed by the last ArrangePass.
   const RectI& getBounds() const { return mBounds; }

   /// @return This control's authored width (Auto/Pixels/Percent), distinct from the resolved extent.
   const NewGuiDimension& getAuthoredWidth() const { return mWidth; }

   /// @return This control's authored height (Auto/Pixels/Percent), distinct from the resolved extent.
   const NewGuiDimension& getAuthoredHeight() const { return mHeight; }

   /// @return This control's authored left inset (Auto/Pixels/Percent) - Auto means unset/not offset from the leading edge.
   const NewGuiDimension& getAuthoredLeft() const { return mLeft; }

   /// @return This control's authored top inset.
   const NewGuiDimension& getAuthoredTop() const { return mTop; }

   /// @return This control's authored right inset.
   const NewGuiDimension& getAuthoredRight() const { return mRight; }

   /// @return This control's authored bottom inset.
   const NewGuiDimension& getAuthoredBottom() const { return mBottom; }

   /// @return This control's resolved paint-order layer.
   S32 getRenderLayer() const { return mRenderLayer; }

   /// @note A one-shot recursive stamp, not a standing policy: a child added to this subtree
   /// AFTER this call (e.g. a NewGuiPopup's content, re-parented in later) does not retroactively
   /// pick up the override, and stays on the ordinary parent+1 default until the next
   /// elevateToFront() call visits it. In practice this is rarely an issue for something like a
   /// popup opened from inside an elevated window, since NewGuiPopup's own kPopupLayer (40000)
   /// already sits above the whole elevateToFront() band (kElevationBase..40000) regardless.
   void elevateToFront();

   /// Reverts an elevateToFront() call on this control AND every descendant, recursively,
   /// returning the whole subtree to the ordinary parent.mRenderLayer + 1 default. Marks
   /// arrangement dirty - see elevateToFront()'s own note on that.
   void clearElevation();

   /// @return True if this control's current render-layer override falls in elevateToFront()'s
   /// reserved band - i.e. it (or an elevateToFront() call on an ancestor) was elevated and
   /// hasn't been cleared since.
   bool isElevated() const { return mRenderLayerOverride >= kElevationBase; }

   /// Draws this control (if visible) then recurses into children via renderChildControls().
   /// @param batch Render batch to draw into.
   /// @param parentLayer Parent's render layer.
   virtual void RenderPass(NewGuiRenderBatch* batch, S32 parentLayer);

   /// Draws this control's own content. A leaf pushes its own quads/text;
   /// a container typically pushes background/border and relies on
   /// renderChildControls() for children. Base default paints the "background"
   /// and "border" parts from style.findSkinImage() when the resolved style
   /// defines a skin image for that part (see NewGuiSkinImageDef), falling
   /// back independently per-part to the equivalent flat-color painting 
   /// (backgroundColor fill / borderColor lines) otherwise. A control with
   /// its own extra skinnable parts (thumb, track, glyphs, titlebar, ...)
   /// should follow the same style.findSkinImage(partName)-then-fallback
   /// pattern in its own override - see NewGuiStyleDrawSkinImage().
   /// @param batch Render batch to draw into.
   /// @param bounds This control's resolved bounds.
   /// @param style This control's resolved style.
   /// @param layer This control's render layer.
   virtual void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer);

   /// Recurses RenderPass() into every child, in tree order. Override
   /// to wrap children in a clip rect or similar - push before calling
   /// Parent::renderChildControls(), pop after it returns.
   /// @param batch Render batch to draw into.
   virtual void renderChildControls(NewGuiRenderBatch* batch);

   /// Appends this control (if tabbable, active, and visible) then
   /// recurses into every child, in the same order as RenderPass().
   /// @param outStops Receives tab stops in tree order.
   virtual void collectTabStops(Vector<NewGuiControl*>& outStops);

   /// Hit-tests against already-resolved bounds.
   /// @param point Point to test, in the same space as mBounds.
   /// @return The deepest hit-testable control under point, or NULL.
   virtual NewGuiControl* findHitControl(const Point2I& point);

   virtual void onOutsideHitTest(NewGuiControl* hit) {}

   /// Handles one input event not already covered by onMouseDown/Up/Enter/Leave.
   /// Set event.handled = true to claim it and stop it reaching ancestors.
   /// @param event Event to handle.
   virtual void onInputEvent(NewGuiInputEvent& event);

   virtual void onMouseEnter(NewGuiInputEvent& event);
   virtual void onMouseLeave(NewGuiInputEvent& event);

   /// Base default claims the event (event.handled = true).
   virtual void onMouseDown(NewGuiInputEvent& event);
   virtual void onMouseUp(NewGuiInputEvent& event);

   /// Fired when this control is the first responder and an activate
   /// input arrives (keyboard Enter/Space, or a gamepad confirm
   /// button) - the keyboard/gamepad equivalent of a click. Base
   /// default is a no-op; NewGuiButton overrides this to click itself.
   /// @param event Event to handle.
   virtual void onActivate(NewGuiInputEvent& event);

   /// Fired when a registered accelerator (see NewGuiCanvas::registerAccelerator())
   /// matches, regardless of what currently has keyboard focus. Base default is a
   /// no-op; NewGuiButton overrides this to call performClick(), so an accelerator-
   /// triggered activation is indistinguishable downstream from a real mouse click.
   virtual void onAccelerator() {}

   /// True if this control wants raw keyboard input routed to it while focused, in
   /// preference to being intercepted by the canvas's accelerator table (see
   /// NewGuiCanvas::checkAccelerators()). Base default false; NewGuiTextEdit overrides
   /// to true while it is the first responder, so its own Ctrl+C/X/V (and ordinary
   /// typing) is never stolen by an unrelated control's accelerator binding.
   virtual bool wantsRawKeyboardInput() const { return false; }

   // Interaction-state mutators - each marks style dirty, never touches layout state.
   void setMouseOver(bool over);
   void setMouseActive(bool active);
   virtual void setFirstResponder(bool responder);
   void setDisabled(bool disabled);
   void setChecked(bool checked);
   void setHasError(bool error);
   void setNativeChangeNotify(std::function<void(NewGuiControl*)> fn) { mNativeChangeNotify = std::move(fn); }

   bool isMouseOver() const { return mMouseOver; }
   bool isMouseActive() const { return mMouseActive; }
   bool isFirstResponder() const { return mFirstResponder; }
   bool isDisabled() const { return mDisabled; }
   bool isChecked() const { return mChecked; }
   bool hasError() const { return mHasError; }

   bool isVisible() const { return mVisible; }
   void setVisible(bool visible) { mVisible = visible; }

   bool isActive() const { return mActive; }

   bool isHitTestable() const { return mHitTestable; }
   void setHitTestable(bool testable) { mHitTestable = testable; }

   bool isTabbable() const { return mIsTabbable; }
   void setTabbable(bool tabbable) { mIsTabbable = tabbable; }

   S32 getTabIndex() const { return mTabIndex; }
   void setTabIndex(S32 index) { mTabIndex = index; }
   void setRenderLayerOverride(S32 layer) { mRenderLayerOverride = layer; }

   bool getPreserveAspect() const { return mPreserveAspect; }
   void setPreserveAspect(bool preserve) { mPreserveAspect = preserve; setArrangementDirty(); }

   /// @return The design-to-device scale ArrangePass() last resolved this control's Pixels-mode values against.
   F32 getResolvedUIScaleX() const { return mResolvedUIScaleX; }
   F32 getResolvedUIScaleY() const { return mResolvedUIScaleY; }

   /// Pushes a cursor shape onto the real platform cursor stack, via
   /// this control's owning canvas. If a NewGuiCursor is bound with an
   /// image for this shape, the canvas draws that image instead.
   /// @param shape Cursor shape to push.
   void pushCursor(NewGuiCursorShape shape);

   /// Pops whatever this control's most recent pushCursor() pushed.
   void popCursor();

   /// @return This control's owning NewGuiCanvas, found by walking up via getGroup(), or NULL if unattached.
   class NewGuiCanvas* getOwningCanvas() const;

   void setStyleAsset(NewGuiStyle* style);
   NewGuiStyle* getStyleAsset() const { return mStyle; }

   void setTooltipText(const char* text) { mTooltipText = text ? StringTable->insert(text) : NULL; }
   const char* getTooltipText() const { return mTooltipText ? mTooltipText : ""; }

   void setTooltipContent(NewGuiControl* content) { mTooltipContent = content; }
   NewGuiControl* getTooltipContent() const { return mTooltipContent; }

   /// @return True if this control has a tooltip authored (either plain-text or content form).
   bool hasTooltip() const { return mTooltipContent != NULL || (mTooltipText && mTooltipText[0]); }

   void setTooltipDelay(S32 ms) { mTooltipDelayMS = ms; }
   S32 getTooltipDelay() const { return mTooltipDelayMS; }
};

#endif // _NEWGUICONTROL_H_
