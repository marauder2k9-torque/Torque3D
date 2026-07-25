//-----------------------------------------------------------------------------
// macTime.mm — macOS time functions.
//
// Ported from macTime.mm. mach_absolute_time()-based getRealMilliseconds()
// is real, native Apple API (no external dependency) and was already
// correct — kept as-is.
//-----------------------------------------------------------------------------
#import <mach/mach_time.h>
#import "platform/platformTimer.h"
#import <time.h>
#import <unistd.h>

namespace
{
    U32 sCurrentTime = 0;
}

void Platform::getLocalTime(LocalTime &lt)
{
    struct tm systime;
    time_t long_time;

    time(&long_time);
    localtime_r(&long_time, &systime);

    lt.sec      = systime.tm_sec;
    lt.min      = systime.tm_min;
    lt.hour     = systime.tm_hour;
    lt.month    = systime.tm_mon;
    lt.monthday = systime.tm_mday;
    lt.weekday  = systime.tm_wday;
    lt.year     = systime.tm_year;
    lt.yearday  = systime.tm_yday;
    lt.isdst    = systime.tm_isdst;
}

String Platform::localTimeToString(const LocalTime &lt)
{
    tm systime;

    systime.tm_sec   = lt.sec;
    systime.tm_min   = lt.min;
    systime.tm_hour  = lt.hour;
    systime.tm_mon   = lt.month;
    systime.tm_mday  = lt.monthday;
    systime.tm_wday  = lt.weekday;
    systime.tm_year  = lt.year;
    systime.tm_yday  = lt.yearday;
    systime.tm_isdst = lt.isdst;

    return asctime(&systime);
}

U32 Platform::getTime()
{
    time_t epochTime;
    time(&epochTime);
    return static_cast<U32>(epochTime);
}

U32 Platform::getRealMilliseconds()
{
    constexpr uint32_t oneMillion = 1000000;
    static mach_timebase_info_data_t sTimebaseInfo;

    if (sTimebaseInfo.denom == 0)
        mach_timebase_info(&sTimebaseInfo);

    // mach_absolute_time() ticks are converted to nanoseconds via the
    // timebase fraction, then divided down to milliseconds.
    return static_cast<U32>((mach_absolute_time() * sTimebaseInfo.numer) / (oneMillion * sTimebaseInfo.denom));
}

U32 Platform::getVirtualMilliseconds()
{
    return sCurrentTime;
}

void Platform::advanceTime(U32 delta)
{
    sCurrentTime += delta;
}

void Platform::sleep(U32 ms)
{
    // Overflows past ~49 days of requested sleep - not a practical concern.
    usleep(ms * 1000);
}

void Platform::fileToLocalTime(const FileTime &ft, LocalTime *lt)
{
    if (!lt)
        return;

    time_t long_time = static_cast<time_t>(ft);

    struct tm systime;
    localtime_r(&long_time, &systime);

    lt->sec      = systime.tm_sec;
    lt->min      = systime.tm_min;
    lt->hour     = systime.tm_hour;
    lt->month    = systime.tm_mon;
    lt->monthday = systime.tm_mday;
    lt->weekday  = systime.tm_wday;
    lt->year     = systime.tm_year;
    lt->yearday  = systime.tm_yday;
    lt->isdst    = systime.tm_isdst;
}

//-----------------------------------------------------------------------------
PlatformTimer* PlatformTimer::create()
{
    return new DefaultPlatformTimer;
}
