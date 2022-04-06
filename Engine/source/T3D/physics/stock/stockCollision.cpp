#include "platform/platform.h"
#include "T3D/physics/stock/stockCollision.h"

#include "math/mPoint3.h"
#include "math/mMatrix.h"
#include "scene/sceneObject.h"

//
StockCollision::StockCollision() :
   mLocalXfm(true),
   mObject(NULL)
{
}

StockCollision::~StockCollision()
{
   mObject = NULL;
}

void StockCollision::setObject(SceneObject* obj)
{
}

//
void StockCollision::addPlane(const PlaneF& plane)
{
}

void StockCollision::addBox(const Point3F& halfWidth, const MatrixF& localXfm)
{
}

void StockCollision::addSphere(F32 radius, const MatrixF& localXfm)
{

}

void StockCollision::addCapsule(F32 radius, F32 height, const MatrixF& localXfm)
{

}

bool StockCollision::addConvex(const Point3F* points, U32 count, const MatrixF& localXfm)
{

   return true;
}

bool StockCollision::addTriangleMesh(const Point3F* vert, U32 vertCount, const U32* index, U32 triCount, const MatrixF& localXfm)
{
   return false;
}

bool StockCollision::addHeightfield(const U16* heights, const bool* holes, U32 blockSize, F32 metersPerSample, const MatrixF& localXfm)
{
   return false;
}
