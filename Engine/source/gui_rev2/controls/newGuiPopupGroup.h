//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiPopupGroup.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIPOPUPGROUP_H_
#define _NEWGUIPOPUPGROUP_H_

#ifndef _NEWGUISTACK_H_
#include "gui_rev2/controls/newGuiStack.h"
#endif

class NewGuiPopup;

/// Holds a NewGuiPopup's floating content while it's open. NewGuiControl::findHitControl()
/// resolves overlapping siblings by tree order (last-added wins), independent of render layer -
/// so a popup floating above an unrelated sibling would still lose the hit-test race if that
/// sibling came later in the tree. NewGuiPopup fixes this by re-parenting its NewGuiPopupGroup
/// to be the last child of the tree root while open, making tree order agree with paint order.
///
/// Click-outside-to-close runs through onOutsideHitTest() (see
/// newGuiControl_onOutsideHitTest.diff.txt): the tree root calls it once per Down on every
/// direct child, passing whatever the hit-test walk resolved to. This closes mOwner unless the
/// hit landed on this group's own subtree or on mOwner itself (the trigger, which is not a
/// descendant of this group once opened).
///
/// Derives from NewGuiStack rather than NewGuiControl so it gets correct sizing for its one
/// child for free (ComputePreferredSize(), RecomputeContentExtentAndReclamp()).
///
/// mBounds is forced to mFixedBounds regardless of what ArrangePass()/ArrangePassWithFixedExtent()
/// are called with or by whom - otherwise the ordinary frame driver's own top-down walk would
/// re-arrange this control against a default slot-derived rect on the next dirty frame.
///
/// mOwner records which NewGuiPopup this group belongs to - once reparented, this group is no
/// longer a tree descendant of mOwner, so NewGuiPopup::findParentPopup() bridges that gap via
/// getOwner().
class NewGuiPopupGroup : public NewGuiStack
{
public:

   typedef NewGuiStack Parent;

   NewGuiPopupGroup();
   virtual ~NewGuiPopupGroup();

   DECLARE_CONOBJECT(NewGuiPopupGroup);

   /// Draws nothing - this control is purely structural.
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override {}

   void ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;
   void ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// Sets the rect ArrangePass()/ArrangePassWithFixedExtent() resolve to regardless of what
   /// they're called with. Called by NewGuiPopup::repositionPopupGroup().
   void setFixedBounds(const RectI& bounds) { mFixedBounds = bounds; }

   void setOwner(NewGuiPopup* owner) { mOwner = owner; }
   NewGuiPopup* getOwner() const { return mOwner; }

   /// Closes mOwner if this Down's resolved hit is neither this group's subtree nor mOwner
   /// itself. See class doc comment.
   void onOutsideHitTest(NewGuiControl* hit) override;

protected:

   NewGuiPopup* mOwner;
   RectI mFixedBounds;

   /// @return True if `control` is this group or a descendant of it.
   bool isSelfOrAncestorOf(const NewGuiControl* control) const;
};

#endif // _NEWGUIPOPUPGROUP_H_
