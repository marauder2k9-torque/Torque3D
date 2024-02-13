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

#ifndef _TSSHAPE_LOADER_H_
#define _TSSHAPE_LOADER_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _APPNODE_H_
#include "ts/loader/appNode.h"
#endif
#ifndef _APPMESH_H_
#include "ts/loader/appMesh.h"
#endif
#ifndef _APPSEQUENCE_H_
#include "ts/loader/appSequence.h"
#endif

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

struct aiNode;
struct aiMetadata;
struct aiTexture;

class TSShapeLoader
{

// Supported Format List
protected:
   struct ShapeFormat
   {
      String mName;
      String mExtension;
   };
   static Vector<ShapeFormat> smFormats;
public:
   static void addFormat(String name, String extension);
   static String getFormatExtensions();
   static String getFormatFilters();
   static bool isSupportedFormat(String extension);

public:
   enum eLoadPhases
   {
      Load_ReadFile = 0,
      Load_ParseFile,
      Load_ExternalRefs,
      Load_EnumerateScene,
      Load_GenerateSubshapes,
      Load_GenerateObjects,
      Load_GenerateDefaultStates,
      Load_GenerateSkins,
      Load_GenerateMaterials,
      Load_GenerateSequences,
      Load_InitShape,
      NumLoadPhases,
      Load_Complete = NumLoadPhases
   };

   static void updateProgress(S32 major, const char* msg, S32 numMinor=0, S32 minor=0);

public:
   static const F32 DefaultTime;
   static const F64 MinFrameRate;
   static const F64 MaxFrameRate;
   static const F64 AppGroundFrameRate;

protected:
   // Variables used during loading that must be held until the shape is deleted
   const struct aiScene*         mScene;
   TSShape*                      shape;

   // Variables used during loading, but that can be discarded afterwards
   static Torque::Path           shapePath;
   Vector<String>                shapeMeshes;

   Vector<QuatF*>                nodeRotCache;
   Vector<Point3F*>              nodeTransCache;
   Vector<Point3F*>              nodeScaleCache;

   //--------------------------------------------------------------------------
   TSMesh* constructTSMesh(aiMesh* inMesh);
   void processMeshs(aiNode* inNode, S32 nodeIdx);
   void processNode(aiNode* inNode, S32 parentIdx, bool recurse);
   bool isBillboard(const char* inName);
   bool isZBillboard(const char* inName);

   // Collect the nodes, objects and sequences for the scene
   bool ignoreNode(const String& name);
   bool ignoreMesh(const String& name);

   void addSkin(AppMesh* mesh);
   void addDetailMesh(AppMesh* mesh);
   void addSubshape(AppNode* node);
   void addObject(AppMesh* mesh, S32 nodeIndex, S32 subShapeNum);

   // Node transform methods
   MatrixF getLocalNodeMatrix(AppNode* node, F32 t);
   void generateNodeTransform(AppNode* node, F32 t, bool blend, F32 referenceTime,
                              QuatF& rot, Point3F& trans, Point3F& scale);

   virtual void computeBounds(Box3F& bounds);

   void generateSkins();
   void generateDefaultStates();
   void generateObjectState(TSShape::Object& obj, F32 t, bool addFrame, bool addMatFrame);
   void generateFrame(TSShape::Object& obj, F32 t, bool addFrame, bool addMatFrame);

   void generateMaterialList();

   void generateSequences();

   // Determine what is actually animated in the sequence
   void setNodeMembership(TSShape::Sequence& seq, const AppSequence* appSeq);
   void setRotationMembership(TSShape::Sequence& seq);
   void setTranslationMembership(TSShape::Sequence& seq);
   void setScaleMembership(TSShape::Sequence& seq);
   void setObjectMembership(TSShape::Sequence& seq, const AppSequence* appSeq);

   // Manage a cache of all node transform elements for the sequence
   void clearNodeTransformCache();
   void fillNodeTransformCache(TSShape::Sequence& seq, const AppSequence* appSeq);

   // Add node transform elements
   void addNodeRotation(QuatF& rot, bool defaultVal);
   void addNodeTranslation(Point3F& trans, bool defaultVal);
   void addNodeUniformScale(F32 scale);
   void addNodeAlignedScale(Point3F& scale);
   void addNodeArbitraryScale(QuatF& qrot, Point3F& scale);

   // Generate animation data
   void generateNodeAnimation(TSShape::Sequence& seq);
   void generateObjectAnimation(TSShape::Sequence& seq, const AppSequence* appSeq);
   void generateGroundAnimation(TSShape::Sequence& seq, const AppSequence* appSeq);
   void generateFrameTriggers(TSShape::Sequence& seq, const AppSequence* appSeq);

   // Shape construction
   void sortDetails();
   void install();

   void assimpToTorqueMat(const aiMatrix4x4& inAssimpMat, MatrixF& outMat);
   void extractTexture(U32 index, aiTexture* pTex);

public:
   TSShapeLoader() : shape(NULL), mScene(NULL) { }
   virtual ~TSShapeLoader();

   static const Torque::Path& getShapePath() { return shapePath; }

  
   static void zapScale(MatrixF& mat);

   TSShape* generateShape(const Torque::Path& path);
};

#endif // _TSSHAPE_LOADER_H_
