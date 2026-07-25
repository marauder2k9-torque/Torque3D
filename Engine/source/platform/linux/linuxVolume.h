//-----------------------------------------------------------------------------
// linuxVolume.h — Linux native Torque::FS::FileSystem implementation
// (LinuxFileSystem/LinuxFile/LinuxDirectory).
//
// Structured the same way as macVolume.h: standalone native classes using
// plain POSIX stat/fopen/opendir primitives directly (the same primitives
// linuxFileIO.cpp uses), not inherited from a shared PosixFileSystem base.
// Kept at global scope for the same reason macVolume.h documents — no
// namespace wrapping.
//
// Unlike macOS (FSEventStream) or Windows (FindFirstChangeNotificationW),
// there is no single native "watch this directory" API here — Linux's
// analog is inotify, which is genuinely native (a syscall family, not an
// external library) but is left as a follow-up rather than included here,
// since none of file/CPU/time/memory/msgbox/volume/font/process-control
// requires it. FileSystemChangeNotifier is intentionally left
// unimplemented (base class default, i.e. no live change notifications)
// rather than stubbed with fake behavior.
//-----------------------------------------------------------------------------
#pragma once

#include "platform/platformTypes.h"
#include "core/volume.h"
#include "core/util/tVector.h"

#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

class LinuxFileSystem;

//-----------------------------------------------------------------------------
class LinuxFile : public Torque::FS::File
{
    friend class LinuxFileSystem;

    Torque::Path _path;
    String _name;
    FILE* _handle;
    Torque::FS::FileNode::NodeStatus _status;

    LinuxFile(const Torque::Path& path, String name)
        : _path(path), _name(std::move(name)), _handle(nullptr), _status(Torque::FS::FileNode::Closed) {}

    void _updateStatus();
    virtual U32 calculateChecksum();

public:
    virtual ~LinuxFile();

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
class LinuxDirectory : public Torque::FS::Directory
{
    friend class LinuxFileSystem;

    Torque::Path _path;
    String _name;
    DIR* _handle;
    Torque::FS::FileNode::NodeStatus _status;

    LinuxDirectory(const Torque::Path& path, String name)
        : _path(path), _name(std::move(name)), _handle(nullptr), _status(Torque::FS::FileNode::Closed) {}

    void _updateStatus();
    virtual U32 calculateChecksum() { return 0; }

public:
    virtual ~LinuxDirectory();

    virtual Torque::Path getName() const { return _path; }
    virtual Torque::FS::FileNode::NodeStatus getStatus() const { return _status; }
    virtual bool getAttributes(Attributes*);

    virtual bool open();
    virtual bool close();
    virtual bool read(Attributes*);
};

//-----------------------------------------------------------------------------
class LinuxFileSystem : public Torque::FS::FileSystem
{
    String _volume;

public:
    LinuxFileSystem(String volume);

    virtual String getTypeStr() const { return "Linux"; }

    virtual Torque::FS::FileNodeRef resolve(const Torque::Path& path);
    virtual Torque::FS::FileNodeRef create(const Torque::Path& path, Torque::FS::FileNode::Mode);
    virtual bool remove(const Torque::Path& path);
    virtual bool rename(const Torque::Path& from, const Torque::Path& to);
    virtual Torque::Path mapTo(const Torque::Path& path);
    virtual Torque::Path mapFrom(const Torque::Path& path);
};
