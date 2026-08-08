//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiInputEvent.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUIINPUTEVENT_H_
#define _NEWGUIINPUTEVENT_H_

#ifndef _MPOINT2_H_
#include "math/mPoint2.h"
#endif

/// Which physical input device produced an event.
enum class NewGuiDeviceKind : U8
{
   Mouse = 0,
   Touch,
   Keyboard,
   Gamepad,
};

/// What kind of interaction an event represents, shared across every device kind.
enum class NewGuiInputAction : U8
{
   Move = 0,      ///< Pointer moved with no button/finger state change.
   Down,          ///< Button pressed, finger touched down, or key pressed.
   Up,            ///< Button released, finger lifted, or key released.
   Repeat,        ///< Key auto-repeat while held.
   Wheel,         ///< Scroll wheel or equivalent gesture.
};

/// One input event, dispatched by reference up a control's ancestor chain
/// until some control sets handled = true or there are no more ancestors.
struct NewGuiInputEvent
{
   NewGuiDeviceKind   deviceKind;
   NewGuiInputAction  action;

   bool handled;           ///< True once some control has claimed this event.

   Point2I screenPoint;    ///< Absolute position in canvas device-pixel space. Unused for Keyboard events.
   Point2I localPoint;     ///< screenPoint relative to the current dispatch target's own bounds.

   U32 modifier;           ///< SI_LSHIFT/SI_LCTRL/etc bitmask. 0 for device kinds with no modifier state.
   U8  wheelAxis;          ///< For action == Wheel: 1 = vertical, 0 = horizontal.
   F32 wheelDelta;         ///< For action == Wheel: scroll delta.
   U16 ascii;              ///< Decoded ASCII for Keyboard events.
   U16 keyCode;            ///< Key code for Keyboard events.
   U8  clickCount;         ///< Consecutive-click count for action == Down (1 = single click, 2 = double-click).
   bool isCharInput;       ///< True only for a translated character input

   NewGuiInputEvent()
      : deviceKind(NewGuiDeviceKind::Mouse),
      action(NewGuiInputAction::Move),
      handled(false),
      screenPoint(0, 0),
      localPoint(0, 0),
      modifier(0),
      wheelAxis(0),
      wheelDelta(0.0f),
      ascii(0),
      keyCode(0),
      clickCount(1),
      isCharInput(false)
   {
   }
};

#endif // _NEWGUIINPUTEVENT_H_
