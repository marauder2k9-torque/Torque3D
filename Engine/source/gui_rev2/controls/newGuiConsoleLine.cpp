//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiConsoleLine.cpp
//-----------------------------------------------------------------------------
#include "gui_rev2/controls/newGuiConsoleLine.h"

IMPLEMENT_CONOBJECT(NewGuiConsoleLine);

NewGuiConsoleLine::NewGuiConsoleLine()
   : mLevel(ConsoleLogEntry::Normal)
{
   mWidth = NewGuiDimension::fromPercent(100.0f);
   mText.setWrap(true);
}

void NewGuiConsoleLine::setLogEntry(ConsoleLogEntry::Level level, const char* text)
{
   mLevel = level;

   // Warning -> setChecked()
   setChecked(level == ConsoleLogEntry::Warning);
   setHasError(level == ConsoleLogEntry::Error);

   setText(text);
}
