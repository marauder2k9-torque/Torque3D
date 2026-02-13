#pragma once

#ifndef _TBVH_H_
#define _TBVH_H_

#ifndef _TSPATIALSTRUCTURE_H_
#include "core/util/tSpatialStructure.h"
#endif // !_TSPATIALSTRUCTURE_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif // !_MATH_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif

#ifndef _COLLISION_H_
#include "collision/collision.h"
#endif

/// Note: This proxy class is useful and i have only ever seen box bounds used for bvh,
/// but we could / maybe should, allow the use of other types such as spheres.
struct BVHNode; // forward declaration.
/// <summary>
/// Proxy for interfacing with the bvh, objects that are included in a bvh should
/// derive from this and make sure they overload the functions below
/// </summary>
class BVHProxy
{
public:
   virtual ~BVHProxy() = default;

   /// <summary>
   /// The function to call to return the bounds of this object
   /// to build the node bounds. Dynamic objects should add
   /// a margin and expand by their velocity. Static objects
   /// should just add a margin.
   /// </summary>
   /// <returns>The Box3F bounds.</returns>
   virtual Box3F getBounds() const = 0;

   /// <summary>
   /// Get the node this object is in.
   /// </summary>
   /// <returns>The node that owns the object.</returns>
   virtual BVHNode* getBVHNode() const = 0;

   /// <summary>
   /// The simple collision geometry ray cast.
   /// </summary>
   /// <param name="start">Start of the ray.</param>
   /// <param name="end">End of the ray.</param>
   /// <param name="info">The ray info for this ray test.</param>
   /// <returns>True if the ray hit.</returns>
   virtual bool castRay(const Point3F& start, const Point3F& end, RayInfo* info) const = 0;

   /// <summary>
   /// The more costly rendered geometry ray cast.
   /// </summary>
   /// <param name="start">Start of the ray.</param>
   /// <param name="end">End of the ray.</param>
   /// <param name="info">The ray info for this ray test.</param>
   /// <returns>True if the ray hit.</returns>
   virtual bool castRayRendered(const Point3F& start, const Point3F& end, RayInfo* info) const = 0;
};

struct BVHNode
{
   Box3F bounds = Box3F::Invalid;

   BVHNode* parent = NULL;
   BVHNode* left = NULL;
   BVHNode* right = NULL;

   BVHProxy* object = NULL;

   bool isLeaf()
   {
      // just a safety to make sure this is cleared.
      if (left || right)
         object = NULL;

      return left == NULL && right == NULL;
   }
};

class BVH
{
public:
   using Node = BVHNode;

   Node* mRoot = NULL;

public:
   //------------------------------------------------
   // CORE FUNCTIONALITY FUNCTIONS
   //------------------------------------------------

   Box3F merge(const Box3F& a, const Box3F& b)
   {
      Box3F r = a;
      r.extend(b.minExtents);
      r.extend(b.maxExtents);
      return r;
   }

   F32 mergeCost(const Box3F& a, const Box3F& b)
   {
      Box3F merged = merge(a, b);
      return merged.len();
   }

   F32 trySwap(Node* parent, Node* a, Node* b, F32 bestCost, Node*& bestA, Node*& bestB)
   {
      F32 cost = mergeCost(a->bounds, b->bounds);

      if (cost < bestCost)
      {
         bestCost = cost;
         bestA = a;
         bestB = b;
      }

      return bestCost;
   }

   bool rotateLeftIfBetter(Node* P)
   {
      Node* A = P->left;
      Node* B = P->right;
      if (!A || !B || A->isLeaf())
         return false;

      Node* C = A->left;
      Node* D = A->right;

      F32 costBefore = P->bounds.len();

      Box3F newPBound = merge(D->bounds, B->bounds);
      Box3F newABound = merge(C->bounds, newPBound);

      F32 costAfter = newABound.len();

      if (costAfter >= costBefore)
         return false;

      // Perform rotation
      A->left = C;
      A->right = P;
      C->parent = A;
      A->object = NULL;

      P->left = D;
      P->right = B;
      D->parent = P;
      B->parent = P;
      P->object = NULL;

      // Fix parents
      A->parent = P->parent;
      if (P->parent)
      {
         if (P->parent->left == P)
            P->parent->left = A;
         else
            P->parent->right = A;
      }
      else
      {
         mRoot = A;
      }

      P->parent = A;

      P->bounds = merge(D->bounds, B->bounds);
      A->bounds = merge(C->bounds, P->bounds);

      return true;
   }

   bool rotateRightIfBetter(Node* P)
   {
      Node* A = P->left;
      Node* B = P->right;
      if (!A || !B || B->isLeaf())
         return false;

      Node* C = B->left;
      Node* D = B->right;

      F32 costBefore = P->bounds.len();

      Box3F newPBound = merge(A->bounds, C->bounds);
      Box3F newBBound = merge(newPBound, D->bounds);

      F32 costAfter = newBBound.len();

      if (costAfter >= costBefore)
         return false;

      // Perform rotation
      B->left = C;
      B->right = P;
      C->parent = B;
      B->object = NULL;

      P->left = A;
      P->right = C;
      A->parent = P;
      C->parent = P;
      P->object = NULL;

      // Fix parents
      B->parent = P->parent;
      if (P->parent)
      {
         if (P->parent->left == P)
            P->parent->left = B;
         else
            P->parent->right = B;
      }
      else
      {
         mRoot = B;
      }

      P->parent = B;

      P->bounds = merge(A->bounds, C->bounds);
      B->bounds = merge(P->bounds, D->bounds);

      return true;
   }

   void tryRotate(Node* node)
   {
      if (!node || node->isLeaf())
         return;

      if (rotateLeftIfBetter(node))
         return;

      rotateRightIfBetter(node);
   }

   void rotateUpwards(Node* node)
   {
      while (node)
      {
         if (!node->isLeaf())
            tryRotate(node);
         node = node->parent;
      }
   }

   void refitUpwards(Node* node)
   {
      while (node)
      {
         if (!node->isLeaf())
         {
            node->bounds = merge(node->left->bounds, node->right->bounds);
         }
         node = node->parent;
      }
   }

   Node* chooseBestSibling(Node* start, const Box3F& leafBounds)
   {
      Node* node = start;

      while (!node->isLeaf())
      {
         F32 costLeft = mergeCost(node->left->bounds, leafBounds);
         F32 costRight = mergeCost(node->right->bounds, leafBounds);

         node = (costLeft < costRight) ? node->left : node->right;
      }

      return node;
   }

   BVH() = default;
   ~BVH() {
      destroyRecursive(mRoot);
   }

   void destroyRecursive(Node* node)
   {
      if (!node) return;
      destroyRecursive(node->left);
      destroyRecursive(node->right);
      delete node;
   }

   //------------------------------------------------
   // LEAF FUNCTIONS
   //------------------------------------------------

   Node* createLeaf(BVHProxy* obj)
   {
      if (!obj)
         return NULL;

      Node* node = new Node();
      node->bounds = obj->getBounds();
      node->object = obj;

      return node;
   }

   void insertLeaf(Node* leaf)
   {
      if (!mRoot)
      {
         mRoot = leaf;
         leaf->parent = NULL;
         return;
      }

      // 1. Find sibling
      Node* sibling = chooseBestSibling(mRoot, leaf->bounds);

      // 2. Create new parent
      Node* oldParent = sibling->parent;
      Node* newParent = new Node();

      newParent->parent = oldParent;
      newParent->left = sibling;
      newParent->right = leaf;
      newParent->object = NULL;

      newParent->bounds = merge(sibling->bounds, leaf->bounds);

      sibling->parent = newParent;
      leaf->parent = newParent;

      // 3. Patch parent link
      if (oldParent)
      {
         if (oldParent->left == sibling)
            oldParent->left = newParent;
         else
            oldParent->right = newParent;
      }
      else
      {
         mRoot = newParent;
      }

      // 4. Refit upwards
      refitUpwards(newParent);
      rotateUpwards(newParent);
   }

   void removeLeaf(Node* leaf)
   {
      if (leaf == mRoot)
      {
         mRoot = NULL;
         return;
      }

      Node* parent = leaf->parent;
      Node* grandParent = parent->parent;

      Node* sibling = (parent->left == leaf) ? parent->right : parent->left;

      if (grandParent)
      {
         // Replace parent with sibling
         if (grandParent->left == parent)
            grandParent->left = sibling;
         else
            grandParent->right = sibling;

         sibling->parent = grandParent;

         refitUpwards(grandParent);
      }
      else
      {
         // Parent was root
         mRoot = sibling;
         sibling->parent = NULL;
      }

      delete parent;
   }

   void updateLeaf(Node* leaf)
   {
      if (!leaf->isLeaf())
         return; // something bad has happened.

      Box3F objB = leaf->object->getBounds();

      if (leaf->bounds.isContained(objB))
         return;

      removeLeaf(leaf);
      leaf->bounds = objB;
      insertLeaf(leaf);
   }

   //------------------------------------------------
   // QUERY FUNCTIONS
   //------------------------------------------------

   Vector<BVHProxy*> queryRegion(const Box3F& region)
   {
      if (!mRoot)
         return NULL;

      Vector<BVHProxy*> prox_vec;
      queryFromNode(region, mRoot, &prox_vec);

      return prox_vec;
   }

   void queryFromNode(const Box3F& region, Node* node, Vector< BVHProxy* >* outFound)
   {
      if (!node)
         return;

      if (!node->bounds.isOverlapped(region))
         return;

      if (node->isLeaf())
      {
         outFound->push_back(node->object);
         return;
      }

      queryFromNode(region, node->left, outFound);
      queryFromNode(region, node->right, outFound);
   }
};

#endif // !_TBVH_H_
