//-----------------------------------------------------------------------------
// gui_rev2/controls/button_types/newGuiRadioButton.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIRADIOBUTTON_H_
#define _NEWGUIRADIOBUTTON_H_

#ifndef _NEWGUIBUTTON_H_
#include "gui_rev2/controls/newGuiButton.h"
#endif

/// A NewGuiButton fixed to ButtonType_Radio - editor/authoring convenience matching
/// NewGuiToggleButton's shape. groupNum is still authored per-instance (radio buttons in
/// different mutually-exclusive sets need different values), so that field is deliberately
/// NOT removed - only "buttonType" is, since that part is fixed by this class.
///
/// @code
/// new NewGuiRadioButton( OptionA ) { text = "Option A"; groupNum = 1; };
/// new NewGuiRadioButton( OptionB ) { text = "Option B"; groupNum = 1; };
/// @endcode
class NewGuiRadioButton : public NewGuiButton
{
public:

   typedef NewGuiButton Parent;

   NewGuiRadioButton();

   DECLARE_CONOBJECT(NewGuiRadioButton);

   static void initPersistFields();
};

#endif // _NEWGUIRADIOBUTTON_H_
