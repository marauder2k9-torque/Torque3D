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

#import "platformIOS/iosAppDelegate.h"

#include "platform/platformInput.h"
#include "console/console.h"

extern void _iosInnerLoop();
extern void _iosResignActive();
extern void _iosBecomeActive();
extern void _iosWillTerminate();

extern void _iosChangeOrientation(S32 newOrientation);
UIDeviceOrientation curOrientation;

bool _iosFatalError = false;

@implementation iosAppDelegate;

@synthesize window = _window;

- (void)applicationDidFinishLaunching:(UIApplication *)application
{
   [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
   
   curOrientation = [UIDevice currentDevice].orientation;
   
   [[NSNotificationCenter defaultCenter] addObserver:self
                                            selector:@selector(didRotate:)
                                                name:UIDeviceOrientationDidChangeNotification object:nil];
}

- (void)applicationWillResignActive:(UIApplication *)application
{
   _iosResignActive();
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
   if(!_iosFatalError)
      _iosBecomeActive();
}

- (void)applicationWillTerminate:(UIApplication *)application
{
   _iosWillTerminate();
   
   [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
}

- (void)didRotate:(NSNotification *)notifaction
{
   UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
   if(curOrientation != orientation)
   {
      curOrientation = orientation;
      _iosChangeOrientation((S32)curOrientation);
   }
}

- (void)run
{
   _iosInnerLoop();
}

@end
