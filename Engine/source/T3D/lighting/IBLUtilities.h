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

#ifndef IBL_UTILS_H_
#define IBL_UTILS_H_

#ifndef _GFXTARGET_H_
#include "gfx/gfxTarget.h"
#endif

#ifndef _GFXCUBEMAP_H_
#include "gfx/gfxCubemap.h"
#endif

#ifndef _COLOR_H_
#include "core/color.h"
#endif


namespace IBLUtilities
{
   void GenerateIrradianceMap(GFXTextureTargetRef renderTarget, GFXCubemapHandle cubemap, GFXCubemapHandle &cubemapOut);
   void GenerateAndSaveIrradianceMap(String outputPath, S32 resolution, GFXCubemapHandle cubemap, GFXCubemapHandle &cubemapOut);

   void GeneratePrefilterMap(GFXTextureTargetRef renderTarget, GFXCubemapHandle cubemap, U32 mipLevels, GFXCubemapHandle &cubemapOut);
   void GenerateAndSavePrefilterMap(String outputPath, S32 resolution, GFXCubemapHandle cubemap, U32 mipLevels, GFXCubemapHandle &cubemapOut);

   void SaveCubeMap(String outputPath, GFXCubemapHandle &cubemap);

   //------------------------------------------------
   // Spherical harmonics functions
   //------------------------------------------------
   /// <summary>
   /// Generates a random direction converting from spherical to cartesian.
   /// </summary>
   /// <returns>The cartesian direction as a <c>VectorF</c></returns>
   VectorF randomDirectionOnSphere();

   /// <summary>
   /// Generates a random direction on a specified cubemap face.
   /// </summary>
   /// <param name="face">The face id for the cubemap.</param>
   /// <param name="cubemapResolution">The cubemap resolution (note assumed square resolution)</param>
   /// <returns>The normalized cartesian direction as a <c>VectorF</c>.</returns>
   VectorF getRandomDirectionFromCubemapFace(const U32 face, const U32 cubemapResolution);

   /// <summary>
   /// Gets the uv coordinates this direction is pointing towards.
   /// </summary>
   /// <param name="direction">The direction in cartesian.</param>
   /// <param name="face">The face ID.</param>
   /// <param name="cubemapResolution">The cubemap resolution (assumed square).</param>
   /// <returns>The uv coords of the direction in the cubemap as a <c>Point2I</c></returns>
   Point2F getPixelFromCubemapDirection(const VectorF& direction, const U32 face, const U32 cubemapResolution);

   /// <summary>
   /// Gets the Spherical Harmonic index.
   /// </summary>
   /// <param name="l">The l component.</param>
   /// <param name="m">The m component.</param>
   /// <returns>The index of the sh in the vector.</returns>
   U32 getSHIndex(S32 l, S32 m);

   /// <summary>
   /// Evaluats the sh basis from the direction for this coefficient.
   /// </summary>
   /// <param name="l">The l component.</param>
   /// <param name="m">The m component.</param>
   /// <param name="direction">The direction in cartesian.</param>
   /// <returns>An <c>F32</c> value representing the sh basis.</returns>
   F32 evaluateSHBasis(S32 l, S32 m, const VectorF& direction);

   /// <summary>
   /// Calculates the associated polynormal for this coefficient.
   /// </summary>
   /// <param name="l">The l component.</param>
   /// <param name="m">The m component.</param>
   /// <param name="cosTheta"></param>
   /// <returns>The associated polynormal for the coefficent as <c>F32</c>.</returns>
   F32 associatedPolynormal(S32 l, S32 m, F32 cosTheta);
};

#endif
