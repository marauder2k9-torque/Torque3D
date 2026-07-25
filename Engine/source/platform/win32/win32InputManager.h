//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _WIN32INPUTMANAGER_H_
#define _WIN32INPUTMANAGER_H_

// Win32-native path only: not SDL, not another OS.
#if !defined(TORQUE_SDL) && defined(TORQUE_OS_WIN)

#ifndef _PLATFORMINPUT_H_
#include "platform/platformInput.h"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

//------------------------------------------------------------------------------
/// One raw HID joystick/gamepad tracked via RawInput, distinct from an
/// XInput-class pad (see Win32InputManager's two device lists below).
/// Axis/button/hat layout varies per device, so caps are parsed once at
/// connect time via the HID parser (HidP_GetCaps/GetButtonCaps/
/// GetValueCaps) rather than assumed.
struct RawHidJoystickState
{
   HANDLE  rawInputHandle;    // RawInput device handle - our "backend id" for this device (see DeviceIdentifier)
   U32     torqueInstID;
   U32     numAxes;
   U32     numButtons;
   U32     numHats;

   // Cached per-axis logical value ranges, needed to normalize HID raw
   // values (which vary per device/driver) to Torque's -1.0..1.0 range.
   enum { MaxAxes = 8 };
   LONG    axisLogicalMin[MaxAxes];
   LONG    axisLogicalMax[MaxAxes];
   USHORT  axisUsage[MaxAxes];

   // Last-seen button/hat state, to detect edges (RawInput reports
   // absolute state per packet, not deltas).
   enum { MaxButtons = 32 };
   bool    lastButtonState[MaxButtons];
   U8      lastHatState;

   void reset();
};

/// One XInput-class controller (Xbox 360/One/Series pad or compatible).
/// XUSER_MAX_COUNT (4) of these exist always; unused slots simply have
/// isConnected == false rather than being allocated/freed dynamically,
/// matching how XInputGetState's fixed slot model actually works.
struct XInputControllerState
{
   U32   torqueInstID;
   bool  isConnected;
   XINPUT_STATE lastState;

   void reset();
};

//------------------------------------------------------------------------------
class Win32InputManager : public InputManager
{
   enum Constants
   {
      MaxXInputControllers = 4,        // XUSER_MAX_COUNT
      MaxRawJoysticks = 8,             // Arbitrary but generous cap on simultaneous raw HID devices
   };

private:
   typedef InputManager Parent;

   XInputControllerState mXInputControllers[MaxXInputControllers];
   RawHidJoystickState   mRawJoysticks[MaxRawJoysticks];
   U32                   mNumRawJoysticks;

   bool mRawInputRegistered;

   /// Poll all 4 XInput slots for connect/disconnect and state changes,
   /// posting Torque input events for whatever changed. Called once per
   /// process(). XInputGetState on a disconnected slot is a normal,
   /// well-defined (if moderately expensive) call — Microsoft's own
   /// guidance is to poll rather than assume a slot stays empty/full,
   /// since there's no push notification for XInput hot-plug.
   void pollXInput();

   /// Register for HID joystick/gamepad RawInput on the given window.
   /// Deferred until a window actually exists (see enable()/process()),
   /// since RegisterRawInputDevices needs a target HWND and the input
   /// manager can be constructed before any window is.
   bool registerRawInput(HWND targetWindow);

   /// Look up or create tracking state for a newly-seen RawInput device
   /// handle, parsing its HID capabilities the first time it's seen.
   RawHidJoystickState* findOrAddRawJoystick(HANDLE rawInputHandle);

   RawHidJoystickState* findRawJoystickByHandle(HANDLE rawInputHandle);

public:
   Win32InputManager();

   bool enable() override;
   void disable() override;
   void process() override;

   static void init();

   /// Called from Win32Window::WindowProc on WM_INPUT — parses the raw
   /// HID report and posts the corresponding Torque input event(s).
   /// lParam is the RawInput handle exactly as received by WM_INPUT
   /// (i.e. pass lParam through unchanged).
   void processRawInput(WPARAM wParam, LPARAM lParam);

   /// Called from Win32Window::WindowProc on WM_INPUT_DEVICE_CHANGE —
   /// updates DeviceIdentifier registration for the device that
   /// connected/disconnected. wParam is GIDC_ARRIVAL or GIDC_REMOVAL;
   /// lParam is the device handle, exactly as received.
   void processRawInputDeviceChange(WPARAM wParam, LPARAM lParam);

   // Console interface (mirrors SDLInputManager's shape where a native
   // equivalent exists; SDL-specific concepts like GUID mapping strings
   // don't have a meaningful RawInput/XInput analog and are intentionally
   // not replicated here):
   S32 getXInputControllerCount();
   S32 getRawJoystickCount();
};

#endif  // !defined(TORQUE_SDL) && defined(TORQUE_OS_WIN)

#endif  // _WIN32XINPUTMANAGER_H_
