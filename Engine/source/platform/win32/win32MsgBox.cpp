//-----------------------------------------------------------------------------
// win32MsgBox.cpp — Windows message box implementation.
//
// Fresh rewrite, self-contained: no dependency on Torque's legacy
// Win32PlatState (winState) or the windowManager module.
//
// The original win32MsgBox.cpp reached into `winState.renderThreadBlocked`
// and `WindowManager->getFirstWindow()` (from platformWin32.h /
// windowManager/win32/win32Window.h) to find a parent HWND and to pause
// the render thread / toggle cursor state around the blocking dialog call.
// That's why it was wrapped in "#ifndef TORQUE_SDL" in the first place —
// winState only exists when the native (non-SDL) Win32 window/render
// stack is actually linked in, so the file couldn't compile on its own.
//
// Per the "use NFD for file dialogs, keep the rest native and
// self-contained per platform, no SDL coupling" direction: this file no
// longer touches winState, PlatformWindow, or WindowManager at all.
// ::MessageBoxW works perfectly well with a NULL parent HWND (it simply
// becomes an application-modal box not tied to a specific window, which
// is standard, well-defined Win32 behavior) — cursor/render-thread
// bookkeeping around the modal call, if ever needed, belongs in whatever
// owns the window/render loop, not in the platform layer's message-box
// implementation.
//
// MBAlertAssert (Debug/Ignore/Ignore All/Exit — 4 custom-labeled buttons)
// still has no representation in MessageBox's fixed button layouts, so it
// still goes through the small custom dialog below (CreateWindowExW, no
// .rc resource file, no extra build dependency); every other MBButtons
// value goes through the native ::MessageBoxW.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/nativeDialogs/msgBox.h"
#include "console/console.h"
#include "core/strings/unicode.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <string>

namespace
{
   UINT toWinButtonFlags(MBButtons buttons)
   {
      switch (buttons)
      {
      case MBOk:                 return MB_OK;
      case MBOkCancel:           return MB_OKCANCEL;
      case MBRetryCancel:        return MB_RETRYCANCEL;
      case MBSaveDontSave:       return MB_YESNO;
      case MBSaveDontSaveCancel: return MB_YESNOCANCEL;
      case MBAlertAssert:        return MB_OK; // unused — handled separately
      }
      return MB_OK;
   }

   UINT toWinIconFlags(MBIcons icon)
   {
      switch (icon)
      {
      case MIWarning:     return MB_ICONWARNING;
      case MIInformation: return MB_ICONINFORMATION;
      case MIQuestion:    return MB_ICONQUESTION;
      case MIStop:        return MB_ICONSTOP;
      }
      return MB_ICONINFORMATION;
   }

   S32 fromWinResult(int winResult)
   {
      switch (winResult)
      {
      case IDCANCEL: return MRCancel;
      case IDNO:     return MRDontSave;
      case IDOK:     return MROk;
      case IDRETRY:  return MRRetry;
      case IDYES:    return MROk;
      default:       return MRCancel;
      }
   }

   //-------------------------------------------------------------------
   // Minimal custom dialog for MBAlertAssert's 4 buttons, built without
   // any .rc resource file so it has no extra build-time dependency.
   //-------------------------------------------------------------------
   struct AssertDialogButton
   {
      const wchar_t* label;
      S32 returnValue;
      HWND hwnd = nullptr;
   };

   LRESULT CALLBACK assertDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
   {
      switch (msg)
      {
      case WM_COMMAND:
      {
         const S32 id = LOWORD(wParam);
         if (id != 0)
         {
            // Button IDs are the 1-based index into the button
            // array + 100, chosen arbitrarily to avoid clashing
            // with any system-reserved control ID range.
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(id));
            ::PostMessage(hwnd, WM_CLOSE, 0, 0);
         }
         return 0;
      }
      case WM_CLOSE:
         ::DestroyWindow(hwnd);
         return 0;
      case WM_DESTROY:
         ::PostQuitMessage(0);
         return 0;
      }
      return ::DefWindowProcW(hwnd, msg, wParam, lParam);
   }

   S32 runAssertDialog(const wchar_t* title, const wchar_t* message, HWND parent)
   {
      static bool classRegistered = false;
      const wchar_t* className = L"TorqueAssertDialogClass";

      HINSTANCE hInst = ::GetModuleHandleW(nullptr);

      if (!classRegistered)
      {
         WNDCLASSEXW wc{};
         wc.cbSize = sizeof(wc);
         wc.lpfnWndProc = assertDialogProc;
         wc.hInstance = hInst;
         wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
         wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
         wc.lpszClassName = className;
         ::RegisterClassExW(&wc);
         classRegistered = true;
      }

      constexpr int width = 460;
      constexpr int height = 200;

      HWND hwnd = ::CreateWindowExW(
         WS_EX_DLGMODALFRAME, className, title,
         WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
         CW_USEDEFAULT, CW_USEDEFAULT, width, height,
         parent, nullptr, hInst, nullptr);

      if (!hwnd)
         return Platform::ALERT_ASSERT_EXIT;

      // Center on parent (or screen, if no parent window).
      RECT parentRect;
      if (parent && ::GetWindowRect(parent, &parentRect))
      {
         const int px = parentRect.left + ((parentRect.right - parentRect.left) - width) / 2;
         const int py = parentRect.top + ((parentRect.bottom - parentRect.top) - height) / 2;
         ::SetWindowPos(hwnd, nullptr, px, py, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
      }

      ::CreateWindowExW(0, L"STATIC", message, WS_CHILD | WS_VISIBLE | SS_LEFT,
         16, 16, width - 48, height - 100, hwnd, nullptr, hInst, nullptr);

      std::array<AssertDialogButton, 4> buttons = { {
          { L"Debug",      Platform::ALERT_ASSERT_DEBUG },
          { L"Ignore",     Platform::ALERT_ASSERT_IGNORE },
          { L"Ignore All", Platform::ALERT_ASSERT_IGNORE_ALL },
          { L"Exit",       Platform::ALERT_ASSERT_EXIT },
      } };

      constexpr int buttonWidth = 96;
      constexpr int buttonHeight = 28;
      constexpr int buttonSpacing = 8;
      int bx = width - 16 - static_cast<int>(buttons.size()) * (buttonWidth + buttonSpacing) + buttonSpacing;
      const int by = height - 72;

      for (size_t i = 0; i < buttons.size(); ++i)
      {
         buttons[i].hwnd = ::CreateWindowExW(
            0, L"BUTTON", buttons[i].label, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            bx, by, buttonWidth, buttonHeight,
            hwnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(100 + i)), hInst, nullptr);
         bx += buttonWidth + buttonSpacing;
      }

      ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);

      MSG msg;
      while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
      {
         ::TranslateMessage(&msg);
         ::DispatchMessageW(&msg);
         if (!::IsWindow(hwnd))
            break;
      }

      const LONG_PTR pressedId = ::GetWindowLongPtrW(hwnd, GWLP_USERDATA);
      if (pressedId >= 100 && pressedId < 100 + static_cast<LONG_PTR>(buttons.size()))
         return buttons[pressedId - 100].returnValue;

      return Platform::ALERT_ASSERT_EXIT; // window closed without a button press
   }
}

//-----------------------------------------------------------------------------
S32 Platform::messageBox(const UTF8* title, const UTF8* message, MBButtons buttons, MBIcons icon)
{
   const UTF16* msg = createUTF16string(message);
   const UTF16* t = createUTF16string(title);

   // ::GetActiveWindow() is a best-effort parent: it returns the active
   // window belonging to the CALLING THREAD's message queue, or NULL if
   // there isn't one — always safe to call, no dependency on any
   // particular window manager or global platform state being linked in.
   // A NULL parent is well-defined for MessageBoxW: it just becomes an
   // application-modal dialog not associated with a specific window.
   HWND parent = ::GetActiveWindow();

   S32 result;
   if (buttons == MBAlertAssert)
   {
      result = runAssertDialog(reinterpret_cast<const wchar_t*>(t), reinterpret_cast<const wchar_t*>(msg), parent);
   }
   else
   {
      const int winResult = ::MessageBoxW(parent, reinterpret_cast<const wchar_t*>(msg),
         reinterpret_cast<const wchar_t*>(t),
         toWinButtonFlags(buttons) | toWinIconFlags(icon));
      result = fromWinResult(winResult);
   }

   delete[] msg;
   delete[] t;

   return result;
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
