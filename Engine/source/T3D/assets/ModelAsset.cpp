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
#include "T3D/assets/ModelAsset.h"
#endif // !_MODEL_ASSET_H_

#ifndef _ASSET_MANAGER_H_
#include "assets/assetManager.h"
#endif

#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif

#ifndef _TAML_
#include "persistence/taml/taml.h"
#endif

#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif

// Debug Profiling.
#include "platform/profiler.h"

StringTableEntry ModelAsset::smNoShapeAssetFallback = NULL;

IMPLEMENT_CONOBJECT(ModelAsset);

ConsoleType(ModelAssetPtr, TypeModelAssetPtr, ModelAsset, ASSET_ID_FIELD_PREFIX)

ConsoleGetType(TypeModelAssetPtr)
{
   return (*((AssetPtr<ModelAsset>*)dptr)).getAssetId();
}

ConsoleSetType(TypeModelAssetPtr)
{
   if (argc == 1)
   {
      const char* pFieldValue = argv[0];

      AssetPtr<ModelAsset>* pAssetPtr = dynamic_cast<AssetPtr<ModelAsset>*>((AssetPtrBase*)(dptr));

      if (pAssetPtr == NULL)
      {
         //Con::warnf("(TypeModelAssetPtr) - Failed to set asset id '%d'.", pFieldValue);
         return;
      }

      pAssetPtr->setAssetId(pFieldValue);

      return;
   }

   // Warn.
   Con::warnf("(TypeModelAssetPtr) - Cannot set multiple args to a single asset.");
}

const String ModelAsset::mErrCodeStrings[] =
{
   "TooManyVerts",
   "TooManyBones",
   "MissingAnimatons",
   "UnKnown"
};

ModelAsset::ModelAsset()
{
   mFileName = StringTable->EmptyString();
   mConstructorFileName = StringTable->EmptyString();
   mDiffuseImposterFileName = StringTable->EmptyString();
   mNormalImposterFileName = StringTable->EmptyString();

   mLoadedState = AssetErrCode::NotLoaded;
}

ModelAsset::~ModelAsset()
{
}

//-----------------------------------------------------------------------------
// SIM
//-----------------------------------------------------------------------------
void ModelAsset::consoleInit()
{
   Parent::consoleInit();

   Con::addVariable("$Core::NoShapeAssetFallback", TypeString, &smNoShapeAssetFallback,
      "The assetId of the shape to display when the requested shape asset is missing.\n"
      "@ingroup GFX\n");

   smNoShapeAssetFallback = StringTable->insert(Con::getVariable("$Core::NoShapeAssetFallback"));
}

void ModelAsset::initPersistFields()
{
   docsURL;
   // Call parent.
   Parent::initPersistFields();

   addProtectedField("modelFile", TypeAssetLooseFilePath, Offset(mFileName, ModelAsset),
      &setShapeFile, &getShapeFile, &writeShapeFile, "Path to shape File");

   addProtectedField("constuctorFileName", TypeAssetLooseFilePath, Offset(mConstructorFileName, ModelAsset),
      &setConstructorFile, &getConstructorFile, &writeConstructorFile, "Path to shape script file");

   addProtectedField("diffuseImposterFileName", TypeAssetLooseFilePath, Offset(mDiffuseImposterFileName, ModelAsset),
      &setDiffuseImposterFile, &getDiffuseImposterFile, &writeDiffuseImposterFile, "Path to the diffuse imposter file we want to render");

   addProtectedField("normalImposterFileName", TypeAssetLooseFilePath, Offset(mNormalImposterFileName, ModelAsset),
      &setNormalImposterFile, &getNormalImposterFile, &writeNormalImposterFile, "Path to the normal imposter file we want to render");
}

void ModelAsset::copyTo(SimObject* object)
{
   Parent::copyTo(object);

   ModelAsset* pAsset = static_cast<ModelAsset*>(object);

   // Sanity!
   AssertFatal(pAsset != NULL, "ModelAsset::copyTo() - Object is not the correct type.");

   pAsset->setShapeFile(getShapeFile());
   pAsset->setShapeConstructorFile(getConstructorFile());
   pAsset->setDiffuseImposterFile(getDiffuseImposterFile());
   pAsset->setNormalImposterFile(getNormalImposterFile());
}

bool ModelAsset::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void ModelAsset::onRemove()
{
   // Call Parent.
   Parent::onRemove();
}

bool ModelAsset::reload()
{
   refreshAsset();

   //createShapeResource();

   if (mLoadedState == AssetErrCode::Ok)
      return true;

   return false;
}

//-----------------------------------------------------------------------------
// ASSET HANDLING / CALLBACK
//-----------------------------------------------------------------------------

void ModelAsset::initializeAsset(void)
{
   // Call parent.
   Parent::initializeAsset();

   if (mFileName == StringTable->EmptyString())
      return;

   ResourceManager::get().getChangedSignal().notify(this, &ModelAsset::_onFileChanged);

   mFileName = expandAssetFilePath(mFileName);
   mConstructorFileName = expandAssetFilePath(mConstructorFileName);
   if (!Torque::FS::IsFile(mConstructorFileName))
      Con::errorf("ShapeAsset::initializeAsset (%s) could not find %s!", getAssetName(), mConstructorFileName);

   mDiffuseImposterFileName = expandAssetFilePath(mDiffuseImposterFileName);

   if (mDiffuseImposterFileName == StringTable->EmptyString())
   {
      String diffusePath = String(mFileName) + "_imposter.dds";
      mDiffuseImposterFileName = StringTable->insert(diffusePath.c_str());
   }

   mNormalImposterFileName = expandAssetFilePath(mNormalImposterFileName);

   if (mNormalImposterFileName == StringTable->EmptyString())
   {
      String normalPath = String(mFileName) + "_imposter_normals.dds";
      mNormalImposterFileName = StringTable->insert(normalPath.c_str());
   }
}

void ModelAsset::onAssetRefresh(void)
{
   if (!isProperlyAdded())
      return;

   Parent::onAssetRefresh();

   createShapeResource();
}

void ModelAsset::onTamlPreWrite(void)
{
   // Call parent.
   Parent::onTamlPreWrite();

   // Ensure the files are collapsed.
   mFileName = collapseAssetFilePath(mFileName);
   mConstructorFileName = collapseAssetFilePath(mConstructorFileName);
   mDiffuseImposterFileName = collapseAssetFilePath(mDiffuseImposterFileName);
   mNormalImposterFileName = collapseAssetFilePath(mNormalImposterFileName);
}

void ModelAsset::onTamlPostWrite(void)
{
   // Call parent.
   Parent::onTamlPostWrite();

   // Ensure the files are expanded.
   mFileName = expandAssetFilePath(mFileName);
   mConstructorFileName = expandAssetFilePath(mConstructorFileName);
   mDiffuseImposterFileName = expandAssetFilePath(mDiffuseImposterFileName);
   mNormalImposterFileName = expandAssetFilePath(mNormalImposterFileName);
}
#ifdef TORQUE_TOOLS
const char* ModelAsset::generateCachedPreviewImage(S32 resolution, String overrideMaterial)
{
   return nullptr;
}
#endif

void ModelAsset::createShapeResource()
{
   // if loaded do nothing.
   if (mLoadedState == AssetErrCode::Ok)
      return;

   // Debug Profiling.
   PROFILE_SCOPE(ModelAsset_createShapeResource);

   mMaterialAssets.clear();
   mMaterialAssetIds.clear();

   //First, load any material, animation, etc assets we may be referencing in our asset
   // Find any asset dependencies.
   AssetManager::typeAssetDependsOnHash::Iterator assetDependenciesItr = mpOwningAssetManager->getDependedOnAssets()->find(mpAssetDefinition->mAssetId);

   // Does the asset have any dependencies?
   if (assetDependenciesItr != mpOwningAssetManager->getDependedOnAssets()->end())
   {
      // Iterate all dependencies.
      while (assetDependenciesItr != mpOwningAssetManager->getDependedOnAssets()->end() && assetDependenciesItr->key == mpAssetDefinition->mAssetId)
      {
         StringTableEntry assetType = mpOwningAssetManager->getAssetType(assetDependenciesItr->value);

         if (assetType == StringTable->insert("MaterialAsset"))
         {
            mMaterialAssetIds.push_front(assetDependenciesItr->value);

            //Force the asset to become initialized if it hasn't been already
            AssetPtr<MaterialAsset> matAsset = assetDependenciesItr->value;

            mMaterialAssets.push_front(matAsset);
         }
         else if (assetType == StringTable->insert("ShapeAnimationAsset"))
         {
            mAnimationAssetIds.push_back(assetDependenciesItr->value);

            //Force the asset to become initialized if it hasn't been already
            AssetPtr<ShapeAnimationAsset> animAsset = assetDependenciesItr->value;

            mAnimationAssets.push_back(animAsset);
         }

         // Next dependency.
         assetDependenciesItr++;
      }
   }

   mShape = ResourceManager::get().load(mFileName);

   if (!mShape)
   {
      Con::errorf("ShapeAsset::loadShape : failed to load shape file %s (%s)!", getAssetName(), mFileName);
      mLoadedState = BadFileReference;
      return; //if it failed to load, bail out
   }

   // Construct billboards if not done already
   if (GFXDevice::devicePresent())
      mShape->setupBillboardDetails(mFileName, mDiffuseImposterFileName, mNormalImposterFileName);

   bool hasBlends = false;

   //Now that we've successfully loaded our shape and have any materials and animations loaded
   //we need to set up the animations we're using on our shape
   for (S32 i = mAnimationAssets.size() - 1; i >= 0; --i)
   {
      String srcName = mAnimationAssets[i]->getAnimationName();
      String srcPath(mAnimationAssets[i]->getAnimationFilename());
      //SplitSequencePathAndName(srcPath, srcName);

      if (!mShape->addSequence(srcPath, srcName, srcName,
         mAnimationAssets[i]->getStartFrame(), mAnimationAssets[i]->getEndFrame(), mAnimationAssets[i]->getPadRotation(), mAnimationAssets[i]->getPadTransforms()))
      {
         mLoadedState = MissingAnimatons;
         return;
      }
      if (mAnimationAssets[i]->isBlend())
         hasBlends = true;
   }

   //if any of our animations are blends, set those up now
   if (hasBlends)
   {
      for (U32 i = 0; i < mAnimationAssets.size(); ++i)
      {
         if (mAnimationAssets[i]->isBlend() && mAnimationAssets[i]->getBlendAnimationName() != StringTable->EmptyString())
         {
            //gotta do a bit of logic here.
            //First, we need to make sure the anim asset we depend on for our blend is loaded
            AssetPtr<ShapeAnimationAsset> blendAnimAsset = mAnimationAssets[i]->getBlendAnimationName();

            U32 assetStatus = ShapeAnimationAsset::getAssetErrCode(blendAnimAsset);
            if (assetStatus != AssetBase::Ok)
            {
               Con::errorf("ShapeAsset::initializeAsset - Unable to acquire reference animation asset %s for asset %s to blend!", mAnimationAssets[i]->getBlendAnimationName(), mAnimationAssets[i]->getAssetName());
               {
                  mLoadedState = MissingAnimatons;
                  return;
               }
            }

            String refAnimName = blendAnimAsset->getAnimationName();
            if (!mShape->setSequenceBlend(mAnimationAssets[i]->getAnimationName(), true, blendAnimAsset->getAnimationName(), mAnimationAssets[i]->getBlendFrame()))
            {
               Con::errorf("ShapeAnimationAsset::initializeAsset - Unable to set animation clip %s for asset %s to blend!", mAnimationAssets[i]->getAnimationName(), mAnimationAssets[i]->getAssetName());
               {
                  mLoadedState = MissingAnimatons;
                  return;
               }
            }
         }
      }
   }

   mLoadedState = AssetErrCode::Ok;

}

//-----------------------------------------------------------------------------
// SETTERS
//-----------------------------------------------------------------------------

void ModelAsset::setShapeFile(const char* pScriptFile)
{
   AssertFatal(pScriptFile != NULL, "Cannot use null shape file.");

   pScriptFile = StringTable->insert(pScriptFile);

   if (pScriptFile == mFileName)
      return;

   mFileName = getOwned() ? expandAssetFilePath(pScriptFile) : StringTable->insert(pScriptFile);

   // after every setter.
   refreshAsset();
}

void ModelAsset::setShapeConstructorFile(const char* pScriptFile)
{
   AssertFatal(pScriptFile != NULL, "Cannot use null shape file.");

   pScriptFile = StringTable->insert(pScriptFile);

   if (pScriptFile == mConstructorFileName)
      return;

   mConstructorFileName = getOwned() ? expandAssetFilePath(pScriptFile) : StringTable->insert(pScriptFile);

   // after every setter.
   refreshAsset();
}

void ModelAsset::setDiffuseImposterFile(const char* pScriptFile)
{
   AssertFatal(pScriptFile != NULL, "Cannot use null shape file.");

   pScriptFile = StringTable->insert(pScriptFile);

   if (pScriptFile == mDiffuseImposterFileName)
      return;

   mDiffuseImposterFileName = getOwned() ? expandAssetFilePath(pScriptFile) : StringTable->insert(pScriptFile);

   // after every setter.
   refreshAsset();
}

void ModelAsset::setNormalImposterFile(const char* pScriptFile)
{
   AssertFatal(pScriptFile != NULL, "Cannot use null shape file.");

   pScriptFile = StringTable->insert(pScriptFile);

   if (pScriptFile == mNormalImposterFileName)
      return;

   mNormalImposterFileName = getOwned() ? expandAssetFilePath(pScriptFile) : StringTable->insert(pScriptFile);

   // after every setter.
   refreshAsset();
}

#ifdef TORQUE_TOOLS
//-----------------------------------------------------------------------------
// GuiInspectorTypeAssetId
//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiInspectorTypeModelAssetPtr);

ConsoleDocClass(GuiInspectorTypeModelAssetPtr,
   "@brief Inspector field type for Shapes\n\n"
   "Editor use only.\n\n"
   "@internal"
);

void GuiInspectorTypeModelAssetPtr::consoleInit()
{
   Parent::consoleInit();

   ConsoleBaseType::getType(TypeModelAssetPtr)->setInspectorFieldType("GuiInspectorTypeModelAssetPtr");
}

GuiControl* GuiInspectorTypeModelAssetPtr::constructEditControl()
{
   // Create base filename edit controls
   GuiControl* retCtrl = Parent::constructEditControl();
   if (retCtrl == NULL)
      return retCtrl;

   // Change filespec
   char szBuffer[512];

   const char* previewImage;

   if (mInspector->getInspectObject() != nullptr)
   {
      dSprintf(szBuffer, sizeof(szBuffer), "AssetBrowser.showDialog(\"ShapeAsset\", \"AssetBrowser.changeAsset\", %s, %s);",
         mInspector->getIdString(), mCaption);
      mBrowseButton->setField("Command", szBuffer);

      setDataField(StringTable->insert("targetObject"), NULL, mInspector->getInspectObject()->getIdString());

      previewImage = mInspector->getInspectObject()->getDataField(mCaption, NULL);
   }
   else
   {
      //if we don't have a target object, we'll be manipulating the desination value directly
      dSprintf(szBuffer, sizeof(szBuffer), "AssetBrowser.showDialog(\"ShapeAsset\", \"AssetBrowser.changeAsset\", %s, \"%s\");",
         mInspector->getIdString(), mVariableName);
      mBrowseButton->setField("Command", szBuffer);

      previewImage = Con::getVariable(mVariableName);
   }

   mLabel = new GuiTextCtrl();
   mLabel->registerObject();
   mLabel->setControlProfile(mProfile);
   mLabel->setText(mCaption);
   addObject(mLabel);

   //
   GuiTextEditCtrl* editTextCtrl = static_cast<GuiTextEditCtrl*>(retCtrl);
   GuiControlProfile* toolEditProfile;
   if (Sim::findObject("ToolsGuiTextEditProfile", toolEditProfile))
      editTextCtrl->setControlProfile(toolEditProfile);

   GuiControlProfile* toolDefaultProfile = nullptr;
   Sim::findObject("ToolsGuiDefaultProfile", toolDefaultProfile);

   //
   mPreviewImage = new GuiBitmapCtrl();
   mPreviewImage->registerObject();

   if (toolDefaultProfile)
      mPreviewImage->setControlProfile(toolDefaultProfile);

   updatePreviewImage();

   addObject(mPreviewImage);

   //
   mPreviewBorderButton = new GuiBitmapButtonCtrl();
   mPreviewBorderButton->registerObject();

   if (toolDefaultProfile)
      mPreviewBorderButton->setControlProfile(toolDefaultProfile);

   mPreviewBorderButton->_setBitmap(StringTable->insert("ToolsModule:cubemapBtnBorder_n_image"));

   mPreviewBorderButton->setField("Command", szBuffer); //clicking the preview does the same thing as the edit button, for simplicity
   addObject(mPreviewBorderButton);

   //
   // Create "Open in Editor" button
   mEditButton = new GuiBitmapButtonCtrl();

   dSprintf(szBuffer, sizeof(szBuffer), "ShapeEditorPlugin.openShapeAssetId(%d.getText());", retCtrl->getId());
   mEditButton->setField("Command", szBuffer);

   mEditButton->setText("Edit");
   mEditButton->setSizing(horizResizeLeft, vertResizeAspectTop);

   mEditButton->setDataField(StringTable->insert("Profile"), NULL, "ToolsGuiButtonProfile");
   mEditButton->setDataField(StringTable->insert("tooltipprofile"), NULL, "GuiToolTipProfile");
   mEditButton->setDataField(StringTable->insert("hovertime"), NULL, "1000");
   mEditButton->setDataField(StringTable->insert("tooltip"), NULL, "Open this asset in the Shape Editor");

   mEditButton->registerObject();
   addObject(mEditButton);

   //
   mUseHeightOverride = true;
   mHeightOverride = 72;

   return retCtrl;
}

bool GuiInspectorTypeModelAssetPtr::updateRects()
{
   S32 rowSize = 18;
   S32 dividerPos, dividerMargin;
   mInspector->getDivider(dividerPos, dividerMargin);
   Point2I fieldExtent = getExtent();
   Point2I fieldPos = getPosition();

   mEditCtrlRect.set(0, 0, fieldExtent.x, fieldExtent.y);
   mLabel->resize(Point2I(mProfile->mTextOffset.x, 0), Point2I(fieldExtent.x, rowSize));

   RectI previewRect = RectI(Point2I(mProfile->mTextOffset.x, rowSize), Point2I(50, 50));
   mPreviewBorderButton->resize(previewRect.point, previewRect.extent);
   mPreviewImage->resize(previewRect.point, previewRect.extent);

   S32 editPos = previewRect.point.x + previewRect.extent.x + 10;
   mEdit->resize(Point2I(editPos, rowSize * 1.5), Point2I(fieldExtent.x - editPos - 5, rowSize));

   mEditButton->resize(Point2I(fieldExtent.x - 105, previewRect.point.y + previewRect.extent.y - rowSize), Point2I(100, rowSize));

   mBrowseButton->setHidden(true);

   return true;
}

void GuiInspectorTypeModelAssetPtr::updateValue()
{
   Parent::updateValue();

   updatePreviewImage();
}

void GuiInspectorTypeModelAssetPtr::updatePreviewImage()
{
   const char* previewImage;
   if (mInspector->getInspectObject() != nullptr)
      previewImage = mInspector->getInspectObject()->getDataField(mCaption, NULL);
   else
      previewImage = Con::getVariable(mVariableName);

   //if what we're working with isn't even a valid asset, don't present like we found a good one
   if (!AssetDatabase.isDeclaredAsset(previewImage))
   {
      mPreviewImage->_setBitmap(StringTable->EmptyString());
      return;
   }

   String shpPreviewAssetId = String(previewImage) + "_PreviewImage";
   shpPreviewAssetId.replace(":", "_");
   shpPreviewAssetId = "ToolsModule:" + shpPreviewAssetId;
   if (AssetDatabase.isDeclaredAsset(shpPreviewAssetId.c_str()))
   {
      mPreviewImage->setBitmap(StringTable->insert(shpPreviewAssetId.c_str()));
   }

   if (mPreviewImage->getBitmapAsset().isNull())
      mPreviewImage->_setBitmap(StringTable->insert("ToolsModule:genericAssetIcon_image"));
}

void GuiInspectorTypeModelAssetPtr::setPreviewImage(StringTableEntry assetId)
{
   //if what we're working with isn't even a valid asset, don't present like we found a good one
   if (!AssetDatabase.isDeclaredAsset(assetId))
   {
      mPreviewImage->_setBitmap(StringTable->EmptyString());
      return;
   }

   String shpPreviewAssetId = String(assetId) + "_PreviewImage";
   shpPreviewAssetId.replace(":", "_");
   shpPreviewAssetId = "ToolsModule:" + shpPreviewAssetId;
   if (AssetDatabase.isDeclaredAsset(shpPreviewAssetId.c_str()))
   {
      mPreviewImage->setBitmap(StringTable->insert(shpPreviewAssetId.c_str()));
   }

   if (mPreviewImage->getBitmapAsset().isNull())
      mPreviewImage->_setBitmap(StringTable->insert("ToolsModule:genericAssetIcon_image"));
}

#endif
