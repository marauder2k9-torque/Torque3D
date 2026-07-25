//-----------------------------------------------------------------------------
// GuiInspectorComponentGroup
//-----------------------------------------------------------------------------
#ifndef _GUI_INSPECTOR_COMPONENTGROUP_H_
#define _GUI_INSPECTOR_COMPONENTGROUP_H_

#include "gui/editor/inspector/group.h"

class SimComponent;
class SimObject;

class GuiInspectorComponentGroup : public GuiInspectorGroup
{
   typedef GuiInspectorGroup Parent;

protected:
   /// The component instance whose field list this group enumerates
   /// (names/types only - values are never read from or written to this
   /// pointer directly by this class). Not owned by this group.
   SimComponent* mComponent;

   /// The owner SimObject every constructed GuiInspectorField actually
   /// targets (via the normal setTargetObject). Not owned by this group.
   SimObject* mOwner;

public:
   DECLARE_CONOBJECT(GuiInspectorComponentGroup);

   GuiInspectorComponentGroup() : mComponent(NULL), mOwner(NULL) {}

   GuiInspectorComponentGroup(const String& groupName, SimObjectPtr<GuiInspector> parent, SimObject* owner, SimComponent* component)
      : GuiInspectorGroup(groupName, parent), mComponent(component), mOwner(owner)
   {
   }

   /// inspectGroup is overridden to enumerate mComponent's own field
   /// list (mComponent->getFieldList(), via ConsoleObject - components
   /// are not SimObjects, see simComponent.h) instead of the base
   /// class's findCommonAncestorClass()/mParent-target-list walk. Every
   /// constructed field targets mOwner (not mComponent) via the normal
   /// setTargetObject - see file header for why that's sufficient.
   bool inspectGroup() override;
   void updateAllFields() override;

   SimComponent* getComponent() const { return mComponent; }

protected:
   bool createContent() override;
};

#endif // _GUI_INSPECTOR_COMPONENTGROUP_H_
