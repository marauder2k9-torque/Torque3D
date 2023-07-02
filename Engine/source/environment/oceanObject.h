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
#ifndef _OCEANOBJECT_H_
#define _OCEANOBJECT_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXSTRUCTS_H_
#include "gfx/gfxStructs.h"
#endif
#ifndef _FOGSTRUCTS_H_
#include "scene/fogStructs.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif
#ifndef _REFLECTOR_H_
#include "scene/reflector.h"
#endif
#ifndef _ALIGNEDARRAY_H_
#include "core/util/tAlignedArray.h"
#endif
#ifndef _MATTEXTURETARGET_H_
#include "materials/matTextureTarget.h"
#endif

#include "T3D/assets/ImageAsset.h"

GFXDeclareVertexFormat(GFXOceanVertex)
{
   Point3F point;
};

class MaterialParameterHandle;
class BaseMatInstance;
class ShaderData;
class MatrixSet;

struct OceanMatParams
{
   MaterialParameterHandle* mModelMatSC;
   MaterialParameterHandle* mModelMatInvSC;
   MaterialParameterHandle* mMVPInvSC;
   MaterialParameterHandle* mWaveDataSC;
   MaterialParameterHandle* mElapsedTimeSC;
   MaterialParameterHandle* mRippleSamplerSC;
   MaterialParameterHandle* mCubemapSamplerSC;
   MaterialParameterHandle* mFoamSamplerSC;
   MaterialParameterHandle* mAmbientColorSC;
   MaterialParameterHandle* mSurfaceColorSC;
   MaterialParameterHandle* mShoreColorSC;
   MaterialParameterHandle* mDepthColorSC;
   MaterialParameterHandle* mColorExtinctionSC;
   MaterialParameterHandle* mRefractionParamsSC;
   MaterialParameterHandle* mWaterPropertiesSC;
   MaterialParameterHandle* mFoamParametersSC;
   MaterialParameterHandle* mLightDirSC;
   MaterialParameterHandle* mSpecParamsSC;
   MaterialParameterHandle* mDistortionSC;
   MaterialParameterHandle* mShoreFadeSC;
   MaterialParameterHandle* mSunColorSC;
   MaterialParameterHandle* mAmbientDensitySC;
   MaterialParameterHandle* mDiffuseDensitySC;
   MaterialParameterHandle* mRadianceFacSC;

   void clear();
   void init(BaseMatInstance* matInst);
};

class GFXOcclusionQuery;
class PostEffect;
class CubemapData;

class OceanObject : public SceneObject
{
   typedef SceneObject Parent;

protected:
   enum MaskBits {
      UpdateMask = Parent::NextFreeMask << 0,
      WaveMask = Parent::NextFreeMask << 1,
      MaterialMask = Parent::NextFreeMask << 2,
      TextureMask = Parent::NextFreeMask << 3,
      SoundMask = Parent::NextFreeMask << 4,
      NextFreeMask = Parent::NextFreeMask << 5
   };

public:

   OceanObject();
   virtual ~OceanObject();

   DECLARE_CONOBJECT(OceanObject);

   // ConsoleObject
   static void initPersistFields();

   // SimObject
   virtual bool onAdd();
   virtual void onRemove();
   virtual void inspectPostApply();

   // NetObject
   virtual U32  packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
   virtual void unpackUpdate(NetConnection* conn, BitStream* stream);

   void initTextures();

   // SceneObject
   virtual void prepRenderImage(SceneRenderState* state);
   virtual void renderObject(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat);

   bool _initMaterial();
   void setShaderParams(SceneRenderState* state, const OceanMatParams& paramHandles);
   void setCustomTexture(U32 pass, const OceanMatParams& paramHandles);

   // Ocean object.
   void getOceanPlane(PlaneF& outPlane, Point3F& outPos);


   void onRippleTexChanged() {}
   void onFoamTexChanged() {}
   void setGridSize(U32 inSize);
   void setGridElementSize(F32 inSize);

private:
   // vertex / index buffers
   GFXVertexBufferHandle<GFXOceanVertex> mVertBuff;
   GFXPrimitiveBufferHandle mPrimBuff;

   // misc
   U32            mGridSize;
   U32            mGridSizeMinusOne;
   F32            mGridElementSize;
   U32            mVertCount;
   U32            mIndxCount;
   U32            mPrimCount;
   Frustum        mFrustum;

   void setupVBIB(SceneRenderState* state);
   SceneData setupSceneGraphInfo(SceneRenderState* state);

protected:

   LinearColorF mSurfaceColor;
   LinearColorF mShoreColor;
   LinearColorF mDeepColor;

   F32 mAmbDen;
   F32 mDiffDen;
   F32 mClarity;
   F32 mTransparency;

   //Foam
   F32 mShoreRange;
   F32 mNearShoreRange;
   F32 mCrestThreshold;
   F32 mShoreFadeRange;

   DECLARE_IMAGEASSET(OceanObject, RippleTex, onRippleTexChanged, GFXStaticTextureProfile);
   DECLARE_ASSET_NET_SETGET(OceanObject, RippleTex, TextureMask);
   DECLARE_IMAGEASSET(OceanObject, FoamTex, onFoamTexChanged, GFXStaticTextureSRGBProfile);
   DECLARE_ASSET_NET_SETGET(OceanObject, FoamTex, TextureMask);

   CubemapData* mCubemap;
   MatrixSet* mMatrixSet;
   BaseMatInstance* mOceanMat;
   OceanMatParams mOceanParams;

   PlaneReflector mPlaneReflector;
   ReflectorDesc mReflectorDesc;
   PlaneF mWaterPlane;
   Point3F mWaterPos;

   Point3F mColorExtinction;
   Point3F mRefractionParams;

   StringTableEntry mCubemapName;

   Point4F mSpecParams;
   F32 mRadianceFactor;
   F32 mDistortionAmt;

   // Waves   
   Point4F  mWaveData[3];

   bool mGenerateVB;
};

#endif
