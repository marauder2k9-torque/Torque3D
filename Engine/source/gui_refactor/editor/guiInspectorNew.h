//-----------------------------------------------------------------------------
// guiInspectorNew.h
//
// GuiInspectorNew -- inspects a SimObject and displays its persistent fields
// as an editable, scrollable list: one row per field, each row a
// GuiLabelCtrlNew (the field's name) beside a value control (a
// GuiTextEditCtrlNew for most types, a GuiButtonCtrlNew in "check" mode for
// TypeBool fields -- see GuiInspectorBoolFieldNew). Replaces the old
// inspector, which was a recurring source of bugs; this version is
// deliberately small and built entirely on the new control set
// (GuiScrollCtrlNew/GuiLabelCtrlNew/GuiTextEditCtrlNew/GuiButtonCtrlNew)
// rather than any bespoke rendering of its own.
//
// ARCHITECTURE
//   GuiInspectorNew          : GuiScrollCtrlNew -- the panel itself. Owns
//                               the inspected object (weakly, via
//                               SimObjectPtr) and one row per visible field
//                               (either a GuiInspectorFieldNew or a
//                               GuiInspectorBoolFieldNew/GuiInspectorGroup
//                               HeaderNew -- see _rebuildFields()), stacked
//                               top-to-bottom, scrollable via the inherited
//                               GuiScrollCtrlNew behavior.
//   GuiInspectorGroupHeaderNew : GuiControlNew -- a non-editing divider row
//                               showing a group's name (from
//                               AbstractClassRep::Field::pGroupname on a
//                               StartGroupFieldType marker), so the panel's
//                               field order visibly matches the class's own
//                               addGroup()/endGroup() structure instead of
//                               presenting one flat list.
//   GuiInspectorFieldNew     : GuiControlNew -- one editable row: a
//                               GuiLabelCtrlNew (name) beside a value
//                               control, laid out side by side.
//                               refresh()/apply() push the field's current
//                               value into/out of the value control; WHICH
//                               control that is, and how it's refreshed/
//                               applied, are the three protected virtuals
//                               _createValueControl()/_refreshValueControl()/
//                               _applyValueControl() -- this class's own
//                               implementations build/drive a
//                               GuiInspectorTextEditNew, which is the right
//                               default for most console types (string,
//                               numeric, enum-as-string, etc.).
//   GuiInspectorBoolFieldNew : GuiInspectorFieldNew -- overrides just those
//                               three virtuals to use a GuiInspectorCheck
//                               ButtonNew (see below) instead of a text
//                               field, for TypeBool fields specifically
//                               (see _rebuildFields()'s type check). No
//                               dropdown control exists yet for enum
//                               fields, so those still fall through to the
//                               default text-edit row -- a reasonable
//                               follow-up, not attempted here.
//   GuiInspectorCheckButtonNew : GuiButtonCtrlNew -- a check-mode button
//                               that commits on click, via the same
//                               direct-virtual-override-to-owning-row
//                               pattern as GuiInspectorTextEditNew (NOT a
//                               console "command" string -- an earlier
//                               version of this row used a
//                               "<row id>.apply();" command bound to the
//                               button's click, which turned out not to
//                               reliably reach the row; overriding
//                               onAction() directly, the same proven
//                               mechanism the text row already uses,
//                               removed that indirection entirely).
//   GuiInspectorTextEditNew  : GuiTextEditCtrlNew -- a GuiTextEditCtrlNew
//                               that commits on Enter (onAction(), inherited
//                               behavior already fires this) AND on losing
//                               focus (onLoseFirstResponder(), which the
//                               base class does NOT treat as a commit point)
//                               by calling back to its owning
//                               GuiInspectorFieldNew.
//
// FIELD VALUES go through SimObject::getDataField()/setDataField() --
// the general string-based accessor SimObject already exposes for exactly
// this purpose (see simObject.h) -- rather than reaching into
// AbstractClassRep::Field's setDataFn/getDataFn directly. This is still
// true for GuiInspectorBoolFieldNew: TypeBool's string form is just "1"/"0"
// (see consoleTypes.cpp's ConsoleGetType(TypeBool)/ConsoleSetType(TypeBool)),
// so the checkbox row is still driven purely through getDataField()/
// setDataField(), just interpreted as a bool at the row level instead of
// being handed to a text field verbatim.
//
// WHAT'S SHOWN: only real, visible fields -- static fields from the class
// hierarchy's field list (AbstractClassRep::FieldList), skipping array
// markers, deprecated fields, and anything flagged
// AbstractClassRep::FIELD_HideInInspectors. Group markers (StartGroupField
// Type/EndGroupFieldType) are NOT skipped anymore -- StartGroupFieldType
// produces a GuiInspectorGroupHeaderNew row (see above); EndGroupFieldType
// carries no displayable info of its own and is simply not turned into a
// row. Dynamic (script-added) fields are intentionally not enumerated
// here: SimObject has no general "list of dynamic field names" API to
// walk (only a dictionary keyed by name that's already known), so
// surfacing them would need a different mechanism; a reasonable follow-up,
// not attempted here either.
//-----------------------------------------------------------------------------

#ifndef _GUIINSPECTORNEW_H_
#define _GUIINSPECTORNEW_H_

#ifndef _GUISCROLLCTRLNEW_H_
#include "gui_refactor/controls/containers/guiScrollCtrlNew.h"
#endif
#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _GUITEXTEDITCTRLNEW_H_
#include "gui_refactor/controls/text/guiTextEditCtrlNew.h"
#endif
#ifndef _GUILABELCTRLNEW_H_
#include "gui_refactor/controls/text/guiLabelCtrlNew.h"
#endif
#ifndef _GUIBUTTONCTRLNEW_H_
#include "gui_refactor/controls/buttons/guiButtonCtrlNew.h"
#endif

class GuiInspectorFieldNew;

//-----------------------------------------------------------------------------
//    GuiInspectorTextEditNew
//-----------------------------------------------------------------------------

/// A GuiTextEditCtrlNew that additionally commits its value back to its
/// owning GuiInspectorFieldNew when it loses first-responder status, not
/// just on Enter (onAction()). See file header.
class GuiInspectorTextEditNew : public GuiTextEditCtrlNew
{
public:

   typedef GuiTextEditCtrlNew Parent;

protected:

   /// Owning row -- raw pointer, not SimObjectPtr: this control is always
   /// created, owned (as a child), and destroyed by its
   /// GuiInspectorFieldNew, so its lifetime is strictly nested inside its
   /// owner's; the owner never outlives it, and it never outlives the
   /// owner. Cleared defensively in clearOwner() if that ever needs to
   /// change (see GuiInspectorFieldNew::onRemove()).
   GuiInspectorFieldNew* mOwnerField;

public:

   GuiInspectorTextEditNew();

   DECLARE_CONOBJECT(GuiInspectorTextEditNew);
   DECLARE_CATEGORY("Gui Inspector");
   DECLARE_DESCRIPTION("Inspector value field -- a text edit control that commits on Enter or on losing focus.");

   void setOwnerField(GuiInspectorFieldNew* owner) { mOwnerField = owner; }
   void clearOwner() { mOwnerField = NULL; }

   void onAction() override;
   void onLoseFirstResponder() override;
};

//-----------------------------------------------------------------------------
//    GuiInspectorFieldNew
//-----------------------------------------------------------------------------

/// One inspector row: a field name label beside an editable value control.
/// The value control itself is pluggable -- see _createValueControl()/
/// _refreshValueControl()/_applyValueControl() and
/// GuiInspectorBoolFieldNew, which overrides all three to swap in a
/// checkbox-style GuiButtonCtrlNew instead of this class's own default
/// GuiInspectorTextEditNew.
class GuiInspectorFieldNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

   friend class GuiInspectorNew;

protected:

   /// The object this row edits a field of. Weak -- see mInspectedObject's
   /// doc comment on GuiInspectorNew; this is the SAME object, just held
   /// again here so a row can refresh/apply itself independently of its
   /// owning panel walking its whole child list.
   SimObjectPtr<SimObject> mObject;

   /// Name of the field this row edits, e.g. "position". Looked up via
   /// SimObject::getDataField()/setDataField() -- see file header.
   StringTableEntry mFieldName;

   /// NULL for a non-array field. Non-NULL (e.g. "0", "1", ...) selects
   /// one element of an array field -- passed straight through as the
   /// "array" argument to getDataField()/setDataField().
   StringTableEntry mArrayIndex;

   GuiLabelCtrlNew* mNameLabel;

   /// The value-editing control -- created by _createValueControl(),
   /// refreshed/applied by _refreshValueControl()/_applyValueControl().
   /// Base type is GuiControlNew since a subclass (GuiInspectorBoolFieldNew)
   /// puts a GuiButtonCtrlNew here instead of a text field; this class's
   /// own _layoutChildren()/onRemove() etc. only ever need the base
   /// GuiControlNew interface (position/extent/setActive()), never
   /// anything text-field-specific, so the base pointer is sufficient
   /// everywhere except this class's own default virtual implementations
   /// (which know it's really a GuiInspectorTextEditNew and static_cast
   /// accordingly).
   GuiControlNew* mValueControl;

   static const S32 smRowHeight = 22;
   static const S32 smRowPadding = 2;
   static const S32 smLabelWidthPct = 45; ///< Label takes this % of the row's width; the value field takes the rest.

   /// Creates and registers (but does not yet addObject()) this row's
   /// value control, returning it as a base GuiControlNew* -- called once
   /// by init(). Default builds a GuiInspectorTextEditNew wired to this
   /// row via setOwnerField(). GuiInspectorBoolFieldNew overrides this to
   /// build a GuiButtonCtrlNew in "check" mode instead.
   virtual GuiControlNew* _createValueControl();

   /// Pushes mObject's current field value into mValueControl's displayed
   /// state. Default reads getDataField() as a string and calls
   /// GuiTextEditCtrlNew::setText() on mValueControl (static_cast, since
   /// the base class always puts a GuiInspectorTextEditNew there).
   /// GuiInspectorBoolFieldNew overrides this to interpret the same
   /// string as a bool and call GuiButtonCtrlNew::setChecked() instead.
   /// Called by refresh() -- guarded by _shouldSkipRefreshWhileEditing(),
   /// NOT a hardcoded check in refresh() itself, since whether "currently
   /// first responder" means "leave it alone, mid-edit" is itself
   /// control-type-specific -- see that method's own doc comment.
   virtual void _refreshValueControl();

   /// Reads mValueControl's current edited state and writes it back to
   /// mObject via setDataField() -- NOT bracketed by inspectPreApply()/
   /// inspectPostApply() here; apply() does that once, uniformly, around
   /// this call, so overrides don't each need to remember to. Default
   /// reads GuiTextEditCtrlNew::getText() (static_cast). GuiInspectorBool
   /// FieldNew overrides this to read GuiButtonCtrlNew::isChecked()
   /// instead and write "1"/"0".
   virtual void _applyValueControl();

   /// Whether refresh() should skip _refreshValueControl() while
   /// mValueControl->isFirstResponder() is true. Default (true) is
   /// correct for a text field: first-responder means the user is
   /// actively mid-edit, and overwriting the displayed text out from
   /// under a live keystroke is exactly the clobbering refresh() exists
   /// to avoid. GuiInspectorBoolFieldNew overrides this to false: a
   /// button click is atomic (mouse-down grabs first-responder purely as
   /// UI-focus bookkeeping, THEN a same-frame commit -- e.g. Enter fired
   /// while still holding the mouse down, see GuiButtonCtrlNew::
   /// onKeyDown() -- can apply() while it's still, transiently, first
   /// responder), so there is no "mid-edit" state on a checkbox for this
   /// guard to protect; skipping the refresh there was exactly what left
   /// the panel showing a stale value after that exact interaction.
   virtual bool _shouldSkipRefreshWhileEditing() const { return true; }

public:

   GuiInspectorFieldNew();

   DECLARE_CONOBJECT(GuiInspectorFieldNew);
   DECLARE_CATEGORY("Gui Inspector");
   DECLARE_DESCRIPTION("One inspector row: a field name label beside an editable value field.");

   bool onWake() override;
   void onRemove() override;
   void onDeleteNotify(SimObject* object) override;

   /// Configures this (already registered, not-yet-added-to-a-parent) row
   /// to edit the given field of the given object, and builds/attaches its
   /// two child controls (mNameLabel + whatever _createValueControl()
   /// returns). Called once, immediately after construction+
   /// registerObject(), by GuiInspectorNew::_rebuildFields().
   void init(SimObject* object, StringTableEntry fieldName, StringTableEntry arrayIndex, const char* fieldDoc);

   StringTableEntry getFieldName() const { return mFieldName; }

   /// Re-reads the field's current value from mObject and pushes it into
   /// the value control via _refreshValueControl(). Skipped while the
   /// value control is the first responder AND
   /// _shouldSkipRefreshWhileEditing() says that should block a refresh
   /// for this control type (true by default -- see that method) -- so
   /// refreshing an inspector (e.g. GuiInspectorNew::refresh(), or a
   /// periodic auto-refresh) never clobbers an edit the user is actively
   /// mid-edit on, for control types where "first responder" actually
   /// means that.
   void refresh();

   /// Applies the value control's current edited state back to mObject
   /// via _applyValueControl(), bracketed by mObject->inspectPreApply()/
   /// inspectPostApply() -- SimObject's own standard editor-application
   /// contract (see simObject.h). Called by GuiInspectorTextEditNew on
   /// commit (Enter or losing focus), or by GuiInspectorBoolFieldNew's
   /// value button on click -- see those classes' doc comments.
   void apply();

   /// resolves this row's fixed height (smRowHeight) as its preferred
   /// content height, so a parent stacking rows with auto height (see
   /// GuiInspectorNew::_rebuildFields()) sizes each row correctly without
   /// hardcoding the row height a second time at the call site.
   bool getPreferredContentExtent(Point2I& outExtent) const override;

   /// Repositions mNameLabel/mValueControl to split this row's current
   /// width per smLabelWidthPct. Called from init() and again from
   /// resize() so a live width change (e.g. the panel itself being
   /// resized) keeps both children correctly proportioned.
   bool resize(const Point2I& newPosition, const Point2I& newExtent) override;

protected:

   void _layoutChildren();
};

//-----------------------------------------------------------------------------
//    GuiInspectorCheckButtonNew
//-----------------------------------------------------------------------------

/// A GuiButtonCtrlNew that additionally commits back to its owning
/// GuiInspectorFieldNew when clicked, via a direct virtual override --
/// same owner-back-pointer pattern as GuiInspectorTextEditNew (see that
/// class's doc comment), NOT a "<row id>.apply();" console command string.
/// Used by GuiInspectorBoolFieldNew as its checkbox value control.
class GuiInspectorCheckButtonNew : public GuiButtonCtrlNew
{
public:

   typedef GuiButtonCtrlNew Parent;

protected:

   /// Owning row -- see GuiInspectorTextEditNew::mOwnerField's doc
   /// comment; identical lifetime reasoning applies here.
   GuiInspectorFieldNew* mOwnerField;

public:

   GuiInspectorCheckButtonNew();

   DECLARE_CONOBJECT(GuiInspectorCheckButtonNew);
   DECLARE_CATEGORY("Gui Inspector");
   DECLARE_DESCRIPTION("Inspector checkbox value field -- a check-mode button that commits on click.");

   void setOwnerField(GuiInspectorFieldNew* owner) { mOwnerField = owner; }
   void clearOwner() { mOwnerField = NULL; }

   void onAction() override;
};

//-----------------------------------------------------------------------------
//    GuiInspectorBoolFieldNew
//-----------------------------------------------------------------------------

/// A GuiInspectorFieldNew whose value control is a GuiInspectorCheckButtonNew
/// (a check-mode GuiButtonCtrlNew) instead of a text field -- used for
/// TypeBool fields (see GuiInspectorNew::_rebuildFields()'s type check).
/// There is no dedicated checkbox glyph in this control set (see
/// guiButtonCtrlNew.cpp's onRender() -- Check mode is purely a
/// style-state-driven fill/border, same as Toggle), so the button's own
/// caption is used to show the current state in text ("On"/"Off") -- see
/// _refreshValueControl().
class GuiInspectorBoolFieldNew : public GuiInspectorFieldNew
{
public:

   typedef GuiInspectorFieldNew Parent;

protected:

   GuiControlNew* _createValueControl() override;
   void _refreshValueControl() override;
   void _applyValueControl() override;

   /// A checkbox click is atomic, not a mid-edit state -- see this
   /// virtual's base doc comment for the exact interaction (Enter fired
   /// while still holding the mouse down) that a true guard here
   /// silently broke.
   bool _shouldSkipRefreshWhileEditing() const override { return false; }

public:

   DECLARE_CONOBJECT(GuiInspectorBoolFieldNew);
   DECLARE_CATEGORY("Gui Inspector");
   DECLARE_DESCRIPTION("One inspector row for a TypeBool field: a name label beside a checkbox-style button.");
};

//-----------------------------------------------------------------------------
//    GuiInspectorGroupHeaderNew
//-----------------------------------------------------------------------------

/// A non-editing divider row showing one field group's name -- built from
/// an AbstractClassRep::Field::pGroupname on a StartGroupFieldType marker
/// (see GuiInspectorNew::_rebuildFields()). Deliberately NOT a
/// GuiInspectorFieldNew subclass: it has no object/field to edit, and
/// mixing "sometimes this row means something editable, sometimes it's
/// just a caption" into one class via null-checks throughout would be
/// exactly the kind of implicit-state bug class this rewrite is trying to
/// avoid (see gui-rewrite-design.md's general rationale for the rewrite).
class GuiInspectorGroupHeaderNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

   friend class GuiInspectorNew;

protected:

   GuiLabelCtrlNew* mNameLabel;

   static const S32 smRowHeight = 24;
   static const S32 smRowPadding = 2;

public:

   GuiInspectorGroupHeaderNew();

   DECLARE_CONOBJECT(GuiInspectorGroupHeaderNew);
   DECLARE_CATEGORY("Gui Inspector");
   DECLARE_DESCRIPTION("A non-editing divider row showing one field group's name.");

   /// Configures this (already registered, not-yet-added-to-a-parent) row
   /// to show groupName, and builds/attaches mNameLabel. Called once by
   /// GuiInspectorNew::_rebuildFields().
   void init(const char* groupName);

   bool getPreferredContentExtent(Point2I& outExtent) const override;
   bool resize(const Point2I& newPosition, const Point2I& newExtent) override;

protected:

   void _layoutChildren();
};

//-----------------------------------------------------------------------------
//    GuiInspectorNew
//-----------------------------------------------------------------------------

/// Scrollable panel listing every visible persistent field of an inspected
/// SimObject, each as an editable GuiInspectorFieldNew row. See file header.
class GuiInspectorNew : public GuiScrollCtrlNew
{
public:

   typedef GuiScrollCtrlNew Parent;

protected:

   /// Weak -- an inspector outliving the object it was inspecting (e.g.
   /// the object gets deleted while the inspector window is still open)
   /// is a completely ordinary occurrence, not a lifetime error; see
   /// onDeleteNotify(), which clears this and empties the panel rather
   /// than leaving a dangling pointer around.
   SimObjectPtr<SimObject> mInspectedObject;

   /// Every currently-built row, in display order -- same contents as
   /// this control's own child list (every row is a child), kept as a
   /// separate Vector purely so _clearFields() doesn't need to
   /// dynamic_cast its way through the generic SimGroup iterator. Includes
   /// GuiInspectorGroupHeaderNew rows as well as field rows -- anything
   /// this control itself created and must destroy on rebuild/clear.
   Vector<GuiControlNew*> mAllRows;

   /// Subset of mAllRows that are actually editable field rows (i.e.
   /// excludes GuiInspectorGroupHeaderNew) -- what refresh() iterates,
   /// since a group header has no refresh() method of its own.
   Vector<GuiInspectorFieldNew*> mFields;

   /// Destroys every current row (mAllRows/mFields + their corresponding
   /// children) and empties both. Called by _rebuildFields() before
   /// repopulating, and by setInspectedObject(NULL)/onDeleteNotify().
   void _clearFields();

   /// Fully rebuilds mFields from mInspectedObject's current field list --
   /// walks AbstractClassRep::FieldList (via getClassRep()->getFieldList()),
   /// skipping array markers, deprecated fields, and anything flagged
   /// FIELD_HideInInspectors (see AbstractClassRep::FieldFlags). A
   /// StartGroupFieldType marker produces a GuiInspectorGroupHeaderNew
   /// row (named from Field::pGroupname); EndGroupFieldType produces no
   /// row. Every remaining real field produces one row -- a
   /// GuiInspectorBoolFieldNew if the field's type is TypeBool, otherwise
   /// a plain GuiInspectorFieldNew -- stacked top-to-bottom at
   /// smRowHeight-derived Y offsets. Called by setInspectedObject() any
   /// time the inspected object actually changes (not on every refresh()
   /// -- see that method).
   ///
   /// Each row is left at WIDTH auto (never given an explicit pixel
   /// width) -- this control's own Parent, GuiScrollCtrlNew, overrides
   /// getClientExtent() to report its extent minus the reserved
   /// scrollbar gutter, which is what auto width resolves against (see
   /// GuiControlNew::_resolveAutoDimension()); a row therefore fills
   /// exactly the visible/reachable viewport, automatically, on every
   /// future layout pass (including this panel being resized later),
   /// with no bookkeeping needed here at all. An earlier version of this
   /// method computed a narrowed pixel width by hand and re-pinned every
   /// row's width on every resize -- that workaround is gone now that
   /// the actual bug (GuiScrollCtrlNew's children resolving auto/percent
   /// width against its full extent instead of its client area) is fixed
   /// at the source.
   void _rebuildFields();

public:

   GuiInspectorNew();

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiInspectorNew);
   DECLARE_CATEGORY("Gui Inspector");
   DECLARE_DESCRIPTION("Inspects a SimObject, listing its fields as editable name/value rows.");

   bool onWake() override;
   void onRemove() override;
   void onDeleteNotify(SimObject* object) override;

   /// Points this inspector at a new object and rebuilds every row from
   /// scratch (see _rebuildFields()). Passing NULL clears the panel.
   /// No-op if object == the object already being inspected (call
   /// refresh() instead to re-pull values without rebuilding rows).
   void setInspectedObject(SimObject* object);
   SimObject* getInspectedObject() const { return mInspectedObject; }

   /// Re-pulls every row's displayed value from the (unchanged) inspected
   /// object -- see GuiInspectorFieldNew::refresh(). Cheap relative to
   /// setInspectedObject(): no rows are destroyed/recreated, so this is
   /// safe to call frequently (e.g. once a frame) to keep the panel live
   /// while the underlying object's fields change from elsewhere (script,
   /// simulation, another inspector on the same object).
   void refresh();
};

#endif // _GUIINSPECTORNEW_H_
