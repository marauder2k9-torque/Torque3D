//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiConsoleLine.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUICONSOLELINE_H_
#define _NEWGUICONSOLELINE_H_

#ifndef _NEWGUILABEL_H_
#include "gui_rev2/controls/newGuiLabel.h"
#endif
#ifndef _CONSOLE_LOGGER_H_
#include "console/consoleLogger.h"
#endif

/// One line of console output - a plain NewGuiLabel whose only job is to
/// mark itself with the right EXISTING interaction state for its
/// severity, so it can be skinned through the ordinary style cascade:
///
///    Error   -> setHasError(true)   (NewGuiState_Error, already exists)
///    Warning -> setChecked(true)    (NewGuiState_Checked, REUSED - see below)
///    Normal  -> neither set
///
/// Warning deliberately reuses NewGuiState_Checked rather than adding a
/// new NewGuiState_Warning bit.
///
/// A style author skins severity exactly like any other state:
///
/// @code
/// new NewGuiStyle( ConsoleLineStyle )
/// {
///    textColor = "220 220 220 255";
///    new NewGuiStyle() { stateMask = "checked"; textColor = "230 190 60 255"; };  // Warning
///    new NewGuiStyle() { stateMask = "error";   textColor = "230 70 70 255"; };   // Error
/// };
/// @endcode
///
/// Owned and created exclusively by NewGuiConsole - never authored
/// directly in a GUI file, the same way legacy GuiConsole's cells were
/// never separately authored objects.
class NewGuiConsoleLine : public NewGuiLabel
{
public:

   typedef NewGuiLabel Parent;

protected:

   ConsoleLogEntry::Level mLevel;   ///< Fixed at construction (see NewGuiConsole::_appendLine()) - a line's severity never changes after it's created.

public:

   NewGuiConsoleLine();

   DECLARE_CONOBJECT(NewGuiConsoleLine);

   /// Sets the displayed text AND the severity used to pick this line's
   /// state (via setHasError()/setChecked() - see this class's own doc
   /// comment for why Warning maps to setChecked()) - always called
   /// together (see NewGuiConsole::_appendLine()), so a line's color and
   /// level can never disagree.
   void setLogEntry(ConsoleLogEntry::Level level, const char* text);

   ConsoleLogEntry::Level getLevel() const { return mLevel; }
};

#endif // _NEWGUICONSOLELINE_H_
