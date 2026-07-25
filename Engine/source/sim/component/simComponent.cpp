//-----------------------------------------------------------------------------
// SimComponent.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "sim/component/simComponent.h"
#include "console/typeValidators.h"
#include "sim/simObject.h"

IMPLEMENT_CONOBJECT(SimComponent);

//-----------------------------------------------------------------------------

SimComponent::SimComponent()
   : mOwner(NULL),
     mInstanceName(NULL),
     mOwnerNetMask(0),
     mEnabled(true)
{
}

SimComponent::~SimComponent()
{
   // mOwner is a raw, non-owning pointer; the owner is responsible for
   // calling onComponentRemove() before this destructor runs (see
   // SimObject::removeComponent / SimObject::onRemove). We do not touch
   // mOwner here.
}

bool SimComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void SimComponent::onRemove()
{
   mOwner = NULL;
   Parent::onRemove();
}

void SimComponent::initPersistFields()
{
   ADD_FIELD("instanceName", TypeString, Offset(mInstanceName, SimComponent))
      .doc("Instance name for the component, to be used to differentiat multiple of the same component.");
}

//-----------------------------------------------------------------------------

bool SimComponent::onComponentAdd(SimObject* owner)
{
   mOwner = owner;
   return true;
}



