//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspectorGroup.cpp
//-----------------------------------------------------------------------------
#include "gui_rev2/editor/newGuiInspectorGroup.h"
#include "gui_rev2/editor/newGuiInspectorField.h"
#include "gui_rev2/editor/newGuiInspector.h"
#include "gui_rev2/controls/newGuiLabel.h"
#include "gui_rev2/controls/newGuiButton.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"

IMPLEMENT_CONOBJECT(NewGuiInspectorGroup);

//-----------------------------------------------------------------------------

NewGuiInspectorGroup::NewGuiInspectorGroup()
   : mOwningInspector(NULL),
   mKind(GroupKind_Group),
   mFieldList(NULL),
   mSpanFirst(0),
   mSpanLast(0),
   mArrayIndex(-1),
   mExpanded(true)
{
   // mAxis already defaults to StackAxis_Vertical (see NewGuiStack::NewGuiStack()) - this
   // group's own top-level layout (header row, then content stack) is vertical, so no explicit
   // axis field-write is needed here.
}

//-----------------------------------------------------------------------------

NewGuiInspectorGroup::~NewGuiInspectorGroup()
{
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::buildHeader(const char* caption)
{
   // This group itself must fill its parent's width (the owning NewGuiInspector's/parent
   // group's mContentStack, now correctly 100% wide itself - see NewGuiInspector::rebuild()) -
   // "auto" (the control default) would instead size this group down to its widest child's own
   // preferred width, and that shrink-to-fit would cascade into every row beneath it the same
   // way it did before mContentStack itself was fixed.
   setDataField(StringTable->insert("width"), NULL, "100%");
   setDataField(StringTable->insert("height"), NULL, "auto");

   mHeaderRow = new NewGuiStack();
   mHeaderRow->registerObject();
   mHeaderRow->setDataField(StringTable->insert("axis"), NULL, "horizontal");
   mHeaderRow->setDataField(StringTable->insert("align"), NULL, "center");
   mHeaderRow->setDataField(StringTable->insert("width"), NULL, "100%");
   mHeaderRow->setDataField(StringTable->insert("height"), NULL, "24");
   addObject(mHeaderRow);

   mExpandButton = new NewGuiButton();
   mExpandButton->registerObject();
   mExpandButton->setButtonType(NewGuiButton::ButtonType_Toggle);
   mExpandButton->setText(mExpanded ? "-" : "+");   // Placeholder glyph - swap for a real
   // disclosure-triangle skin part once one
   // exists; not blocking on that here.
   mExpandButton->setDataField(StringTable->insert("width"), NULL, "24");
   mExpandButton->setDataField(StringTable->insert("height"), NULL, "100%");
   mHeaderRow->addObject(mExpandButton);

   // Requires the newGuiControl.h/.cpp diff noted alongside NewGuiInspectorField (see
   // gui_rev2/editor's own notes there): NewGuiControl::mNativeChangeNotify plus
   // NewGuiButton::performClick() calling notifyNativeChange() right after onClick_callback().
   // Same hook the field rows use for their write-side commit - an expand toggle is a "user
   // committed an action" event in exactly the same sense.
   mExpandButton->setNativeChangeNotify([this](NewGuiControl*)
   {
      onExpandButtonClicked();
   });

   mCaptionLabel = new NewGuiLabel();
   mCaptionLabel->registerObject();
   mCaptionLabel->setText(caption);
   mCaptionLabel->setDataField(StringTable->insert("width"), NULL, "auto");
   mCaptionLabel->setDataField(StringTable->insert("height"), NULL, "100%");
   mHeaderRow->addObject(mCaptionLabel);

   mContentStack = new NewGuiStack();
   mContentStack->registerObject();
   // mAxis already defaults to StackAxis_Vertical - no explicit field-write needed.
   mContentStack->setDataField(StringTable->insert("width"), NULL, "100%");
   mContentStack->setDataField(StringTable->insert("height"), NULL, "auto");
   mContentStack->setVisible(mExpanded);
   addObject(mContentStack);
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::onExpandButtonClicked()
{
   setExpanded(!mExpanded);
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::setExpanded(bool expanded)
{
   mExpanded = expanded;
   setContentVisible(mExpanded);
   if (mExpandButton)
      mExpandButton->setText(mExpanded ? "-" : "+");
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::setContentVisible(bool visible)
{
   // setVisible(), never destruction - same reasoning as NewGuiTreeGroup::collapseSlot(): the
   // whole subtree stays alive, off-screen, unhittable and unrendered until expanded again. A
   // collapsed group whose target field values changed elsewhere still gets refreshed (see
   // refresh() below - it does not check mExpanded), so re-expanding shows current data, not
   // stale data from before the collapse.
   if (mContentStack)
      mContentStack->setVisible(visible);
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::configure(NewGuiInspector* owningInspector, SimObject* target, const char* caption,
   const AbstractClassRep::FieldList* fieldList, U32 spanFirst, U32 spanLast,
   GroupKind kind)
{
   AssertFatal(target != NULL, "NewGuiInspectorGroup::configure - NULL target");
   AssertFatal(fieldList != NULL, "NewGuiInspectorGroup::configure - NULL fieldList");
   AssertFatal(kind != GroupKind_ArrayElement, "NewGuiInspectorGroup::configure - use configureArrayElement() for array elements");

   mOwningInspector = owningInspector;
   mTargetObject = target;
   mKind = kind;
   mFieldList = fieldList;
   mSpanFirst = spanFirst;
   mSpanLast = spanLast;
   mArrayIndex = -1;

   // Seed expand state from the StartGroupFieldType sentinel's own groupExpand (".expanded()"
   // on the FieldDescriptor builder), if there is one immediately before spanFirst. A
   // GroupKind_Component span has no such sentinel (it's a whole separate field list, not a
   // sub-span of one) - defaults to expanded.
   mExpanded = true;
   if (kind == GroupKind_Group && spanFirst > 0 && spanFirst <= fieldList->size())
   {
      const AbstractClassRep::Field& startField = (*fieldList)[spanFirst - 1];
      if (startField.type == AbstractClassRep::StartGroupFieldType)
         mExpanded = startField.groupExpand;
   }

   buildHeader(caption);
   buildFieldRows();

   // Only a top-level group (an ordinary field group or a component group) self-registers for
   // NewGuiInspector's flat refresh walk - see NewGuiInspector::registerGroupForRefresh()'s own
   // doc comment on why array-element groups deliberately don't: their owning group's refresh()
   // already recurses into them directly.
   if (mOwningInspector)
      mOwningInspector->registerGroupForRefresh(this);
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::configureArrayElement(NewGuiInspector* owningInspector, SimObject* target, const char* caption,
   const AbstractClassRep::FieldList* fieldList, U32 spanFirst, U32 spanLast,
   S32 arrayIndex)
{
   AssertFatal(target != NULL, "NewGuiInspectorGroup::configureArrayElement - NULL target");
   AssertFatal(fieldList != NULL, "NewGuiInspectorGroup::configureArrayElement - NULL fieldList");

   mOwningInspector = owningInspector;
   mTargetObject = target;
   mKind = GroupKind_ArrayElement;
   mFieldList = fieldList;
   mSpanFirst = spanFirst;
   mSpanLast = spanLast;
   mArrayIndex = arrayIndex;
   mExpanded = true;   // Array elements default open - there's no groupExpand sentinel for an
   // individual element, only for the array as a whole (StartArrayFieldType
   // itself carries no groupExpand field per the AbstractClassRep::Field
   // struct - only StartGroupFieldType's builder path sets it).

   buildHeader(caption);
   buildFieldRows();

   // Deliberately does NOT call registerGroupForRefresh() - an array-element group is reached
   // via its owning group's own refresh() recursing into entry.arrayElementGroups (see
   // NewGuiInspectorGroup::refresh()), not via NewGuiInspector's flat mGroups walk. Registering
   // here too would refresh it twice per frame for no benefit.
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::buildFieldRows()
{
   if (!mFieldList)
      return;

   for (U32 i = mSpanFirst; i < mSpanLast; ++i)
   {
      const AbstractClassRep::Field& field = (*mFieldList)[i];

      // Nested groups (StartGroupFieldType inside another group's span) are not expected in
      // practice - addGroup()/endGroup() calls are not meant to nest in initPersistFields() - so
      // this walk treats StartGroupFieldType/EndGroupFieldType as belonging to a DIFFERENT
      // top-level span entirely and skips them if encountered here. NewGuiInspector::rebuild()
      // is what partitions the field list into top-level group spans in the first place; this
      // loop only ever sees the fields (and possibly array sentinels) INSIDE one such span.
      if (field.type == AbstractClassRep::StartGroupFieldType ||
         field.type == AbstractClassRep::EndGroupFieldType)
         continue;

      if (field.type == AbstractClassRep::StartArrayFieldType)
      {
         // Find the matching EndArrayFieldType (same pGroupname, per addArray()/endArray()'s own
         // convention - see consoleObject.cpp).
         U32 arrayEndIndex = i + 1;
         for (; arrayEndIndex < mSpanLast; ++arrayEndIndex)
         {
            if ((*mFieldList)[arrayEndIndex].type == AbstractClassRep::EndArrayFieldType &&
               (*mFieldList)[arrayEndIndex].pGroupname == field.pGroupname)
               break;
         }
         AssertFatal(arrayEndIndex < mSpanLast, "NewGuiInspectorGroup::buildFieldRows - unterminated array span");

         FieldRowEntry entry;
         entry.field = &field;
         entry.row = NULL;

         char captionBuf[128];
         for (S32 elem = 0; elem < field.elementCount; ++elem)
         {
            dSprintf(captionBuf, sizeof(captionBuf), "%s[%d]", field.pGroupname, elem);

            NewGuiInspectorGroup* elementGroup = new NewGuiInspectorGroup();
            elementGroup->registerObject();
            mContentStack->addObject(elementGroup);
            elementGroup->configureArrayElement(mOwningInspector, mTargetObject, captionBuf,
               mFieldList, i + 1, arrayEndIndex, elem);

            entry.arrayElementGroups.push_back(elementGroup);
         }

         mFieldRows.push_back(entry);
         updateFieldRowVisibility(mFieldRows.last());

         i = arrayEndIndex;   // Skip past EndArrayFieldType - loop's ++i lands one past it.
         continue;
      }

      // Fields explicitly marked for component-style display only (FIELD_ComponentInspectors)
      // are skipped in an ordinary group span - they're meant to appear via a
      // GroupKind_Component group instead, mirroring SimObject::writeFields()'s own treatment
      // of the same flag.
      if (mKind != GroupKind_Component &&
         field.flag.test(AbstractClassRep::FieldFlags::FIELD_ComponentInspectors))
         continue;

      FieldRowEntry entry;
      entry.field = &field;

      NewGuiInspectorField* row = new NewGuiInspectorField();
      row->registerObject();
      mContentStack->addObject(row);
      row->configure(mTargetObject, &field, -1);

      entry.row = row;
      mFieldRows.push_back(entry);
      updateFieldRowVisibility(mFieldRows.last());
   }
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::updateFieldRowVisibility(FieldRowEntry& entry)
{
   if (entry.row)
   {
      entry.row->setVisible(entry.row->isFieldVisible());
      return;
   }

   // Array: apply visibilityFn per element, then enforce the sequential-fill convention -
   // element N is hidden if element N-1 is hidden, regardless of what visibilityFn(target, "N")
   // itself says for N. This is the inspector-imposed policy from the design discussion ("you
   // don't want to be able to set number 10 in the array if 1-9 is not filled in") rather than
   // something every visibilityFn implementation has to reproduce individually.
   bool previousVisible = true;
   for (U32 elem = 0; elem < entry.arrayElementGroups.size(); ++elem)
   {
      NewGuiInspectorGroup* elementGroup = entry.arrayElementGroups[elem];
      if (!elementGroup)
         continue;

      bool visibilityFnResult = true;
      if (entry.field->visibilityFn && mTargetObject)
      {
         char idxBuf[12];
         dSprintf(idxBuf, sizeof(idxBuf), "%d", elem);
         visibilityFnResult = entry.field->visibilityFn(mTargetObject, idxBuf);
      }

      bool visible = previousVisible && visibilityFnResult;
      elementGroup->setVisible(visible);
      previousVisible = visible;
   }
}

//-----------------------------------------------------------------------------

void NewGuiInspectorGroup::refresh()
{
   if (!mTargetObject)
      return;

   for (U32 i = 0; i < mFieldRows.size(); ++i)
   {
      FieldRowEntry& entry = mFieldRows[i];

      if (entry.row)
      {
         entry.row->refresh();
      }
      else
      {
         for (U32 elem = 0; elem < entry.arrayElementGroups.size(); ++elem)
         {
            if (entry.arrayElementGroups[elem])
               entry.arrayElementGroups[elem]->refresh();
         }
      }

      updateFieldRowVisibility(entry);
   }
}
