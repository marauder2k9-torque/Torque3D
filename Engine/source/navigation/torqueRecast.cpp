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
#include "navigation/torqueRecast.h"

// This file just sets up the recast api for interaction with initpersistfields etc.

/// <summary>
/// This enum is used to calculate the cost of a specific tile based on its type.
/// Add to it whatever other types you need, but anywhere this is used be sure to update that.
/// (Check NavMeshTesterTool for example of setting costs.)
/// </summary>
ImplementEnumType(PolyAreaType, "NavMesh PolyArea type.\n\n")
{ PolyAreas::GroundArea,   "Ground",   "..." },
{ PolyAreas::WaterArea,    "Water",    "..." },
{ PolyAreas::RoadArea,     "Road",     "..." },
{ PolyAreas::DoorArea,     "Door",     "..." },
{ PolyAreas::GrassArea,    "Grass",    "..." },
{ PolyAreas::OffMeshArea,  "OffMesh",  "..." }
EndImplementEnumType;

/// <summary>
/// These are built in partitioning alorithms used by recast. 
/// </summary>
ImplementEnumType(NavigationPartitionType, "NavMesh Partitioning algorithm.\n\n")
{ NavigationPartition::PartitionWaterShed,"WaterShed",   "..." },
{ NavigationPartition::PartitionMonotone, "Monotone",    "..." },
{ NavigationPartition::PartitionLayers,   "Layers",      "..." }
EndImplementEnumType;

/// <summary>
/// This bitfield should be added to for specific types that are required in your own implementation.
/// at the moment the basics are covered.
/// </summary>
ImplementBitfieldType(PolyFlagType,
   "The flags attributed to this path or navmesh tile.\n"
   "@ingroup Navigation\n\n")
{ PolyFlags::WalkFlag,     "$WalkFlag",      "Tile is walkable.\n" },
{ PolyFlags::SwimFlag,     "$SwimFlag",      "Tile is swimmable.\n" },
{ PolyFlags::JumpFlag,     "$JumpFlag",      "Tile is for jumping.\n" },
{ PolyFlags::LedgeFlag,    "$LedgeFlag",     "Tile is a ledge.\n" },
{ PolyFlags::DropFlag,     "$DropFlag",      "Tile is a drop.\n" },
{ PolyFlags::ClimbFlag,    "$ClimbFlag",     "Tile is a climb.\n" },
{ PolyFlags::TeleportFlag, "$TeleportFlag",  "Tile is a telporter.\n" },
{ PolyFlags::DoorFlag,     "$DoorFlag",      "Tile is a door.\n" },
{ PolyFlags::DisabledFlag, "$DisabledFlag",  "Tile is disabled.\n" }
EndImplementBitfieldType;
