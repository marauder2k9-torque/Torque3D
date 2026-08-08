//-----------------------------------------------------------------------------
// gui_rev2/controls/button_types/newGuiRadioButton.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/button_types/newGuiRadioButton.h"

IMPLEMENT_CONOBJECT(NewGuiRadioButton);

NewGuiRadioButton::NewGuiRadioButton()
{
   mButtonType = ButtonType_Radio;
}

// Removes only "buttonType" - groupNum stays authorable (see class doc comment).
void NewGuiRadioButton::initPersistFields()
{
   Parent::initPersistFields();
   removeField("buttonType");
}
