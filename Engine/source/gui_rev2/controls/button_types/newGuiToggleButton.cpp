//-----------------------------------------------------------------------------
// gui_rev2/controls/button_types/newGuiToggleButton.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/button_types/newGuiToggleButton.h"

IMPLEMENT_CONOBJECT(NewGuiToggleButton);

NewGuiToggleButton::NewGuiToggleButton()
{
   mButtonType = ButtonType_Toggle;
}

// Removes "buttonType" from the inspector - fixed by this class, not authorable. Everything
// else (text, groupNum, accelerator, checkboxStyle, ...) stays exactly as Parent registered it.
void NewGuiToggleButton::initPersistFields()
{
   Parent::initPersistFields();
   removeField("buttonType");
}
