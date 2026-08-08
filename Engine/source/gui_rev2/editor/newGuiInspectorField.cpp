//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspectorField.cpp
//-----------------------------------------------------------------------------
#include "gui_rev2/editor/newGuiInspectorField.h"
#include "gui_rev2/controls/newGuiLabel.h"
#include "gui_rev2/controls/newGuiTextEdit.h"
#include "gui_rev2/core/newGuiStyle.h"
#include "console/dynamicTypes.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "sim/sim.h"

IMPLEMENT_CONOBJECT(NewGuiInspectorField);

//-----------------------------------------------------------------------------

NewGuiInspectorField::NewGuiInspectorField()
   : mField(NULL),
   mArrayIndex(-1),
   mBinding(NULL)
{
   // Axis is set to horizontal in configure(), after registerObject() - setDataField() here in
   // the constructor would run before this object is a properly registered SimObject (findField()
   // et al need a resolvable class rep and object state that isn't guaranteed mid-construction),
   // so it's deferred to configure() like every other field-list-driven setup this class does.
}

//-----------------------------------------------------------------------------

NewGuiInspectorField::~NewGuiInspectorField()
{
}

//-----------------------------------------------------------------------------

const char* NewGuiInspectorField::arrayIndexArg() const
{
   static char sBuf[12];
   if (mArrayIndex < 0)
      return NULL;

   dSprintf(sBuf, sizeof(sBuf), "%d", mArrayIndex);
   return sBuf;
}

//-----------------------------------------------------------------------------

const InspectorFieldBinding& NewGuiInspectorField::fallbackBinding()
{
   // Generic single-text-edit binding: works for any type as a raw string round-trip, even one
   // nobody has registered a dedicated InspectorFieldBinding for yet. Constructed once and
   // shared - none of its closures capture anything per-instance, everything they need comes
   // through the 'row' argument.
   static InspectorFieldBinding sBinding = []() -> InspectorFieldBinding
   {
      InspectorFieldBinding b;

      b.buildControls = [](NewGuiInspectorField* row, NewGuiControl* valueArea)
      {
         NewGuiTextEdit* edit = new NewGuiTextEdit();
         edit->registerObject();

         // valueArea IS the row itself now (see NewGuiInspectorField::getValueArea()) - this
         // edit is the row's second child, alongside mLabelControl's 50%, so it takes the other
         // 50% directly. Without an explicit width, the control defaults to width="auto" and
         // sizes itself to its current TEXT CONTENT, which is why an empty or short-valued
         // field showed up as a sliver too small to click.
         edit->setDataField(StringTable->insert("width"), NULL, "50%");
         edit->setDataField(StringTable->insert("height"), NULL, "100%");

         // Without its own style, a text edit inherits the inspector panel's own
         // background/border via normal style cascade (see NewGuiControl::StylePass()) - which
         // made it visually indistinguishable from the panel behind it. Look for a well-known
         // style name so script/skin authors control the actual look; falls back to whatever
         // cascades in if it isn't authored, rather than hardcoding colors here.
         NewGuiStyle* fieldStyle = dynamic_cast<NewGuiStyle*>(Sim::findObject("NewGuiInspectorFieldStyle"));
         if (fieldStyle)
            edit->setStyleAsset(fieldStyle);

         valueArea->addObject(edit);

         edit->setNativeChangeNotify([row](NewGuiControl* changed)
         {
            row->onValueControlChanged(changed);
         });
      };

      b.refresh = [](NewGuiInspectorField* row)
      {
         NewGuiTextEdit* edit = dynamic_cast<NewGuiTextEdit*>(row->getValueControl());
         if (edit)
            edit->setText(row->readCurrentValueString());
      };

      b.apply = [](NewGuiInspectorField* row, NewGuiControl* changedControl)
      {
         NewGuiTextEdit* edit = dynamic_cast<NewGuiTextEdit*>(changedControl);
         if (edit)
            row->writeValueString(edit->getText());
      };

      return b;
   }();

   return sBinding;
}

//-----------------------------------------------------------------------------

void NewGuiInspectorField::configure(SimObject* target, const AbstractClassRep::Field* field, S32 arrayIndex)
{
   AssertFatal(target != NULL, "NewGuiInspectorField::configure - NULL target");
   AssertFatal(field != NULL, "NewGuiInspectorField::configure - NULL field");

   mTargetObject = target;
   mField = field;
   mArrayIndex = arrayIndex;

   // This row itself lays out horizontally: [label | valueArea], each taking half the row's
   // width, vertically centered against each other (label and text-edit controls do NOT share
   // the same preferred height, so StackAlign_Start - the default - left them visibly offset;
   // see design notes). Row itself is 100% of whatever contains it (its owning group's
   // mContentStack, ultimately the inspector's own width) and a fixed pixel height, rather than
   // "auto" sized from children whose own heights can differ per binding.
   //
   // Set here rather than in the constructor - registerObject() has already run by the time
   // configure() is called (see NewGuiInspectorGroup::buildFieldRows()), so this is a properly
   // registered SimObject and setDataField()'s findField()/notify machinery has something valid
   // to work against.
   setDataField(StringTable->insert("axis"), NULL, "horizontal");
   setDataField(StringTable->insert("align"), NULL, "center");
   setDataField(StringTable->insert("width"), NULL, "100%");
   setDataField(StringTable->insert("height"), NULL, "24");

   ConsoleBaseType* cbt = ConsoleBaseType::getType(field->type);
   const InspectorFieldBinding* binding = cbt ? InspectorFieldBindingRegistry::find(cbt->getTypeID()) : NULL;
   mBinding = binding ? binding : &fallbackBinding();

   // Label - left half of the row.
   mLabelControl = new NewGuiLabel();
   mLabelControl->registerObject();
   mLabelControl->setText(field->pFieldname);
   mLabelControl->setDataField(StringTable->insert("width"), NULL, "50%");
   mLabelControl->setDataField(StringTable->insert("height"), NULL, "100%");
   addObject(mLabelControl);

   // Right half of the row is handed to the binding as-is - THIS ROW is the parent
   // buildControls() adds into, not a separate pre-built container. A binding that only needs
   // one control (the common case: a text edit, a checkbox) sizes that one control to
   // width="50%" (matching the label's own half) or "auto" (for a fixed-size control like a
   // checkbox that shouldn't stretch) and adds it directly here, with nothing in between. A
   // binding that genuinely needs several sub-controls side by side (color channels, vector
   // components) creates its own NewGuiStack, sizes IT to width="50%", and adds it here instead
   // - see InspectorFieldBinding::buildControls' own doc comment. Either way, this row has
   // exactly two children: mLabelControl and whatever the binding added - never a container
   // this class built and the binding didn't actually need.
   if (mBinding->buildControls)
      mBinding->buildControls(this, getValueArea());

   refresh();
}

//-----------------------------------------------------------------------------

const char* NewGuiInspectorField::readCurrentValueString() const
{
   if (!isTargetValid() || !mField)
      return "";

   return mTargetObject->getDataField(mField->pFieldname, arrayIndexArg());
}

//-----------------------------------------------------------------------------

void NewGuiInspectorField::writeValueString(const char* value)
{
   if (!isTargetValid() || !mField)
      return;

   // Submits the request only. Does NOT touch mLabelControl or the value control in any way -
   // the control only ever reflects a value via refresh(), called separately (and always) by
   // onValueControlChanged() right after this returns. See InspectorFieldBinding's ordering
   // contract for why: in a server-authoritative object this write may be rejected, clamped, or
   // simply not confirmed yet, and the control must not show a value the object doesn't
   // actually hold.
   mTargetObject->setDataField(mField->pFieldname, arrayIndexArg(), value);
}

//-----------------------------------------------------------------------------

void NewGuiInspectorField::refresh()
{
   if (!isTargetValid() || !mBinding || !mBinding->refresh)
      return;

   mBinding->refresh(this);
}

//-----------------------------------------------------------------------------

void NewGuiInspectorField::onValueControlChanged(NewGuiControl* changedControl)
{
   if (!isTargetValid() || !mBinding)
      return;

   if (mBinding->apply)
      mBinding->apply(this, changedControl);

   // Always re-read and re-push, regardless of what apply() just did or whether the request
   // has round-tripped through anything authoritative yet. See InspectorFieldBinding's ordering
   // contract - this is not an optimization, it's the correctness requirement.
   refresh();
}

//-----------------------------------------------------------------------------

bool NewGuiInspectorField::isFieldVisible() const
{
   if (!mField)
      return false;

   if (mField->flag.test(AbstractClassRep::FieldFlags::FIELD_HideInInspectors))
      return false;

   if (mField->visibilityFn && isTargetValid())
      return mField->visibilityFn(mTargetObject, arrayIndexArg());

   return true;
}
