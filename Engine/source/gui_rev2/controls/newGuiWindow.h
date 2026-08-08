//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiWindow.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIWINDOW_H_
#define _NEWGUIWINDOW_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#ifndef _NEWGUITEXT_H_
#include "gui_rev2/core/newGuiText.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif

/// An embedded, floating panel control: titlebar (drag to move, optional
/// close/minimize/maximize buttons), sizeable via edge/corner grips, brings
/// itself to front on press. This is NOT a new OS-level window - no
/// PlatformWindow is created; it is an ordinary NewGuiControl subtree that
/// behaves like one visually, living inside whatever canvas/parent already
/// hosts it.
///
/// Minimize collapses the window to just its titlebar (no taskbar/dock
/// concept exists yet to restore it FROM once minimized other than script -
/// see performRestore()). Maximize snaps the window to fill its immediate
/// parent's client rect and disables drag/resize until restored - see
/// performMaximize()/performMinimize()/performRestore()'s own doc comments.
///
/// Titlebar height and resize-grip thickness are authored in DESIGN
/// units, same as every other authored dimension on NewGuiControl (see
/// NewGuiControl::resolveAxis()'s doc comment and the class comment on
/// ArrangePass()) - they are only ever converted to device pixels inside
/// ArrangePass()/hit-testing/drawing, using mResolvedUIScaleX/Y, never
/// baked in at MeasurePass() or authoring time. A drag or resize
/// likewise accumulates its delta in device pixels (screenPoint is
/// already device space) and divides by the resolved scale before
/// writing back into mLeft/mTop/mWidth/mHeight, since those authored
/// fields are themselves design-unit values per resolveAxis()'s
/// contract - never write a raw device-pixel delta into an authored field.
///
/// A single content child is expected, authored with no internalName
/// requirement - GetChildSlot()/GetClientRect() give every child the
/// area below the titlebar (inset further by the resolved style's own
/// padding), same shape as NewGuiControl's default flow for more than
/// one child if that's ever authored.
///
/// @code
/// new NewGuiWindow( MyWindow )
/// {
///    left = "100"; top = "80";
///    width = "400"; height = "300";
///    titleText = "Inventory";
///    closable = "true";
///    minimizable = "true";
///    maximizable = "true";
///    resizable = "true";
///
///    new NewGuiStack() { width = "100%"; height = "100%"; ... };
/// };
/// @endcode
class NewGuiWindow : public NewGuiControl
{
public:

   typedef NewGuiControl Parent;

   /// Which edge(s)/corner of the window a resize drag is anchored to -
   /// determines which authored fields (mWidth/mHeight and/or mLeft/mTop)
   /// the drag writes back into. None means "not currently resizing."
   enum ResizeEdge : U8
   {
      Edge_None = 0,
      Edge_Left = BIT(0),
      Edge_Right = BIT(1),
      Edge_Top = BIT(2),
      Edge_Bottom = BIT(3),
   };

protected:

   NewGuiText mTitleText;             ///< Titlebar label text/measurement/drawing.
   Resource<GFont> mTitleFont;
   StringTableEntry mCachedFontFamily;
   F32 mCachedFontSize;

   F32 mTitlebarHeight;               ///< Authored, design units. Titlebar chrome height.
   F32 mResizeGripSize;               ///< Authored, design units. Thickness of the edge/corner hit region.
   F32 mMinWidth;                     ///< Authored, design units. Floor applied to a resize drag's resulting width.
   F32 mMinHeight;                    ///< Authored, design units. Floor applied to a resize drag's resulting height.

   bool mResizable;                   ///< Authored - false disables all edge/corner resize hit-testing.
   bool mMovable;                     ///< Authored - false disables titlebar drag-to-move.
   bool mClosable;                    ///< Authored - true draws the close glyph and enables its hit region.
   bool mMinimizable;                 ///< Authored - true draws the minimize glyph and enables its hit region.
   bool mMaximizable;                 ///< Authored - true draws the maximize/restore glyph and enables its hit region.

   /// True from a titlebar-landing onMouseDown() until the matching
   /// onMouseUp(); while true, Move events reposition the window.
   bool mDragInProgress;
   Point2I mDragLastPoint;            ///< screenPoint as of the last drag update, device pixels.

   /// Nonzero (a combination of ResizeEdge bits) from an edge/corner-landing onMouseDown() until
   /// the matching onMouseUp(); while set, Move events resize the window.
   U8 mResizeEdges;
   Point2I mResizeLastPoint;          ///< screenPoint as of the last resize update, device pixels.

   /// True while the pointer press that armed a close/minimize/maximize glyph click is still
   /// down and still tracking over THAT SAME glyph - mirrors NewGuiButton's mPressArmed/
   /// bounds-on-release contract, scoped to one glyph's own rect rather than the whole control.
   /// Only one of the three is ever meaningfully true at once (a press picks exactly one glyph -
   /// see onMouseDown()), but each is tracked separately so a release is checked against the
   /// SAME glyph the press armed, not whichever glyph the pointer happens to be over at release.
   bool mCloseArmed;
   bool mMinimizeArmed;
   bool mMaximizeArmed;

   /// True once minimized (collapsed to just the titlebar) - see performMinimize()/performRestore().
   bool mMinimized;
   NewGuiDimension mPreMinimizeHeight;   ///< This window's own authored height, captured on performMinimize(), restored on performRestore().

   /// True once maximized (snapped to fill the parent's client rect) - see performMaximize()/
   /// performRestore(). Independent of mMinimized; the two states aren't mutually exclusive
   /// bits of one enum because minimizing an already-maximized window and then restoring should
   /// still land back in the maximized state, not lose it.
   bool mMaximized;
   NewGuiDimension mPreMaximizeLeft;     ///< This window's own authored left/top/width/height,
   NewGuiDimension mPreMaximizeTop;      ///< captured on performMaximize(), restored on
   NewGuiDimension mPreMaximizeWidth;    ///< performRestore(). NewGuiDimension (not a raw F32)
   NewGuiDimension mPreMaximizeHeight;   ///< so an authored Percent/Auto value round-trips exactly.

   /// Which chrome glyph (if any) the pointer currently hovers, for cursor/tint push-pop on
   /// transition only - mirrors NewGuiScroll::mThumbHoverAxis/updateThumbHover()'s reasoning.
   U8 mHoverResizeEdges;
   bool mCloseHovered;
   bool mMinimizeHovered;
   bool mMaximizeHovered;

   static bool _setTitleText(void* obj, const char* index, const char* data);
   static bool _setTitlebarHeight(void* obj, const char* index, const char* data);
   static bool _setResizeGripSize(void* obj, const char* index, const char* data);
   static bool _setResizable(void* obj, const char* index, const char* data);
   static bool _setMovable(void* obj, const char* index, const char* data);
   static bool _setClosable(void* obj, const char* index, const char* data);
   static bool _setMinimizable(void* obj, const char* index, const char* data);
   static bool _setMaximizable(void* obj, const char* index, const char* data);
   static bool _setMinWidth(void* obj, const char* index, const char* data);
   static bool _setMinHeight(void* obj, const char* index, const char* data);

   /// Ensures mTitleFont matches the resolved style's fontFamily/fontSize, reloading only when changed.
   void resolveFont();

   /// @return This window's titlebar rect, in local space (0,0 = mBounds.point), device pixels.
   /// Empty if mTitlebarHeight resolves to 0.
   RectI getTitlebarRect() const;

   /// @return The close glyph's rect within the titlebar, in local space, device pixels.
   /// Empty if !mClosable or the titlebar itself is empty.
   RectI getCloseGlyphRect() const;

   /// @return The maximize/restore glyph's rect within the titlebar, in local space, device
   /// pixels - immediately to the left of the close glyph (or where the close glyph would sit,
   /// if !mClosable). Empty if !mMaximizable or the titlebar itself is empty.
   RectI getMaximizeGlyphRect() const;

   /// @return The minimize glyph's rect within the titlebar, in local space, device pixels -
   /// immediately to the left of the maximize glyph (or where it would sit, if !mMaximizable).
   /// Empty if !mMinimizable or the titlebar itself is empty.
   RectI getMinimizeGlyphRect() const;

   /// Hit-tests localPoint against mBounds' own edges/corners (NOT the titlebar) to determine
   /// which ResizeEdge bit(s), if any, a press/hover at that point would resize. Returns
   /// Edge_None if !mResizable, mMaximized (a maximized window can't be resized until restored -
   /// see class doc comment), or localPoint isn't within mResizeGripSize (device pixels) of any edge.
   U8 hitTestResizeEdges(const Point2I& localPoint) const;

   /// Maps a ResizeEdge combination onto the matching NewGuiCursorShape (diagonal for a corner,
   /// straight for a single edge).
   static NewGuiCursorShape resizeEdgesToCursorShape(U8 edges);

   /// Re-hit-tests localPoint against every titlebar glyph and the resize edges, push/popping
   /// the pointer/resize cursor and glyph hover tints only on transition - mirrors
   /// NewGuiScroll::updateThumbHover(). Skipped entirely while a drag or resize is already in progress.
   void updateHoverState(const Point2I& localPoint);

   /// Applies a device-pixel resize delta for the given edge set: divides by the resolved
   /// per-axis scale to recover a design-unit delta, then writes into mWidth/mHeight (and
   /// mLeft/mTop for edges that also move the origin), clamped to mMinWidth/mMinHeight.
   void applyResizeDelta(U8 edges, const Point2I& deviceDelta);

public:

   NewGuiWindow();
   virtual ~NewGuiWindow();

   DECLARE_CONOBJECT(NewGuiWindow);

   static void initPersistFields();

   /// Sets the titlebar's label text.
   void setTitleText(const char* text);
   const char* getTitleText() const { return mTitleText.getText().c_str(); }

   /// Reparents this window to the end of its own parent's child list (hit-test priority - see
   /// NewGuiControl::findHitControl()) AND calls elevateToFront() (paint priority - see that
   /// method's own doc comment on NewGuiControl). Tree order alone isn't enough once ANY sibling
   /// window has its own render-layer override in play (including another NewGuiWindow this same
   /// method already elevated), since paint order is decided by mRenderLayer, not tree position -
   /// elevateToFront() is what actually guarantees this window paints (and its whole content
   /// subtree, via the normal parent+1 cascade) above every other window regardless of what order
   /// they were last touched in. No-op (tree-order half only skipped) if this window has no
   /// parent yet.
   void bringToFront();

   /// This window's own content area sits below the titlebar - see class doc comment. Collapses
   /// to nothing (zero height) while mMinimized, same as the titlebar-only bounds performMinimize()
   /// leaves the window in.
   /// @return mBounds inset by the titlebar height (device pixels) and the resolved style's padding.
   RectI GetClientRect() const override;

   /// Claims itself directly - WITHOUT recursing into children at all - for any point in the
   /// resize-grip band.
   NewGuiControl* findHitControl(const Point2I& point) override;

   /// Installs a clip rect for the viewport around Parent's own child recursion.
   void renderChildControls(NewGuiRenderBatch* batch) override;

   /// Resolves this window's own bounds via Parent:: normally, EXCEPT while mMaximized: then
   /// mBounds is simply set to slotRect directly
   void ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// Draws background/border via Parent::, then the titlebar chrome (fill, title text,
   /// minimize/maximize/close glyphs per their respective authored flags).
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override;

   /// Bring-to-front on any press within this window, then dispatches to whichever of
   /// close-glyph / maximize-glyph / minimize-glyph / resize-edge / titlebar-drag the press
   /// landed on (in that priority order). A press that lands on none of those is left unhandled
   /// so it can keep bubbling to a normal child in the content area.
   void onMouseDown(NewGuiInputEvent& event) override;
   void onMouseUp(NewGuiInputEvent& event) override;
   void onMouseLeave(NewGuiInputEvent& event) override;

   /// Drives drag-to-move, resize-in-progress, and (when neither is active) hover-cursor updates.
   void onInputEvent(NewGuiInputEvent& event) override;

   /// Measures the title text plus every enabled glyph's own allowance for the titlebar's own
   /// preferred contribution, unioned with Parent's normal children-preferred-size default so an
   /// auto-sized window is never narrower than its own title/chrome.
   Point2I ComputePreferredSize() override;

   bool isResizable() const { return mResizable; }
   void setResizable(bool resizable) { mResizable = resizable; }

   bool isMovable() const { return mMovable; }
   void setMovable(bool movable) { mMovable = movable; }

   bool isClosable() const { return mClosable; }
   void setClosable(bool closable) { mClosable = closable; setContentDirty(); setArrangementDirty(); }

   bool isMinimizable() const { return mMinimizable; }
   void setMinimizable(bool minimizable) { mMinimizable = minimizable; setContentDirty(); setArrangementDirty(); }

   bool isMaximizable() const { return mMaximizable; }
   void setMaximizable(bool maximizable) { mMaximizable = maximizable; setContentDirty(); setArrangementDirty(); }

   bool isMinimized() const { return mMinimized; }
   bool isMaximized() const { return mMaximized; }

   /// Runs the same close path a real click on the close glyph would (onClose callback, then
   /// hides the window) - script-callable without needing a synthetic click.
   virtual void performClose();

   /// Collapses this window to just its titlebar
   virtual void performMinimize();

   /// Snaps this window to fill its parent's client rect
   virtual void performMaximize();

   /// Reverts EITHER (or both) of performMinimize()/performMaximize()
   virtual void performRestore();

   DECLARE_CALLBACK(void, onClose, ());
   DECLARE_CALLBACK(void, onMinimize, ());
   DECLARE_CALLBACK(void, onMaximize, ());
   DECLARE_CALLBACK(void, onRestore, ());
   DECLARE_CALLBACK(void, onWindowMoved, ());
};

#endif // _NEWGUIWINDOW_H_
