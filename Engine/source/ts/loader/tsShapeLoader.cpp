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
#include "console/engineAPI.h"
#include "ts/loader/tsShapeLoader.h"

#include "core/volume.h"
#include "materials/materialList.h"
#include "materials/matInstance.h"
#include "materials/materialManager.h"
#include "ts/tsShapeInstance.h"
#include "ts/tsMaterialList.h"

// assimp include files. 
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>
#include <assimp/config.h>
#include <exception>

#include <assimp/Importer.hpp>

MODULE_BEGIN(ShapeLoader)
MODULE_INIT_AFTER(GFX)
MODULE_INIT
{
   TSShapeLoader::addFormat("Torque DTS", "dts");
   TSShapeLoader::addFormat("Torque DSQ", "dsq");
   TSShapeLoader::addFormat("DirectX X", "x");
   TSShapeLoader::addFormat("Autodesk FBX", "fbx");
   TSShapeLoader::addFormat("Blender 3D", "blend");
   TSShapeLoader::addFormat("3ds Max 3DS", "3ds");
   TSShapeLoader::addFormat("3ds Max ASE", "ase");
   TSShapeLoader::addFormat("Wavefront Object", "obj");
   TSShapeLoader::addFormat("Industry Foundation Classes (IFC/Step)", "ifc");
   TSShapeLoader::addFormat("Stanford Polygon Library", "ply");
   TSShapeLoader::addFormat("AutoCAD DXF", "dxf");
   TSShapeLoader::addFormat("LightWave", "lwo");
   TSShapeLoader::addFormat("LightWave Scene", "lws");
   TSShapeLoader::addFormat("Modo", "lxo");
   TSShapeLoader::addFormat("Stereolithography", "stl");
   TSShapeLoader::addFormat("AC3D", "ac");
   TSShapeLoader::addFormat("Milkshape 3D", "ms3d");
   TSShapeLoader::addFormat("TrueSpace COB", "cob");
   TSShapeLoader::addFormat("TrueSpace SCN", "scn");
   TSShapeLoader::addFormat("Ogre XML", "xml");
   TSShapeLoader::addFormat("Irrlicht Mesh", "irrmesh");
   TSShapeLoader::addFormat("Irrlicht Scene", "irr");
   TSShapeLoader::addFormat("Quake I", "mdl");
   TSShapeLoader::addFormat("Quake II", "md2");
   TSShapeLoader::addFormat("Quake III Mesh", "md3");
   TSShapeLoader::addFormat("Quake III Map/BSP", "pk3");
   TSShapeLoader::addFormat("Return to Castle Wolfenstein", "mdc");
   TSShapeLoader::addFormat("Doom 3", "md5");
   TSShapeLoader::addFormat("Valve SMD", "smd");
   TSShapeLoader::addFormat("Valve VTA", "vta");
   TSShapeLoader::addFormat("Starcraft II M3", "m3");
   TSShapeLoader::addFormat("Unreal", "3d");
   TSShapeLoader::addFormat("BlitzBasic 3D", "b3d");
   TSShapeLoader::addFormat("Quick3D Q3D", "q3d");
   TSShapeLoader::addFormat("Quick3D Q3S", "q3s");
   TSShapeLoader::addFormat("Neutral File Format", "nff");
   TSShapeLoader::addFormat("Object File Format", "off");
   TSShapeLoader::addFormat("PovRAY Raw", "raw");
   TSShapeLoader::addFormat("Terragen Terrain", "ter");
   TSShapeLoader::addFormat("3D GameStudio (3DGS)", "mdl");
   TSShapeLoader::addFormat("3D GameStudio (3DGS) Terrain", "hmp");
   TSShapeLoader::addFormat("Izware Nendo", "ndo");
   TSShapeLoader::addFormat("gltf", "gltf");
   TSShapeLoader::addFormat("gltf binary", "glb");
}
MODULE_END;

const F32 TSShapeLoader::DefaultTime = -1.0f;
const F64 TSShapeLoader::MinFrameRate = 15.0f;
const F64 TSShapeLoader::MaxFrameRate = 60.0f;
const F64 TSShapeLoader::AppGroundFrameRate = 10.0f;
Torque::Path TSShapeLoader::shapePath;

Vector<TSShapeLoader::ShapeFormat> TSShapeLoader::smFormats;

//------------------------------------------------------------------------------
// UTILITY FUNCTIONS END
//-----------------------------------------------------------------------------

void assimpLogCallback(const char* message, char* user)
{
   Con::printf("Assimp log: %s", StringUnit::getUnit(message, 0, "\n"));
}

void TSShapeLoader::assimpToTorqueMat(const aiMatrix4x4& inAssimpMat, MatrixF& outMat)
{
   outMat.setRow(0, Point4F((F32)inAssimpMat.a1, (F32)inAssimpMat.a2,
      (F32)inAssimpMat.a3, (F32)inAssimpMat.a4));

   outMat.setRow(1, Point4F((F32)inAssimpMat.b1, (F32)inAssimpMat.b2,
      (F32)inAssimpMat.b3, (F32)inAssimpMat.b4));

   outMat.setRow(2, Point4F((F32)inAssimpMat.c1, (F32)inAssimpMat.c2,
      (F32)inAssimpMat.c3, (F32)inAssimpMat.c4));

   outMat.setRow(3, Point4F((F32)inAssimpMat.d1, (F32)inAssimpMat.d2,
      (F32)inAssimpMat.d3, (F32)inAssimpMat.d4));
}

void TSShapeLoader::extractTexture(U32 index, aiTexture* pTex)
{  // Cache an embedded texture to disk
   updateProgress(Load_EnumerateScene, "Extracting Textures...", mScene->mNumTextures, index);
   Con::printf("[Assimp] Extracting Texture %s, W: %d, H: %d, %d of %d, format hint: (%s)", pTex->mFilename.C_Str(),
      pTex->mWidth, pTex->mHeight, index, mScene->mNumTextures, pTex->achFormatHint);

   // Create the texture filename
   String cleanFile = AppMaterial::cleanString(TSShapeLoader::getShapePath().getFileName());
   String texName = String::ToString("%s_cachedTex%d", cleanFile.c_str(), index);
   Torque::Path texPath = shapePath;
   texPath.setFileName(texName);

   if (pTex->mHeight == 0)
   {  // Compressed format, write the data directly to disc
      texPath.setExtension(pTex->achFormatHint);
      FileStream* outputStream;
      if ((outputStream = FileStream::createAndOpen(texPath.getFullPath(), Torque::FS::File::Write)) != NULL)
      {
         outputStream->setPosition(0);
         outputStream->write(pTex->mWidth, pTex->pcData);
         outputStream->close();
         delete outputStream;
      }
   }
   else
   {  // Embedded pixel data, fill a bitmap and save it.
      GFXTexHandle shapeTex;
      shapeTex.set(pTex->mWidth, pTex->mHeight, GFXFormatR8G8B8A8_SRGB, &GFXDynamicTextureSRGBProfile,
         String::ToString("AssimpShapeLoader (%s:%i)", __FILE__, __LINE__), 1, 0);
      GFXLockedRect* rect = shapeTex.lock();

      for (U32 y = 0; y < pTex->mHeight; ++y)
      {
         for (U32 x = 0; x < pTex->mWidth; ++x)
         {
            U32 targetIndex = (y * rect->pitch) + (x * 4);
            U32 sourceIndex = ((y * pTex->mWidth) + x) * 4;
            rect->bits[targetIndex] = pTex->pcData[sourceIndex].r;
            rect->bits[targetIndex + 1] = pTex->pcData[sourceIndex].g;
            rect->bits[targetIndex + 2] = pTex->pcData[sourceIndex].b;
            rect->bits[targetIndex + 3] = pTex->pcData[sourceIndex].a;
         }
      }
      shapeTex.unlock();

      texPath.setExtension("png");
      shapeTex->dumpToDisk("PNG", texPath.getFullPath());
   }
}

void TSShapeLoader::zapScale(MatrixF& mat)
{
   Point3F invScale = mat.getScale();
   invScale.x = invScale.x ? (1.0f / invScale.x) : 0;
   invScale.y = invScale.y ? (1.0f / invScale.y) : 0;
   invScale.z = invScale.z ? (1.0f / invScale.z) : 0;
   mat.scale(invScale);
}

void TSShapeLoader::updateProgress(S32 major, const char* msg, S32 numMinor, S32 minor)
{
   // Calculate progress value
   F32 progress = (F32)major / (F32)NumLoadPhases;
   const char* progressMsg = msg;

   if (numMinor)
   {
      progress += (minor * (1.0f / (F32)NumLoadPhases) / numMinor);
      progressMsg = avar("%s (%d of %d)", msg, minor + 1, numMinor);
   }

   if (Con::isFunction("updateTSShapeLoadProgress"))
      Con::executef("updateTSShapeLoadProgress", Con::getFloatArg(progress), progressMsg);
}

typedef bool (*NameCmpFunc)(const String&, const Vector<String>&, void*, void*);

bool cmpNodeName(const String& key, const Vector<String>& names, void* arg1, void* arg2)
{
   for (S32 i = 0; i < names.size(); i++)
   {
      if (names[i].compare(key, 0, String::NoCase) == 0)
         return false;
   }
   return true;
}

bool cmpMeshNameAndSize(const String& key, const Vector<String>& names, void* arg1, void* arg2)
{
   S32 meshSize = (intptr_t)arg1;

   for (S32 i = 0; i < names.size(); i++)
   {
      S32 sizeCmp = 2;
      String name = String::GetTrailingNumber(names[i], sizeCmp);

      if (name.compare(key, 0, String::NoCase) == 0)
         if(meshSize == sizeCmp)
            return false;
         
   }

   return true;
}

String getUniqueName(const char* name, NameCmpFunc isNameUnique, const Vector<String>& names, void* arg1 = 0, void* arg2 = 0)
{
   const S32 MAX_ITERATIONS = 0x10000;   // maximum of 4 characters (A-P) will be appended

   String suffix;
   for (S32 i = 0; i < MAX_ITERATIONS; i++)
   {
      // Generate a suffix using the first 16 characters of the alphabet
      suffix.clear();
      for (S32 value = i; value != 0; value >>= 4)
         suffix = suffix + (char)('A' + (value & 0xF));

      String uname = name + suffix;
      if (isNameUnique(uname, names, arg1, arg2))
         return uname;
   }
   return name;
}

bool TSShapeLoader::isBillboard(const char* inName)
{
   return   !dStrnicmp(inName, "BB::", 4) ||
            !dStrnicmp(inName, "BB_", 3) ||
            isZBillboard(inName);
}

bool TSShapeLoader::isZBillboard(const char* inName)
{
   return   !dStrnicmp(inName, "BBZ::", 5) ||
            !dStrnicmp(inName, "BBZ_", 4);
}

bool TSShapeLoader::ignoreNode(const String& name)
{
   // Do not add AssimpFbx dummy nodes to the TSShape. See: Assimp::FBX::ImportSettings::preservePivots
   // https://github.com/assimp/assimp/blob/master/code/FBXImportSettings.h#L116-L135
   if (name.find("_$AssimpFbx$_") != String::NPos)
      return true;

   if (FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().alwaysImport, name, false))
      return false;

   return FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().neverImport, name, false);
}

bool TSShapeLoader::ignoreMesh(const String& name)
{
   if (FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().alwaysImportMesh, name, false))
      return false;
   else
      return FindMatch::isMatchMultipleExprs(ColladaUtils::getOptions().neverImportMesh, name, false);
}

//-----------------------------------------------------------------------------
// UTILITY FUNCTIONS END
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FORMAT FUNCTIONS
//-----------------------------------------------------------------------------

// Static functions to handle supported formats for shape loader.
void TSShapeLoader::addFormat(String name, String extension)
{
   ShapeFormat newFormat;
   newFormat.mName = name;
   newFormat.mExtension = extension;
   smFormats.push_back(newFormat);
}

String TSShapeLoader::getFormatExtensions()
{
   // "*.dsq TAB *.dae TAB
   StringBuilder output;
   for (U32 n = 0; n < smFormats.size(); ++n)
   {
      output.append("*.");
      output.append(smFormats[n].mExtension);
      output.append("\t");
   }
   return output.end();
}

String TSShapeLoader::getFormatFilters()
{
   // "DSQ Files|*.dsq|COLLADA Files|*.dae|"
   StringBuilder output;
   for (U32 n = 0; n < smFormats.size(); ++n)
   {
      output.append(smFormats[n].mName);
      output.append("|*.");
      output.append(smFormats[n].mExtension);
      output.append("|");
   }
   return output.end();
}

bool TSShapeLoader::isSupportedFormat(String extension)
{
   String extLower = String::ToLower(extension);
   for (U32 n = 0; n < smFormats.size(); ++n)
   {
      if (smFormats[n].mExtension.equal(extLower))
         return true;
   }
   return false;
}

//-----------------------------------------------------------------------------
// FORMAT FUNCTIONS END
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PROCESS FUNCTIONS
//-----------------------------------------------------------------------------

TSMesh* TSShapeLoader::constructTSMesh(aiMesh* inMesh)
{
   bool isSkin = false;

   for (U32 i = 0; i < inMesh->mNumBones; i++)
   {
      if (inMesh->mBones[i]->mNumWeights > 0)
      {
         isSkin = true;
         break;
      }
   }

   TSMesh* tsmesh;

   if (isSkin)
   {
      TSSkinMesh* tsskin = new TSSkinMesh();
      tsmesh = tsskin;
   }
   else
   {
      tsmesh = new TSMesh();
   }

   return tsmesh;
}

void TSShapeLoader::processMeshs(aiNode* inNode, S32 nodeIdx)
{
   const String* lastName = 0;

   for (S32 i = 0; i < inNode->mNumMeshes; i++)
   {
      aiMesh* mesh = mScene->mMeshes[inNode->mMeshes[i]];

      // if we ignore this mesh name, continue to the next.
      if (ignoreMesh(mesh->mName.C_Str()))
         continue;

      // get detail size.
      S32 detailSize = 2;
      String name = String::GetTrailingNumber(mesh->mName.C_Str(), detailSize);
      name = getUniqueName(name, cmpMeshNameAndSize, shapeMeshes, (void*)(uintptr_t)detailSize);

      // fix up collision details that dont have a negative level.
      if (dStrStartsWith(name, "Collision") ||
         dStrStartsWith(name, "LOSCol"))
      {
         if (detailSize > 0)
            detailSize = -detailSize;
      }

      // now join name and detail size.
      shapeMeshes.push_back(name + String::ToString(detailSize));

      // in assimp, these should already be sorted... i think....

      if (!lastName || (name != *lastName))
      {
         shape->objects.increment();
         TSShape::Object& lastObject = shape->objects.last();
         lastObject.nameIndex = shape->addName(name);
         lastObject.nodeIndex = nodeIdx;
         lastObject.startMeshIndex = shape->meshes.size(); // we are using tsmesh directly.
         lastObject.numMeshes = 0;
         lastName = &name;
      }

      shape->objects.last().numMeshes++;

      if (isBillboard(name))
      {
         if (isZBillboard(name))
         {

         }
      }

      // Set the detail name... do fixups for collision details.
      const char* detailName = "detail";
      if (detailSize < 0)
      {
         if (dStrStartsWith(name, "Collision") ||
            dStrStartsWith(name, "Col"))
            detailName = "Collision";
         else if (dStrStartsWith(name, "LOSCol"))
            detailName = "LOS";
      }

      // Attempt to add the detail (will fail if it already exists)
      S32 oldNumDetails = shape->details.size();
      shape->addDetail(detailName, detailSize, nodeIdx);
      if (shape->details.size() > oldNumDetails)
      {
         Con::warnf("Object mesh \"%s\" has no matching detail (\"%s%d\" has"
            " been added automatically)", name, detailName, detailSize);
      }

   }

}

void TSShapeLoader::processNode(aiNode* inNode, S32 parentIdx, bool recurse)
{
   if (dStricmp(inNode->mName.C_Str(), "bounds") == 0)
   {
      //return;
   }

   S32 subShapeNum = shape->subShapeFirstNode.size() - 1;

   // Check if we should collapse this node
   S32 myIndex;
   if (ignoreNode(inNode->mName.C_Str()))
   {
      myIndex = parentIdx;
   }
   else
   {
      // Check that adding this node will not exceed the maximum node count
      if (shape->nodes.size() >= MAX_TS_SET_SIZE)
         return;

      myIndex = shape->nodes.size();

      String nodeName = getUniqueName(inNode->mName.C_Str(), cmpNodeName, shape->names);

      // Create the 3space node
      shape->nodes.increment();
      TSShape::Node& lastNode = shape->nodes.last();
      lastNode.nameIndex = shape->addName(nodeName);
      lastNode.parentIndex = parentIdx;
      lastNode.firstObject = -1;
      lastNode.firstChild = -1;
      lastNode.nextSibling = -1;

      if (inNode->mNumChildren == 0 && inNode->mNumMeshes == 0)
      {
         S32 size = 0x7FFFFFFF;
         String dname(String::GetTrailingNumber(inNode->mName.C_Str(), size));

         if (dStrEqual(dname, "nulldetail") && (size != 0x7FFFFFFF))
         {
            shape->addDetail("detail", size, subShapeNum);
         }
         else if (isBillboard(inNode->mName.C_Str()) && (size != 0x7FFFFFFF))
         {
            // AutoBillboard detail
            S32 numEquatorSteps = 4;
            S32 numPolarSteps = 0;
            F32 polarAngle = 0.0f;
            S32 dl = 0;
            S32 dim = 64;
            bool includePoles = true;

            S32 detIndex = shape->addDetail("bbDetail", size, -1);

            TSShape::Detail& detIndexDetail = shape->details[detIndex];
            detIndexDetail.bbEquatorSteps = numEquatorSteps;
            detIndexDetail.bbPolarSteps = numPolarSteps;
            detIndexDetail.bbDetailLevel = dl;
            detIndexDetail.bbDimension = dim;
            detIndexDetail.bbIncludePoles = includePoles;
            detIndexDetail.bbPolarAngle = polarAngle;
         }
      }
   }

   // collect geometry.
   if (inNode->mNumMeshes > 0)
   {
      processMeshs(inNode, myIndex);
   }

   if (recurse)
   {
      for (S32 child = 0; child < inNode->mNumChildren; child++)
      {
         processNode(inNode->mChildren[child], myIndex, true);
      }
   }
}

//-----------------------------------------------------------------------------
// PROCESS FUNCTIONS END
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// GENERATE FUNCTIONS
//-----------------------------------------------------------------------------

TSShape* TSShapeLoader::generateShape(const Torque::Path& path)
{
   // initializers.
   shapePath = path;
   shape = new TSShape();

   shape->mExporterVersion = 124;
   shape->mSmallestVisibleSize = 999999;
   shape->mSmallestVisibleDL = 0;
   shape->mReadVersion = 24;
   shape->mFlags = 0;
   shape->mSequencesConstructed = 0;

   S32 firstNode = shape->nodes.size();
   shape->subShapeFirstNode.push_back(firstNode);

   // Update progress
   updateProgress(Load_EnumerateScene, "Loading scene data...");

   // Post-Processing
   unsigned int ppsteps =
      (ColladaUtils::getOptions().convertLeftHanded ? aiProcess_MakeLeftHanded : 0) |
      (ColladaUtils::getOptions().reverseWindingOrder ? aiProcess_FlipWindingOrder : 0) |
      (ColladaUtils::getOptions().calcTangentSpace ? aiProcess_CalcTangentSpace : 0) |
      (ColladaUtils::getOptions().joinIdenticalVerts ? aiProcess_JoinIdenticalVertices : 0) |
      (ColladaUtils::getOptions().removeRedundantMats ? aiProcess_RemoveRedundantMaterials : 0) |
      (ColladaUtils::getOptions().genUVCoords ? aiProcess_GenUVCoords : 0) |
      (ColladaUtils::getOptions().transformUVCoords ? aiProcess_TransformUVCoords : 0) |
      (ColladaUtils::getOptions().flipUVCoords ? aiProcess_FlipUVs : 0) |
      (ColladaUtils::getOptions().findInstances ? aiProcess_FindInstances : 0) |
      (ColladaUtils::getOptions().limitBoneWeights ? aiProcess_LimitBoneWeights : 0);

   if (Con::getBoolVariable("$Assimp::OptimizeMeshes", false))
      ppsteps |= aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;

   if (Con::getBoolVariable("$Assimp::SplitLargeMeshes", false))
      ppsteps |= aiProcess_SplitLargeMeshes;

   // Mandatory options
   //ppsteps |= aiProcess_ValidateDataStructure | aiProcess_Triangulate | aiProcess_ImproveCacheLocality;
   ppsteps |= aiProcess_Triangulate;
   //aiProcess_SortByPType              | // make 'clean' meshes which consist of a single typ of primitives

   aiPropertyStore* props = aiCreatePropertyStore();

   struct aiLogStream shapeLog = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
   shapeLog.callback = assimpLogCallback;
   shapeLog.user = 0;
   aiAttachLogStream(&shapeLog);
#ifdef TORQUE_DEBUG
   aiEnableVerboseLogging(true);
#endif

   updateProgress(Load_ReadFile, "Reading File...");
   Con::printf("Attempting to load file: %s", shapePath.getFullPath().c_str());
   mScene = (aiScene*)aiImportFileExWithProperties(shapePath.getFullPath().c_str(), ppsteps, NULL, props);

   aiReleasePropertyStore(props);

   if (mScene)
   {
      Con::printf("Mesh Count: %d", mScene->mNumMeshes);
      Con::printf("Material Count: %d", mScene->mNumMaterials);

      // Setup default units for shape format
      String importFormat;

      String fileExt = String::ToLower(shapePath.getExtension());
      const aiImporterDesc* importerDescription = aiGetImporterDesc(fileExt.c_str());
      if (importerDescription && StringTable->insert(importerDescription->mName) == StringTable->insert("Autodesk FBX Importer"))
      {
         ColladaUtils::getOptions().formatScaleFactor = 0.01f;
      }

      //---------------------------------- 
      // handle nodes and put them to tsshape.
      // initializers for tsshape data.
         // TSShapeLoader::processNode was always only ever called once
         // so it only ever created 1 subshape.
         S32 firstNode = shape->nodes.size();
         shape->subShapeFirstNode.push_back(firstNode);
         shape->subShapeFirstObject.push_back(shape->objects.size());
         processNode(mScene->mRootNode, -1, true);
         shape->subShapeNumNodes.push_back(shape->nodes.size() - firstNode);
         shape->subShapeNumObjects.push_back(shape->objects.size() - shape->subShapeFirstObject.last());
      // nodes processed.
      //---------------------------------- 

   }

}

//-----------------------------------------------------------------------------
// GENERATE FUNCTIONS END
//-----------------------------------------------------------------------------
