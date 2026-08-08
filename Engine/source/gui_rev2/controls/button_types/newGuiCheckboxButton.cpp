//-----------------------------------------------------------------------------
// gui_rev2/controls/button_types/newGuiCheckboxButton.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/button_types/newGuiCheckboxButton.h"

IMPLEMENT_CONOBJECT(NewGuiCheckboxButton);

NewGuiCheckboxButton::NewGuiCheckboxButton()
{
   mButtonType = ButtonType_Toggle;
   mCheckboxStyle = true;
}

// Removes "buttonType" and "checkboxStyle" - both fixed by this class, not authorable.
void NewGuiCheckboxButton::initPersistFields()
{
   Parent::initPersistFields();
   removeField("buttonType");
   removeField("checkboxStyle");
}
