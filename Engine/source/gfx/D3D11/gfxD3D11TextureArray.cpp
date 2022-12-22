#include "gfxD3D11TextureArray.h"

#include <d3d11.h>

#include "gfxD3D11Device.h"
#include "gfxD3D11EnumTranslate.h"
#include "core/util/tVector.h"
#include "gfx/util/screenspace.h"
#include "shaderGen/shaderFeature.h"
#include "gfx/bitmap/imageUtils.h"


void GFXD3D11TextureArray::init()
{
   mTextureArrayDesc.Width = mWidth;
   mTextureArrayDesc.Height = mHeight;
   mTextureArrayDesc.MipLevels = mMipLevels;
   mTextureArrayDesc.ArraySize = mArraySize;
   mTextureArrayDesc.Format = GFXD3D11TextureFormat[mFormat];
   mTextureArrayDesc.SampleDesc.Count = 1;
   mTextureArrayDesc.SampleDesc.Quality = 0;
   mTextureArrayDesc.Usage = D3D11_USAGE_DEFAULT;
   mTextureArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
   mTextureArrayDesc.CPUAccessFlags = 0;
   mTextureArrayDesc.MiscFlags = 0;
   VECTOR_SET_ASSOCIATION(mRTView);

   HRESULT hr = D3D11DEVICE->CreateTexture2D(&mTextureArrayDesc, NULL, &mTextureArray);
   AssertFatal(SUCCEEDED(hr), "GFXD3D11TextureArray::init failed to create texture array!");

   //---------------------------------------------------------------------------------------
   //					Create a resource view to the texture array.
   //---------------------------------------------------------------------------------------
   createResourceView(mTextureArrayDesc.Format, mTextureArrayDesc.MipLevels, mTextureArrayDesc.BindFlags);
   //---------------------------------------------------------------------------------------
}

void GFXD3D11TextureArray::initDynamic(U32 texSize, GFXFormat faceFormat, U32 mipLevels, U32 arraySize)
{
   mDynamic = true;
   mWidth = texSize;
   mHeight = texSize;
   mFormat = faceFormat;
   if (!mipLevels)
      mAutoGenMips = true;

   mArraySize = arraySize;
   mMipLevels = mipLevels;

   bool compressed = ImageUtil::isCompressedFormat(mFormat);

   UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
   if (!compressed)
   {
      bindFlags |= D3D11_BIND_RENDER_TARGET;
   }

   D3D11_TEXTURE2D_DESC desc;

   desc.Width = mWidth;
   desc.Height = mWidth;
   desc.MipLevels = mMipLevels;
   desc.ArraySize = 6;
   desc.Format = GFXD3D11TextureFormat[mFormat];
   desc.SampleDesc.Count = 1;
   desc.SampleDesc.Quality = 0;
   desc.Usage = D3D11_USAGE_DEFAULT;
   desc.BindFlags = bindFlags;
   desc.CPUAccessFlags = 0;
   desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

   HRESULT hr = D3D11DEVICE->CreateTexture2D(&desc, NULL, &mTextureArray);

   D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
   SMViewDesc.Format = GFXD3D11TextureFormat[mFormat];
   SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
   SMViewDesc.Texture2DArray.MostDetailedMip = 0;
   SMViewDesc.Texture2DArray.MipLevels = mipLevels;
   SMViewDesc.Texture2DArray.FirstArraySlice = 0;
   SMViewDesc.Texture2DArray.ArraySize = mArraySize;

   hr = D3D11DEVICE->CreateShaderResourceView(mTextureArray, &SMViewDesc, &mSRView);
   AssertFatal(SUCCEEDED(hr), "GFXD3D11TextureArray::CreateShaderResourceView failed to create view!");

   //Generate mips
   if (mAutoGenMips && !compressed)
   {
      D3D11DEVICECONTEXT->GenerateMips(mSRView);
      //get mip level count
      D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
      mSRView->GetDesc(&viewDesc);
      mMipLevels = viewDesc.Texture2DArray.MipLevels;
   }

   D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
   viewDesc.Format = desc.Format;
   viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
   viewDesc.Texture2DArray.ArraySize = 1;
   viewDesc.Texture2DArray.MipSlice = 0;

   mRTView.setSize(mArraySize);
   for (U32 i = 0; i < mArraySize; i++)
   {
      viewDesc.Texture2DArray.FirstArraySlice = i;
      hr = D3D11DEVICE->CreateRenderTargetView(mTextureArray, &viewDesc, &mRTView[i]);
      AssertFatal(SUCCEEDED(hr), "GFXD3D11TextureArray::CreateRenderTargetView failed to create view!");
   }



   D3D11_TEXTURE2D_DESC depthTexDesc;
   depthTexDesc.Width = mWidth;
   depthTexDesc.Height = mWidth;
   depthTexDesc.MipLevels = 1;
   depthTexDesc.ArraySize = mArraySize;
   depthTexDesc.SampleDesc.Count = 1;
   depthTexDesc.SampleDesc.Quality = 0;
   depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
   depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
   depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
   depthTexDesc.CPUAccessFlags = 0;
   depthTexDesc.MiscFlags = 0;

   ID3D11Texture2D* depthTex = 0;
   hr = D3D11DEVICE->CreateTexture2D(&depthTexDesc, 0, &depthTex);

   if (FAILED(hr))
   {
      AssertFatal(false, "GFXD3D11TextureArray::initDynamic - CreateTexture2D for depth stencil call failure");
   }

   // Create the depth stencil view for the entire cube
   D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
   dsvDesc.Format = depthTexDesc.Format; //The format must match the depth texture we created above
   dsvDesc.Flags = 0;
   dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
   dsvDesc.Texture2D.MipSlice = 0;
   hr = D3D11DEVICE->CreateDepthStencilView(depthTex, &dsvDesc, &mDSView);

   if (FAILED(hr))
   {
      AssertFatal(false, "GFXD3D11TextureArray::initDynamic - CreateDepthStencilView call failure");
   }

   SAFE_RELEASE(depthTex);

}

void GFXD3D11TextureArray::_setTexture(const GFXTexHandle& texture, U32 slot)
{
   GFXD3D11TextureObject *texObj = dynamic_cast<GFXD3D11TextureObject*>(texture.getPointer());
   ID3D11Texture2D* tex2d = texObj->get2DTex();
   D3D11_TEXTURE2D_DESC desc;
   tex2d->GetDesc(&desc);
   // for each mipmap level...
   for (UINT j = 0; j < desc.MipLevels; ++j)
   {
      const U32 srcSubResource = D3D11CalcSubresource(j, 0, desc.MipLevels);
      const U32 dstSubResource = D3D11CalcSubresource(j, slot, mTextureArrayDesc.MipLevels);
      D3D11DEVICECONTEXT->CopySubresourceRegion(mTextureArray, dstSubResource, 0, 0, 0, tex2d, srcSubResource, NULL);
   }
}

void GFXD3D11TextureArray::setToTexUnit(U32 tuNum)
{
   D3D11DEVICECONTEXT->PSSetShaderResources(tuNum, 1, &mSRView);
}

void GFXD3D11TextureArray::createResourceView(DXGI_FORMAT format, U32 numMipLevels, U32 usageFlags)
{
   HRESULT hr;
   if (usageFlags & D3D11_BIND_SHADER_RESOURCE)
   {
      D3D11_SHADER_RESOURCE_VIEW_DESC desc;

      if (usageFlags & D3D11_BIND_DEPTH_STENCIL)
         desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // reads the depth
      else
         desc.Format = format;

      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
      desc.Texture2DArray.MostDetailedMip = 0;
      desc.Texture2DArray.MipLevels = numMipLevels;
      desc.Texture2DArray.FirstArraySlice = 0;
      desc.Texture2DArray.ArraySize = mArraySize;

      hr = D3D11DEVICE->CreateShaderResourceView(mTextureArray, &desc, &mSRView);
      AssertFatal(SUCCEEDED(hr), "GFXD3D11TextureArray::CreateShaderResourceView failed to create view!");
   }

   if (usageFlags & D3D11_BIND_RENDER_TARGET)
   {
      D3D11_RENDER_TARGET_VIEW_DESC desc;
      desc.Format = format;
      desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
      desc.Texture2DArray.MipSlice = 0;
      desc.Texture2DArray.ArraySize = 1;
      desc.Texture2DArray.MipSlice = 0;
      for (U32 i = 0; i < mArraySize; i++)
      {
         desc.Texture2DArray.FirstArraySlice = i;
         hr = D3D11DEVICE->CreateRenderTargetView(mTextureArray, &desc, &mRTView[i]);
         AssertFatal(SUCCEEDED(hr), "GFXD3D11TextureArray::CreateRenderTargetView failed to create view!");
      }
   }

   if (usageFlags & D3D11_BIND_DEPTH_STENCIL)
   {
      D3D11_DEPTH_STENCIL_VIEW_DESC desc;
      desc.Format = format;
      desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
      desc.Texture2DArray.MipSlice = 0;
      desc.Texture2DArray.FirstArraySlice = 0;
      desc.Texture2DArray.ArraySize = mArraySize;
      desc.Flags = 0;
      hr = D3D11DEVICE->CreateDepthStencilView(mTextureArray, &desc, &mDSView);
      AssertFatal(SUCCEEDED(hr), "GFXD3D11TextureArray::CreateDepthStencilView failed to create view!");
   }
}


void GFXD3D11TextureArray::Release()
{
   SAFE_RELEASE(mSRView)
      for (U32 i = 0; i < mArraySize; i++)
      {
            SAFE_RELEASE(mRTView[i]);
      }
   SAFE_RELEASE(mDSView)
   SAFE_RELEASE(mTextureArray)

   GFXTextureArray::Release();
}

ID3D11ShaderResourceView* GFXD3D11TextureArray::getSRView()
{
   return mSRView;
}
ID3D11RenderTargetView* GFXD3D11TextureArray::getRTView(U32 rtSlot)
{
   return mRTView[rtSlot];
}
ID3D11DepthStencilView* GFXD3D11TextureArray::getDSView()
{
   return mDSView;
}

ID3D11ShaderResourceView** GFXD3D11TextureArray::getSRViewPtr()
{
   return &mSRView;
}
ID3D11RenderTargetView** GFXD3D11TextureArray::getRTViewPtr(U32 rtSlot)
{
   return &mRTView[rtSlot];
}

ID3D11DepthStencilView** GFXD3D11TextureArray::getDSViewPtr()
{
   return &mDSView;
}

ID3D11Texture2D* GFXD3D11TextureArray::get2DTex()
{
   return mTextureArray;
}

void GFXD3D11TextureArray::zombify()
{
   // Unsupported
}

void GFXD3D11TextureArray::resurrect()
{
   // Unsupported
}
