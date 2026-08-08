//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspectorGroup.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIINSPECTORGROUP_H_
#define _NEWGUIINSPECTORGROUP_H_

#ifndef _NEWGUISTACK_H_
#include "gui_rev2/controls/newGuiStack.h"
#endif
#ifndef _CONSOLEOBJECT_H_
#include "console/consoleObject.h"
#endif
#ifndef _SIMOBJECT_H_
#include "sim/simObject.h"
#endif

class NewGuiLabel;
class NewGuiButton;
class NewGuiInspectorField;
class NewGuiInspector;

/// One collapsible group in an inspector: a header row (name + expand arrow) plus a vertical
/// stack of NewGuiInspectorField rows for every field between a matching StartGroupFieldType/
/// EndGroupFieldType pair in the target's flat AbstractClassRep::FieldList - see
/// NewGuiInspector::rebuild() for how that pair is located and handed to configure() as a
/// [firstFieldIndex, lastFieldIndex) span.
///
/// Also used, with a different configure() overload, for ONE ARRAY ELEMENT'S fields (the span
/// between a StartArrayFieldType/EndArrayFieldType pair, for one index) and for ONE COMPONENT's
/// fields (a SimComponent's own getFieldList(), same span-walk, different source object) - see
/// GroupKind below. All three are "a named, collapsible span of fields on some object", which is
/// exactly what this class already does; a separate class per kind would just re-fork the same
/// slot-sync logic NewGuiTreeGroup already proved out (see that class's own header for the
/// reasoning this follows: reuse existing slots on resync, never destroy a slot on a plain
/// collapse, only on the field/slot actually going away).
class NewGuiInspectorGroup : public NewGuiStack
{
public:

   typedef NewGuiStack Parent;

   enum GroupKind : U8
   {
      GroupKind_Group,        ///< A StartGroupFieldType/EndGroupFieldType span - an ordinary named group.
      GroupKind_ArrayElement, ///< One index of a StartArrayFieldType/EndArrayFieldType span.
      GroupKind_Component,    ///< A SimComponent's own field list, headed by getComponentDisplayName().
   };

protected:

   NewGuiInspector* mOwningInspector;
   GroupKind mKind;

   SimObjectPtr<SimObject> mTargetObject;   ///< Object whose fields this group displays. For
                                             ///< GroupKind_Component, this is the SimComponent
                                             ///< itself, NOT the owning SimObject - its field
                                             ///< list is entirely its own (see SimComponent's
                                             ///< own initPersistFields()).

   const AbstractClassRep::FieldList* mFieldList;   ///< mTargetObject->getClassRep()->getFieldList(), cached at configure() time.
   U32 mSpanFirst;   ///< Index of the first field belonging to this group (exclusive of the StartGroup/StartArray sentinel itself).
   U32 mSpanLast;    ///< Index one past the last field belonging to this group (exclusive of the EndGroup/EndArray sentinel).

   S32 mArrayIndex;  ///< Valid only for GroupKind_ArrayElement - which element of the array this instance displays.

   bool mExpanded;   ///< Current expand state. Seeded from the StartGroupFieldType field's own
                      ///< groupExpand at first configure() (see Field::groupExpand's doc comment
                      ///< on AbstractClassRep - ".expanded()" on the builder), user-toggleable afterward.

   SimObjectPtr<NewGuiStack> mHeaderRow;       ///< [expand arrow | caption], built once.
   SimObjectPtr<NewGuiButton> mExpandButton;
   SimObjectPtr<NewGuiLabel> mCaptionLabel;
   SimObjectPtr<NewGuiStack> mContentStack;    ///< Vertical stack of field rows (and nested array-element groups). Hidden, not destroyed, while collapsed.

   /// One entry per field slot currently represented under mContentStack - either a
   /// NewGuiInspectorField (an ordinary field, or one array element's worth of them wrapped in
   /// their own nested GroupKind_ArrayElement group) or, for a StartArrayFieldType field
   /// encountered mid-span, a Vector of per-index nested groups. Mirrors NewGuiTreeGroup's
   /// ChildSlot in spirit: built once from the field span (fields don't come and go at runtime,
   /// unlike a tree's dynamic item list), then only ever refreshed or shown/hidden afterward -
   /// never rebuilt.
   struct FieldRowEntry
   {
      const AbstractClassRep::Field* field;   ///< The plain (non-sentinel) field this entry is for.
      SimObjectPtr<NewGuiInspectorField> row;   ///< NULL for a StartArrayFieldType entry - see arrayElementGroups below.
      Vector<SimObjectPtr<NewGuiInspectorGroup> > arrayElementGroups;   ///< One GroupKind_ArrayElement child per element index, only populated when field is a StartArrayFieldType field.
   };
   Vector<FieldRowEntry> mFieldRows;

   /// Builds mHeaderRow/mExpandButton/mCaptionLabel once. Called from both configure()
   /// overloads.
   void buildHeader(const char* caption);

   /// Walks [mSpanFirst, mSpanLast) once, building one FieldRowEntry per plain field and one
   /// nested GroupKind_ArrayElement group per array element per StartArrayFieldType field
   /// encountered. Called once, from configure().
   void buildFieldRows();

   /// Click handler for mExpandButton.
   void onExpandButtonClicked();

   void setContentVisible(bool visible);

public:

   NewGuiInspectorGroup();
   virtual ~NewGuiInspectorGroup();

   DECLARE_CONOBJECT(NewGuiInspectorGroup);

   /// Configures this as an ordinary GroupKind_Group or GroupKind_Component span.
   /// @param owningInspector The NewGuiInspector this group ultimately belongs to (rows need
   ///        this to reach the target object correctly is not required - rows read straight off
   ///        mTargetObject - but the group itself calls back into it for e.g. registering for
   ///        periodic refresh; see NewGuiInspector::registerGroupForRefresh()).
   /// @param target Object whose field list this group displays.
   /// @param caption Header text. For GroupKind_Component, pass target's own
   ///        getComponentDisplayName() (SimComponent-specific - see that method's own doc
   ///        comment on AbstractClassRep... actually SimComponent.h - it already documents
   ///        itself as "used for the inspector section header").
   /// @param fieldList target->getClassRep()->getFieldList().
   /// @param spanFirst Index of the first field in this group (just after the StartGroupFieldType
   ///        sentinel, or 0 for GroupKind_Component's whole list).
   /// @param spanLast Index one past the last field in this group (the EndGroupFieldType
   ///        sentinel's own index, or fieldList.size() for GroupKind_Component).
   /// @param kind GroupKind_Group or GroupKind_Component.
   void configure(NewGuiInspector* owningInspector, SimObject* target, const char* caption,
                   const AbstractClassRep::FieldList* fieldList, U32 spanFirst, U32 spanLast,
                   GroupKind kind = GroupKind_Group);

   /// Configures this as one array element (GroupKind_ArrayElement) - same target/fieldList/span
   /// shape as above but scoped to a StartArrayFieldType/EndArrayFieldType pair, for one index.
   /// Called by buildFieldRows() when it encounters a StartArrayFieldType field, not by
   /// NewGuiInspector directly.
   void configureArrayElement(NewGuiInspector* owningInspector, SimObject* target, const char* caption,
                               const AbstractClassRep::FieldList* fieldList, U32 spanFirst, U32 spanLast,
                               S32 arrayIndex);

   GroupKind getKind() const { return mKind; }
   SimObject* getTargetObject() const { return mTargetObject; }
   bool isExpanded() const { return mExpanded; }
   void setExpanded(bool expanded);

   /// Re-reads every row's value (NewGuiInspectorField::refresh()) and recomputes per-field and
   /// per-array-element visibility (Field::visibilityFn - see updateFieldVisibility()). Called
   /// by NewGuiInspector's periodic refresh pass; never triggered by any row itself - see
   /// newGuiInspectorFieldBinding.h's ordering contract.
   void refresh();

protected:

   /// Applies isFieldVisible()/visibilityFn-driven show/hide to one FieldRowEntry - for a plain
   /// field, hides its row; for a StartArrayFieldType field, hides/shows each element's nested
   /// group individually AND enforces the sequential-fill convention (see class-level note
   /// below) by additionally hiding element N if element N-1 is not visible, regardless of what
   /// visibilityFn(target, "N") itself returns for element N. This is imposed here, generically,
   /// rather than left to every visibilityFn implementation to reproduce - see design
   /// conversation: "how arrays are displayed... comes down to certain logic," resolved as
   /// inspector-side policy rather than a per-field convention.
   void updateFieldRowVisibility(FieldRowEntry& entry);
};

#endif // _NEWGUIINSPECTORGROUP_H_
