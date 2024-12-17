#pragma once
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
#pragma once

#ifndef _ASSET_BASE_H_
#include "assets/assetBase.h"
#endif

#ifndef _GFXTEXTUREHANDLE_H_
#include "gfx/gfxTextureHandle.h"
#endif

//-----------------------------------------------------------------------------
class ImageAsset : public AssetBase
{
   typedef AssetBase Parent;
   typedef AssetPtr<ImageAsset> ConcreteAssetPtr;

public:
   /// The different types of image use cases
   enum ImageTypes
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

   static StringTableEntry smNoImageAssetFallback;

   enum ImageAssetErrCode
   {
      TooManyMips = AssetErrCode::Extended,
      Extended
   };

   static const String mErrCodeStrings[U32(ImageAssetErrCode::Extended) - U32(Parent::Extended) + 1];
   static U32 getAssetErrCode(ConcreteAssetPtr checkAsset) { if (checkAsset) return checkAsset->mLoadedState; else return 0; }

   static String getAssetErrstrn(U32 errCode)
   {
      if (errCode < Parent::Extended) return Parent::getAssetErrstrn(errCode);
      if (errCode > ImageAssetErrCode::Extended) return "undefined error";
      return mErrCodeStrings[errCode - Parent::Extended];
   };

   class Frame
   {
   public:
      Frame(const S32 pixelOffsetX, const S32 pixelOffsetY,
         const U32 pixelWidth, const U32 pixelHeight,
         const F32 texelWidthScale, const F32 texelHeightScale,
         StringTableEntry inRegionName = StringTable->EmptyString())
         : regionName(inRegionName)
      {
         pixelOffset.set(pixelOffsetY, pixelOffsetY);
         pixelSize.set(pixelWidth, pixelHeight);

         texelLower.set(pixelOffsetX * texelWidthScale, pixelOffsetY * texelHeightScale);
         texelSize.set(pixelWidth * texelWidthScale, pixelHeight * texelHeightScale);
         texelUpper.set(texelLower.x + texelSize.x, texelLower.y + texelSize.y);
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

   StringTableEntry  mImageFile;
   bool              mUseMips;
   bool              mIsHDRImage;
   GFXTexHandle      mTextureHandle;
   ImageTypes        mImageType;
   HashMap<GFXTextureProfile*, GFXTexHandle> mResourceMap;

   void generateTexture(void);
public:
   ImageAsset();
   virtual ~ImageAsset();

   /// Set up some global script interface stuff.
   static void consoleInit();

   /// Engine.
   static void initPersistFields();

   /// Sim
   bool onAdd() override;
   void onRemove() override;
   void copyTo(SimObject* object) override;

   /// Declare Console Object.
   DECLARE_CONOBJECT(ImageAsset);

   void _onResourceChanged(const Torque::Path& path);

   // asset Base load
   U32 load() override;

   void                    setImageFile(StringTableEntry pImageFile);
   inline StringTableEntry getImageFile(void) const { return mImageFile; };

   void                    setGenMips(const bool pGenMips);
   inline bool             getGenMips(void) const { return mUseMips; };

   void                    setTextureHDR(const bool pIsHDR);
   inline bool             getTextureHDR(void) const { return mIsHDRImage; };

   inline GFXTexHandle&    getTexture(void) { load(); return mTextureHandle; }
   GFXTexHandle            getTexture(GFXTextureProfile* requestedProfile);

   StringTableEntry        getImageTypeNameFromType(ImageTypes type);
   ImageTypes              getImageTypeFromName(StringTableEntry name);

   inline U32              getTextureWidth(void) const { return mTextureHandle->getWidth(); }
   inline U32              getTextureHeight(void) const { return mTextureHandle->getHeight(); }
   inline U32              getTextureDepth(void) const { return mTextureHandle->getDepth(); }

   inline U32              getTextureBitmapWidth(void) const { return mTextureHandle->getBitmapWidth(); }
   inline U32              getTextureBitmapHeight(void) const { return mTextureHandle->getBitmapHeight(); }
   inline U32              getTextureBitmapDepth(void) const { return mTextureHandle->getBitmapDepth(); }
   bool                    isAssetValid(void) const override { return !mTextureHandle.isNull(); }

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
   static bool setImageFile(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<ImageAsset*>(obj)->setImageFile(data); return false; }
   static const char* getImageFile(void* obj, StringTableEntry data) { return static_cast<ImageAsset*>(obj)->getImageFile(); }
   static bool writeImageFile(void* obj, StringTableEntry pFieldName) { return static_cast<ImageAsset*>(obj)->getImageFile() != StringTable->EmptyString(); }

   // Gen mips?
   static bool setGenMips(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<ImageAsset*>(obj)->setGenMips(dAtob(data)); return false; }
   static bool writeGenMips(void* obj, StringTableEntry pFieldName) { return static_cast<ImageAsset*>(obj)->getGenMips() == true; }

   // Texture Is Hdr?
   static bool setTextureHDR(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<ImageAsset*>(obj)->setTextureHDR(dAtob(data)); return false; }
   static bool writeTextureHDR(void* obj, StringTableEntry pFieldName) { return static_cast<ImageAsset*>(obj)->getTextureHDR() == true; }
};

//-----------------------------------------------------------------------------

DefineUnmappedConsoleType(TypeImageAssetPtr, AssetPtr<ImageAsset>)

//-----------------------------------------------------------------------------

typedef ImageAsset::ImageTypes ImageAssetType;
DefineEnumType(ImageAssetType);

//Singular assets
/// <Summary>
/// Declares an image asset
/// This establishes the assetId, asset and legacy filepath fields, along with supplemental getter and setter functions
/// </Summary>
#define DECLARE_IMAGEASSET(className, name, changeFunc, profile)

#define INITPERSISTFIELD_IMAGEASSET(name, consoleClass, docs)

#define LOAD_IMAGEASSET(name)

#define DECLARE_IMAGEASSET_ARRAY(className, name, max)

#define DECLARE_IMAGEASSET_ARRAY_SETGET(className, name)
 
#define INIT_IMAGEASSET_ARRAY(name, profile, index)

#define DEF_IMAGEASSET_ARRAY_BINDS(className,name)

#define INITPERSISTFIELD_IMAGEASSET_ARRAY(name, arraySize, consoleClass, docs)

#define LOAD_IMAGEASSET_ARRAY(name, index)


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
#define DECLARE_IMAGEASSET(className, name, profile) public: \
AssetPtr<ImageAsset> m##name##Asset;\
GFXTextureProfile*   m##name##Profile = &profile;\
void                 set##name( const char* pAssetId );\
inline const AssetPtr<ImageAsset>& get##name(void) const { return m##name##Asset; }\
protected:\
static bool _set##name##Data(void* obj, const char* index, const char* data) { static_cast<className*>(obj)->set##name##(data); return false; }\

#define INITPERSISTFIELD_IMAGEASSET(name, consoleClass, docs) \
   addProtectedField(assetText(name, Asset), TypeImageAssetPtr, Offset(m##name##Asset, consoleClass), &_set##name##Data, &defaultProtectedGetFn, assetDoc(name, asset docs.));

#define INIT_IMAGEASSET(name) \
   m##name##Asset = NULL;

#define PACK_IMAGEASSET(netconn, name)\
   if (stream->writeFlag(m##name##Asset.notNull()))\
   {\
      NetStringHandle assetIdStr = m##name##Asset.getAssetId();\
      netconn->packNetStringHandleU(stream, assetIdStr);\
   }\

#define UNPACK_IMAGEASSET(netconn, name)\
   if (stream->readFlag())\
   {\
      m##name##Asset.setAssetId(StringTable->insert(netconn->unpackNetStringHandleU(stream).getString()));\
   }

#define PACKDATA_IMAGEASSET(name)\
   if (stream->writeFlag(m##name##Asset.notNull()))\
   {\
      stream->writeString(m##name##Asset.getAssetId());\
   }\

#define UNPACKDATA_IMAGEASSET(name)\
   if (stream->readFlag())\
   {\
      m##name##Asset.setAssetId(StringTable->insert(stream->readSTString()));\
   }

#define DEF_IMAGEASSET_BINDS(className, name)\
DefineEngineMethod(className, get##name, StringTableEntry, (), , "get name")\
{\
   return object->get##name()->getAssetId(); \
}\
DefineEngineMethod(className, get##name##Asset, StringTableEntry, (), , assetText(name, asset reference))\
{\
   return object->m##name##Asset->getAssetId(); \
}\
DefineEngineMethod(className, set##name, void, (const char* assetName), , assetText(name,assignment. first tries asset then flat file.))\
{\
   object->set##name(StringTable->insert(assetName));\
}
