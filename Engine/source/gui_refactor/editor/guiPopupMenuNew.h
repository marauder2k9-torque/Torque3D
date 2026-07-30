//-----------------------------------------------------------------------------
// guiPopupMenuNew.h
//
// TORQUE_TOOLS only. A lightweight dropdown list control: a background box
// containing a vertical stack of text items, each of which behaves like a
// button (hover highlight, click fires a callback/console command). Used
// by GuiMenuBarNew to show a top-level menu's items when it's clicked, but
// written generically enough that anything else needing a simple popup
// list (a context menu, a combo-box dropdown, etc) can reuse it directly.
//
// Built entirely against the new GuiControlNew/GuiStyle stack -- no
// GuiControlProfile, no mBitmapArrayRects, no old sizing enums. See
// gui-rewrite-design.md/gui-migration-plan.md for the architecture this
// fits into.
//
// Lifecycle: a GuiPopupMenuNew is a transient overlay, not a persistent
// authored control. Typical use:
//
//    GuiPopupMenuNew *menu = new GuiPopupMenuNew();
//    menu->registerObject();
//    menu->addItem( "New",  "onNewFile();" );
//    menu->addItem( "Open", "onOpenFile();" );
//    menu->addSeparator();
//    menu->addItem( "Exit", "onExit();" );
//    menu->showAt( canvas, screenPos );
//
// Click-away dismissal mechanics (worth being explicit about, since it is
// not obvious from GuiCanvasNew's API alone): GuiCanvasNew::rootMouseDown()
// only ever routes a mouse-down to the FIRST mCapturesInput control it
// finds walking its dialog layers top-down, and if that control's own
// findHitControl() doesn't hit anything more specific inside itself, the
// event still goes to that same control (see GuiControlNew::findHitControl()
// -- a capturing control is the fallback hit for its own empty space).
// That means a click on some OTHER part of the canvas, behind this menu,
// never reaches this menu's onMouseDown() at all unless this menu's own
// bounds cover that area. So: this menu resizes ITSELF to the full canvas
// extent (invisibly -- see onRender(), which only paints the actual
// visible box sub-rect, not the full capture area) when shown. Its own
// mVisibleRect (not mBounds) is what actually gets painted and hit-tested
// against for row selection; everywhere else within the full-canvas
// capture area is "outside the menu" and dismisses it on mouse-down.
//
// The menu pushes itself onto the canvas's dialog stack (see
// GuiCanvasNew::pushDialogControl()) with mCapturesInput = true. Selecting
// an item, or pressing Escape, closes it too. Either way the menu pops
// itself off the dialog stack and deletes itself; callers never need to
// manage its lifetime beyond the initial showAt() call.
//-----------------------------------------------------------------------------

#ifndef _GUIPOPUPMENU_H_
#define _GUIPOPUPMENU_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif

class GuiCanvasNew;
class GuiPopupMenuNew;

/// One row in a GuiPopupMenuNew -- either a clickable text item or a
/// separator (a thin divider line, not clickable, not hoverable).
struct GuiPopupMenuItem
{
   String mText;
   String mConsoleCommand;
   bool   mIsSeparator;
   bool   mEnabled;

   /// Optional submenu -- set via addSubMenuItem(). When non-null,
   /// clicking this item opens childMenu (anchored to this item's right
   /// edge) instead of firing mConsoleCommand and closing the whole
   /// chain. Not owned by GuiPopupMenuItem in the sense of a raw
   /// pointer-delete -- it's a registered SimObject, so it's deleted
   /// through the normal deleteObject() path, same as the top-level menu.
   GuiPopupMenuNew *mSubMenu;

   GuiPopupMenuItem()
      : mIsSeparator( false ), mEnabled( true ), mSubMenu( NULL ) {}
};

/// See file header.
class GuiPopupMenuNew : public GuiControlNew
{
   public:

      typedef GuiControlNew Parent;

   protected:

      Vector< GuiPopupMenuItem > mItems;

      /// Index into mItems of the row currently under the mouse, or -1.
      /// Drives per-row hover highlight in onRender() and is what
      /// onMouseUp() checks against to decide which item was clicked.
      S32 mHighlightIndex;

      /// Height, in logical units, of one item row -- separators use
      /// mSeparatorHeight instead. Both pulled from mStyle if it sets
      /// custom values via inline style fields, else a sensible default.
      S32 mItemHeight;
      S32 mSeparatorHeight;

      /// Horizontal inset (logical units) reserved on the right edge of
      /// every row's clickable area for the submenu arrow glyph -- items
      /// without a submenu just render text into the shortened width
      /// too, matching every OS menu's aligned-arrow-column look.
      S32 mSubMenuArrowInset;

      /// The canvas this menu was shown on -- cached by showAt() so
      /// closeMenu()/onMouseDown outside-click handling doesn't need to
      /// re-derive getRoot() after the menu's already been popped off
      /// the dialog stack (getRoot() only works while still attached).
      GuiCanvasNew *mOwningCanvas;

      /// The actual visible/clickable box, in THIS control's local
      /// coordinates -- see file header's "click-away dismissal
      /// mechanics" note. mBounds itself is resized to cover the whole
      /// canvas (invisibly) so outside clicks reach onMouseDown() at
      /// all; mVisibleRect is the real menu box within that, computed by
      /// autoSizeToContent()+showAt() and used by onRender()/hit-testing
      /// instead of (0,0)-getExtent().
      RectI mVisibleRect;

      /// True once this menu (or a descendant submenu) has already
      /// begun closing -- guards against closeMenu() re-entering itself
      /// (e.g. once from Escape's onKeyDown, again from the dialog-pop's
      /// own onMouseDown-outside path on the same event).
      bool mClosing;

      /// If this menu was opened AS a submenu of some other
      /// GuiPopupMenuNew item, that parent menu -- used so closing this
      /// submenu (via outside click or Escape) doesn't also close the
      /// parent chain unless the click was truly outside the whole
      /// chain. NULL for a top-level menu.
      GuiPopupMenuNew *mParentMenu;

      /// Currently-open submenu launched from one of mItems, if any --
      /// at most one can be open at a time (opening a different item's
      /// submenu, or closing this menu, closes it first).
      GuiPopupMenuNew *mOpenSubMenu;

      /// Returns the logical-space rectangle, relative to mVisibleRect's
      /// own top-left corner (NOT this control's (0,0) -- see
      /// mVisibleRect's doc comment), occupied by mItems[index]. Used by
      /// both onRender() (row backgrounds/text) and hit-testing
      /// (onMouseMove()/onMouseUp()/onMouseDown() all walk this the same
      /// way rather than duplicating the row-height accumulation logic
      /// in four places).
      RectI getItemRect( S32 index ) const;

      /// Returns the item index whose row rect contains
      /// visibleRectLocalPoint (a point already relative to
      /// mVisibleRect's top-left -- callers translate from raw local/
      /// event coordinates by subtracting mVisibleRect.point first), or
      /// -1 if the point is over a separator or outside every row.
      S32 getItemAt( const Point2I &visibleRectLocalPoint ) const;

      /// Closes and opens submenus as the hover index changes -- called
      /// from onMouseMove() once the new hover index is known. Handles:
      /// closing mOpenSubMenu if hover moved off its owning item, and
      /// opening a new one if hover landed on an item with mSubMenu set
      /// and it isn't already open.
      void updateOpenSubMenu( S32 newHighlightIndex );

      /// Fires mItems[index]'s console command (if any) and closes the
      /// entire menu chain (this menu and every ancestor, up to and
      /// including the top-level menu bar's dropdown) -- selecting a
      /// leaf item should dismiss the whole thing, not just this one
      /// submenu level.
      void selectItem( S32 index );

      /// Recomputes mBounds' extent (logical units) from mItems --
      /// widest text row plus padding for width, summed row/separator
      /// heights for height. Called once by showAt() before the menu is
      /// actually positioned/pushed, since positioning (clamping to stay
      /// on-screen) needs the final extent first.
      void autoSizeToContent();

   public:

      DECLARE_CONOBJECT( GuiPopupMenuNew );
      DECLARE_CATEGORY( "Gui Editor" );
      DECLARE_DESCRIPTION( "A transient dropdown list of clickable text items, used by GuiMenuBarNew "
         "and reusable directly for context menus or combo-box dropdowns. TORQUE_TOOLS only." );

      GuiPopupMenuNew();
      virtual ~GuiPopupMenuNew();

      static void initPersistFields();

      /// @name Content
      /// @{

      /// Appends a clickable text row. consoleCommand is evaluated
      /// (with $ThisControl set to this menu) when the item is clicked;
      /// may be empty for a placeholder/label row. Returns the new
      /// item's index.
      S32 addItem( const String &text, const String &consoleCommand, bool enabled = true );

      /// Appends a non-clickable divider line.
      void addSeparator();

      /// Appends an item that opens childMenu as a submenu (anchored to
      /// this item's right edge) instead of firing a console command --
      /// see mSubMenu's doc comment. childMenu should already be
      /// registered and fully populated (addItem()/addSeparator() calls
      /// already made on it) before being attached here; ownership
      /// transfers to this menu in the sense that closing/deleting this
      /// menu also closes/deletes childMenu if it's currently open (see
      /// closeMenu()).
      S32 addSubMenuItem( const String &text, GuiPopupMenuNew *childMenu, bool enabled = true );

      /// Removes every item, resetting this menu to empty. Does not
      /// affect an already-open mOpenSubMenu until the next
      /// autoSizeToContent()/showAt() call.
      void clearItems();

      S32 getItemCount() const { return mItems.size(); }
      /// @}

      /// @name Showing / Closing
      /// @{

      /// Shows this menu on the given canvas, top-left corner anchored
      /// at screenPos (canvas-global logical coordinates -- e.g. a menu
      /// bar item's own global bottom-left corner), clamped so the whole
      /// menu stays within the canvas's bounds (flips above/left of
      /// screenPos instead of off-screen, the same behavior every OS
      /// menu has near a screen edge). Auto-sizes to content first (see
      /// autoSizeToContent()), then pushes itself onto canvas's dialog
      /// stack with mCapturesInput = true.
      virtual void showAt( GuiCanvasNew *canvas, const Point2I &screenPos );

      /// Convenience overload for opening this menu as a submenu of
      /// parentItemGlobalRect (the already-resolved global bounds of
      /// the menu item that owns this submenu) -- anchors to the item's
      /// top-right corner rather than a raw point, and records
      /// parentMenu so this submenu's outside-click test treats
      /// parentMenu's own bounds as still "inside." Used internally by
      /// updateOpenSubMenu(); exposed in case a caller wants the same
      /// anchor-to-a-rect behavior directly.
      virtual void showAsSubMenu( GuiPopupMenuNew *parentMenu, const RectI &parentItemGlobalRect );

      /// Closes this menu (and, if closeWholeChain is true, every
      /// ancestor menu up to and including the top-level one) --
      /// recursively closes mOpenSubMenu first, pops itself off the
      /// dialog stack, then deletes itself. Safe to call more than once
      /// (guarded by mClosing).
      virtual void closeMenu( bool closeWholeChain = true );
      /// @}

      /// @name Rendering
      /// @{
      void onRender( Point2I offset, const RectI &updateRect ) override;
      /// @}

      /// @name Events
      /// @{
      bool onWake() override;
      void onMouseMove( const GuiEvent &event ) override;
      void onMouseLeave( const GuiEvent &event ) override;

      /// mBounds covers the full canvas (see file header), so this is
      /// where click-away dismissal actually happens: if event.mousePoint
      /// (translated to local space) falls outside mVisibleRect (and,
      /// when this is a submenu, outside every ancestor's mVisibleRect
      /// too -- see mParentMenu), the whole chain closes. Otherwise the
      /// click is inside the visible box and is treated as a row press,
      /// same as onMouseUp() below resolving the actual selection.
      void onMouseDown( const GuiEvent &event ) override;
      void onMouseUp( const GuiEvent &event ) override;
      bool onKeyDown( const GuiEvent &event ) override;
      /// @}
};

#endif // _GUIPOPUPMENU_H_
