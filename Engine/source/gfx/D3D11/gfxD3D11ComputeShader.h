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

#ifndef _GFXD3D11COMPUTESHADER_H_
#define _GFXD3D11COMPUTESHADER_H_

#include "core/util/path.h"
#include "core/util/tDictionary.h"
#include "gfx/gfxShader.h"
#include "gfx/gfxResource.h"
#include "gfx/genericConstBuffer.h"
#include "gfx/D3D11/gfxD3D11Device.h"

// forward declaration
class GFXD3D11ComputeShader;

/////////////////// Constant Buffers /////////////////////////////
// Maximum number of CBuffers ($Globals & $Params)
//const U32 CBUFFER_MAX = 2;
//
//struct ConstSubBufferDesc
//{
//   U32 start;
//   U32 size;
//
//   ConstSubBufferDesc() : start(0), size(0) {}
//};

class GFXD3D11ComputeConstBufferLayout : public GenericConstBufferLayout
{
public:
   GFXD3D11ComputeConstBufferLayout();
   /// Get our constant sub buffer data
   Vector<ConstSubBufferDesc>& getSubBufferDesc() { return mSubBuffers; }

   /// We need to manually set the size due to D3D11 alignment
   void setSize(U32 size) { mBufferSize = size; }

   /// Set a parameter, given a base pointer
   virtual bool set(const ParamDesc& pd, const GFXShaderConstType constType, const U32 size, const void* data, U8* basePointer);

protected:
   /// Set a matrix, given a base pointer
   virtual bool setMatrix(const ParamDesc& pd, const GFXShaderConstType constType, const U32 size, const void* data, U8* basePointer);

   Vector<ConstSubBufferDesc> mSubBuffers;
};

class GFXD3D11ComputeShaderConstHandle : public GFXShaderConstHandle
{
public:

   // GFXShaderConstHandle
   const String& getName() const;
   GFXShaderConstType getType() const;
   U32 getArraySize() const;

   WeakRefPtr<GFXD3D11ComputeShader> mShader;

   bool mComputeConstant;
   GenericConstBufferLayout::ParamDesc mComputeHandle;

   void setValid(bool valid) { mValid = valid; }
   S32 getSamplerRegister() const;

   // Returns true if this is a handle to a sampler register.
   bool isSampler() const
   {
      return (mComputeConstant && mComputeHandle.constType >= GFXSCT_Sampler);
   }

   /// Restore to uninitialized state.
   void clear()
   {
      mShader = NULL;
      mComputeConstant = false;
      mComputeHandle.clear();

      mValid = false;
   }

   GFXD3D11ComputeShaderConstHandle();
};

/// The D3D11 implementation of a shader constant buffer.
class GFXD3D11ComputeShaderConstBuffer : public GFXShaderConstBuffer
{
   friend class GFXD3D11ComputeShader;
   // Cache device context
   ID3D11DeviceContext* mDeviceContext;

public:

   GFXD3D11ComputeShaderConstBuffer(GFXD3D11ComputeShader* shader,
      GFXD3D11ComputeConstBufferLayout* computeLayout);

   virtual ~GFXD3D11ComputeShaderConstBuffer();

   /// Called by GFXD3D11Device to activate this buffer.
   /// @param mPrevShaderBuffer The previously active buffer
   void activate(GFXD3D11ComputeShaderConstBuffer* prevShaderBuffer);

   /// Used internally by GXD3D11ShaderConstBuffer to determine if it's dirty.
   bool isDirty();

   /// Called from GFXD3D11Shader when constants have changed and need
   /// to be the shader this buffer references is reloaded.
   void onShaderReload(GFXD3D11ComputeShader* shader);

   // GFXShaderConstBuffer
   virtual GFXShader* getShader();
   virtual void set(GFXShaderConstHandle* handle, const F32 fv);
   virtual void set(GFXShaderConstHandle* handle, const Point2F& fv);
   virtual void set(GFXShaderConstHandle* handle, const Point3F& fv);
   virtual void set(GFXShaderConstHandle* handle, const Point4F& fv);
   virtual void set(GFXShaderConstHandle* handle, const PlaneF& fv);
   virtual void set(GFXShaderConstHandle* handle, const LinearColorF& fv);
   virtual void set(GFXShaderConstHandle* handle, const S32 f);
   virtual void set(GFXShaderConstHandle* handle, const Point2I& fv);
   virtual void set(GFXShaderConstHandle* handle, const Point3I& fv);
   virtual void set(GFXShaderConstHandle* handle, const Point4I& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<F32>& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<Point2F>& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<Point3F>& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<Point4F>& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<S32>& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<Point2I>& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<Point3I>& fv);
   virtual void set(GFXShaderConstHandle* handle, const AlignedArray<Point4I>& fv);
   virtual void set(GFXShaderConstHandle* handle, const MatrixF& mat, const GFXShaderConstType matType = GFXSCT_Float4x4);
   virtual void set(GFXShaderConstHandle* handle, const MatrixF* mat, const U32 arraySize, const GFXShaderConstType matrixType = GFXSCT_Float4x4);

   // GFXResource
   virtual const String describeSelf() const;
   virtual void zombify();
   virtual void resurrect();

protected:

   void _createBuffers();

   template<class T>
   inline void SET_CONSTANT(GFXShaderConstHandle* handle,
      const T& fv,
      GenericConstBuffer* cBuffer);

   // Constant buffers
   ID3D11Buffer* mConstantBuffersC[CBUFFER_MAX];

   /// We keep a weak reference to the shader 
   /// because it will often be deleted.
   WeakRefPtr<GFXD3D11ComputeShader> mShader;

   //Compute
   GFXD3D11ComputeConstBufferLayout* mComputeConstBufferLayout;
   GenericConstBuffer* mComputeConstBuffer;
};


class gfxD3D11ComputeInclude;
typedef StrongRefPtr<gfxD3D11ComputeInclude> gfxD3DComputeIncludeRef;
/////////////////// GFXShader implementation /////////////////////////////

class GFXD3D11ComputeShader : public GFXShader
{
   friend class GFXD3D11Device;
   friend class GFXD3D11ComputeShaderConstBuffer;

public:
   typedef Map<String, GFXD3D11ComputeShaderConstHandle*> HandleMap;

   GFXD3D11ComputeShader();
   virtual ~GFXD3D11ComputeShader();

   // GFXShader
   virtual GFXShaderConstBufferRef allocConstBuffer();
   virtual const Vector<GFXShaderConstDesc>& getShaderConstDesc() const;
   virtual GFXShaderConstHandle* getShaderConstHandle(const String& name);
   virtual GFXShaderConstHandle* findShaderConstHandle(const String& name);
   virtual U32 getAlignmentValue(const GFXShaderConstType constType) const;
   virtual bool getDisassembly(String& outStr) const;

   // GFXResource
   virtual void zombify();
   virtual void resurrect();

   void setComputeInput(U32 slot, GFXTextureObject* texture);

protected:
   // This is not the other type of shader, shouldn't be here.
   virtual bool _init() { return false; }

   /// This is a compute shader
   virtual bool _initCompute();

   static const U32 smCompiledShaderTag;

   ID3D11ComputeShader* mCompShader;

   GFXD3D11ComputeConstBufferLayout* mComputeConstBufferLayout;

   static gfxD3DComputeIncludeRef smD3DInclude;

   HandleMap mHandles;

   /// The shader disassembly from DX when this shader is compiled.
   /// We only store this data in non-release builds.
   String mDissasembly;

   /// Vector of sampler type descriptions consolidated from _compileShader.
   Vector<GFXShaderConstDesc> mSamplerDescriptions;

   /// Vector of descriptions (consolidated for the getShaderConstDesc call)
   Vector<GFXShaderConstDesc> mShaderConsts;

   // These two functions are used when compiling shaders from hlsl
   virtual bool _compileShader(const Torque::Path& filePath,
      const String& target,
      const D3D_SHADER_MACRO* defines,
      GenericConstBufferLayout* bufferLayout,
      Vector<GFXShaderConstDesc>& samplerDescriptions);

   void _getShaderConstants(ID3D11ShaderReflection* refTable,
      GenericConstBufferLayout* bufferLayout,
      Vector<GFXShaderConstDesc>& samplerDescriptions);

   bool _convertShaderVariable(const D3D11_SHADER_TYPE_DESC& typeDesc, GFXShaderConstDesc& desc);


   bool _saveCompiledOutput(const Torque::Path& filePath,
      ID3DBlob* buffer,
      GenericConstBufferLayout* bufferLayout,
      Vector<GFXShaderConstDesc>& samplerDescriptions);

   // Loads precompiled shaders
   bool _loadCompiledOutput(const Torque::Path& filePath,
      const String& target,
      GenericConstBufferLayout* bufferLayoutF,
      Vector<GFXShaderConstDesc>& samplerDescriptions);


   // This is used in both cases
   virtual void _buildShaderConstantHandles(GenericConstBufferLayout* layout);

   virtual void _buildSamplerShaderConstantHandles(Vector<GFXShaderConstDesc>& samplerDescriptions);
};

inline bool GFXD3D11ComputeShader::getDisassembly(String& outStr) const
{
   outStr = mDissasembly;
   return (outStr.isNotEmpty());
}


#endif // !_GFXD3D11COMPUTESHADER_H_
