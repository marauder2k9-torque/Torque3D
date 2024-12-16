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

   class Frame
   {
   public:
      Frame() {}
      Frame(const S32 pixelOffsetX, const S32 pixelOffsetY,
            const U32 pixelWidth, const U32 pixelHeight,
            const F32 texelWidthScale, const F32 texelHeightScale,
            StringTableEntry inRegionName = StringTable->EmptyString())
      {
         pixelOffset.set(pixelOffsetY, pixelOffsetY);
         pixelSize.set(pixelWidth, pixelHeight);

         texelLower.set(pixelOffsetX * texelWidthScale, pixelOffsetY * texelHeightScale);
         texelSize.set(pixelWidth * texelWidthScale, pixelHeight * texelHeightScale);
         texelUpper.set(texelLower.x + texelSize.x, texelLower.y + texelSize.y);

         if (inRegionName != StringTable->EmptyString()) {
            regionName = StringTable->insert(inRegionName);
         }
      }

      void setFlip(bool flipX, bool flipY)
      {
         if (flipX) mSwap(texelLower.x, texelUpper.x);
         if (flipY) mSwap(texelLower.y, texelUpper.y);
      }

      Point2I pixelOffset;
      Point2I pixelSize;

      Point2F texelLower;
      Point2F texelUpper;
      Point2F texelSize;

      StringTableEntry regionName;
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

   // asset Base load
   U32 load() override;

   void                    setImageFile(StringTableEntry pImageFile);
   inline StringTableEntry getImageFile(void) const { return mTextureFile; };

   void                    setGenMips(const bool pGenMips);
   inline bool             getGenMips(void) const { return mGenMips; };

   void                    setTextureHDR(const bool pIsHDR);
   inline bool             getTextureHDR(void) const { return mIsHDR; };

   inline GFXTexHandle&    getTexture(void) { load(); return mTextureHandle; }
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
   void _onFileChanged(const Torque::Path& path);

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

DefineUnmappedConsoleType(TypeTextureAssetPtr, AssetPtr<TextureAsset>)

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------

#pragma region Singular Asset Macros

/*! Macro for declaring an asset.
* 
* This will add a function set<name>(const char* pAssetId) to the class
* which must be filled in for the classes handling of asset changes.
* 
* @param className The class we want to add this asset to.
* @param name The name of the Asset variable
* @param profile The GFXTextureProfile we want to use with this texture asset.
*/
#define DECLARE_TEXTUREASSET(className, name, profile) public: \
AssetPtr<TextureAsset> m##name##Asset;\
GFXTextureProfile*   m##name##Profile = &profile;\
void                 set##name( const char* pAssetId );\
inline const AssetPtr<TextureAsset>& get##name(void) const { return m##name##Asset; }\
protected:\
static bool _set##name##Data(void* obj, const char* index, const char* data) { static_cast<className*>(obj)->set##name##(data); return false; }\

#define INITPERSISTFIELD_TEXTUREASSET(name, consoleClass, docs) \
   addProtectedField(assetText(name, Asset), TypeTextureAssetPtr, Offset(m##name##Asset, consoleClass), &_set##name##Data, &defaultProtectedGetFn, assetDoc(name, asset docs.));

#define INIT_TEXTUREASSET(name) \
   m##name##Asset = NULL;

#define PACK_TEXTUREASSET(netconn, name)\
   if (stream->writeFlag(m##name##Asset.notNull()))\
   {\
      NetStringHandle assetIdStr = m##name##Asset.getAssetId();\
      netconn->packNetStringHandleU(stream, assetIdStr);\
   }\

#define UNPACK_TEXTUREASSET(netconn, name)\
   if (stream->readFlag())\
   {\
      m##name##Asset.setAssetId(StringTable->insert(netconn->unpackNetStringHandleU(stream).getString()));\
   }

#endif // !_TEXTURE_ASSET_H_
