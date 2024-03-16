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

#include "platform/platform.h"
#include "gfx/metal/gfxMetalDevice.h"

#include "gfx/gfxCubemap.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/primBuilder.h"
#include "console/console.h"
#include "materials/shaderData.h"
#include "shaderGen/shaderGen.h"

#if defined( TORQUE_SDL ) && !defined( TORQUE_DEDICATED )

#include "gfx/screenshot.h"
#include "windowManager/sdl/sdlWindow.h"
#include "SDL.h"

void EnumerateVideoModesMetal(Vector<GFXVideoMode>& outModes)
{
   int count = SDL_GetNumDisplayModes( 0 );
   if( count < 0)
   {
      AssertFatal(false, "");
      return;
   }
   
   SDL_DisplayMode mode;
   for(int i = 0; i < count; ++i)
   {
      SDL_GetDisplayMode( 0, i, &mode);
      GFXVideoMode outMode;
      outMode.resolution.set( mode.w, mode.h );
      outMode.refreshRate = mode.refresh_rate;

      // BBP = 32 for some reason the engine knows it should be 32, but then we
      // add some extra code to break what the engine knows.
      //outMode.bitDepth = SDL_BYTESPERPIXEL( mode.format );     // sets bitdepths to 4
      //outMode.bitDepth = SDL_BITSPERPIXEL(mode.format);        // sets bitdepth to 24
      
      // hardcoded magic numbers ftw
      // This value is hardcoded in DX, probably to avoid the shenanigans going on here
      outMode.bitDepth = 32;

      outMode.wideScreen = (mode.w / mode.h) > (4 / 3);
      outMode.fullScreen = true;
      
      outModes.push_back( outMode );
   }
}

void GFXMETALDevice::enumerateAdapters(Vector<GFXAdapter *> &adapterList)
{
#ifdef TORQUE_TESTS_ENABLED
   return;
#endif
   
   AssertFatal(SDL_WasInit(SDL_INIT_VIDEO), "Enumerate Metal Devices: SDL was not initialized");
   
   // Create a dummy window with metal.
  SDL_Window* tempWindow =  SDL_CreateWindow(
       "",                                // window title
       SDL_WINDOWPOS_UNDEFINED,           // initial x position
       SDL_WINDOWPOS_UNDEFINED,           // initial y position
       640,                               // width, in pixels
       480,                               // height, in pixels
       SDL_WINDOW_METAL | SDL_WINDOW_HIDDEN // flags - see below
   );
   
   if(!tempWindow)
   {
      const char* err = SDL_GetError();
      Con::printf(err);
      AssertFatal(0, err);
      return;
   }
   
   GFXAdapter* toAdd = new GFXAdapter;
   toAdd->mIndex = 0;
   
   // we just create this device here to get the name.
   MTL::Device* _tempDevice = MTL::CreateSystemDefaultDevice();
   
   dStrcpy(toAdd->mName, _tempDevice->name()->cString(NS::ASCIIStringEncoding), GFXAdapter::MaxAdapterNameLen);
   dStrcat(toAdd->mName, " Metal", GFXAdapter::MaxAdapterNameLen);
   
   // release it... that seemed useless didnt it....
   _tempDevice->release();
   
   toAdd->mType = Metal;
   
   toAdd->mShaderModel = 0.f;
   toAdd->mCreateDeviceInstanceDelegate = mCreateDeviceInstance;

   // Enumerate all available resolutions:
   EnumerateVideoModesMetal(toAdd->mAvailableModes);

   // Add to the list of available adapters.
   adapterList.push_back(toAdd);
   
   // Cleanup window
   SDL_DestroyWindow( tempWindow );
}

void GFXMETALDevice::init(const GFXVideoMode &mode, PlatformWindow *window)
{
   
}

GFXWindowTarget* GFXMETALDevice::allocWindowTarget(PlatformWindow *window)
{
   
}

void GFXMETALDevice::enumerateVideoModes() {
   mVideoModes.clear();
   EnumerateVideoModesMetal(mVideoModes);
}

#endif


GFXAdapter::CreateDeviceInstanceDelegate GFXMETALDevice::mCreateDeviceInstance(GFXMETALDevice::createInstance);

GFXDevice *GFXMETALDevice::createInstance(U32 adapterIndex)
{
   return new GFXMETALDevice(adapterIndex);
}

GFXMETALDevice::GFXMETALDevice( U32 adapterIndex )
{
   
}

bool GFXMETALDevice::beginSceneInternal() {
   mCanCurrentlyRender = true;
   return true;
}

void GFXMETALDevice::endSceneInternal() {
   mCanCurrentlyRender = false;
}

GFXMETALDevice::~GFXMETALDevice() noexcept {
}


GFXCubemapArray *GFXMETALDevice::createCubemapArray() {
}

GFXTextureArray *GFXMETALDevice::createTextureArray() {
}

GFXTextureTarget *GFXMETALDevice::allocRenderToTextureTarget(bool genMips) {
}

void GFXMETALDevice::enterDebugEvent(ColorI color, const char *name) {
}

void GFXMETALDevice::leaveDebugEvent() {
}

void GFXMETALDevice::setDebugMarker(ColorI color, const char *name) {
}

GFXFormat GFXMETALDevice::selectSupportedFormat(GFXTextureProfile *profile, const Vector<GFXFormat> &formats, bool texture, bool mustblend, bool mustfilter) {
}

GFXStateBlockRef GFXMETALDevice::createStateBlockInternal(const GFXStateBlockDesc &desc) {
}

void GFXMETALDevice::setStateBlockInternal(GFXStateBlock *block, bool force) {
}

void GFXMETALDevice::setShaderConstBufferInternal(GFXShaderConstBuffer *buffer) {
}

void GFXMETALDevice::setTextureInternal(U32 textureUnit, const GFXTextureObject *texture) {
}

void GFXMETALDevice::initStates() {
}

GFXVertexBuffer *GFXMETALDevice::allocVertexBuffer(U32 numVerts,
                                                   const GFXVertexFormat *vertexFormat,
                                                   U32 vertSize,
                                                   GFXBufferType bufferType, void *data) {
}

GFXVertexDecl *GFXMETALDevice::allocVertexDecl(const GFXVertexFormat *vertexFormat) {
}

void GFXMETALDevice::setVertexDecl(const GFXVertexDecl *decl) {
}

void GFXMETALDevice::setVertexStream(U32 stream, GFXVertexBuffer *buffer) {
}

void GFXMETALDevice::setVertexStreamFrequency(U32 stream, U32 frequency) {
}

GFXPrimitiveBuffer *GFXMETALDevice::allocPrimitiveBuffer(U32 numIndices,
                                                         U32 numPrimitives,
                                                         GFXBufferType bufferType, void *data) {
}

void GFXMETALDevice::_updateRenderTargets() {
}

F32 GFXMETALDevice::getPixelShaderVersion() const {
}

void GFXMETALDevice::setPixelShaderVersion(F32 version) {
}

U32 GFXMETALDevice::getNumSamplers() const {
}

U32 GFXMETALDevice::getNumRenderTargets() const {
}

GFXShader *GFXMETALDevice::createShader() {
}

void GFXMETALDevice::copyResource(GFXTextureObject *pDst, GFXCubemap *pSrc, const U32 face) {
}

void GFXMETALDevice::clear(U32 flags, const LinearColorF &color, F32 z, U32 stencil) {
}

void GFXMETALDevice::clearColorAttachment(const U32 attachment, const LinearColorF &color) {
}

void GFXMETALDevice::drawPrimitive(GFXPrimitiveType primType, U32 vertexStart, U32 primitiveCount) {
}

void GFXMETALDevice::drawIndexedPrimitive(GFXPrimitiveType primType,
                                          U32 startVertex,
                                          U32 minIndex,
                                          U32 numVerts,
                                          U32 startIndex,
                                          U32 primitiveCount) {
}

GFXFence *GFXMETALDevice::createFence() {
}

void GFXMETALDevice::setClipRect(const RectI &rect) {
}

const RectI &GFXMETALDevice::getClipRect() const {
}

F32 GFXMETALDevice::getFillConventionOffset() const {
}

U32 GFXMETALDevice::getMaxDynamicVerts() {
}

U32 GFXMETALDevice::getMaxDynamicIndices() {
}

void GFXMETALDevice::_updateRenerTargets() {
}


U32 GFXMETALDevice::getTotalVideoMemory() {
}


GFXCubemap *GFXMETALDevice::createCubemap() {
}

class GFXMetalRegisterDevice
{
public:
   GFXMetalRegisterDevice()
   {
      GFXInit::getRegisterDeviceSignal().notify(&GFXMETALDevice::enumerateAdapters);
   }
};

static GFXMetalRegisterDevice pMetalRegisterDevice;
