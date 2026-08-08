//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiPopup.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiPopup.h"
#include "gui_rev2/controls/newGuiPopupGroup.h"

IMPLEMENT_CONOBJECT(NewGuiPopup);

IMPLEMENT_CALLBACK(NewGuiPopup, onOpen, void, (), (),
   "Called right after this popup's content has been opened.");
IMPLEMENT_CALLBACK(NewGuiPopup, onClose, void, (), (),
   "Called right after this popup's content has been closed (including a self-close from an "
   "outside click).");

NewGuiPopup::NewGuiPopup()
   : mContent(NULL),
   mPopupGroup(NULL),
   mOpen(false),
   mPreferredEdge(Edge_Bottom),
   mOpenOnHover(false),
   mOpenOnHoverInMenu(false)
{
   setButtonType(ButtonType_Push);

}

NewGuiPopup::~NewGuiPopup()
{
   // mPopupGroup is never Sim-registered (same pattern as NewGuiCanvas::mDefaultTooltipBox) -
   // detach it from the tree before deleting it directly.
   if (mPopupGroup)
   {
      SimGroup* parent = mPopupGroup->getGroup();
      if (parent)
         parent->removeObject(mPopupGroup);

      if (mPopupGroup->isProperlyAdded())
         mPopupGroup->deleteObject();
   }
}

void NewGuiPopup::addObject(SimObject* object)
{
   Parent::addObject(object);

   if (mContent)
      return;

   NewGuiControl* control = dynamic_cast<NewGuiControl*>(object);
   if (control && control->getInternalName() == StringTable->insert("content"))
   {
      mContent = control;
      mContent->setVisible(false);
   }
}

// Same walk getOwningCanvas() does, just without needing canvas-specific typing.
NewGuiControl* NewGuiPopup::findTreeRoot() const
{
   const NewGuiControl* root = this;
   for (;;)
   {
      NewGuiControl* parent = dynamic_cast<NewGuiControl*>(root->getGroup());
      if (!parent)
         break;
      root = parent;
   }
   return const_cast<NewGuiControl*>(root);
}

// Moves mContent out of this control's own child list and into mPopupGroup's, once, on first
// open. Every later open/close reuses the same mPopupGroup instance.
void NewGuiPopup::ensurePopupGroup()
{
   if (mPopupGroup || !mContent)
      return;

   mPopupGroup = new NewGuiPopupGroup();
   mPopupGroup->registerObject();
   mPopupGroup->setRenderLayerOverride(kPopupLayer);
   mPopupGroup->setVisible(false);
   mPopupGroup->setOwner(this);

   removeObject(mContent);
   mPopupGroup->addObject(mContent);
}

// NewGuiStack::ComputePreferredSize() never reads its own authored width/height, only the union
// of its children's - so a Pixels-mode-authored mContent with 100%-width rows measures narrower
// than its real width (the rows have nothing to resolve 100% against yet). Falls back to the
// authored value directly whenever it's Pixels-mode; Auto/Percent still use the measured size.
static Point2I resolveContentDesignSize(const NewGuiControl* content)
{
   Point2I size = content->getPreferredSize();

   if (content->getAuthoredWidth().isPixels())
      size.x = (S32)content->getAuthoredWidth().value;

   if (content->getAuthoredHeight().isPixels())
      size.y = (S32)content->getAuthoredHeight().value;

   return size;
}

// mPopupGroup is never a canvas descendant in the ordinary sense while mid-reparent, so it
// needs its own fresh Style/Measure pass (same reasoning as NewGuiCanvas::positionTooltip())
// before placement can be computed and it can be arranged into position.
void NewGuiPopup::repositionPopupGroup()
{
   if (!mPopupGroup || !mContent)
      return;

   NewGuiResolvedStyle rootInherited;
   mPopupGroup->StylePass(rootInherited, 0);
   mPopupGroup->MeasurePass();

   const RectI placement = computePlacement(resolveContentDesignSize(mContent));

   mPopupGroup->setFixedBounds(placement);
   mPopupGroup->ArrangePassWithFixedExtent(placement, mRenderLayer, mResolvedUIScaleX, mResolvedUIScaleY);
}

// contentSize arrives in design space (MeasurePass()/ComputePreferredSize() never apply scale -
// see resolveAxis(), the only place that happens); anchorBounds/rootBounds are already
// device-space. Scaled to device space here, at the same boundary resolveAxis() would use,
// before any position math runs.
RectI NewGuiPopup::computePlacement(const Point2I& designSpaceContentSize) const
{
   const Point2I contentSize(
      S32(designSpaceContentSize.x * mResolvedUIScaleX),
      S32(designSpaceContentSize.y * mResolvedUIScaleY));

   const RectI& anchorBounds = mBounds;
   const RectI& rootBounds = findTreeRoot()->getBounds();

   const Point2I rootMin = rootBounds.point;
   const Point2I rootMax(rootBounds.point.x + rootBounds.extent.x, rootBounds.point.y + rootBounds.extent.y);

   Point2I pos;

   switch (mPreferredEdge)
   {
   case Edge_Right:
      pos.x = anchorBounds.point.x + anchorBounds.extent.x;
      pos.y = anchorBounds.point.y;
      if (pos.x + contentSize.x > rootMax.x)
         pos.x = anchorBounds.point.x - contentSize.x;   // flip to the left
      break;

   case Edge_Left:
      pos.x = anchorBounds.point.x - contentSize.x;
      pos.y = anchorBounds.point.y;
      if (pos.x < rootMin.x)
         pos.x = anchorBounds.point.x + anchorBounds.extent.x;   // flip to the right
      break;

   case Edge_Top:
      pos.x = anchorBounds.point.x;
      pos.y = anchorBounds.point.y - contentSize.y;
      if (pos.y < rootMin.y)
         pos.y = anchorBounds.point.y + anchorBounds.extent.y;   // flip below
      break;

   case Edge_Bottom:
   default:
      pos.x = anchorBounds.point.x;
      pos.y = anchorBounds.point.y + anchorBounds.extent.y;
      if (pos.y + contentSize.y > rootMax.y)
         pos.y = anchorBounds.point.y - contentSize.y;   // flip above
      break;
   }

   pos.x = mClamp(pos.x, rootMin.x, getMax(rootMin.x, rootMax.x - contentSize.x));
   pos.y = mClamp(pos.y, rootMin.y, getMax(rootMin.y, rootMax.y - contentSize.y));

   return RectI(pos, contentSize);
}

// Bridges across the mPopupGroup reparent: once an ancestor popup has opened, its content is a
// tree descendant of the root, not of the ancestor - so this asks a NewGuiPopupGroup for its
// owner instead of continuing the plain getGroup() walk through it.
NewGuiPopup* NewGuiPopup::findParentPopup() const
{
   SimObject* obj = getGroup();

   while (obj)
   {
      if (NewGuiPopup* popup = dynamic_cast<NewGuiPopup*>(obj))
         return popup;

      if (NewGuiPopupGroup* group = dynamic_cast<NewGuiPopupGroup*>(obj))
      {
         obj = group->getOwner();
         continue;
      }

      obj = obj->getGroup();
   }

   return NULL;
}

bool NewGuiPopup::_setPreferredEdge(void* obj, const char* index, const char* data)
{
   NewGuiPopup* popup = static_cast<NewGuiPopup*>(obj);

   if (dStricmp(data, "right") == 0)
      popup->mPreferredEdge = Edge_Right;
   else if (dStricmp(data, "top") == 0)
      popup->mPreferredEdge = Edge_Top;
   else if (dStricmp(data, "left") == 0)
      popup->mPreferredEdge = Edge_Left;
   else
      popup->mPreferredEdge = Edge_Bottom;

   return false;
}

void NewGuiPopup::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Popup");

   ADD_FIELD("preferredEdge", TypeString, 0)
      .onSet(_setPreferredEdge)
      .doc("Which edge of this control the popup content prefers to open against: bottom "
         "(default - a dropdown), right (a submenu flyout), top, or left. Flips to the "
         "opposite edge automatically if the preferred side doesn't fit.");

   ADD_FIELD("openOnHover", TypeBool, Offset(mOpenOnHover, NewGuiPopup))
      .doc("If true, hovering this popup while a SIBLING popup (same parent) is already open "
         "switches to this one without requiring a click - the menu-bar behavior.");

   ADD_FIELD("openOnHoverInMenu", TypeBool, Offset(mOpenOnHoverInMenu, NewGuiPopup))
      .doc("If true, hovering this popup opens it automatically (no click needed) whenever its "
         "own PARENT popup is already open - the submenu behavior (e.g. File > Export).");

   GROUP_END("Popup");
}

void NewGuiPopup::openPopup()
{
   if (!mContent || mOpen)
      return;

   ensurePopupGroup();
   if (!mPopupGroup)
      return;

   NewGuiControl* root = findTreeRoot();
   if (!root)
      return;

   SimGroup* currentParent = mPopupGroup->getGroup();
   if (currentParent && currentParent != root)
      currentParent->removeObject(mPopupGroup);

   if (mPopupGroup->getGroup() != root)
      root->addObject(mPopupGroup);   // becomes the last child - see NewGuiPopupGroup's class doc.

   mContent->setVisible(true);
   mPopupGroup->setVisible(true);

   repositionPopupGroup();

   mOpen = true;
   onOpen_callback();
}

// A nested popup's own mPopupGroup is reparented to the SAME tree root, independent of its
// ancestor - closing the ancestor has to explicitly close every open nested popup first, or
// their groups are orphaned at root level.
void NewGuiPopup::closePopup()
{
   if (!mOpen)
      return;

   if (mContent)
   {
      Vector<NewGuiPopup*> openDescendants;
      for (SimSetIterator itr(mContent); *itr; ++itr)
      {
         NewGuiPopup* descendant = dynamic_cast<NewGuiPopup*>(*itr);
         if (descendant && descendant->isOpen())
            openDescendants.push_back(descendant);
      }
      for (U32 i = 0; i < openDescendants.size(); ++i)
         openDescendants[i]->closePopup();
   }

   mOpen = false;

   if (mPopupGroup)
   {
      SimGroup* parent = mPopupGroup->getGroup();
      if (parent)
         parent->removeObject(mPopupGroup);
      mPopupGroup->setVisible(false);
   }

   if (mContent)
      mContent->setVisible(false);

   onClose_callback();
}

// Requires NewGuiButton::performClick() to be virtual (see
// newGuiButton_virtual_performClick.diff.txt).
void NewGuiPopup::performClick()
{
   Parent::performClick();

   if (mOpen)
      closePopup();
   else
      openPopup();
}

void NewGuiPopup::onMouseEnter(NewGuiInputEvent& event)
{
   Parent::onMouseEnter(event);

   if (mOpenOnHoverInMenu && !mOpen)
   {
      NewGuiPopup* parent = findParentPopup();
      if (parent && parent->isOpen())
         openPopup();
   }

   if (!mOpenOnHover)
      return;

   SimGroup* parentGroup = getGroup();
   if (!parentGroup)
      return;

   for (SimSet::iterator itr = parentGroup->begin(); itr != parentGroup->end(); ++itr)
   {
      NewGuiPopup* sibling = dynamic_cast<NewGuiPopup*>(*itr);
      if (sibling && sibling != this && sibling->isOpen())
      {
         sibling->closePopup();
         openPopup();
         return;
      }
   }
}
