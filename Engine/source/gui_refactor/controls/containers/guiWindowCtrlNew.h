//-----------------------------------------------------------------------------
// guiWindowCtrlNew.h
//
// GuiWindowCtrlNew -- a floating window container: optional title bar (drag
// to move, optional close button), background/border fill, and children
// clamped to the client area.
//
// This first pass adds minimize (collapse to just the title bar /
// restore), maximize (fill the parent / restore), and resize (drag a
// bottom-right grip). All three are just ordinary resize()/setPosition()
// calls under the hood -- there's no separate "collapsed" rendering mode
// or anything like that; minimized is simply "this window's own height
// currently equals _getMinExtentWithChrome().y (title-bar-only height),
// with mRestoreBounds remembering what to resize back to." Maximize is
// the same idea against the parent's full extent instead. The two are
// mutually exclusive (maximizing while minimized restores height first,
// and vice versa) and share the one mRestoreBounds slot since only one
// can be active at a time.
//
// Docking is still an open design question (see below) and still not
// attempted here.
//
// Placement: works both auto-resolved (percent/anchor fields inherited
// from GuiControlNew, e.g. centered via centerHorizontal/centerVertical)
// and designed/fixed placement (plain pixel left/top/width/height) --
// nothing window-specific is needed for either, since that's just the
// ordinary GuiDimension field behavior every control already has. The
// window-specific parts are: dragging the title bar calls setPosition()
// directly (moving is a runtime interaction, not a layout resolve), and
// children stay confined to the client area via the base class's existing
// containment clamp (mAllowOverflow defaults false -- see
// GuiControlNew::resolveLayout()) which this class doesn't override, so
// it's inherited for free.
//
// Title bar fill comes from a dedicated 'titleBarStyle' GuiStyle
// reference, same pattern as GuiScrollCtrlNew's mThumbStyle: resolved
// with its own state mask (Active while being dragged) and falling back
// to this control's own 'style' if unset. NOTE: the title CAPTION TEXT
// still colors/fonts itself from this control's own 'style' (via
// renderText()), not titleBarStyle -- see onRender()'s comment for why
// that's a known follow-up rather than wired now. The close button is a
// small hit-region drawn/handled directly by this class, not a real
// GuiButtonCtrlNew child -- keeps it out of the child list so it can
// never be mistaken for window content (clipped, iterated, scrolled,
// etc. the way real children are).
//-----------------------------------------------------------------------------

#ifndef _GUIWINDOWCTRLNEW_H_
#define _GUIWINDOWCTRLNEW_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _GUITEXT_H_
#include "gui_refactor/core/guiText.h"
#endif

class GuiRenderBatch;

class GuiWindowCtrlNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

protected:

   /// If false, no title bar is drawn/hit-tested at all -- the whole
   /// control is just a bordered/filled box around its children, and
   /// mMovable/mShowCloseButton are both moot (nothing to drag or close
   /// from). Independent of mMovable: a title bar can be shown purely as
   /// a caption without allowing drag (see mMovable).
   bool mShowTitleBar;

   /// Caption text drawn in the title bar. Meaningless if !mShowTitleBar.
   String mText;
   GuiText mGuiText;

   /// Fixed height of the title bar, logical units. Meaningless if
   /// !mShowTitleBar.
   S32 mTitleBarHeight;

   /// If true (and mShowTitleBar), dragging the title bar moves the
   /// window. Separate from mShowTitleBar so a window can show a caption
   /// but stay put (e.g. a fixed HUD panel with a label strip).
   bool mMovable;

   /// If true (and mShowTitleBar), a close button is drawn at the title
   /// bar's right edge; clicking it fires onWindowClose() (default: hides
   /// the window -- see that method).
   bool mShowCloseButton;

   /// Style consulted for the title bar fill/text and close button --
   /// resolved with its own state mask (Active while dragging -- see
   /// _getTitleBarStyleStateMask()), same pattern as GuiScrollCtrlNew's
   /// mThumbStyle. Falls back to this control's own 'style' if unset.
   GuiStyle* mTitleBarStyle;

   /// True while the user has the title bar actively grabbed for a move.
   bool mDraggingTitleBar;

   /// Device-pixel offset from the window's own top-left to the point
   /// where the drag was grabbed -- keeps the cursor's grab point fixed
   /// on the title bar as the window moves, rather than snapping the
   /// window's corner to the cursor.
   Point2I mDragGrabOffsetDevice;

   /// If true, a minimize (collapse-to-titlebar) button is drawn in the
   /// title bar, left of the maximize/close buttons. Meaningless if
   /// !mShowTitleBar.
   bool mMinimizable;

   /// If true, a maximize (fill-parent) button is drawn in the title
   /// bar, left of the close button. Meaningless if !mShowTitleBar.
   bool mMaximizable;

   /// If true, a small grip in the bottom-right corner can be dragged to
   /// resize the window (clamped to getMinExtent() and, when
   /// mShowTitleBar, tall enough to keep every title bar button from
   /// overlapping -- see _getMinExtentWithChrome()). Independent of
   /// mMovable/mShowTitleBar -- a window can be resizable without a
   /// title bar at all.
   bool mResizable;

   /// True while the window is collapsed to just its title bar (client
   /// area hidden, mRestoreBounds holds what to resize back to on
   /// restore). Mutually exclusive with mMaximized -- see
   /// setMinimized()/setMaximized().
   bool mMinimized;

   /// True while the window is filling its parent's full extent.
   /// Mutually exclusive with mMinimized.
   bool mMaximized;

   /// Position+extent to resize back to on restore -- set the moment
   /// either setMinimized(true) or setMaximized(true) runs (whichever
   /// ISN'T already true; see those methods), read by whichever restore
   /// call comes next. Meaningless when neither mMinimized nor
   /// mMaximized is true.
   RectI mRestoreBounds;

   /// True while the user has the resize grip actively grabbed.
   bool mDraggingResize;

   /// Device-pixel offset from the window's bottom-right corner to the
   /// point where the resize grab started -- same "keep the cursor's
   /// grab point fixed" rationale as mDragGrabOffsetDevice, just for the
   /// opposite corner.
   Point2I mResizeGrabOffsetDevice;

   /// Device-pixel rect of the title bar, relative to this control's own
   /// onRender() offset. Empty if !mShowTitleBar.
   RectI _getTitleBarDeviceRect(const RectI& ctrlDeviceRect) const;

   /// Device-pixel rect of the close button within the title bar. Empty
   /// if !mShowTitleBar || !mShowCloseButton.
   RectI _getCloseButtonDeviceRect(const RectI& ctrlDeviceRect) const;

   /// Device-pixel rect of the maximize button, immediately left of the
   /// close button (or where the close button WOULD be, so the maximize
   /// button doesn't jump around if showCloseButton is toggled). Empty
   /// if !mShowTitleBar || !mMaximizable.
   RectI _getMaximizeButtonDeviceRect(const RectI& ctrlDeviceRect) const;

   /// Device-pixel rect of the minimize button, immediately left of the
   /// maximize button (or where it would be). Empty if !mShowTitleBar ||
   /// !mMinimizable.
   RectI _getMinimizeButtonDeviceRect(const RectI& ctrlDeviceRect) const;

   /// Device-pixel rect of the bottom-right resize grip. Empty if
   /// !mResizable.
   RectI _getResizeGripDeviceRect(const RectI& ctrlDeviceRect) const;

   U32 _getTitleBarStyleStateMask() const;

   /// getMinExtent(), further raised (if mShowTitleBar) so the title
   /// bar's own buttons can never be squeezed into overlapping each
   /// other -- resize()/the resize-grip drag both clamp against this
   /// instead of calling getMinExtent() directly.
   Point2I _getMinExtentWithChrome() const;

   static bool setTitleBarStyleProt(void* object, const char* index, const char* data);
   static bool setTextProt(void* object, const char* index, const char* data);

public:

   GuiWindowCtrlNew();

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiWindowCtrlNew);
   DECLARE_CATEGORY("Gui Containers");
   DECLARE_DESCRIPTION("Floating window container with an optional draggable title bar and close button.");

   bool onWake() override;
   void onSleep() override;
   void onRemove() override;
   void onDeleteNotify(SimObject* object) override;

   void setTitleBarStyle(GuiStyle* style);
   GuiStyle* getTitleBarStyle() const { return mTitleBarStyle; }

   void setText(const String& text);
   const String& getText() const { return mText; }

   bool isShowingTitleBar() const { return mShowTitleBar; }
   bool isMovable() const { return mMovable; }

   bool isResizable() const { return mResizable; }
   bool isMinimizable() const { return mMinimizable; }
   bool isMaximizable() const { return mMaximizable; }

   bool isMinimized() const { return mMinimized; }
   bool isMaximized() const { return mMaximized; }

   /// Collapses the window to just its title bar (true) or restores it
   /// to mRestoreBounds (false). No-op if already in the requested
   /// state. Minimizing while maximized restores the maximized bounds
   /// first, then collapses THAT -- mirrors how a restore-then-minimize
   /// click sequence would behave, and keeps mRestoreBounds meaning
   /// "the one true prior size" rather than needing two separate saved
   /// rects. Fires onWindowMinimize_callback()/onWindowRestore_callback()
   /// -- see .cpp.
   void setMinimized(bool minimized);

   /// Grows the window to fill its parent's full extent (true) or
   /// restores it to mRestoreBounds (false). No-op if already in the
   /// requested state. Maximizing while minimized restores the
   /// minimized(pre-collapse) bounds first, same rationale as
   /// setMinimized(). Fires onWindowMaximize_callback()/
   /// onWindowRestore_callback() -- see .cpp.
   void setMaximized(bool maximized);

   /// Called when the close button is clicked. Default hides the window
   /// (setVisible(false)) -- override/script-hook (onWindowClose callback,
   /// see .cpp) for destroy-on-close or other behavior instead.
   virtual void onWindowClose();

   void onMouseDown(const GuiEvent& event) override;
   void onMouseDragged(const GuiEvent& event) override;
   void onMouseUp(const GuiEvent& event) override;

   bool resize(const Point2I& newPosition, const Point2I& newExtent) override;

   void onRender(Point2I offset, const RectI& updateRect) override;

protected:

   DECLARE_CALLBACK(void, onWindowClose, ());
   DECLARE_CALLBACK(void, onWindowMinimize, ());
   DECLARE_CALLBACK(void, onWindowMaximize, ());
   DECLARE_CALLBACK(void, onWindowRestore, ());
};

#endif // _GUIWINDOWCTRLNEW_H_
