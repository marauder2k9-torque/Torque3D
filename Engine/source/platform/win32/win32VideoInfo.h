//-----------------------------------------------------------------------------
// win32VideoInfo.h — Windows PlatformVideoInfo implementation (WMIVideoInfo).
// Ported from wmiVideoInfo.h. See win32VideoInfo.cpp for the implementation
// and full explanation of the DXGI/DxDiag/WMI three-layer fallback.
//-----------------------------------------------------------------------------
#pragma once

#include "platform/platformVideoInfo.h"

// DWORD/BOOL/LPVOID/HRESULT/STDMETHODCALLTYPE/IUnknown/LPWSTR/LPCWSTR/
// VARIANT/WCHAR are all Win32/COM types used below — without this include
// none of them are declared yet at this point in the file, which is why
// every line in the two DxDiag interface structs below would otherwise
// fail to parse (the compiler doesn't recognize HRESULT, so it treats
// "virtual HRESULT STDMETHODCALLTYPE Foo(...)" as nonsense rather than a
// member function declaration).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <oaidl.h> // VARIANT

struct IWbemLocator;
struct IWbemServices;
struct IDXGIFactory1;

//-----------------------------------------------------------------------------
// DxDiag declarations. DxDiag's COM interfaces aren't shipped in the public
// SDK headers (they're accessed via CoCreateInstance against known
// CLSID/IID GUIDs instead, defined in win32VideoInfo.cpp), so they're
// declared here directly, matching the original.
//-----------------------------------------------------------------------------
struct DXDIAG_INIT_PARAMS
{
   DWORD  dwSize;
   DWORD  dwDxDiagHeaderVersion;
   BOOL   bAllowWHQLChecks;
   LPVOID pReserved;
};

struct IDxDiagContainer : public IUnknown
{
   virtual HRESULT STDMETHODCALLTYPE GetNumberOfChildContainers(DWORD* pdwCount) = 0;
   virtual HRESULT STDMETHODCALLTYPE EnumChildContainerNames(DWORD dwIndex, LPWSTR pwszContainer, DWORD cchContainer) = 0;
   virtual HRESULT STDMETHODCALLTYPE GetChildContainer(LPCWSTR pwszContainer, IDxDiagContainer** ppInstance) = 0;
   virtual HRESULT STDMETHODCALLTYPE GetNumberOfProps(DWORD* pdwCount) = 0;
   virtual HRESULT STDMETHODCALLTYPE EnumPropNames(DWORD dwIndex, LPWSTR pwszPropName, DWORD cchPropName) = 0;
   virtual HRESULT STDMETHODCALLTYPE GetProp(LPCWSTR pwszPropName, VARIANT* pvarProp) = 0;
};

struct IDxDiagProvider : public IUnknown
{
   virtual HRESULT STDMETHODCALLTYPE Initialize(DXDIAG_INIT_PARAMS* pParams) = 0;
   virtual HRESULT STDMETHODCALLTYPE GetRootContainer(IDxDiagContainer** ppInstance) = 0;
};

//-----------------------------------------------------------------------------
class WMIVideoInfo final : public PlatformVideoInfo
{
   IWbemLocator* mLocator = nullptr;
   IWbemServices* mServices = nullptr;
   bool mComInitialized = false;

   void* mDXGIModule = nullptr;
   IDXGIFactory1* mDXGIFactory = nullptr;
   IDxDiagProvider* mDxDiagProvider = nullptr;

   bool _initializeDXGI();
   bool _initializeDxDiag();
   bool _initializeWMI();

   bool _queryPropertyDXGI(const PVIQueryType queryType, const U32 adapterId, String* outValue);
   bool _queryPropertyDxDiag(const PVIQueryType queryType, const U32 adapterId, String* outValue);
   bool _queryPropertyWMI(const PVIQueryType queryType, const U32 adapterId, String* outValue);

   static const WCHAR* smPVIQueryTypeToWMIString[];

protected:
   bool _queryProperty(const PVIQueryType queryType, const U32 adapterId, String* outValue) override;
   bool _initialize() override;
   String _lookUpVendorId(U32 vendorId);

public:
   WMIVideoInfo();
   ~WMIVideoInfo() override;
};
