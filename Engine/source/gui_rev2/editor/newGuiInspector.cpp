//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspector.cpp
//-----------------------------------------------------------------------------
#include "gui_rev2/editor/newGuiInspector.h"
#include "gui_rev2/editor/newGuiInspectorGroup.h"
#include "gui_rev2/controls/newGuiStack.h"
#include "console/consoleObject.h"
#include "console/engineAPI.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"

IMPLEMENT_CONOBJECT(NewGuiInspector);

//-----------------------------------------------------------------------------

NewGuiInspector::NewGuiInspector()
{
}

//-----------------------------------------------------------------------------

NewGuiInspector::~NewGuiInspector()
{
}

//-----------------------------------------------------------------------------

void NewGuiInspector::onDeleteNotify(SimObject* object)
{
   Parent::onDeleteNotify(object);

   if (object == mTargetObject)
      inspectObject(NULL);
}

//-----------------------------------------------------------------------------

void NewGuiInspector::inspectObject(SimObject* object)
{
   if (mTargetObject)
      mTargetObject->removeNotify(this, SimObject::Notify::DeleteNotify);   // Un-registers the deleteNotify() below, for the object we're leaving. NOT clearNotify() - that registers a DIFFERENT notification type (fires when mTargetObject itself is cleared, not what we want here) rather than un-registering anything.

   mTargetObject = object;

   if (mTargetObject)
      mTargetObject->deleteNotify(this);   // So onDeleteNotify() above fires if it's deleted out from under us.

   rebuild();
}

//-----------------------------------------------------------------------------

void NewGuiInspector::rebuild()
{
   // Tear down every existing group. removeObject() + deleteObject(), never a bare `delete` -
   // same convention NewGuiTreeGroup::syncChildren() follows for the same reason (SimObject
   // teardown needs to run, not just C++ destruction).
   if (mContentStack)
   {
      removeObject(mContentStack);
      mContentStack->deleteObject();
      mContentStack = NULL;
   }
   mGroups.clear();

   mContentStack = new NewGuiStack();
   mContentStack->registerObject();
   // mAxis already defaults to StackAxis_Vertical - no explicit field-write needed. Width IS
   // explicit: "auto" (the control default) would size this stack to its widest child's own
   // preferred width instead of filling the scroll viewport - every row/group beneath it is
   // authored as a Percent of ITS parent, so this has to actually be 100% for that chain to
   // resolve against anything meaningful.
   mContentStack->setDataField(StringTable->insert("width"), NULL, "100%");
   mContentStack->setDataField(StringTable->insert("height"), NULL, "auto");
   addObject(mContentStack);

   if (!mTargetObject)
      return;

   // getFieldList() is a ConsoleObject method (see consoleObject.h) - it internally forwards to
   // getClassRep()->mFieldList, but is called on the OBJECT, not fetched via a separately-held
   // AbstractClassRep* first. AbstractClassRep itself has no getFieldList() member - only the
   // raw mFieldList data member (protected) plus this accessor on ConsoleObject/SimObject.
   buildGroupsForFieldList(mTargetObject, mTargetObject->getFieldList());
   buildComponentGroups();
}

//-----------------------------------------------------------------------------

void NewGuiInspector::buildGroupsForFieldList(SimObject* target, const AbstractClassRep::FieldList& fieldList)
{
   U32 i = 0;
   U32 count = fieldList.size();

   // Fields before the first StartGroupFieldType (if any) get an implicit "General" group -
   // defensive rather than assumed-never-happens; every initPersistFields() reviewed so far
   // (SimObject::initPersistFields()) opens with addGroup() immediately, but nothing enforces
   // that as a rule.
   U32 leadingSpanEnd = i;
   while (leadingSpanEnd < count && fieldList[leadingSpanEnd].type != AbstractClassRep::StartGroupFieldType)
      ++leadingSpanEnd;

   if (leadingSpanEnd > i)
   {
      NewGuiInspectorGroup* group = new NewGuiInspectorGroup();
      group->registerObject();
      mContentStack->addObject(group);
      group->configure(this, target, "General", &fieldList, i, leadingSpanEnd, NewGuiInspectorGroup::GroupKind_Group);
      // configure() self-registers via registerGroupForRefresh() - no direct push_back needed.
   }

   i = leadingSpanEnd;

   while (i < count)
   {
      if (fieldList[i].type != AbstractClassRep::StartGroupFieldType)
      {
         // Anything here that isn't a StartGroupFieldType at this point is a field with no
         // enclosing addGroup() (shouldn't happen given the leading-span handling above, but
         // stay defensive rather than infinite-loop or misindex on unexpected input) - skip
         // forward one field rather than assert-crash an inspector over a field list quirk in
         // some class we don't control.
         ++i;
         continue;
      }

      const char* groupCaption = fieldList[i].pGroupname;
      U32 spanFirst = i + 1;
      U32 spanLast = spanFirst;

      while (spanLast < count &&
         !(fieldList[spanLast].type == AbstractClassRep::EndGroupFieldType &&
            fieldList[spanLast].pGroupname == fieldList[i].pGroupname))
         ++spanLast;

      AssertFatal(spanLast < count, "NewGuiInspector::buildGroupsForFieldList - unterminated group span");

      NewGuiInspectorGroup* group = new NewGuiInspectorGroup();
      group->registerObject();
      mContentStack->addObject(group);
      group->configure(this, target, groupCaption, &fieldList, spanFirst, spanLast, NewGuiInspectorGroup::GroupKind_Group);
      // configure() self-registers via registerGroupForRefresh() - no direct push_back needed.

      i = spanLast + 1;   // Past the EndGroupFieldType sentinel.
   }
}

//-----------------------------------------------------------------------------

void NewGuiInspector::buildComponentGroups()
{
   if (!mTargetObject)
      return;

   U32 componentCount = mTargetObject->getComponentCount();
   for (U32 i = 0; i < componentCount; ++i)
   {
      SimComponent* component = mTargetObject->getComponent(i);
      if (!component)
         continue;

      NewGuiInspectorGroup* group = new NewGuiInspectorGroup();
      group->registerObject();
      mContentStack->addObject(group);

      // A component's own field list is entirely its own, not a sub-span of the owning object's
      // list - see SimComponent::initPersistFields() (static, per-class, same as any other
      // ConsoleObject). Whole list, not a StartGroupFieldType span, hence spanFirst=0/
      // spanLast=size() and GroupKind_Component rather than GroupKind_Group. getFieldList() is
      // called on component itself (a ConsoleObject method), not on a separately-fetched
      // AbstractClassRep* - see rebuild()'s own comment on why.
      const AbstractClassRep::FieldList& componentFields = component->getFieldList();
      group->configure(this, component, component->getComponentDisplayName(),
         &componentFields, 0, componentFields.size(),
         NewGuiInspectorGroup::GroupKind_Component);
      // configure() self-registers via registerGroupForRefresh() - no direct push_back needed.
   }
}

//-----------------------------------------------------------------------------

void NewGuiInspector::registerGroupForRefresh(NewGuiInspectorGroup* group)
{
   // Every top-level group (buildGroupsForFieldList()/buildComponentGroups()) reaches mGroups
   // solely through this call, from within its own configure(). The duplicate-guard below is
   // defensive against a future call-site mistake (e.g. someone re-adding a direct push_back
   // alongside this), not something the current code paths actually trigger.
   for (U32 i = 0; i < mGroups.size(); ++i)
      if (mGroups[i] == group)
         return;

   mGroups.push_back(group);
}

//-----------------------------------------------------------------------------

void NewGuiInspector::RenderPass(NewGuiRenderBatch* batch, S32 parentLayer)
{
   Parent::RenderPass(batch, parentLayer);

   if (!mTargetObject)
      return;

   for (U32 i = 0; i < mGroups.size(); ++i)
   {
      if (mGroups[i])
         mGroups[i]->refresh();
   }
}

//=============================================================================
//    Console Methods.
//=============================================================================
// Authored fields (initPersistFields()) cover properties set at object-creation time; ordinary
// C++ methods still need their own DefineEngineMethod binding to be callable from a .tscript
// file - same split NewGuiTree::setRoot() etc. follow. Every method below is a thin call-through
// to the already-implemented C++ method of the same name - no new logic lives here.

DefineEngineMethod(NewGuiInspector, inspectObject, void, (SimObject* target), ,
   "Sets the object this inspector displays, tearing down and rebuilding every group against "
   "it. Pass 0 to clear the inspector to empty.\n"
   "@ingroup GuiCore")
{
   object->inspectObject(target);
}

DefineEngineMethod(NewGuiInspector, getInspectObject, SimObject*, (), ,
   "@return The object currently being inspected, or 0 if none.\n"
   "@ingroup GuiCore")
{
   return object->getInspectObject();
}
