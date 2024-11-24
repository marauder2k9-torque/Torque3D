#pragma once
#ifndef _GFXDX11DEVICE_H_
#define _GFXDX11DEVICE_H_

#include <d3d11_1.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include "platform/tmm_off.h"

#ifndef _PLATFORMWIN32_H_
#include "platformWin32/platformWin32.h"
#endif

#ifndef _GFXDEVICE_H_
#include "gfx2/gfxDevice.h"
#endif

//
#include "platform/tmm_on.h"

class GFXDX11Device : public GFXDevice
{
private:
   ComPtr<ID3D11Device> mDX11Device;
   ComPtr<ID3D11DeviceContext> mDX11DeviceContext;
   ComPtr<IDXGISwapChain> mSwapChain;
};

#endif // !_GFXDX11DEVICE_H_
