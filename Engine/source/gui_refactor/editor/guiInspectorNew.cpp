//-----------------------------------------------------------------------------
// guiInspectorNew.cpp
//-----------------------------------------------------------------------------

#include "gui_refactor/editor/guiInspectorNew.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT(GuiInspectorTextEditNew);
IMPLEMENT_CONOBJECT(GuiInspectorFieldNew);
IMPLEMENT_CONOBJECT(GuiInspectorCheckButtonNew);
IMPLEMENT_CONOBJECT(GuiInspectorBoolFieldNew);
IMPLEMENT_CONOBJECT(GuiInspectorGroupHeaderNew);
IMPLEMENT_CONOBJECT(GuiInspectorNew);

namespace
{
   /// Pins ctrl's position/extent as fixed-pixel AUTHORED dimensions
   /// (left/top/height always; width too, UNLESS autoWidth is true), via
   /// the same public setDataField() field interface a .gui file's own
   /// field assignments go through -- see this file's own recent fix
   /// history for why a plain resize() call alone is not sufficient
   /// (auto width/top get silently re-resolved on every later
   /// resolveLayout() pass, undoing resize()'s one-off effect).
   ///
   /// autoWidth (default false) leaves width at "auto" instead of pinning
   /// it to extent.x -- appropriate when the caller WANTS ordinary auto-
   /// width behavior (fill the parent's client area, correctly excluding
   /// a scrollbar gutter now that GuiScrollCtrlNew overrides
   /// getClientExtent() -- see that class's own fix) rather than a
   /// specific computed pixel value. extent.x is still used for THIS
   /// call's immediate resize() (needed to get the initial layout right
   /// before the next resolveLayout() pass re-derives auto width
   /// properly), just not pinned as the permanent authored width.
   ///
   /// A free function (not a GuiInspectorFieldNew/GuiInspectorGroupHeaderNew
   /// member) specifically because it operates on ARBITRARY GuiControlNew
   /// instances -- this row's own children (mNameLabel, mValueControl),
   /// not just `this` -- and C++ only allows a derived class to reach a
   /// base class's PROTECTED members through a pointer/reference of the
   /// derived type, not through a plain GuiControlNew*; going through the
   /// public field API sidesteps that restriction entirely and matches
   /// how position/size are set from script anyway.
   void pinFixedBounds(GuiControlNew* ctrl, const Point2I& pos, const Point2I& extent, bool autoWidth = false)
   {
      if (!ctrl)
         return;

      char buf[32];

      dSprintf(buf, sizeof(buf), "%d", pos.x);
      ctrl->setDataField(StringTable->insert("left"), NULL, buf);

      dSprintf(buf, sizeof(buf), "%d", pos.y);
      ctrl->setDataField(StringTable->insert("top"), NULL, buf);

      if (autoWidth)
         ctrl->setDataField(StringTable->insert("width"), NULL, "auto");
      else
      {
         dSprintf(buf, sizeof(buf), "%d", extent.x);
         ctrl->setDataField(StringTable->insert("width"), NULL, buf);
      }

      dSprintf(buf, sizeof(buf), "%d", extent.y);
      ctrl->setDataField(StringTable->insert("height"), NULL, buf);
   }
}

//=============================================================================
//    GuiInspectorTextEditNew
//=============================================================================

GuiInspectorTextEditNew::GuiInspectorTextEditNew()
   : mOwnerField(NULL)
{
}

//-----------------------------------------------------------------------------

void GuiInspectorTextEditNew::onAction()
{
   // Enter -- commit, same as any other field's "I'm done typing" moment.
   // Parent::onAction() also fires mConsoleCommand/onAction_callback if
   // either is bound; a caller wiring up its own inspector-value-changed
   // hook on this control directly still gets that after the field itself
   // has been applied, which is the correct order (the object's data is
   // already updated by the time any such hook runs).
   if (mOwnerField)
      mOwnerField->apply();

   Parent::onAction();
}

//-----------------------------------------------------------------------------

void GuiInspectorTextEditNew::onLoseFirstResponder()
{
   // Base class does NOT treat focus loss as a commit point (see this
   // control's own file header) -- an inspector field needs it to,
   // otherwise clicking from one field straight to another (rather than
   // pressing Enter first) would silently discard the edit. Applied
   // BEFORE Parent::onLoseFirstResponder() so the value is already
   // written back by the time anything that call triggers (e.g. a
   // tooltip/focus callback elsewhere) might go looking at the object.
   if (mOwnerField)
      mOwnerField->apply();

   Parent::onLoseFirstResponder();
}

//=============================================================================
//    GuiInspectorFieldNew
//=============================================================================

GuiInspectorFieldNew::GuiInspectorFieldNew()
   : mFieldName(NULL),
   mArrayIndex(NULL),
   mNameLabel(NULL),
   mValueControl(NULL)
{
   // NOTE: mCanHit is deliberately left at its default (true). This row
   // is purely a layout container with no rendering/interaction of its
   // own, so it may look like it should be setCanHit(false) -- but
   // GuiControlNew::findHitControl() only recurses into a child's OWN
   // children if that child itself passes ctrl->mCanHit (see
   // guiControlNew.cpp's findHitControl(): the recursive call happens
   // inside the same `else if (ctrl->mCanHit && ...)` branch that gates
   // hit-testing the child itself). A false here made mNameLabel/
   // mValueControl permanently unreachable by mouse -- clicking anywhere
   // on a row silently hit nothing, and only Tab (first-responder
   // navigation, which doesn't go through findHitControl() at all) could
   // ever focus the value control. mCapturesInput is left at ITS default
   // (false) instead, which is what actually keeps this row from
   // intercepting a hit that should land on one of its two children.
   setAllowOverflow(true); // this row's children are positioned explicitly by _layoutChildren(), not flow-resolved
}

//-----------------------------------------------------------------------------

bool GuiInspectorFieldNew::onWake()
{
   if (!Parent::onWake())
      return false;

   return true;
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::onRemove()
{
   // Defensive: a value control's onLoseFirstResponder()/onAction() would
   // otherwise call back into this row (via its owner back-pointer) after
   // it's partway through being torn down. mValueControl is about to be
   // destroyed as this row's own SimGroup child anyway (normal
   // SimGroup::onRemove() cascade), but clearing the back-pointer first
   // means that cascade can't produce a call back into a half-destroyed
   // GuiInspectorFieldNew even transiently. Only one of the two
   // dynamic_casts below can ever succeed for a given row (mValueControl
   // is exactly one concrete type at a time), but checking both here
   // keeps this method correct regardless of which _createValueControl()
   // override built it, without this base class needing to know which
   // subclass it's actually dealing with.
   GuiInspectorTextEditNew* textEdit = dynamic_cast<GuiInspectorTextEditNew*>(mValueControl);
   if (textEdit)
      textEdit->clearOwner();

   GuiInspectorCheckButtonNew* checkButton = dynamic_cast<GuiInspectorCheckButtonNew*>(mValueControl);
   if (checkButton)
      checkButton->clearOwner();

   if (mObject)
      clearNotify(mObject);

   Parent::onRemove();
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::onDeleteNotify(SimObject* object)
{
   if (object == (SimObject*)mObject)
   {
      // The inspected object went away out from under this one row
      // specifically (rather than the whole panel being torn down via
      // GuiInspectorNew::onDeleteNotify(), which clears every row at
      // once) -- disable this row rather than leaving it pointing at a
      // dead object. The owning GuiInspectorNew will also get its own
      // onDeleteNotify() for the same object and rebuild/clear the whole
      // panel; this local guard just makes sure THIS row can't apply()
      // a stray commit into that narrow window in between.
      mObject = NULL;
      if (mValueControl)
         mValueControl->setActive(false);
   }

   Parent::onDeleteNotify(object);
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::init(SimObject* object, StringTableEntry fieldName, StringTableEntry arrayIndex, const char* fieldDoc)
{
   mObject = object;
   mFieldName = fieldName;
   mArrayIndex = arrayIndex;

   if (mObject)
      deleteNotify(mObject);

   mNameLabel = new GuiLabelCtrlNew();
   mNameLabel->registerObject();
   mNameLabel->setText(fieldName);
   if (fieldDoc && fieldDoc[0])
      mNameLabel->setDataField(StringTable->insert("tooltip"), NULL, fieldDoc);

   mValueControl = _createValueControl();

   addObject(mNameLabel);
   addObject(mValueControl);

   refresh();
   _layoutChildren();
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiInspectorFieldNew::_createValueControl()
{
   GuiInspectorTextEditNew* edit = new GuiInspectorTextEditNew();
   edit->registerObject();
   edit->setOwnerField(this);
   return edit;
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::_refreshValueControl()
{
   GuiInspectorTextEditNew* edit = static_cast<GuiInspectorTextEditNew*>(mValueControl);

   const char* value = mObject->getDataField(mFieldName, mArrayIndex);
   edit->setText(value ? value : "");
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::_applyValueControl()
{
   GuiInspectorTextEditNew* edit = static_cast<GuiInspectorTextEditNew*>(mValueControl);

   mObject->setDataField(mFieldName, mArrayIndex, edit->getText().c_str());
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::refresh()
{
   if (!mObject || !mValueControl)
      return;

   // Never stomp an edit the user is actively making -- but "first
   // responder" only means "mid-edit, leave it alone" for SOME value-
   // control types (a text field, where first-responder tracks real
   // in-progress typing) -- not others (a checkbox button, where
   // mouse-down grabs first-responder purely as transient UI-focus
   // bookkeeping, not an edit-in-progress marker -- see
   // _shouldSkipRefreshWhileEditing()'s own doc comment for the exact
   // interaction, Enter fired while still holding the mouse down on the
   // button, that a blanket check here used to silently break: apply()
   // would run and correctly write the new value, but this refresh()
   // call immediately after it would bail out because the button was
   // still, transiently, first responder -- leaving the panel showing
   // the OLD value even though the object's field had already changed).
   if (mValueControl->isFirstResponder() && _shouldSkipRefreshWhileEditing())
      return;

   _refreshValueControl();
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::apply()
{
   if (!mObject || !mValueControl)
      return;

   // Standard SimObject editor-application contract (see simObject.h's
   // own "Torque Editors" doc comment): inspectPreApply()/
   // inspectPostApply() bracket the actual field write, giving the
   // object a chance to react before/after -- e.g. GuiControlNew's own
   // inspectPostApply() override runs a sleep()/awaken() cycle so a
   // changed field (font, style, layout dimension, etc.) actually takes
   // effect without every single control needing its own bespoke
   // "field X changed, now do Y" logic. Both are public virtuals
   // declared directly on SimObject, so this works uniformly regardless
   // of what mObject actually is -- not GuiControlNew-specific, even
   // though that's the override that currently does something with it.
   // Applied once here, uniformly, around _applyValueControl() -- NOT
   // inside each _applyValueControl() override -- so every value-control
   // type gets the same bracket without needing to remember it.
   mObject->inspectPreApply();
   _applyValueControl();
   mObject->inspectPostApply();

   // Re-read rather than trust the edited state verbatim: setDataField()
   // can reject or reformat the value (validators, enum coercion,
   // protected setters that clamp/normalize) -- pulling it back through
   // _refreshValueControl()/getDataField() shows the user what the
   // object actually ended up storing, same as every other console-
   // driven field editor in this codebase. Deliberately AFTER
   // inspectPostApply(): that call may itself have further side effects
   // on the field (e.g. a sleep/wake cycle re-deriving something), so
   // refreshing before it ran could still show a stale value.
   refresh();
}

//-----------------------------------------------------------------------------

bool GuiInspectorFieldNew::getPreferredContentExtent(Point2I& outExtent) const
{
   outExtent = getRawBounds().extent;
   outExtent.y = smRowHeight;
   return true;
}

//-----------------------------------------------------------------------------

bool GuiInspectorFieldNew::resize(const Point2I& newPosition, const Point2I& newExtent)
{
   const bool result = Parent::resize(newPosition, newExtent);
   _layoutChildren();
   return result;
}

//-----------------------------------------------------------------------------

void GuiInspectorFieldNew::_layoutChildren()
{
   if (!mNameLabel || !mValueControl)
      return;

   const Point2I extent = getExtent();
   const S32 labelWidth = (extent.x * smLabelWidthPct) / 100;
   const S32 valueWidth = extent.x - labelWidth - smRowPadding;
   const S32 rowH = extent.y > 0 ? extent.y : smRowHeight;

   pinFixedBounds(mNameLabel, Point2I(0, 0), Point2I(labelWidth, rowH));
   pinFixedBounds(mValueControl, Point2I(labelWidth + smRowPadding, 0), Point2I(getMax(valueWidth, 0), rowH));
}

//=============================================================================
//    GuiInspectorCheckButtonNew
//=============================================================================

GuiInspectorCheckButtonNew::GuiInspectorCheckButtonNew()
   : mOwnerField(NULL)
{
}

//-----------------------------------------------------------------------------

void GuiInspectorCheckButtonNew::onAction()
{
   // GuiButtonCtrlNew::_fireClick() has already flipped mChecked (via
   // setChecked()) BEFORE calling onAction() (see guiButtonCtrlNew.cpp)
   // -- so by the time this override runs, isChecked() already reflects
   // the click's new state; apply() just needs to persist it. Applied
   // BEFORE Parent::onAction() for the same reason GuiInspectorTextEditNew
   // does (see that class's onAction() override): the object's data
   // should already be updated by the time anything Parent::onAction()
   // triggers (mConsoleCommand/onAction_callback, if either happens to
   // also be bound) might go looking at it.
   if (mOwnerField)
      mOwnerField->apply();

   Parent::onAction();
}

//=============================================================================
//    GuiInspectorBoolFieldNew
//=============================================================================

GuiControlNew* GuiInspectorBoolFieldNew::_createValueControl()
{
   GuiInspectorCheckButtonNew* button = new GuiInspectorCheckButtonNew();
   button->registerObject();
   button->setMode(GuiButtonMode_Check);
   button->setOwnerField(this);
   return button;
}

//-----------------------------------------------------------------------------

void GuiInspectorBoolFieldNew::_refreshValueControl()
{
   GuiInspectorCheckButtonNew* button = static_cast<GuiInspectorCheckButtonNew*>(mValueControl);

   const char* value = mObject->getDataField(mFieldName, mArrayIndex);
   const bool checked = value && dAtob(value);

   button->setChecked(checked);
   button->setText(checked ? "On" : "Off");
}

//-----------------------------------------------------------------------------

void GuiInspectorBoolFieldNew::_applyValueControl()
{
   GuiInspectorCheckButtonNew* button = static_cast<GuiInspectorCheckButtonNew*>(mValueControl);

   // By the time this runs, the button's own click handling
   // (GuiButtonCtrlNew::_fireClick(), then this class's own
   // GuiInspectorCheckButtonNew::onAction() override) has already
   // flipped mChecked -- so isChecked() already reflects the NEW,
   // post-click state; this call is just persisting that state to the
   // object, not the one that decided it.
   mObject->setDataField(mFieldName, mArrayIndex, button->isChecked() ? "1" : "0");
}

//=============================================================================
//    GuiInspectorGroupHeaderNew
//=============================================================================

GuiInspectorGroupHeaderNew::GuiInspectorGroupHeaderNew()
   : mNameLabel(NULL)
{
   setAllowOverflow(true); // mNameLabel is positioned explicitly by _layoutChildren(), not flow-resolved
}

//-----------------------------------------------------------------------------

void GuiInspectorGroupHeaderNew::init(const char* groupName)
{
   mNameLabel = new GuiLabelCtrlNew();
   mNameLabel->registerObject();
   mNameLabel->setText(groupName ? groupName : "");

   addObject(mNameLabel);
   _layoutChildren();
}

//-----------------------------------------------------------------------------

bool GuiInspectorGroupHeaderNew::getPreferredContentExtent(Point2I& outExtent) const
{
   outExtent = getRawBounds().extent;
   outExtent.y = smRowHeight;
   return true;
}

//-----------------------------------------------------------------------------

bool GuiInspectorGroupHeaderNew::resize(const Point2I& newPosition, const Point2I& newExtent)
{
   const bool result = Parent::resize(newPosition, newExtent);
   _layoutChildren();
   return result;
}

//-----------------------------------------------------------------------------

void GuiInspectorGroupHeaderNew::_layoutChildren()
{
   if (!mNameLabel)
      return;

   const Point2I extent = getExtent();
   const S32 rowH = extent.y > 0 ? extent.y : smRowHeight;

   pinFixedBounds(mNameLabel, Point2I(0, 0), Point2I(extent.x, rowH));
}

//=============================================================================
//    GuiInspectorNew
//=============================================================================

GuiInspectorNew::GuiInspectorNew()
{
   setCanHit(true);
   setCapturesInput(true);
}

//-----------------------------------------------------------------------------

void GuiInspectorNew::initPersistFields()
{
   docsURL;

   // NOTE: the inspected object is intentionally NOT exposed as an
   // addField()/addProtectedField() here. mInspectedObject is a
   // SimObjectPtr<SimObject> (a WeakRefPtr wrapper -- see this class's
   // own header), not a bare SimObject*, so it is not layout-compatible
   // with what TypeSimObjectPtr's Con::getData()/setData() (and the
   // generic defaultProtectedGetFn/defaultProtectedSetFn pair) expect to
   // read/write at a raw Offset() into this object. Script access goes
   // through the inspect()/getInspectedObject() engine methods below
   // instead, which go through setInspectedObject()/mInspectedObject
   // properly rather than any raw memory offset.

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool GuiInspectorNew::onWake()
{
   if (!Parent::onWake())
      return false;

   // If an object was assigned via script before this control woke (e.g.
   // "inspectedObject" set in a .gui file, or setInspectedObject() called
   // right after construction but before addObject() onto a live canvas),
   // the rows couldn't have been built yet -- _rebuildFields() needs a
   // live/awake control tree to add child controls into. Build them now.
   if (mInspectedObject && mFields.empty())
      _rebuildFields();

   return true;
}

//-----------------------------------------------------------------------------

void GuiInspectorNew::onRemove()
{
   _clearFields();

   if (mInspectedObject)
      clearNotify(mInspectedObject);

   Parent::onRemove();
}

//-----------------------------------------------------------------------------

void GuiInspectorNew::onDeleteNotify(SimObject* object)
{
   if (object == (SimObject*)mInspectedObject)
   {
      mInspectedObject = NULL;
      _clearFields();
   }

   Parent::onDeleteNotify(object);
}

//-----------------------------------------------------------------------------

void GuiInspectorNew::setInspectedObject(SimObject* object)
{
   if (object == (SimObject*)mInspectedObject)
      return;

   if (mInspectedObject)
      clearNotify(mInspectedObject);

   mInspectedObject = object;

   if (mInspectedObject)
      deleteNotify(mInspectedObject);

   // Deferred to onWake() if this control isn't awake yet -- see that
   // method's own comment on why rebuilding requires a live control tree.
   if (mAwake)
      _rebuildFields();
}

//-----------------------------------------------------------------------------

void GuiInspectorNew::refresh()
{
   for (U32 i = 0; i < mFields.size(); i++)
      mFields[i]->refresh();
}

//-----------------------------------------------------------------------------

void GuiInspectorNew::_clearFields()
{
   for (U32 i = 0; i < mAllRows.size(); i++)
   {
      GuiControlNew* row = mAllRows[i];
      if (row)
      {
         removeObject(row);
         row->deleteObject();
      }
   }

   mAllRows.clear();
   mFields.clear();
}

//-----------------------------------------------------------------------------

void GuiInspectorNew::_rebuildFields()
{
   _clearFields();

   SimObject* object = mInspectedObject;
   if (!object)
      return;

   const AbstractClassRep::FieldList& list = object->getFieldList();

   S32 nextY = 0;
   const S32 fieldRowSpacing = GuiInspectorFieldNew::smRowHeight + GuiInspectorFieldNew::smRowPadding;
   const S32 groupRowSpacing = GuiInspectorGroupHeaderNew::smRowHeight + GuiInspectorGroupHeaderNew::smRowPadding;

   // Used only as the initial-paint width passed to pinFixedBounds()
   // below (which leaves each row's AUTHORED width at "auto" -- see its
   // own doc comment) -- getClientExtent() is now correctly scrollbar-
   // aware (see GuiScrollCtrlNew::getClientExtent()), so this already
   // excludes the reserved gutter; auto width will re-derive the exact
   // same number itself on the next real layout pass regardless.
   const S32 rowWidth = getClientExtent().x;

   StringTableEntry lastGroupName = NULL;

   for (U32 i = 0; i < list.size(); i++)
   {
      const AbstractClassRep::Field& f = list[i];

      // Group headers get their own row type -- see
      // GuiInspectorGroupHeaderNew. EndGroupFieldType carries no
      // displayable info of its own (see consoleObject.cpp's endGroup())
      // and is simply skipped, same as before.
      if (f.type == AbstractClassRep::StartGroupFieldType)
      {
         // "Ungrouped" is not an author-facing group name -- it's
         // synthesized once per class level by the docsURL; macro (see
         // consoleFunctions.h: under TORQUE_TOOLS, docsURL; expands to
         // addGroup("Ungrouped"); addProtectedField("docsURL", ...,
         // FIELD_ComponentInspectors); endGroup("Ungrouped");). Every
         // class in the hierarchy that calls initPersistFields() has
         // this at its top (established convention -- see this file's
         // own initPersistFields() too), so a deep hierarchy produces
         // one "Ungrouped" header per level, each containing nothing but
         // that one docsURL field -- which is itself already filtered
         // out below via FIELD_ComponentInspectors. Skipping the header
         // by this reserved name (rather than, say, only emitting a
         // header once its group turns out to contain a real field)
         // keeps the group-header logic a simple one-marker-ahead scan;
         // suppressing a genuinely empty non-"Ungrouped" group is not
         // attempted here.
         static StringTableEntry sUngrouped = StringTable->insert("Ungrouped");
         if (f.pGroupname == sUngrouped)
            continue;

         // Collapse an immediately-repeated group name into a single
         // header instead of two identical-looking ones back to back --
         // e.g. GuiControlNew itself registers "Control" via two
         // separate addGroup("Control")/endGroup("Control") blocks (see
         // guiControlNew.cpp). Only adjacent repeats are collapsed; the
         // same name reappearing after a DIFFERENT group in between
         // still gets its own header, since that's a much rarer/odder
         // case not worth the bookkeeping a full occurred-anywhere-
         // already set would need.
         if (f.pGroupname == lastGroupName)
            continue;

         lastGroupName = f.pGroupname;

         GuiInspectorGroupHeaderNew* header = new GuiInspectorGroupHeaderNew();
         header->registerObject();

         addObject(header);
         pinFixedBounds(header, Point2I(0, nextY), Point2I(rowWidth, GuiInspectorGroupHeaderNew::smRowHeight), /*autoWidth*/ true);
         header->init(f.pGroupname);

         mAllRows.push_back(header);
         nextY += groupRowSpacing;
         continue;
      }

      // Skip remaining marker types (array start/end) and deprecated
      // fields -- all share the property of type being
      // >= ARCFirstCustomField (see consoleObject.h's ACRFieldTypes
      // enum). StartGroupFieldType was handled above and EndGroupFieldType
      // produces no row, so both are correctly caught by this same check
      // and simply skipped here.
      if (f.type >= AbstractClassRep::ARCFirstCustomField)
         continue;

      // Fields explicitly marked as not belonging in an inspector.
      if (f.flag.test(AbstractClassRep::FIELD_HideInInspectors))
         continue;

      // Component-inspector fields are a different, non-standard-layout
      // presentation (see their own flag's doc comment in consoleObject.h)
      // that this general row-per-field panel isn't built to show.
      if (f.flag.test(AbstractClassRep::FIELD_ComponentInspectors))
         continue;

      // TypeBool fields get a checkbox-style row (GuiInspectorBoolFieldNew)
      // instead of the default text-edit row -- see that class's doc
      // comment. No dropdown control exists yet for enum fields, so those
      // still fall through to the default text row below.
      const bool isBoolField = (f.type == TypeBool);

      const S32 elementCount = getMax((S32)f.elementCount, 1);

      for (S32 element = 0; element < elementCount; element++)
      {
         char indexBuf[16];
         StringTableEntry arrayIndex = NULL;
         if (elementCount > 1)
         {
            dSprintf(indexBuf, sizeof(indexBuf), "%d", element);
            arrayIndex = StringTable->insert(indexBuf);
         }

         GuiInspectorFieldNew* row = isBoolField
            ? static_cast<GuiInspectorFieldNew*>(new GuiInspectorBoolFieldNew())
            : new GuiInspectorFieldNew();
         row->registerObject();

         addObject(row);
         pinFixedBounds(row, Point2I(0, nextY), Point2I(rowWidth, GuiInspectorFieldNew::smRowHeight), /*autoWidth*/ true);
         row->init(object, f.pFieldname, arrayIndex, f.pFieldDocs);

         mAllRows.push_back(row);
         mFields.push_back(row);
         nextY += fieldRowSpacing;
      }
   }
}

//-----------------------------------------------------------------------------
//    GuiInspectorFieldNew -- Script API
//-----------------------------------------------------------------------------

// A plain public C++ method is not, by itself, callable from script --
// only DefineEngineMethod-registered methods are. Both value-control
// types (GuiInspectorTextEditNew, GuiInspectorCheckButtonNew) commit via
// a direct C++ virtual-override callback to their owning row, not a
// console command string, so nothing internal to this file actually
// needs this binding -- it's exposed purely as general script API, so
// e.g. an external tool driving the panel programmatically can force a
// specific row to (re-)apply its current value-control state.
DefineEngineMethod(GuiInspectorFieldNew, apply, void, (), ,
   "Applies this row's current value-control state back to the inspected object's field. "
   "Normally called internally (on Enter/focus-loss for a text row, on click for a checkbox row); "
   "exposed to script for external callers that want to force it directly.\n")
{
   object->apply();
}

//-----------------------------------------------------------------------------
//    Script API
//-----------------------------------------------------------------------------

DefineEngineMethod(GuiInspectorNew, inspect, void, (SimObject* target), ,
   "Points this inspector at the given object and rebuilds its field list.\n"
   "@param target The SimObject to inspect. Pass an empty/null argument to clear the inspector.\n")
{
   object->setInspectedObject(target);
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiInspectorNew, getInspectedObject, S32, (), ,
   "@return The id of the object currently being inspected, or 0 if none.\n")
{
   SimObject* inspected = object->getInspectedObject();
   return inspected ? inspected->getId() : 0;
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiInspectorNew, refresh, void, (), ,
   "Re-reads every field's current value from the inspected object without rebuilding rows.\n")
{
   object->refresh();
}
