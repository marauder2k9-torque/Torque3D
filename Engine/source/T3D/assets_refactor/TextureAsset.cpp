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

#include "T3D/assets_refactor/TextureAsset.h"

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(TextureAsset);

ConsoleType(TextureAssetPtr, TypeTextureAssetPtr, AssetPtr<TextureAsset>, ASSET_ID_FIELD_PREFIX)
ImplementConsoleTypeCasters(TypeTextureAssetPtr, AssetPtr<TextureAsset>)

//-----------------------------------------------------------------------------

ConsoleGetType(TypeTextureAssetPtr)
{
   return(*((AssetPtr<TextureAsset>*)dptr)).getAssetId();
}

ConsoleSetType(TypeTextureAssetPtr)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset pointer.
      AssetPtr<TextureAsset>* pAssetPtr = dynamic_cast<AssetPtr<TextureAsset>*>((AssetPtrBase*)(dptr));

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

typedef TextureAsset::TextureTypes TextureType;
DefineEnumType(TextureType);

ImplementEnumType(TextureType,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ TextureType::Albedo,      "Albedo",      ""},
{ TextureType::Normal,      "Normal",      "" },
{ TextureType::ORMConfig,   "ORMConfig",   "" },
{ TextureType::GUI,         "GUI",         "" },
{ TextureType::Roughness,   "Roughness",   "" },
{ TextureType::AO,          "AO",          "" },
{ TextureType::Metalness,   "Metalness",   "" },
{ TextureType::Glow,        "Glow",        "" },
{ TextureType::Particle,    "Particle",    "" },
{ TextureType::Decal,       "Decal",       "" },
{ TextureType::Cubemap,     "Cubemap",       "" },

EndImplementEnumType;

//-----------------------------------------------------------------------------

TextureAsset::TextureAsset()
   :  mTextureFile(StringTable->EmptyString()),
      mTextureType(TextureType::Albedo),
      mGenMips(false),
      mIsHDR(false),
      mTextureHandle(NULL)
{
   mLoadedState = AssetErrCode::NotLoaded;
}

TextureAsset::~TextureAsset()
{
   if (mLoadedState == Ok)
      Torque::FS::RemoveChangeNotification(mTextureFile, this, &TextureAsset::_onFileChanged);

   SAFE_DELETE(mTextureHandle);
}

void TextureAsset::initPersistFields()
{
   // Asset Base fields
   Parent::initPersistFields();

   addProtectedField("TextureFile", TypeAssetLooseFilePath, Offset(mTextureFile, TextureAsset), &setTextureFile, &getTextureFile, defaultProtectedWriteFn, "Path to the texture image.");
   addProtectedField("GenMips", TypeBool, Offset(mIsHDR, TextureAsset), &setGenMips, &defaultProtectedGetFn, &writeGenMips, "Generate mip maps?");
   addProtectedField("isHDR", TypeBool, Offset(mIsHDR, TextureAsset), &setTextureHDR, &defaultProtectedGetFn, &writeTextureHDR, "HDR Image?");
   addField("TextureType", TypeTextureType, Offset(mTextureType, TextureAsset), "The texture type eg:Albedo.");
}

bool TextureAsset::onAdd()
{
   // Call Parent.
   if (!Parent::onAdd())
      return false;

   return true;
}

void TextureAsset::onRemove()
{
   // Call Parent.
   Parent::onRemove();
}

void TextureAsset::copyTo(SimObject* object)
{
   // Call to parent.
   Parent::copyTo(object);

   TextureAsset* pAsset = static_cast<TextureAsset*>(object);

   // Sanity!
   AssertFatal(pAsset != NULL, "TextureAsset::copyTo() - Object is not the correct type.");

   pAsset->setImageFile(getImageFile());
   pAsset->setGenMips(getGenMips());
   pAsset->setTextureHDR(getTextureHDR());   
}

void TextureAsset::setImageFile(StringTableEntry pImageFile)
{
   // Sanity!
   AssertFatal(pImageFile != NULL, "Cannot use a NULL image file.");

   pImageFile = StringTable->insert(pImageFile);

   if (pImageFile == mTextureFile)
      return;

   // if we previously loaded, remove the listener for the file.
   if(mLoadedState == Ok)
      Torque::FS::RemoveChangeNotification(mTextureFile, this, &TextureAsset::_onFileChanged);

   mTextureFile = getOwned() ? expandAssetFilePath(pImageFile) : StringTable->insert(pImageFile);

   refreshAsset();
}

void TextureAsset::setGenMips(const bool pGenMips)
{
   if (pGenMips == mGenMips)
      return;

   mGenMips = pGenMips;

   refreshAsset();
}


void TextureAsset::setTextureHDR(const bool pIsHDR)
{
   if (pIsHDR == mIsHDR)
      return;

   mIsHDR = pIsHDR;

   refreshAsset();
}

GFXTexHandle TextureAsset::getTexture(GFXTextureProfile* requestedProfile)
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
         GFXTexHandle newTex = TEXMGR->createTexture(mTextureFile, requestedProfile);
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

void TextureAsset::initializeAsset(void)
{
   // Call parent.
   Parent::initializeAsset();

   // Ensure the image-file is expanded.
   mTextureFile = expandAssetFilePath(mTextureFile);
}

void TextureAsset::onAssetRefresh(void)
{
   // Ignore if not yet added to the sim.
   if (!isProperlyAdded())
      return;

   // Call parent.
   Parent::onAssetRefresh();

   mLoadedState = NotLoaded;
}

U32 TextureAsset::load()
{
   if (mLoadedState == Ok)
      return;

   if (!Torque::FS::IsFile(mTextureFile))
   {
      Con::errorf("ImageAsset::initializeAsset: Attempted to load file %s but it was not valid!", mTextureFile);
      mLoadedState = BadFileReference;
      return mLoadedState;
   }
   else
   {
      Torque::FS::AddChangeNotification(mTextureFile, this, &TextureAsset::_onFileChanged);
   }

   generateTexture();

   return mLoadedState;
}


void TextureAsset::_onFileChanged(const Torque::Path& path)
{
   if (path != Torque::Path(mTextureFile))
      return;

   refreshAsset();
}

void TextureAsset::onTamlPreWrite(void)
{
   // Call parent.
   Parent::onTamlPreWrite();

   // Ensure the image-file is collapsed.
   mTextureFile = collapseAssetFilePath(mTextureFile);
}

void TextureAsset::onTamlPostWrite(void)
{
   // Call parent.
   Parent::onTamlPostWrite();

   // Ensure the image-file is expanded.
   mTextureFile = expandAssetFilePath(mTextureFile);
}

void TextureAsset::generateTexture(void)
{
   StringBuilder str;
   str.append("TextureAssetProfile");
   // implement some defaults, eventually SRGB should be optional.
   U32 flags = GFXTextureProfile::Static | GFXTextureProfile::SRGB;

   str.append("STATICSRGB");

   // dont want mips?
   if (!mGenMips)
   {
      flags |= GFXTextureProfile::NoMipmap;
      str.append("NOMIP");
   }

   GFXTextureProfile::Types type = GFXTextureProfile::Types::DiffuseMap;

   if (mTextureType == TextureTypes::Normal) {
      str.append("NORMAL");
      type = GFXTextureProfile::Types::NormalMap;
   }
   else
   {
      str.append("DIFFUSE");
   }

   GFXTextureProfile* tempProfile = new GFXTextureProfile(str.end(), type, flags);

   mTextureHandle = TEXMGR->createTexture(mTextureFile, tempProfile);

   if (mTextureHandle.isValid())
      mLoadedState = AssetErrCode::Ok;
   else
      mLoadedState = AssetErrCode::Failed;
}

