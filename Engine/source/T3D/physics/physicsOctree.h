#ifndef _OCTREE_H_
#define _OCTREE_H_

#ifndef _DYNAMIC_CONSOLETYPES_H_
#include "console/dynamicTypes.h"
#endif

#ifndef _PHYSICSSHAPE_H_
#include "T3D/physics/physicsShape.h"
#endif

namespace Octree {

   enum class Octant : U8 {
      q1 = 0x01,
      q2 = 0x02,
      q3 = 0x04,
      q4 = 0x08,
      q5 = 0x10,
      q6 = 0x20,
      q7 = 0x40,
      q8 = 0x80
   };

   class Node {
   public:
      Node* parent;
      Node* child[8];
      U8 activeOct;
      bool hasChildren = false;
      bool treeReady = false;
      bool treeBuilt = false;

      Vector<PhysicsShape> obj;

   };


}

#endif // !_OCTREE_H_
