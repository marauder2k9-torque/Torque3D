//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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

#ifndef _MODEL_ASSET_H_
#define _MODEL_ASSET_H_
#pragma once

#ifndef _ASSET_BASE_H_
#include "assets/assetBase.h"
#endif

#ifndef _ASSET_DEFINITION_H_
#include "assets/assetDefinition.h"
#endif

#ifndef _STRINGUNIT_H_
#include "string/stringUnit.h"
#endif

#ifndef _ASSET_FIELD_TYPES_H_
#include "assets/assetFieldTypes.h"
#endif

#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif

#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif

#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif

#ifndef MATERIALASSET_H
#include "MaterialAsset.h"
#endif

#ifndef SHAPE_ANIMATION_ASSET_H
#include "ShapeAnimationAsset.h"
#endif

class ModelAsset : public AssetBase
{
   typedef AssetBase Parent;

protected:
   StringTableEntry   mFileName;
   StringTableEntry   mConstructorFileName;
   Resource<TSShape>	 mShape;
   StringTableEntry   mDiffuseImposterFileName;
   StringTableEntry   mNormalImposterFileName;

   //Material assets we're dependent on and use
   Vector<StringTableEntry> mMaterialAssetIds;
   Vector<AssetPtr<MaterialAsset>> mMaterialAssets;

   //Animation assets we're dependent on and use
   Vector<StringTableEntry> mAnimationAssetIds;
   Vector<AssetPtr<ShapeAnimationAsset>> mAnimationAssets;

   Signal<void()> mReloadSignal;
public:
   enum ModelAssetErrCode
   {
      TooManyVerts = AssetErrCode::Extended,
      TooManyBones,
      MissingAnimatons,
      Extended
   };
   static StringTableEntry smNoShapeAssetFallback;
   static const String mErrCodeStrings[U32(ModelAssetErrCode::Extended) - U32(Parent::Extended) + 1];
   static U32 getAssetErrCode(AssetPtr<ModelAsset> checkAsset) { if (checkAsset) return checkAsset->mLoadedState; else return 0; }

   ModelAsset();
   virtual ~ModelAsset();

   /// Set up some global script interface stuff.
   static void consoleInit();
   /// Engine.
   static void initPersistFields();
   void copyTo(SimObject* object) override;

   DECLARE_CONOBJECT(ModelAsset);

   bool onAdd() override;
   void onRemove() override;

   bool reload();

   TSShape* getShape() { createShapeResource(); return mShape; }
   Resource<TSShape> getShapeResource() { createShapeResource(); return mShape; }

   Signal<void()> getReloadSignal() { return mReloadSignal; }

   // setters/getters
   void                    setShapeFile(const char* pScriptFile);
   inline StringTableEntry getShapeFile(void) const { return mFileName; };

   void                    setShapeConstructorFile(const char* pScriptFile);
   inline StringTableEntry getConstructorFile(void) const { return mConstructorFileName; };

   void                    setDiffuseImposterFile(const char* pScriptFile);
   inline StringTableEntry getDiffuseImposterFile(void) const { return mDiffuseImposterFileName; };

   void                    setNormalImposterFile(const char* pScriptFile);
   inline StringTableEntry getNormalImposterFile(void) const { return mNormalImposterFileName; };

#ifdef TORQUE_TOOLS
   const char* generateCachedPreviewImage(S32 resolution, String overrideMaterial = "");
#endif
private:
   void createShapeResource();

protected:
   void onAssetRefresh(void) override;
   void onTamlPreWrite(void) override;
   void onTamlPostWrite(void) override;
   void initializeAsset(void) override;

   void _onFileChanged(const Torque::Path& path) { reload(); }

protected:
   // init persist fields set, get, write
   static bool setShapeFile(void* obj, const char* index, const char* data) { static_cast<ModelAsset*>(obj)->setShapeFile(data); return false; }
   static const char* getShapeFile(void* obj, const char* data) { return static_cast<ModelAsset*>(obj)->getShapeFile(); }
   static bool writeShapeFile(void* obj, StringTableEntry pFieldName) { return static_cast<ModelAsset*>(obj)->getShapeFile() != StringTable->EmptyString(); }

   static bool setConstructorFile(void* obj, const char* index, const char* data) { static_cast<ModelAsset*>(obj)->setShapeConstructorFile(data); return false; }
   static const char* getConstructorFile(void* obj, const char* data) { return static_cast<ModelAsset*>(obj)->getConstructorFile(); }
   static bool writeConstructorFile(void* obj, StringTableEntry pFieldName) { return static_cast<ModelAsset*>(obj)->getConstructorFile() != StringTable->EmptyString(); }

   static bool setDiffuseImposterFile(void* obj, const char* index, const char* data) { static_cast<ModelAsset*>(obj)->setDiffuseImposterFile(data); return false; }
   static const char* getDiffuseImposterFile(void* obj, const char* data) { return static_cast<ModelAsset*>(obj)->getDiffuseImposterFile(); }
   static bool writeDiffuseImposterFile(void* obj, StringTableEntry pFieldName) { return static_cast<ModelAsset*>(obj)->getDiffuseImposterFile() != StringTable->EmptyString(); }

   static bool setNormalImposterFile(void* obj, const char* index, const char* data) { static_cast<ModelAsset*>(obj)->setNormalImposterFile(data); return false; }
   static const char* getNormalImposterFile(void* obj, const char* data) { return static_cast<ModelAsset*>(obj)->getNormalImposterFile(); }
   static bool writeNormalImposterFile(void* obj, StringTableEntry pFieldName) { return static_cast<ModelAsset*>(obj)->getNormalImposterFile() != StringTable->EmptyString(); }
};

DefineConsoleType(TypeModelAssetPtr, ModelAsset)

#ifdef TORQUE_TOOLS

class GuiInspectorTypeModelAssetPtr : public GuiInspectorTypeFileName
{
   typedef GuiInspectorTypeFileName Parent;
public:

   GuiTextCtrl* mLabel;
   GuiBitmapButtonCtrl* mPreviewBorderButton;
   GuiBitmapCtrl* mPreviewImage;
   GuiButtonCtrl* mEditButton;

   DECLARE_CONOBJECT(GuiInspectorTypeModelAssetPtr);
   static void consoleInit();

   GuiControl* constructEditControl() override;
   bool updateRects() override;

   void updateValue() override;

   void updatePreviewImage();
   void setPreviewImage(StringTableEntry assetId);
};
#endif // TORQUE_TOOLS

#pragma region Singular Asset Macros 
#define INIT_SHAPE_ASSET(name)\
m##name##Asset = NULL

#define CLONE_SHAPE_ASSET(name)\
m##name##Asset = other.m##name##Asset

#define DECLARE_SHAPE_ASSET(className, name, changeFunc) public:\
AssetPtr<ModelAsset> m##name##Asset;\
AssetPtr<ModelAsset> get##name##Asset() const {return m##name##Asset; }\
protected:\
static bool _set##name##Data(void* obj, const char* index, const char* data){static_cast<className*>(obj)->set##name##(data); return false; }\
public:\
bool set##name##(const char* data){ m##name##Asset = data; return true; }\
Resource<TSShape> get##name##Resource() \
{\
   return m##name##Asset->getShapeResource();\
}\

#define INITPERSISTFIELD_SHAPE_ASSET(name, consoleClass, docs) addProtectedField(assetText(name, Asset), TypeModelAssetPtr, Offset(m##name##Asset, consoleClass), &_set##name##Data, &defaultProtectedGetFn, assetText(name, docs));\

#define INITPERSISTFIELD_SHAPE_ASSET_FLAG(name, consoleClass, docs, flag) addProtectedField(assetText(name, Asset), TypeModelAssetPtr, Offset(m##name##Asset, consoleClass), &_set##name##Data, &defaultProtectedGetFn, assetText(name, docs), flag);\

//network send - object-instance
#define PACK_SHAPE_ASSET(netconn, name)\
if (stream->writeFlag(m##name##Asset.notNull()))\
{\
   NetStringHandle assetIdStr = m##name##Asset.getAssetId();\
   netconn->packNetStringHandleU(stream, assetIdStr);\
}\

//network recieve - object-instance
#define UNPACK_SHAPE_ASSET(netconn, name) \
if (stream->readFlag()) \
{\
   set##name##(netconn->unpackNetStringHandleU(stream).getString());\
}\

//network send - datablock
#define PACKDATA_SHAPE_ASSET(name)\
if (stream->writeFlag(m##name##Asset.notNull()))\
{\
   stream->writeString(m##name##Asset.getAssetId());\
}\

//network recieve - datablock
#define UNPACKDATA_SHAPE_ASSET(name)\
if (stream->readFlag())\
{\
   set##name##(stream->readSTString());\
}\

//---------- ARRAY
#define INIT_SHAPE_ASSET_ARRAY(name, index)\
m##name##Asset[index] = NULL

#define DECLARE_SHAPE_ASSET_ARRAY(className, name, max)public:\
AssetPtr<ModelAsset> m##name##Asset[max];\
AssetPtr<ModelAsset> get##name##Asset(const U32& index) const {return m##name##Asset[index]; }\
protected:\
static bool _set##name##Data(void* obj, const char* index, const char* data){static_cast<className*>(obj)->set##name##(data, dAtoi(index)); return false; }\
public:\
bool set##name##(const char* data, const U32& index){ m##name##Asset[index] = data; return true; }\
Resource<TSShape> get##name##Resource(const U32& index) \
{\
   return m##name##Asset[index]->getShapeResource();\
}\

#define INITPERSISTFIELD_SHAPE_ASSET_ARRAY(name, consoleClass, docs, arraySize) addProtectedField(assetText(name, Asset), TypeModelAssetPtr, Offset(m##name##Asset, consoleClass), &_set##name##Data, &defaultProtectedGetFn, arraySize, assetText(name, docs));\

//network send - object-instance
#define PACK_SHAPE_ASSET_ARRAY(netconn, name, index)\
if (stream->writeFlag(m##name##Asset[index].notNull()))\
{\
   NetStringHandle assetIdStr = m##name##Asset[index].getAssetId();\
   netconn->packNetStringHandleU(stream, assetIdStr);\
}\

//network recieve - object-instance
#define UNPACK_SHAPE_ASSET_ARRAY(netconn, name, index) \
if (stream->readFlag())\
{\
   m##name##AssetId[index] = StringTable->insert(netconn->unpackNetStringHandleU(stream).getString());\
   set##name(m##name##AssetId[index], index);\
}\

//network send - datablock
#define PACKDATA_SHAPE_ASSET_ARRAY(name, index)\
if (stream->writeFlag(m##name##Asset[index].notNull()))\
{\
   stream->writeString(m##name##Asset[index].getAssetId());\
}\

//network recieve - datablock
#define UNPACKDATA_SHAPE_ASSET_ARRAY(name, index)\
if (stream->readFlag())\
{\
   set##name(stream->readSTString(), index);\
}\

#pragma endregion

#endif
