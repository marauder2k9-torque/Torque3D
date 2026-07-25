//-----------------------------------------------------------------------------
// win32Async.cpp — Windows implementation of AsyncUpdateThread /
// AsyncPeriodicUpdateThread (platform/async/asyncUpdate.h), used by the
// SFX/audio subsystem's async update mechanism.
//
// Ported from winAsync.cpp. Logic unchanged — this is small, real,
// self-contained Win32 event/waitable-timer code with no dependency on
// Win32PlatState or anything else this rewrite has been decoupling from.
// :: prefix added to Win32 API calls for the same name-lookup-safety
// reasoning applied throughout the rest of this rewrite.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/async/asyncUpdate.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

AsyncUpdateThread::AsyncUpdateThread(String name, AsyncUpdateList* updateList)
    : Parent(0, 0, false, false),
      mUpdateList(updateList),
      mName(name)
{
    // Auto-reset event, created in the non-signaled state.
    mUpdateEvent = ::CreateEvent(nullptr, false, false, nullptr);
}

AsyncUpdateThread::~AsyncUpdateThread()
{
    ::CloseHandle(static_cast<HANDLE>(mUpdateEvent));
}

void AsyncUpdateThread::_waitForEventAndReset()
{
    ::WaitForSingleObject(static_cast<HANDLE>(mUpdateEvent), INFINITE);
}

void AsyncUpdateThread::triggerUpdate()
{
    ::SetEvent(static_cast<HANDLE>(mUpdateEvent));
}

//-----------------------------------------------------------------------------
AsyncPeriodicUpdateThread::AsyncPeriodicUpdateThread(String name,
                                                      AsyncUpdateList* updateList,
                                                      U32 intervalMS)
    : Parent(name, updateList)
{
    mUpdateTimer = ::CreateWaitableTimer(nullptr, FALSE, nullptr);

    // dueTime is in 100-nanosecond intervals and relative (negative);
    // period is in milliseconds.
    mIntervalMS = intervalMS;

    LARGE_INTEGER deltaTime;
    deltaTime.QuadPart = -LONGLONG(intervalMS * 10 /* micro */ * 1000 /* milli */);

    ::SetWaitableTimer(static_cast<HANDLE>(mUpdateTimer), &deltaTime, intervalMS, nullptr, nullptr, FALSE);
}

AsyncPeriodicUpdateThread::~AsyncPeriodicUpdateThread()
{
    ::CloseHandle(static_cast<HANDLE>(mUpdateTimer));
}

void AsyncPeriodicUpdateThread::_waitForEventAndReset()
{
    HANDLE handles[2];
    handles[0] = static_cast<HANDLE>(mUpdateEvent);
    handles[1] = static_cast<HANDLE>(mUpdateTimer);

    ::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
}
