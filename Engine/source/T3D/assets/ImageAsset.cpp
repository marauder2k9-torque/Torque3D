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

#ifndef IMAGE_ASSET_H
#include "ImageAsset.h"
#endif

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

#include "gfx/gfxStringEnumTranslate.h"

#include "ImageAssetInspectors.h"

// Debug Profiling.
#include "platform/profiler.h"

#include "T3D/assets/assetImporter.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/bitmap/ddsFile.h"
#ifdef __clang__
#define STBIWDEF static inline
#endif
#pragma warning( push )
#pragma warning( disable : 4505 ) // unreferenced function removed.
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"
#endif
#pragma warning(pop)

//-----------------------------------------------------------------------------

StringTableEntry ImageAsset::smNoImageAssetFallback = NULL;
AssetPtr<ImageAsset> ImageAsset::smNoImageAssetFallbackAssetPtr = NULL;

StringTableEntry ImageAsset::smNamedTargetAssetFallback = NULL;
AssetPtr<ImageAsset> ImageAsset::smNamedTargetAssetFallbackAssetPtr = NULL;

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(ImageAsset);


//-----------------------------------------------------------------------------
// REFACTOR
//-----------------------------------------------------------------------------

IMPLEMENT_STRUCT(AssetPtr<ImageAsset>, AssetPtrImageAsset,, "")
END_IMPLEMENT_STRUCT

ConsoleType(ImageAssetPtr, TypeImageAssetPtr, AssetPtr<ImageAsset>, ASSET_ID_FIELD_PREFIX)


ConsoleGetType(TypeImageAssetPtr)
{
   // Fetch asset Id.
   return (*((AssetPtr<ImageAsset>*)dptr)).getAssetId();
}

ConsoleSetType(TypeImageAssetPtr)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset pointer.
      AssetPtr<ImageAsset>* pAssetPtr = dynamic_cast<AssetPtr<ImageAsset>*>((AssetPtrBase*)(dptr));

      // Is the asset pointer the correct type?
      if (pAssetPtr == NULL)
      {
         Con::warnf("(TypeImageAssetPtr) - Failed to set asset Id '%d'.", pFieldValue);
         return;
      }

      // Set asset.
      pAssetPtr->setAssetId(pFieldValue);

      return;
   }

   // Warn.
   Con::warnf("(TypeImageAssetPtr) - Cannot set multiple args to a single asset.");
}

//-----------------------------------------------------------------------------

IMPLEMENT_STRUCT(AssetRef<ImageAsset>, AssetRefImageAsset, , "")
END_IMPLEMENT_STRUCT

ConsoleType(ImageAssetRef, TypeImageAssetRef, AssetRef<ImageAsset>, ASSET_ID_FIELD_PREFIX)


ConsoleGetType(TypeImageAssetRef)
{
   AssetRef<ImageAsset>& ref = *((AssetRef<ImageAsset>*)dptr);

   if (ref.assetPtr.isNull())
      return ref.assetId;
   else
   {
      if ((ref.assetId[0] == '$' || ref.assetId[0] == '#'))
         return ref.assetId;

      return ref.assetPtr.getAssetId();
   }
}

AssetPtr<ImageAsset> ImageAsset::getNamedTargetAssetPtr(StringTableEntry filePath)
{
   // Do a lookup to see if we can find a hit on this path/id
   // If not, then we'll register it as a private asset and keep going
   // as if we're in this function, we're almost certainly dealing with
   // a named target anyways and require the special case.
   StringTableEntry imageAssetId = getAssetIdByFilename(filePath);
   if (imageAssetId == smNoImageAssetFallback)
   {
      ImageAsset* privateImage = new ImageAsset();
      privateImage->setImageFile(filePath);
      imageAssetId = AssetDatabase.addPrivateAsset(privateImage);
   }

   AssetPtr<ImageAsset> assetPtr;
   assetPtr = imageAssetId;
   return assetPtr;
}

ConsoleSetType(TypeImageAssetRef)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset pointer.
      AssetRef<ImageAsset>* pAssetRef = (AssetRef<ImageAsset>*)(dptr);

      // Is the asset pointer the correct type?
      if (pAssetRef == NULL)
      {
         Con::warnf("(TypeImageAssetRef) - Failed to set asset Id '%d'.", pFieldValue);
         return;
      }

      StringTableEntry _in = StringTable->insert(pFieldValue);

      if (ImageAsset::isNamedTarget(_in))
      {
         pAssetRef->assetId = _in;
         pAssetRef->assetPtr = ImageAsset::getNamedTargetAssetPtr(_in);
         return;
      }

      // Set asset.
      *pAssetRef = _in;

      return;
   }

   // Warn.
   Con::warnf("(TypeImageAssetRef) - Cannot set multiple args to a single asset.");
}

//-----------------------------------------------------------------------------
// REFACTOR END
//-----------------------------------------------------------------------------

ImplementEnumType(ImageAssetType,
   "Type of image data this asset describes.\n"
   "@ingroup gameObjects")
{ ImageAsset::Albedo,      "Albedo",      "" },
{ ImageAsset::Normal,      "Normal",      "" },
{ ImageAsset::ORMConfig,   "ORMConfig",   "" },
{ ImageAsset::GUI,         "GUI",         "" },
{ ImageAsset::Roughness,   "Roughness",   "" },
{ ImageAsset::AO,          "AO",          "" },
{ ImageAsset::Metalness,   "Metalness",   "" },
{ ImageAsset::Glow,        "Glow",        "" },
{ ImageAsset::Particle,    "Particle",    "" },
{ ImageAsset::Decal,       "Decal",       "" },
{ ImageAsset::Cubemap,     "Cubemap",       "" },

EndImplementEnumType;

const String ImageAsset::mErrCodeStrings[] =
{
   "TooManyMips",
   "UnKnown"
};
//-----------------------------------------------------------------------------


ImageAsset::ImageAsset() :
   mImageFile(StringTable->EmptyString()),
   mUseMips(true),
   mIsHDRImage(false),
   mImageType(Albedo),
   mIsNamedTarget(false),
   mImageWidth(-1),
   mImageHeight(-1),
   mImageDepth(-1),
   mImageChannels(-1),
   mFrameWidth(0),
   mFrameHeight(0)
{
   VECTOR_SET_ASSOCIATION(mFrames);
   VECTOR_SET_ASSOCIATION(mExplicitCells);
   mLoadedState = AssetErrCode::NotLoaded;
}

//-----------------------------------------------------------------------------

ImageAsset::~ImageAsset()
{
   ImageTextureMap::iterator resIter = mResourceMap.begin();
   for (; resIter != mResourceMap.end(); ++resIter)
   {
      resIter->value.free();
   }

   mResourceMap.clear();
}


void ImageAsset::consoleInit()
{
   Parent::consoleInit();
   Con::addVariable("$Core::NoImageAssetFallback", TypeString, &smNoImageAssetFallback,
      "The assetId of the texture to display when the requested image asset is missing.\n"
      "@ingroup GFX\n");

   Con::addVariable("$Core::NamedTargetFallback", TypeString, &smNamedTargetAssetFallback,
      "The assetId of the texture to display when the requested image asset is named target.\n"
      "@ingroup GFX\n");

   smNoImageAssetFallback = StringTable->insert(Con::getVariable("$Core::NoImageAssetFallback"));
   smNamedTargetAssetFallback = StringTable->insert(Con::getVariable("$Core::NamedTargetFallback"));
}

//-----------------------------------------------------------------------------

void ImageAsset::initPersistFields()
{
   docsURL;
   // Call parent.
   Parent::initPersistFields();

   addProtectedField("imageFile", TypeAssetLooseFilePath, Offset(mImageFile, ImageAsset), &setImageFile, &getImageFile, &writeImageFile, "Path to the image file.");

   addProtectedField("useMips", TypeBool, Offset(mUseMips, ImageAsset), &setGenMips, &defaultProtectedGetFn, &writeGenMips, "Generate mip maps?");
   addProtectedField("isHDRImage", TypeBool, Offset(mIsHDRImage, ImageAsset), &setTextureHDR, &defaultProtectedGetFn, &writeTextureHDR, "HDR Image?");

   addField("imageType", TypeImageAssetType, Offset(mImageType, ImageAsset), "What the main use-case for the image is for.");

   addProtectedField("frameWidth", TypeS32, Offset(mFrameWidth, ImageAsset), &setFrameWidth, &defaultProtectedGetFn, &writeFrameSize,
      "Width, in pixels, of each implicit grid frame cell. 0 (the default) means no grid.");
   addProtectedField("frameHeight", TypeS32, Offset(mFrameHeight, ImageAsset), &setFrameHeight, &defaultProtectedGetFn, &writeFrameSize,
      "Height, in pixels, of each implicit grid frame cell. 0 (the default) means no grid.");
}
bool ImageAsset::onAdd()
{
   // Call Parent.
   if (!Parent::onAdd())
      return false;

   return true;
}

void ImageAsset::onRemove()
{
   Torque::FS::RemoveChangeNotification(mImageFile, this, &ImageAsset::_onResourceChanged);

   // Call Parent.
   Parent::onRemove();
}

U32 ImageAsset::getAssetByFilename(StringTableEntry fileName, AssetPtr<ImageAsset>* imageAsset)
{
   AssetQuery query;
   S32 foundAssetcount = AssetDatabase.findAssetLooseFile(&query, fileName);
   if (foundAssetcount == 0)
   {
      //Didn't work, so have us fall back to a placeholder asset
      imageAsset->setAssetId(ImageAsset::smNoImageAssetFallback);

      if (imageAsset->isNull())
      {
         //Well that's bad, loading the fallback failed.
         Con::warnf("ImageAsset::getAssetByFilename - Finding of asset associated with file %s failed with no fallback asset", fileName);
         return AssetErrCode::Failed;
      }

      //handle noshape not being loaded itself
      if ((*imageAsset)->mLoadedState == BadFileReference)
      {
         Con::warnf("ImageAsset::getAssetByFilename - Finding of associated with file %s failed, and fallback asset reported error of Bad File Reference.", fileName);
         return AssetErrCode::BadFileReference;
      }

      Con::warnf("ImageAsset::getAssetByFilename - Finding of associated with file %s failed, utilizing fallback asset", fileName);

      (*imageAsset)->mLoadedState = AssetErrCode::UsingFallback;
      return AssetErrCode::UsingFallback;
   }
   else
   {
      //acquire and bind the asset, and return it out
      imageAsset->setAssetId(query.mAssetList[0]);
      return (*imageAsset)->load();
   }
}

StringTableEntry ImageAsset::getAssetIdByFilename(StringTableEntry fileName)
{
   if (fileName == StringTable->EmptyString())
      return StringTable->EmptyString();

   StringTableEntry imageAssetId = ImageAsset::smNoImageAssetFallback;

   AssetQuery query;
   S32 foundAssetcount = AssetDatabase.findAssetLooseFile(&query, fileName);
   if (foundAssetcount != 0)
   {
      //acquire and bind the asset, and return it out
      imageAssetId = query.mAssetList[0];
   }
   else
   {
      foundAssetcount = AssetDatabase.findAssetType(&query, "ImageAsset");
      if (foundAssetcount != 0)
      {
         // loop all image assets and see if we can find one
         // using the same image file/named target.
         for (auto imgAsset : query.mAssetList)
         {
            AssetPtr<ImageAsset> temp = imgAsset;
            if (temp.notNull())
            {
               if (temp->getImageFile() == fileName)
               {
                  return imgAsset;
               }
               else
               {
                  Torque::Path temp1 = temp->getImageFile();
                  Torque::Path temp2 = fileName;

                  if (temp1.getPath() == temp2.getPath() && temp1.getFileName() == temp2.getFileName())
                  {
                     return imgAsset;
                  }
               }
               
            }
         }
      }
      else
      {
         AssetPtr<ImageAsset> imageAsset = imageAssetId; //ensures the fallback is loaded
      }
   }

   return imageAssetId;
}

StringTableEntry ImageAsset::getAssetIdFromFilePath(StringTableEntry filePath)
{
   if (filePath == StringTable->EmptyString())
      return filePath;

   // Already a valid asset id.
   if (AssetDatabase.isDeclaredAsset(filePath))
      return filePath;

   StringTableEntry assetId = getAssetIdByFilename(filePath);
   if (assetId == smNoImageAssetFallback)
   {
      ImageAsset* privateImage = new ImageAsset();
      privateImage->setImageFile(filePath);
      assetId = AssetDatabase.addPrivateAsset(privateImage);
   }

   return assetId;
}

U32 ImageAsset::getAssetById(StringTableEntry assetId, AssetPtr<ImageAsset>* imageAsset)
{
   (*imageAsset) = assetId;

   if (imageAsset->notNull())
   {
      return (*imageAsset)->load();
   }
   else
   {
      //Didn't work, so have us fall back to a placeholder asset
      imageAsset->setAssetId(ImageAsset::smNoImageAssetFallback);

      if (imageAsset->isNull())
      {
         //Well that's bad, loading the fallback failed.
         Con::errorf("ImageAsset::getAssetById - Finding of asset with id %s failed with no fallback asset", assetId);
         return AssetErrCode::Failed;
      }

      //handle fallback not being loaded itself
      if ((*imageAsset)->mLoadedState == BadFileReference)
      {
         Con::warnf("ImageAsset::getAssetById - Finding of asset with id %s failed, and fallback asset reported error of Bad File Reference.", assetId);
         return AssetErrCode::BadFileReference;
      }

      Con::warnf("ImageAsset::getAssetById - Finding of asset with id %s failed, utilizing fallback asset", assetId);

      (*imageAsset)->mLoadedState = AssetErrCode::UsingFallback;
      return AssetErrCode::UsingFallback;
   }
}

void ImageAsset::initializeAsset(void)
{
   // Call parent.
   Parent::initializeAsset();

   // Ensure the image-file is expanded.
   if (isNamedTarget())
      return;

   mImageFile = expandAssetFilePath(mImageFile);

   if (getOwned())
      Torque::FS::AddChangeNotification(mImageFile, this, &ImageAsset::_onResourceChanged);

   populateImage();
}

void ImageAsset::onAssetRefresh(void)
{
   // Ignore if not yet added to the sim.
   if (!isProperlyAdded())
      return;

   // Call parent.
   Parent::onAssetRefresh();

   populateImage();

}

//------------------------------------------------------------------------------

void ImageAsset::copyTo(SimObject* object)
{
   // Call to parent.
   Parent::copyTo(object);

   ImageAsset* pAsset = static_cast<ImageAsset*>(object);

   // Sanity!
   AssertFatal(pAsset != NULL, "ImageAsset::copyTo() - Object is not the correct type.");

   pAsset->setImageFile(getImageFile());
   pAsset->setGenMips(getGenMips());
   pAsset->setTextureHDR(getTextureHDR());
}

void ImageAsset::setImageFile(StringTableEntry pImageFile)
{
   // Sanity!
   AssertFatal(pImageFile != NULL, "Cannot use a NULL image file.");

   pImageFile = StringTable->insert(pImageFile);

   if (pImageFile == mImageFile)
      return;

   Torque::FS::RemoveChangeNotification(mImageFile, this, &ImageAsset::_onResourceChanged);

   if (String(pImageFile).startsWith("#") || String(pImageFile).startsWith("$"))
   {
      mImageFile = StringTable->insert(pImageFile);
      mIsNamedTarget = true;
      refreshAsset();
      return;
   }

   mImageFile = getOwned() ? expandAssetFilePath(pImageFile) : StringTable->insert(pImageFile);

   refreshAsset();
}

void ImageAsset::setGenMips(const bool pGenMips)
{
   if (pGenMips == mUseMips)
      return;

   mUseMips = pGenMips;

   refreshAsset();
}


void ImageAsset::setTextureHDR(const bool pIsHDR)
{
   if (pIsHDR == mIsHDRImage)
      return;

   mIsHDRImage = pIsHDR;

   refreshAsset();
}

U32 ImageAsset::load()
{
   if (mLoadedState == Ok)
      return mLoadedState;

   if (!Torque::FS::IsFile(mImageFile))
   {
      if (isNamedTarget())
      {
         mLoadedState = Ok;
         return mLoadedState;
      }

      if (mLoadedState != AssetErrCode::BadFileReference && mLoadedState != AssetErrCode::Failed)
         Con::errorf("ImageAsset::initializeAsset: Attempted to load file %s but it was not valid!", mImageFile);
      mLoadedState = BadFileReference;
      return mLoadedState;
   }
   else
   {
      mLoadedState = Ok;
   }

   return mLoadedState;
}

GFXTexHandle ImageAsset::getTexture(GFXTextureProfile* requestedProfile)
{
   if (mLoadedState == Ok && mResourceMap.contains(requestedProfile))
   {
      return mResourceMap.find(requestedProfile)->value;
   }
   else
   {
      //try a reload
      load();
   }

   if (isNamedTarget())
   {
      if (getNamedTarget().isValid())
      {
         GFXTexHandle tex = getNamedTarget()->getTexture();
         if (!tex.isNull())
         {
            mResourceMap.insert(requestedProfile, tex);
            return tex;
         }
      }

      if (smNamedTargetAssetFallbackAssetPtr.notNull())
         return smNamedTargetAssetFallbackAssetPtr->getTexture(requestedProfile);

      return NULL;
   }

   if (mLoadedState == Ok)
   {
         //If we don't have an existing map case to the requested format, we'll just create it and insert it in
         GFXTexHandle newTex;
         newTex.set(mImageFile, requestedProfile, avar("%s %s() - mTextureObject (line %d)", mImageFile, __FUNCTION__, __LINE__));
         if (newTex)
         {
            mResourceMap.insert(requestedProfile, newTex);
            return newTex;
         }
   }

   if (smNoImageAssetFallbackAssetPtr.notNull() && smNoImageAssetFallbackAssetPtr != this)
      return smNoImageAssetFallbackAssetPtr->getTexture(requestedProfile);

   return NULL;
}

const char* ImageAsset::getImageTypeNameFromType(ImageAsset::ImageTypes type)
{
   // must match ImageTypes order
   static const char* _names[] = {
      "Albedo",
      "Normal",
      "ORMConfig",
      "GUI",
      "Roughness",
      "AO",
      "Metalness",
      "Glow",
      "Particle",
      "Decal",
      "Cubemap"
   };

   if (type < 0 || type >= ImageTypeCount)
   {
      Con::errorf("ImageAsset::getAdapterNameFromType - Invalid ImageType, defaulting to Albedo");
      return _names[Albedo];
   }

   return _names[type];
}

ImageAsset::ImageTypes ImageAsset::getImageTypeFromName(StringTableEntry name)
{
   if (dStrIsEmpty(name))
   {
      return (ImageTypes)Albedo;
   }

   S32 ret = -1;
   for (S32 i = 0; i < ImageTypeCount; i++)
   {
      if (!dStricmp(getImageTypeNameFromType((ImageTypes)i), name))
         ret = i;
   }

   if (ret == -1)
   {
      Con::errorf("ImageAsset::getImageTypeFromName - Invalid ImageType name, defaulting to Albedo");
      ret = Albedo;
   }

   return (ImageTypes)ret;
}

void ImageAsset::_onResourceChanged(const Torque::Path& path)
{
   if (path != Torque::Path(mImageFile))
      return;

   refreshAsset();
}

void ImageAsset::onTamlPreWrite(void)
{
   // Call parent.
   Parent::onTamlPreWrite();

   if (isNamedTarget())
      return;

   // Ensure the image-file is collapsed.
   mImageFile = collapseAssetFilePath(mImageFile);
}

void ImageAsset::onTamlPostWrite(void)
{
   // Call parent.
   Parent::onTamlPostWrite();

   if (isNamedTarget())
      return;

   // Ensure the image-file is expanded.
   mImageFile = expandAssetFilePath(mImageFile);
}

void ImageAsset::onTamlCustomWrite(TamlCustomNodes& customNodes)
{
   // Debug Profiling.
   PROFILE_SCOPE(ImageAsset_OnTamlCustomWrite);

   // Call parent.
   Parent::onTamlCustomWrite(customNodes);

   TamlCustomNode* pImageMetaData = customNodes.addNode(StringTable->insert("ImageMetadata"));
   TamlCustomNode* pImageInfoNode = pImageMetaData->addNode(StringTable->insert("ImageInfo"));

   pImageInfoNode->addField(StringTable->insert("ImageWidth"), mImageWidth);
   pImageInfoNode->addField(StringTable->insert("ImageHeight"), mImageHeight);
   pImageInfoNode->addField(StringTable->insert("ImageDepth"), mImageDepth);

   if (!mExplicitCells.empty())
   {
      TamlCustomNode* pExplicitCellsNode = pImageMetaData->addNode(StringTable->insert("ExplicitCells"));
      for (U32 i = 0; i < mExplicitCells.size(); i++)
      {
         const ExplicitCellDef& def = mExplicitCells[i];
         TamlCustomNode* pCellNode = pExplicitCellsNode->addNode(StringTable->insert("Cell"));
         pCellNode->addField(StringTable->insert("Name"), def.name);
         pCellNode->addField(StringTable->insert("X"), def.x);
         pCellNode->addField(StringTable->insert("Y"), def.y);
         pCellNode->addField(StringTable->insert("W"), def.w);
         pCellNode->addField(StringTable->insert("H"), def.h);
      }
   }

}

void ImageAsset::onTamlCustomRead(const TamlCustomNodes& customNodes)
{
   // Debug Profiling.
   PROFILE_SCOPE(ImageAsset_OnTamlCustomRead);

   // Call parent.
   Parent::onTamlCustomRead(customNodes);

   const TamlCustomNode* pImageMetaDataNode = customNodes.findNode(StringTable->insert("ImageMetadata"));

   if (pImageMetaDataNode != NULL)
   {
      const TamlCustomNode* pImageInfoNode = pImageMetaDataNode->findNode(StringTable->insert("ImageInfo"));
      // Fetch fields.
      const TamlCustomFieldVector& fields = pImageInfoNode->getFields();
      // Iterate property fields.
      for (TamlCustomFieldVector::const_iterator fieldItr = fields.begin(); fieldItr != fields.end(); ++fieldItr)
      {
         // Fetch field.
         const TamlCustomField* pField = *fieldItr;
         // Fetch field name.
         StringTableEntry fieldName = pField->getFieldName();
         if (fieldName == StringTable->insert("ImageWidth"))
         {
            pField->getFieldValue(mImageWidth);
         }
         else if (fieldName == StringTable->insert("ImageHeight"))
         {
            pField->getFieldValue(mImageHeight);
         }
         else if (fieldName == StringTable->insert("ImageDepth"))
         {
            pField->getFieldValue(mImageDepth);
         }
         else
         {
            // Unknown name so warn.
            Con::warnf("ImageAsset::onTamlCustomRead() - Encountered an unknown custom field name of '%s'.", fieldName);
            continue;
         }
      }

      const TamlCustomNode* pExplicitCellsNode = pImageMetaDataNode->findNode(StringTable->insert("ExplicitCells"));
      if (pExplicitCellsNode != NULL)
      {
         mExplicitCells.clear();

         const StringTableEntry cellNodeName = StringTable->insert("Cell");
         const StringTableEntry nameFieldName = StringTable->insert("Name");
         const StringTableEntry xFieldName = StringTable->insert("X");
         const StringTableEntry yFieldName = StringTable->insert("Y");
         const StringTableEntry wFieldName = StringTable->insert("W");
         const StringTableEntry hFieldName = StringTable->insert("H");

         const TamlCustomNodeVector& cellNodes = pExplicitCellsNode->getChildren();
         for (TamlCustomNodeVector::const_iterator cellItr = cellNodes.begin(); cellItr != cellNodes.end(); ++cellItr)
         {
            const TamlCustomNode* pCellNode = *cellItr;
            if (pCellNode->getNodeName() != cellNodeName)
            {
               Con::warnf("ImageAsset::onTamlCustomRead() - Encountered an unexpected child node '%s' under ExplicitCells (expected 'Cell').", pCellNode->getNodeName());
               continue;
            }

            ExplicitCellDef def;
            def.x = 0;
            def.y = 0;
            def.w = 0;
            def.h = 0;
            def.name = StringTable->EmptyString();

            const TamlCustomFieldVector& cellFields = pCellNode->getFields();
            for (TamlCustomFieldVector::const_iterator fieldItr = cellFields.begin(); fieldItr != cellFields.end(); ++fieldItr)
            {
               const TamlCustomField* pField = *fieldItr;
               StringTableEntry fieldName = pField->getFieldName();

               if (fieldName == nameFieldName)
               {
                  def.name = StringTable->insert(pField->getFieldValue());
               }
               else if (fieldName == xFieldName)
               {
                  pField->getFieldValue(def.x);
               }
               else if (fieldName == yFieldName)
               {
                  pField->getFieldValue(def.y);
               }
               else if (fieldName == wFieldName)
               {
                  pField->getFieldValue(def.w);
               }
               else if (fieldName == hFieldName)
               {
                  pField->getFieldValue(def.h);
               }
               else
               {
                  Con::warnf("ImageAsset::onTamlCustomRead() - Encountered an unknown Cell field name of '%s'.", fieldName);
               }
            }

            if (def.name != StringTable->EmptyString())
               mExplicitCells.push_back(def);
            else
               Con::warnf("ImageAsset::onTamlCustomRead() - A Cell node under ExplicitCells was missing its Name field; skipped.");
         }

      }
   }
}

void ImageAsset::populateImage(void)
{
   if (Torque::FS::IsFile(mImageFile))
   {
      if (dStrEndsWith(mImageFile, ".dds"))
      {
         DDSFile* tempFile = new DDSFile();
         FileStream* ddsFs;
         if ((ddsFs = FileStream::createAndOpen(mImageFile, Torque::FS::File::Read)) == NULL)
         {
            Con::errorf("ImageAsset::setImageFile Failed to open ddsfile: %s", mImageFile);
         }

         if (!tempFile->readHeader(*ddsFs))
         {
            Con::errorf("ImageAsset::setImageFile Failed to read header of ddsfile: %s", mImageFile);
         }
         else
         {
            mImageWidth = tempFile->mWidth;
            mImageHeight = tempFile->mHeight;
         }

         delete tempFile;
         delete ddsFs;
      }
      else
      {
         if (!stbi_info(mImageFile, &mImageWidth, &mImageHeight, &mImageChannels))
         {
            StringTableEntry stbErr = stbi_failure_reason();
            if (stbErr == StringTable->EmptyString())
               stbErr = "ImageAsset::Unkown Error!";

            Con::errorf("ImageAsset::setImageFile STB Get file info failed: %s", stbErr);
         }
      }

      // we only support 2d textures..... for now ;)
      mImageDepth = 1; 
   }

   _rebuildFrames();
}

//-----------------------------------------------------------------------------

void ImageAsset::setFrameSize(S32 frameWidth, S32 frameHeight)
{
   if (frameWidth == mFrameWidth && frameHeight == mFrameHeight)
      return;

   // Negative sizes make no sense as a cell 
   mFrameWidth = getMax(0, frameWidth);
   mFrameHeight = getMax(0, frameHeight);

   _rebuildFrames();
}

//-----------------------------------------------------------------------------

void ImageAsset::addExplicitCell(S32 x, S32 y, S32 w, S32 h, StringTableEntry name)
{
   if (name == NULL || name == StringTable->EmptyString())
   {
      Con::warnf("ImageAsset::addExplicitCell() - a region name is required.");
      return;
   }

   name = StringTable->insert(name);

   // Replace an existing entry of the same name in place
   for (U32 i = 0; i < mExplicitCells.size(); i++)
   {
      if (mExplicitCells[i].name == name)
      {
         mExplicitCells[i].x = x;
         mExplicitCells[i].y = y;
         mExplicitCells[i].w = w;
         mExplicitCells[i].h = h;
         _rebuildFrames();
         return;
      }
   }

   ExplicitCellDef def;
   def.x = x;
   def.y = y;
   def.w = w;
   def.h = h;
   def.name = name;
   mExplicitCells.push_back(def);

   _rebuildFrames();
}

//-----------------------------------------------------------------------------

void ImageAsset::clearExplicitCells()
{
   if (mExplicitCells.empty())
      return;

   mExplicitCells.clear();
   _rebuildFrames();
}

//-----------------------------------------------------------------------------

const ImageAsset::Frame* ImageAsset::findFrameByName(StringTableEntry name) const
{
   if (name == NULL || name == StringTable->EmptyString())
      return NULL;

   name = StringTable->insert(name);

   // Walked in mFrames order
   for (U32 i = 0; i < mFrames.size(); i++)
   {
      if (mFrames[i].regionName == name)
         return &mFrames[i];
   }

   return NULL;
}

void ImageAsset::_rebuildFrames()
{
   mFrames.clear();

   if (mImageWidth <= 0 || mImageHeight <= 0)
      return;

   const F32 texelWidthScale = 1.0f / (F32)mImageWidth;
   const F32 texelHeightScale = 1.0f / (F32)mImageHeight;

   // --- Grid layout --- 
   if (mFrameWidth > 0 && mFrameHeight > 0)
   {
      const S32 cols = mImageWidth / mFrameWidth;
      const S32 rows = mImageHeight / mFrameHeight;

      for (S32 row = 0; row < rows; row++)
      {
         for (S32 col = 0; col < cols; col++)
         {
            mFrames.push_back(Frame(
               col * mFrameWidth, row * mFrameHeight,
               mFrameWidth, mFrameHeight,
               texelWidthScale, texelHeightScale));
         }
      }
   }
   else
   {
      mFrames.push_back(Frame(
         0, 0, (U32)mImageWidth, (U32)mImageHeight,
         texelWidthScale, texelHeightScale));
   }

   // --- Explicitly named cells
   for (U32 i = 0; i < mExplicitCells.size(); i++)
   {
      const ExplicitCellDef& def = mExplicitCells[i];
      mFrames.push_back(Frame(
         def.x, def.y, (U32)def.w, (U32)def.h,
         texelWidthScale, texelHeightScale,
         def.name));
   }
}

const char* ImageAsset::getImageInfo()
{
   if (isAssetValid())
   {
      static const U32 bufSize = 2048;
      char* returnBuffer = Con::getReturnBuffer(bufSize);

      GFXTexHandle newTex = TEXMGR->createTexture(mImageFile, &GFXStaticTextureSRGBProfile);
      if (newTex)
      {
         dSprintf(returnBuffer, bufSize, "%s %d %d %d", GFXStringTextureFormat[newTex->getFormat()], newTex->getHeight(), newTex->getWidth(), newTex->getDepth());
         newTex = NULL;
      }
      else
      {
         dSprintf(returnBuffer, bufSize, "ImageAsset::getImageInfo() - Failed to get image info for %s", getAssetId());
      }

      return returnBuffer;
   }

   return "";
}

DefineEngineMethod(ImageAsset, getImagePath, const char*, (), ,
   "Gets the image filepath of this asset.\n"
   "@return File path of the image file.")
{
   return object->getImageFile();
}

DefineEngineMethod(ImageAsset, getImageInfo, const char*, (), ,
   "Gets the info and properties of the image.\n"
   "@return The info/properties of the image.")
{
   return object->getImageInfo();
}

DefineEngineMethod(ImageAsset, isNamedTarget, bool, (), ,
   "Gets whether this image is a named target.\n"
   "@return bool for isNamedTarget.")
{
   return object->isNamedTarget();
}

DefineEngineFunction(loadImageAssetFallback, S32, (), ,
   "Forces the loading of the ImageAsset fallback asset.\n"
   "@return Load status code.")
{
   if (ImageAsset::smNoImageAssetFallbackAssetPtr.isNull())
   {
      ImageAsset::smNoImageAssetFallbackAssetPtr = ImageAsset::smNoImageAssetFallback;
      if (ImageAsset::smNoImageAssetFallbackAssetPtr.isNull())
         Con::errorf("loadImageAssetFallback could not find fallback asset %s!", ImageAsset::smNoImageAssetFallback);
      else
         return (S32)ImageAsset::smNoImageAssetFallbackAssetPtr->load();
   }
   else
   {
      return (S32)ImageAsset::smNoImageAssetFallbackAssetPtr->getStatus();
   }

   return AssetBase::Failed;
}

DefineEngineFunction(loadNamedTargetFallback, S32, (), ,
   "Forces the loading of the ImageAsset Named Target fallback asset.\n"
   "@return Load status code.")
{
   if (ImageAsset::smNamedTargetAssetFallbackAssetPtr.isNull())
   {
      ImageAsset::smNamedTargetAssetFallbackAssetPtr = ImageAsset::smNamedTargetAssetFallback;
      if (ImageAsset::smNamedTargetAssetFallbackAssetPtr.isNull())
         Con::errorf("loadNamedTargetFallback could not find fallback asset %s!", ImageAsset::smNamedTargetAssetFallback);
      else
         return (S32)ImageAsset::smNamedTargetAssetFallbackAssetPtr->load();
   }
   else
   {
      return (S32)ImageAsset::smNamedTargetAssetFallbackAssetPtr->getStatus();
   }

   return AssetBase::Failed;
}

#ifdef TORQUE_TOOLS
DefineEngineStaticMethod(ImageAsset, getAssetIdByFilename, const char*, (const char* filePath), (""),
   "Queries the Asset Database to see if any asset exists that is associated with the provided file path.\n"
   "@return The AssetId of the associated asset, if any.")
{
   return ImageAsset::getAssetIdByFilename(StringTable->insert(filePath));
}

//-----------------------------------------------------------------------------
// GuiInspectorTypeAssetId
//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiInspectorTypeImageAssetPtr);

ConsoleDocClass(GuiInspectorTypeImageAssetPtr,
   "@brief Inspector field type for Shapes\n\n"
   "Editor use only.\n\n"
   "@internal"
);

void GuiInspectorTypeImageAssetPtr::consoleInit()
{
   Parent::consoleInit();

   ConsoleBaseType::getType(TypeImageAssetPtr)->setInspectorFieldType("GuiInspectorTypeImageAssetPtr");
}

GuiControl* GuiInspectorTypeImageAssetPtr::constructEditControl()
{
   // Create base filename edit controls
   GuiControl* retCtrl = Parent::constructEditControl();
   if (retCtrl == NULL)
      return retCtrl;

   retCtrl->getRenderTooltipDelegate().bind(this, &GuiInspectorTypeImageAssetPtr::renderTooltip);

   // Change filespec
   char szBuffer[512];

   const char* previewImage;

   if (mInspector->getInspectObject() != NULL)
   {
      dSprintf(szBuffer, sizeof(szBuffer), "AssetBrowser.showDialog(\"ImageAsset\", \"AssetBrowser.changeAsset\", %s);",
         getIdString());
      mBrowseButton->setField("Command", szBuffer);

      setDataField(StringTable->insert("targetObject"), NULL, mInspector->getInspectObject()->getIdString());

      previewImage = getData();
   }
   else
   {
      //if we don't have a target object, we'll be manipulating the desination value directly
      dSprintf(szBuffer, sizeof(szBuffer), "AssetBrowser.showDialog(\"ImageAsset\", \"AssetBrowser.changeAsset\", %s, \"%s\");",
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

   editTextCtrl->setPlaceholderText("(None)");

   GuiControlProfile* toolDefaultProfile = NULL;
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

   if (mInspector->getInspectObject() != NULL)
      dSprintf(szBuffer, sizeof(szBuffer), "%d.apply(\"\");", getId());
   else
      dSprintf(szBuffer, sizeof(szBuffer), "%s = \"\";", mVariableName);

   mEditButton->setField("Command", szBuffer);

   mEditButton->setBitmap(StringTable->insert("ToolsModule:delete_n_image"));
   mEditButton->setSizing(horizResizeRight, vertResizeAspectBottom);

   mEditButton->setDataField(StringTable->insert("Profile"), NULL, "ToolsGuiButtonProfile");
   mEditButton->setDataField(StringTable->insert("tooltipprofile"), NULL, "GuiToolTipProfile");
   mEditButton->setDataField(StringTable->insert("hovertime"), NULL, "1000");
   mEditButton->setDataField(StringTable->insert("tooltip"), NULL, "Clear this ImageAsset");

   mEditButton->registerObject();
   addObject(mEditButton);

   //
   mUseHeightOverride = true;
   mHeightOverride = 72;

   return retCtrl;
}

bool GuiInspectorTypeImageAssetPtr::updateRects()
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
   mEdit->resize(Point2I(editPos, rowSize * 1.5), Point2I(fieldExtent.x - editPos - 5 - rowSize, rowSize));

   mEditButton->resize(Point2I(mEdit->getPosition().x + mEdit->getExtent().x, mEdit->getPosition().y), Point2I(rowSize, rowSize));

   mBrowseButton->setHidden(true);

   return true;
}

bool GuiInspectorTypeImageAssetPtr::renderTooltip(const Point2I& hoverPos, const Point2I& cursorPos, const char* tipText)
{
   if (!mAwake)
      return false;

   GuiCanvas* root = getRoot();
   if (!root)
      return false;

   AssetPtr<ImageAsset> imgAsset;
   U32 assetState = ImageAsset::getAssetById(getData(), &imgAsset);
   if (imgAsset == NULL || assetState == ImageAsset::Failed)
      return false;

   StringTableEntry filename = imgAsset->getImageFile();
   if (!filename || !filename[0])
      return false;

   StringTableEntry previewFilename = filename;
   if (Con::isFunction("getAssetPreviewImage"))
   {
      ConsoleValue consoleRet = Con::executef("getAssetPreviewImage", filename);
      previewFilename = StringTable->insert(consoleRet.getString());

      if (AssetDatabase.isDeclaredAsset(previewFilename))
      {
         ImageAsset* previewAsset = AssetDatabase.acquireAsset<ImageAsset>(previewFilename);
         previewFilename = previewAsset->getImageFile();
      }
   }

   GFXTexHandle texture(previewFilename, &GFXStaticTextureSRGBProfile, avar("%s() - tooltip texture (line %d)", __FUNCTION__, __LINE__));
   if (texture.isNull())
      return false;

   // Render image at a reasonable screen size while
   // keeping its aspect ratio...
   Point2I screensize = getRoot()->getWindowSize();
   Point2I offset = hoverPos;
   Point2I tipBounds;

   U32 texWidth = texture.getWidth();
   U32 texHeight = texture.getHeight();
   F32 aspect = (F32)texHeight / (F32)texWidth;

   const F32 newWidth = 150.0f;
   F32 newHeight = aspect * newWidth;

   // Offset below cursor image
   offset.y += 20; // TODO: Attempt to fix?: root->getCursorExtent().y;
   tipBounds.x = newWidth;
   tipBounds.y = newHeight;

   // Make sure all of the tooltip will be rendered width the app window,
   // 5 is given as a buffer against the edge
   if (screensize.x < offset.x + tipBounds.x + 5)
      offset.x = screensize.x - tipBounds.x - 5;
   if (screensize.y < offset.y + tipBounds.y + 5)
      offset.y = hoverPos.y - tipBounds.y - 5;

   RectI oldClip = GFX->getClipRect();
   RectI rect(offset, tipBounds);
   GFX->setClipRect(rect);

   GFXDrawUtil* drawer = GFX->getDrawUtil();
   drawer->clearBitmapModulation();
   GFX->getDrawUtil()->drawBitmapStretch(texture, rect);

   GFX->setClipRect(oldClip);

   return true;
}

void GuiInspectorTypeImageAssetPtr::updateValue()
{
   Parent::updateValue();

   updatePreviewImage();
}

void GuiInspectorTypeImageAssetPtr::updatePreviewImage()
{
   const char* previewImage;
   if (mInspector->getInspectObject() != NULL)
      previewImage = getData();
   else
      previewImage = Con::getVariable(mVariableName);

   //if what we're working with isn't even a valid asset, don't present like we found a good one
   if (!AssetDatabase.isDeclaredAsset(previewImage))
   {
      mPreviewImage->_setBitmap(StringTable->insert("ToolsModule:unknownImage_image"));
      return;
   }

   String imgPreviewAssetId = String(previewImage) + "_PreviewImage";
   imgPreviewAssetId.replace(":", "_");
   imgPreviewAssetId = "ToolsModule:" + imgPreviewAssetId;
   if (AssetDatabase.isDeclaredAsset(imgPreviewAssetId.c_str()))
   {
      mPreviewImage->setBitmap(StringTable->insert(imgPreviewAssetId.c_str()));
   }
   else
   {
      if (AssetDatabase.isDeclaredAsset(previewImage))
      {
         mPreviewImage->_setBitmap(previewImage);
      }
   }

   if (mPreviewImage->getBitmapAsset().isNull())
      mPreviewImage->_setBitmap(StringTable->insert("ToolsModule:genericAssetIcon_image"));
}

void GuiInspectorTypeImageAssetPtr::setPreviewImage(StringTableEntry assetId)
{
   //if what we're working with isn't even a valid asset, don't present like we found a good one
   if (!AssetDatabase.isDeclaredAsset(assetId))
   {
      mPreviewImage->_setBitmap(StringTable->insert("ToolsModule:unknownImage_image"));
      return;
   }

   String imgPreviewAssetId = String(assetId) + "_PreviewImage";
   imgPreviewAssetId.replace(":", "_");
   imgPreviewAssetId = "ToolsModule:" + imgPreviewAssetId;
   if (AssetDatabase.isDeclaredAsset(imgPreviewAssetId.c_str()))
   {
      mPreviewImage->setBitmap(StringTable->insert(imgPreviewAssetId.c_str()));
   }
   else
   {
      if (AssetDatabase.isDeclaredAsset(assetId))
      {
         mPreviewImage->_setBitmap(assetId);
      }
   }

   if (mPreviewImage->getBitmapAsset().isNull())
      mPreviewImage->_setBitmap(StringTable->insert("ToolsModule:genericAssetIcon_image"));
}

void GuiInspectorTypeImageAssetPtr::setCaption(StringTableEntry caption)
{
   mCaption = caption;
   mLabel->setText(mCaption);
}

DefineEngineMethod(GuiInspectorTypeImageAssetPtr, setCaption, void, (String newCaption), , "() - Sets the caption of the field.")
{
   object->setCaption(StringTable->insert(newCaption.c_str()));
}

DefineEngineMethod(GuiInspectorTypeImageAssetPtr, setIsDeleteBtnVisible, void, (bool isVisible), (false), "() - Sets if the delete/clear button is visible for the field")
{
   object->setIsDeleteBtnVisible(isVisible);
}

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiInspectorTypeImageAssetRef);

ConsoleDocClass(GuiInspectorTypeImageAssetRef,
   "@brief Inspector field type for AssetRef<ImageAsset> fields\n\n"
   "Editor use only.\n\n"
   "@internal"
);

void GuiInspectorTypeImageAssetRef::consoleInit()
{
   Parent::consoleInit();

   ConsoleBaseType::getType(TypeImageAssetRef)->setInspectorFieldType("GuiInspectorTypeImageAssetRef");
}
#endif
