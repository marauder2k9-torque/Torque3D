//-----------------------------------------------------------------------------
// linuxFileIO.cpp — Linux implementation of the File class (core/fileio.h)
// and the file/directory functions declared in platform.h.
//
// Fully native POSIX (fopen/fstat/stat/opendir/readdir + <unistd.h>) — no
// external library dependency. Mirrors macFileIO.mm's approach closely
// (both are POSIX under the hood) with one Linux-specific improvement:
// getExecutablePath()/getExecutableName() resolve via /proc/self/exe
// (a native Linux kernel interface — always correct, no NSBundle-style
// guessing or main.cs probing needed) rather than the mac port's
// bundle-relative search.
//
// Platform::deleteDirectory, Platform::compareModifiedTimes, and
// Platform::addExcludedDirectory/clearExcludedDirectories/
// isExcludedDirectory are NOT defined here — those are OS-agnostic and
// already live in the generic platformFileIO.cpp/platform.cpp, matching
// the win32/mac ports of this file.
//-----------------------------------------------------------------------------
#include "core/fileio.h"
#include "core/util/tVector.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"
#include "platform/profiler.h"
#include "core/volume.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>

constexpr U32 kMaxLinuxPath = 4096;

//-----------------------------------------------------------------------------
bool dFileDelete(const char *name)
{
    if (!name)
        return false;

    if (dStrlen(name) > kMaxLinuxPath)
        Con::warnf("dFileDelete: Filename length is pretty long...");

    return remove(name) == 0;
}

bool dFileTouch(const char *path)
{
    if (!path || !*path)
        return false;

    return utimes(path, nullptr) == 0;
}

bool dPathCopy(const char *source, const char *dest, bool nooverwrite)
{
    if (!source || !dest)
        return false;

    struct stat srcStat;
    if (stat(source, &srcStat) != 0)
    {
        Con::errorf("dPathCopy: no file exists at %s", source);
        return false;
    }

    struct stat destStat;
    const bool destExists = stat(dest, &destStat) == 0;

    if (destExists)
    {
        if (nooverwrite)
        {
            Con::errorf("dPathCopy: file already exists at %s", dest);
            return false;
        }
        Con::warnf("Deleting files at path: %s", dest);
        if (remove(dest) != 0 && stat(dest, &destStat) == 0)
        {
            Con::errorf("Copy failed! Could not delete files at path: %s", dest);
            return false;
        }
    }

    // Ensure the destination's parent directory exists.
    char destDir[kMaxLinuxPath];
    dStrncpy(destDir, dest, sizeof(destDir));
    destDir[sizeof(destDir) - 1] = '\0';
    char* slash = dStrrchr(destDir, '/');
    if (slash)
    {
        slash[1] = '\0';
        struct stat dirStat;
        if (stat(destDir, &dirStat) != 0)
            Platform::createPath(destDir);
    }

    FILE* in = fopen(source, "rb");
    if (!in)
    {
        Con::errorf("dPathCopy: could not open source %s", source);
        return false;
    }

    FILE* out = fopen(dest, "wb");
    if (!out)
    {
        fclose(in);
        Con::errorf("dPathCopy: could not open destination %s", dest);
        return false;
    }

    constexpr size_t bufSize = 64 * 1024;
    char buffer[bufSize];
    bool ok = true;
    size_t n;
    while ((n = fread(buffer, 1, bufSize, in)) > 0)
    {
        if (fwrite(buffer, 1, n, out) != n)
        {
            ok = false;
            break;
        }
    }
    if (ferror(in))
        ok = false;

    fclose(in);
    fclose(out);

    if (!ok)
    {
        Con::warnf("The copy failed partway through. Something strange is happening");
        remove(dest);
    }

    return ok;
}

bool dFileRename(const char *source, const char *dest)
{
    if (!source || !dest)
        return false;

    struct stat srcStat;
    if (stat(source, &srcStat) != 0)
    {
        Con::errorf("dFileRename: no file exists at %s", source);
        return false;
    }

    struct stat destStat;
    if (stat(dest, &destStat) == 0)
        Con::warnf("dFileRename: Deleting files at path: %s", dest);

    const bool ret = rename(source, dest) == 0;
    if (!ret)
        Con::warnf("The rename failed. Something strange is happening");

    return ret;
}

//-----------------------------------------------------------------------------
// File
//-----------------------------------------------------------------------------
File::File()
    : currentStatus(Closed), capability(0)
{
    handle = nullptr;
}

File::~File()
{
    close();
    handle = nullptr;
}

File::FileStatus File::open(const char *filename, const AccessMode openMode)
{
    if (dStrlen(filename) > kMaxLinuxPath)
        Con::warnf("File::open: Filename length is pretty long...");

    if (currentStatus != Closed)
        close();

    switch (openMode)
    {
        case Read:        handle = static_cast<void*>(fopen(filename, "rb"));  break;
        case Write:       handle = static_cast<void*>(fopen(filename, "wb"));  break;
        case ReadWrite:    handle = static_cast<void*>(fopen(filename, "ab+")); break;
        case WriteAppend: handle = static_cast<void*>(fopen(filename, "ab"));  break;
        default:
            AssertFatal(false, "File::open: bad access mode");
    }

    if (handle == nullptr)
        return setStatus();

    switch (openMode)
    {
        case Read:        capability = FileRead; break;
        case Write:
        case WriteAppend: capability = FileWrite; break;
        case ReadWrite:   capability = FileRead | FileWrite; break;
        default:
            AssertFatal(false, "File::open: bad access mode");
    }

    // Must set status before setPosition(), since setPosition asserts on it.
    currentStatus = Ok;

    if (openMode == ReadWrite)
        setPosition(0);

    return currentStatus;
}

U32 File::getPosition() const
{
    AssertFatal(currentStatus != Closed, "File::getPosition: file closed");
    AssertFatal(handle != nullptr, "File::getPosition: invalid file handle");

    return static_cast<U32>(ftell(static_cast<FILE*>(handle)));
}

File::FileStatus File::setPosition(S32 position, bool absolutePos)
{
    AssertFatal(Closed != currentStatus, "File::setPosition: file closed");
    AssertFatal(handle != nullptr, "File::setPosition: invalid file handle");

    if (currentStatus != Ok && currentStatus != EOS)
        return currentStatus;

    U32 finalPos;
    if (absolutePos)
    {
        AssertFatal(0 <= position, "File::setPosition: negative absolute position");
        fseek(static_cast<FILE*>(handle), position, SEEK_SET);
        finalPos = static_cast<U32>(ftell(static_cast<FILE*>(handle)));
    }
    else
    {
        AssertFatal((static_cast<S32>(getPosition()) + position) >= 0, "File::setPosition: negative relative position");
        fseek(static_cast<FILE*>(handle), position, SEEK_CUR);
        finalPos = static_cast<U32>(ftell(static_cast<FILE*>(handle)));
    }

    if (0xffffffff == finalPos)
        return setStatus();
    if (finalPos >= getSize())
        return currentStatus = EOS;
    return currentStatus = Ok;
}

U32 File::getSize() const
{
    AssertWarn(Closed != currentStatus, "File::getSize: file closed");
    AssertFatal(handle != nullptr, "File::getSize: invalid file handle");

    if (Ok == currentStatus || EOS == currentStatus)
    {
        struct stat statData;
        if (fstat(fileno(static_cast<FILE*>(handle)), &statData) != 0)
            return 0;
        return static_cast<U32>(statData.st_size);
    }

    return 0;
}

File::FileStatus File::flush()
{
    AssertFatal(Closed != currentStatus, "File::flush: file closed");
    AssertFatal(handle != nullptr, "File::flush: invalid file handle");
    AssertFatal(true == hasCapability(FileWrite), "File::flush: cannot flush a read-only file");

    if (fflush(static_cast<FILE*>(handle)) != 0)
        return setStatus();
    return currentStatus = Ok;
}

File::FileStatus File::close()
{
    if (Closed == currentStatus)
        return currentStatus;

    if (handle != nullptr)
    {
        if (fclose(static_cast<FILE*>(handle)) != 0)
            return setStatus();
    }
    handle = nullptr;
    return currentStatus = Closed;
}

File::FileStatus File::getStatus() const
{
    return currentStatus;
}

File::FileStatus File::setStatus()
{
    switch (errno)
    {
        case EACCES:
            currentStatus = IOError;
            break;
        case EBADF:
        case EINVAL:
        case ENOENT:
        case ENAMETOOLONG:
        default:
            currentStatus = UnknownError;
    }

    return currentStatus;
}

File::FileStatus File::setStatus(File::FileStatus status)
{
    return currentStatus = status;
}

File::FileStatus File::read(U32 size, char *dst, U32 *bytesRead)
{
    AssertFatal(Closed != currentStatus, "File::read: file closed");
    AssertFatal(handle != nullptr, "File::read: invalid file handle");
    AssertFatal(nullptr != dst, "File::read: NULL destination pointer");
    AssertFatal(true == hasCapability(FileRead), "File::read: file lacks capability");
    AssertWarn(0 != size, "File::read: size of zero");

    if (Ok != currentStatus || 0 == size)
        return currentStatus;

    U32 nBytes = static_cast<U32>(fread(dst, 1, size, static_cast<FILE*>(handle)));

    if (nBytes != size)
        currentStatus = EOS;

    if (bytesRead)
        *bytesRead = nBytes;

    return currentStatus;
}

File::FileStatus File::write(U32 size, const char *src, U32 *bytesWritten)
{
    AssertFatal(Closed != currentStatus, "File::write: file closed");
    AssertFatal(handle != nullptr, "File::write: invalid file handle");
    AssertFatal(nullptr != src, "File::write: NULL source pointer");
    AssertFatal(true == hasCapability(FileWrite), "File::write: file lacks capability");
    AssertWarn(0 != size, "File::write: size of zero");

    if ((Ok != currentStatus && EOS != currentStatus) || 0 == size)
        return currentStatus;

    U32 nBytes = static_cast<U32>(fwrite(src, 1, size, static_cast<FILE*>(handle)));

    if (nBytes != size)
        setStatus();

    if (bytesWritten)
        *bytesWritten = nBytes;

    return currentStatus;
}

bool File::hasCapability(Capability cap) const
{
    return (0 != (U32(cap) & capability));
}

//-----------------------------------------------------------------------------
S32 Platform::compareFileTimes(const FileTime &a, const FileTime &b)
{
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

//-----------------------------------------------------------------------------
// Either time param may be null.
//-----------------------------------------------------------------------------
bool Platform::getFileTimes(const char *path, FileTime *createTime, FileTime *modifyTime)
{
    // Linux (like macOS) has no universal "file creation time" concept
    // across filesystems — ctime (status change time) is the closest
    // POSIX-portable stand-in, matching the mac port's own documented
    // rationale.
    if (!path || !*path)
        return false;

    struct stat statData;
    if (stat(path, &statData) == -1)
        return false;

    if (createTime)
        *createTime = static_cast<FileTime>(statData.st_ctime);
    if (modifyTime)
        *modifyTime = static_cast<FileTime>(statData.st_mtime);

    return true;
}

bool Platform::stringToFileTime(const char *string, FileTime *time)
{
    if (!time || !string)
        return false;
    *time = static_cast<FileTime>(dAtoi(string));
    return true;
}

bool Platform::fileTimeToString(FileTime *time, char *string, U32 strLen)
{
    if (!time || !string)
        return false;
    dSprintf(string, strLen, "%lld", static_cast<long long>(*time));
    return true;
}

//-----------------------------------------------------------------------------
bool Platform::createPath(const char *file)
{
    struct stat statData;
    if (stat(file, &statData) == 0)
        return true;

    Con::warnf("creating path %s", file);

    U32 len = dStrlen(file);
    char parent[len + 1];
    bool isDirPath = false;

    dStrncpy(parent, file, len);
    parent[len] = '\0';
    if (parent[len - 1] == '/')
    {
        parent[len - 1] = '\0';
        isDirPath = true;
    }

    char* slash = dStrrchr(parent, '/');
    if (slash && slash != parent)
    {
        slash[1] = '\0';
        if (!Platform::createPath(parent))
            return false;
    }

    if (isDirPath)
    {
        if (mkdir(file, 0777) != 0 && errno != EEXIST)
            return false;
    }

    return true;
}

bool Platform::cdFileExists(const char *filePath, const char *volumeName, S32 serialNum)
{
    return true;
}

//-----------------------------------------------------------------------------
StringTableEntry Platform::getCurrentDirectory()
{
    char* cwd = getcwd(nullptr, 0);
    StringTableEntry ret = StringTable->insert(cwd ? cwd : "");
    if (cwd)
        free(cwd);
    return ret;
}

bool Platform::setCurrentDirectory(StringTableEntry newDir)
{
    return chdir(newDir) == 0;
}

//-----------------------------------------------------------------------------
// Executable path resolution via /proc/self/exe — a native Linux kernel
// interface that always resolves to the real, fully-qualified binary
// path (symlinks and all), unlike argv[0] (which can be relative, or a
// bare name resolved via $PATH) or a getcwd()-based guess.
//-----------------------------------------------------------------------------
namespace
{
    char sExecutablePath[kMaxLinuxPath] = "";
    char sExecutableName[kMaxLinuxPath] = "";
    bool sResolved = false;

    void resolveExecutablePath()
    {
        if (sResolved)
            return;
        sResolved = true;

        char buf[kMaxLinuxPath];
        const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len <= 0)
            return;
        buf[len] = '\0';

        char* slash = dStrrchr(buf, '/');
        if (slash)
        {
            *slash = '\0';
            dStrncpy(sExecutablePath, buf, sizeof(sExecutablePath));
            dStrncpy(sExecutableName, slash + 1, sizeof(sExecutableName));
        }
        else
        {
            dStrncpy(sExecutableName, buf, sizeof(sExecutableName));
        }
    }
}

StringTableEntry Platform::getExecutablePath()
{
    resolveExecutablePath();
    return StringTable->insert(sExecutablePath);
}

StringTableEntry Platform::getExecutableName()
{
    resolveExecutablePath();
    return StringTable->insert(sExecutableName);
}

//-----------------------------------------------------------------------------
// User/system directories. XDG Base Directory Specification is the
// native, standard convention on Linux for exactly this (no library
// needed — it's just a set of environment-variable-driven path
// conventions every well-behaved Linux desktop app follows), so these
// use it directly rather than hardcoding ~/.local/share the way an
// older/simpler port might.
//-----------------------------------------------------------------------------
StringTableEntry Platform::getUserHomeDirectory()
{
    if (const char* home = getenv("HOME"))
        return StringTable->insert(home);

    // No $HOME (unusual, e.g. some minimal container/service-account
    // setups) — fall back to the current directory rather than
    // returning an empty/invalid path.
    return Platform::getCurrentDirectory();
}

// Distinct from getUserHomeDirectory(): this returns the per-user
// application-data directory — $XDG_DATA_HOME if set, otherwise the
// XDG-specified default of ~/.local/share — the same role %APPDATA%
// plays on Windows and ~/Library/Application Support/ plays on macOS.
StringTableEntry Platform::getUserDataDirectory()
{
    if (const char* xdgDataHome = getenv("XDG_DATA_HOME"))
    {
        if (*xdgDataHome)
            return StringTable->insert(xdgDataHome);
    }

    const char* home = getenv("HOME");
    if (!home)
        home = "";

    char buf[kMaxLinuxPath];
    dSprintf(buf, sizeof(buf), "%s/.local/share", home);
    return StringTable->insert(buf);
}

bool Platform::getUserIsAdministrator()
{
    // Native, dependency-free check: root (uid 0), or a member of the
    // "sudo"/"wheel" group would also qualify in practice, but group
    // membership requires getgrnam()+getgroups() bookkeeping for a
    // determination that's inherently fuzzier across distros (sudo vs
    // wheel vs doas config) than the Windows/macOS checks this mirrors.
    // Matching those checks' own documented caveat (macOS's is a
    // best-effort /Library write check, not a rigorous privilege audit),
    // uid 0 is the unambiguous, portable case.
    return geteuid() == 0;
}

StringTableEntry osGetTemporaryDirectory()
{
    // $TMPDIR is the POSIX-conventional override; /tmp is the
    // universal Linux default when it's unset.
    if (const char* tmpdir = getenv("TMPDIR"))
    {
        if (*tmpdir)
            return StringTable->insert(tmpdir);
    }
    return StringTable->insert("/tmp");
}

//-----------------------------------------------------------------------------
StringTableEntry Platform::createPlatformFriendlyFilename(const char* filename)
{
    // No filename-legality translation is needed on Linux (unlike, say,
    // sanitizing characters illegal in Win32 paths) -- matches Win32's own
    // implementation, which is likewise a plain passthrough.
    return StringTable->insert(filename);
}

//-----------------------------------------------------------------------------
// Volume/mount enumeration via /proc/self/mountinfo -- a native Linux
// kernel interface (no library dependency) that lists every currently
// mounted filesystem, standing in for the "drive letters" concept
// getVolumeNamesList/getVolumeInformationList expose on Windows. There is
// no meaningful Linux equivalent of a drive "serial number" or
// permanently fixed drive letter, so those VolumeInformation fields are
// filled with the closest honest approximation (0, and a best-effort
// fixed-vs-removable guess via /sys/block's "removable" flag) rather than
// fabricated values.
//-----------------------------------------------------------------------------
namespace
{
    // Best-effort "is this mount on removable media" check via sysfs --
    // looks up the block device backing a mount point (by matching the
    // mount source against /sys/block/<dev>) and reads its "removable"
    // attribute. Falls back to "not removable" (matching bOnlyFixedDrives'
    // conservative default) if the lookup can't be resolved, e.g. for
    // virtual/network filesystems with no backing block device.
    bool isRemovableDevice(const std::string& source)
    {
        auto lastSlash = source.find_last_of('/');
        if (lastSlash == std::string::npos)
            return false;

        std::string devName = source.substr(lastSlash + 1);
        // Strip trailing partition digits (e.g. sda1 -> sda) since
        // /sys/block entries are keyed by whole-disk name.
        while (!devName.empty() && isdigit(static_cast<unsigned char>(devName.back())))
            devName.pop_back();

        if (devName.empty())
            return false;

        char sysPath[256];
        dSprintf(sysPath, sizeof(sysPath), "/sys/block/%s/removable", devName.c_str());

        std::ifstream f(sysPath);
        if (!f.is_open())
            return false;

        int val = 0;
        f >> val;
        return val != 0;
    }
}

void Platform::getVolumeNamesList(Vector<const char*>& out_rNameVector, bool bOnlyFixedDrives)
{
    out_rNameVector.clear();

    std::ifstream mounts("/proc/self/mountinfo");
    if (!mounts.is_open())
    {
        // No /proc (extremely unusual on Linux) -- at minimum, report the
        // root filesystem so callers always see something.
        out_rNameVector.push_back(StringTable->insert("/"));
        return;
    }

    std::string line;
    while (std::getline(mounts, line))
    {
        // mountinfo format: id parentId major:minor root mountPoint
        // options - fsType source superOptions. The mount point is the
        // 5th whitespace-separated field; the source is the 2nd field
        // after the "-" separator.
        std::istringstream iss(line);
        std::vector<std::string> fields;
        std::string field;
        while (iss >> field)
            fields.push_back(field);

        const auto dashIt = std::find(fields.begin(), fields.end(), "-");
        if (dashIt == fields.end() || fields.size() < 5)
            continue;

        const std::string& mountPoint = fields[4];
        const size_t dashIdx = static_cast<size_t>(dashIt - fields.begin());
        if (dashIdx + 2 >= fields.size())
            continue;
        const std::string& source = fields[dashIdx + 2];

        if (bOnlyFixedDrives && isRemovableDevice(source))
            continue;

        out_rNameVector.push_back(StringTable->insert(mountPoint.c_str()));
    }
}

void Platform::getVolumeInformationList(Vector<Platform::VolumeInformation>& out_rVolumeInfoVector, bool bOnlyFixedDrives)
{
    Vector<const char*> mountPoints;
    getVolumeNamesList(mountPoints, bOnlyFixedDrives);

    for (const char* mountPoint : mountPoints)
    {
        Platform::VolumeInformation info;
        dMemset(&info, 0, sizeof(info));

        info.RootPath = StringTable->insert(mountPoint);
        info.Name = StringTable->insert(mountPoint);
        info.FileSystem = StringTable->insert(""); // Would require parsing mountinfo's fsType field per-entry.
        info.SerialNumber = 0;                      // No drive-serial concept on Linux mounts.
        info.Type = DRIVETYPE_FIXED;                 // getVolumeNamesList already filtered removables when requested.
        info.ReadOnly = (access(mountPoint, W_OK) != 0);

        out_rVolumeInfoVector.push_back(info);
    }
}

//-----------------------------------------------------------------------------
bool Platform::isFile(const char *path)

{
    if (!path || !*path)
        return false;

    struct stat statData;
    if (stat(path, &statData) < 0)
        return Torque::FS::IsFile(path);

    return (statData.st_mode & S_IFMT) == S_IFREG;
}

bool Platform::isDirectory(const char *path)
{
    if (!path || !*path)
        return false;

    struct stat statData;
    if (stat(path, &statData) < 0)
        return false;

    return (statData.st_mode & S_IFMT) == S_IFDIR;
}

S32 Platform::getFileSize(const char* pFilePath)
{
    if (!pFilePath || !*pFilePath)
        return 0;

    struct stat statData;
    if (stat(pFilePath, &statData) < 0)
        return 0;

    return static_cast<S32>(statData.st_size);
}

bool Platform::isSubDirectory(const char *pathParent, const char *pathSub)
{
    char fullpath[kMaxLinuxPath];
    dStrcpyl(fullpath, kMaxLinuxPath, pathParent, "/", pathSub, nullptr);
    return isDirectory(static_cast<const char*>(fullpath));
}

namespace
{
    bool isGoodDirectory(dirent* entry)
    {
        return entry->d_type == DT_DIR
            && dStrcmp(entry->d_name, ".") != 0
            && dStrcmp(entry->d_name, "..") != 0
            && !Platform::isExcludedDirectory(entry->d_name);
    }
}

bool Platform::hasSubDirectory(const char *path)
{
    DIR* dir = opendir(path);
    if (!dir)
        return false;

    dirent* entry;
    while ((entry = readdir(dir)))
    {
        if (isGoodDirectory(entry))
        {
            closedir(dir);
            return true;
        }
    }

    closedir(dir);
    return false;
}

bool Platform::fileDelete(const char *name)
{
    return dFileDelete(name);
}

//-----------------------------------------------------------------------------
namespace
{
    bool recurseDumpDirectories(const char *basePath, const char *subPath, Vector<StringTableEntry> &directoryVector, S32 currentDepth, S32 recurseDepth, bool noBasePath)
    {
        char path[1024];
        DIR* dip;
        struct dirent* d;

        dsize_t trLen = basePath ? dStrlen(basePath) : 0;
        dsize_t subtrLen = subPath ? dStrlen(subPath) : 0;
        char trail = trLen > 0 ? basePath[trLen - 1] : '\0';
        char subTrail = subtrLen > 0 ? subPath[subtrLen - 1] : '\0';

        if (subPath && dStrncmp(subPath, "", 1) != 0)
            dSprintf(path, 1024, subTrail == '/' ? "%s%s" : "%s%s/", basePath, subPath);
        else
            dSprintf(path, 1024, trail == '/' ? "%s" : "%s/", basePath);

        dip = opendir(path);
        if (dip == nullptr)
            return false;

        if (!Platform::isExcludedDirectory(subPath))
        {
            if (noBasePath)
            {
                if (subPath && dStrncmp(subPath, "", 1) != 0)
                    directoryVector.push_back(StringTable->insert(subPath));
            }
            else
            {
                if (subPath && dStrncmp(subPath, "", 1) != 0)
                {
                    char szPath[1024];
                    dMemset(szPath, 0, 1024);
                    if (basePath[dStrlen(basePath) - 1] != '/')
                        dSprintf(szPath, 1024, subPath[0] == '/' ? "%s%s" : "%s/%s", basePath, subPath);
                    else
                        dSprintf(szPath, 1024, subPath[0] == '/' ? "%s%s" : "%s%s", basePath, subPath);
                    directoryVector.push_back(StringTable->insert(szPath));
                }
                else
                {
                    directoryVector.push_back(StringTable->insert(basePath));
                }
            }
        }

        while ((d = readdir(dip)))
        {
            bool isDir = false;
            if (d->d_type == DT_UNKNOWN)
            {
                char child[1024];
                if (path[dStrlen(path) - 1] == '/')
                    dSprintf(child, 1024, "%s%s", path, d->d_name);
                else
                    dSprintf(child, 1024, "%s/%s", path, d->d_name);
                isDir = Platform::isDirectory(child);
            }
            else if (d->d_type & DT_DIR)
            {
                isDir = true;
            }

            if (isDir)
            {
                if (dStrcmp(d->d_name, ".") == 0 || dStrcmp(d->d_name, "..") == 0)
                    continue;
                if (Platform::isExcludedDirectory(d->d_name))
                    continue;

                char child[1024];
                if (subPath && dStrncmp(subPath, "", 1) != 0)
                {
                    if (subPath[dStrlen(subPath) - 1] == '/')
                        dSprintf(child, 1024, "%s%s", subPath, d->d_name);
                    else
                        dSprintf(child, 1024, "%s/%s", subPath, d->d_name);
                }
                else
                {
                    if (basePath[dStrlen(basePath) - 1] == '/')
                        dStrcpy(child, d->d_name, 1024);
                    else
                        dSprintf(child, 1024, "/%s", d->d_name);
                }

                if (currentDepth < recurseDepth || recurseDepth == -1)
                    recurseDumpDirectories(basePath, child, directoryVector, currentDepth + 1, recurseDepth, noBasePath);
            }
        }
        closedir(dip);
        return true;
    }
}

bool Platform::dumpDirectories(const char *path, Vector<StringTableEntry> &directoryVector, S32 depth, bool noBasePath)
{
    bool retVal = recurseDumpDirectories(path, "", directoryVector, -1, depth, noBasePath);
    clearExcludedDirectories();
    return retVal;
}

//-----------------------------------------------------------------------------
namespace
{
    bool recurseDumpPath(const char* curPath, Vector<Platform::FileInfo>& fileVector, S32 depth)
    {
        DIR* dir = opendir(curPath);
        if (!dir)
            return false;

        dirent* entry;
        while ((entry = readdir(dir)))
        {
            if (dStrcmp(entry->d_name, ".") == 0 || dStrcmp(entry->d_name, "..") == 0)
                continue;

            const size_t len = dStrlen(curPath) + dStrlen(entry->d_name) + 2;
            char pathbuf[len];
            dSprintf(pathbuf, len, "%s/%s", curPath, entry->d_name);
            pathbuf[len - 1] = '\0';

            bool isDir = (entry->d_type == DT_DIR);
            if (entry->d_type == DT_UNKNOWN)
                isDir = Platform::isDirectory(pathbuf);

            if (isDir)
            {
                if (depth == 0)
                    continue;
                if (Platform::isExcludedDirectory(entry->d_name))
                    continue;
                recurseDumpPath(pathbuf, fileVector, depth < 0 ? depth : depth - 1);
            }
            else
            {
                const U32 fileSize = Platform::getFileSize(pathbuf);
                fileVector.increment();
                Platform::FileInfo& rInfo = fileVector.last();
                rInfo.pFullPath = StringTable->insert(curPath);
                rInfo.pFileName = StringTable->insert(entry->d_name);
                rInfo.fileSize  = fileSize;
            }
        }
        closedir(dir);
        return true;
    }
}

bool Platform::dumpPath(const char *path, Vector<Platform::FileInfo>& fileVector, S32 depth)
{
    PROFILE_START(dumpPath);

    const size_t len = dStrlen(path);
    char newpath[len + 1];

    dStrncpy(newpath, path, len);
    newpath[len] = '\0';
    if (len > 0 && newpath[len - 1] == '/')
        newpath[len - 1] = '\0';

    bool ret = recurseDumpPath(newpath, fileVector, depth);

    PROFILE_END();
    return ret;
}
