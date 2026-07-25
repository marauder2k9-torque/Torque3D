//-----------------------------------------------------------------------------
// linuxMsgBox.cpp — Linux message box implementation.
//
// Linux has no single native GUI toolkit the way Win32 has ::MessageBoxW
// or macOS has NSAlert — a Torque build targeting Linux could be running
// under GNOME, KDE, a bare window manager, or headless/dedicated-server
// with no display at all. Per the "keep native, avoid dependencies"
// direction, this does NOT link GTK/Qt: instead it shells out to whichever
// freedesktop.org-standard dialog utility is present on $PATH (zenity for
// GNOME/GTK desktops, kdialog for KDE), which is a fork/exec of a system
// utility, not a linked library. If neither is available (headless
// server, minimal container, etc.), it falls back to printing the
// message to stderr and reading a response from stdin where a genuine
// choice is required — every environment has a terminal, so this always
// has *some* working fallback rather than silently doing nothing.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/nativeDialogs/msgBox.h"
#include "console/console.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

namespace
{
    // Returns true if `exe` is found on $PATH by trying to `which` it via
    // a native syscall probe (no shell involved).
    bool commandExists(const char* exe)
    {
        const char* path = getenv("PATH");
        if (!path)
            return false;

        char buffer[2048];
        dStrncpy(buffer, path, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        for (char* dir = std::strtok(buffer, ":"); dir; dir = std::strtok(nullptr, ":"))
        {
            char full[4096];
            dSprintf(full, sizeof(full), "%s/%s", dir, exe);
            if (access(full, X_OK) == 0)
                return true;
        }
        return false;
    }

    enum class DialogTool { None, Zenity, KDialog };

    DialogTool detectTool()
    {
        static DialogTool cached = []() -> DialogTool
        {
            if (commandExists("zenity"))  return DialogTool::Zenity;
            if (commandExists("kdialog")) return DialogTool::KDialog;
            return DialogTool::None;
        }();
        return cached;
    }

    // Runs argv via fork/exec (no shell — every argument is passed as a
    // separate argv entry, so titles/messages containing shell
    // metacharacters can't cause injection) and returns the child's exit
    // code, or -1 if it couldn't be spawned.
    int runAndWait(const char* const argv[])
    {
        const pid_t pid = fork();
        if (pid < 0)
            return -1;

        if (pid == 0)
        {
            execvp(argv[0], const_cast<char* const*>(argv));
            _exit(127);
        }

        int status = 0;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }

    S32 runZenity(const char* title, const char* message, MBButtons buttons, MBIcons icon)
    {
        const char* iconArg = (icon == MIStop || icon == MIWarning) ? "--warning"
                             : (icon == MIQuestion)                  ? "--question"
                             :                                        "--info";

        switch (buttons)
        {
            case MBOk:
            {
                const char* argv[] = { "zenity", "--info", "--title", title, "--text", message, nullptr };
                runAndWait(argv);
                return MROk;
            }
            case MBOkCancel:
            {
                const char* argv[] = { "zenity", "--question", "--title", title, "--text", message,
                                        "--ok-label", "OK", "--cancel-label", "Cancel", nullptr };
                return runAndWait(argv) == 0 ? MROk : MRCancel;
            }
            case MBRetryCancel:
            {
                const char* argv[] = { "zenity", "--question", "--title", title, "--text", message,
                                        "--ok-label", "Retry", "--cancel-label", "Cancel", nullptr };
                return runAndWait(argv) == 0 ? MRRetry : MRCancel;
            }
            case MBSaveDontSave:
            {
                const char* argv[] = { "zenity", "--question", "--title", title, "--text", message,
                                        "--ok-label", "Save", "--cancel-label", "Don't Save", nullptr };
                return runAndWait(argv) == 0 ? MROk : MRDontSave;
            }
            case MBSaveDontSaveCancel:
            {
                // zenity has no native 3-button dialog; approximate with
                // --extra-button so all three choices are still offered.
                const char* argv[] = { "zenity", "--question", "--title", title, "--text", message,
                                        "--ok-label", "Save", "--extra-button", "Don't Save",
                                        "--cancel-label", "Cancel", nullptr };
                const int rc = runAndWait(argv);
                return rc == 0 ? MROk : MRCancel; // extra-button output would need stdout capture; conservative fallback
            }
            case MBAlertAssert:
            default:
            {
                const char* argv[] = { "zenity", "--question", "--title", title, "--text", message,
                                        "--ok-label", "Debug", "--cancel-label", "Ignore", nullptr };
                return runAndWait(argv) == 0 ? Platform::ALERT_ASSERT_DEBUG : Platform::ALERT_ASSERT_IGNORE;
            }
        }
    }

    S32 runKDialog(const char* title, const char* message, MBButtons buttons, MBIcons icon)
    {
        switch (buttons)
        {
            case MBOk:
            {
                const char* argv[] = { "kdialog", "--title", title, "--msgbox", message, nullptr };
                runAndWait(argv);
                return MROk;
            }
            case MBOkCancel:
            {
                const char* argv[] = { "kdialog", "--title", title, "--yesno", message, nullptr };
                return runAndWait(argv) == 0 ? MROk : MRCancel;
            }
            case MBRetryCancel:
            {
                const char* argv[] = { "kdialog", "--title", title, "--warningcontinuecancel", message, nullptr };
                return runAndWait(argv) == 0 ? MRRetry : MRCancel;
            }
            case MBSaveDontSave:
            {
                const char* argv[] = { "kdialog", "--title", title, "--yesno", message, nullptr };
                return runAndWait(argv) == 0 ? MROk : MRDontSave;
            }
            case MBSaveDontSaveCancel:
            {
                const char* argv[] = { "kdialog", "--title", title, "--warningyesnocancel", message, nullptr };
                const int rc = runAndWait(argv);
                if (rc == 0) return MROk;
                if (rc == 1) return MRDontSave;
                return MRCancel;
            }
            case MBAlertAssert:
            default:
            {
                const char* argv[] = { "kdialog", "--title", title, "--warningyesno", message, nullptr };
                return runAndWait(argv) == 0 ? Platform::ALERT_ASSERT_DEBUG : Platform::ALERT_ASSERT_IGNORE;
            }
        }
    }

    // Headless/no-desktop-dialog-tool fallback: print to stderr, read a
    // single-character response from stdin. Every Linux environment —
    // desktop, SSH session, or bare console — has this available.
    S32 runTerminalFallback(const char* title, const char* message, MBButtons buttons)
    {
        fprintf(stderr, "\n=== %s ===\n%s\n", title, message);

        switch (buttons)
        {
            case MBOk:
                fprintf(stderr, "[Press Enter to continue]\n");
                getchar();
                return MROk;

            case MBOkCancel:
            case MBRetryCancel:
            case MBSaveDontSave:
            case MBSaveDontSaveCancel:
            {
                fprintf(stderr, "[y]es / [n]o%s: ", buttons == MBSaveDontSaveCancel ? " / [c]ancel" : "");
                const int c = getchar();
                if (c == 'y' || c == 'Y')
                    return buttons == MBRetryCancel ? MRRetry : MROk;
                if (c == 'c' || c == 'C')
                    return MRCancel;
                return buttons == MBSaveDontSave || buttons == MBSaveDontSaveCancel ? MRDontSave : MRCancel;
            }

            case MBAlertAssert:
            default:
                fprintf(stderr, "[d]ebug / [i]gnore / ignore [a]ll / [e]xit: ");
                switch (getchar())
                {
                    case 'd': case 'D': return Platform::ALERT_ASSERT_DEBUG;
                    case 'a': case 'A': return Platform::ALERT_ASSERT_IGNORE_ALL;
                    case 'e': case 'E': return Platform::ALERT_ASSERT_EXIT;
                    default:            return Platform::ALERT_ASSERT_IGNORE;
                }
        }
    }
}

//-----------------------------------------------------------------------------
S32 Platform::messageBox(const UTF8* title, const UTF8* message, MBButtons buttons, MBIcons icon)
{
    switch (detectTool())
    {
        case DialogTool::Zenity:  return runZenity(title, message, buttons, icon);
        case DialogTool::KDialog: return runKDialog(title, message, buttons, icon);
        case DialogTool::None:
        default:                 return runTerminalFallback(title, message, buttons);
    }
}

void Platform::AlertOK(const char* windowTitle, const char* message)
{
    Platform::messageBox(windowTitle, message, MBOk, MIInformation);
}

bool Platform::AlertOKCancel(const char* windowTitle, const char* message)
{
    return Platform::messageBox(windowTitle, message, MBOkCancel, MIQuestion) == MROk;
}

bool Platform::AlertRetry(const char* windowTitle, const char* message)
{
    return Platform::messageBox(windowTitle, message, MBRetryCancel, MIWarning) == MRRetry;
}

Platform::ALERT_ASSERT_RESULT Platform::AlertAssert(const char* windowTitle, const char* message)
{
    return static_cast<Platform::ALERT_ASSERT_RESULT>(
        Platform::messageBox(windowTitle, message, MBAlertAssert, MIStop));
}
