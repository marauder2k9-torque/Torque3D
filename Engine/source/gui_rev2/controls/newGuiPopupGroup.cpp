//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiPopupGroup.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "gui_rev2/controls/newGuiPopupGroup.h"
#include "gui_rev2/controls/newGuiPopup.h"

IMPLEMENT_CONOBJECT(NewGuiPopupGroup);

NewGuiPopupGroup::NewGuiPopupGroup()
   : mOwner(NULL)
{
   // Never itself the resolved hit - only its content should ever be returned by
   // findHitControl().
   setHitTestable(false);
}

NewGuiPopupGroup::~NewGuiPopupGroup()
{
}

void NewGuiPopupGroup::ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   ArrangePassWithFixedExtent(mFixedBounds, parentRenderLayer, uiScaleX, uiScaleY);
}

// Forces finalBounds to mFixedBounds, then hands off to NewGuiStack's real implementation for
// everything downstream.
void NewGuiPopupGroup::ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   Parent::ArrangePassWithFixedExtent(mFixedBounds, parentRenderLayer, uiScaleX, uiScaleY);
}

bool NewGuiPopupGroup::isSelfOrAncestorOf(const NewGuiControl* control) const
{
   for (const SimObject* obj = control; obj; obj = obj->getGroup())
   {
      if (obj == this)
         return true;
   }
   return false;
}

// mOwner is checked separately from isSelfOrAncestorOf() because it's not a tree descendant of
// this group once opened (they're siblings at root level) - without this, re-clicking the
// trigger to close it would register as an outside click first, then NewGuiPopup::performClick()
// would see mOpen already false and reopen it instead.
void NewGuiPopupGroup::onOutsideHitTest(NewGuiControl* hit)
{
   if (!mOwner || !mOwner->isOpen() || !isVisible())
      return;

   const bool insideGroup = hit && isSelfOrAncestorOf(hit);
   const bool isOwnerTrigger = (hit == mOwner);

   if (!insideGroup && !isOwnerTrigger)
      mOwner->closePopup();
}
