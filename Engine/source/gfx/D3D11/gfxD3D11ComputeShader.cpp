//-----------------------------------------------------------------------------
// Copyright (c) 2015 GarageGames, LLC
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
#include "gfx/D3D11/gfxD3D11ComputeShader.h"
#include "gfx/D3D11/gfxD3D11EnumTranslate.h"
#include "core/frameAllocator.h"
#include "core/stream/fileStream.h"
#include "core/util/safeDelete.h"
#include "console/console.h"

extern bool gDisassembleAllShaders;

#pragma comment(lib, "d3dcompiler.lib")

gfxD3DComputeIncludeRef GFXD3D11ComputeShader::smD3DInclude = NULL;

class gfxD3D11ComputeInclude : public ID3DInclude, public StrongRefBase
{
private:

   Vector<String> mLastPath;

public:

   void setPath(const String& path)
   {
      mLastPath.clear();
      mLastPath.push_back(path);
   }

   gfxD3D11ComputeInclude() {}
   virtual ~gfxD3D11ComputeInclude() {}

   STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
   STDMETHOD(Close)(THIS_ LPCVOID pData);
};

HRESULT gfxD3D11ComputeInclude::Open(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
   using namespace Torque;
   // First try making the path relative to the parent.
   Torque::Path path = Torque::Path::Join(mLastPath.last(), '/', pFileName);
   path = Torque::Path::CompressPath(path);

   if (!Torque::FS::ReadFile(path, (void*&)*ppData, *pBytes, true))
   {
      // Ok... now try using the path as is.
      path = String(pFileName);
      path = Torque::Path::CompressPath(path);

      if (!Torque::FS::ReadFile(path, (void*&)*ppData, *pBytes, true))
      {
         AssertISV(false, avar("Failed to open include '%s'.", pFileName));
         return E_FAIL;
      }
   }

   // If the data was of zero size then we cannot recurse
   // into this file and DX won't call Close() below.
   //
   // So in this case don't push on the path.
   if (*pBytes > 0)
      mLastPath.push_back(path.getRootAndPath());

   return S_OK;
}

HRESULT gfxD3D11ComputeInclude::Close(THIS_ LPCVOID pData)
{
   // Free the data file and pop its path off the stack.
   delete[](U8*)pData;
   mLastPath.pop_back();

   return S_OK;
}

GFXD3D11ComputeShaderConstHandle::GFXD3D11ComputeShaderConstHandle()
{
   clear();
}

const String& GFXD3D11ComputeShaderConstHandle::getName() const
{
   return mComputeHandle.name;
}

GFXShaderConstType GFXD3D11ComputeShaderConstHandle::getType() const
{
   return mComputeHandle.constType;
}

U32 GFXD3D11ComputeShaderConstHandle::getArraySize() const
{
   return mComputeHandle.arraySize;
}

S32 GFXD3D11ComputeShaderConstHandle::getSamplerRegister() const
{
   if (!mValid || !isSampler())
      return -1;

   // We always store sampler type and register index in the pixelHandle,
   // sampler registers are shared between vertex and pixel shaders anyway.

   return mComputeHandle.offset;
}

GFXD3D11ComputeConstBufferLayout::GFXD3D11ComputeConstBufferLayout()
{
   mSubBuffers.reserve(CBUFFER_MAX);
}

bool GFXD3D11ComputeConstBufferLayout::set(const ParamDesc& pd, const GFXShaderConstType constType, const U32 inSize, const void* data, U8* basePointer)
{
   PROFILE_SCOPE(GenericConstBufferLayout_set);
   S32 size = inSize;
   // Shader compilers like to optimize float4x4 uniforms into float3x3s.
   // So long as the real paramater is a matrix of-some-type and the data
   // passed in is a MatrixF ( which is will be ), we DO NOT have a
   // mismatched const type.
   AssertFatal(pd.constType == constType ||
      (
         (pd.constType == GFXSCT_Float2x2 ||
            pd.constType == GFXSCT_Float3x3 ||
            pd.constType == GFXSCT_Float4x3 ||
            pd.constType == GFXSCT_Float4x4) &&
         (constType == GFXSCT_Float2x2 ||
            constType == GFXSCT_Float3x3 ||
            constType == GFXSCT_Float4x3 ||
            constType == GFXSCT_Float4x4)
         ), "Mismatched const type!");

   // This "cute" bit of code allows us to support 2x3 and 3x3 matrices in shader constants but use our MatrixF class.  Yes, a hack. -BTR
   switch (pd.constType)
   {
   case GFXSCT_Float2x2:
   case GFXSCT_Float3x3:
   case GFXSCT_Float4x3:
   case GFXSCT_Float4x4:
      return setMatrix(pd, constType, size, data, basePointer);
      break;
      // TODO add other AlignedVector here
   case GFXSCT_Float2:
      if (size > sizeof(Point2F))
         size = pd.size;
   default:
      break;
   }

   AssertFatal(pd.size >= size, "Not enough room in the buffer for this data!");

   // Ok, we only set data if it's different than the data we already have, this maybe more expensive than just setting the data, but 
   // we'll have to do some timings to see.  For example, the lighting shader constants rarely change, but we can't assume that at the
   // renderInstMgr level, but we can check down here. -BTR
   if (dMemcmp(basePointer + pd.offset, data, size) != 0)
   {
      dMemcpy(basePointer + pd.offset, data, size);
      return true;
   }
   return false;
}

bool GFXD3D11ComputeConstBufferLayout::setMatrix(const ParamDesc& pd, const GFXShaderConstType constType, const U32 size, const void* data, U8* basePointer)
{
   PROFILE_SCOPE(GFXD3D11ComputeConstBufferLayout_setMatrix);

   if (pd.constType == GFXSCT_Float4x4)
   {
      // Special case, we can just blast this guy.
      AssertFatal(pd.size >= size, "Not enough room in the buffer for this data!");
      if (dMemcmp(basePointer + pd.offset, data, size) != 0)
      {
         dMemcpy(basePointer + pd.offset, data, size);
         return true;
      }

      return false;
   }
   else
   {
      PROFILE_SCOPE(GFXD3D11ConstBufferLayout_setMatrix_not4x4);

      // Figure out how big of a chunk we are copying.  We're going to copy 4 columns by N rows of data
      U32 csize;
      switch (pd.constType)
      {
      case GFXSCT_Float2x2:
         csize = 24; //this takes up 16+8
         break;
      case GFXSCT_Float3x3:
         csize = 44; //This takes up 16+16+12
         break;
      case GFXSCT_Float4x3:
         csize = 48;
         break;
      default:
         AssertFatal(false, "Unhandled case!");
         return false;
         break;
      }

      // Loop through and copy 
      bool ret = false;
      U8* currDestPointer = basePointer + pd.offset;
      const U8* currSourcePointer = static_cast<const U8*>(data);
      const U8* endData = currSourcePointer + size;
      while (currSourcePointer < endData)
      {
         if (dMemcmp(currDestPointer, currSourcePointer, csize) != 0)
         {
            dMemcpy(currDestPointer, currSourcePointer, csize);
            ret = true;
         }
         else if (pd.constType == GFXSCT_Float4x3)
         {
            ret = true;
         }

         currDestPointer += csize;
         currSourcePointer += sizeof(MatrixF);
      }

      return ret;
   }
}

//------------------------------------------------------------------------------
GFXD3D11ComputeShaderConstBuffer::GFXD3D11ComputeShaderConstBuffer(  GFXD3D11ComputeShader* shader,
                                                                     GFXD3D11ComputeConstBufferLayout* computeLayout)
{
   AssertFatal(shader, "GFXD3D11ComputeShaderConstBuffer() - Got a null shader!");

   // We hold on to this so we don't have to call
   // this virtual method during activation.
   mShader = shader;

   for (U32 i = 0; i < CBUFFER_MAX; ++i)
   {
      mConstantBuffersC[i] = NULL;
   }

   mComputeConstBufferLayout = computeLayout;
   mComputeConstBuffer = new GenericConstBuffer(computeLayout);

   mDeviceContext = D3D11DEVICECONTEXT;

   _createBuffers();

}

GFXD3D11ComputeShaderConstBuffer::~GFXD3D11ComputeShaderConstBuffer()
{
   // release constant buffer
   for (U32 i = 0; i < CBUFFER_MAX; ++i)
   {
      SAFE_RELEASE(mConstantBuffersC[i]);
   }

   SAFE_DELETE(mComputeConstBuffer);

   if (mShader)
      mShader->_unlinkBuffer(this);
}

void GFXD3D11ComputeShaderConstBuffer::_createBuffers()
{
   HRESULT hr;
   // Create a compute constant buffer
   if (mComputeConstBufferLayout->getBufferSize())
   {
      const Vector<ConstSubBufferDesc>& subBuffers = mComputeConstBufferLayout->getSubBufferDesc();
      for (U32 i = 0; i < subBuffers.size(); ++i)
      {
         // Create a pixel float constant buffer
         D3D11_BUFFER_DESC cbDesc;
         cbDesc.ByteWidth = subBuffers[i].size;
         cbDesc.Usage = D3D11_USAGE_DEFAULT;
         cbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_CONSTANT_BUFFER;
         cbDesc.CPUAccessFlags = 0;
         cbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
         cbDesc.StructureByteStride = 0;

         hr = D3D11DEVICE->CreateBuffer(&cbDesc, NULL, &mConstantBuffersC[i]);

         if (FAILED(hr))
         {
            AssertFatal(false, "can't create constant mConstantBuffersC!");
         }
      }
   }
}

GFXShader* GFXD3D11ComputeShaderConstBuffer::getShader()
{
   return mShader;
}

// This is kind of cheesy, but I don't think templates would work well here because 
// these functions potentially need to be handled differently by other derived types
template<class T>
inline void GFXD3D11ComputeShaderConstBuffer::SET_CONSTANT(GFXShaderConstHandle* handle, const T& fv,
   GenericConstBuffer* cBuffer)
{
   AssertFatal(static_cast<const GFXD3D11ComputeShaderConstHandle*>(handle), "Incorrect const buffer type!");
   const GFXD3D11ComputeShaderConstHandle* h = static_cast<const GFXD3D11ComputeShaderConstHandle*>(handle);
   AssertFatal(h, "Handle is NULL!");
   AssertFatal(h->isValid(), "Handle is not valid!");
   AssertFatal(!h->isSampler(), "Handle is sampler constant!");
   AssertFatal(!mShader.isNull(), "Buffer's shader is null!");
   AssertFatal(!h->mShader.isNull(), "Handle's shader is null!");
   AssertFatal(h->mShader.getPointer() == mShader.getPointer(), "Mismatched shaders!");

   if (h->mComputeConstant)
      cBuffer->set(h->mComputeHandle, fv);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const F32 fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const Point2F& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const Point3F& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const Point4F& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const PlaneF& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const LinearColorF& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const S32 f)
{
   // This is the only type that is allowed to be used
   // with a sampler shader constant type, but it is only
   // allowed to be set from GLSL.
   //
   // So we ignore it here... all other cases will assert.
   //
   if (((GFXD3D11ComputeShaderConstHandle*)handle)->isSampler())
      return;

   SET_CONSTANT(handle, f, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const Point2I& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const Point3I& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const Point4I& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<F32>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<Point2F>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<Point3F>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<Point4F>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<S32>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<Point2I>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<Point3I>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const AlignedArray<Point4I>& fv)
{
   SET_CONSTANT(handle, fv, mComputeConstBuffer);
}
#undef SET_CONSTANT

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const MatrixF& mat, const GFXShaderConstType matrixType)
{
   AssertFatal(handle, "Handle is NULL!");
   AssertFatal(handle->isValid(), "Handle is not valid!");

   AssertFatal(static_cast<const GFXD3D11ComputeShaderConstHandle*>(handle), "Incorrect const buffer type!");
   const GFXD3D11ComputeShaderConstHandle* h = static_cast<const GFXD3D11ComputeShaderConstHandle*>(handle);
   AssertFatal(!h->isSampler(), "Handle is sampler constant!");
   AssertFatal(h->mShader == mShader, "Mismatched shaders!");

   MatrixF transposed;
   if (matrixType == GFXSCT_Float4x3)
   {
      transposed = mat;
   }
   else
   {
      mat.transposeTo(transposed);
   }

   if (h->mComputeConstant)
      mComputeConstBuffer->set(h->mComputeHandle, transposed, matrixType);
}

void GFXD3D11ComputeShaderConstBuffer::set(GFXShaderConstHandle* handle, const MatrixF* mat, const U32 arraySize, const GFXShaderConstType matrixType)
{
   AssertFatal(handle, "Handle is NULL!");
   AssertFatal(handle->isValid(), "Handle is not valid!");

   AssertFatal(static_cast<const GFXD3D11ComputeShaderConstHandle*>(handle), "Incorrect const buffer type!");
   const GFXD3D11ComputeShaderConstHandle* h = static_cast<const GFXD3D11ComputeShaderConstHandle*>(handle);
   AssertFatal(!h->isSampler(), "Handle is sampler constant!");
   AssertFatal(h->mShader == mShader, "Mismatched shaders!");

   static Vector<MatrixF> transposed;
   if (arraySize > transposed.size())
      transposed.setSize(arraySize);

   if (matrixType == GFXSCT_Float4x3)
   {
      dMemcpy(transposed.address(), mat, arraySize * sizeof(MatrixF));
   }
   else
   {
      for (U32 i = 0; i < arraySize; i++)
         mat[i].transposeTo(transposed[i]);
   }

   if (h->mComputeConstant)
      mComputeConstBuffer->set(h->mComputeHandle, transposed.begin(), arraySize, matrixType);
}

const String GFXD3D11ComputeShaderConstBuffer::describeSelf() const
{
   String ret;
   ret = String("   GFXD3D11ComputeShaderConstBuffer\n");

   for (U32 i = 0; i < mComputeConstBufferLayout->getParameterCount(); i++)
   {
      GenericConstBufferLayout::ParamDesc pd;
      mComputeConstBufferLayout->getDesc(i, pd);

      ret += String::ToString("      Constant name: %s", pd.name.c_str());
   }

   return ret;
}

void GFXD3D11ComputeShaderConstBuffer::zombify()
{
}

void GFXD3D11ComputeShaderConstBuffer::resurrect()
{
}

bool GFXD3D11ComputeShaderConstBuffer::isDirty()
{
   bool ret = mComputeConstBuffer->isDirty();

   return ret;
}

void GFXD3D11ComputeShaderConstBuffer::activate(GFXD3D11ComputeShaderConstBuffer* prevShaderBuffer)
{
   PROFILE_SCOPE(GFXD3D11ComputeShaderConstBuffer_activate);

   if (prevShaderBuffer != this)
   {
      if (prevShaderBuffer && !prevShaderBuffer->isDirty())
      {
         mComputeConstBuffer->setDirty(!prevShaderBuffer->mComputeConstBuffer->isEqual(mComputeConstBuffer));
      }
   }

   D3D11_MAPPED_SUBRESOURCE pConstData;
   ZeroMemory(&pConstData, sizeof(D3D11_MAPPED_SUBRESOURCE));

   const U8* buf;
   U32 nbBuffers = 0;
   if (mComputeConstBuffer->isDirty())
   {
      const Vector<ConstSubBufferDesc>& subBuffers = mComputeConstBufferLayout->getSubBufferDesc();
      // TODO: This is not very effecient updating the whole lot, re-implement the dirty system to work with multiple constant buffers.
      // TODO: Implement DX 11.1 UpdateSubresource1 which supports updating ranges with constant buffers
      buf = mComputeConstBuffer->getEntireBuffer();
      for (U32 i = 0; i < subBuffers.size(); ++i)
      {
         const ConstSubBufferDesc& desc = subBuffers[i];
         mDeviceContext->UpdateSubresource(mConstantBuffersC[i], 0, NULL, buf + desc.start, desc.size, 0);
         nbBuffers++;
      }

      mDeviceContext->CSSetConstantBuffers(0, nbBuffers, mConstantBuffersC);
   }

#ifdef TORQUE_DEBUG
   // Make sure all the constants for this buffer were assigned.
   if (mWasLost)
   {
      mComputeConstBuffer->assertUnassignedConstants(mShader->getComputeShaderFile().c_str());
   }
#endif

   // Clear the lost state.
   mWasLost = false;
}

void GFXD3D11ComputeShaderConstBuffer::onShaderReload(GFXD3D11ComputeShader* shader)
{
   AssertFatal(shader == mShader, "GFXD3D11ShaderConstBuffer::onShaderReload is hosed!");

   // release constant buffers
   for (U32 i = 0; i < CBUFFER_MAX; ++i)
   {
      SAFE_RELEASE(mConstantBuffersC[i]);
   }

   SAFE_DELETE(mComputeConstBuffer);

   AssertFatal(mComputeConstBufferLayout == shader->mComputeConstBufferLayout, "GFXD3D11ComputeShaderConstBuffer::onShaderReload is hosed!");

   mComputeConstBuffer = new GenericConstBuffer(mComputeConstBufferLayout);

   _createBuffers();

   // Set the lost state.
   mWasLost = true;
}

GFXD3D11ComputeShader::GFXD3D11ComputeShader()
{
   VECTOR_SET_ASSOCIATION(mShaderConsts);

   AssertFatal(D3D11DEVICE, "Invalid device for shader.");
   mCompShader = NULL;

   mComputeConstBufferLayout = NULL;

   if (smD3DInclude == NULL)
      smD3DInclude = new gfxD3D11ComputeInclude;

}

GFXD3D11ComputeShader::~GFXD3D11ComputeShader()
{
   for (HandleMap::Iterator i = mHandles.begin(); i != mHandles.end(); i++)
      delete i->value;

   // delete const buffer layouts
   SAFE_DELETE(mComputeConstBufferLayout);

   // release shaders
   SAFE_RELEASE(mCompShader);
}

bool GFXD3D11ComputeShader::_initCompute()
{
   SAFE_RELEASE(mCompShader);

   // Create the macro array including the system wide macros.
   const U32 macroCount = smGlobalMacros.size() + mMacros.size() + 2;
   FrameTemp<D3D_SHADER_MACRO> d3dMacros(macroCount);

   for (U32 i = 0; i < smGlobalMacros.size(); i++)
   {
      d3dMacros[i].Name = smGlobalMacros[i].name.c_str();
      d3dMacros[i].Definition = smGlobalMacros[i].value.c_str();
   }

   for (U32 i = 0; i < mMacros.size(); i++)
   {
      d3dMacros[i + smGlobalMacros.size()].Name = mMacros[i].name.c_str();
      d3dMacros[i + smGlobalMacros.size()].Definition = mMacros[i].value.c_str();
   }

   d3dMacros[macroCount - 2].Name = "TORQUE_SM";
   d3dMacros[macroCount - 2].Definition = D3D11->getShaderModel().c_str();

   memset(&d3dMacros[macroCount - 1], 0, sizeof(D3D_SHADER_MACRO));

   if (!mComputeConstBufferLayout)
      mComputeConstBufferLayout = new GFXD3D11ComputeConstBufferLayout();

   String compTarget = D3D11->getComputeShaderTarget();

   if (!Con::getBoolVariable("$shaders::forceLoadCSF", false))
   {
      if (!mComputeFile.isEmpty() && !_compileShader(mComputeFile, compTarget, d3dMacros, mComputeConstBufferLayout, mSamplerDescriptions ) );
         return false;

   }
   else
   {
      if (!_loadCompiledOutput(mComputeFile, compTarget, mComputeConstBufferLayout, mSamplerDescriptions))
      {
         if (smLogErrors)
            Con::errorf("GFXD3D11ComputeShader::initCompute - Unable to load precompiled compute shader for '%s'.", mComputeFile.getFullPath().c_str());

         return false;
      }

   }

   HandleMap::Iterator iter = mHandles.begin();
   for (; iter != mHandles.end(); iter++)
      (iter->value)->clear();

   _buildShaderConstantHandles(mComputeConstBufferLayout);

   _buildSamplerShaderConstantHandles(mSamplerDescriptions);

   // Notify any existing buffers that the buffer 
   // layouts have changed and they need to update.
   Vector<GFXShaderConstBuffer*>::iterator biter = mActiveBuffers.begin();
   for (; biter != mActiveBuffers.end(); biter++)
      ((GFXD3D11ComputeShaderConstBuffer*)(*biter))->onShaderReload(this);

   return true;
}

bool GFXD3D11ComputeShader::_compileShader(  const Torque::Path& filePath,
                                             const String& target,
                                             const D3D_SHADER_MACRO* defines,
                                             GenericConstBufferLayout* bufferLayout,
                                             Vector<GFXShaderConstDesc>& samplerDescriptions)
{
   PROFILE_SCOPE(GFXD3D11ComputeShader_CompileShader);

   using namespace Torque;

   HRESULT res = E_FAIL;
   ID3DBlob* code = NULL;
   ID3DBlob* errorBuff = NULL;
   ID3D11ShaderReflection* reflectionTable = NULL;

#ifdef TORQUE_GFX_VISUAL_DEBUG //for use with NSight, GPU Perf studio, VS graphics debugger
   U32 flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION;
#elif defined(TORQUE_DEBUG) //debug build
   U32 flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else //release build
   U32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

#ifdef D3D11_DEBUG_SPEW
   Con::printf("Compiling Shader: '%s'", filePath.getFullPath().c_str());
#endif

   // Is it an HLSL shader?
   if (filePath.getExtension().equal("hlsl", String::NoCase))
   {
      // Set this so that the D3DInclude::Open will have this 
      // information for relative paths.
      smD3DInclude->setPath(filePath.getRootAndPath());

      FileStream s;
      if (!s.open(filePath, Torque::FS::File::Read))
      {
         AssertISV(false, avar("GFXD3D11ComputeShader::initComputeShader - failed to open shader '%s'.", filePath.getFullPath().c_str()));

         if (smLogErrors)
            Con::errorf("GFXD3D11ComputeShader::_compileShader - Failed to open shader file '%s'.", filePath.getFullPath().c_str());

         return false;
      }

      // Convert the path which might have virtualized
      // mount paths to a real file system path.
      Torque::Path realPath;
      if (!FS::GetFSPath(filePath, realPath))
         realPath = filePath;

      U32 bufSize = s.getStreamSize();

      FrameAllocatorMarker fam;
      char* buffer = NULL;

      buffer = (char*)fam.alloc(bufSize + 1);
      s.read(bufSize, buffer);
      buffer[bufSize] = 0;

      res = D3DCompile(buffer, bufSize, realPath.getFullPath().c_str(), defines, smD3DInclude, "main", target, flags, 0, &code, &errorBuff);

   }

   // Is it a precompiled obj shader?
   else if (filePath.getExtension().equal("obj", String::NoCase))
   {
      FileStream  s;
      if (!s.open(filePath, Torque::FS::File::Read))
      {
         AssertISV(false, avar("GFXD3D11ComputeShader::initComputeShader - failed to open shader '%s'.", filePath.getFullPath().c_str()));

         if (smLogErrors)
            Con::errorf("GFXD3D11ComputeShader::_compileShader - Failed to open shader file '%s'.", filePath.getFullPath().c_str());

         return false;
      }

      res = D3DCreateBlob(s.getStreamSize(), &code);
      AssertISV(SUCCEEDED(res), "Unable to create buffer!");
      s.read(s.getStreamSize(), code->GetBufferPointer());
   }
   else
   {
      if (smLogErrors)
         Con::errorf("GFXD3D11Shader::_compileShader - Unsupported shader file type '%s'.", filePath.getFullPath().c_str());

      return false;
   }

   if (errorBuff)
   {
      // remove \n at end of buffer
      U8* buffPtr = (U8*)errorBuff->GetBufferPointer();
      U32 len = dStrlen((const char*)buffPtr);
      buffPtr[len - 1] = '\0';

      if (FAILED(res))
      {
         if (smLogErrors)
            Con::errorf("failed to compile shader: %s", buffPtr);
      }
      else
      {
         if (smLogWarnings)
            Con::errorf("shader compiled with warning(s): %s", buffPtr);
      }
   }
   else if (code == NULL && smLogErrors)
      Con::errorf("GFXD3D11ComputeShader::_compileShader - no compiled code produced; possibly missing file '%s'.", filePath.getFullPath().c_str());

   AssertISV(SUCCEEDED(res), "Unable to compile shader!");

   if (code != NULL)
   {
#ifndef TORQUE_SHIPPING         

      if (gDisassembleAllShaders)
      {
         ID3DBlob* disassem = NULL;
         D3DDisassemble(code->GetBufferPointer(), code->GetBufferSize(), 0, NULL, &disassem);
         mDissasembly = (const char*)disassem->GetBufferPointer();

         String filename = filePath.getFullPath();
         filename.replace(".hlsl", "_dis.txt");

         FileStream* fstream = FileStream::createAndOpen(filename, Torque::FS::File::Write);
         if (fstream)
         {
            fstream->write(mDissasembly);
            fstream->close();
            delete fstream;
         }

         SAFE_RELEASE(disassem);
      }

#endif

      if (target.compare("cs_", 3) == 0)
         res = D3D11DEVICE->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &mCompShader);

      if (FAILED(res))
      {
         AssertFatal(false, "D3D11Shader::_compilershader- failed to create shader");
      }

      if (res == S_OK) {
         HRESULT reflectionResult = D3DReflect(code->GetBufferPointer(), code->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflectionTable);
         if (FAILED(reflectionResult))
            AssertFatal(false, "D3D11ComputeShader::_compilershader - Failed to get shader reflection table interface");
      }

      if (res == S_OK)
         _getShaderConstants(reflectionTable, bufferLayout, samplerDescriptions);

#ifdef TORQUE_ENABLE_CSF_GENERATION

      // Ok, we've got a valid shader and constants, let's write them all out.
      if (!_saveCompiledOutput(filePath, code, bufferLayout) && smLogErrors)
         Con::errorf("GFXD3D11Shader::_compileShader - Unable to save shader compile output for: %s",
            filePath.getFullPath().c_str());
#endif

      if (FAILED(res) && smLogErrors)
         Con::errorf("GFXD3D11Shader::_compileShader - Unable to create shader for '%s'.", filePath.getFullPath().c_str());

   }

   //bool result = code && SUCCEEDED(res) && HasValidConstants;
   bool result = code && SUCCEEDED(res);

#ifdef TORQUE_DEBUG
   if (target.compare("cs_", 3) == 0)
   {
      String compShader = mVertexFile.getFileName();
      mCompShader->SetPrivateData(WKPDID_D3DDebugObjectName, compShader.size(), compShader.c_str());
   }
#endif

   SAFE_RELEASE(code);
   SAFE_RELEASE(reflectionTable);
   SAFE_RELEASE(errorBuff);

   return result;
}

void GFXD3D11ComputeShader::_getShaderConstants(ID3D11ShaderReflection* refTable,
                                                GenericConstBufferLayout* bufferLayoutIn,
                                                Vector<GFXShaderConstDesc>& samplerDescriptions)
{
   PROFILE_SCOPE(GFXD3D11ComputeShader_GetShaderConstants);

   AssertFatal(refTable, "NULL constant table not allowed, is this an assembly shader?");

   GFXD3D11ComputeConstBufferLayout* bufferLayout = (GFXD3D11ComputeConstBufferLayout*)bufferLayoutIn;
   Vector<ConstSubBufferDesc>& subBuffers = bufferLayout->getSubBufferDesc();
   subBuffers.clear();

   D3D11_SHADER_DESC tableDesc;
   HRESULT hr = refTable->GetDesc(&tableDesc);
   if (FAILED(hr))
   {
      AssertFatal(false, "Shader Reflection table unable to be created");
   }

   //offset for sub constant buffers
   U32 bufferOffset = 0;
   for (U32 i = 0; i < tableDesc.ConstantBuffers; i++)
   {
      ID3D11ShaderReflectionConstantBuffer* constantBuffer = refTable->GetConstantBufferByIndex(i);
      D3D11_SHADER_BUFFER_DESC constantBufferDesc;

      if (constantBuffer->GetDesc(&constantBufferDesc) == S_OK)
      {

#ifdef TORQUE_DEBUG
         AssertFatal(constantBufferDesc.Type == D3D_CT_CBUFFER, "Only scalar cbuffers supported for now.");

         if (String::compare(constantBufferDesc.Name, "$Globals") != 0 && String::compare(constantBufferDesc.Name, "$Params") != 0)
            AssertFatal(false, "Only $Global and $Params cbuffer supported for now.");
#endif
#ifdef D3D11_DEBUG_SPEW
         Con::printf("Constant Buffer Name: %s", constantBufferDesc.Name);
#endif 

         for (U32 j = 0; j < constantBufferDesc.Variables; j++)
         {
            GFXShaderConstDesc desc;
            ID3D11ShaderReflectionVariable* variable = constantBuffer->GetVariableByIndex(j);
            D3D11_SHADER_VARIABLE_DESC variableDesc;
            D3D11_SHADER_TYPE_DESC variableTypeDesc;

            variable->GetDesc(&variableDesc);

            ID3D11ShaderReflectionType* variableType = variable->GetType();

            variableType->GetDesc(&variableTypeDesc);
            desc.name = String(variableDesc.Name);
            // Prepend a "$" if it doesn't exist.  Just to make things consistent.
            if (desc.name.find("$") != 0)
               desc.name = String::ToString("$%s", desc.name.c_str());

            bool unusedVar = variableDesc.uFlags & D3D_SVF_USED ? false : true;

            if (variableTypeDesc.Elements == 0)
               desc.arraySize = 1;
            else
               desc.arraySize = variableTypeDesc.Elements;

#ifdef D3D11_DEBUG_SPEW
            Con::printf("Variable Name %s:, offset: %d, size: %d, constantDesc.Elements: %d", desc.name.c_str(), variableDesc.StartOffset, variableDesc.Size, desc.arraySize);
#endif           
            if (_convertShaderVariable(variableTypeDesc, desc))
            {
               //The HLSL compiler for 4.0 and above doesn't strip out unused registered constants. We'll have to do it manually
               if (!unusedVar)
               {
                  mShaderConsts.push_back(desc);
                  U32 alignBytes = getAlignmentValue(desc.constType);
                  U32 paramSize = variableDesc.Size;
                  bufferLayout->addParameter(desc.name,
                     desc.constType,
                     variableDesc.StartOffset + bufferOffset,
                     paramSize,
                     desc.arraySize,
                     alignBytes);

               } //unusedVar
            } //_convertShaderVariable
         } //constantBufferDesc.Variables

         // fill out our const sub buffer sizes etc
         ConstSubBufferDesc subBufferDesc;
         subBufferDesc.size = constantBufferDesc.Size;
         subBufferDesc.start = bufferOffset;
         subBuffers.push_back(subBufferDesc);
         // increase our bufferOffset by the constant buffer size
         bufferOffset += constantBufferDesc.Size;

      }
      else
         AssertFatal(false, "Unable to get shader constant description! (may need more elements of constantDesc");
   }

   // Set buffer size to the aligned size
   bufferLayout->setSize(bufferOffset);


   //get the sampler descriptions from the resource binding description
   U32 resourceCount = tableDesc.BoundResources;
   for (U32 i = 0; i < resourceCount; i++)
   {
      GFXShaderConstDesc desc;
      D3D11_SHADER_INPUT_BIND_DESC bindDesc;
      refTable->GetResourceBindingDesc(i, &bindDesc);

      switch (bindDesc.Type)
      {
      case D3D_SIT_SAMPLER:
         // Prepend a "$" if it doesn't exist.  Just to make things consistent.
         desc.name = String(bindDesc.Name);
         if (desc.name.find("$") != 0)
            desc.name = String::ToString("$%s", desc.name.c_str());
         desc.constType = GFXSCT_Sampler;
         desc.arraySize = bindDesc.BindPoint;
         samplerDescriptions.push_back(desc);
         break;

      }
   }
}

bool GFXD3D11ComputeShader::_convertShaderVariable(const D3D11_SHADER_TYPE_DESC& typeDesc, GFXShaderConstDesc& desc)
{
   switch (typeDesc.Type)
   {
   case D3D_SVT_INT:
   {
      switch (typeDesc.Class)
      {
      case D3D_SVC_SCALAR:
         desc.constType = GFXSCT_Int;
         break;
      case D3D_SVC_VECTOR:
      {
         switch (typeDesc.Columns)
         {
         case 1:
            desc.constType = GFXSCT_Int;
            break;
         case 2:
            desc.constType = GFXSCT_Int2;
            break;
         case 3:
            desc.constType = GFXSCT_Int3;
            break;
         case 4:
            desc.constType = GFXSCT_Int4;
            break;
         }
      }
      break;
      }
      break;
   }
   case D3D_SVT_FLOAT:
   {
      switch (typeDesc.Class)
      {
      case D3D_SVC_SCALAR:
         desc.constType = GFXSCT_Float;
         break;
      case D3D_SVC_VECTOR:
      {
         switch (typeDesc.Columns)
         {
         case 1:
            desc.constType = GFXSCT_Float;
            break;
         case 2:
            desc.constType = GFXSCT_Float2;
            break;
         case 3:
            desc.constType = GFXSCT_Float3;
            break;
         case 4:
            desc.constType = GFXSCT_Float4;
            break;
         }
      }
      break;
      case D3D_SVC_MATRIX_ROWS:
      case D3D_SVC_MATRIX_COLUMNS:
      {
         switch (typeDesc.Rows)
         {
         case 3:
            desc.constType = typeDesc.Columns == 4 ? GFXSCT_Float3x4 : GFXSCT_Float3x3;
            break;
         case 4:
            desc.constType = typeDesc.Columns == 3 ? GFXSCT_Float4x3 : GFXSCT_Float4x4;
            break;
         }
      }
      break;
      case D3D_SVC_OBJECT:
      case D3D_SVC_STRUCT:
         return false;
      }
   }
   break;

   default:
      AssertFatal(false, "Unknown shader constant class enum");
      break;
   }

   return true;
}

const U32 GFXD3D11ComputeShader::smCompiledShaderTag = MakeFourCC('t', 'c', 's', 'f');

bool GFXD3D11ComputeShader::_saveCompiledOutput(const Torque::Path& filePath,
                                                ID3DBlob* buffer,
                                                GenericConstBufferLayout* bufferLayout,
                                                Vector<GFXShaderConstDesc>& samplerDescriptions)
{
   Torque::Path outputPath(filePath);
   outputPath.setExtension("csf");     // "C"ompiled "S"hader "F"ile (fancy!)

   FileStream f;
   if (!f.open(outputPath, Torque::FS::File::Write))
      return false;
   if (!f.write(smCompiledShaderTag))
      return false;
   // We could reverse engineer the structure in the compiled output, but this
   // is a bit easier because we can just read it into the struct that we want.
   if (!bufferLayout->write(&f))
      return false;

   U32 bufferSize = buffer->GetBufferSize();
   if (!f.write(bufferSize))
      return false;
   if (!f.write(bufferSize, buffer->GetBufferPointer()))
      return false;

   // Write out sampler descriptions.

   f.write(samplerDescriptions.size());

   for (U32 i = 0; i < samplerDescriptions.size(); i++)
   {
      f.write(samplerDescriptions[i].name);
      f.write((U32)(samplerDescriptions[i].constType));
      f.write(samplerDescriptions[i].arraySize);
   }

   f.close();

   return true;
}

bool GFXD3D11ComputeShader::_loadCompiledOutput(const Torque::Path& filePath,
                                                const String& target,
                                                GenericConstBufferLayout* bufferLayout,
                                                Vector<GFXShaderConstDesc>& samplerDescriptions)
{
   Torque::Path outputPath(filePath);
   outputPath.setExtension("csf");     // "C"ompiled "S"hader "F"ile (fancy!)

   FileStream f;
   if (!f.open(outputPath, Torque::FS::File::Read))
      return false;
   U32 fileTag;
   if (!f.read(&fileTag))
      return false;
   if (fileTag != smCompiledShaderTag)
      return false;
   if (!bufferLayout->read(&f))
      return false;
   U32 bufferSize;
   if (!f.read(&bufferSize))
      return false;
   U32 waterMark = FrameAllocator::getWaterMark();
   DWORD* buffer = static_cast<DWORD*>(FrameAllocator::alloc(bufferSize));
   if (!f.read(bufferSize, buffer))
      return false;

   // Read sampler descriptions.

   U32 samplerCount;
   f.read(&samplerCount);

   for (U32 i = 0; i < samplerCount; i++)
   {
      GFXShaderConstDesc samplerDesc;
      f.read(&(samplerDesc.name));
      f.read((U32*)&(samplerDesc.constType));
      f.read(&(samplerDesc.arraySize));

      samplerDescriptions.push_back(samplerDesc);
   }

   f.close();

   HRESULT res;
   if (target.compare("ps_", 3) == 0)
      res = D3D11DEVICE->CreateComputeShader(buffer, bufferSize, NULL, &mCompShader);

   AssertFatal(SUCCEEDED(res), "Unable to load shader!");

   FrameAllocator::setWaterMark(waterMark);
   return SUCCEEDED(res);
}

void GFXD3D11ComputeShader::_buildShaderConstantHandles(GenericConstBufferLayout* layout)
{
   for (U32 i = 0; i < layout->getParameterCount(); i++)
   {
      GenericConstBufferLayout::ParamDesc pd;
      layout->getDesc(i, pd);

      GFXD3D11ComputeShaderConstHandle* handle;
      HandleMap::Iterator j = mHandles.find(pd.name);

      if (j != mHandles.end())
      {
         handle = j->value;
         handle->mShader = this;
         handle->setValid(true);
      }
      else
      {
         handle = new GFXD3D11ComputeShaderConstHandle();
         handle->mShader = this;
         mHandles[pd.name] = handle;
         handle->setValid(true);
      }

      handle->mComputeConstant = true;
      handle->mComputeHandle = pd;
   }
}

void GFXD3D11ComputeShader::_buildSamplerShaderConstantHandles(Vector<GFXShaderConstDesc>& samplerDescriptions)
{
   Vector<GFXShaderConstDesc>::iterator iter = samplerDescriptions.begin();
   for (; iter != samplerDescriptions.end(); iter++)
   {
      const GFXShaderConstDesc& desc = *iter;

      AssertFatal(desc.constType == GFXSCT_Sampler ||
         desc.constType == GFXSCT_SamplerCube ||
         desc.constType == GFXSCT_SamplerCubeArray ||
         desc.constType == GFXSCT_SamplerTextureArray,
         "GFXD3D11Shader::_buildSamplerShaderConstantHandles - Invalid samplerDescription type!");

      GFXD3D11ComputeShaderConstHandle* handle;
      HandleMap::Iterator j = mHandles.find(desc.name);

      if (j != mHandles.end())
         handle = j->value;
      else
      {
         handle = new GFXD3D11ComputeShaderConstHandle();
         mHandles[desc.name] = handle;
      }

      handle->mShader = this;
      handle->setValid(true);
      handle->mComputeConstant = true;
      handle->mComputeHandle.name = desc.name;
      handle->mComputeHandle.constType = desc.constType;
      handle->mComputeHandle.offset = desc.arraySize;
   }
}

GFXShaderConstBufferRef GFXD3D11ComputeShader::allocConstBuffer()
{
   if (mComputeConstBufferLayout)
   {
      GFXD3D11ComputeShaderConstBuffer* buffer;
      buffer = new GFXD3D11ComputeShaderConstBuffer(this, mComputeConstBufferLayout);

      mActiveBuffers.push_back(buffer);
      buffer->registerResourceWithDevice(getOwningDevice());
      return buffer;
   }

   return NULL;
}

/// Returns a shader constant handle for name, if the variable doesn't exist NULL is returned.
GFXShaderConstHandle* GFXD3D11ComputeShader::getShaderConstHandle(const String& name)
{
   HandleMap::Iterator i = mHandles.find(name);
   if (i != mHandles.end())
   {
      return i->value;
   }
   else
   {
      GFXD3D11ComputeShaderConstHandle* handle = new GFXD3D11ComputeShaderConstHandle();
      handle->setValid(false);
      handle->mShader = this;
      mHandles[name] = handle;

      return handle;
   }
}

GFXShaderConstHandle* GFXD3D11ComputeShader::findShaderConstHandle(const String& name)
{
   HandleMap::Iterator i = mHandles.find(name);
   if (i != mHandles.end())
      return i->value;
   else
   {
      return NULL;
   }
}

const Vector<GFXShaderConstDesc>& GFXD3D11ComputeShader::getShaderConstDesc() const
{
   return mShaderConsts;
}

U32 GFXD3D11ComputeShader::getAlignmentValue(const GFXShaderConstType constType) const
{
   const U32 mRowSizeF = 16;
   const U32 mRowSizeI = 16;

   switch (constType)
   {
   case GFXSCT_Float:
   case GFXSCT_Float2:
   case GFXSCT_Float3:
   case GFXSCT_Float4:
      return mRowSizeF;
      break;
      // Matrices
   case GFXSCT_Float2x2:
      return mRowSizeF * 2;
      break;
   case GFXSCT_Float3x3:
      return mRowSizeF * 3;
      break;
   case GFXSCT_Float4x3:
      return mRowSizeF * 3;
      break;
   case GFXSCT_Float4x4:
      return mRowSizeF * 4;
      break;
      //// Scalar
   case GFXSCT_Int:
   case GFXSCT_Int2:
   case GFXSCT_Int3:
   case GFXSCT_Int4:
      return mRowSizeI;
      break;
   default:
      AssertFatal(false, "Unsupported type!");
      return 0;
      break;
   }
}

void GFXD3D11ComputeShader::zombify()
{
   // Shaders don't need zombification
}

void GFXD3D11ComputeShader::resurrect()
{
   // Shaders are never zombies, and therefore don't have to be brought back.
}

void GFXD3D11ComputeShader::setComputeInput(U32 slot, GFXTextureObject* texture)
{
   GFXD3D11TextureObject* tex = static_cast<GFXD3D11TextureObject*>(texture);

   // create a byte pointer..
   byte* texData;
   U32 textureDataSize;

   U32 bytesPerPixel = tex->getFormatByteSize();
   bool Tex3d = false;

   if (tex->getDepth() > 0)
      Tex3d = true;

   DXGI_FORMAT d3dTextureFormat = GFXD3D11TextureFormat[tex->getFormat()];

   // are we 3d?
   if (Tex3d)
   {
      D3D11_TEXTURE3D_DESC desc;
      static_cast<ID3D11Texture3D*>(tex->getResource())->GetDesc(&desc);
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

      ID3D11Texture3D* tempTex = NULL;
      HRESULT hr = D3D11DEVICE->CreateTexture3D(&desc, NULL, &tempTex);
      if (FAILED(hr))
      {
         Con::errorf("GFXD3D11ComputeShader::setComputeInput::3D - Failed to create staging texture!");
         return;
      }

      D3D11_MAPPED_SUBRESOURCE mappedResource;
      hr = D3D11DEVICECONTEXT->Map(tempTex, 0, D3D11_MAP_READ, 0, &mappedResource);
      if (FAILED(hr))
      {
         Con::errorf("GFXD3D11ComputeShader::setComputeInput::3D - Failed to create mapped resource!");
         return;
      }

      textureDataSize = mappedResource.RowPitch * desc.Height;

      dMemset(texData, 0, textureDataSize);

      dMemcpy(texData, mappedResource.pData, textureDataSize);

      D3D11DEVICECONTEXT->Unmap(tempTex, 0);
   }
   else // must be 2d
   {
      D3D11_TEXTURE2D_DESC desc;
      static_cast<ID3D11Texture2D*>(tex->getResource())->GetDesc(&desc);
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

      ID3D11Texture2D* tempTex = NULL;
      HRESULT hr = D3D11DEVICE->CreateTexture2D(&desc, NULL, &tempTex);
      if (FAILED(hr))
      {
         Con::errorf("GFXD3D11ComputeShader::setComputeInput::2D - Failed to create staging texture!");
         return;
      }

      D3D11_MAPPED_SUBRESOURCE mappedResource;
      hr = D3D11DEVICECONTEXT->Map(tempTex, 0, D3D11_MAP_READ, 0, &mappedResource);
      if (FAILED(hr))
      {
         Con::errorf("GFXD3D11ComputeShader::setComputeInput::3D - Failed to create mapped resource!");
         return;
      }

      textureDataSize = mappedResource.RowPitch * desc.Height;

      dMemset(texData, 0, textureDataSize);

      dMemcpy(texData, mappedResource.pData, textureDataSize);

      D3D11DEVICECONTEXT->Unmap(tempTex, 0);
   }

   if (texData)
   {
      ID3D11Buffer* csDataBuffer;
      D3D11_BUFFER_DESC descCShader;
      ZeroMemory(&descCShader, sizeof(descCShader));
      descCShader.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
      descCShader.ByteWidth = textureDataSize;
      descCShader.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
      descCShader.StructureByteStride = bytesPerPixel;

      D3D11_SUBRESOURCE_DATA initData;
      initData.pSysMem = texData;

      HRESULT hr = D3D11DEVICE->CreateBuffer(&descCShader, &initData, &csDataBuffer);
      if (FAILED(hr))
      {
         Con::errorf("GFXD3D11ComputeShader::setComputeInput- Failed to create buffer!");
         return;
      }

      D3D11_BUFFER_DESC descBuf;
      ZeroMemory(&descBuf, sizeof(descBuf));
      csDataBuffer->GetDesc(&descBuf);

      D3D11_SHADER_RESOURCE_VIEW_DESC descView;
      ZeroMemory(&descView, sizeof(descView));
      descView.ViewDimension = Tex3d ? D3D11_SRV_DIMENSION_TEXTURE3D : D3D11_SRV_DIMENSION_TEXTURE2D;
      descView.BufferEx.FirstElement = 0;

      descView.Format = d3dTextureFormat;
      descView.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;

      ID3D11ShaderResourceView* mCsResourecView;

      hr = D3D11DEVICE->CreateShaderResourceView(csDataBuffer, &descView, &mCsResourecView);
      if (FAILED(hr))
      {
         Con::errorf("GFXD3D11ComputeShader::setComputeInput- Failed to create shader resource view!");
         return;
      }

      D3D11DEVICECONTEXT->CSSetShaderResources(slot, 0, &mCsResourecView);
   }

}
