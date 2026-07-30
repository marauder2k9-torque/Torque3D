//-----------------------------------------------------------------------------
// guiEditCtrlNew.h
//
// GuiEditCtrlNew -- the first real implementation of the editor hook already
// declared (but unimplemented) throughout guiControlNew.h: mIsContainer,
// smDesignTime/smEditorHandle, and the onMouseDownEditor/onMouseUpEditor/
// onRightMouseDownEditor/onMouseDraggedEditor virtuals all predate this
// file and are simply being wired up here for the first time.
//
// SCOPE (first pass -- see gui-migration-plan.md's "next step" discussion):
//   - Single-selection only (click to select, click empty space to
//     deselect). No multi-select/marquee yet.
//   - Move: drag inside a selected control's body.
//   - Resize: drag one of 8 handles drawn around the selection.
//   - Palette: a strip of buttons, one per registered placeable class;
//     drag from a palette entry onto the edit surface creates a new
//     instance of that class at the drop point.
//   - Explicitly NOT included yet: property inspector, multi-select,
//     keyboard nudge, copy/paste, undo/redo. See migration plan notes on
//     not building speculatively ahead of real need.
//
// WRITE-BACK SEMANTICS: move/resize preserve whichever GuiDimension::Mode
// (Auto/Pixels/Percent) a field already had -- e.g. dragging a
// percent-positioned control writes back a new Percent value (resolved
// against the current parent extent), not a Pixels value that would
// silently opt it out of proportional scaling. A field with no live
// authored value (Auto) becomes Pixels, since Auto has no numeric value
// of its own to preserve. This mirrors _resolveAndApplyDimension()'s own
// handling and goes through the exact same protected entry point (see
// GuiEditCtrlNew's friend access into GuiControlNew), rather than the public
// setLeft()/setTop()/setWidth()/setHeight() setters, which write mBounds
// directly and do NOT update the authored mLeft/mTop/mWidth/mHeight
// fields -- using them here would desync mBounds from the authored
// dimension, and the next layout resolve would silently snap the control
// back to its old position/size.
//-----------------------------------------------------------------------------

#ifndef _GUIEDITCTRLNEW_H_
#define _GUIEDITCTRLNEW_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _GUIDIMENSION_H_
#include "gui_refactor/core/guiDimension.h"
#endif

class GuiCanvasNew;

/// Which resize handle (if any) the mouse is currently down on. Order
/// matches clockwise-from-top-left, purely for readability -- not relied
/// on numerically anywhere.
enum GuiEditHandle : U8
{
   EditHandle_None = 0,
   EditHandle_TopLeft,
   EditHandle_Top,
   EditHandle_TopRight,
   EditHandle_Right,
   EditHandle_BottomRight,
   EditHandle_Bottom,
   EditHandle_BottomLeft,
   EditHandle_Left
};

/// One entry in the editor's placement palette -- a class that can be
/// dragged onto the edit surface to create a new instance.
struct GuiEditPaletteEntry
{
   StringTableEntry className;   ///< Passed to ConsoleObject::create()
   StringTableEntry displayName; ///< Shown on the palette button; defaults to className if not given

   /// Persistent, reused every draw -- see _drawPalette()'s doc comment
   /// on why this must NOT be a fresh stack-local GuiText per frame (the
   /// same bug class already hit once with tooltip rendering; see
   /// GuiControlNew::defaultTooltipRender()'s cache in guiControlNew.cpp
   /// for the original fix and its own doc comment on the cost this
   /// avoids). Configured once in GuiEditCtrlNew::addPaletteEntry() (text
   /// never changes after that) and reconfirmed (font/box-extent only)
   /// every draw in _drawPalette() -- see there for why that's cheap.
   GuiText labelText;
};

/// Overlay control that sits above a canvas's content control and, while
/// active (see setEditActive()), intercepts mouse input to support
/// selecting, moving, and resizing child controls, plus a palette for
/// dragging new controls into the tree. Mirrors old Torque's GuiEditCtrlNew
/// role: mIsContainer is true so controls can be dropped INTO it, and it
/// installs itself as GuiControlNew::smEditorHandle while active so any
/// control's onMouseDownEditor()/onMouseDraggedEditor()/etc. hooks (see
/// guiControlNew.h) can special-case editor interaction if they choose to.
///
/// Usage: create one as the canvas's content control (or a floating
/// dialog), add the control tree to be edited as a CHILD of it (e.g.
/// nested directly in the .gui/script, or via addObject()), call
/// setEditRoot() to point at that child, then setEditActive(true).
/// mEditRoot must be a descendant of this editor -- see mEditRoot's own
/// doc comment for why.
class GuiEditCtrlNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

protected:

   /// The control tree being edited -- new controls are added as its
   /// children; existing children are selectable/movable/resizable.
   /// Must be a descendant of this editor (typically a direct child,
   /// added via addObject() before calling setEditRoot()) so the normal
   /// GuiControlNew render/hit-test traversal reaches it; the editor
   /// does not render or hit-test a control outside its own subtree.
   SimObjectPtr<GuiControlNew> mEditRoot;

   /// Currently selected control, or NULL. Single-selection only -- see
   /// file header.
   SimObjectPtr<GuiControlNew> mSelected;

   /// True while the editor is intercepting input (installed as
   /// smEditorHandle); false lets input pass through to the edited
   /// controls normally, e.g. for spot-checking real interaction without
   /// tearing the editor down.
   bool mEditActive;

   /// @name Drag state
   /// Only one of these is meaningful at a time, discriminated by
   /// mActiveHandle: EditHandle_None + mDragging means a move-drag;
   /// any other value means a resize-drag on that handle.
   /// @{
   bool mDragging;
   GuiEditHandle mActiveHandle;

   /// Mouse position (logical, editor-local) at drag start.
   Point2I mDragStartMouse;

   /// mSelected's bounds at drag start, captured once so every
   /// onMouseDraggedEditor() call computes a fresh delta from the same
   /// baseline rather than compounding per-event rounding error --
   /// same lesson as the compounding-rounding bug noted in the
   /// migration plan for the layout core itself.
   RectI mDragStartBounds;
   /// @}

   /// Half-size of a resize handle's hit/draw box, in logical units.
   static const S32 smHandleHalfSize = 4;

   /// @name Palette
   /// @{
   Vector<GuiEditPaletteEntry> mPaletteEntries;

   /// Editor-local LOGICAL rect of each palette entry's button,
   /// recomputed every onRender() -- simplest way to keep hit-testing
   /// and drawing in sync without a second real control per entry. Kept
   /// in logical units (not device pixels) so hit-testing can compare
   /// directly against event.mousePoint/globalToLocalCoord() results,
   /// which are always logical -- see _hitTestPalette()'s doc comment
   /// for why mixing spaces here previously caused a scale-dependent
   /// cursor/hit offset.
   Vector<RectI> mPaletteButtonRects;

   /// Height in logical units of the palette strip along the top edge.
   static const S32 smPaletteHeight = 32;

   /// Index into mPaletteEntries currently being dragged from the
   /// palette, or -1 if no palette drag is in progress.
   S32 mPaletteDragIndex;

   /// Current logical position of an in-progress palette drag, used to
   /// draw a placement preview and as the drop point on release.
   Point2I mPaletteDragPos;
   /// @}

   /// @return the handle the given editor-local logical point is over,
   /// or EditHandle_None if it's not over any handle of the current
   /// selection.
   GuiEditHandle _hitTestHandle(const Point2I& localPt) const;

   /// @return the 8 handle center points (logical, editor-local) for the
   /// current selection's bounds, indexed by GuiEditHandle (index 0
   /// unused).
   void _getHandlePoints(const RectI& bounds, Point2I outPoints[9]) const;

   /// Writes newBounds back onto target, preserving each axis's existing
   /// GuiDimension::Mode -- see file header's WRITE-BACK SEMANTICS note.
   /// Percent values are resolved against target's current parent
   /// extent (captured once, before any field is written, so left/top/
   /// width/height are all computed against the same reference frame
   /// even though writing one field can change the control's own
   /// extent that a later field's percent math might otherwise read).
   void _applyBoundsPreservingMode(GuiControlNew* target, const RectI& newBounds);

   /// Finds the topmost selectable control under the given editor-local
   /// logical point, searching mEditRoot's descendants. Excludes this
   /// editor and its own palette/handle chrome (not part of the edited
   /// tree). NULL if the point is outside mEditRoot or hits nothing.
   GuiControlNew* _findSelectableAt(const Point2I& localPt) const;

   /// Recursive bounds-only hit walk used by _findSelectableAt() --
   /// deliberately ignores mCanHit/getCapturesInput() (see its own doc
   /// comment in guiEditCtrlNew.cpp for why the engine's real hit-test
   /// paths are the wrong tool for editor selection specifically).
   /// localPt is in ctrl's own local space. Returns the deepest control
   /// (possibly ctrl itself) whose bounds contain localPt, honoring only
   /// isVisible() and z-order (last-added child = topmost, matching
   /// findHitControl()'s own convention).
   static GuiControlNew* _findDeepestAtIgnoringHitFlags(GuiControlNew* ctrl, const Point2I& localPt);

   void _drawSelectionAndHandles(GuiRenderBatch& batch, const Point2I& offset, S32 baseLayer);
   void _drawPalette(GuiRenderBatch& batch, const Point2I& offset, S32 baseLayer);
   void _drawPaletteDragPreview(GuiRenderBatch& batch, const Point2I& offset, S32 baseLayer);

   /// @return the palette entry index at the given editor-local LOGICAL
   /// point, or -1. See mPaletteButtonRects' doc comment for why this
   /// must be logical, not device-pixel.
   S32 _hitTestPalette(const Point2I& localPt) const;

public:

   GuiEditCtrlNew();

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiEditCtrlNew);
   DECLARE_CATEGORY("Gui Editor");
   DECLARE_DESCRIPTION("Overlay editor for selecting, moving, resizing, and placing GUI controls.");

   bool onWake() override;

   /// @name Setup
   /// @{
   void setEditRoot(GuiControlNew* root);
   GuiControlNew* getEditRoot() const { return mEditRoot; }

   /// Registers a class as a palette entry. displayName may be NULL to
   /// default to className.
   void addPaletteEntry(const char* className, const char* displayName = NULL);

   /// Installs/removes this editor as GuiControlNew::smEditorHandle and
   /// toggles GuiControlNew::smDesignTime.
   void setEditActive(bool active);
   bool isEditActive() const { return mEditActive; }
   /// @}

   /// @name Selection
   /// @{
   void select(GuiControlNew* ctrl);
   void deselect() { select(NULL); }
   GuiControlNew* getSelected() const { return mSelected; }
   /// @}

   /// @name Event overrides
   /// Intercepts input while mEditActive; falls through to Parent
   /// otherwise.
   /// @{
   void onRender(Point2I offset, const RectI& updateRect) override;
   void onMouseDown(const GuiEvent& event) override;
   void onMouseUp(const GuiEvent& event) override;
   void onMouseDragged(const GuiEvent& event) override;
   bool onKeyDown(const GuiEvent& event) override;
   /// @}

   /// @name Diagnostics
   /// TEMPORARY -- added to chase down a reported memory-climbs-until-
   /// first-interaction symptom that doesn't match any allocation this
   /// file is known to perform (see the investigation in chat: every
   /// per-frame path in onRender()/its helpers was checked and none
   /// allocate unboundedly). No profiler was available at diagnosis
   /// time, so this exists to let script correlate the climbing number
   /// against this class's own actual per-frame counters instead of
   /// guessing further. Safe to delete once the cause is found.
   /// @{

   /// Call once, wait, call again -- compares mOnRenderCallCount and the
   /// palette-rect vector's capacity (the only per-frame-touched
   /// container in this class) against their previous values and prints
   /// the deltas, so a genuinely growing allocation inside THIS class
   /// would show up as a widening capacity number across calls, while a
   /// flat capacity alongside a climbing external memory reading would
   /// point the leak OUTSIDE this file entirely (GuiCanvasNew's own
   /// per-frame update, GFont, or elsewhere).
   void debugDumpAllocState();
   /// @}

protected:

   /// Incremented once per onRender() call -- see debugDumpAllocState().
   U32 mOnRenderCallCount;
};

#endif // _GUIEDITCTRLNEW_H_
