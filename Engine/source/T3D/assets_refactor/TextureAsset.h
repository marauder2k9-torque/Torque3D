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

   /// Declare Console Object.
   DECLARE_CONOBJECT(TextureAsset);

protected:
   // Asset Base callback
   void initializeAsset(void) override;
   void onAssetRefresh(void) override;

   /// Taml callbacks.
   void onTamlPreWrite(void) override;
   void onTamlPostWrite(void) override;
   void onTamlCustomWrite(TamlCustomNodes& customNodes) override;
   void onTamlCustomRead(const TamlCustomNodes& customNodes) override;

protected:
   static bool setTextureFile(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<TextureAsset*>(obj)->setImageFile(data); return false; }
   static const char* getTextureFile(void* obj, StringTableEntry data) { return static_cast<TextureAsset*>(obj)->getImageFile(); }
   static bool writeTextureFile(void* obj, StringTableEntry pFieldName) { return static_cast<TextureAsset*>(obj)->getImageFile() != StringTable->EmptyString(); }

};

//-----------------------------------------------------------------------------

DefineConsoleType(TypeTextureAssetPtr, TextureAsset)

//-----------------------------------------------------------------------------


#endif // !_TEXTURE_ASSET_H_
