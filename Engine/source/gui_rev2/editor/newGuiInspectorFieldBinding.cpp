//-----------------------------------------------------------------------------
// gui_rev2/editor/newGuiInspectorFieldBinding.cpp
//-----------------------------------------------------------------------------
#include "gui_rev2/editor/newGuiInspectorFieldBinding.h"

void InspectorFieldBindingRegistry::registerBinding(S32 consoleTypeId, InspectorFieldBinding binding)
{
   map()[consoleTypeId] = std::move(binding);
}

const InspectorFieldBinding* InspectorFieldBindingRegistry::find(S32 consoleTypeId)
{
   auto& m = map();
   auto it = m.find(consoleTypeId);
   return it != m.end() ? &it->second : NULL;
}

std::unordered_map<S32, InspectorFieldBinding>& InspectorFieldBindingRegistry::map()
{
   // Function-local static: registerBinding() may run from other translation units' own
   // static-init (e.g. a binding registered next to its type's declaration), so this must be
   // safely constructible regardless of static-init order across TUs.
   static std::unordered_map<S32, InspectorFieldBinding> sMap;
   return sMap;
}
