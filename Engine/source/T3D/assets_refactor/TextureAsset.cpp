#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "assets/assetManager.h"
#include "assets/assetPtr.h"

#include "platform/profiler.h"

#include "T3D/assets_refactor/TextureAsset.h"

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(TextureAsset);

ConsoleType(TextureAssetPtr, TypeTextureAssetPtr, AssetPtr<TextureAsset>, ASSET_ID_FIELD_PREFIX)

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

}

TextureAsset::~TextureAsset()
{
}

void TextureAsset::initPersistFields()
{
   // Asset Base fields
   Parent::initPersistFields();

   addProtectedField("TextureFile", TypeAssetLooseFilePath, Offset(mTextureFile, TextureAsset), &setTextureFile, &getTextureFile, "Path to the texture image.");

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
}

void TextureAsset::setImageFile(StringTableEntry pImageFile)
{
   // Sanity!
   AssertFatal(pImageFile != NULL, "Cannot use a NULL image file.");

   pImageFile = StringTable->insert(pImageFile);

   if (pImageFile == mTextureFile)
      return;

   mTextureFile = getOwned() ? expandAssetFilePath(pImageFile) : StringTable->insert(pImageFile);

   refreshAsset();
}
