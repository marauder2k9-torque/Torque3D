//-----------------------------------------------------------------------------
// linuxVolume.cpp — Linux native FileSystem implementation, plus
// Platform::FS::InstallFileSystems() (VFS mount bring-up).
//
// At global scope, matching macVolume.mm's structure (see linuxVolume.h's
// header comment). Torque::Path/Torque::FS::* referenced fully-qualified
// throughout.
//
// InstallFileSystems() mirrors the Windows/macOS "only the raw,
// unrestricted root mount is security sensitive" fix: TORQUE_SECURE_VFS
// gates just the "/" root mount (equivalent to mounting every drive letter
// on Windows), while cwd setup and the home:/ mount still run
// unconditionally, since other engine code depends on both having
// happened by this point regardless of secure-VFS mode.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformVolume.h"
#include "platform/linux/linuxVolume.h"
#include "core/crc.h"
#include "core/frameAllocator.h"
#include "core/util/str.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"

#include <errno.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>

using Torque::FS::FileNode;
using Torque::FS::FileNodeRef;
using Torque::FS::File;
using Torque::FS::Directory;

namespace
{
    String buildFileName(const String& prefix, const Torque::Path& path)
    {
        String file = prefix;
        file = Torque::Path::Join(file, '/', path.getPath());
        file = Torque::Path::Join(file, '/', path.getFileName());

        if (!path.getExtension().isEmpty())
            file = Torque::Path::Join(file, '.', path.getExtension());

        return file;
    }

    void fillAttributesFromStat(const struct stat& st, FileNode::Attributes* attr)
    {
        attr->flags = 0;
        if (S_ISDIR(st.st_mode)) attr->flags |= FileNode::Directory;
        if (S_ISREG(st.st_mode)) attr->flags |= FileNode::File;

        attr->size = static_cast<U64>(st.st_size);
        // Linux, like macOS, has no universal "file creation time"
        // concept — ctime stands in for it, matching linuxFileIO.mm's
        // convention.
        attr->mtime = Torque::UnixTimeToTime(static_cast<U32>(st.st_mtime));
        attr->atime = Torque::UnixTimeToTime(static_cast<U32>(st.st_atime));
        attr->ctime = Torque::UnixTimeToTime(static_cast<U32>(st.st_ctime));
    }
}

//-----------------------------------------------------------------------------
// LinuxFile
//-----------------------------------------------------------------------------
LinuxFile::~LinuxFile()
{
    if (_handle)
        close();
}

void LinuxFile::_updateStatus()
{
    switch (errno)
    {
        case EACCES:  _status = FileNode::AccessDenied;   break;
        case ENOSPC:  _status = FileNode::FileSystemFull; break;
        case ENOTDIR: _status = FileNode::NoSuchFile;      break;
        case ENOENT:  _status = FileNode::NoSuchFile;      break;
        case EISDIR:  _status = FileNode::AccessDenied;    break;
        case EROFS:   _status = FileNode::AccessDenied;    break;
        default:      _status = FileNode::UnknownError;    break;
    }
}

U32 LinuxFile::calculateChecksum()
{
    if (!open(Read))
        return 0;

    U64 remaining = getSize();
    constexpr U32 bufSize = 4 * 1024 * 1024;
    FrameTemp<U8> buf(bufSize);
    U32 crc = CRC::INITIAL_CRC_VALUE;

    while (remaining > 0)
    {
        const U32 chunk = static_cast<U32>(getMin<U64>(remaining, bufSize));
        if (read(buf, chunk) != chunk)
        {
            close();
            return 0;
        }
        remaining -= chunk;
        crc = CRC::calculateCRC(buf, chunk, crc);
    }

    close();
    return crc;
}

bool LinuxFile::getAttributes(Attributes* attr)
{
    struct stat st{};
    const int result = _handle ? fstat(fileno(_handle), &st) : stat(_name.c_str(), &st);
    if (result < 0)
    {
        _updateStatus();
        return false;
    }
    fillAttributesFromStat(st, attr);
    attr->name = _path;
    return true;
}

bool LinuxFile::open(AccessMode mode)
{
    close();

    if (_name.isEmpty())
        return false;

    const char* fmode = "rb";
    switch (mode)
    {
        case Read:        fmode = "rb"; break;
        case Write:       fmode = "wb"; break;
        case WriteAppend: fmode = "ab"; break;
        case ReadWrite:
        {
            // r+b requires the file to already exist — touch it via
            // append-open first if it doesn't.
            if (FILE* probe = fopen(_name.c_str(), "ab"))
                fclose(probe);
            fmode = "r+b";
            break;
        }
    }

    _handle = fopen(_name.c_str(), fmode);
    if (!_handle)
    {
        _updateStatus();
        return false;
    }

    _status = FileNode::Open;
    return true;
}

bool LinuxFile::close()
{
    if (_handle)
    {
        fflush(_handle);
        fclose(_handle);
        _handle = nullptr;
    }
    _status = FileNode::Closed;
    return true;
}

U32 LinuxFile::getPosition()
{
    return (_status == FileNode::Open || _status == FileNode::EndOfFile) ? static_cast<U32>(ftell(_handle)) : 0;
}

U32 LinuxFile::setPosition(U32 delta, SeekMode mode)
{
    if (_status != FileNode::Open && _status != FileNode::EndOfFile)
        return 0;

    int whence = SEEK_SET;
    switch (mode)
    {
        case Begin:   whence = SEEK_SET; break;
        case Current: whence = SEEK_CUR; break;
        case End:     whence = SEEK_END; break;
    }

    if (fseek(_handle, static_cast<long>(delta), whence) != 0)
    {
        _status = FileNode::UnknownError;
        return 0;
    }
    _status = FileNode::Open;
    return static_cast<U32>(ftell(_handle));
}

U32 LinuxFile::read(void* dst, U32 size)
{
    if (_status != FileNode::Open && _status != FileNode::EndOfFile)
        return 0;

    const U32 bytesRead = static_cast<U32>(fread(dst, 1, size, _handle));
    if (bytesRead != size)
    {
        if (feof(_handle)) _status = FileNode::EndOfFile;
        else _updateStatus();
    }
    return bytesRead;
}

U32 LinuxFile::write(const void* src, U32 size)
{
    if ((_status != FileNode::Open && _status != FileNode::EndOfFile) || size == 0)
        return 0;

    const U32 bytesWritten = static_cast<U32>(fwrite(src, 1, size, _handle));
    if (bytesWritten != size)
        _updateStatus();
    return bytesWritten;
}

//-----------------------------------------------------------------------------
// LinuxDirectory
//-----------------------------------------------------------------------------
LinuxDirectory::~LinuxDirectory()
{
    if (_handle)
        close();
}

void LinuxDirectory::_updateStatus()
{
    switch (errno)
    {
        case EACCES:  _status = FileNode::AccessDenied; break;
        case ENOTDIR: _status = FileNode::NoSuchFile;   break;
        case ENOENT:  _status = FileNode::NoSuchFile;   break;
        default:      _status = FileNode::UnknownError; break;
    }
}

bool LinuxDirectory::getAttributes(Attributes* attr)
{
    struct stat st{};
    if (stat(_name.c_str(), &st) != 0)
    {
        _updateStatus();
        return false;
    }
    fillAttributesFromStat(st, attr);
    attr->name = _path;
    return true;
}

bool LinuxDirectory::open()
{
    _handle = opendir(_name.c_str());
    if (!_handle)
    {
        _updateStatus();
        return false;
    }
    _status = FileNode::Open;
    return true;
}

bool LinuxDirectory::close()
{
    if (_handle)
    {
        closedir(_handle);
        _handle = nullptr;
        return true;
    }
    return false;
}

bool LinuxDirectory::read(Attributes* entry)
{
    if (_status != FileNode::Open)
        return false;

    dirent* de = readdir(_handle);
    if (!de)
    {
        _status = FileNode::EndOfFile;
        return false;
    }

    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
        return read(entry);

    struct stat st{};
    const std::string full = std::string(_name.c_str()) + "/" + de->d_name;
    if (stat(full.c_str(), &st) != 0)
    {
        _updateStatus();
        return false;
    }

    fillAttributesFromStat(st, entry);
    entry->name = de->d_name;
    return true;
}

//-----------------------------------------------------------------------------
// LinuxFileSystem
//-----------------------------------------------------------------------------
LinuxFileSystem::LinuxFileSystem(String volume)
    : _volume(std::move(volume))
{
    // No FileSystemChangeNotifier is installed here — see linuxVolume.h's
    // header comment on inotify being a follow-up rather than in scope
    // right now. mChangeNotifier stays null (the base class's default),
    // so directory-change notifications simply aren't delivered on Linux
    // yet, rather than being faked.
}

FileNodeRef LinuxFileSystem::resolve(const Torque::Path& path)
{
    const String fullPath = buildFileName(_volume, path);

    struct stat st{};
    if (stat(fullPath.c_str(), &st) != 0)
        return nullptr;

    if (S_ISREG(st.st_mode))
        return new LinuxFile(path, fullPath);
    if (S_ISDIR(st.st_mode))
        return new LinuxDirectory(path, fullPath);

    return nullptr;
}

FileNodeRef LinuxFileSystem::create(const Torque::Path& path, FileNode::Mode mode)
{
    const String fullPath = buildFileName(_volume, path);

    if (mode & FileNode::File)
        return new LinuxFile(path, fullPath);

    if (mode & FileNode::Directory)
    {
        if (mkdir(fullPath.c_str(), 0777) == 0)
            return new LinuxDirectory(path, fullPath);
    }

    return nullptr;
}

bool LinuxFileSystem::remove(const Torque::Path& path)
{
    const String fullPath = buildFileName(_volume, path);

    struct stat st{};
    if (stat(fullPath.c_str(), &st) != 0)
        return false;

    if (S_ISDIR(st.st_mode))
        return ::rmdir(fullPath.c_str()) == 0;

    return ::unlink(fullPath.c_str()) == 0;
}

bool LinuxFileSystem::rename(const Torque::Path& from, const Torque::Path& to)
{
    const String fa = buildFileName(_volume, from);
    const String fb = buildFileName(_volume, to);
    return ::rename(fa.c_str(), fb.c_str()) == 0;
}

Torque::Path LinuxFileSystem::mapTo(const Torque::Path& path)
{
    return Torque::Path(buildFileName(_volume, path));
}

Torque::Path LinuxFileSystem::mapFrom(const Torque::Path& path)
{
    const String full = path.getFullPath();
    const String::SizeType volumeLen = _volume.length();

    if (_volume.compare(full, volumeLen, String::NoCase) != 0)
        return Torque::Path();

    return Torque::Path(full.substr(volumeLen, full.length() - volumeLen));
}

//-----------------------------------------------------------------------------
// Platform::FS entry points
//-----------------------------------------------------------------------------
bool Torque::FS::VerifyWriteAccess(const Torque::Path &path)
{
    // A real create/write/read/verify/delete round trip, matching the
    // logic already used on Windows/macOS.
    String temp = path.getFullPath();
    temp += "/torque_write_test.tmp";

    ::unlink(temp.c_str());

    const int fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return false;

    const U32 t = Platform::getTime();
    if (::write(fd, &t, sizeof(t)) != static_cast<ssize_t>(sizeof(t)))
    {
        ::close(fd);
        ::unlink(temp.c_str());
        return false;
    }

    ::close(fd);

    const int readFD = ::open(temp.c_str(), O_RDONLY);
    if (readFD < 0)
    {
        ::unlink(temp.c_str());
        return false;
    }

    U32 t2 = 0;
    const bool readOK = ::read(readFD, &t2, sizeof(t2)) == static_cast<ssize_t>(sizeof(t2));
    ::close(readFD);
    ::unlink(temp.c_str());

    return readOK && (t == t2);
}

Torque::FS::FileSystemRef Platform::FS::createNativeFS(const String &volume)
{
    return new LinuxFileSystem(volume);
}

String Platform::FS::getAssetDir()
{
    return Platform::getExecutablePath();
}

bool Platform::FS::InstallFileSystems()
{
    // Only the raw, unrestricted OS-root mount is security sensitive —
    // that's what TORQUE_SECURE_VFS exists to prevent (see the identical
    // Windows/macOS fix for the reasoning). cwd setup and home:/ mounting
    // still run unconditionally.
#ifndef TORQUE_SECURE_VFS
    Platform::FS::Mount("/", Platform::FS::createNativeFS(String()));
#endif

    char buffer[PATH_MAX];
    if (::getcwd(buffer, sizeof(buffer)))
    {
        std::string cwdStr(buffer);
        if (cwdStr.empty() || cwdStr.back() != '/')
            cwdStr += '/';
        Platform::FS::SetCwd(cwdStr.c_str());
    }

    return true;
}
