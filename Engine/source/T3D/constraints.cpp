#include "constraints.h"

Spring::Spring(Resource<TSShape> inShape, S32 inBoneA, S32 inBoneB)
{
   shape = inShape;

   // found nodes create our rigids.
   if (shape->findNode(inBoneA) != -1 && shape->findNode(inBoneB) != -1)
   {
      boneA = inBoneA;
      boneB = inBoneB;

      MatrixF mat;

      // set body a's transform to bonea
      shape->getNodeWorldTransform(boneA, &mat);
      bodyA.setTransform(mat);
      bodyA.mass = 1.0f;

      // set body b's transform to boneb
      shape->getNodeWorldTransform(boneB, &mat);
      bodyB.setTransform(mat);
      bodyB.mass = 1.0f;
   }

   restLength = 1.0f;
   stiffness = 0.1f;

   // default to no anchor.
   anchor = -1;
}

Spring::Spring(Resource<TSShape> inShape, S32 inBoneA, S32 inBoneB, F32 inStiffness, F32 inRestlength)
   : Spring(inShape, inBoneA, inBoneB)
{
   restLength = inRestlength;
   stiffness = inStiffness;
}

void Spring::setStiffness(F32 inStiff)
{
   if (stiffness != inStiff)
      stiffness = inStiff;
}

void Spring::setAnchor(S32 boneId)
{
   // set anchor
   if (anchor != boneId)
      anchor = boneId;
}

void Spring::update(F32 delta)
{
   // clear rigd body forces.
   bodyA.clearForces();
   bodyB.clearForces();

   // set vector positions.
   MatrixF mat;
   shape->getNodeWorldTransform(boneA, &mat);
   VectorF posA = mat.getPosition();

   shape->getNodeWorldTransform(boneB, &mat);
   VectorF posB = mat.getPosition();

   // pos B - pos A
   VectorF force = posB - posA;

   // x  = displacement.
   F32 x = force.magnitudeSafe() - restLength;

   force.normalize();
   force *= stiffness * x;

   bodyA.force.set(force);

   force *= -1;

   bodyB.force.set(force);

   // now our rigids should be updated with the forces and all interpolation is handled by rigid.
   // we do not apply updates to the anchor it remains where it is. Sometimes we may want to
   // change the orientation of the anchor though.
   if (anchor != boneA)
   {
      // integrate forces.
      bodyA.integrate(delta);

      MatrixF matA;
      bodyA.getTransform(&matA);

      Point3F pos(matA.getPosition());
      QuatF rot(matA);

      // this is world space transform, to effect the node, translate to object space.
      if (shape->nodes[boneA].parentIndex != -1)
      {
         shape->getNodeWorldTransform(shape->nodes[boneA].parentIndex, &mat);

         // Pre-multiply by inverse of parent's world transform to get
         // local node transform
         mat.inverse();
         mat.mul(matA);

         rot.set(mat);
         pos = mat.getPosition();
      }

      shape->setNodeTransform(boneA, pos, rot);

   }

   if (anchor != boneB)
   {
      // integrate forces.
      bodyB.integrate(delta);

      MatrixF matB;
      bodyB.getTransform(&matB);

      Point3F pos(matB.getPosition());
      QuatF rot(matB);

      // this is world space transform, to effect the node, translate to object space.
      if (shape->nodes[boneB].parentIndex != -1)
      {
         shape->getNodeWorldTransform(shape->nodes[boneB].parentIndex, &mat);

         // Pre-multiply by inverse of parent's world transform to get
         // local node transform
         mat.inverse();
         mat.mul(matB);

         rot.set(mat);
         pos = mat.getPosition();
      }

      shape->setNodeTransform(boneB, pos, rot);
   }

}
