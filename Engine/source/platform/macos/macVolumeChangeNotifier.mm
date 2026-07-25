//-----------------------------------------------------------------------------
// macVolumeChangeNotifier.mm — MacFileSystemChangeNotifier implementation
// (real, native FSEventStream-based directory change notifications).
//
// STRUCTURE NOTE: at global scope, matching the original macVolume.mm
// exactly (MacFileSystemChangeNotifier is not inside any namespace). Also
// matches the original's include order: <CoreServices/CoreServices.h>
// FIRST, before platform/console headers — this is deliberately the same
// order the original, proven-working macVolume.mm used.
//
// ISOLATION NOTE: kept in its own translation unit, separate from
// macVolume.mm, so that <CoreServices/CoreServices.h> — which pulls in
// Apple's Security.framework headers transitively — is not compiled
// alongside anything unrelated. This mirrors the original's actual
// structure (the original also only pulled in CoreServices.h where the
// FSEvents code lived), rather than being additional new isolation.
//-----------------------------------------------------------------------------
#import <CoreServices/CoreServices.h>
#import "platform/platform.h"
#import "platform/macos/macVolume.h"
#import "platform/platformVolume.h"
#import "console/console.h"

struct MacFileSystemChangeNotifier::Event
{
    FSEventStreamRef mStream;
    Torque::Path mDir;
    bool mHasChanged;
};

static void fsNotifyCallback(
    ConstFSEventStreamRef stream,
    void* callbackInfo,
    size_t numEvents,
    void* eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
    MacFileSystemChangeNotifier::Event* event =
        reinterpret_cast<MacFileSystemChangeNotifier::Event*>(callbackInfo);

    // Deferred to internalProcessOnce() so notification handling stays
    // consistent with how the volume system expects it to be reported.
    event->mHasChanged = true;
}

//-----------------------------------------------------------------------------
//    Change notifications.
//-----------------------------------------------------------------------------
MacFileSystemChangeNotifier::MacFileSystemChangeNotifier(MacFileSystem* fs)
    : Parent(fs)
{
}

MacFileSystemChangeNotifier::~MacFileSystemChangeNotifier()
{
    for (U32 i = 0, num = mEvents.size(); i < num; ++i)
    {
        FSEventStreamStop(mEvents[i]->mStream);
        FSEventStreamInvalidate(mEvents[i]->mStream);
        FSEventStreamRelease(mEvents[i]->mStream);
        delete mEvents[i];
    }
}

void MacFileSystemChangeNotifier::internalProcessOnce()
{
    for (U32 i = 0; i < mEvents.size(); ++i)
    {
        if (mEvents[i]->mHasChanged)
        {
            internalNotifyDirChanged(mEvents[i]->mDir);
            mEvents[i]->mHasChanged = false;
        }
    }
}

bool MacFileSystemChangeNotifier::internalAddNotification(const Torque::Path& dir)
{
    // Map the path.
    Torque::Path fullFSPath = mFS->mapTo(dir);
    String osPath = PathToOS(fullFSPath);

    // Create event stream.
    Event* event = new Event;

    CFStringRef path = CFStringCreateWithCharacters(NULL, osPath.utf16(), osPath.numChars());
    CFArrayRef paths = CFArrayCreate(NULL, (const void**)&path, 1, NULL);

    FSEventStreamRef stream;
    CFAbsoluteTime latency = 3.f;

    FSEventStreamContext context;
    dMemset(&context, 0, sizeof(context));
    context.info = event;

    stream = FSEventStreamCreate(
        NULL,
        &fsNotifyCallback,
        &context,
        paths,
        kFSEventStreamEventIdSinceNow,
        latency,
        kFSEventStreamCreateFlagNone
    );

    event->mStream = stream;
    event->mDir = dir;
    event->mHasChanged = false;

    mEvents.push_back(event);

    // Put it in the run loop and start the stream.
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);

    CFRelease(path);
    CFRelease(paths);

    return true;
}

bool MacFileSystemChangeNotifier::internalRemoveNotification(const Torque::Path& dir)
{
    for (U32 i = 0, num = mEvents.size(); i < num; ++i)
    {
        if (mEvents[i]->mDir == dir)
        {
            FSEventStreamStop(mEvents[i]->mStream);
            FSEventStreamInvalidate(mEvents[i]->mStream);
            FSEventStreamRelease(mEvents[i]->mStream);
            delete mEvents[i];
            mEvents.erase(i);
            return true;
        }
    }

    return false;
}
