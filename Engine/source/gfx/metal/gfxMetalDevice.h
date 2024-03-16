//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
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

#ifndef _GFXMETALDEVICE_H_
#define _GFXMETALDEVICE_H_

#include "platform/platform.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxInit.h"
#include "SDL.h"

#ifndef NS_PRIVATE_IMPLEMENTATION
#define NS_PRIVATE_IMPLEMENTATION
#endif
#ifndef CA_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#endif
#ifndef MTL_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#endif
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

class GFXMETALDevice : public GFXDevice
{
   typedef GFXDevice Parent;
   static GFXAdapter::CreateDeviceInstanceDelegate mCreateDeviceInstance;
   U32 mAdapterIndex;
public:
   // we probably dont even need this adapter index.
   GFXMETALDevice(U32 adapterIndex);
   virtual ~GFXMETALDevice();
   
   static void enumerateAdapters(Vector<GFXAdapter*> &adapterList);
   static GFXDevice *createInstance(U32 adapterIndex);
   
   virtual void init(const GFXVideoMode &mode, PlatformWindow *window = NULL) override;
   
   virtual void activate() {}
   virtual void deactivate() {}
   virtual GFXAdapterType getAdapterType() override {return Metal;}
   virtual void enumerateVideoModes() override;
   virtual U32 getTotalVideoMemory();
   
   virtual GFXCubemap* createCubemap() override;
   virtual GFXCubemapArray* createCubemapArray() override;
   virtual GFXTextureArray* createTextureArray() override;
   
   virtual GFXTextureTarget* allocRenderToTextureTarget(bool genMips = true) override;
   virtual GFXWindowTarget* allocWindowTarget(PlatformWindow* window) override;
   
   void enterDebugEvent(ColorI color, const char *name) override;
   
   void leaveDebugEvent() override;
   
   void setDebugMarker(ColorI color, const char *name) override;
   
   GFXFormat selectSupportedFormat(GFXTextureProfile *profile, const Vector<GFXFormat> &formats, bool texture, bool mustblend, bool mustfilter) override;
   
   GFXStateBlockRef createStateBlockInternal(const GFXStateBlockDesc &desc) override;
   
   void setStateBlockInternal(GFXStateBlock *block, bool force) override;
   
   void setShaderConstBufferInternal(GFXShaderConstBuffer *buffer) override;
   
   void setTextureInternal(U32 textureUnit, const GFXTextureObject *texture) override;
   
   bool beginSceneInternal() override;
   
   void endSceneInternal() override;
   
   void initStates() override;
   
   GFXVertexBuffer *allocVertexBuffer(U32 numVerts, const GFXVertexFormat *vertexFormat, U32 vertSize, GFXBufferType bufferType, void *data = __null) override;
   
   GFXVertexDecl *allocVertexDecl(const GFXVertexFormat *vertexFormat) override;
   
   void setVertexDecl(const GFXVertexDecl *decl) override;
   
   void setVertexStream(U32 stream, GFXVertexBuffer *buffer) override;
   
   void setVertexStreamFrequency(U32 stream, U32 frequency) override;
   
   GFXPrimitiveBuffer *allocPrimitiveBuffer(U32 numIndices, U32 numPrimitives, GFXBufferType bufferType, void *data = __null) override;
   
   void _updateRenderTargets() override;
   
   F32 getPixelShaderVersion() const override;
   
   void setPixelShaderVersion(F32 version) override;
   
   U32 getNumSamplers() const override;
   
   U32 getNumRenderTargets() const override;
   
   GFXShader *createShader() override;
   
   void copyResource(GFXTextureObject *pDst, GFXCubemap *pSrc, const U32 face) override;
   
   void clear(U32 flags, const LinearColorF &color, F32 z, U32 stencil) override;
   
   void clearColorAttachment(const U32 attachment, const LinearColorF &color) override;
   
   void drawPrimitive(GFXPrimitiveType primType, U32 vertexStart, U32 primitiveCount) override;
   
   void drawIndexedPrimitive(GFXPrimitiveType primType, U32 startVertex, U32 minIndex, U32 numVerts, U32 startIndex, U32 primitiveCount) override;
   
   GFXFence *createFence() override;
   
   void setClipRect(const RectI &rect) override;
   
   const RectI &getClipRect() const override;
   
   F32 getFillConventionOffset() const override;
   
   U32 getMaxDynamicVerts() override;
   
   U32 getMaxDynamicIndices() override;
   
   virtual void _updateRenerTargets();
   
private:
   CA::MetalLayer* layer;
};

#endif
