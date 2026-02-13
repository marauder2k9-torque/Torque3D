#pragma once

#ifndef _TSPATIALSTRUCTURE_H_
#define _TSPATIALSTRUCTURE_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif // !_MATH_H_

// forward declaration.
template <class BoundsT>
struct SpatialNodeT;

/// <summary>
/// Base Proxy class for use with spatial structures.
/// </summary>
/// <typeparam name="BoundsT">The bounds type eg: Box3F or RectF</typeparam>
template <typename BoundsT>
class SpatialProxyT
{
public:
   using NodeBounds = BoundsT;
   using NodeType = SpatialNodeT<BoundsT>;

   virtual ~SpatialProxyT() = default;
   virtual BoundsT getBounds() const = 0;
   virtual NodeType* getNode() const = 0;
};

/// <summary>
/// Base Node type class for use with spatial structures.
/// This represents a single node, derivatives from this must implement
/// their own logic.
/// </summary>
/// <typeparam name="BoundsT">The bounds type eg: Box3F or RectF</typeparam>
template <typename BoundsT>
struct SpatialNodeT
{
public:
   using NodeBounds = BoundsT;
   BoundsT bounds;
   SpatialNodeT* parent = NULL;
   SpatialProxyT<BoundsT>* object = NULL;
};

/// <summary>
/// Spatial structure base class to be derived from for spatial structures. This is setup with a CRTP
/// structure to avoid as many virtual functions as possible.
/// Classes that are derived from this are able to specify their own logic but they MUST
/// contain function definitions for insertImpl, updateImpl and removeImpl
/// </summary>
/// <typeparam name="Derived">The class that will be deriving from this eg BVH.</typeparam>
/// <typeparam name="NodeT">The SpatialNodeT derived class.</typeparam>
template <class Derived, class NodeT>
class SpatialStructureT
{
public:
   using Node = NodeT;
   using BoundsType = typename Node::NodeBounds;

protected:
   Node* mRoot = NULL;

public:
   ~SpatialStructureT()
   {
      destroyRecursive(mRoot);
   }

   void insert(SpatialProxyT<BoundsType>* obj)
   {
      static_cast<Derived*>(this)->insertImpl(obj);
   }

   void remove(SpatialProxyT<BoundsType>* obj)
   {
      static_cast<Derived*>(this)->removeImpl(obj);
   }

   void update(SpatialProxyT<BoundsType>* obj)
   {
      static_cast<Derived*>(this)->updateImpl(obj);
   }
protected:
   void destroyRecursive(Node* node)
   {
      if (!node) return;
      static_cast<Derived*>(this)->destroyChildren(node);
      delete node;
   }

   virtual void destroyChildren(Node* node) = 0;
};

#endif // !_TSPATIALSTRUCTURE_H_
