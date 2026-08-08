//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiConsole.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUICONSOLE_H_
#define _NEWGUICONSOLE_H_

#ifndef _NEWGUISTACK_H_
#include "gui_rev2/controls/newGuiStack.h"
#endif
#ifndef _CONSOLE_LOGGER_H_
#include "console/consoleLogger.h"
#endif

class NewGuiConsoleLine;

/// On-screen console output: a NewGuiStack (vertical) that owns one
/// NewGuiConsoleLine child per visible log entry. Meant to sit inside a
/// NewGuiScroll the same way any other tall content would, authored
/// exactly like the @code example on NewGuiStack itself - there is
/// deliberately no bespoke scrolling/cell-position code here at all,
/// unlike the legacy GuiConsole (GuiArrayCtrl subclass, hand-rolled
/// mCellSize/onRenderCell/scrollCellVisible bookkeeping). Real child
/// controls, laid out by a layout class every other part of this system
/// already uses and already trusts, replace all of that.
///
/// @code
/// new NewGuiScroll()
/// {
///    width = "100%"; height = "300"; scrollBarMode = "vertical";
///    new NewGuiConsole( MyConsole )
///    {
///       width = "100%"; height = "auto";
///       maxLines = "1024";
///    };
/// };
/// @endcode
///
/// New log lines arrive via a Con::addConsumer() callback
class NewGuiConsole : public NewGuiStack
{
public:

   typedef NewGuiStack Parent;

protected:

   bool mDisplayErrors;
   bool mDisplayWarnings;
   bool mDisplayNormalMessages;

   S32 mMaxLines;   ///< Authored - oldest line is destroyed once exceeded. See this class's own doc comment.

   /// One raw log line captured by the Con::addConsumer() callback,
   /// queued for the main thread to actually turn into a child control.
   /// Deliberately just the two pieces of data the callback receives -
   /// nothing else is safe to touch off the main thread.
   struct PendingLine
   {
      ConsoleLogEntry::Level level;
      String text;
   };

   /// Appended to by the Con::addConsumer() callback (any thread),
   /// drained by _tick() (main thread only).
   Vector<PendingLine> mPendingLines;

   /// Guards mPendingLines.
   void* mQueueMutex;

   /// True once this instance has registered itself with Process::notify()
   bool mTickRegistered;

   /// The consumer function registered with Con::addConsumer() -
   /// forwards straight to whichever NewGuiConsole instance(s) are
   /// currently in the Sim (see smActiveConsoles).
   static void _consumerCallback(U32 level, const char* line);

   /// Every currently-added (onAdd() called, onRemove() not yet called)
   /// NewGuiConsole - the consumer callback is process-global
   /// (Con::addConsumer has no per-instance "context" parameter), so it
   /// needs a way to reach every listening instance. Mirrors
   /// NewGuiCanvas::smAllCanvases's existing shape/reasoning exactly
   /// (see newGuiCanvas.h) rather than inventing a new pattern.
   static Vector<NewGuiConsole*> smActiveConsoles;

   /// Registered with Process::notify() in onAdd() - drains
   /// mPendingLines into real child controls via _appendLine(), once
   /// per tick, on the main thread.
   void _tick();

   /// Turns one already-dequeued PendingLine into a real
   /// NewGuiConsoleLine child, applying current filters to its initial
   /// visibility, then evicts the oldest child if mMaxLines is now
   /// exceeded. The only place children are added or removed after
   /// initial construction.
   void _appendLine(ConsoleLogEntry::Level level, const char* text);

   /// Re-applies mDisplay* to the CURRENT visibility of every existing
   /// child (does not create/destroy any) - see setDisplayFilters().
   void _reapplyFilters();

public:

   NewGuiConsole();
   virtual ~NewGuiConsole();

   DECLARE_CONOBJECT(NewGuiConsole);

   static void initPersistFields();

   /// Registers the log consumer (once, process-wide - see
   /// smActiveConsoles) and this instance's Process::notify() tick.
   bool onAdd() override;

   /// Unregisters this instance's tick, and the log consumer once no
   /// NewGuiConsole is left listening.
   void onRemove() override;

   /// Matches every already-created line's visibility to the new
   /// filter set.
   void setDisplayFilters(bool errors, bool warnings, bool normal);

   bool getErrorFilter() const { return mDisplayErrors; }
   bool getWarningFilter() const { return mDisplayWarnings; }
   bool getNormalFilter() const { return mDisplayNormalMessages; }

   void toggleErrorFilter() { setDisplayFilters(!mDisplayErrors, mDisplayWarnings, mDisplayNormalMessages); }
   void toggleWarningFilter() { setDisplayFilters(mDisplayErrors, !mDisplayWarnings, mDisplayNormalMessages); }
   void toggleNormalFilter() { setDisplayFilters(mDisplayErrors, mDisplayWarnings, !mDisplayNormalMessages); }

   /// Destroys every line currently shown.
   void clear();
};

#endif // _NEWGUICONSOLE_H_
