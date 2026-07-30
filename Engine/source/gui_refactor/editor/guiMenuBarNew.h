//-----------------------------------------------------------------------------
// guiMenuBarNew.h
//
// TORQUE_TOOLS only (see gui-migration-plan.md/GuiCanvasNew::setMenuBar()) --
// this is the "File | Edit | View | ..." strip along the top of a tool
// window. It is NOT used in shipped games; it only exists so
// torque_tools-configured builds (the editor, world builder, etc) have a
// standard menu bar chrome to attach to a GuiCanvasNew via setMenuBar().
//
// Each top-level entry (a "menu", e.g. "File") is a button-like field in
// this horizontal strip. Clicking one opens a GuiPopupMenuNew (see
// guiPopupMenuNew.h) anchored directly beneath that field, populated with
// whatever items were added to it via addMenuItem()/addMenuSeparator().
// Built entirely against the new GuiControlNew/GuiStyle/GuiRenderBatch stack
// -- no GuiControlProfile, no immediate-mode GFXDrawUtil calls. See
// gui-rewrite-design.md/gui-migration-plan.md for the architecture this
// fits into, and guiPopupMenuNew.h for the dropdown mechanics this reuses
// directly rather than re-implementing.
//
// Typical script use:
//
//    %bar = new GuiMenuBarNew() { style = "MenuBarStyle"; };
//    %fileMenu = %bar.addMenu( "File" );
//    %bar.addMenuItem( %fileMenu, "New",  "onNewFile();" );
//    %bar.addMenuItem( %fileMenu, "Open", "onOpenFile();" );
//    %bar.addMenuSeparator( %fileMenu );
//    %bar.addMenuItem( %fileMenu, "Exit", "onExit();" );
//    %canvas.setMenuBar( %bar );
//
// Layout: this control does not participate in the anchor/dimension
// layout system for ITS OWN top-level fields -- top-level menu widths are
// derived directly from their text (see _layoutMenus()), the same
// "content determines size" approach GuiPopupMenuNew's autoSizeToContent()
// already uses for dropdown rows, since a menu bar's own field widths
// are exactly analogous to a dropdown's row widths. The control's own
// overall extent (height in particular) still goes through the normal
// mHeight/mWidth fields as usual -- only the INTERNAL per-item widths are
// self-sized.
//-----------------------------------------------------------------------------

#ifndef _GUIMENUBAR_H_
#define _GUIMENUBAR_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _GUIPOPUPMENU_H_
#include "gui_refactor/editor/guiPopupMenuNew.h"
#endif

/// One top-level entry in a GuiMenuBarNew (e.g. "File") -- holds its own
/// dropdown contents directly as a GuiPopupMenuNew instance (unregistered/
/// hidden until actually opened -- see GuiMenuBarNew::onMenuFieldClicked()),
/// rather than building a fresh GuiPopupMenuNew from scratch on every click.
/// Reusing the same instance across opens keeps script-added items/
/// submenus stable without needing to re-populate on every click.
struct GuiMenuBarEntry
{
   String        mText;
   bool          mEnabled;
   GuiPopupMenuNew *mDropdown; // owned: registered but never made visible/pushed except while actually open

   /// Cached, logical-unit rect of this entry's own field within the bar
   /// (relative to the bar's top-left) -- recomputed by _layoutMenus()
   /// whenever entries change or the bar resizes. Used by both
   /// onRender() and hit-testing (findMenuAt()) the same way
   /// GuiPopupMenuNew::getItemRect() is used for dropdown rows.
   RectI mFieldRect;

   GuiMenuBarEntry()
      : mEnabled( true ), mDropdown( NULL ) {}
};

/// See file header.
class GuiMenuBarNew : public GuiControlNew
{
   public:

      typedef GuiControlNew Parent;

   protected:

      Vector< GuiMenuBarEntry > mMenus;

      /// Index into mMenus of the field currently under the mouse, or -1.
      S32 mHighlightIndex;

      /// Index into mMenus whose dropdown is CURRENTLY open, or -1. Only
      /// one top-level dropdown can be open at a time -- opening a
      /// different one closes whichever was open first (matching every
      /// OS menu bar's "hover across the bar while a menu is open swaps
      /// which dropdown shows" behavior).
      S32 mOpenIndex;

      /// Horizontal padding (logical units) on either side of each
      /// field's own text -- mirrors GuiPopupMenuNew's own row-padding
      /// constant rather than introducing a second unrelated magic
      /// number for essentially the same visual idea.
      S32 mFieldPaddingH;

      /// Recomputes every entry's mFieldRect from its text width plus
      /// mFieldPaddingH, laid out left-to-right starting at (0,0) --
      /// called whenever mMenus changes (addMenu()) or this control
      /// resizes (see resize() override).
      void _layoutMenus();

      /// Returns the index into mMenus whose mFieldRect contains
      /// localPoint, or -1.
      S32 _findMenuAt( const Point2I &localPoint ) const;

      /// Called once a click has resolved to entry index -- closes
      /// whichever dropdown was previously open (if a different one),
      /// then opens entry's own dropdown anchored beneath its field, via
      /// GuiPopupMenuNew::showAt(). If entry's dropdown is ALREADY the one
      /// open, this instead just closes it (matching standard menu-bar
      /// "click the already-open menu's own field again to dismiss"
      /// behavior).
      void _openOrCloseMenu( S32 index );

      /// Registered as mMenus[index].mDropdown's onClose-equivalent path
      /// -- since GuiPopupMenuNew doesn't have a dedicated close callback
      /// (see guiPopupMenuNew.h), this bar instead just polls mOpenIndex's
      /// dropdown's registration state each time it needs to know
      /// whether that dropdown is still open (see isMenuOpen()) rather
      /// than needing a push notification.
      bool _isDropdownOpen( S32 index ) const;

   public:

      DECLARE_CONOBJECT( GuiMenuBarNew );
      DECLARE_CATEGORY( "Gui Editor" );
      DECLARE_DESCRIPTION( "A horizontal strip of top-level dropdown menus (File/Edit/...), for "
         "TORQUE_TOOLS builds only -- see GuiCanvasNew::setMenuBar()." );

      GuiMenuBarNew();
      virtual ~GuiMenuBarNew();

      static void initPersistFields();

      /// @name Content
      /// @{

      /// Adds a new top-level field (e.g. "File") and returns its
      /// GuiPopupMenuNew, already registered and ready to receive
      /// addItem()/addSeparator()/addSubMenuItem() calls directly (see
      /// guiPopupMenuNew.h) -- GuiMenuBarNew itself only owns the top-level
      /// strip; populating each dropdown's own contents is done through
      /// the returned GuiPopupMenuNew's own API rather than duplicating
      /// that API a second time here.
      GuiPopupMenuNew* addMenu( const String &text, bool enabled = true );

      /// Convenience pass-throughs so script can populate a menu without
      /// needing to hold onto the GuiPopupMenuNew* addMenu() returned --
      /// menuIndex is the index addMenu() returned (or findMenu()'s
      /// result). Equivalent to calling the same-named method directly
      /// on getMenu(menuIndex).
      S32 addMenuItem( S32 menuIndex, const String &text, const String &consoleCommand, bool enabled = true );
      void addMenuSeparator( S32 menuIndex );

      /// Returns the top-level dropdown for menuIndex, or NULL if out of
      /// range -- lets script hold onto a menu by index (e.g. from
      /// addMenu()'s return) and populate/query it later without a
      /// separate object reference.
      GuiPopupMenuNew* getMenu( S32 menuIndex ) const;

      /// Returns the index of the first top-level entry whose text
      /// matches, or -1 -- convenience for script that only knows a
      /// menu's label (e.g. "File") rather than the index addMenu()
      /// originally returned.
      S32 findMenu( const String &text ) const;

      S32 getMenuCount() const { return mMenus.size(); }

      /// Removes every top-level entry (closing any open dropdown
      /// first).
      void clearMenus();
      /// @}

      /// @name Layout / Rendering
      /// @{
      bool resize( const Point2I &newPosition, const Point2I &newExtent ) override;
      void onRender( Point2I offset, const RectI &updateRect ) override;
      /// @}

      /// @name Events
      /// @{
      bool onWake() override;
      void onSleep() override;
      void onMouseMove( const GuiEvent &event ) override;
      void onMouseLeave( const GuiEvent &event ) override;
      void onMouseDown( const GuiEvent &event ) override;
      /// @}

      /// See guiCanvasNew.cpp's setMenuBar() -- the rewritten GuiCanvasNew now
      /// drives accelerator registration entirely through the standard
      /// GuiControlNew::buildAcceleratorMap()/addAcceleratorKey() mechanism
      /// (every top-level canvas child, menu bar included, is walked the
      /// same way any other control is), so GuiMenuBarNew itself needs no
      /// menu-bar-specific accelerator API of its own. This class
      /// intentionally does NOT declare buildWindowAcceleratorMap()/
      /// removeWindowAcceleratorMap() -- those belonged to a separate,
      /// OS-window-level accelerator hook (WindowInputGenerator) that
      /// doesn't exist anywhere in the rewritten GUI stack.
};

#endif // _GUIMENUBAR_H_
