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

#include "T3D/lighting/IBLUtilities.h"
#include "console/engineAPI.h"
#include "materials/shaderData.h"
#include "gfx/gfxTextureManager.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/bitmap/cubemapSaver.h"
#include "core/stream/fileStream.h"
#include "gfx/bitmap/imageUtils.h"
#include "gfx/bitmap/gBitmap.h"

namespace IBLUtilities
{
   void GenerateIrradianceMap(GFXTextureTargetRef renderTarget, GFXCubemapHandle cubemap, GFXCubemapHandle &cubemapOut)
   {
      GFXTransformSaver saver;

      GFXStateBlockRef irrStateBlock;

      ShaderData *irrShaderData;
      GFXShaderRef irrShader = Sim::findObject("IrradianceShader", irrShaderData) ? irrShaderData->getShader() : NULL;
      if (!irrShader)
      {
         Con::errorf("IBLUtilities::GenerateIrradianceMap() - could not find IrradianceShader");
         return;
      }

      GFXShaderConstBufferRef irrConsts = irrShader->allocConstBuffer();
      GFXShaderConstHandle* irrFaceSC = irrShader->getShaderConstHandle("$face");

#if 1
      GFXShaderConstHandle* irrSHCoef = irrShader->getShaderConstHandle("$shCoefficients");

      static AlignedArray<Point4F> mSHArray(9, sizeof(Point4F));
      dMemset(mSHArray.getBuffer(), 0, mSHArray.getBufferSize());
      LinearColorF shCoefficients[9];

      for (U32 i = 0; i < 9; ++i) {
         shCoefficients[i] = LinearColorF::BLACK; // Initialize coefficients to 0
      }

      GBitmap* cubeFaceBitmaps[6];

      // Get the cubemap bitmaps.
      if (!CubemapSaver::getBitmaps(cubemap, cubemap->getFormat(), cubeFaceBitmaps))
         return;

      U32 width = cubeFaceBitmaps[0]->getWidth(); // assumed square texture and all faces matching.

      // Total number of samples processed
      U32 totalSamples = 0;
      F32 weight = (1.0f / (width * width));
      for (U32 face = 0; face < 6; ++face)
      {
         for (U32 x = 0; x < width; ++x)
         {
            for (U32 y = 0; y < width; ++y)
            {
               VectorF pixDir = getCubeDir(face, Point2F(x, y));
               LinearColorF sampleColor = cubeFaceBitmaps[face]->sampleTexel(x, y);

               // Evaluate the SH basis functions for the sample direction
               for (S32 l = 0; l <= 2; l++) // First 3 bands
               {
                  for (S32 m = -l; m <= l; m++)
                  {
                     F32 shBasis = evaluateSHBasis(l, m, pixDir);
                     shCoefficients[getSHIndex(l, m)] += sampleColor * shBasis * weight;
                  }
               }
            }
         }
         totalSamples++;
      }

      for (U32 i = 0; i < 9; ++i)
      {
         shCoefficients[i] *= (4.0f * M_PI_F) / F32(totalSamples);
         Con::printf("SHCoefficents: %i r:%g g:%g b:%g", i, shCoefficients[i].red, shCoefficients[i].green, shCoefficients[i].blue);
         mSHArray[i] = Point4F(shCoefficients[i]);
      }

      irrConsts->setSafe(irrSHCoef, mSHArray);
#endif

      GFXStateBlockDesc desc;
      desc.zEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0].addressModeU = GFXAddressClamp;
      desc.samplers[0].addressModeV = GFXAddressClamp;
      desc.samplers[0].addressModeW = GFXAddressClamp;
      desc.samplers[0].magFilter = GFXTextureFilterLinear;
      desc.samplers[0].minFilter = GFXTextureFilterLinear;
      desc.samplers[0].mipFilter = GFXTextureFilterLinear;

      irrStateBlock = GFX->createStateBlock(desc);

      GFX->pushActiveRenderTarget();
      GFX->setShader(irrShader);
      GFX->setShaderConstBuffer(irrConsts);
      GFX->setStateBlock(irrStateBlock);
      GFX->setVertexBuffer(NULL);
      GFX->setCubeTexture(0, cubemap);

      for (U32 i = 0; i < 6; i++)
      {
         renderTarget->attachTexture(GFXTextureTarget::Color0, cubemapOut, i);
         irrConsts->setSafe(irrFaceSC, (S32)i);
         GFX->setActiveRenderTarget(renderTarget);
         GFX->clear(GFXClearTarget, LinearColorF::BLACK, 1.0f, 0);
         GFX->drawPrimitive(GFXTriangleList, 0, 1);
         renderTarget->resolve();
      }

      GFX->popActiveRenderTarget();
   }

   void GenerateAndSaveIrradianceMap(String outputPath, S32 resolution, GFXCubemapHandle cubemap, GFXCubemapHandle &cubemapOut)
   {
      if (outputPath.isEmpty())
      {
         Con::errorf("IBLUtilities::GenerateAndSaveIrradianceMap - Cannot save to an empty path!");
         return;
      }

      GFXTextureTargetRef renderTarget = GFX->allocRenderToTextureTarget(false);

      IBLUtilities::GenerateIrradianceMap(renderTarget, cubemap, cubemapOut);

      //Write it out
      CubemapSaver::save(cubemapOut, outputPath);

      if (!Platform::isFile(outputPath))
      {
         Con::errorf("IBLUtilities::GenerateAndSaveIrradianceMap - Failed to properly save out the baked irradiance!");
      }
   }

   void SaveCubeMap(String outputPath, GFXCubemapHandle &cubemap)
   {
      if (outputPath.isEmpty())
      {
         Con::errorf("IBLUtilities::SaveCubeMap - Cannot save to an empty path!");
         return;
      }

      //Write it out
      CubemapSaver::save(cubemap, outputPath);

      if (!Platform::isFile(outputPath))
      {
         Con::errorf("IBLUtilities::SaveCubeMap - Failed to properly save out the baked irradiance!");
      }
   }

   void GeneratePrefilterMap(GFXTextureTargetRef renderTarget, GFXCubemapHandle cubemap, U32 mipLevels, GFXCubemapHandle &cubemapOut)
   {
      GFXTransformSaver saver;

      ShaderData *prefilterShaderData;
      GFXShaderRef prefilterShader = Sim::findObject("PrefiterCubemapShader", prefilterShaderData) ? prefilterShaderData->getShader() : NULL;
      if (!prefilterShader)
      {
         Con::errorf("IBLUtilities::GeneratePrefilterMap() - could not find PrefiterCubemapShader");
         return;
      }

      GFXShaderConstBufferRef prefilterConsts = prefilterShader->allocConstBuffer();
      GFXShaderConstHandle* prefilterFaceSC = prefilterShader->getShaderConstHandle("$face");
      GFXShaderConstHandle* prefilterRoughnessSC = prefilterShader->getShaderConstHandle("$roughness");
      GFXShaderConstHandle* prefilterMipSizeSC = prefilterShader->getShaderConstHandle("$mipSize");
      GFXShaderConstHandle* prefilterResolutionSC = prefilterShader->getShaderConstHandle("$resolution");

      GFXStateBlockDesc desc;
      desc.zEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0].addressModeU = GFXAddressClamp;
      desc.samplers[0].addressModeV = GFXAddressClamp;
      desc.samplers[0].addressModeW = GFXAddressClamp;
      desc.samplers[0].magFilter = GFXTextureFilterLinear;
      desc.samplers[0].minFilter = GFXTextureFilterLinear;
      desc.samplers[0].mipFilter = GFXTextureFilterLinear;

      GFXStateBlockRef preStateBlock;
      preStateBlock = GFX->createStateBlock(desc);
      GFX->setStateBlock(preStateBlock);

      GFX->pushActiveRenderTarget();
      GFX->setShader(prefilterShader);
      GFX->setShaderConstBuffer(prefilterConsts);
      GFX->setCubeTexture(0, cubemap);

      U32 prefilterSize = cubemapOut->getSize();

      U32 resolutionSize = prefilterSize;

      for (U32 face = 0; face < 6; face++)
      {
         prefilterConsts->setSafe(prefilterFaceSC, (S32)face);
         prefilterConsts->setSafe(prefilterResolutionSC, (S32)resolutionSize);

         for (U32 mip = 0; mip < mipLevels; mip++)
         {
            S32 mipSize = prefilterSize >> mip;
            F32 roughness = (F32)mip / (F32)(mipLevels - 1);
            prefilterConsts->setSafe(prefilterRoughnessSC, roughness);
            prefilterConsts->setSafe(prefilterMipSizeSC, mipSize);
            U32 size = prefilterSize * mPow(0.5f, mip);
            renderTarget->attachTexture(GFXTextureTarget::Color0, cubemapOut, face);
            GFX->setActiveRenderTarget(renderTarget, false);//we set the viewport ourselves
            GFX->setViewport(RectI(0, 0, size, size));
            GFX->clear(GFXClearTarget, LinearColorF::BLACK, 1.0f, 0);
            GFX->drawPrimitive(GFXTriangleList, 0, 1);
            renderTarget->resolve();
         }
      }

      GFX->popActiveRenderTarget();
   }

   void GenerateAndSavePrefilterMap(String outputPath, S32 resolution, GFXCubemapHandle cubemap, U32 mipLevels, GFXCubemapHandle &cubemapOut)
   {
      if (outputPath.isEmpty())
      {
         Con::errorf("IBLUtilities::GenerateAndSavePrefilterMap - Cannot save to an empty path!");
         return;
      }

      GFXTextureTargetRef renderTarget = GFX->allocRenderToTextureTarget(false);

      IBLUtilities::GeneratePrefilterMap(renderTarget, cubemap, mipLevels, cubemapOut);

      //Write it out
      CubemapSaver::save(cubemapOut, outputPath);

      if (!Platform::isFile(outputPath))
      {
         Con::errorf("IBLUtilities::GenerateAndSavePrefilterMap - Failed to properly save out the baked irradiance!");
      }
   }

   //------------------------------------------------
   // Spherical harmonics functions
   //------------------------------------------------

   U32 getSHIndex(S32 l, S32 m)
   {
      U32 ret = 0;

      ret = l * (l + 1) + m; // Flatten (l, m) to a 1D index
      return ret;
   }

   F32 evaluateSHBasis(S32 l, S32 m, const VectorF& direction)
   {
      F32 phi = mAtan2(direction.y, direction.x); // azimuth angle
      F32 theta = mAcos(direction.z);             // elevation angle

      F32 p_lm = associatedPolynormal(l, m, mCos(theta));

      F32 n_lm = mSqrt((2.0f * l + 1.0f) / (4.0f * M_PI_F) * mFact(l - mAbs(m)) / mFact(l + mAbs(m)));

      if (m == 0)
      {
         return n_lm * p_lm;
      }
      else if (m > 0)
      {
         return mSqrt(2.0f) * n_lm * p_lm * mCos(m * phi);
      }
      else // m < 0
      {
         return mSqrt(2.0f) * n_lm * p_lm * mSin(mAbs(m) * phi);
      }
   }

   F32 associatedPolynormal(S32 l, S32 m, F32 cosTheta)
   {
      F32 pmm = 1.0f;

      if (m > 0)
      {
         F32 mx2 = mSqrt((1.0f - cosTheta) * (1.0f + cosTheta));
         F32 fac = 1.0f;

         for (U32 i = 1; i <= m; ++i)
         {
            pmm *= -fac * mx2;
            fac += 2.0f;
         }
      }

      if (l == m)
         return pmm;

      F32 pmm1 = cosTheta * (2.0f * m + 1.0f) * pmm;

      if (l == m + 1)
         return pmm1;

      F32 pll = 0.0f;

      for (U32 ll = m + 2; ll <= l; ++ll)
      {
         pll = ((2.0f * ll - 1.0f) * cosTheta * pmm1 - (ll + m - 1.0f) * pmm) / (ll - m);
         pmm = pmm1;
         pmm1 = pll;
      }

      return pll;
   }

   VectorF randomDirectionOnSphere()
   {
      // Generate random azimuthal angle (theta) in the range [0, 2 * M_PI]
      F32 theta = mRandF(0.0f, 2.0f * M_PI);  // Azimuthal angle, 0 to 2 * PI

      // Generate random polar angle (phi) in the range [-PI / 2, PI / 2]
      F32 phi = mRandF(-M_PI_F / 2, M_PI_F / 2);  // Polar angle, -PI/2 to PI/2

      // Convert spherical coordinates to Cartesian coordinates
      F32 x = cos(phi) * sin(theta);
      F32 y = sin(phi);
      F32 z = cos(phi) * cos(theta);

      return VectorF(x, y, z);
   }

   VectorF getRandomDirectionFromCubemapFace(const U32 face, const U32 cubemapResolution)
   {
      {
         // Randomly pick a pixel (u, v) on the cubemap face
         U32 x = mRandF(0.0f, cubemapResolution - 1);
         U32 y = mRandF(0.0f, cubemapResolution - 1);

         // Convert to normalized UV coordinates
         F32 u = F32(x) / F32(cubemapResolution - 1);
         F32 v = F32(y) / F32(cubemapResolution - 1);

         // Convert UV coordinates to a 3D direction vector based on cubemap face
         VectorF dir(0.0f, 0.0f, 0.0f);

         switch (face)
         {
         case 0: // +X face
            dir = VectorF(1.0f, -1.0f + 2.0f * v, -1.0f + 2.0f * u);
            break;
         case 1: // -X face
            dir = VectorF(-1.0f, -1.0f + 2.0f * v, 1.0f - 2.0f * u);
            break;
         case 2: // +Y face
            dir = VectorF(1.0f - 2.0f * u, 1.0f, -1.0f + 2.0f * v);
            break;
         case 3: // -Y face
            dir = VectorF(1.0f - 2.0f * u, -1.0f, 1.0f - 2.0f * v);
            break;
         case 4: // +Z face
            dir = VectorF(1.0f - 2.0f * u, -1.0f + 2.0f * v, 1.0f);
            break;
         case 5: // -Z face
            dir = VectorF(-1.0f + 2.0f * u, -1.0f + 2.0f * v, -1.0f);
            break;
         default:
            break;
         }

         // Normalize the resulting direction vector
         dir.normalize();

         return dir;
      }
   }

   Point2F getPixelFromCubemapDirection(const VectorF& direction, const U32 face, const U32 cubemapResolution)
   {
      F32 absX = mFabs(direction.x);
      F32 absY = mFabs(direction.y);
      F32 absZ = mFabs(direction.z);

      Point2F uv(0.0f, 0.0f);
      switch (face) {
      case 0: // +X face
         uv.x = -direction.z / absX * 0.5f + 0.5f; // Map z to u
         uv.y = direction.y / absX * 0.5f + 0.5f;  // Map y to v
         break;
      case 1: // -X face
         uv.x = direction.z / absX * 0.5f + 0.5f;  // Map z to u
         uv.y = direction.y / absX * 0.5f + 0.5f;  // Map y to v
         break;
      case 2: // +Y face
         uv.x = direction.x / absY * 0.5f + 0.5f;  // Map x to u
         uv.y = direction.z / absY * 0.5f + 0.5f;  // Map z to v
         break;
      case 3: // -Y face
         uv.x = direction.x / absY * 0.5f + 0.5f;  // Map x to u
         uv.y = -direction.z / absY * 0.5f + 0.5f; // Map z to v
         break;
      case 4: // +Z face
         uv.x = direction.x / absZ * 0.5f + 0.5f;  // Map x to u
         uv.y = direction.y / absZ * 0.5f + 0.5f;  // Map y to v
         break;
      case 5: // -Z face
         uv.x = -direction.x / absZ * 0.5f + 0.5f; // Map x to u
         uv.y = direction.y / absZ * 0.5f + 0.5f;  // Map y to v
         break;
      }

      return uv;
   }

   VectorF getCubeDir(const U32 face, const Point2F& uv)
   {
      // Convert UV to [-1, 1] range
      float u = 2.0f * uv.x - 1.0f; // Map [0, 1] -> [-1, 1]
      float v = 2.0f * uv.y - 1.0f; // Map [0, 1] -> [-1, 1]

      VectorF dir;

      // Map the UV coordinates to a direction based on the face
      switch (face)
      {
      case 0: // +X
         dir = VectorF(1.0f, -v, -u);
         break;
      case 1: // -X
         dir = VectorF(-1.0f, -v, u);
         break;
      case 2: // +Y
         dir = VectorF(u, 1.0f, v);
         break;
      case 3: // -Y
         dir = VectorF(u, -1.0f, -v);
         break;
      case 4: // +Z
         dir = VectorF(u, -v, 1.0f);
         break;
      case 5: // -Z
         dir = VectorF(-u, -v, -1.0f);
         break;
      default:
         dir = VectorF(0.0f, 0.0f, 0.0f); // Fallback, should never happen
         break;
      }

      // Normalize the direction vector to ensure it's a unit vector
      dir.normalizeSafe();

      return dir;
   }

};
