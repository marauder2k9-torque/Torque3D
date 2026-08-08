//-----------------------------------------------------------------------------
// platform/win32/win32Console.h — see win32Console.cpp for details.
//-----------------------------------------------------------------------------
#ifndef _WIN32CONSOLE_H_
#define _WIN32CONSOLE_H_

/// Mirrors engine console output (Con::printf/warnf/errorf) to the IDE's
/// debug output window via OutputDebugString.
namespace Win32Console
{
   /// Registers the console consumer. Call once during Windows platform
   /// init.
   void init();

   /// Unregisters the consumer. Call once during Windows platform
   /// shutdown.
   void destroy();
}

#endif // _WIN32CONSOLE_H_
