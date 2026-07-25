//-----------------------------------------------------------------------------
// macMsgBox.mm — macOS message box implementation.
//
// Genuinely new — no mac message-box implementation existed anywhere in
// the original uploads (grep across every mac*.mm/.h file turned up
// nothing for AlertOK/AlertRetry/AlertAssert/messageBox). Implemented here
// using NSAlert, the real, native, dependency-free Cocoa API for exactly
// this purpose.
//
// Unlike Win32's ::MessageBox (which only offers a handful of fixed
// button layouts and therefore needed a custom CreateWindowExW-based
// dialog for the 4-button MBAlertAssert case — see win32MsgBox.cpp),
// NSAlert supports an arbitrary number of buttons with arbitrary labels
// via addButtonWithTitle:, so MBAlertAssert needs no special-cased custom
// window here — a plain NSAlert with 4 buttons handles it directly.
//-----------------------------------------------------------------------------
#import <Cocoa/Cocoa.h>

#import "platform/platform.h"
#import "platform/nativeDialogs/msgBox.h"
#import "console/console.h"

namespace
{
    // NSAlert buttons are added in order and the FIRST one added becomes
    // the default (Return-key) button, with later ones added left of it.
    S32 runAlert(NSAlertStyle style, const char* title, const char* message,
                 const char* button1, S32 result1,
                 const char* button2 = nullptr, S32 result2 = 0,
                 const char* button3 = nullptr, S32 result3 = 0,
                 const char* button4 = nullptr, S32 result4 = 0)
    {
        @autoreleasepool {
            NSAlert* alert = [[NSAlert alloc] init];
            alert.alertStyle = style;
            alert.messageText = [NSString stringWithUTF8String:title];
            alert.informativeText = [NSString stringWithUTF8String:message];

            [alert addButtonWithTitle:[NSString stringWithUTF8String:button1]];
            if (button2) [alert addButtonWithTitle:[NSString stringWithUTF8String:button2]];
            if (button3) [alert addButtonWithTitle:[NSString stringWithUTF8String:button3]];
            if (button4) [alert addButtonWithTitle:[NSString stringWithUTF8String:button4]];

            const NSModalResponse response = [alert runModal];

            // NSAlertFirstButtonReturn == 1000, incrementing per button.
            switch (response)
            {
                case NSAlertFirstButtonReturn:  return result1;
                case NSAlertSecondButtonReturn: return result2;
                case NSAlertThirdButtonReturn:  return result3;
                default:                        return result4;
            }
        }
    }
}

//-----------------------------------------------------------------------------
S32 Platform::messageBox(const UTF8 *title, const UTF8 *message, MBButtons buttons, MBIcons icon)
{
    const NSAlertStyle style = (icon == MIStop || icon == MIWarning) ? NSAlertStyleCritical
                              : (icon == MIQuestion)                  ? NSAlertStyleWarning
                              :                                        NSAlertStyleInformational;

    switch (buttons)
    {
        case MBOk:
            return runAlert(style, title, message, "OK", MROk);

        case MBOkCancel:
            return runAlert(style, title, message, "OK", MROk, "Cancel", MRCancel);

        case MBRetryCancel:
            return runAlert(style, title, message, "Retry", MRRetry, "Cancel", MRCancel);

        case MBSaveDontSave:
            return runAlert(style, title, message, "Save", MROk, "Don't Save", MRDontSave);

        case MBSaveDontSaveCancel:
            return runAlert(style, title, message, "Save", MROk, "Don't Save", MRDontSave, "Cancel", MRCancel);

        case MBAlertAssert:
            return runAlert(NSAlertStyleCritical, title, message,
                             "Debug", Platform::ALERT_ASSERT_DEBUG,
                             "Ignore", Platform::ALERT_ASSERT_IGNORE,
                             "Ignore All", Platform::ALERT_ASSERT_IGNORE_ALL,
                             "Exit", Platform::ALERT_ASSERT_EXIT);
    }

    return MROk;
}

void Platform::AlertOK(const char *windowTitle, const char *message)
{
    Platform::messageBox(windowTitle, message, MBOk, MIInformation);
}

bool Platform::AlertOKCancel(const char *windowTitle, const char *message)
{
    return Platform::messageBox(windowTitle, message, MBOkCancel, MIQuestion) == MROk;
}

bool Platform::AlertRetry(const char *windowTitle, const char *message)
{
    return Platform::messageBox(windowTitle, message, MBRetryCancel, MIWarning) == MRRetry;
}

Platform::ALERT_ASSERT_RESULT Platform::AlertAssert(const char *windowTitle, const char *message)
{
    return static_cast<Platform::ALERT_ASSERT_RESULT>(
        Platform::messageBox(windowTitle, message, MBAlertAssert, MIStop));
}
