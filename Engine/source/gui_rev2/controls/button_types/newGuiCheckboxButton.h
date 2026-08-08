//-----------------------------------------------------------------------------
// gui_rev2/controls/button_types/newGuiCheckboxButton.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUICHECKBOXBUTTON_H_
#define _NEWGUICHECKBOXBUTTON_H_

#ifndef _NEWGUIBUTTON_H_
#include "gui_rev2/controls/newGuiButton.h"
#endif

/// A NewGuiButton fixed to ButtonType_Toggle with mCheckboxStyle enabled - a plain checkbox.
/// Editor/authoring convenience matching NewGuiToggleButton/NewGuiRadioButton's shape; removes
/// both "buttonType" and "checkboxStyle" from the inspector since this class fixes both.
///
/// @code
/// new NewGuiCheckboxButton( EnableFogCheckbox ) { text = "Enable Fog"; };
/// @endcode
class NewGuiCheckboxButton : public NewGuiButton
{
public:

   typedef NewGuiButton Parent;

   NewGuiCheckboxButton();

   DECLARE_CONOBJECT(NewGuiCheckboxButton);

   static void initPersistFields();
};

#endif // _NEWGUICHECKBOXBUTTON_H_
