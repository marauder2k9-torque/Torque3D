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
#if defined( TORQUE_SDL ) && !defined( TORQUE_DEDICATED )

#include "gfx/gfxCubemap.h"
#include "gfx/screenshot.h"

#include "gfx/metal/gfxMetalDevice.h"
#include "windowManager/sdl/sdlWindow.h"
#include "SDL.h"

void EnumerateVideoModes(Vector<GFXVideoMode>& outModes)
{
   int count = SDL_GetNumDisplayModes( 0 );
   if( count < 0)
   {
      AssertFatal(0, "");
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
   
   // Create a dummy window & openGL context so that gl functions can be used here
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
   
   dStrcpy(toAdd->mName, "Metal Device", GFXAdapter::MaxAdapterNameLen);
   
   SDL_MetalView view = SDL_Metal_CreateView(tempWindow);
   
   CA::MetalLayer* layer = static_cast<CA::MetalLayer*>(SDL_Metal_GetLayer(view));
   
}

void GFXMETALDevice::init(const GFXVideoMode &mode, PlatformWindow *window)
{
   
}

GFXWindowTarget* GFXMETALDevice::allocWindowTarget(PlatformWindow *window)
{
   
}

#endif
