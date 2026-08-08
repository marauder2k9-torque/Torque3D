//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspectorFieldBinding.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIINSPECTORFIELDBINDING_H_
#define _NEWGUIINSPECTORFIELDBINDING_H_

#include <functional>
#include <unordered_map>

#ifndef _CONSOLEOBJECT_H_
#include "console/consoleObject.h"
#endif

class NewGuiInspectorField;
class NewGuiControl;

/// One field-type's control strategy: how to build its value control(s), how to push a
/// confirmed field value into them, and how to submit a value change requested through them.
///
/// Replaces the old GuiInspectorFieldXXX-subclass-per-type pattern (see
/// gui_rev2/editor/README - or ask before adding a new subclass here). A type is supported by
/// registering one of these once, wherever that type is declared - not by writing a new
/// NewGuiControl-derived class.
///
/// Ordering contract (server-authoritative - see refresh()/apply() below):
///   - refresh() is the ONLY function allowed to push a value into the control(s). It is always
///     driven by a fresh getDataField() read, never by an assumption that a prior apply()
///     succeeded.
///   - apply() is the ONLY function allowed to submit a change (via setDataField). It must NOT
///     touch the control(s) - no setText(), no setChecked(), nothing. The control's displayed
///     value only ever changes via a subsequent refresh() call, which re-reads the field and
///     may show the same old value again if the request didn't (yet, or ever) take effect.
struct InspectorFieldBinding
{
   /// Builds whatever child control(s) this type needs into valueArea. Called once, from
   /// NewGuiInspectorField::configure(). valueArea is already parented and already configured
   /// for horizontal layout by the caller - this only adds children to it.
   std::function<void(NewGuiInspectorField* row, NewGuiControl* valueArea)> buildControls;

   /// Reads the field's current value (row->readCurrentValueString()) and pushes it into the
   /// control(s) built above. Called on initial configure(), on every periodic refresh pass
   /// (see NewGuiInspector's poll-refresh - there is no push notification for external field
   /// changes; see design notes on SimObject::onStaticModified), and again immediately after
   /// apply() submits a request, to confirm (or correct) what the control shows.
   std::function<void(NewGuiInspectorField* row)> refresh;

   /// Submits a value change request (row->writeValueString()) built from the control(s)'
   /// current state. changedControl is whichever control's native-change-notify fired this
   /// call. Must not write to any control - see the ordering contract above.
   std::function<void(NewGuiInspectorField* row, NewGuiControl* changedControl)> apply;
};

/// Maps a console type id (AbstractClassRep::Field::type) to its InspectorFieldBinding.
///
/// Registration is independent of ConsoleBaseType/dynamicTypes.h - a binding can be registered
/// from anywhere (next to the type's own declaration, from a dedicated bindings translation
/// unit, from editor-only code that doesn't ship with a headless server build, etc). Order of
/// registration doesn't matter; this is just a map.
class InspectorFieldBindingRegistry
{
public:
   /// Registers (or replaces) the binding for a console type id.
   static void registerBinding(S32 consoleTypeId, InspectorFieldBinding binding);

   /// Returns the binding for a console type id, or NULL if none was registered. Callers
   /// (NewGuiInspectorField::configure()) fall back to a generic single-text-edit binding when
   /// this returns NULL, so every field is at least editable even if nobody registered a
   /// dedicated control for its type.
   static const InspectorFieldBinding* find(S32 consoleTypeId);

private:
   static std::unordered_map<S32, InspectorFieldBinding>& map();
};

#endif // _NEWGUIINSPECTORFIELDBINDING_H_
