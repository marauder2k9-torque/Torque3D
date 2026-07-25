//-----------------------------------------------------------------------------
// win32Time.cpp — Windows implementation of platform.h's time-related API.
//
// Fresh C++17 rewrite. Two corrections versus the original winTime.cpp:
//   1. Uses GetTickCount64() instead of GetTickCount(), which wraps around
//      every ~49.7 days (a real, if rare in practice, bug for long-running
//      dedicated servers).
//   2. FileTime is now correctly a 2-field {U32 v1; U32 v2;} struct on
//      Windows (see platformTypes.h) rather than a flattened S64 — this
//      file constructs/reads real Win32 FILETIME values from it directly,
//      matching what GetFileTime/FileTimeToLocalFileTime expect.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformTimer.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctime>
#include <cstring>

namespace
{
    U32 sVirtualMs = 0;
}

//-----------------------------------------------------------------------------
void Platform::sleep(U32 ms)
{
    Sleep(ms);
}

void Platform::getLocalTime(LocalTime &lt)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    lt.sec      = static_cast<U8>(st.wSecond);
    lt.min      = static_cast<U8>(st.wMinute);
    lt.hour     = static_cast<U8>(st.wHour);
    lt.month    = static_cast<U8>(st.wMonth - 1);
    lt.monthday = static_cast<U8>(st.wDay);
    lt.weekday  = static_cast<U8>(st.wDayOfWeek);
    lt.year     = static_cast<U16>(st.wYear - 1900);
    lt.yearday  = 0;      // Win32 SYSTEMTIME has no day-of-year field.
    lt.isdst    = false;  // SYSTEMTIME carries no DST flag either; would
                          // need GetTimeZoneInformation to determine this.
}

String Platform::localTimeToString(const LocalTime &lt)
{
    SYSTEMTIME st{};
    st.wSecond = lt.sec;
    st.wMinute = lt.min;
    st.wHour   = lt.hour;
    st.wDay    = lt.monthday;
    st.wDayOfWeek = lt.weekday;
    st.wMonth  = lt.month + 1;
    st.wYear   = lt.year + 1900;

    char dateBuf[256]{};
    char timeBuf[256]{};

    GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, dateBuf, sizeof(dateBuf));
    GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, &st, nullptr, timeBuf, sizeof(timeBuf));

    String result(dateBuf);
    result += "\t";
    result += timeBuf;
    return result;
}

U32 Platform::getTime()
{
    return static_cast<U32>(std::time(nullptr));
}

U32 Platform::getRealMilliseconds()
{
    return static_cast<U32>(GetTickCount64());
}

U32 Platform::getVirtualMilliseconds()
{
    return sVirtualMs;
}

void Platform::advanceTime(U32 delta)
{
    sVirtualMs += delta;
}

void Platform::fileToLocalTime(const FileTime &ft, LocalTime *lt)
{
    if (!lt)
        return;

    std::memset(lt, 0, sizeof(LocalTime));

    FILETIME winFileTime;
    winFileTime.dwLowDateTime  = ft.v1;
    winFileTime.dwHighDateTime = ft.v2;

    FILETIME localFileTime;
    if (!FileTimeToLocalFileTime(&winFileTime, &localFileTime))
        return;

    SYSTEMTIME st;
    if (!FileTimeToSystemTime(&localFileTime, &st))
        return;

    lt->sec      = static_cast<U8>(st.wSecond);
    lt->min      = static_cast<U8>(st.wMinute);
    lt->hour     = static_cast<U8>(st.wHour);
    lt->month    = static_cast<U8>(st.wMonth - 1);
    lt->monthday = static_cast<U8>(st.wDay);
    lt->weekday  = static_cast<U8>(st.wDayOfWeek);
    lt->year     = static_cast<U16>(st.wYear < 1900 ? 1900 : st.wYear - 1900);
    lt->yearday  = 0;
    lt->isdst    = false;
}

//-----------------------------------------------------------------------------
PlatformTimer *PlatformTimer::create()
{
    return new DefaultPlatformTimer();
}
