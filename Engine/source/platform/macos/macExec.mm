//-----------------------------------------------------------------------------
// macExec.mm — macOS implementation of the shellExecute console function,
// mirroring win32Exec.cpp/linuxExec.cpp's ExecuteThread/
// ExecuteCleanupEvent shape: launches an external program on a
// background Thread, polls for completion without blocking the sim, and
// posts an ExecuteCleanupEvent back to the root group when the child
// exits so "onExecuteDone" fires on the main thread, matching Windows'
// contract exactly.
//
// Uses fork()+execvp() (real, native, dependency-free — part of
// libSystem, same as every other macOS file in this platform layer)
// rather than NSTask, which would pull in a Foundation/Objective-C
// dependency layer for something POSIX already provides directly. This
// mirrors linuxExec.cpp's approach closely (both are POSIX under the
// hood, including the fork()+chdir()-in-child pattern for applying the
// working directory).
//
// NOTE ON posix_spawn: an earlier revision of this file used
// posix_spawn() with posix_spawn_file_actions_addchdir_np() to apply the
// working directory without an explicit fork(). That symbol is a genuine
// portability hazard: it's absent from libSystem on macOS 10.14 and
// earlier (confirmed — there are real-world "dyld: Symbol not found:
// _posix_spawn_file_actions_addchdir_np" crash reports for binaries that
// assumed it was universally present), and this platform layer has no
// established minimum macOS deployment target to justify assuming it's
// always available. Plain fork()/chdir()/execvp() requires no
// version-gated symbol at all and is guaranteed present on every macOS
// version, so it's used here instead — matching linuxExec.cpp's own
// approach almost exactly (unlike Linux's version, this does NOT shell
// through /bin/sh -c: execvp() takes a real argv array directly, so args
// are split on whitespace here rather than needing a shell to do it).
//-----------------------------------------------------------------------------
#import "platform/platform.h"
#import "console/console.h"
#import "console/engineAPI.h"
#import "sim/simBase.h"
#import "platform/threads/thread.h"
#import "core/util/safeDelete.h"
#import "core/util/tVector.h"

#import <sys/wait.h>
#import <unistd.h>
#import <cstring>

//-----------------------------------------------------------------------------
// Thread for executing in
//-----------------------------------------------------------------------------

class ExecuteThread : public Thread
{
    // Matches win32Exec.cpp/linuxExec.cpp's own reasoning: mPid is only
    // touched in the constructor before the thread starts, and in the
    // thread itself — no mutex needed.
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

    // Split args on whitespace into a real argv array for execvp() — see
    // file header comment for why this differs from Linux's shell-based
    // approach. Simple split, no quoting support (matching this platform
    // layer's other simple-splitter, win32Main.cpp's own command-line
    // parsing, which is likewise deliberately simple).
    Vector<char*> argv;
    argv.push_back(dStrdup(exeBuf));

    if (args && *args)
    {
        char argsCopy[1024];
        dStrncpy(argsCopy, args, sizeof(argsCopy));
        argsCopy[sizeof(argsCopy) - 1] = '\0';

        for (char* word = strtok(argsCopy, " "); word; word = strtok(nullptr, " "))
            argv.push_back(dStrdup(word));
    }

    argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0)
    {
        mSpawnFailed = true;
        for (U32 i = 0; i < argv.size(); ++i)
            if (argv[i]) dFree(argv[i]);
        return;
    }

    if (pid == 0)
    {
        // Child: apply the working directory before exec — POSIX has no
        // way to pass this directly to execvp(), so it's applied here,
        // matching linuxExec.cpp's identical approach exactly.
        if (directory && *directory)
        {
            if (chdir(directory) != 0)
            {
                Con::warnf("ExecuteThread: could not chdir to '%s', launching in current directory instead", directory);
            }
        }

        execvp(exeBuf, argv.address());
        _exit(127); // exec failed
    }

    // Parent.
    for (U32 i = 0; i < argv.size(); ++i)
        if (argv[i]) dFree(argv[i]);

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
    // call, matching win32's 200ms WaitForSingleObject polling loop and
    // linuxExec.cpp's identical approach — see linuxExec.cpp's header
    // comment for why polling stands in for Windows' direct
    // wait-with-timeout API on POSIX.
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
            exited = true;
            ok = false;
            break;
        }

        usleep(200 * 1000); // 200ms, matching win32/linux's own poll interval
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
