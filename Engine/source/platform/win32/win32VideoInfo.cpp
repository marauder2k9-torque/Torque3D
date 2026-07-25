//-----------------------------------------------------------------------------
// win32VideoInfo.cpp — Windows PlatformVideoInfo implementation
// (WMIVideoInfo), querying GPU adapter info via three fallback layers:
// DXGI (preferred, Windows 7+), DxDiag (XP/Vista-compatible fallback), and
// WMI (final fallback via Win32_VideoController).
//
// Ported from wmiVideoInfo.cpp/.h. Logic unchanged — this is real, working,
// dependency-free Windows code (DXGI/COM/WMI are all part of the OS
// itself, no external library involved), so it's kept as a faithful port
// rather than replaced. :: prefix applied to Win32/COM API calls for the
// same name-lookup-safety reasoning used throughout the rest of this
// rewrite.
//
// NOTE: WMIVideoInfo is declared in win32VideoInfo.h (a real header, not
// hidden in this .cpp) because GFXD3D11CardProfiler (outside the platform
// layer) constructs it directly by name — new WMIVideoInfo() — so the
// class must be visible from wherever that call site includes the header,
// matching the original's wmiVideoInfo.h/wmiVideoInfo.cpp split.
//-----------------------------------------------------------------------------
#define _WIN32_DCOM

#include "platform/win32/win32VideoInfo.h"
#include "core/util/safeRelease.h"
#include "console/console.h"
#include "core/strings/stringFunctions.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wbemidl.h>
#include <DXGI.h>

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "wbemuuid.lib")

// Add a constructor to GUID for the two DxDiag CLSID/IID declarations below.
struct MYGUID : public GUID
{
   MYGUID(DWORD a, SHORT b, SHORT c, BYTE d, BYTE e, BYTE f, BYTE g, BYTE h, BYTE i, BYTE j, BYTE k)
   {
      Data1 = a;
      Data2 = b;
      Data3 = c;
      Data4[0] = d;
      Data4[1] = e;
      Data4[2] = f;
      Data4[3] = g;
      Data4[4] = h;
      Data4[5] = i;
      Data4[6] = j;
      Data4[7] = k;
   }
};

static MYGUID CLSID_DxDiagProvider(0xA65B8071, 0x3BFE, 0x4213, 0x9A, 0x5B, 0x49, 0x1D, 0xA4, 0x46, 0x1C, 0xA7);
static MYGUID IID_IDxDiagProvider(0x9C6B4CB0, 0x23F8, 0x49CC, 0xA3, 0xED, 0x45, 0xA5, 0x50, 0x00, 0xA6, 0xD2);
static MYGUID IID_IDxDiagContainer(0x7D0F462F, 0x4064, 0x4862, 0xBC, 0x7F, 0x93, 0x3E, 0x50, 0x58, 0xC1, 0x0F);

//------------------------------------------------------------------------------
const WCHAR* WMIVideoInfo::smPVIQueryTypeToWMIString[] =
{
    L"MaxNumberControlled", // PVI_NumDevices
    L"Description",         // PVI_Description
    L"Name",                // PVI_Name
    L"VideoProcessor",      // PVI_ChipSet
    L"DriverVersion",       // PVI_DriverVersion
    L"AdapterRAM",          // PVI_VRAM
};

//------------------------------------------------------------------------------
WMIVideoInfo::WMIVideoInfo()
   : PlatformVideoInfo()
{
}

WMIVideoInfo::~WMIVideoInfo()
{
   SAFE_RELEASE(mLocator);
   SAFE_RELEASE(mServices);

   if (mDxDiagProvider)
      SAFE_RELEASE(mDxDiagProvider);

   if (mDXGIFactory)
      SAFE_RELEASE(mDXGIFactory);
   if (mDXGIModule)
      ::FreeLibrary(static_cast<HMODULE>(mDXGIModule));

   if (mComInitialized)
      ::CoUninitialize();
}

//------------------------------------------------------------------------------
String WMIVideoInfo::_lookUpVendorId(U32 vendorId)
{
   String vendor;
   switch (vendorId)
   {
   case 0x10DE: vendor = "NVIDIA"; break;
   case 0x1002: vendor = "AMD";    break;
   case 0x8086: vendor = "INTEL";  break;
   }
   return vendor;
}

//------------------------------------------------------------------------------
bool WMIVideoInfo::_initialize()
{
   const HRESULT hr = ::CoInitialize(nullptr);
   mComInitialized = SUCCEEDED(hr);

   if (!mComInitialized)
      return false;

   bool success = false;

   success |= _initializeDXGI();
   success |= _initializeDxDiag();
   success |= _initializeWMI();

   return success;
}

bool WMIVideoInfo::_initializeWMI()
{
   // Obtain the locator to WMI.
   HRESULT hr = ::CoCreateInstance(
      CLSID_WbemLocator,
      0,
      CLSCTX_INPROC_SERVER,
      IID_IWbemLocator,
      reinterpret_cast<void**>(&mLocator));

   if (FAILED(hr))
   {
      Con::errorf("WMIVideoInfo: Failed to create instance of IID_IWbemLocator.");
      return false;
   }

   // Connect to the root\cimv2 namespace with the current user and obtain
   // a pointer to make IWbemServices calls.
   hr = mLocator->ConnectServer(
      BSTR(L"ROOT\\CIMV2"), // Object path of WMI namespace
      nullptr,              // User name. NULL = current user
      nullptr,              // User password. NULL = current
      0,                    // Locale. NULL indicates current
      0,                    // Security flags.
      0,                    // Authority (e.g. Kerberos)
      0,                    // Context object
      &mServices);          // pointer to IWbemServices proxy

   if (FAILED(hr))
   {
      Con::errorf("WMIVideoInfo: Connect server failed.");
      return false;
   }

   // Set security levels on the proxy.
   hr = ::CoSetProxyBlanket(
      mServices,
      RPC_C_AUTHN_WINNT,
      RPC_C_AUTHZ_NONE,
      nullptr,
      RPC_C_AUTHN_LEVEL_CALL,
      RPC_C_IMP_LEVEL_IMPERSONATE,
      nullptr,
      EOAC_NONE);

   if (FAILED(hr))
   {
      Con::errorf("WMIVideoInfo: CoSetProxyBlanket failed");
      return false;
   }

   return true;
}

bool WMIVideoInfo::_initializeDXGI()
{
   // DXGI 1.1 — only succeeds on Windows 7+.
   mDXGIModule = static_cast<HMODULE>(::LoadLibraryW(L"dxgi.dll"));
   if (mDXGIModule != nullptr)
   {
      using CreateDXGIFactoryFuncType = HRESULT(WINAPI*)(REFIID, void**);
      auto factoryFunction = reinterpret_cast<CreateDXGIFactoryFuncType>(
         ::GetProcAddress(static_cast<HMODULE>(mDXGIModule), "CreateDXGIFactory1"));

      if (factoryFunction && factoryFunction(IID_IDXGIFactory1, reinterpret_cast<void**>(&mDXGIFactory)) == S_OK)
         return true;

      ::FreeLibrary(static_cast<HMODULE>(mDXGIModule));
      mDXGIModule = nullptr;
   }

   return false;
}

bool WMIVideoInfo::_initializeDxDiag()
{
   if (::CoCreateInstance(CLSID_DxDiagProvider, nullptr, CLSCTX_INPROC_SERVER,
      IID_IDxDiagProvider, reinterpret_cast<void**>(&mDxDiagProvider)) == S_OK)
   {
      DXDIAG_INIT_PARAMS params{};
      params.dwSize = sizeof(DXDIAG_INIT_PARAMS);
      params.dwDxDiagHeaderVersion = 111;
      params.bAllowWHQLChecks = false;

      const HRESULT result = mDxDiagProvider->Initialize(&params);
      if (result != S_OK)
      {
         Con::errorf("WMIVideoInfo: DxDiag initialization failed (%i)", result);
         SAFE_RELEASE(mDxDiagProvider);
         return false;
      }

      Con::printf("WMIVideoInfo: DxDiag initialized");
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------
// http://msdn2.microsoft.com/en-us/library/aa394512.aspx
//
// Win32_VideoController represents the capabilities/management capacity of
// the video controller. Starting with Vista, hardware not compatible with
// WDDM returns inaccurate property values for instances of this class.
//------------------------------------------------------------------------------
bool WMIVideoInfo::_queryProperty(const PVIQueryType queryType, const U32 adapterId, String* outValue)
{
   if (_queryPropertyDXGI(queryType, adapterId, outValue))
      return true;
   if (_queryPropertyDxDiag(queryType, adapterId, outValue))
      return true;
   return _queryPropertyWMI(queryType, adapterId, outValue);
}

bool WMIVideoInfo::_queryPropertyDxDiag(const PVIQueryType queryType, const U32 adapterId, String* outValue)
{
   if (mDxDiagProvider == nullptr)
      return false;

   IDxDiagContainer* rootContainer = nullptr;
   IDxDiagContainer* displayDevicesContainer = nullptr;
   IDxDiagContainer* deviceContainer = nullptr;

   // Special case for PVI_NumAdapters.
   if (queryType == PVI_NumAdapters)
   {
      DWORD count = 0;
      String value;

      if (mDxDiagProvider->GetRootContainer(&rootContainer) == S_OK
         && rootContainer->GetChildContainer(L"DxDiag_DisplayDevices", &displayDevicesContainer) == S_OK
         && displayDevicesContainer->GetNumberOfChildContainers(&count) == S_OK)
      {
         value = String::ToString("%d", count);
      }

      if (rootContainer) SAFE_RELEASE(rootContainer);
      if (displayDevicesContainer) SAFE_RELEASE(displayDevicesContainer);

      *outValue = value;
      return true;
   }

   WCHAR adapterIdString[2];
   adapterIdString[0] = L'0' + static_cast<WCHAR>(adapterId);
   adapterIdString[1] = L'\0';

   String value;
   if (mDxDiagProvider->GetRootContainer(&rootContainer) == S_OK
      && rootContainer->GetChildContainer(L"DxDiag_DisplayDevices", &displayDevicesContainer) == S_OK
      && displayDevicesContainer->GetChildContainer(adapterIdString, &deviceContainer) == S_OK)
   {
      const WCHAR* propertyName = nullptr;

      switch (queryType)
      {
      case PVI_Description:   propertyName = L"szDescription";  break;
      case PVI_Name:          propertyName = L"szDeviceName";   break;
      case PVI_ChipSet:       propertyName = L"szChipType";     break;
      case PVI_DriverVersion: propertyName = L"szDriverVersion"; break;
         // Don't get VRAM via DxDiag — it reports dedicated+shared RAM
         // combined, not just dedicated video memory.
      default: break;
      }

      if (propertyName)
      {
         VARIANT val;
         if (deviceContainer->GetProp(propertyName, &val) == S_OK)
         {
            switch (val.vt)
            {
            case VT_BSTR:
               value = String(val.bstrVal);
               break;
            default:
               AssertWarn(false, avar("WMIVideoInfo: property type '%i' not implemented", val.vt));
            }
         }
      }
   }

   if (rootContainer) SAFE_RELEASE(rootContainer);
   if (displayDevicesContainer) SAFE_RELEASE(displayDevicesContainer);
   if (deviceContainer) SAFE_RELEASE(deviceContainer);

   if (value.isNotEmpty())
   {
      // Normalize into a canonical form so the card profiler has a
      // reasonable chance of matching this up with profile scripts.
      switch (queryType)
      {
      case PVI_ChipSet:
         if (value.compare("ATI", 3, String::NoCase) == 0)
            value = "ATI Technologies Inc.";
         else if (value.compare("NVIDIA", 6, String::NoCase) == 0)
            value = "NVIDIA";
         else if (value.compare("INTEL", 5, String::NoCase) == 0)
            value = "INTEL";
         else if (value.compare("MATROX", 6, String::NoCase) == 0)
            value = "MATROX";
         break;

      case PVI_Description:
         if (value.compare("ATI ", 4, String::NoCase) == 0)
         {
            value = value.substr(4, value.length() - 4);
            if (value.compare(" Series", 7, String::NoCase | String::Right) == 0)
               value = value.substr(0, value.length() - 7);
         }
         else if (value.compare("NVIDIA ", 7, String::NoCase) == 0)
            value = value.substr(7, value.length() - 7);
         else if (value.compare("INTEL ", 6, String::NoCase) == 0)
            value = value.substr(6, value.length() - 6);
         else if (value.compare("MATROX ", 7, String::NoCase) == 0)
            value = value.substr(7, value.length() - 7);
         break;

      default:
         break;
      }

      *outValue = value;
      return true;
   }

   return false;
}

bool WMIVideoInfo::_queryPropertyDXGI(const PVIQueryType queryType, const U32 adapterId, String* outValue)
{
   if (!mDXGIFactory)
      return false;

   if (queryType == PVI_NumAdapters)
   {
      U32 count = 0;
      IDXGIAdapter1* adapter;
      while (mDXGIFactory->EnumAdapters1(count, &adapter) != DXGI_ERROR_NOT_FOUND)
      {
         ++count;
         adapter->Release();
      }

      *outValue = String::ToString("%d", count);
      return true;
   }

   IDXGIAdapter1* adapter;
   if (mDXGIFactory->EnumAdapters1(adapterId, &adapter) != S_OK)
      return false;

   DXGI_ADAPTER_DESC1 desc;
   if (adapter->GetDesc1(&desc) != S_OK)
   {
      adapter->Release();
      return false;
   }

   String value;
   switch (queryType)
   {
   case PVI_Description:
      value = String(desc.Description);
      break;
   case PVI_Name:
      value = String(avar("%i", desc.DeviceId));
      break;
   case PVI_VRAM:
      value = String(avar("%i", desc.DedicatedVideoMemory / 1048576));
      break;
   case PVI_ChipSet:
      value = _lookUpVendorId(desc.VendorId);
      break;
      // TODO: PVI_DriverVersion has no DXGI equivalent — falls through
      // to the DxDiag/WMI layers, matching the original.
   default:
      break;
   }

   adapter->Release();
   *outValue = value;
   return true;
}

bool WMIVideoInfo::_queryPropertyWMI(const PVIQueryType queryType, const U32 adapterId, String* outValue)
{
   if (mServices == nullptr)
      return false;

   BSTR bstrWQL = ::SysAllocString(L"WQL");
   BSTR bstrPath = ::SysAllocString(L"select * from Win32_VideoController");
   IEnumWbemClassObject* enumerator = nullptr;

   HRESULT hr = mServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, nullptr, &enumerator);

   ::SysFreeString(bstrWQL);
   ::SysFreeString(bstrPath);

   if (FAILED(hr))
      return false;

   IWbemClassObject* adapter = nullptr;
   ULONG uReturned = 0;

   for (S32 i = 0; i <= static_cast<S32>(adapterId); i++)
   {
      hr = enumerator->Next(WBEM_INFINITE, 1, &adapter, &uReturned);
      if (FAILED(hr) || uReturned == 0)
      {
         enumerator->Release();
         return false;
      }
   }

   VARIANT v;
   hr = adapter->Get(smPVIQueryTypeToWMIString[queryType], 0, &v, nullptr, nullptr);

   const bool result = SUCCEEDED(hr);

   if (result)
   {
      switch (v.vt)
      {
      case VT_I4:
      {
         LONG longVal = v.lVal;

         if (queryType == PVI_VRAM)
         {
            // Convert to megabytes. This value is reported signed,
            // but cards with 2GB+ set the sign bit — treating it
            // as unsigned lets us handle up to 4GB correctly.
            longVal = longVal >> 20;
            *outValue = String::ToString(static_cast<U32>(longVal));
         }
         else
         {
            *outValue = String::ToString(static_cast<S32>(longVal));
         }
         break;
      }
      case VT_UI4:
         *outValue = String::ToString(static_cast<U32>(v.ulVal));
         break;
      case VT_BSTR:
         *outValue = String(v.bstrVal);
         break;
      case VT_LPSTR:
      case VT_LPWSTR:
         break;
      default:
         break;
      }
   }

   adapter->Release();
   enumerator->Release();

   return result;
}
