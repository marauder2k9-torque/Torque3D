//-----------------------------------------------------------------------------
// linuxTime.cpp — Linux implementation of platform.h's time-related API.
//
// Fully native (libc + POSIX clocks only). getRealMilliseconds() uses
// clock_gettime(CLOCK_MONOTONIC), which is the correct choice for elapsed-
// time measurement on Linux: unlike CLOCK_REALTIME it can't jump backward
// due to NTP/manual clock adjustments, and unlike times()-based ticks it
// doesn't wrap on any practical timescale (matches the intent behind
// win32Time.cpp's move to GetTickCount64() over GetTickCount()).
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformTimer.h"

#include <ctime>
#include <cstring>
#include <unistd.h>
#include <cerrno>

namespace
{
    U32 sVirtualMs = 0;

    // Cached monotonic start time so getRealMilliseconds() returns
    // small, stable values from process start (matching the general
    // shape of GetTickCount64()/mach_absolute_time()-derived values)
    // rather than an arbitrary large epoch-relative count.
    struct MonotonicBase
    {
        timespec start{};
        MonotonicBase() { clock_gettime(CLOCK_MONOTONIC, &start); }
    };

    const MonotonicBase& monotonicBase()
    {
        static MonotonicBase base;
        return base;
    }
}

//-----------------------------------------------------------------------------
void Platform::sleep(U32 ms)
{
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = static_cast<long>(ms % 1000) * 1000000L;

    // Restart on EINTR (signal interruption) with the remaining time,
    // rather than returning early — matches the "sleeps for at least ms"
    // contract every caller of Platform::sleep expects.
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
    {
    }
}

void Platform::getLocalTime(LocalTime &lt)
{
    time_t longTime;
    time(&longTime);

    tm systime;
    localtime_r(&longTime, &systime);

    lt.sec      = static_cast<U8>(systime.tm_sec);
    lt.min      = static_cast<U8>(systime.tm_min);
    lt.hour     = static_cast<U8>(systime.tm_hour);
    lt.month    = static_cast<U8>(systime.tm_mon);
    lt.monthday = static_cast<U8>(systime.tm_mday);
    lt.weekday  = static_cast<U8>(systime.tm_wday);
    lt.year     = static_cast<U16>(systime.tm_year);
    lt.yearday  = static_cast<U16>(systime.tm_yday);
    lt.isdst    = systime.tm_isdst > 0;
}

String Platform::localTimeToString(const LocalTime &lt)
{
    tm systime{};
    systime.tm_sec   = lt.sec;
    systime.tm_min   = lt.min;
    systime.tm_hour  = lt.hour;
    systime.tm_mon   = lt.month;
    systime.tm_mday  = lt.monthday;
    systime.tm_wday  = lt.weekday;
    systime.tm_year  = lt.year;
    systime.tm_yday  = lt.yearday;
    systime.tm_isdst = lt.isdst;

    char buf[64];
    asctime_r(&systime, buf);

    // asctime_r's result is newline-terminated ("Www Mmm dd hh:mm:ss yyyy\n");
    // trim that trailing newline so this matches the plain-string contract
    // callers expect (e.g. for embedding in log lines).
    const size_t len = std::strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';

    return String(buf);
}

U32 Platform::getTime()
{
    return static_cast<U32>(std::time(nullptr));
}

U32 Platform::getRealMilliseconds()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    const timespec& start = monotonicBase().start;
    const S64 deltaSec  = static_cast<S64>(ts.tv_sec) - static_cast<S64>(start.tv_sec);
    const S64 deltaNsec = static_cast<S64>(ts.tv_nsec) - static_cast<S64>(start.tv_nsec);

    return static_cast<U32>(deltaSec * 1000 + deltaNsec / 1000000);
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

    // FileTime is a plain S64 epoch-seconds value on all POSIX-family
    // platforms (see platformTypes.h) — Linux, like macOS, reads it
    // straight from stat()'s st_mtime/st_ctime, so no FILETIME-style
    // decode is needed here.
    const time_t longTime = static_cast<time_t>(ft);

    tm systime;
    localtime_r(&longTime, &systime);

    lt->sec      = static_cast<U8>(systime.tm_sec);
    lt->min      = static_cast<U8>(systime.tm_min);
    lt->hour     = static_cast<U8>(systime.tm_hour);
    lt->month    = static_cast<U8>(systime.tm_mon);
    lt->monthday = static_cast<U8>(systime.tm_mday);
    lt->weekday  = static_cast<U8>(systime.tm_wday);
    lt->year     = static_cast<U16>(systime.tm_year);
    lt->yearday  = static_cast<U16>(systime.tm_yday);
    lt->isdst    = systime.tm_isdst > 0;
}

//-----------------------------------------------------------------------------
PlatformTimer *PlatformTimer::create()
{
    return new DefaultPlatformTimer();
}
