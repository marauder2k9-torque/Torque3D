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

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "assets/assetManager.h"
#include "assets/assetPtr.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTextureManager.h"
#include "gfx/bitmap/gBitmap.h"
#include "core/util/str.h"
#include "core/volume.h"

#include "platform/profiler.h"

#include "T3D/assets/ImageAsset.h"
#include "T3D/assets/ImageAssetInspectors.h"

//-----------------------------------------------------------------------------

StringTableEntry ImageAsset::smNoImageAssetFallback = NULL;

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(ImageAsset);

ConsoleType(ImageAssetPtr, TypeImageAssetPtr, AssetPtr<ImageAsset>, ASSET_ID_FIELD_PREFIX)
ImplementConsoleTypeCasters(TypeImageAssetPtr, AssetPtr<ImageAsset>)

//-----------------------------------------------------------------------------

ConsoleGetType(TypeImageAssetPtr)
{
   return(*((AssetPtr<ImageAsset>*)dptr)).getAssetId();
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
         // No, so fail.
         Con::warnf("(TypeImageAssetPtr) - Failed to set asset Id '%d'.", pFieldValue);
         return;
      }

      // Set asset.
      pAssetPtr->setAssetId(pFieldValue);

      return;
   }

   // Warn.
   Con::warnf("(TypeTextureAssetPtr) - Cannot set multiple args to a single asset.");
}

//-----------------------------------------------------------------------------

ImplementEnumType(ImageAssetType,
   "Type of mesh data available in a shape.\n"
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
{ ImageAsset::Cubemap,     "Cubemap",     "" },

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
   mTextureHandle(NULL)
{
   mLoadedState = AssetErrCode::NotLoaded;
}

//-----------------------------------------------------------------------------

ImageAsset::~ImageAsset()
{
   SAFE_DELETE(mTextureHandle);
}


void ImageAsset::consoleInit()
{
   Parent::consoleInit();
   Con::addVariable("$Core::NoImageAssetFallback", TypeString, &smNoImageAssetFallback,
      "The assetId of the texture to display when the requested image asset is missing.\n"
      "@ingroup GFX\n");

   smNoImageAssetFallback = StringTable->insert(Con::getVariable("$Core::NoImageAssetFallback"));
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
   // Call Parent.
   Parent::onRemove();
}

void ImageAsset::initializeAsset(void)
{
   // Call parent.
   Parent::initializeAsset();

   // Ensure the image-file is expanded.
   mImageFile = expandAssetFilePath(mImageFile);
}

void ImageAsset::onAssetRefresh(void)
{
   // Ignore if not yet added to the sim.
   if (!isProperlyAdded())
      return;

   // Call parent.
   Parent::onAssetRefresh();

   mLoadedState = NotLoaded;
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

   // if we previously loaded, remove the listener for the file.
   if (mLoadedState == Ok)
      Torque::FS::RemoveChangeNotification(mImageFile, this, &ImageAsset::_onFileChanged);

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
      return;

   if (!Torque::FS::IsFile(mImageFile))
   {
      Con::errorf("ImageAsset::initializeAsset: Attempted to load file %s but it was not valid!", mImageFile);
      mLoadedState = BadFileReference;
      return mLoadedState;
   }
   else
   {
      Torque::FS::AddChangeNotification(mImageFile, this, &ImageAsset::_onFileChanged);
   }

   generateTexture();

   return mLoadedState;
}


void ImageAsset::_onResourceChanged(const Torque::Path& path)
{
   if (path != Torque::Path(mImageFile))
      return;

   refreshAsset();
}

GFXTexHandle ImageAsset::getTexture(GFXTextureProfile* requestedProfile)
{
   load();

   if (mLoadedState == Ok)
   {
      if (mResourceMap.contains(requestedProfile))
      {
         return mResourceMap.find(requestedProfile)->value;
      }
      else
      {
         //If we don't have an existing map case to the requested format, we'll just create it and insert it in
         GFXTexHandle newTex = TEXMGR->createTexture(mImageFile, requestedProfile);
         if (newTex)
         {
            mResourceMap.insert(requestedProfile, newTex);
            return newTex;
         }
         else
         {
            // return successfully generated texture instead.
            return mTextureHandle;
         }
      }
   }

   return nullptr;
}

void ImageAsset::generateTexture(void)
{
   StringBuilder str;
   str.append("GFX");
   // implement some defaults, eventually SRGB should be optional.
   U32 flags = GFXTextureProfile::Static | GFXTextureProfile::SRGB;

   str.append("StaticSRGB");

   // dont want mips?
   if (!mUseMips)
   {
      flags |= GFXTextureProfile::NoMipmap;
      str.append("NOMIP");
   }

   GFXTextureProfile::Types type = GFXTextureProfile::Types::DiffuseMap;

   if (mImageType == ImageTypes::Normal) {
      str.append("NORMAL");
      type = GFXTextureProfile::Types::NormalMap;
   }
   else
   {
      str.append("DIFFUSE");
   }

   GFXTextureProfile* tempProfile = new GFXTextureProfile(str.end(), type, flags);

   mTextureHandle = TEXMGR->createTexture(mImageFile, tempProfile);

   if (mTextureHandle.isValid())
      mLoadedState = AssetErrCode::Ok;
   else
      mLoadedState = AssetErrCode::Failed;

   ResourceManager::get().getChangedSignal().notify(this, &ImageAsset::_onResourceChanged);
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

void ImageAsset::_onFileChanged(const Torque::Path& path)
{
   if (path != Torque::Path(mImageFile))
      return;

   refreshAsset();
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

   // Ensure the image-file is collapsed.
   mImageFile = collapseAssetFilePath(mImageFile);
}

void ImageAsset::onTamlPostWrite(void)
{
   // Call parent.
   Parent::onTamlPostWrite();

   // Ensure the image-file is expanded.
   mImageFile = expandAssetFilePath(mImageFile);
}


//-----------------------------------------------------------------------------
// GuiInspectorTypeAssetId
//-----------------------------------------------------------------------------
/*
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

   if (mInspector->getInspectObject() != nullptr)
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
   mEdit->resize(Point2I(editPos, rowSize * 1.5), Point2I(fieldExtent.x - editPos - 5, rowSize));

   //mEditButton->resize(Point2I(fieldExtent.x - 105, previewRect.point.y + previewRect.extent.y - rowSize), Point2I(100, rowSize));

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

   StringTableEntry filename = imgAsset->getImagePath();
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
         previewFilename = previewAsset->getImagePath();
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
   if (mInspector->getInspectObject() != nullptr)
      previewImage = getData();
   else
      previewImage = Con::getVariable(mVariableName);

   //if what we're working with isn't even a valid asset, don't present like we found a good one
   if (!AssetDatabase.isDeclaredAsset(previewImage))
   {
      mPreviewImage->_setBitmap(StringTable->EmptyString());
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
         ImageAsset* imgAsset = AssetDatabase.acquireAsset<ImageAsset>(previewImage);
         if (imgAsset && imgAsset->isAssetValid())
         {
            mPreviewImage->_setBitmap(imgAsset->getAssetId());
         }
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
      mPreviewImage->_setBitmap(StringTable->EmptyString());
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
         ImageAsset* imgAsset = AssetDatabase.acquireAsset<ImageAsset>(assetId);
         if (imgAsset && imgAsset->isAssetValid())
         {
            mPreviewImage->_setBitmap(imgAsset->getAssetId());
         }
      }
   }

   if (mPreviewImage->getBitmapAsset().isNull())
      mPreviewImage->_setBitmap(StringTable->insert("ToolsModule:genericAssetIcon_image"));
}

IMPLEMENT_CONOBJECT(GuiInspectorTypeImageAssetId);

ConsoleDocClass(GuiInspectorTypeImageAssetId,
   "@brief Inspector field type for Shapes\n\n"
   "Editor use only.\n\n"
   "@internal"
);

void GuiInspectorTypeImageAssetId::consoleInit()
{
   Parent::consoleInit();

   ConsoleBaseType::getType(TypeImageAssetId)->setInspectorFieldType("GuiInspectorTypeImageAssetId");
}
*/

