//-----------------------------------------------------------------------------
// Copyright (c) 2014 Daniel Buckmaster
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

#include "navContext.h"
#include "console/sim.h"

void NavContext::doResetLog()
{
}

void NavContext::log(const rcLogCategory category, const String &msg)
{
   doLog(category, msg.c_str(), msg.length());
}

void NavContext::doLog(const rcLogCategory category, const char* msg, const int len)
{
   if(category == RC_LOG_ERROR)
      Con::errorf(msg);
   else
      Con::printf(msg);
}

void NavContext::doResetTimers()
{
   for(U32 i = 0; i < RC_MAX_TIMERS; i++)
   {
      mAccTime[i] = -1;
   }
}

void NavContext::doStartTimer(const rcTimerLabel label)
{
   // Store starting time.
   mStartTime[label] = Platform::getRealMilliseconds();
}

void NavContext::doStopTimer(const rcTimerLabel label)
{
   // Compute final time based on starting time.
   const S32 endTime = Platform::getRealMilliseconds();
   const S32 delta = endTime - mStartTime[label];
   if (mAccTime[label] == -1)
      mAccTime[label] = delta;
   else
      mAccTime[label] += delta;
}

int NavContext::doGetAccumulatedTime(const rcTimerLabel label) const
{
   return mAccTime[label];
}
