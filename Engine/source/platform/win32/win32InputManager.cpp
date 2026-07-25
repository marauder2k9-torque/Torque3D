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

#include "platform/platform.h"

#if !defined(TORQUE_SDL) && defined(TORQUE_OS_WIN)

#include "platform/win32/win32InputManager.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "windowManager/win32/win32Window.h"
#include "windowManager/win32/win32WindowMgr.h"
#include "windowManager/platformWindowMgr.h"

#include <hidsdi.h>
#include <hidpi.h>

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "hid.lib")

//------------------------------------------------------------------------------
void XInputControllerState::reset()
{
   torqueInstID = 0;
   isConnected = false;
   ZeroMemory(&lastState, sizeof(lastState));
}

void RawHidJoystickState::reset()
{
   rawInputHandle = NULL;
   torqueInstID = 0;
   numAxes = 0;
   numButtons = 0;
   numHats = 0;
   lastHatState = 0;

   for (U32 i = 0; i < MaxAxes; ++i)
   {
      axisLogicalMin[i] = 0;
      axisLogicalMax[i] = 0;
      axisUsage[i] = 0;
   }
   for (U32 i = 0; i < MaxButtons; ++i)
      lastButtonState[i] = false;
}

//------------------------------------------------------------------------------
Win32InputManager::Win32InputManager()
   : mNumRawJoysticks(0), mRawInputRegistered(false)
{
   mEnabled = false;

   for (U32 i = 0; i < MaxXInputControllers; ++i)
      mXInputControllers[i].reset();
   for (U32 i = 0; i < MaxRawJoysticks; ++i)
      mRawJoysticks[i].reset();
}

//------------------------------------------------------------------------------
void Win32InputManager::init()
{
   // Nothing beyond construction — XInput needs no init call (unlike
   // DirectInput's IDirectInput8Create), and RawInput registration
   // happens lazily once a window exists (see enable()/process()).
}

//------------------------------------------------------------------------------
bool Win32InputManager::enable()
{
   mEnabled = true;

   // Try to register RawInput immediately if a window already exists;
   // if not, process() will retry each frame until one does (a window
   // may not exist yet at Input::init() time).
   PlatformWindow* firstWindow = WindowManager ? WindowManager->getFirstWindow() : NULL;
   if (firstWindow)
   {
      Win32Window* win32Window = dynamic_cast<Win32Window*>(firstWindow);
      if (win32Window)
         registerRawInput(win32Window->getHWND());
   }

   return true;
}

//------------------------------------------------------------------------------
void Win32InputManager::disable()
{
   mEnabled = false;

   for (U32 i = 0; i < MaxXInputControllers; ++i)
   {
      if (mXInputControllers[i].isConnected)
         DeviceIdentifier::removeDevice(XInputDeviceType, i);
      mXInputControllers[i].reset();
   }

   for (U32 i = 0; i < mNumRawJoysticks; ++i)
      DeviceIdentifier::removeDevice(JoystickDeviceType, (S32)(intptr_t)mRawJoysticks[i].rawInputHandle);
   mNumRawJoysticks = 0;

   mRawInputRegistered = false;
}

//------------------------------------------------------------------------------
bool Win32InputManager::registerRawInput(HWND targetWindow)
{
   if (mRawInputRegistered || !targetWindow)
      return mRawInputRegistered;

   RAWINPUTDEVICE devices[2];

   // Generic joysticks (flight sticks, wheels, arbitrary HID game controllers).
   devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
   devices[0].usUsage     = HID_USAGE_GENERIC_JOYSTICK;
   devices[0].dwFlags     = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
   devices[0].hwndTarget  = targetWindow;

   // Gamepads that show up as HID gamepads rather than joysticks (some
   // third-party/non-XInput pads report this way). XInput-class pads
   // are still handled separately via pollXInput() — this is only for
   // gamepad-class HID devices that AREN'T also enumerable via XInput,
   // which Windows itself de-duplicates for true Xbox-family hardware.
   devices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
   devices[1].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
   devices[1].dwFlags     = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
   devices[1].hwndTarget  = targetWindow;

   if (!RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE)))
   {
      Con::errorf("Win32InputManager: RegisterRawInputDevices failed (error %lu)", GetLastError());
      return false;
   }

   mRawInputRegistered = true;
   return true;
}

//------------------------------------------------------------------------------
void Win32InputManager::process()
{
   if (!mEnabled)
      return;

   if (!mRawInputRegistered)
   {
      PlatformWindow* firstWindow = WindowManager ? WindowManager->getFirstWindow() : NULL;
      Win32Window* win32Window = firstWindow ? dynamic_cast<Win32Window*>(firstWindow) : NULL;
      if (win32Window)
         registerRawInput(win32Window->getHWND());
   }

   pollXInput();

   // RawInput itself is event-driven (processRawInput()/
   // processRawInputDeviceChange(), called from Win32Window::WindowProc
   // on WM_INPUT/WM_INPUT_DEVICE_CHANGE) — nothing to poll here for it.
}

//------------------------------------------------------------------------------
void Win32InputManager::pollXInput()
{
   for (DWORD userIndex = 0; userIndex < MaxXInputControllers; ++userIndex)
   {
      XInputControllerState& ctrl = mXInputControllers[userIndex];

      XINPUT_STATE state;
      ZeroMemory(&state, sizeof(state));
      DWORD result = XInputGetState(userIndex, &state);

      if (result != ERROR_SUCCESS)
      {
         // Slot empty. If it was connected last poll, report disconnect.
         if (ctrl.isConnected)
         {
            DeviceIdentifier::removeDevice(XInputDeviceType, (S32)userIndex);
            ctrl.reset();
         }
         continue;
      }

      if (!ctrl.isConnected)
      {
         // Newly connected this poll.
         ctrl.isConnected = true;
         ctrl.torqueInstID = userIndex;
         ctrl.lastState = state;   // seed so we don't fire a wall of "changed" events on first poll

         char nameBuf[32];
         dSprintf(nameBuf, sizeof(nameBuf), "XInput Controller %u", userIndex + 1);
         DeviceIdentifier::addDevice(XInputDeviceType, (S32)userIndex, nameBuf);

         InputEventInfo event;
         event.deviceType = XInputDeviceType;
         event.deviceInst = userIndex;
         event.objType    = SI_BUTTON;
         event.objInst    = XI_CONNECT;
         event.action     = SI_MAKE;
         event.fValue     = 1.0f;
         event.postToSignal(Input::smInputEvent);

         continue;   // Next poll will start diffing state; avoids a spurious "everything moved" burst.
      }

      const XINPUT_GAMEPAD& pad = state.Gamepad;
      const XINPUT_GAMEPAD& last = ctrl.lastState.Gamepad;

      // Thumbsticks — normalize to -1.0..1.0, with Microsoft's documented
      // per-stick deadzone constants (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE /
      // ..._RIGHT_THUMB_DEADZONE) applied so a stick at rest reports
      // exactly 0 rather than drifting on worn hardware.
      auto normalizeStick = [](SHORT raw, SHORT deadzone) -> F32
      {
         if (raw > deadzone)
            return (F32)(raw - deadzone) / (F32)(32767 - deadzone);
         if (raw < -deadzone)
            return (F32)(raw + deadzone) / (F32)(32768 - deadzone);
         return 0.0f;
      };

      auto postAxis = [&](InputObjectInstances axis, F32 value, F32 lastValue)
      {
         if (value == lastValue)
            return;
         InputEventInfo event;
         event.deviceType = XInputDeviceType;
         event.deviceInst = userIndex;
         event.objType    = SI_AXIS;
         event.objInst    = axis;
         event.action     = SI_MOVE;
         event.fValue     = value;
         event.postToSignal(Input::smInputEvent);
      };

      postAxis(XI_THUMBLX, normalizeStick(pad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE),
                            normalizeStick(last.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
      postAxis(XI_THUMBLY, normalizeStick(pad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE),
                            normalizeStick(last.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
      postAxis(XI_THUMBRX, normalizeStick(pad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE),
                            normalizeStick(last.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
      postAxis(XI_THUMBRY, normalizeStick(pad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE),
                            normalizeStick(last.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));

      // Triggers — 0..255 native range, own documented threshold
      // (XINPUT_GAMEPAD_TRIGGER_THRESHOLD), normalized to 0.0..1.0
      // (triggers don't go negative, unlike thumbsticks).
      auto normalizeTrigger = [](BYTE raw) -> F32
      {
         if (raw < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            return 0.0f;
         return (F32)(raw - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / (F32)(255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
      };

      postAxis(XI_LEFT_TRIGGER, normalizeTrigger(pad.bLeftTrigger), normalizeTrigger(last.bLeftTrigger));
      postAxis(XI_RIGHT_TRIGGER, normalizeTrigger(pad.bRightTrigger), normalizeTrigger(last.bRightTrigger));

      // Buttons — XINPUT_GAMEPAD_* bits to XI_* Torque codes. D-pad is
      // reported through SI_POV (like joystick hats) rather than as
      // discrete XI_DPAD_* buttons, since event.h has no such codes
      // (they're commented out there) and POV is the existing,
      // consistent way this engine represents a directional pad.
      auto postButton = [&](WORD bit, InputObjectInstances code)
      {
         bool wasDown = (last.wButtons & bit) != 0;
         bool isDown  = (pad.wButtons & bit) != 0;
         if (wasDown == isDown)
            return;
         InputEventInfo event;
         event.deviceType = XInputDeviceType;
         event.deviceInst = userIndex;
         event.objType    = SI_BUTTON;
         event.objInst    = code;
         event.action     = isDown ? SI_MAKE : SI_BREAK;
         event.fValue     = isDown ? 1.0f : 0.0f;
         event.postToSignal(Input::smInputEvent);
      };

      postButton(XINPUT_GAMEPAD_START, XI_START);
      postButton(XINPUT_GAMEPAD_BACK, XI_BACK);
      postButton(XINPUT_GAMEPAD_LEFT_THUMB, XI_LEFT_THUMB);
      postButton(XINPUT_GAMEPAD_RIGHT_THUMB, XI_RIGHT_THUMB);
      postButton(XINPUT_GAMEPAD_LEFT_SHOULDER, XI_LEFT_SHOULDER);
      postButton(XINPUT_GAMEPAD_RIGHT_SHOULDER, XI_RIGHT_SHOULDER);
      postButton(XINPUT_GAMEPAD_A, XI_A);
      postButton(XINPUT_GAMEPAD_B, XI_B);
      postButton(XINPUT_GAMEPAD_X, XI_X);
      postButton(XINPUT_GAMEPAD_Y, XI_Y);

      // D-pad -> POV hat, matching SDLInputManager::buildHatEvents'
      // SI_UPOV/SI_DPOV/SI_LPOV/SI_RPOV convention.
      auto postPov = [&](WORD bit, InputObjectInstances povCode)
      {
         bool wasDown = (last.wButtons & bit) != 0;
         bool isDown  = (pad.wButtons & bit) != 0;
         if (wasDown == isDown)
            return;
         InputEventInfo event;
         event.deviceType = XInputDeviceType;
         event.deviceInst = userIndex;
         event.objType    = SI_POV;
         event.objInst    = povCode;
         event.action     = isDown ? SI_MAKE : SI_BREAK;
         event.fValue     = isDown ? 1.0f : 0.0f;
         event.postToSignal(Input::smInputEvent);
      };

      postPov(XINPUT_GAMEPAD_DPAD_UP, SI_UPOV);
      postPov(XINPUT_GAMEPAD_DPAD_DOWN, SI_DPOV);
      postPov(XINPUT_GAMEPAD_DPAD_LEFT, SI_LPOV);
      postPov(XINPUT_GAMEPAD_DPAD_RIGHT, SI_RPOV);

      ctrl.lastState = state;
   }
}

//------------------------------------------------------------------------------
RawHidJoystickState* Win32InputManager::findRawJoystickByHandle(HANDLE rawInputHandle)
{
   for (U32 i = 0; i < mNumRawJoysticks; ++i)
   {
      if (mRawJoysticks[i].rawInputHandle == rawInputHandle)
         return &mRawJoysticks[i];
   }
   return NULL;
}

//------------------------------------------------------------------------------
RawHidJoystickState* Win32InputManager::findOrAddRawJoystick(HANDLE rawInputHandle)
{
   RawHidJoystickState* existing = findRawJoystickByHandle(rawInputHandle);
   if (existing)
      return existing;

   if (mNumRawJoysticks >= MaxRawJoysticks)
   {
      Con::warnf("Win32InputManager: too many raw HID joysticks connected, ignoring device.");
      return NULL;
   }

   // Query the required buffer size, then fetch the device's preparsed
   // HID data so we can ask the HID parser for its capabilities
   // (axis/button/hat counts and each axis's logical value range) —
   // every device is potentially laid out differently, so this cannot
   // be assumed/hardcoded the way XInput's fixed layout can.
   UINT bufferSize = 0;
   if (GetRawInputDeviceInfo(rawInputHandle, RIDI_PREPARSEDDATA, NULL, &bufferSize) != 0)
      return NULL;

   Vector<BYTE> preparsedData;
   preparsedData.setSize(bufferSize);
   if ((UINT)GetRawInputDeviceInfo(rawInputHandle, RIDI_PREPARSEDDATA, preparsedData.address(), &bufferSize) != bufferSize)
      return NULL;

   PHIDP_PREPARSED_DATA hidData = (PHIDP_PREPARSED_DATA)preparsedData.address();

   HIDP_CAPS caps;
   if (HidP_GetCaps(hidData, &caps) != HIDP_STATUS_SUCCESS)
      return NULL;

   RawHidJoystickState& js = mRawJoysticks[mNumRawJoysticks++];
   js.reset();
   js.rawInputHandle = rawInputHandle;
   js.torqueInstID = mNumRawJoysticks - 1;
   js.numButtons = min((U32)caps.NumberInputButtonCaps, (U32)RawHidJoystickState::MaxButtons);

   // Value (axis) caps — each entry describes one axis's usage and
   // logical min/max, needed to normalize raw reports to -1.0..1.0.
   USHORT valueCapsCount = caps.NumberInputValueCaps;
   if (valueCapsCount > 0)
   {
      Vector<HIDP_VALUE_CAPS> valueCaps;
      valueCaps.setSize(valueCapsCount);
      if (HidP_GetValueCaps(HidP_Input, valueCaps.address(), &valueCapsCount, hidData) == HIDP_STATUS_SUCCESS)
      {
         js.numAxes = min((U32)valueCapsCount, (U32)RawHidJoystickState::MaxAxes);
         for (U32 i = 0; i < js.numAxes; ++i)
         {
            js.axisUsage[i] = valueCaps[i].Range.UsageMin;
            js.axisLogicalMin[i] = valueCaps[i].LogicalMin;
            js.axisLogicalMax[i] = valueCaps[i].LogicalMax;
         }
      }
   }

   // Device name/product string, best-effort — RawInput devices don't
   // always have one via GetRawInputDeviceInfo(RIDI_DEVICENAME); a
   // human-readable product string requires opening the underlying
   // device file and calling HidD_GetProductString, which needs
   // SetupAPI to resolve the device path to a file handle. Kept simple
   // here (generic name) rather than adding that extra SetupAPI
   // dependency for a display-only string; can be added later if
   // script/UI code wants nicer names.
   char nameBuf[32];
   dSprintf(nameBuf, sizeof(nameBuf), "HID Joystick %u", js.torqueInstID + 1);
   DeviceIdentifier::addDevice(JoystickDeviceType, (S32)(intptr_t)rawInputHandle, nameBuf);

   return &js;
}

//------------------------------------------------------------------------------
void Win32InputManager::processRawInput(WPARAM wParam, LPARAM lParam)
{
   UINT dataSize = 0;
   GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
   if (dataSize == 0)
      return;

   Vector<BYTE> buffer;
   buffer.setSize(dataSize);

   if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.address(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
      return;

   RAWINPUT* raw = (RAWINPUT*)buffer.address();
   if (raw->header.dwType != RIM_TYPEHID)
      return;   // Not a HID joystick/gamepad report (keyboard/mouse handled elsewhere).

   RawHidJoystickState* js = findOrAddRawJoystick(raw->header.hDevice);
   if (!js)
      return;

   UINT bufferSize = 0;
   GetRawInputDeviceInfo(raw->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize);
   if (bufferSize == 0)
      return;

   Vector<BYTE> preparsedData;
   preparsedData.setSize(bufferSize);
   if ((UINT)GetRawInputDeviceInfo(raw->header.hDevice, RIDI_PREPARSEDDATA, preparsedData.address(), &bufferSize) != bufferSize)
      return;

   PHIDP_PREPARSED_DATA hidData = (PHIDP_PREPARSED_DATA)preparsedData.address();
   PCHAR report = (PCHAR)raw->data.hid.bRawData;
   ULONG reportLen = raw->data.hid.dwSizeHid;

   // Buttons: HidP_GetUsages returns the list of currently-pressed
   // button usages in this report; diff against last-seen state to
   // generate MAKE/BREAK edges (RawInput, like XInput, reports absolute
   // state per packet rather than deltas).
   ULONG usageLength = js->numButtons;
   Vector<USAGE> pressedUsages;
   pressedUsages.setSize(js->numButtons > 0 ? js->numButtons : 1);

   bool currentButtonState[RawHidJoystickState::MaxButtons];
   for (U32 i = 0; i < RawHidJoystickState::MaxButtons; ++i)
      currentButtonState[i] = false;

   if (js->numButtons > 0 &&
       HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, pressedUsages.address(), &usageLength, hidData, report, reportLen) == HIDP_STATUS_SUCCESS)
   {
      for (ULONG i = 0; i < usageLength; ++i)
      {
         U32 buttonIndex = pressedUsages[(U32)i] - 1;   // Button usages are 1-based
         if (buttonIndex < RawHidJoystickState::MaxButtons)
            currentButtonState[buttonIndex] = true;
      }
   }

   for (U32 i = 0; i < js->numButtons; ++i)
   {
      if (currentButtonState[i] == js->lastButtonState[i])
         continue;

      InputEventInfo event;
      event.deviceType = JoystickDeviceType;
      event.deviceInst = js->torqueInstID;
      event.objType    = SI_BUTTON;
      event.objInst    = (InputObjectInstances)(KEY_BUTTON0 + i);
      event.action     = currentButtonState[i] ? SI_MAKE : SI_BREAK;
      event.fValue     = currentButtonState[i] ? 1.0f : 0.0f;
      event.postToSignal(Input::smInputEvent);

      js->lastButtonState[i] = currentButtonState[i];
   }

   // Axes: HidP_GetUsageValue per cached axis, normalized via the
   // logical min/max captured at connect time in findOrAddRawJoystick().
   for (U32 i = 0; i < js->numAxes; ++i)
   {
      ULONG rawValue = 0;
      if (HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, js->axisUsage[i], &rawValue, hidData, report, reportLen) != HIDP_STATUS_SUCCESS)
         continue;

      LONG range = js->axisLogicalMax[i] - js->axisLogicalMin[i];
      if (range <= 0)
         continue;

      // Normalize to -1.0..1.0 around the axis's own logical center,
      // rather than assuming a symmetric native range like XInput has.
      F32 normalized = (2.0f * (F32)((LONG)rawValue - js->axisLogicalMin[i]) / (F32)range) - 1.0f;

      InputEventInfo event;
      event.deviceType = JoystickDeviceType;
      event.deviceInst = js->torqueInstID;
      event.objType    = SI_AXIS;
      event.objInst    = (InputObjectInstances)(SI_XAXIS + i);
      event.action     = SI_MOVE;
      event.fValue     = normalized;
      event.postToSignal(Input::smInputEvent);
   }
}

//------------------------------------------------------------------------------
void Win32InputManager::processRawInputDeviceChange(WPARAM wParam, LPARAM lParam)
{
   HANDLE deviceHandle = (HANDLE)lParam;

   if (wParam == GIDC_REMOVAL)
   {
      RawHidJoystickState* js = findRawJoystickByHandle(deviceHandle);
      if (js)
      {
         DeviceIdentifier::removeDevice(JoystickDeviceType, (S32)(intptr_t)deviceHandle);
         // Compact the array so mNumRawJoysticks stays a tight count;
         // torqueInstID of the moved entry is intentionally left as-is
         // rather than renumbered, so any script binding to a specific
         // instance number isn't silently redirected to a different
         // physical device mid-session.
         U32 index = (U32)(js - mRawJoysticks);
         for (U32 i = index; i + 1 < mNumRawJoysticks; ++i)
            mRawJoysticks[i] = mRawJoysticks[i + 1];
         --mNumRawJoysticks;
      }
   }
   else if (wParam == GIDC_ARRIVAL)
   {
      // Actual registration happens lazily on first WM_INPUT report
      // from this device (findOrAddRawJoystick(), called from
      // processRawInput()) rather than here, since GIDC_ARRIVAL alone
      // doesn't carry a HID report to parse caps against in the same
      // way — nothing further to do at this point.
   }
}

//------------------------------------------------------------------------------
S32 Win32InputManager::getXInputControllerCount()
{
   S32 count = 0;
   for (U32 i = 0; i < MaxXInputControllers; ++i)
   {
      if (mXInputControllers[i].isConnected)
         ++count;
   }
   return count;
}

S32 Win32InputManager::getRawJoystickCount()
{
   return (S32)mNumRawJoysticks;
}

//------------------------------------------------------------------------------
DefineEngineFunction(isJoystickDetected, bool, (), ,
   "Returns true if at least one XInput controller or raw HID joystick is currently connected.\n"
   "@ingroup Input")
{
   Win32InputManager* mgr = dynamic_cast<Win32InputManager*>(Input::getManager());
   if (!mgr)
      return false;
   return mgr->getXInputControllerCount() > 0 || mgr->getRawJoystickCount() > 0;
}

#endif  // !defined(TORQUE_SDL) && defined(TORQUE_OS_WIN)
