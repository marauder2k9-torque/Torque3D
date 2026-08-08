//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspectorField.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIINSPECTORFIELD_H_
#define _NEWGUIINSPECTORFIELD_H_

#ifndef _NEWGUISTACK_H_
#include "gui_rev2/controls/newGuiStack.h"
#endif
#ifndef _CONSOLEOBJECT_H_
#include "console/consoleObject.h"
#endif
#ifndef _SIMOBJECT_H_
#include "sim/simObject.h"
#endif

#include "gui_rev2/editor/newGuiInspectorFieldBinding.h"

class NewGuiLabel;

/// One row in an inspector: a label plus whatever value control(s) the field's
/// InspectorFieldBinding builds. Replaces the old GuiInspectorFieldXXX-subclass-per-type
/// pattern with ONE concrete class - see newGuiInspectorFieldBinding.h for how a type supplies
/// its own control shape without a new C++ class.
///
/// A NewGuiInspectorField is itself a horizontal NewGuiStack: [ label | binding's control(s) ].
/// Built once by configure(), never rebuilt afterward - only refreshed.
///
/// There is NO separate value-area container - a binding's buildControls() is handed THIS ROW
/// as the parent to add into (see configure()), and its control(s) are sized to width="50%" to
/// match the label's own half. A pre-built wrapper control here would sit between the row and
/// the binding's control for every field, even the overwhelming majority that add exactly one
/// control - see design discussion: a label needs no such wrapper, and neither does a single
/// value control. A binding that genuinely needs several sub-controls side by side (color
/// channels, vector components) creates its own NewGuiStack, sizes THAT to width="50%", and
/// adds it here instead - see InspectorFieldBinding::buildControls' own doc comment.
class NewGuiInspectorField : public NewGuiStack
{
public:

   typedef NewGuiStack Parent;

protected:

   SimObjectPtr<SimObject> mTargetObject;          ///< Object this row edits a field on.
   const AbstractClassRep::Field* mField;           ///< The field this row edits. Never NULL after configure().
   S32 mArrayIndex;                                 ///< -1 for a scalar field; 0..elementCount-1 for one array slot.

   const InspectorFieldBinding* mBinding;           ///< Resolved once in configure(). Never NULL - falls back to a generic binding.

   SimObjectPtr<NewGuiLabel> mLabelControl;         ///< Built once in configure(). Always this row's FIRST child.

   /// Formats the array-index argument getDataField()/setDataField() expect ("" for a scalar
   /// field, "3" for array slot 3). Backed by a small fixed buffer - the returned pointer is
   /// only valid until the next call on this row.
   const char* arrayIndexArg() const;

   /// Returns the generic fallback binding (single NewGuiTextEdit, raw string get/set) used
   /// when no binding is registered for this field's console type. Lazily constructed once,
   /// shared by every row that falls back to it.
   static const InspectorFieldBinding& fallbackBinding();

public:

   NewGuiInspectorField();
   virtual ~NewGuiInspectorField();

   DECLARE_CONOBJECT(NewGuiInspectorField);

   /// Wires this row to one field (or one array element of a field) on target: resolves its
   /// binding via InspectorFieldBindingRegistry, builds the label and value control(s), and
   /// does an initial refresh(). Call once; not meant to be reconfigured in place - build a new
   /// row instead if the target/field changes.
   /// @param target Object being inspected. Row keeps a SimObjectPtr - if target is deleted,
   ///        isTargetValid() goes false and refresh()/onValueControlChanged() become no-ops.
   /// @param field The static field this row edits.
   /// @param arrayIndex -1 for a scalar field, or 0..field->elementCount-1 for one array slot.
   void configure(SimObject* target, const AbstractClassRep::Field* field, S32 arrayIndex = -1);

   SimObject* getTargetObject() const { return mTargetObject; }
   const AbstractClassRep::Field* getField() const { return mField; }
   S32 getArrayIndex() const { return mArrayIndex; }

   /// The parent a binding's buildControls() should add its control(s) into - this row itself.
   /// Kept as a named accessor (rather than every binding writing `row` directly) so a binding
   /// doesn't need to know or care that "the parent" and "the row" happen to be the same object;
   /// that's this class's own layout choice, not something a binding should depend on directly.
   NewGuiControl* getValueArea() { return this; }

   /// @return This row's own value control - whichever child isn't mLabelControl. The label is
   /// always added first (index 0), so a binding's own control(s) are always at index 1 - this
   /// is what a binding's refresh()/apply() lambdas should look up rather than assuming any
   /// particular stored pointer, since there IS no separate stored valueArea anymore.
   NewGuiControl* getValueControl() const { return (size() > 1) ? static_cast<NewGuiControl*>(at(1)) : NULL; }

   bool isTargetValid() const { return mTargetObject != NULL; }

   /// Reads this field's current value straight from the target object via getDataField().
   /// The one and only source of truth a binding's refresh() should read from.
   const char* readCurrentValueString() const;

   /// Submits a value change request via setDataField(). The one and only way a binding's
   /// apply() should write a value out. Does NOT touch any control - see
   /// InspectorFieldBinding's ordering contract.
   void writeValueString(const char* value);

   /// Pure push: calls mBinding->refresh(this). Never triggered by the value control(s)
   /// themselves - only by configure(), NewGuiInspector's periodic refresh pass, or immediately
   /// following onValueControlChanged() below.
   void refresh();

   /// Called by a value control after a user-driven commit (see
   /// gui_rev2/editor/README on the native-change-notify hook and why it exists only on this
   /// write path). Calls mBinding->apply(), then unconditionally refresh()es regardless of what
   /// apply() just did - the control must reflect the object's actual current state, not an
   /// optimistic guess about whether the request took effect.
   void onValueControlChanged(NewGuiControl* changedControl);

   /// Whole-field visibility: false if FIELD_HideInInspectors is set, or if
   /// field->visibilityFn(target, arrayIndexArg) returns false. Does NOT itself decide
   /// per-array-element sequential-fill gating - see NewGuiInspectorGroup's array handling for
   /// that (it calls this same visibilityFn per slot; whether slot N requires slot N-1 to be
   /// filled is a convention visibilityFn implementations follow, not logic this row imposes).
   bool isFieldVisible() const;
};

#endif // _NEWGUIINSPECTORFIELD_H_
