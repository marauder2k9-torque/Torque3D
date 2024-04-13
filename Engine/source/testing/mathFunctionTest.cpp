//-----------------------------------------------------------------------------
// Copyright (c) 2014 GarageGames, LLC
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

#include "testing/unitTesting.h"
#include "math/mMathFn.h"

/// <summary>
/// Tests for mMax
/// </summary>
TEST(MathFunctionTest, MAX)
{
   U32 uTest = mMax(2, 10);
   EXPECT_EQ(uTest, 10) << "U32 test did not return correct value";

   S32 sTest = mMax(-2, 10);
   EXPECT_EQ(sTest, 10) << "S32 test did not return correct value";

   F32 fTest = mMax(0.5f, 10.5f);
   EXPECT_TRUE(mIsEqual(fTest, 10.5f, 0.0001f)) << "F32 test did not return true within eps.";

   F64 dTest = mMax(0.5, 10.5);
   EXPECT_EQ(dTest, 10.5) << "F64 test did not return the correct value";
}

/// <summary>
/// Test for mMin
/// </summary>
TEST(MathFunctionTest, MIN)
{
   U32 uTest = mMin(2, 10);
   EXPECT_EQ(uTest, 2) << "U32 test did not return correct value";

   S32 sTest = mMin(-2, 10);
   EXPECT_EQ(sTest, -2) << "S32 test did not return correct value";

   F32 fTest = mMin(0.5f, 10.5f);
   EXPECT_TRUE(mIsEqual(fTest, 0.5f, 0.0001f)) << "F32 test did not return true within eps.";

   F64 dTest = mMin(0.5, 10.5);
   EXPECT_EQ(dTest, 0.5) << "F64 test did not return the correct value";
}

/// <summary>
/// Test for mSign
/// </summary>
TEST(MathFunctionTest, SIGN)
{
   S32 signTest = mSign(10u);
   EXPECT_EQ(signTest, 1) << "U32 test did not return correct value";

   signTest = mSign(10);
   EXPECT_EQ(signTest, 1) << "S32 postive test did not return correct value";

   signTest = mSign(-10);
   EXPECT_EQ(signTest, -1) << "S32 negative test did not return correct value";

   signTest = mSign(0);
   EXPECT_EQ(signTest, 0) << "S32 zero test did not return correct value";

   signTest = mSign(0.5f);
   EXPECT_EQ(signTest, 1) << "F32 positive test did not return correct value";

   signTest = mSign(-0.5f);
   EXPECT_EQ(signTest, -1) << "F32 negative test did not return correct value";

   // can this even be possible?.. i guess so....
   signTest = mSign(0.0f);
   EXPECT_EQ(signTest, 0) << "F32 zero test did not return correct value";

   signTest = mSign(0.5);
   EXPECT_EQ(signTest, 1) << "F64 positive test did not return correct value";

   signTest = mSign(-0.5);
   EXPECT_EQ(signTest, -1) << "F64 negative test did not return correct value";

   signTest = mSign(0.0);
   EXPECT_EQ(signTest, 0) << "F64 zero test did not return correct value";
}

/// <summary>
/// Test for mWrap
/// </summary>
TEST(MathFunctionTest, WRAP)
{
   S32 sTest = 0;
   // test all 1 returns
   sTest = 1;
   while (sTest < 30)
   {
      S32 val = mWrap(sTest, -1, 1);
      EXPECT_EQ(val, 1) << "S32 mwrap test returned unexpected value";
      sTest += 3;
   }

   // test all 0 returns
   sTest = 0;
   while (sTest < 30)
   {
      S32 val = mWrap(sTest, -1, 1);
      EXPECT_EQ(val, 0) << "S32 mwrap test returned unexpected value";
      sTest += 3;
   }

   // test all -1 returns
   sTest = -1;
   while (sTest < 30)
   {
      S32 val = mWrap(sTest, -1, 1);
      EXPECT_EQ(val, -1) << "S32 mwrap test returned unexpected value";
      sTest += 3;
   }

   // Float tests. we check for is equal with epsilon, precision is important for these.
   F32 fTest = 0.0f;
   // test all 1.0 returns
   fTest = 1.0000f;
   while (fTest < 30.0f)
   {
      F32 val = mWrap(fTest, -1.0000f, 1.0000f);
      EXPECT_TRUE(mIsEqual(val, 1.0f, 0.0001f)) << "F32 test did not return true within eps.";
      fTest += 3.0000f;
   }

   // test all 0.0 returns
   fTest = -0.0000f;
   while (fTest < 30.0f)
   {
      F32 val = mWrap(fTest, -1.0000f, 1.0000f);
      EXPECT_TRUE(mIsEqual(val, 0.0f, 0.0001f)) << "F32 test did not return true within eps.";
      fTest += 3.0000f;
   }

   // test all -1.0 returns
   fTest = -1.000f;
   while (fTest < 30.0f)
   {
      F32 val = mWrap(fTest, -1.0000f, 1.0000f);
      EXPECT_TRUE(mIsEqual(val, -1.0f, 0.0001f)) << "F32 test did not return true within eps.";
      fTest += 3.0000f;
   }

   // Doubles Tests.
   F64 dTest = 0.0;
   // test all 1.0 returns
   dTest = 1.0;
   while (dTest < 30.0)
   {
      F64 val = mWrap(dTest, -1.0, 1.0);
      EXPECT_EQ(val, 1.0) << "F64 mwrap test returned unexpected value";
      dTest += 3.0;
   }

   // test all 0.0 returns
   dTest = 0.0;
   while (dTest < 30.0)
   {
      F64 val = mWrap(dTest, -1.0, 1.0);
      EXPECT_EQ(val, 0.0) << "F64 mwrap test returned unexpected value";
      dTest += 3.0;
   }

   // test all -1.0 returns
   dTest = -1.0;
   while (dTest < 30.0)
   {
      F64 val = mWrap(dTest, -1.0, 1.0);
      EXPECT_EQ(val, -1.0) << "F64 mwrap test returned unexpected value";
      dTest += 3.0;
   }

}
