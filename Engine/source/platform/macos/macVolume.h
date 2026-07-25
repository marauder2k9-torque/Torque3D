//-----------------------------------------------------------------------------
// macVolume.h — macOS native Torque::FS::FileSystem implementation
// (MacFileSystem/MacFile/MacDirectory) plus MacFileSystemChangeNotifier
// (native FSEventStream-based directory change notifications).
//
// STRUCTURE NOTE: deliberately at GLOBAL scope, matching the original
// macVolume.h exactly (no namespace wrapping) — a prior revision of this
// file wrapped everything in "namespace Torque { using namespace FS;
// namespace Mac { ... } }", which is a real structural departure from the
// original's proven-working layout and is the most likely actual cause
// of a persistent, hard-to-pin-down compile failure that survived several
// rounds of CoreServices/FastDelegate include-order fixes. Matching the
// original's flat structure removes that variable entirely. Torque::Path/
// Torque::FS::File/etc. are referenced fully-qualified below, exactly as
// the original macVolume.mm did (e.g. "Torque::Path& dir", not a `using
// namespace FS;` block), rather than reintroducing the same risk.
//
// CONTENT NOTE: the original macVolume.h had MacFileSystem inherit
// directly from Torque::Posix::PosixFileSystem (#import
// "platformPOSIX/posixVolume.h") — the "mac shares code with the
// POSIX/Linux layer" coupling this rewrite has been removing throughout.
// All of the actual file/directory operations (resolve/create/remove/
// rename/mapTo/mapFrom, plus the File/Directory subclasses themselves)
// were inherited from PosixFileSystem and never had a native mac
// implementation at all. This file still provides one — MacFileSystem/
// MacFile/MacDirectory are real, standalone classes using the same POSIX
// stat/fopen/opendir primitives macFileIO.mm already uses directly — that
// part of the earlier rewrite was correct and is kept; only the namespace
// wrapping is reverted.
//
// MacFileSystemChangeNotifier (real FSEventStream-based live directory
// watching) is preserved from the original — it's real, working,
// dependency-free native functionality with no equivalent Linux ever had
// in this rewrite, not something to drop.
//-----------------------------------------------------------------------------
#pragma once

#include "platform/platformTypes.h"
#include "core/volume.h"
#include "core/util/tVector.h"

#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

// Deliberately NOT including <CoreServices/CoreServices.h> here — that
// include lives only in macVolumeChangeNotifier.mm (see that file's
// header comment). Nothing in this header's own declarations needs it:
// Event is only forward-declared below (as
// Torque::FS::FileSystemChangeNotifier requires), with its real
// definition, including the FSEventStreamRef member, living entirely in
// that isolated .mm.

class MacFileSystem;

//-----------------------------------------------------------------------------
/// File system change notifications on macOS, via FSEventStream.
class MacFileSystemChangeNotifier : public Torque::FS::FileSystemChangeNotifier
{
public:
    typedef Torque::FS::FileSystemChangeNotifier Parent;

    struct Event;

protected:
    /// Pointers to heap-allocated Events, so we can pass them around
    /// (e.g. into the FSEventStream callback's context) without them
    /// being invalidated by Vector reallocation.
    Vector<Event*> mEvents;

    virtual void internalProcessOnce();
    virtual bool internalAddNotification(const Torque::Path& dir);
    virtual bool internalRemoveNotification(const Torque::Path& dir);

public:
    MacFileSystemChangeNotifier(MacFileSystem* fs);
    virtual ~MacFileSystemChangeNotifier();
};

//-----------------------------------------------------------------------------
class MacFile : public Torque::FS::File
{
    friend class MacFileSystem;

    Torque::Path _path;
    String _name;
    FILE* _handle;
    Torque::FS::FileNode::NodeStatus _status;

    MacFile(const Torque::Path& path, String name)
        : _path(path), _name(std::move(name)), _handle(nullptr), _status(Torque::FS::FileNode::Closed) {}

    void _updateStatus();
    virtual U32 calculateChecksum();

public:
    virtual ~MacFile();

    virtual Torque::Path getName() const { return _path; }
    virtual Torque::FS::FileNode::NodeStatus getStatus() const { return _status; }
    virtual bool getAttributes(Attributes*);

    virtual U32 getPosition();
    virtual U32 setPosition(U32, SeekMode);

    virtual bool open(AccessMode);
    virtual bool close();

    virtual U32 read(void* dst, U32 size);
    virtual U32 write(const void* src, U32 size);
};

//-----------------------------------------------------------------------------
class MacDirectory : public Torque::FS::Directory
{
    friend class MacFileSystem;

    Torque::Path _path;
    String _name;
    DIR* _handle;
    Torque::FS::FileNode::NodeStatus _status;

    MacDirectory(const Torque::Path& path, String name)
        : _path(path), _name(std::move(name)), _handle(nullptr), _status(Torque::FS::FileNode::Closed) {}

    void _updateStatus();
    virtual U32 calculateChecksum() { return 0; }

public:
    virtual ~MacDirectory();

    virtual Torque::Path getName() const { return _path; }
    virtual Torque::FS::FileNode::NodeStatus getStatus() const { return _status; }
    virtual bool getAttributes(Attributes*);

    virtual bool open();
    virtual bool close();
    virtual bool read(Attributes*);
};

//-----------------------------------------------------------------------------
class MacFileSystem : public Torque::FS::FileSystem
{
    String _volume;

public:
    MacFileSystem(String volume);

    virtual String getTypeStr() const { return "Mac"; }

    virtual Torque::FS::FileNodeRef resolve(const Torque::Path& path);
    virtual Torque::FS::FileNodeRef create(const Torque::Path& path, Torque::FS::FileNode::Mode);
    virtual bool remove(const Torque::Path& path);
    virtual bool rename(const Torque::Path& from, const Torque::Path& to);
    virtual Torque::Path mapTo(const Torque::Path& path);
    virtual Torque::Path mapFrom(const Torque::Path& path);
};
