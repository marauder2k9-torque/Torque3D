//-----------------------------------------------------------------------------
// gui_rev2/controls/button_types/newGuiToggleButton.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUITOGGLEBUTTON_H_
#define _NEWGUITOGGLEBUTTON_H_

#ifndef _NEWGUIBUTTON_H_
#include "gui_rev2/controls/newGuiButton.h"
#endif

/// A NewGuiButton fixed to ButtonType_Toggle - a plain latching button, distinct from
/// NewGuiCheckboxButton only in that it draws as an ordinary bordered button with centered
/// text rather than a checkbox glyph (see NewGuiButton::mCheckboxStyle). Exists purely as an
/// editor/authoring convenience: the constructor sets mButtonType once, and
/// initPersistFields() removes the "buttonType" field so the inspector doesn't offer a
/// dropdown for a value that's fixed. buttonType may still be changed in C++ via
/// setButtonType() if a subclass genuinely needs to (rare - at that point NewGuiButton
/// itself is probably the right base to author against instead).
///
/// @code
/// new NewGuiToggleButton( MuteButton )
/// {
///    text = "Mute";
/// };
/// @endcode
class NewGuiToggleButton : public NewGuiButton
{
public:

   typedef NewGuiButton Parent;

   NewGuiToggleButton();

   DECLARE_CONOBJECT(NewGuiToggleButton);

   static void initPersistFields();
};

#endif // _NEWGUITOGGLEBUTTON_H_
