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

#ifndef _PLATFORMINPUT_H_
#define _PLATFORMINPUT_H_

#ifndef _SIMBASE_H_
#include "sim/simBase.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include "platform/input/event.h"

//------------------------------------------------------------------------------
U8 TranslateOSKeyCode( U8 vcode );
U8 TranslateKeyCodeToOS(U8 keycode);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class InputDevice : public SimObject
{
protected:
   char mName[30];

public:
   struct ObjInfo
   {
      InputEventType      mType;
      InputObjectInstances  mInst;
      S32   mMin, mMax;
   };

   inline const char* getDeviceName()
   {
      return mName;
   }

   virtual bool process() = 0;
};

//------------------------------------------------------------------------------

class InputManager : public SimGroup
{
protected:
   bool  mEnabled;

public:
   inline bool  isEnabled()
   {
      return mEnabled;
   }

   virtual bool enable() = 0;
   virtual void disable() = 0;
   virtual void process() = 0;
};

/// Describes one physical (or virtual, e.g. a touchscreen surface) input
/// device that a backend has identified, independent of SDL vs native.
struct DeviceInfo
{
   DeviceInfo()
      : deviceType((InputDeviceTypes)UnknownDeviceType),
      torqueInstance(0),
      backendId(-1),
      numAxes(0),
      numButtons(0),
      numHats(0),
      isConnected(false)
   {
      name[0] = 0;
      guid[0] = 0;
   }

   /// Torque's classification for this device: MouseDeviceType,
   /// KeyboardDeviceType, JoystickDeviceType, GamepadDeviceType,
   /// XInputDeviceType, or TouchDeviceType. UnknownDeviceType means the
   /// backend saw a device but couldn't classify it — that's still
   /// reported (not dropped), so script/log output can surface it.
   InputDeviceTypes deviceType;

   /// Torque-side instance id (joystick0, joystick1, controller0, ...)
   U32 torqueInstance;

   /// Backend-native identifier (SDL_JoystickID, a DirectInput device
   /// index, an Android device id, etc). Opaque outside the backend that
   /// produced it — used only to look this DeviceInfo back up when a
   /// disconnect/event for it comes in.
   S32 backendId;

   /// Human-readable device name, if the backend/OS provides one.
   char name[64];

   /// Device GUID/product string, if available (SDL provides this
   /// natively; native backends may need to synthesize one from
   /// vendor/product IDs, or leave it empty).
   char guid[64];

   U32 numAxes;
   U32 numButtons;
   U32 numHats;

   bool isConnected;
};

/// Shared device-discovery registry. A backend calls addDevice()/
/// removeDevice() as it discovers or loses devices (joystick plugged in,
/// touch surface appears, etc); engine code that just wants to know
/// "what's out there" calls the query methods rather than reaching into
/// SDLInputManager or NativeInputManager directly. This keeps calling
/// code backend-agnostic — it's written once against DeviceIdentifier
/// and works whether TORQUE_SDL is defined or not.
class DeviceIdentifier
{
public:
   /// Register a newly-discovered device and assign/return its
   /// DeviceInfo. If a device with the same (deviceType, backendId) is
   /// already registered, updates and returns the existing entry instead
   /// of creating a duplicate.
   static DeviceInfo* addDevice(InputDeviceTypes deviceType, S32 backendId, const char* name, const char* guid = "");

   /// Mark a previously-registered device as disconnected. The entry is
   /// kept (not erased) so late-arriving events referencing it don't
   /// dereference a dangling pointer; isConnected simply goes false.
   static void removeDevice(InputDeviceTypes deviceType, S32 backendId);

   /// Look up a device by the backend-native id the backend itself uses
   /// to refer to it (e.g. an SDL_JoystickID or a DirectInput index).
   static DeviceInfo* findByBackendId(InputDeviceTypes deviceType, S32 backendId);

   /// Look up a device by its Torque-side instance number.
   static DeviceInfo* findByTorqueInstance(InputDeviceTypes deviceType, U32 torqueInstance);

   /// Number of currently-known devices of a given type (connected or
   /// not — check isConnected on each if that distinction matters).
   static U32 getDeviceCount(InputDeviceTypes deviceType);

   /// Enumerate all known devices of a given type, connected or not.
   static void getDevices(InputDeviceTypes deviceType, Vector<DeviceInfo*>& outDevices);

   /// Drop all registered devices. Called on Input::destroy() so a
   /// shutdown/reinit cycle doesn't accumulate stale entries.
   static void clear();
};

//------------------------------------------------------------------------------
// Cross-backend touch identification.
//
//------------------------------------------------------------------------------
class TouchIdentifier
{
public:
   /// Resolve a backend-native finger id to a stable SI_TOUCH_FINGERn
   /// slot, assigning the next free slot on first contact. Returns false
   /// (and does not populate outSlot) if all MaxTouchFingers slots are
   /// already in use — the touch is dropped rather than aliased onto an
   /// in-use slot, since aliasing would corrupt whatever's already bound
   /// to that finger.
   static bool acquireSlot(S64 backendFingerId, InputObjectInstances& outSlot);

   /// Release the slot associated with a backend-native finger id (call
   /// on finger-up), making it available for reuse.
   static void releaseSlot(S64 backendFingerId);

   /// Release all slots, e.g. on Input::destroy() or when a touch
   /// device disconnects mid-gesture.
   static void releaseAll();
};

enum KEY_STATE
{
   STATE_LOWER,
   STATE_UPPER,
   STATE_GOOFY
};

//------------------------------------------------------------------------------
class Input
{
protected:
   static InputManager* smManager;

   static bool smActive;

   /// Current modifier keys.
   static U8 smModifierKeys;

   static bool smLastKeyboardActivated;
   static bool smLastMouseActivated;
   static bool smLastJoystickActivated;

public:
   static void init();
   static void destroy();

   static bool enable();
   static void disable();

   static void activate();
   static void deactivate();

   static U16  getAscii( U16 keyCode, KEY_STATE keyState );
   static U16  getKeyCode( U16 asciiCode );

   static bool isEnabled();
   static bool isActive();

   static void process();

   static InputManager* getManager();

   static U8 getModifierKeys() {return smModifierKeys;}
   static void setModifierKeys(U8 mod) {smModifierKeys = mod;}
#ifdef LOG_INPUT
   static void log( const char* format, ... );
#endif

   /// Build and post a single SI_TOUCH event through the normal input
   /// pipeline (smInputEvent)
   /// ActionMap/script is identical regardless of backend.
   ///   x, y      — normalized 0.0..1.0 position (see InputEventInfo::fValue/fValue2)
   ///   pressure  — 0.0..1.0, or 1.0 if the source hardware doesn't report it
   ///   action    — SI_MAKE (finger down), SI_MOVE (finger drag), or SI_BREAK (finger up)
   static void buildTouchEvent(InputObjectInstances fingerSlot, F32 x, F32 y, F32 pressure, InputActionType action);

   /// Global input routing JournaledSignal; post input events here for
   /// processing.
   static InputEvent smInputEvent;
};

#endif // _H_PLATFORMINPUT_
