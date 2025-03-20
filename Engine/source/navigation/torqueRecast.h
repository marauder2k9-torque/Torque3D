//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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

#ifndef _TORQUE_RECAST_H_
#define _TORQUE_RECAST_H_

#ifndef _ENGINEAPI_H_
#include "console/engineAPI.h"
#endif

#ifndef _SIMSET_H_
#include "console/simSet.h"
#endif

#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif

#ifndef _MBOX_H_
#include "math/mBox.h"
#endif

inline Point3F DTStoRC(F32 x, F32 y, F32 z) { return Point3F(x, z, -y); }
inline Point3F DTStoRC(const Point3F& point){ return Point3F(point.x, point.z, -point.y); }
inline Point3F RCtoDTS(const F32* xyz)      { return Point3F(xyz[0], -xyz[2], xyz[1]); }
inline Point3F RCtoDTS(F32 x, F32 y, F32 z) { return Point3F(x, -z, y); }
inline Point3F RCtoDTS(const Point3F& point){ return Point3F(point.x, -point.z, point.y); }
inline Box3F DTStoRC(const Box3F& box)
{
   return Box3F(box.minExtents.x, box.minExtents.z, -box.maxExtents.y,
                  box.maxExtents.x, box.maxExtents.z, -box.minExtents.y);
}
inline Box3F RCtoDTS(const F32 *min, const F32 *max)
{
   return Box3F(min[0], -max[2], min[1], max[0], -min[2], max[1]);
}

/// Convert a Rcast colour integer to RGBA components.
inline void rcCol(unsigned int col, U8 &r, U8 &g, U8 &b, U8 &a)
{
   r = col % 256; col /= 256;
   g = col % 256; col /= 256;
   b = col % 256; col /= 256;
   a = col % 256;
}

enum NavigationTool {
   EmptyTool = 0,
   TileEditTool,
   TileHighlightTool,
   TempObstacleTool,
   NavmeshTesterTool,
   NavmeshPruneTool,
   OffmeshConnectionTool,
   ConvexVolumeTool,
   CrowdTool,
   NumTools
};

enum PolyAreas {
   GroundArea,
   WaterArea,
   RoadArea,
   DoorArea,
   GrassArea,
   OffMeshArea,
   NumAreas
};

enum NavigationPartition {
   PartitionWaterShed,
   PartitionMonotone,
   PartitionLayers
};

enum PolyFlags {
   WalkFlag      = BIT(0),
   SwimFlag      = BIT(1),
   JumpFlag      = BIT(2),
   LedgeFlag     = BIT(3),
   DropFlag      = BIT(4),
   ClimbFlag     = BIT(5),
   TeleportFlag  = BIT(6),
   DoorFlag      = BIT(7),
   DisabledFlag  = BIT(8),
   AllFlags      = 0xffff
};

/// Stores information about a link.
/// Deprecated!
struct LinkData {
   bool walk;
   bool jump;
   bool drop;
   bool swim;
   bool ledge;
   bool climb;
   bool teleport;
   LinkData(U16 flags = 0)
   {
      walk = flags & WalkFlag;
      jump = flags & JumpFlag;
      drop = flags & DropFlag;
      swim = flags & SwimFlag;
      ledge = flags & LedgeFlag;
      climb = flags & ClimbFlag;
      teleport = flags & TeleportFlag;
   }
   U16 getFlags() const
   {
      return
         (walk ? WalkFlag : 0) |
         (jump ? JumpFlag : 0) |
         (drop ? DropFlag : 0) |
         (swim ? SwimFlag : 0) |
         (ledge ? LedgeFlag : 0) |
         (climb ? ClimbFlag : 0) |
         (teleport ? TeleportFlag : 0);
   }
};

typedef PolyAreas PolyAreaType;
DefineEnumType(PolyAreaType)

typedef NavigationPartition NavigationPartitionType;
DefineEnumType(NavigationPartitionType)

typedef PolyFlags PolyFlagType;
DefineBitfieldType(PolyFlagType)

#endif
