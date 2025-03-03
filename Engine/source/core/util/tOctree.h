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

#ifndef _TOCTREE_H_
#define _TOCTREE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif // !_TDICTIONARY_H_


/// <summary>
/// Octree data structure for combining objects.
/// </summary>
/// <typeparam name="T">Object type to contain in the octree.</typeparam>
template <class T>
class Octree
{
protected:
   struct OctreeNode
   {
      Box3F bounds = Box3F::Zero;                 /// The AABB bounds of this node.
      Vector<T> objs;               /// The list of objects within this node.
      OctreeNode* children[8];      /// The eight child nodes (because octree).
      OctreeNode* parent = NULL;

      struct ObjectEntry
      {
         T object;
         OctreeNode* currentNode;
      };

      OctreeNode();
      OctreeNode(const Box3F& b, OctreeNode* parentNode = NULL);
      ~OctreeNode();

      void subdivide();

      /// <summary>
      /// Checks if this node is a leaf.
      /// </summary>
      /// <returns>True if this node has no children, else false.</returns>
      bool isLeaf() const { return children[0] == NULL; }

      void insert(ObjectEntry& entry, const Box3F& objectBounds);
      void remove(ObjectEntry& entry);
      void query(const Box3F& region, Vector<T>& results) const;
      void queryAll(Vector<T>& results) const;
   };

   Map<T, typename Octree<T>::OctreeNode::ObjectEntry> entryMap;
   OctreeNode* mRootNode = NULL;
   F32 mMinNodeSize;  /// Smallest node size before stopping subdivision
   static constexpr int MAX_OBJECTS = 4; // 4 objects per node.
   static constexpr F32 MIN_NODE_SIZE = 1.0f;
   void expandRoot(const Box3F& objBounds);

public:
   Octree(F32 minNodeSize = 1.0f);
   Octree(F32 minNodeSize = 1.0f, const Box3F& rootBounds = Box3F::Max);
   ~Octree();

   void insert(T obj, const Box3F& objBounds);
   void remove(T object);
   void update(T object, const Box3F& newBounds);
   void query(const Box3F& region, Vector<T>& results) const;
   void queryAll(Vector<T>& results) const;
};

/// <summary>
/// Expand the root node if needed.
/// </summary>
/// <param name="objBounds">The entry bounds that will be expanding the root node.</param>
template<class T>
inline void Octree<T>::expandRoot(const Box3F& objBounds)
{
   mRootNode->bounds.extend(objBounds.minExtents);
   mRootNode->bounds.extend(objBounds.maxExtents);
}

/// <summary>
/// Default constructor
/// </summary>
/// <param name="minNodeSize">Smallest node size allowed.</param>
template<class T>
inline Octree<T>::Octree(F32 minNodeSize)
   : mMinNodeSize(minNodeSize)
{
   mRootNode = new OctreeNode(Box3F::Max);
}

/// <summary>
/// Default constructor with bounds parameter.
/// </summary>
/// <param name="minNodeSize">Smallest node size allowed.</param>
/// <param name="rootBounds">The size of the root node.</param>
template<class T>
inline Octree<T>::Octree(F32 minNodeSize, const Box3F& rootBounds)
   : mMinNodeSize(minNodeSize)
{
   mRootNode = new OctreeNode(rootBounds);
}

/// <summary>
/// Deconstructor
/// </summary>
template<class T>
inline Octree<T>::~Octree()
{
   SAFE_DELETE(mRootNode);
}

/// <summary>
/// Insert an entry into the octree
/// </summary>
/// <param name="obj">The entry to insert.</param>
/// <param name="objBounds">The objects bounds in world space.</param>
template<class T>
inline void Octree<T>::insert(T obj, const Box3F& objBounds)
{
   if (!mRootNode)
   {
      mRootNode = new OctreeNode(objBounds);
   }
   else
   {
      // if we dont fully contain the bounds, expand our root node.
      if (!mRootNode->bounds.isContained(objBounds))
      {
         expandRoot(objBounds);
      }
   }

   // calls findOrInsert on the underlying hashtable.
   OctreeNode::ObjectEntry& entry = entryMap[obj];
   entry.object = obj;

   mRootNode->insert(entry, objBounds);
}

/// <summary>
/// Remove an entry from the octree.
/// </summary>
/// <param name="entry">The entry to be removed.</param>
template<class T>
inline void Octree<T>::remove(T object)
{
   if (!mRootNode)
      return;

   // not in map, gtfo.
   if (!entryMap.contains(object))
      return;

   // else lets go.
   OctreeNode::ObjectEntry& entry = entryMap[object];

   // remove from its node.
   entry.currentNode->remove(entry);

   // remove from our map.
   entryMap.erase(object);
}

template<class T>
inline void Octree<T>::update(T object, const Box3F& newBounds)
{
   // no root node, get out!
   if (!mRootNode)
      return;

   // not in map, gtfo.
   if (!entryMap.contains(object))
      return;

   // else lets go.
   OctreeNode::ObjectEntry& entry = entryMap[object];

   // remove from its current node.
   entry.currentNode->remove(entry);

   // reinsert to its current node, this should automatically move it up or down
   // based on the new bounds.
   entry.currentNode->insert(entry, newBounds);
}

template<class T>
inline void Octree<T>::query(const Box3F& region, Vector<T>& results) const
{
   if (mRootNode)
      mRootNode->query(region, results);
}

template<class T>
inline void Octree<T>::queryAll(Vector<T>& results) const
{
   if(mRootNode)
      mRootNode->queryAll(results);
}

///-------------------------------------------------------------------
/// OctreeNode functions
///-------------------------------------------------------------------
template<class T>
inline Octree<T>::OctreeNode::OctreeNode()
{
   parent = NULL;
   // Reserve enough room for our max objects.
   objs.reserve(MAX_OBJECTS);

   for (U32 i = 0; i < 8; i++)
   {
      children[i] = NULL;
   }
}
/// <summary>
/// Default constructor.
/// </summary>
/// <param name="b">The bounds for this node.</param>
template<class T>
inline Octree<T>::OctreeNode::OctreeNode(const Box3F& b, OctreeNode* parentNode)
{
   bounds = b;
   for (U32 i = 0; i < 8; i++)
   {
      children[i] = NULL;
   }

   parent = parentNode;

   // Reserve enough room for our max objects.
   objs.reserve(MAX_OBJECTS);
}

/// <summary>
/// Deconstructor
/// </summary>
template<class T>
inline Octree<T>::OctreeNode::~OctreeNode()
{
   // clear our objects.
   objs.clear();

   // clear any child nodes.
   for (U32 i = 0; i < 8; i++)
   {
      SAFE_DELETE(children[i]);
   }

   if(parent) parent = NULL;
}

/// <summary>
/// Subdivide this node.
/// </summary>
template<class T>
inline void Octree<T>::OctreeNode::subdivide()
{
   Point3F center = bounds.getCenter();
   for (int i = 0; i < 8; i++)
   {
      Point3F min = center;
      Point3F max = bounds.maxExtents;

      if (i & 1) min.x = bounds.minExtents.x;
      if (i & 2) min.y = bounds.minExtents.y;
      if (i & 4) min.z = bounds.minExtents.z;

      if (!(i & 1)) max.x = center.x;
      if (!(i & 2)) max.y = center.y;
      if (!(i & 4)) max.z = center.z;

      children[i] = new OctreeNode(Box3F(min, max));
   }

}

/// <summary>
/// Insert an entry from the octree node, subdivides the node if its obj vector is the max_objects size.
/// </summary>
/// <param name="entry">The entry to be removed.</param>
template<class T>
inline void Octree<T>::OctreeNode::insert(ObjectEntry& entry, const Box3F& objectBounds)
{
   // If object is not fully contained in this node, keep it in the parent
   if (!bounds.isContained(objectBounds))
   {
      if (parent)
      {
         parent->insert(entry, objectBounds);
      }
      return;
   }

   // If this is a leaf and has space, add object here
   if (isLeaf() && objs.size() < MAX_OBJECTS)
   {
      entry.currentNode = this;
      objs.push_back(entry.object);
      return;
   }

   if (isLeaf())
      subdivide();

   bool insertedIntoChild = false;

   // Try to insert into children
   for (OctreeNode* node : children)
   {
      if (node->bounds.isContained(objectBounds))
      {
         node->insert(entry, objectBounds);
         insertedIntoChild = true;
         break;
      }
   }

   // If the object spans multiple nodes, keep it in the parent
   if (!insertedIntoChild)
   {
      entry.currentNode = this;
      objs.push_back(entry.object);
   }
}

/// <summary>
/// Remove an entry from the octree node.
/// </summary>
/// <param name="entry">The entry to be removed.</param>
template<class T>
inline void Octree<T>::OctreeNode::remove(ObjectEntry& entry)
{
   // Current node is the first node that this object exists in.
   // objects can be big enough to exist in multiple nodes.
   if (entry.currentNode)
   {
      objs.remove(entry.object);
      if (!isLeaf())
      {
         for (OctreeNode* node : children)
         {
            node->remove(entry);
         }
      }
   }
}

template<class T>
inline void Octree<T>::OctreeNode::query(const Box3F& region, Vector<T>& results) const
{
   // if we are not overlapped skip.
   if (!bounds.isOverlapped(region))
      return;

   // add every entry in this node to the results.
   for (const T& obj : objs)
   {
      results.push_back_unique(obj);
   }

   // if we have children loop them.
   if (!isLeaf())
   {
      for (OctreeNode* node : children)
      {
         if(node && node->bounds.isOverlapped(region))
            node->query(region, results);
      }
   }
}

template<class T>
inline void Octree<T>::OctreeNode::queryAll(Vector<T>& results) const
{
   for (const T& obj : objs)
   {
      // obj can exist on multiple nodes, if it is big enough.
      results.push_back_unique(obj);
   }

   // if we have children loop them.
   if (!isLeaf())
   {
      for (OctreeNode* node : children)
      {
         node->queryAll(results);
      }
   }
}


#endif // !_TOCTREE_H_
