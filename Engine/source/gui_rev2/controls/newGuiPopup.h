//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiPopup.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIPOPUP_H_
#define _NEWGUIPOPUP_H_

#ifndef _NEWGUIBUTTON_H_
#include "gui_rev2/controls/newGuiButton.h"
#endif

class NewGuiPopupGroup;

/// A NewGuiButton that opens a floating content subtree above everything else on click,
/// dismissed by an outside click. Has no relationship with NewGuiCanvas - the tree root is
/// found by walking getGroup() upward, and click-outside-to-close is driven generically via
/// NewGuiControl::onOutsideHitTest() (see NewGuiPopupGroup).
///
/// Content is authored as a child with internalName = "content". It is not arranged as an
/// ordinary child - it's moved into a NewGuiPopupGroup, which gets re-parented to be the last
/// child of the tree root while open (and removed again on close), so tree-order hit-testing
/// agrees with its render layer. See NewGuiPopupGroup's class doc for the full reasoning.
///
/// @code
/// new NewGuiPopup(FileMenu)
/// {
///    text = "File";
///
///    new NewGuiStack()
///    {
///       internalName = "content";
///       axis = "vertical";
///       sizeChildren = true;
///
///       new NewGuiButton() { text = "New"; command = "..."; };
///       new NewGuiButton() { text = "Open"; command = "..."; };
///
///       new NewGuiPopup()   // nested = submenu, opens off this row
///       {
///          text = "Export";
///          preferredEdge = "right";
///          openOnHoverInMenu = "true";
///          new NewGuiStack() { internalName = "content"; ... };
///       };
///    };
/// };
/// @endcode
class NewGuiPopup : public NewGuiButton
{
public:

   typedef NewGuiButton Parent;

   /// Which edge of the anchor a popup prefers to open against. Defaults to Bottom (a
   /// dropdown); a submenu typically authors preferredEdge = "right". Flips to the opposite
   /// edge if the preferred side doesn't fit.
   enum PreferredEdge : U8
   {
      Edge_Bottom = 0,
      Edge_Right,
      Edge_Top,
      Edge_Left,
   };

   /// Fixed paint-order layer forced onto mPopupGroup so it paints above ordinary tree content.
   /// Below the canvas's own tooltip layer (50000).
   static const S32 kPopupLayer = 40000;

protected:

   NewGuiControl* mContent;           ///< Child with internalName == "content". Lives inside mPopupGroup once opened at least once.
   NewGuiPopupGroup* mPopupGroup;     ///< Lazily created on first open. Holds mContent; re-parented to the tree root while open.
   bool mOpen;
   PreferredEdge mPreferredEdge;
   bool mOpenOnHover;                 ///< Menu-bar siblings: hovering switches away from an already-open sibling under the same parent.
   bool mOpenOnHoverInMenu;           ///< Submenu rows: hovering opens this popup once its own parent popup is already open.

   static bool _setPreferredEdge(void* obj, const char* index, const char* data);

   /// @return The nearest NewGuiPopup ancestor, or NULL if this is top-level. Bridges across
   /// the mPopupGroup reparent via NewGuiPopupGroup::getOwner().
   NewGuiPopup* findParentPopup() const;

   /// @return The topmost ancestor in the tree - the clamp target for computePlacement() and
   /// the reparent target for mPopupGroup.
   NewGuiControl* findTreeRoot() const;

   /// Computes mPopupGroup's placement rect, in device-space screen coordinates, against this
   /// control's own mBounds - tries mPreferredEdge first, flips if it doesn't fit within the
   /// tree root's bounds, then clamps.
   /// @param contentPreferredSize mContent's measured preferred size, in design space.
   RectI computePlacement(const Point2I& contentPreferredSize) const;

   /// Lazily creates mPopupGroup and moves mContent into it, once, on first open.
   void ensurePopupGroup();

   /// Runs a real Style/Measure/Arrange pass on mPopupGroup and positions it via
   /// computePlacement().
   void repositionPopupGroup();

public:

   NewGuiPopup();
   virtual ~NewGuiPopup();

   DECLARE_CONOBJECT(NewGuiPopup);

   static void initPersistFields();

   /// Finds the child with internalName == "content" and hides it.
   void addObject(SimObject* object) override;

   /// @return True if this popup is nested inside another popup's content (a submenu).
   bool isNested() const { return findParentPopup() != NULL; }

   bool isOpen() const { return mOpen; }

   /// Opens this popup: ensures mPopupGroup exists, re-parents it to the tree root, positions
   /// and shows it.
   void openPopup();

   /// Closes this popup: removes mPopupGroup from the tree root and hides it. Nested popups
   /// close automatically along with it.
   void closePopup();

   /// Runs Parent::performClick() first, then opens or closes - so a popup trigger arms on
   /// Down and fires on Up exactly like any other button.
   void performClick() override;

   void onMouseEnter(NewGuiInputEvent& event) override;

   DECLARE_CALLBACK(void, onOpen, ());
   DECLARE_CALLBACK(void, onClose, ());
};

#endif // _NEWGUIPOPUP_H_
