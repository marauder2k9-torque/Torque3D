#pragma once
#ifndef _TEXTURE_ASSET_H_

#ifndef _ASSET_BASE_H_
#include "assets/assetBase.h"
#endif

#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

class TextureAsset : public AssetBase
{
private:
   typedef AssetBase Parent;

public:
   enum TextureTypes
   {
      Albedo = 0,
      Normal = 1,
      ORMConfig = 2,
      GUI = 3,
      Roughness = 4,
      AO = 5,
      Metalness = 6,
      Glow = 7,
      Particle = 8,
      Decal = 9,
      Cubemap = 10,
      ImageTypeCount = 11
   };

private:

   StringTableEntry  mTextureFile;
   bool              mGenMips;
   bool              mIsHDR;
   GFXTexHandle      mTextureHandle;
   TextureTypes      mTextureType;
   HashMap<GFXTextureProfile*, GFXTexHandle> mResourceMap;

   void generateTexture(void);
public:
   TextureAsset();
   virtual ~TextureAsset();

   /// Engine initializer.
   static void initPersistFields();

   /// Sim
   bool onAdd() override;
   void onRemove() override;
   void copyTo(SimObject* object) override;

   void                    setImageFile(StringTableEntry pImageFile);
   inline StringTableEntry getImageFile(void) const { return mTextureFile; };

   void                    setGenMips(const bool pGenMips);
   inline bool             getGenMips(void) const { return mGenMips; };

   void                    setTextureHDR(const bool pIsHDR);
   inline bool             getTextureHDR(void) const { return mIsHDR; };

   inline GFXTexHandle&    getTexture(void) { return mTextureHandle; }
   GFXTexHandle            getTexture(GFXTextureProfile* requestedProfile);

   inline U32              getTextureWidth(void) const { return mTextureHandle->getWidth(); }
   inline U32              getTextureHeight(void) const { return mTextureHandle->getHeight(); }
   inline U32              getTextureDepth(void) const { return mTextureHandle->getDepth(); }

   inline U32              getTextureBitmapWidth(void) const { return mTextureHandle->getBitmapWidth(); }
   inline U32              getTextureBitmapHeight(void) const { return mTextureHandle->getBitmapHeight(); }
   inline U32              getTextureBitmapDepth(void) const { return mTextureHandle->getBitmapDepth(); }
   bool                    isAssetValid(void) const override { return !mTextureHandle.isNull(); }

   /// Declare Console Object.
   DECLARE_CONOBJECT(TextureAsset);

protected:
   // Asset Base callback
   void initializeAsset(void) override;
   void onAssetRefresh(void) override;

   /// Taml callbacks.
   void onTamlPreWrite(void) override;
   void onTamlPostWrite(void) override;

protected:
   // Texture file 
   static bool setTextureFile(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<TextureAsset*>(obj)->setImageFile(data); return false; }
   static const char* getTextureFile(void* obj, StringTableEntry data) { return static_cast<TextureAsset*>(obj)->getImageFile(); }
   static bool writeTextureFile(void* obj, StringTableEntry pFieldName) { return static_cast<TextureAsset*>(obj)->getImageFile() != StringTable->EmptyString(); }

   // Gen mips?
   static bool setGenMips(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<TextureAsset*>(obj)->setGenMips(dAtob(data)); return false; }
   static bool writeGenMips(void* obj, StringTableEntry pFieldName) { return static_cast<TextureAsset*>(obj)->getGenMips() == true; }

   // Texture Is Hdr?
   static bool setTextureHDR(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<TextureAsset*>(obj)->setTextureHDR(dAtob(data)); return false; }
   static bool writeTextureHDR(void* obj, StringTableEntry pFieldName) { return static_cast<TextureAsset*>(obj)->getTextureHDR() == true; }

};

//-----------------------------------------------------------------------------

DefineConsoleType(TypeTextureAssetPtr, TextureAsset)

//-----------------------------------------------------------------------------


#endif // !_TEXTURE_ASSET_H_
