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

#include "platform/platform.h"
#include "environment/oceanObject.h"

#include "console/consoleTypes.h"
#include "materials/materialParameters.h"
#include "materials/baseMatInstance.h"
#include "materials/materialManager.h"
#include "materials/customMaterialDefinition.h"
#include "materials/sceneData.h"
#include "core/stream/bitStream.h"
#include "scene/reflectionManager.h"
#include "scene/sceneRenderState.h"
#include "lighting/lightInfo.h"
#include "math/mathIO.h"
#include "math/mathUtils.h"
#include "postFx/postEffect.h"
#include "T3D/gameBase/gameConnection.h"
#include "T3D/shapeBase.h"
#include "gfx/gfxOcclusionQuery.h"
#include "gfx/gfxTextureManager.h"
#include "gfx/sim/cubemapData.h"
#include "math/util/matrixSet.h"
#include "sfx/sfxAmbience.h"
#include "T3D/sfx/sfx3DWorld.h"
#include "sfx/sfxTypes.h"

// just position for now, work out normals in shader.
GFXImplementVertexFormat(GFXOceanVertex)
{
   addElement("POSITION", GFXDeclType_Float3);
}

void OceanMatParams::clear()
{
   mModelMatSC = NULL;
   mModelMatInvSC = NULL;
   mMVPInvSC = NULL;
   mWaveDataSC = NULL;
   mElapsedTimeSC = NULL;
   mRippleSamplerSC = NULL;
   mCubemapSamplerSC = NULL;
   mFoamSamplerSC = NULL;
   mAmbientColorSC = NULL;
   mSurfaceColorSC = NULL;
   mShoreColorSC = NULL;
   mDepthColorSC = NULL;
   mColorExtinctionSC = NULL;
   mRefractionParamsSC = NULL;
   mWaterPropertiesSC = NULL;
   mFoamParametersSC = NULL;
   mLightDirSC = NULL;
   mSpecParamsSC = NULL;
   mDistortionSC = NULL;
   mShoreFadeSC = NULL;
   mSunColorSC = NULL;
   mAmbientDensitySC = NULL;
   mDiffuseDensitySC = NULL;
   mRadianceFacSC = NULL;
}

void OceanMatParams::init(BaseMatInstance* matInst)
{
   clear();

   mModelMatSC = matInst->getMaterialParameterHandle("$modelMat");
   mModelMatInvSC = matInst->getMaterialParameterHandle("$modelMatInverse");
   mMVPInvSC = matInst->getMaterialParameterHandle("$mvpInverse");
   mWaveDataSC = matInst->getMaterialParameterHandle("$waveData");
   mElapsedTimeSC = matInst->getMaterialParameterHandle("$elapsedTime");
   mCubemapSamplerSC = matInst->getMaterialParameterHandle("$skyMap");
   mRippleSamplerSC = matInst->getMaterialParameterHandle("$bumpMap");
   mFoamSamplerSC = matInst->getMaterialParameterHandle("$foamMap");
   mAmbientColorSC = matInst->getMaterialParameterHandle("$ambientColor");
   mSurfaceColorSC = matInst->getMaterialParameterHandle("$surfaceColor");
   mShoreColorSC = matInst->getMaterialParameterHandle("$shoreColor");
   mDepthColorSC = matInst->getMaterialParameterHandle("$depthColor");
   mColorExtinctionSC = matInst->getMaterialParameterHandle("$colorExtinction");
   mRefractionParamsSC = matInst->getMaterialParameterHandle("$refractionParams");
   mWaterPropertiesSC = matInst->getMaterialParameterHandle("$waterProperties");
   mFoamParametersSC = matInst->getMaterialParameterHandle("$foamParameters");
   mLightDirSC = matInst->getMaterialParameterHandle("$inLightVec");
   mSpecParamsSC = matInst->getMaterialParameterHandle("$specParams");
   mDistortionSC = matInst->getMaterialParameterHandle("$distortionAmt");
   mShoreFadeSC = matInst->getMaterialParameterHandle("$shoreFadeRange");
   mSunColorSC = matInst->getMaterialParameterHandle("$sunColor");
   mAmbientDensitySC = matInst->getMaterialParameterHandle("$ambDensity");
   mDiffuseDensitySC = matInst->getMaterialParameterHandle("$diffuseDensity");
   mRadianceFacSC = matInst->getMaterialParameterHandle("$radianceFactor");
}

//-------------------------------------------------------------------------
// OceanObject Class
//-------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(OceanObject);

ConsoleDocClass(OceanObject,
   "@brief Class for creating an ocean.\n\n"

   "@ingroup Water"
);

OceanObject::OceanObject()
 : mSurfaceColor(0.0f, 0.3f, 1.0f,1.0f),
   mShoreColor(0.14f, 0.76f, 0.26f, 1.0f),
   mDeepColor(0.016f, 0.04f, 0.15f, 1.0f),
   mOceanMat(NULL),
   mClarity(0.25f),
   mAmbDen(0.15f),
   mDiffDen(0.1f),
   mTransparency(10.0f),
   mCubemap(NULL),
   mShoreRange(2.25f),
   mNearShoreRange(3.3f),
   mCrestThreshold(10.4f),
   mShoreFadeRange(0.2f),
   mRadianceFactor(0.2f),
   mDistortionAmt(0.0345f)
{
   mTypeMask = OceanObjectType;
   mWaveData[0].set(0.1f, 0.2f, 0.3f, 60.0f);
   mWaveData[1].set(0.1f, -0.2f, 0.3f, 20.0f);
   mWaveData[2].set(0.2f, 0.2f, 0.3f, 10.0f);

   mSpecParams.set(30.0f, 768.0f, 0.25f, 0.8f);

   mColorExtinction.set(3.0f, 11.6f, 16.7f);
   mRefractionParams.set(0.17f, 0.11f, 0.31f);

   mMatrixSet = reinterpret_cast<MatrixSet*>(dMalloc_aligned(sizeof(MatrixSet), 16));
   constructInPlace(mMatrixSet);

   INIT_ASSET(RippleTex);
   INIT_ASSET(FoamTex);

   mGenerateVB = true;
   mGridElementSize = 1.0f;
   mGridSize = 101;
   mGridSizeMinusOne = mGridSize - 1;
   mNetFlags.set(Ghostable | ScopeAlways);
   mVertCount = 0;
   mIndxCount = 0;
   mPrimCount = 0;

   mCubemapName = StringTable->EmptyString();
}

OceanObject::~OceanObject()
{
   dFree_aligned(mMatrixSet);
}

void OceanObject::initPersistFields()
{
   docsURL;

   addGroup("Ocean");
      addField("Ambient Density", TypeF32, Offset(mAmbDen, OceanObject), "Ambient color density.");
      addField("Diffuse Density", TypeF32, Offset(mDiffDen, OceanObject), "Ambient color density.");
      addField("Surface Color", TypeColorF, Offset(mSurfaceColor, OceanObject), "Color used for the water surface.");
      addField("Shore Tint", TypeColorF, Offset(mShoreColor, OceanObject), "Color used for the water surface.");
      addField("Deep color", TypeColorF, Offset(mDeepColor, OceanObject), "Color used for the water surface.");

      INITPERSISTFIELD_IMAGEASSET(RippleTex, OceanObject, "Normal map used to simulate small surface ripples");

      addArray("Waves.", 3);
         addField("Wave Data", TypePoint4F, Offset(mWaveData, OceanObject), 3, " xy = dir, z = steepness, w = amplitude.");
      endArray("Waves");
   endGroup("Ocean");

   addGroup("Refraction");
      addField("Refraction Parameters", TypePoint3F, Offset(mRefractionParams, OceanObject), "x = IOR, y = Power, z = RefractionScale");
      addField("Clarity", TypeF32, Offset(mClarity, OceanObject), "Clarity of the water surface.");
      addField("Transparency", TypeF32, Offset(mTransparency, OceanObject), "Transparency of the water surface.");
      addField("Color Extinction", TypePoint3F, Offset(mColorExtinction, OceanObject), "Extinction rate of colors");
   endGroup("Refraction");

   addGroup("Reflection");
      addField("Cubemap", TypeCubemapName, Offset(mCubemapName, OceanObject), "Cubemap used instead of reflection texture if fullReflect is off.");
      addField("Specularity", TypePoint4F, Offset(mSpecParams, OceanObject), " xy = intensity, z = shininess, w = shininess exp.");
      addField("Distortion Amount", TypeF32, Offset(mDistortionAmt, OceanObject), "Reflection Distortion amt.");
      addField("Radiance Factor", TypeF32, Offset(mRadianceFactor, OceanObject), "Raidiance Factor.");
   endGroup("Reflection");

   addGroup("Foam");
      INITPERSISTFIELD_IMAGEASSET(FoamTex, OceanObject, "Diffuse texture for foam");
      addField("Minimum Shore Range", TypeF32, Offset(mShoreRange, OceanObject), "Minimum Shore range.");
      addField("Near shore Range", TypeF32, Offset(mNearShoreRange, OceanObject), "Near Shore range.");
      addField("Crest threshold", TypeF32, Offset(mCrestThreshold, OceanObject), "Minimum height for crest.");
      addField("Shore Fade", TypeF32, Offset(mShoreFadeRange, OceanObject), "Shore fade range.");
   endGroup("Foam");

   Parent::initPersistFields();

   removeField("rotation");
   removeField("scale");
}

bool OceanObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   if (isClientObject())
   {
      initTextures();

      mPlaneReflector.registerReflector(this, &mReflectorDesc);
   }

   setGlobalBounds();
   resetWorldBox();
   addToScene();

   return true;
}

void OceanObject::onRemove()
{
   removeFromScene();

   if (isClientObject())
   {
      mPlaneReflector.unregisterReflector();
   }

   Parent::onRemove();
}

void OceanObject::inspectPostApply()
{
   Parent::inspectPostApply();

   setMaskBits(UpdateMask | WaveMask | TextureMask | SoundMask);
}

U32 OceanObject::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   stream->write(mGridSize);
   stream->write(mGridElementSize);

   if (stream->writeFlag(mask & UpdateMask))
   {
      stream->write(mSurfaceColor);
      stream->write(mShoreColor);
      stream->write(mDeepColor);
      stream->write(mAmbDen);
      stream->write(mDiffDen);
      stream->write(mClarity);
      stream->write(mTransparency);

      // foam
      stream->write(mShoreRange);
      stream->write(mNearShoreRange);
      stream->write(mCrestThreshold);
      stream->write(mShoreFadeRange);
      

      mathWrite(*stream, mColorExtinction);
      mathWrite(*stream, mRefractionParams);

      mathWrite(*stream, mSpecParams);
      stream->write(mDistortionAmt);
      stream->write(mRadianceFactor);

      stream->write(getPosition().z);
   }

   if (stream->writeFlag(mask & WaveMask))
   {
      for (U32 i = 0; i < 3; i++)
      {
         mathWrite(*stream, mWaveData[i]);
      }
   }

   if (stream->writeFlag(mask & TextureMask))
   {
      PACK_ASSET(conn, RippleTex);
      PACK_ASSET(conn, FoamTex);

      stream->writeString(mCubemapName);
   }

   return retMask;
}

void OceanObject::unpackUpdate(NetConnection* conn, BitStream* stream)
{
   Parent::unpackUpdate(conn, stream);

   U32 inGridSize;
   stream->read(&inGridSize);
   setGridSize(inGridSize);

   F32 inGridElementSize;
   stream->read(&inGridElementSize);
   setGridElementSize(inGridElementSize);

   // UpdateMask
   if (stream->readFlag())
   {
      stream->read(&mSurfaceColor);
      stream->read(&mShoreColor);
      stream->read(&mDeepColor);
      stream->read(&mAmbDen);
      stream->read(&mDiffDen);
      stream->read(&mClarity);
      stream->read(&mTransparency);

      // foam
      stream->read(&mShoreRange);
      stream->read(&mNearShoreRange);
      stream->read(&mCrestThreshold);
      stream->read(&mShoreFadeRange);

      mathRead(*stream, &mColorExtinction);
      mathRead(*stream, &mRefractionParams);

      mathRead(*stream, &mSpecParams);
      stream->read(&mDistortionAmt);
      stream->read(&mRadianceFactor);

      F32 posZ;
      stream->read(&posZ);
      Point3F newPos = getPosition();
      newPos.z = posZ;
      setPosition(newPos);

      if (isProperlyAdded() && !mPlaneReflector.isEnabled())
         mPlaneReflector.registerReflector(this, &mReflectorDesc);

   }

   // WaveMask
   if (stream->readFlag())
   {
      for (U32 i = 0; i < 3; i++)
      {
         mathRead(*stream, &mWaveData[i]);
      }
   }

   // TextureMask
   if (stream->readFlag())
   {
      UNPACK_ASSET(conn, RippleTex);
      UNPACK_ASSET(conn, FoamTex);

      mCubemapName = stream->readSTString();

      if (isProperlyAdded())
         initTextures();
   }

}

void OceanObject::initTextures()
{
   if (mCubemapName != StringTable->EmptyString())
      Sim::findObject(mCubemapName, mCubemap);
   if (mCubemap)
      mCubemap->createMap();
}


void OceanObject::prepRenderImage(SceneRenderState* state)
{
   // We only render during the normal diffuse render pass.
   if (!state->isDiffusePass())
      return;


   // Setup scene transforms
   mMatrixSet->setSceneProjection(GFX->getProjectionMatrix());
   mMatrixSet->setSceneView(GFX->getWorldMatrix());

   const Frustum& frustum = state->getCameraFrustum();

   if (mPrimBuff.isNull() ||
      mGenerateVB ||
      frustum != mFrustum)
   {
      mFrustum = frustum;
      setupVBIB(state);
      mGenerateVB = false;

      MatrixF proj(true);
      MathUtils::getZBiasProjectionMatrix(0.0f, mFrustum, &proj);
      mMatrixSet->setSceneProjection(proj);
   }

   getOceanPlane(mWaterPlane, mWaterPos);
   mPlaneReflector.refplane = mWaterPlane;

   ObjectRenderInst* ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(this, &OceanObject::renderObject);
   ri->type = RenderPassManager::RIT_Water;
   ri->defaultKey = 1;
   state->getRenderPass()->addInst(ri);
}

SceneData OceanObject::setupSceneGraphInfo(SceneRenderState* state)
{
   SceneData sgData;

   sgData.lights[0] = LIGHTMGR->getSpecialLight(LightManager::slSunLightType);

   // fill in water's transform
   sgData.objTrans = &getRenderTransform();

   // fog
   sgData.setFogParams(state->getSceneManager()->getFogData());

   // misc
   sgData.backBuffTex = REFLECTMGR->getRefractTex();
   sgData.reflectTex = mPlaneReflector.reflectTex;
   sgData.wireframe = GFXDevice::getWireframe();

   return sgData;
}

void OceanObject::renderObject(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat)
{
   if (overrideMat)
      return;

   GFXOcclusionQuery* query = mPlaneReflector.getOcclusionQuery();

   bool doQuery = (!mPlaneReflector.mQueryPending && query && mReflectorDesc.useOcclusionQuery);

   // We need to call this for avoid a DX9 or Nvidia bug.
   // At some resollutions read from render target,
   // break current occlusion query.
   REFLECTMGR->getRefractTex();

   if (doQuery)
      query->begin();

   const Point3F& camPosition = state->getCameraPosition();

   Point3F rvec, fvec, uvec, pos;

   const MatrixF& objMat = getTransform(); //getRenderTransform();
   const MatrixF& camMat = state->getCameraTransform();

   MatrixF renderMat(true);

   camMat.getColumn(1, &fvec);
   uvec.set(0, 0, 1);
   rvec = mCross(fvec, uvec);
   rvec.normalize();
   fvec = mCross(uvec, rvec);
   pos = camPosition;
   pos.z = objMat.getPosition().z;

   renderMat.setColumn(0, rvec);
   renderMat.setColumn(1, fvec);
   renderMat.setColumn(2, uvec);
   renderMat.setColumn(3, pos);

   setRenderTransform(renderMat);

   // Setup SceneData
   SceneData sgData = setupSceneGraphInfo(state);

   if (!_initMaterial())
      return;

   // setup proj/world transform
   mMatrixSet->restoreSceneViewProjection();
   mMatrixSet->setWorld(getRenderTransform());

   setShaderParams(state, mOceanParams);

   while (mOceanMat->setupPass(state, sgData))
   {
      mOceanMat->setSceneInfo(state, sgData);
      mOceanMat->setTransforms(*mMatrixSet, state);
      setCustomTexture(mOceanMat->getCurPass(), mOceanParams);

      // set vert/prim buffer
      GFX->setVertexBuffer(mVertBuff);
      GFX->setPrimitiveBuffer(mPrimBuff);
      GFX->drawIndexedPrimitive(GFXTriangleList, 0, 0, mVertCount, 0, mPrimCount);
   }

   if (doQuery)
      query->end();

}

bool OceanObject::_initMaterial()
{
   if (mOceanMat && mOceanMat->isValid())
      return true;

   // for now we only want 1 material.
   CustomMaterial* custMat;
   if (Sim::findObject("OceanMat", custMat) && custMat->mShaderData)
   {
      mOceanMat = custMat->createMatInstance();

      const GFXVertexFormat* flags = getGFXVertexFormat<GFXVertexPCT>();
      if (mOceanMat && mOceanMat->init(MATMGR->getDefaultFeatures(), flags))
      {
         mOceanParams.init(mOceanMat);
         return true;
      }
   }

   return false;
}

void OceanObject::setShaderParams(SceneRenderState* state, const OceanMatParams &paramHandles)
{
   MaterialParameters* matParams = mOceanMat->getMaterialParameters();

   // elapsedTime
   matParams->setSafe(paramHandles.mElapsedTimeSC, (F32)Sim::getCurrentTime() / 1000.0f);

   MatrixF modelMat = getRenderTransform();

   if (paramHandles.mModelMatSC->isValid())
      matParams->set(paramHandles.mModelMatSC, modelMat, GFXSCT_Float4x4);

   if (paramHandles.mModelMatInvSC->isValid())
      matParams->set(paramHandles.mModelMatInvSC, modelMat.inverse(), GFXSCT_Float4x4);


   if (paramHandles.mMVPInvSC->isValid())
   {
      MatrixF mvp = mMatrixSet->getWorldViewProjection();
      matParams->set(paramHandles.mMVPInvSC, mvp.inverse(), GFXSCT_Float4x4);
   }


   // Set our colors.
   Point3F surfaceColor = mSurfaceColor;
   matParams->setSafe(paramHandles.mSurfaceColorSC, surfaceColor);

   Point3F shoreColor = mShoreColor;
   matParams->setSafe(paramHandles.mShoreColorSC, shoreColor);

   Point3F deepColor = mDeepColor;
   matParams->setSafe(paramHandles.mDepthColorSC, deepColor);

   Point2F waterProperty(mClarity, mTransparency);
   matParams->setSafe(paramHandles.mWaterPropertiesSC, waterProperty);

   matParams->setSafe(paramHandles.mAmbientDensitySC, mAmbDen);
   matParams->setSafe(paramHandles.mDiffuseDensitySC, mDiffDen);

   Point3F foamParams(mShoreRange, mNearShoreRange, mCrestThreshold);
   matParams->setSafe(paramHandles.mFoamParametersSC, foamParams);

   matParams->setSafe(paramHandles.mShoreFadeSC, mShoreFadeRange);

   matParams->setSafe(paramHandles.mSpecParamsSC, mSpecParams);
   matParams->setSafe(paramHandles.mDistortionSC, mDistortionAmt);
   matParams->setSafe(paramHandles.mRadianceFacSC, mRadianceFactor);

   static AlignedArray<Point4F> mConstArray4F(3, sizeof(Point4F));
   for (U32 i = 0; i < 3; i++)
   {
      mConstArray4F[i] = mWaveData[i];
   }
   matParams->setSafe(paramHandles.mWaveDataSC, mConstArray4F);

   matParams->setSafe(paramHandles.mColorExtinctionSC, mColorExtinction);
   matParams->setSafe(paramHandles.mRefractionParamsSC, mRefractionParams);

   LightInfo* sun = LIGHTMGR->getSpecialLight(LightManager::slSunLightType);
   const LinearColorF& sunlight = state->getAmbientLightColor();
   Point3F ambientColor = sunlight;
   matParams->setSafe(paramHandles.mAmbientColorSC, ambientColor);
   matParams->setSafe(paramHandles.mLightDirSC, sun->getDirection());

   matParams->setSafe(paramHandles.mSunColorSC, sun->getColor());
}

void OceanObject::setCustomTexture(U32 pass, const OceanMatParams& paramHandles)
{
   GFX->setTexture(paramHandles.mRippleSamplerSC->getSamplerRegister(pass), mRippleTex);
   //GFX->setTexture(paramHandles.mFoamSamplerSC->getSamplerRegister(pass), mFoamTex);

   if(mCubemap)
      GFX->setCubeTexture(paramHandles.mCubemapSamplerSC->getSamplerRegister(pass), mCubemap->mCubemap);
   else if (paramHandles.mCubemapSamplerSC->getSamplerRegister(pass) != -1)
      GFX->setCubeTexture(paramHandles.mCubemapSamplerSC->getSamplerRegister(pass), NULL);
}

void OceanObject::getOceanPlane(PlaneF& outPlane, Point3F& outPos)
{
   outPos = getPosition();
   outPlane.set(outPos, Point3F(0, 0, 1));
}

void OceanObject::setupVBIB(SceneRenderState* state)
{
   const Frustum& frustum = state->getCullingFrustum();

   // World-Up vector, assigned as normal for all verts.
   const Point3F worldUp(0.0f, 0.0f, 1.0f);

   // World-unit size of a grid cell.
   const F32 squareSize = mGridElementSize;

   // Column/Row count.
   // So we don't neet to access class-specific member variables
   // in the code below.
   const U32 gridSize = mGridSize;

   // Number of verts in one column / row
   const U32 gridStride = gridSize + 1;

   // Length of a grid row/column divided by two.
   F32 gridSideHalfLen = squareSize * gridSize * 0.5f;

   // Position of the first vertex in the grid.
   // Relative to the camera this is the "Back Left" corner vert.
   const Point3F cornerPosition(-gridSideHalfLen, -gridSideHalfLen, 0.0f);

   // Number of verts in the grid centered on the camera.
   const U32 gridVertCount = gridStride * gridStride;

   // Total number of verts. Calculation explained above.
   mVertCount = gridVertCount; // +borderVertCount + horizonVertCount;


   // Fill the vertex buffer...
   mVertBuff.set(GFX, mVertCount, GFXBufferTypeStatic);
   GFXOceanVertex* vertPtr = mVertBuff.lock();

   // Temorary storage for calculation of vert position.
   F32 xVal, yVal;

   for (U32 i = 0; i < gridStride; i++)
   {
      yVal = cornerPosition.y + (F32)(i * squareSize);

      for (U32 j = 0; j < gridStride; j++)
      {
         xVal = cornerPosition.x + (F32)(j * squareSize);

         vertPtr->point.set(xVal, yVal, 0.0f);
         vertPtr++;
      }
   }

   mVertBuff.unlock();


   // Fill in the PrimitiveBuffer...

   // 2 triangles per cell/quad   
   const U32 gridTriCount = gridSize * gridSize * 2;

   mPrimCount = gridTriCount;

   // 3 indices per triangle.
   mIndxCount = mPrimCount * 3;

   mPrimBuff.set(GFX, mIndxCount, mPrimCount, GFXBufferTypeStatic);
   U16* idxPtr;
   mPrimBuff.lock(&idxPtr);

   // Temporaries to hold indices for the corner points of a quad.
   U32 p00, p01, p11, p10;
   U32 offset = 0;

   // Given a single cell of the grid diagramed below,
   // quad indice variables are in this orientation.
   //
   // p01 --- p11
   //  |       |
   //  |       |
   // p00 --- p10
   //
   //   Positive Y points UP the diagram ( p00, p01 ).
   //   Positive X points RIGHT across the diagram ( p00, p10 )
   //

   // i iterates bottom to top "column-wise"
   for (U32 i = 0; i < mGridSize; i++)
   {
      // j iterates left to right "row-wise"
      for (U32 j = 0; j < mGridSize; j++)
      {
         // where (j,i) is a particular cell.
         p00 = offset;
         p10 = offset + 1;
         p01 = offset + gridStride;
         p11 = offset + 1 + gridStride;

         // Top Left Triangle

         *idxPtr = p00;
         idxPtr++;
         *idxPtr = p01;
         idxPtr++;
         *idxPtr = p11;
         idxPtr++;

         // Bottom Right Triangle

         *idxPtr = p00;
         idxPtr++;
         *idxPtr = p11;
         idxPtr++;
         *idxPtr = p10;
         idxPtr++;

         offset += 1;
      }

      offset += 1;
   }

   mPrimBuff.unlock();

}

void OceanObject::setGridSize(U32 inSize)
{
   if (inSize == mGridSize)
      return;

   // GridSize must be an odd number.
   //if ( inSize % 2 == 0 )
   //   inSize++;

   // GridSize must be at least 1
   inSize = getMax(inSize, (U32)1);

   mGridSize = inSize;
   mGridSizeMinusOne = mGridSize - 1;
   mGenerateVB = true;
   setMaskBits(UpdateMask);
}

void OceanObject::setGridElementSize(F32 inSize)
{
   if (inSize == mGridElementSize)
      return;

   // GridElementSize must be greater than 0
   inSize = getMax(inSize, 0.0001f);

   mGridElementSize = inSize;
   mGenerateVB = true;
   setMaskBits(UpdateMask);
}
