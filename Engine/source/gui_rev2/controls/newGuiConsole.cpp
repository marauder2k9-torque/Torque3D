//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiConsole.cpp
//-----------------------------------------------------------------------------
// mQueueMutex's lock/unlock calls below are written against Torque's
// conventional platform/threads/mutex.h (Mutex::createMutex()/lockMutex()/
// unlockMutex()/destroyMutex()). Unlike almost everything else in this
// file, that API has no confirmed usage anywhere else in this project's
// files to cross-check against - the same category of gap flagged for
// Stream/FileStream in newLangTable.cpp. Worth a compile check before
// relying on this.
//
// _tick() is registered with PROCESS_RENDER_ORDER (the only
// Process::notify() order constant with any confirmed usage in this
// project - see NewGuiCanvas::setAutoRender()) rather than a dedicated
// "logic tick" order, since no such constant has confirmed usage here
// either. This runs the drain at the same priority tier as the canvas's
// own render tick; ordering relative to that tick within the tier is
// NOT independently confirmed - worth checking that a newly-added line
// is measured/arranged the same frame it's created, rather than one
// frame late, once this compiles.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/threads/mutex.h"
#include "core/util/journal/process.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiConsole.h"
#include "gui_rev2/controls/newGuiConsoleLine.h"

IMPLEMENT_CONOBJECT(NewGuiConsole);

Vector<NewGuiConsole*> NewGuiConsole::smActiveConsoles;

NewGuiConsole::NewGuiConsole()
   : mDisplayErrors(true),
   mDisplayWarnings(true),
   mDisplayNormalMessages(true),
   mMaxLines(0),
   mQueueMutex(NULL),
   mTickRegistered(false)
{
   mAxis = StackAxis_Vertical;
   mQueueMutex = Mutex::createMutex();
}

NewGuiConsole::~NewGuiConsole()
{
   if (mQueueMutex)
      Mutex::destroyMutex(mQueueMutex);
}

void NewGuiConsole::initPersistFields()
{
   addGroup("NewGuiConsole");

   ADD_FIELD("displayErrors", TypeBool, Offset(mDisplayErrors, NewGuiConsole))
      .doc("Whether Error-severity lines are shown. Affects only NEW lines - use setDisplayFilters() "
         "from script to also re-apply visibility to already-created lines.");

   ADD_FIELD("displayWarnings", TypeBool, Offset(mDisplayWarnings, NewGuiConsole))
      .doc("Whether Warning-severity lines are shown. See displayErrors's doc comment.");

   ADD_FIELD("displayNormalMessages", TypeBool, Offset(mDisplayNormalMessages, NewGuiConsole))
      .doc("Whether Normal-severity lines are shown. See displayErrors's doc comment.");

   ADD_FIELD("maxLines", TypeS32, Offset(mMaxLines, NewGuiConsole))
      .doc("Oldest displayed line is destroyed once this many lines exist (regardless of filter - "
         "see this class's own header doc comment). 0 or negative means unlimited.");

   endGroup("NewGuiConsole");

   Parent::initPersistFields();
}

bool NewGuiConsole::onAdd()
{
   if (!Parent::onAdd())
      return false;

   if (smActiveConsoles.empty())
      Con::addConsumer(_consumerCallback);

   smActiveConsoles.push_back(this);

   if (!mTickRegistered)
   {
      Process::notify(this, &NewGuiConsole::_tick, PROCESS_RENDER_ORDER);
      mTickRegistered = true;
   }

   return true;
}

void NewGuiConsole::onRemove()
{
   if (mTickRegistered)
   {
      Process::remove(this, &NewGuiConsole::_tick);
      mTickRegistered = false;
   }

   for (U32 i = 0; i < smActiveConsoles.size(); i++)
   {
      if (smActiveConsoles[i] == this)
      {
         smActiveConsoles.erase(i);
         break;
      }
   }

   if (smActiveConsoles.empty())
      Con::removeConsumer(_consumerCallback);

   Parent::onRemove();
}

void NewGuiConsole::_consumerCallback(U32 level, const char* line)
{
   // May run on any thread (see this file's top-of-file comment and
   // win32Console.cpp's identical caution) - every active console just
   // queues the line; nothing here touches the GUI tree.
   for (U32 i = 0; i < smActiveConsoles.size(); i++)
   {
      NewGuiConsole* console = smActiveConsoles[i];

      Mutex::lockMutex(console->mQueueMutex);

      PendingLine pending;
      pending.level = (ConsoleLogEntry::Level)level;
      pending.text = line;
      console->mPendingLines.push_back(pending);

      Mutex::unlockMutex(console->mQueueMutex);
   }
}

void NewGuiConsole::_tick()
{
   // Copy the queue out under the lock, then clear it and do the
   // (potentially many lines') worth of actual control-creation work
   // outside the lock - so a burst of logging from another thread is
   // never blocked waiting on this tick's GUI work to finish. (Not
   // Vector::swap() - no confirmed use of that call anywhere else in
   // this project to check the signature against; a plain copy+clear
   // only relies on operations already proven elsewhere in this codebase.)
   Vector<PendingLine> drained;

   Mutex::lockMutex(mQueueMutex);
   if (!mPendingLines.empty())
   {
      drained = mPendingLines;
      mPendingLines.clear();
   }
   Mutex::unlockMutex(mQueueMutex);

   for (U32 i = 0; i < drained.size(); i++)
      _appendLine(drained[i].level, drained[i].text.c_str());
}

void NewGuiConsole::_appendLine(ConsoleLogEntry::Level level, const char* text)
{
   bool shouldShow =
      (level == ConsoleLogEntry::Error && mDisplayErrors) ||
      (level == ConsoleLogEntry::Warning && mDisplayWarnings) ||
      (level == ConsoleLogEntry::Normal && mDisplayNormalMessages);

   NewGuiConsoleLine* newLine = new NewGuiConsoleLine();
   newLine->registerObject();

   // Hand this line the SAME style asset authored on the console itself
   // (see NewGuiConsole's own style field) - a NewGuiConsoleLine is never
   // authored in a .gui/.cs file (it only ever exists via `new
   // NewGuiConsoleLine()` here), so it has no other way to pick up a
   // style. Without this, mStyle stays NULL, and per NewGuiStyle::Cascade()
   // ("if (!ownStyle) { ...; return result; }") a control with no own
   // style just inherits the parent's ALREADY-RESOLVED style outright -
   // no per-instance state-mask rule (stateMask = "checked"/"error") is
   // ever consulted, because there's no ownStyle object on the line
   // itself to search for matching child rules. That's what made every
   // line render in the same color regardless of severity: every line's
   // resolved textColor was just the console container's own resolved
   // color, and the console container's OWN computeStateMask() (its own
   // hover/active/etc, nothing to do with any individual line's
   // severity) is irrelevant here. Giving each line its own ownStyle
   // pointer makes Cascade() evaluate that line's OWN computeStateMask()
   // (which correctly reflects THIS line's mChecked/mHasError, set by
   // setLogEntry() below) against TestGui_ConsoleLineStyle's "checked"/
   // "error" child rules, same as any other control.
   newLine->setStyleAsset(getStyleAsset());

   newLine->setLogEntry(level, text);
   newLine->setVisible(shouldShow);

   addObject(newLine);

   if (mMaxLines > 0 && (S32)size() > mMaxLines)
   {
      SimSet::iterator itr = begin();
      if (itr != end())
      {
         SimObject* oldest = *itr;
         removeObject(oldest);
         oldest->deleteObject();
      }
   }
}

void NewGuiConsole::_reapplyFilters()
{
   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiConsoleLine* line = dynamic_cast<NewGuiConsoleLine*>(*itr);
      if (!line)
         continue;

      bool shouldShow =
         (line->getLevel() == ConsoleLogEntry::Error && mDisplayErrors) ||
         (line->getLevel() == ConsoleLogEntry::Warning && mDisplayWarnings) ||
         (line->getLevel() == ConsoleLogEntry::Normal && mDisplayNormalMessages);

      line->setVisible(shouldShow);
   }
}

void NewGuiConsole::setDisplayFilters(bool errors, bool warnings, bool normal)
{
   mDisplayErrors = errors;
   mDisplayWarnings = warnings;
   mDisplayNormalMessages = normal;

   _reapplyFilters();
}

void NewGuiConsole::clear()
{
   while (size() > 0)
   {
      SimSet::iterator itr = begin();
      SimObject* child = *itr;
      removeObject(child);
      child->deleteObject();
   }
}

//-----------------------------------------------------------------------------
// Console methods
//-----------------------------------------------------------------------------

DefineEngineMethod(NewGuiConsole, setDisplayFilters, void, (bool errors, bool warnings, bool normal), (true, true, true),
   "Sets which severities are currently shown. Already-created lines are shown/hidden to match; "
   "nothing is destroyed or re-created.\n"
   "@ingroup GuiCore")
{
   object->setDisplayFilters(errors, warnings, normal);
}

DefineEngineMethod(NewGuiConsole, getErrorFilter, bool, (), , "@ingroup GuiCore")
{
   return object->getErrorFilter();
}

DefineEngineMethod(NewGuiConsole, getWarningFilter, bool, (), , "@ingroup GuiCore")
{
   return object->getWarningFilter();
}

DefineEngineMethod(NewGuiConsole, getNormalFilter, bool, (), , "@ingroup GuiCore")
{
   return object->getNormalFilter();
}

DefineEngineMethod(NewGuiConsole, toggleErrorFilter, void, (), , "@ingroup GuiCore")
{
   object->toggleErrorFilter();
}

DefineEngineMethod(NewGuiConsole, toggleWarningFilter, void, (), , "@ingroup GuiCore")
{
   object->toggleWarningFilter();
}

DefineEngineMethod(NewGuiConsole, toggleNormalFilter, void, (), , "@ingroup GuiCore")
{
   object->toggleNormalFilter();
}

DefineEngineMethod(NewGuiConsole, clear, void, (), ,
   "Destroys every line currently shown. The underlying console log itself is untouched.\n"
   "@ingroup GuiCore")
{
   object->clear();
}
