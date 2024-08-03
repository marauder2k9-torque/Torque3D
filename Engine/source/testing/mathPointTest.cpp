#include "testing/unitTesting.h"

#include "platform/platform.h"
#include "console/simBase.h"
#include "console/consoleTypes.h"
#include "console/scriptObjects.h"
#include "console/simBase.h"
#include "console/engineAPI.h"
#include "math/mMath.h"
#include "math/util/frustum.h"
#include "math/mathUtils.h"

typedef PointT<F32, 3> PointT3F;
typedef PointT<F32, 2> PointT2F;

TEST(PointTest, TestInitFunctions)
{
   Point3F test;
   EXPECT_NEAR(test.x, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.0f, POINT_EPSILON);

   Point3F test2(1.0f);
   EXPECT_NEAR(test2.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test2.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test2.z, 1.0f, POINT_EPSILON);

   Point3F test3(test2);
   EXPECT_NEAR(test3.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test3.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test3.z, 1.0f, POINT_EPSILON);

   Point3F test4(1.0f, 0.5f, 0.25f);
   EXPECT_NEAR(test4.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test4.y, 0.5f, POINT_EPSILON); EXPECT_NEAR(test4.z, 0.25f, POINT_EPSILON);
}

TEST(TemplatePointTest, TestInitFunctions)
{
   PointT3F test;
   EXPECT_NEAR(test.x, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.0f, POINT_EPSILON);

   PointT3F test2(1.0f);
   EXPECT_NEAR(test2.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test2.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test2.z, 1.0f, POINT_EPSILON);

   PointT3F test3(test2);
   EXPECT_NEAR(test3.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test3.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test3.z, 1.0f, POINT_EPSILON);

   PointT3F test4(1.0f, 0.5f, 0.25f);
   EXPECT_NEAR(test4.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test4.y, 0.5f, POINT_EPSILON); EXPECT_NEAR(test4.z, 0.25f, POINT_EPSILON);

   PointT2F test5(1.0f, 1.0f);
   PointT3F test53(test5);
   EXPECT_NEAR(test53.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test53.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test53.z, 0.0f, POINT_EPSILON);
}

TEST(PointTest, TestSetFunctions)
{
   Point3F test;
   EXPECT_NEAR(test.x, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.0f, POINT_EPSILON);

   test.set(1.0f);
   EXPECT_NEAR(test.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 1.0f, POINT_EPSILON);

   Point3F test2(1.0f, 0.5f, 0.25f);
   test.set(test2);
   EXPECT_NEAR(test.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.5f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.25f, POINT_EPSILON);

   test.set(0.25f, 1.0f, 0.5f);
   EXPECT_NEAR(test.x, 0.25f, POINT_EPSILON); EXPECT_NEAR(test.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.5f, POINT_EPSILON);

   test.zero();
   EXPECT_NEAR(test.x, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.0f, POINT_EPSILON);
}

TEST(TemplatePointTest, TestSetFunctions)
{
   PointT3F test;
   EXPECT_NEAR(test.x, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.0f, POINT_EPSILON);

   test.set(1.0f);
   EXPECT_NEAR(test.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 1.0f, POINT_EPSILON);

   PointT3F test2(1.0f, 0.5f, 0.25f);
   test.set(test2);
   EXPECT_NEAR(test.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.5f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.25f, POINT_EPSILON);

   PointT2F test22(1.0f, 1.0f);
   test.set(test22);
   EXPECT_NEAR(test.x, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.0f, POINT_EPSILON);

   test.set(0.25f, 1.0f, 0.5f);
   EXPECT_NEAR(test.x, 0.25f, POINT_EPSILON); EXPECT_NEAR(test.y, 1.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.5f, POINT_EPSILON);

   test.zero();
   EXPECT_NEAR(test.x, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.y, 0.0f, POINT_EPSILON); EXPECT_NEAR(test.z, 0.0f, POINT_EPSILON);
}
