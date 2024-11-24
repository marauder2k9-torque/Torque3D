#pragma once
#ifndef _GFXDEVICE_H_
#define _GFXDEVICE_H_

#ifndef _PLATFORM_PLATFORMTIMER_H_
#include "platform/platformTimer.h"
#endif

#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif

#ifndef _TSIGNAL_H_
#include "core/util/tSignal.h"
#endif

#ifndef _MATHUTIL_FRUSTUM_H_
#include "math/util/frustum.h"
#endif

#ifndef _REFBASE_H_
#include "core/util/refBase.h"
#endif

#ifndef _GFXSTRUCTS_H_
#include "gfx2/gfxStructs.h"
#endif // !_GFXSTRUCTS_H_

#ifndef _GFXENUMS_H_
#include "gfx2/gfxEnums.h"
#endif


class GFXDevice : public StrongRefBase
{
public:

   enum GFXDeviceEventType
   {
      /// The device has been created, but not initialized
      deCreate,

      /// The device has been initialized
      deInit,

      /// The device is about to be destroyed.
      deDestroy,

      /// The device has started rendering a frame
      deStartOfFrame,

      /// The device is about to finish rendering a frame
      deEndOfFrame,

      /// The device has rendered a frame and ended the scene
      dePostFrame,

      /// The device has started rendering a frame's field (such as for side-by-side rendering)
      deStartOfField,

      /// The device is about to finish rendering a frame's field
      deEndOfField,
   };

   using GFXDeviceSignal = Signal<bool(GFXDeviceEventType)>;
   static GFXDeviceSignal& getDeviceEventSignal();

public:
   GFXDevice();
   virtual ~GFXDevice();
};

#endif // _GFXDEVICE_H_
