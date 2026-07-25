//-----------------------------------------------------------------------------
// linuxProcessControl.cpp — Linux implementation of the remaining
// process-control surface declared in platform.h: init/shutdown, debug
// break, debug string output, quit/force-shutdown, restart, single-instance
// exclusion, FPU/SSE control-word get/set, web browser launch, and a basic
// RNG.
//
// Entirely native (libc + POSIX only):
//   - excludeOtherInstances uses flock() on a lockfile under the user's
//     runtime directory, the standard native single-instance pattern on
//     Linux (no D-Bus/systemd dependency required).
//   - getMathControlState/setMathControlState manipulate the SSE MXCSR
//     register directly via _mm_getcsr/_mm_setcsr (a compiler intrinsic
//     backed by a single instruction, not an external library) — the
//     control word x86_64 Linux actually uses for floating point, mirroring
//     why win32ProcessControl.cpp moved off legacy x87 control-word asm.
//     On Arm64 there's no equivalent single control register in the same
//     sense, so these are no-ops there, matching macProcessControl.mm's
//     documented "nothing to do here" conclusion for non-x87 targets.
//   - openWebBrowser shells out to xdg-open, the freedesktop.org standard
//     "open this URL/file with whatever the user has configured" utility
//     present on effectively every Linux desktop; this is a fork/exec of a
//     system utility, not a linked library dependency.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformInput.h"
#include "console/console.h"
#include "core/util/journal/process.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <csignal>
#include <random>

#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>

#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
#include <xmmintrin.h>
#endif

namespace
{
    int sLockFd = -1;

    // Runs a command via fork/exec (not system()/popen(), so no shell is
    // invoked and no shell-quoting concerns arise) and returns immediately
    // without waiting — matching the "fire and forget" contract of
    // openWebBrowser/openFile/openFolder on other platforms.
    bool spawnDetached(const char* exe, const char* arg)
    {
        const pid_t pid = fork();
        if (pid < 0)
            return false;

        if (pid == 0)
        {
            // Child: detach stdio so the browser/file-manager's own
            // output doesn't interleave with the engine's console, then
            // exec. execlp searches PATH, matching how xdg-open is
            // normally invoked from a shell.
            const int devNull = open("/dev/null", O_RDWR);
            if (devNull >= 0)
            {
                dup2(devNull, STDIN_FILENO);
                dup2(devNull, STDOUT_FILENO);
                dup2(devNull, STDERR_FILENO);
            }
            execlp(exe, exe, arg, static_cast<char*>(nullptr));
            _exit(127); // exec failed
        }

        // Parent: reap the immediate child asynchronously so it doesn't
        // become a zombie, without blocking on the (potentially
        // long-lived, if it forks again internally) grandchild process.
        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

//-----------------------------------------------------------------------------
void Platform::postQuitMessage(const S32 in_quitVal)
{
    Process::requestShutdown();
}

void Platform::debugBreak()
{
    raise(SIGTRAP);
}

void Platform::forceShutdown(S32 returnValue)
{
    // Deliberately not _exit()/abort() — exit() still runs atexit
    // handlers and flushes stdio, matching win32's rationale for not
    // using ExitProcess (skips destructors / can leave state
    // inconsistent).
    exit(returnValue);
}

void Platform::outputDebugString(const char* string, ...)
{
    char buffer[2048];

    va_list args;
    va_start(args, string);
    dVsprintf(buffer, sizeof(buffer), string, args);
    va_end(args);

    U32 length = static_cast<U32>(std::strlen(buffer));
    if (length == sizeof(buffer) - 1)
        --length;

    buffer[length]     = '\n';
    buffer[length + 1] = '\0';

    fputs(buffer, stderr);
    fflush(stderr);
}

bool Platform::openWebBrowser(const char* webAddress)
{
    return spawnDetached("xdg-open", webAddress);
}

#ifndef TORQUE_SDL

void Platform::openFolder(const char* path)
{
    spawnDetached("xdg-open", path);
}

void Platform::openFile(const char* path)
{
    spawnDetached("xdg-open", path);
}

#endif

bool Platform::excludeOtherInstances(const char* mutexName)
{
    // Lockfile path under $XDG_RUNTIME_DIR (falls back to /tmp if unset,
    // e.g. when running headless outside a full desktop session) — the
    // native Linux equivalent of a Win32 named mutex / macOS
    // NSDistributedLock: flock() is an advisory kernel-level lock, held
    // only for this process's lifetime, and released automatically if
    // the process dies without cleaning up.
    const char* runtimeDir = getenv("XDG_RUNTIME_DIR");
    char path[512];
    dSprintf(path, sizeof(path), "%s/%s.lock", runtimeDir && *runtimeDir ? runtimeDir : "/tmp", mutexName);

    const int fd = open(path, O_CREAT | O_RDWR, 0600);
    if (fd < 0)
        return false;

    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        close(fd);
        return false;
    }

    sLockFd = fd; // Kept open (and thus locked) for the process's lifetime.
    return true;
}

bool Platform::checkOtherInstances(const char* mutexName)
{
    const char* runtimeDir = getenv("XDG_RUNTIME_DIR");
    char path[512];
    dSprintf(path, sizeof(path), "%s/%s.lock", runtimeDir && *runtimeDir ? runtimeDir : "/tmp", mutexName);

    const int fd = open(path, O_CREAT | O_RDWR, 0600);
    if (fd < 0)
        return false;

    const bool wouldBlock = flock(fd, LOCK_EX | LOCK_NB) != 0;
    if (!wouldBlock)
        flock(fd, LOCK_UN);
    close(fd);

    return wouldBlock; // true = another instance already holds the lock
}

void Platform::restartInstance()
{
    char exePath[4096];
    const ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len <= 0)
    {
        exit(0);
        return;
    }
    exePath[len] = '\0';

    const pid_t pid = fork();
    if (pid == 0)
    {
        execl(exePath, exePath, static_cast<char*>(nullptr));
        _exit(127);
    }

    exit(0);
}

//-----------------------------------------------------------------------------
// FPU/SSE control state (MXCSR on x86_64; no-op on Arm64 — see file header).
//-----------------------------------------------------------------------------
U32 Platform::getMathControlState()
{
#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
    return _mm_getcsr();
#else
    return 0;
#endif
}

void Platform::setMathControlState(U32 state)
{
#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
    _mm_setcsr(state);
#endif
}

void Platform::setMathControlStateKnown()
{
#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
    // Default MXCSR: all exceptions masked, round-to-nearest, flush-to-
    // zero/denormals-are-zero left at their power-on defaults (0) —
    // 0x1F80 is the standard SSE reset state, matching the intent of
    // win32's setMathControlStateKnown() for its supported x64 mask.
    _mm_setcsr(0x1F80);
#endif
}

//-----------------------------------------------------------------------------
F32 Platform::getRandom()
{
    static thread_local std::mt19937 engine{ std::random_device{}() };
    static thread_local std::uniform_real_distribution<F32> dist(0.0f, 1.0f);
    return dist(engine);
}

//-----------------------------------------------------------------------------
void Platform::init()
{
    Con::printf("Initializing platform...");

    Con::setVariable("$platform", "linux");

    Input::init();

    Con::printf("Done");
}

void Platform::shutdown()
{
    Input::destroy();

    if (sLockFd >= 0)
    {
        flock(sLockFd, LOCK_UN);
        close(sLockFd);
        sLockFd = -1;
    }
}
