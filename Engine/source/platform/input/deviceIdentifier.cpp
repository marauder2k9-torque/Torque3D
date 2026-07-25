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

#include "platform/platformInput.h"
#include "core/util/tDictionary.h"

namespace
{
   /// One DeviceInfo per (deviceType, backendId) pair.
   Vector<DeviceInfo*> sDevices;

   /// backendFingerId -> assigned SI_TOUCH_FINGERn slot. Small (at most
   /// MaxTouchFingers live at once)
   struct TouchSlot
   {
      bool inUse = false;
      S64  backendFingerId = 0;
   };
   TouchSlot sTouchSlots[MaxTouchFingers];
}

//------------------------------------------------------------------------------
// DeviceIdentifier
//------------------------------------------------------------------------------

DeviceInfo* DeviceIdentifier::addDevice(InputDeviceTypes deviceType, S32 backendId, const char* name, const char* guid)
{
   // Already registered? Update in place rather than duplicating — this
   // covers the case of a backend re-announcing a device (SDL does this
   // on remap events, for instance).
   DeviceInfo* existing = findByBackendId(deviceType, backendId);
   if (existing)
   {
      dStrncpy(existing->name, name ? name : "", sizeof(existing->name) - 1);
      existing->name[sizeof(existing->name) - 1] = 0;
      if (guid && guid[0])
      {
         dStrncpy(existing->guid, guid, sizeof(existing->guid) - 1);
         existing->guid[sizeof(existing->guid) - 1] = 0;
      }
      existing->isConnected = true;
      return existing;
   }

   DeviceInfo* info = new DeviceInfo();
   info->deviceType = deviceType;
   info->backendId = backendId;
   info->isConnected = true;

   dStrncpy(info->name, name ? name : "", sizeof(info->name) - 1);
   info->name[sizeof(info->name) - 1] = 0;

   if (guid && guid[0])
   {
      dStrncpy(info->guid, guid, sizeof(info->guid) - 1);
      info->guid[sizeof(info->guid) - 1] = 0;
   }

   // Assign the next free Torque instance number among devices of the
   // same type - mirrors the joystick0/joystick1/... numbering both
   // backends already expect ActionMap to use.
   U32 nextInstance = 0;
   for (U32 i = 0; i < sDevices.size(); ++i)
   {
      if (sDevices[i]->deviceType == deviceType && sDevices[i]->torqueInstance >= nextInstance)
         nextInstance = sDevices[i]->torqueInstance + 1;
   }
   info->torqueInstance = nextInstance;

   sDevices.push_back(info);
   return info;
}

void DeviceIdentifier::removeDevice(InputDeviceTypes deviceType, S32 backendId)
{
   DeviceInfo* info = findByBackendId(deviceType, backendId);
   if (info)
      info->isConnected = false;

   // Deliberately not erased from sDevices — see the Vector<DeviceInfo*>
   // comment above. A late-arriving event for a just-disconnected device
   // will find isConnected == false rather than dereferencing freed
   // memory or a reused slot.
}

DeviceInfo* DeviceIdentifier::findByBackendId(InputDeviceTypes deviceType, S32 backendId)
{
   for (U32 i = 0; i < sDevices.size(); ++i)
   {
      if (sDevices[i]->deviceType == deviceType && sDevices[i]->backendId == backendId)
         return sDevices[i];
   }
   return NULL;
}

DeviceInfo* DeviceIdentifier::findByTorqueInstance(InputDeviceTypes deviceType, U32 torqueInstance)
{
   for (U32 i = 0; i < sDevices.size(); ++i)
   {
      if (sDevices[i]->deviceType == deviceType && sDevices[i]->torqueInstance == torqueInstance)
         return sDevices[i];
   }
   return NULL;
}

U32 DeviceIdentifier::getDeviceCount(InputDeviceTypes deviceType)
{
   U32 count = 0;
   for (U32 i = 0; i < sDevices.size(); ++i)
   {
      if (sDevices[i]->deviceType == deviceType)
         ++count;
   }
   return count;
}

void DeviceIdentifier::getDevices(InputDeviceTypes deviceType, Vector<DeviceInfo*>& outDevices)
{
   for (U32 i = 0; i < sDevices.size(); ++i)
   {
      if (sDevices[i]->deviceType == deviceType)
         outDevices.push_back(sDevices[i]);
   }
}

void DeviceIdentifier::clear()
{
   for (U32 i = 0; i < sDevices.size(); ++i)
      delete sDevices[i];
   sDevices.clear();
}

//------------------------------------------------------------------------------
// TouchIdentifier
//------------------------------------------------------------------------------

bool TouchIdentifier::acquireSlot(S64 backendFingerId, InputObjectInstances& outSlot)
{
   // Already tracking this finger (e.g. a MOVE after the initial MAKE)?
   // Return its existing slot rather than assigning a new one.
   for (U32 i = 0; i < MaxTouchFingers; ++i)
   {
      if (sTouchSlots[i].inUse && sTouchSlots[i].backendFingerId == backendFingerId)
      {
         outSlot = (InputObjectInstances)(SI_TOUCH_FINGER0 + i);
         return true;
      }
   }

   // First contact for this finger — find a free slot.
   for (U32 i = 0; i < MaxTouchFingers; ++i)
   {
      if (!sTouchSlots[i].inUse)
      {
         sTouchSlots[i].inUse = true;
         sTouchSlots[i].backendFingerId = backendFingerId;
         outSlot = (InputObjectInstances)(SI_TOUCH_FINGER0 + i);
         return true;
      }
   }

   // All MaxTouchFingers slots in use — drop the touch rather than
   // alias it onto an in-use slot (see header comment).
   return false;
}

void TouchIdentifier::releaseSlot(S64 backendFingerId)
{
   for (U32 i = 0; i < MaxTouchFingers; ++i)
   {
      if (sTouchSlots[i].inUse && sTouchSlots[i].backendFingerId == backendFingerId)
      {
         sTouchSlots[i].inUse = false;
         sTouchSlots[i].backendFingerId = 0;
         return;
      }
   }
}

void TouchIdentifier::releaseAll()
{
   for (U32 i = 0; i < MaxTouchFingers; ++i)
   {
      sTouchSlots[i].inUse = false;
      sTouchSlots[i].backendFingerId = 0;
   }
}
