//-----------------------------------------------------------------------------
// linuxExec.cpp — Linux implementation of the shellExecute console
// function, mirroring winExec.cpp's ExecuteThread/ExecuteCleanupEvent
// shape: launches an external program on a background Thread (fork/exec,
// not ShellExecuteEx), polls for completion without blocking the sim, and
// posts an ExecuteCleanupEvent back to the root group when the child
// exits so "onExecuteDone" fires on the main thread, matching Windows'
// contract exactly.
//
// Fully native (libc + POSIX only): fork()/execvp() to launch, waitpid()
// with WNOHANG polled from the background thread to detect completion
// without blocking (no direct process-completion notification API exists
// on POSIX the way WaitForSingleObject provides on Windows, so polling is
// the correct native equivalent here — same pattern this platform layer
// already uses in linuxProcessControl.cpp's spawnDetached, but this
// version tracks completion instead of firing-and-forgetting).
//
// "directory" (working directory for the launched process) is applied by
// the child via chdir() after fork(), before exec() — POSIX has no
// equivalent of SHELLEXECUTEINFO::lpDirectory to pass directly to a spawn
// call, so this is the standard native way to achieve the same effect.
//
// "args" is passed as a single opaque argument string on Windows (via
// SHELLEXECUTEINFO::lpParameters, which the shell itself re-splits) — to
// match that exact contract (rather than inventing our own quoting/
// splitting rules), this shells the executable+args through
// /bin/sh -c "<executable> <args>" exactly once, letting the system shell
// do the same kind of splitting a native double-click/ShellExecute launch
// would. This is a deliberate, narrow exception to this platform layer's
// general "avoid invoking a shell" rule (see linuxProcessControl.cpp's
// spawnDetached, which explicitly avoids a shell) — shellExecute's own
// contract, unlike openWebBrowser/openFolder/openFile, is inherently
// "run this command line", not "open this single path", so a shell is
// the correct tool for matching that contract on this platform.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "sim/simBase.h"
#include "platform/threads/thread.h"
#include "core/util/safeDelete.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdio>

//-----------------------------------------------------------------------------
// Thread for executing in
//-----------------------------------------------------------------------------

class ExecuteThread : public Thread
{
    // Matches winExec.cpp's own reasoning: mPid is only touched in the
    // constructor before the thread starts, and in the thread itself —
    // no mutex needed.
    pid_t mPid;
    bool  mSpawnFailed;

public:
    ExecuteThread(const char *executable, const char *args = nullptr, const char *directory = nullptr);

    void run(void *arg = 0) override;
};

//-----------------------------------------------------------------------------
// Event for cleanup
//-----------------------------------------------------------------------------

class ExecuteCleanupEvent : public SimEvent
{
    ExecuteThread *mThread;
    bool mOK;

public:
    ExecuteCleanupEvent(ExecuteThread *thread, bool ok)
    {
        mThread = thread;
        mOK = ok;
    }

    void process(SimObject *object) override
    {
        if( Con::isFunction( "onExecuteDone" ) )
            Con::executef( "onExecuteDone", Con::getIntArg( mOK ) );
        SAFE_DELETE(mThread);
    }
};

//-----------------------------------------------------------------------------

ExecuteThread::ExecuteThread(const char *executable, const char *args /* = nullptr */, const char *directory /* = nullptr */)
    : Thread(0, NULL, false), mPid(-1), mSpawnFailed(false)
{
    char exeBuf[1024];
    Platform::makeFullPathName(executable, exeBuf, sizeof(exeBuf));

    // Build a single command line for /bin/sh -c — see file header
    // comment for why a shell is used here specifically (matching
    // SHELLEXECUTEINFO::lpParameters' own "opaque argument string that
    // gets re-split by the shell" contract), unlike the rest of this
    // platform layer which deliberately avoids invoking a shell.
    char commandLine[2048];
    if (args && *args)
        dSprintf(commandLine, sizeof(commandLine), "\"%s\" %s", exeBuf, args);
    else
        dSprintf(commandLine, sizeof(commandLine), "\"%s\"", exeBuf);

    const pid_t pid = fork();
    if (pid < 0)
    {
        mSpawnFailed = true;
        return;
    }

    if (pid == 0)
    {
        // Child: apply the working directory (POSIX's equivalent of
        // SHELLEXECUTEINFO::lpDirectory — there's no way to pass this
        // directly to a spawn call, so it's applied here, before exec).
        if (directory && *directory)
        {
            if (chdir(directory) != 0)
            {
                // Matches ShellExecuteEx's own behavior of still
                // attempting the launch even if some auxiliary part of
                // the request is questionable — a bad/missing directory
                // shouldn't silently abandon the whole launch.
                Con::warnf("ExecuteThread: could not chdir to '%s', launching in current directory instead", directory);
            }
        }

        execl("/bin/sh", "sh", "-c", commandLine, static_cast<char*>(nullptr));
        _exit(127); // exec failed
    }

    // Parent.
    mPid = pid;
    start();
}

void ExecuteThread::run(void *arg /* = 0 */)
{
    if (mSpawnFailed || mPid <= 0)
    {
        Sim::postEvent(Sim::getRootGroup(), new ExecuteCleanupEvent(this, false), -1);
        return;
    }

    // Poll for completion with WNOHANG rather than a blocking waitpid()
    // call, so checkForStop() can still be honored if the engine is
    // shutting down while this thread is still waiting — mirroring
    // win32's own 200ms WaitForSingleObject polling loop exactly, just
    // with POSIX's non-blocking waitpid() standing in for
    // WaitForSingleObject's timeout parameter (POSIX has no direct
    // "wait with timeout" call for a specific pid the way Windows does).
    int status = 0;
    bool exited = false;
    bool ok = false;

    while (!checkForStop())
    {
        const pid_t result = waitpid(mPid, &status, WNOHANG);
        if (result == mPid)
        {
            exited = true;
            ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            break;
        }
        if (result < 0)
        {
            // ECHILD or similar — the process is gone and we can't reap
            // it (e.g. already reaped elsewhere); treat as completed,
            // status unknown.
            exited = true;
            ok = false;
            break;
        }

        usleep(200 * 1000); // 200ms, matching win32's own poll interval
    }

    Sim::postEvent(Sim::getRootGroup(), new ExecuteCleanupEvent(this, exited && ok), -1);
}

//-----------------------------------------------------------------------------
// Console Functions
//-----------------------------------------------------------------------------

DefineEngineFunction( shellExecute, bool, (const char * executable, const char * args, const char * directory), ("", ""), "(string executable, string args, string directory)"
                "@brief Launches an outside executable or batch file\n\n"
                "@param executable Name of the executable or batch file\n"
                "@param args Optional list of arguments, in string format, to pass to the executable\n"
                "@param directory Optional string containing path to output or shell\n"
                "@return true if executed, false if not\n"
                "@ingroup Platform")
{
    ExecuteThread *et = new ExecuteThread( executable, args, directory );
    if(! et->isAlive())
    {
        delete et;
        return false;
    }

    return true;
}
