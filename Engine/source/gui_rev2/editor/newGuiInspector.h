//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspector.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIINSPECTOR_H_
#define _NEWGUIINSPECTOR_H_

#ifndef _NEWGUISCROLL_H_
#include "gui_rev2/controls/newGuiScroll.h"
#endif
#ifndef _SIMOBJECT_H_
#include "sim/simObject.h"
#endif

class NewGuiStack;
class NewGuiInspectorGroup;

/// Top-level inspector control: a scrollable list of NewGuiInspectorGroup sections for one
/// SimObject's fields, plus one further section per attached SimComponent.
///
/// Single-object inspection only, for now. The old GuiInspector supported inspecting several
/// objects at once (editing a shared field across a multi-selection), which needs a common-
/// ancestor AbstractClassRep walk (see GuiInspectorGroup::findCommonAncestorClass() in the old
/// code) to decide which fields are even safe to show together - deliberately not built here;
/// inspectObject() below replaces whatever was previously inspected rather than adding to a set.
///
/// Refresh is poll-driven, not push-driven: there is no generic "a field changed" broadcast on
/// SimObject to subscribe to (onStaticModified()/onDynamicModified() are plain override-only
/// virtuals - see design notes; the inspector doesn't own the target class and can't override
/// them). Instead, RenderPass() (already called every frame) walks every group and calls
/// refresh() on it, which re-reads every row's field value fresh via getDataField(). This is
/// also the structurally correct behavior for a server-authoritative object: a value only ever
/// appears in a control because it was just read from the object, never because something
/// assumed a write took effect - see newGuiInspectorFieldBinding.h's ordering contract, which
/// this refresh loop is the outermost driver of.
class NewGuiInspector : public NewGuiScroll
{
public:

   typedef NewGuiScroll Parent;

protected:

   SimObjectPtr<SimObject> mTargetObject;

   SimObjectPtr<NewGuiStack> mContentStack;   ///< Vertical stack of NewGuiInspectorGroup sections. Built fresh by rebuild() each inspectObject() call.

   Vector<SimObjectPtr<NewGuiInspectorGroup> > mGroups;   ///< Every top-level group currently shown - ordinary field groups, then one per component. Walked by RenderPass()'s refresh pass.

   /// Tears down mContentStack's children and rebuilds them from scratch against
   /// mTargetObject's current getClassRep()->getFieldList() (and, per component,
   /// getComponent(i)->getClassRep()->getFieldList()). Called once from inspectObject() - not
   /// meant to run every frame; see refresh() below for the cheap per-frame path instead.
   void rebuild();

   /// Partitions fieldList into top-level StartGroupFieldType/EndGroupFieldType spans and builds
   /// one NewGuiInspectorGroup per span (plus one implicit leading span for any fields before the
   /// first StartGroupFieldType, captioned "General" - initPersistFields() implementations in
   /// this codebase always open with addGroup() first in practice, per the SimObject.cpp example
   /// already reviewed, but this stays defensive rather than assuming that always holds).
   /// @param target Object whose field list this is.
   /// @param fieldList target->getClassRep()->getFieldList().
   void buildGroupsForFieldList(SimObject* target, const AbstractClassRep::FieldList& fieldList);

   /// Builds one GroupKind_Component NewGuiInspectorGroup per mTargetObject->getComponent(i).
   void buildComponentGroups();

public:

   NewGuiInspector();
   virtual ~NewGuiInspector();

   DECLARE_CONOBJECT(NewGuiInspector);

   void onDeleteNotify(SimObject* object) override;

   /// Recurses Parent::RenderPass() as normal, then walks every group and calls refresh() on
   /// it - see class-level note on why this is the refresh mechanism instead of a push
   /// notification. No-ops entirely if mTargetObject has gone NULL (deleted - see
   /// onDeleteNotify()).
   void RenderPass(NewGuiRenderBatch* batch, S32 parentLayer) override;

   /// Sets the object this inspector displays, tearing down and rebuilding every group against
   /// it. Passing NULL clears the inspector to empty.
   void inspectObject(SimObject* object);

   SimObject* getInspectObject() const { return mTargetObject; }

   /// Called by a top-level NewGuiInspectorGroup during its own configure() - registers it into
   /// mGroups so RenderPass()'s refresh pass reaches it directly. Nested array-element groups
   /// (GroupKind_ArrayElement) do NOT call this themselves - their owning group's own refresh()
   /// already recurses into them (see NewGuiInspectorGroup::refresh()), so registering them here
   /// too would refresh them twice per frame for no benefit. Only rebuild()'s own top-level
   /// buildGroupsForFieldList()/buildComponentGroups() calls end up registered.
   void registerGroupForRefresh(NewGuiInspectorGroup* group);
};

#endif // _NEWGUIINSPECTOR_H_
