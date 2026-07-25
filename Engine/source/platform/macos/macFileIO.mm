//-----------------------------------------------------------------------------
// macFileIO.mm — macOS implementation of the File class (core/fileio.h)
// and the file/directory functions declared in platform.h.
//
// Ported from macFileIO.mm. Real, working native code (raw POSIX
// fopen/fstat/stat for File and directory scans, NSFileManager for
// copy/rename, NSBundle for executable path resolution) — no external
// dependency, kept largely as-is.
//
// Changes from the original:
//   - Platform::deleteDirectory, Platform::compareModifiedTimes,
//     Platform::addExcludedDirectory/clearExcludedDirectories/
//     isExcludedDirectory are NOT defined here. These have no OS-specific
//     content and are already owned by the generic platformFileIO.cpp/
//     platform.cpp layer — this is the same duplicate-definition mistake
//     already caught and fixed on the Windows and Linux ports of this
//     file (see win32FileIO.cpp/linuxFileIO.cpp's header comments for the
//     LNK2005 history).
//   - Platform::stringToFileTime / Platform::fileTimeToString were TODO
//     stubs in the original (`return false` unconditionally). Implemented
//     properly here, matching the Linux/Windows ports of these functions.
//-----------------------------------------------------------------------------
#import <Cocoa/Cocoa.h>
#import <stdio.h>
#import <stdlib.h>
#import <errno.h>
#import <utime.h>
#import <sys/time.h>
#import <sys/types.h>
#import <dirent.h>
#import <unistd.h>
#import <sys/stat.h>

#import "core/fileio.h"
#import "core/util/tVector.h"
#import "core/stringTable.h"
#import "core/strings/stringFunctions.h"
#import "console/console.h"
#import "platform/profiler.h"
#import "core/volume.h"

constexpr U32 kMaxMacPathLong = 2048;

//-----------------------------------------------------------------------------
bool dFileDelete(const char *name)
{
    if (!name)
        return false;

    if (dStrlen(name) > kMaxMacPathLong)
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

    @autoreleasepool {
        NSFileManager* manager = [NSFileManager defaultManager];

        NSString* nsource = [manager stringWithFileSystemRepresentation:source length:dStrlen(source)];
        NSString* ndest   = [manager stringWithFileSystemRepresentation:dest length:dStrlen(dest)];
        NSString* ndestFolder = [ndest stringByDeletingLastPathComponent];

        if (![manager fileExistsAtPath:nsource])
        {
            Con::errorf("dPathCopy: no file exists at %s", source);
            return false;
        }

        if ([manager fileExistsAtPath:ndest])
        {
            if (nooverwrite)
            {
                Con::errorf("dPathCopy: file already exists at %s", dest);
                return false;
            }
            Con::warnf("Deleting files at path: %s", dest);
            if (![manager removeItemAtPath:ndest error:nil] || [manager fileExistsAtPath:ndest])
            {
                Con::errorf("Copy failed! Could not delete files at path: %s", dest);
                return false;
            }
        }

        if ([manager fileExistsAtPath:ndestFolder] == NO)
        {
            ndestFolder = [ndestFolder stringByAppendingString:@"/"];
            Platform::createPath([ndestFolder UTF8String]);
        }

        bool ret = [manager copyItemAtPath:nsource toPath:ndest error:nil];
        if (![manager fileExistsAtPath:ndest])
        {
            Con::warnf("The filemanager returned success, but the file was not copied. Something strange is happening");
            ret = false;
        }
        return ret;
    }
}

bool dFileRename(const char *source, const char *dest)
{
    if (!source || !dest)
        return false;

    @autoreleasepool {
        NSFileManager* manager = [NSFileManager defaultManager];

        NSString* nsource = [manager stringWithFileSystemRepresentation:source length:dStrlen(source)];
        NSString* ndest   = [manager stringWithFileSystemRepresentation:dest length:dStrlen(dest)];

        if (![manager fileExistsAtPath:nsource])
        {
            Con::errorf("dFileRename: no file exists at %s", source);
            return false;
        }

        if ([manager fileExistsAtPath:ndest])
            Con::warnf("dFileRename: Deleting files at path: %s", dest);

        bool ret = [manager moveItemAtPath:nsource toPath:ndest error:nil];
        if (![manager fileExistsAtPath:ndest])
        {
            Con::warnf("The filemanager returned success, but the file was not moved. Something strange is happening");
            ret = false;
        }

        return ret;
    }
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
    if (dStrlen(filename) > kMaxMacPathLong)
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
    // macOS isn't guaranteed to be running off an HFS+/APFS volume, and
    // POSIX doesn't keep a record of a file's creation time anywhere
    // universal — so, matching the Linux port, ctime (status change time)
    // stands in for creation time.
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
        if (mkdir(file, 0777) != 0)
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
    StringTableEntry ret = StringTable->insert(cwd);
    free(cwd);
    return ret;
}

bool Platform::setCurrentDirectory(StringTableEntry newDir)
{
    return chdir(newDir) == 0;
}

//-----------------------------------------------------------------------------
namespace
{
    bool isMainDotCsPresent(NSString* dir)
    {
       NSString* filename = [@"main." stringByAppendingString:@TORQUE_SCRIPT_EXTENSION];
       return [[NSFileManager defaultManager] fileExistsAtPath:[dir stringByAppendingPathComponent:filename]] == YES;
       
    }
}

StringTableEntry Platform::getExecutablePath()
{
    static const char* cwd = nullptr;

    if (!cwd)
    {
        @autoreleasepool {
            char buf[4096];
            NSString* currentDir = [[NSString alloc] initWithUTF8String:getcwd(buf, sizeof(buf))];

            if (isMainDotCsPresent(currentDir))
            {
                cwd = dStrdup(buf);
                return cwd;
            }

            NSString* string = [[NSBundle mainBundle] pathForResource:@"main" ofType:@TORQUE_SCRIPT_EXTENSION];
            if (!string)
                string = [[NSBundle mainBundle] bundlePath];

            string = [string stringByDeletingLastPathComponent];
            AssertISV(isMainDotCsPresent(string), "Platform::getExecutablePath - Failed to find main.cs!");
            cwd = dStrdup([string UTF8String]);
            chdir(cwd);
        }
    }

    return cwd;
}

StringTableEntry Platform::getExecutableName()
{
    static const char* name = nullptr;
    if (!name)
    {
        @autoreleasepool {
            name = dStrdup([[[[NSBundle mainBundle] bundlePath] lastPathComponent] UTF8String]);
        }
    }
    return name;
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
    char fullpath[kMaxMacPathLong];
    dStrcpyl(fullpath, kMaxMacPathLong, pathParent, "/", pathSub, nullptr);
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
        char Path[1024];
        DIR* dip;
        struct dirent* d;

        dsize_t trLen = basePath ? dStrlen(basePath) : 0;
        dsize_t subtrLen = subPath ? dStrlen(subPath) : 0;
        char trail = trLen > 0 ? basePath[trLen - 1] : '\0';
        char subTrail = subtrLen > 0 ? subPath[subtrLen - 1] : '\0';

        if (trail == '/')
        {
            if (subPath && dStrncmp(subPath, "", 1) != 0)
                dSprintf(Path, 1024, subTrail == '/' ? "%s%s" : "%s%s/", basePath, subPath);
            else
                dSprintf(Path, 1024, "%s", basePath);
        }
        else
        {
            if (subPath && dStrncmp(subPath, "", 1) != 0)
                dSprintf(Path, 1024, subTrail == '/' ? "%s%s" : "%s%s/", basePath, subPath);
            else
                dSprintf(Path, 1024, "%s/", basePath);
        }

        dip = opendir(Path);
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
                    if (trail == '/')
                    {
                        if (basePath[dStrlen(basePath) - 1] != '/')
                            dSprintf(szPath, 1024, "%s%s", basePath, &subPath[1]);
                        else
                            dSprintf(szPath, 1024, "%s%s", basePath, subPath);
                    }
                    else
                    {
                        if (basePath[dStrlen(basePath) - 1] != '/')
                            dSprintf(szPath, 1024, "%s%s", basePath, subPath);
                        else
                            dSprintf(szPath, 1024, "%s/%s", basePath, subPath);
                    }
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
                if (Path[dStrlen(Path) - 1] == '/')
                    dSprintf(child, 1024, "%s%s", Path, d->d_name);
                else
                    dSprintf(child, 1024, "%s/%s", Path, d->d_name);
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

                if (subPath && dStrncmp(subPath, "", 1) != 0)
                {
                    char child[1024];
                    if (subPath[dStrlen(subPath) - 1] == '/')
                        dSprintf(child, 1024, "%s%s", subPath, d->d_name);
                    else
                        dSprintf(child, 1024, "%s/%s", subPath, d->d_name);
                    if (currentDepth < recurseDepth || recurseDepth == -1)
                        recurseDumpDirectories(basePath, child, directoryVector, currentDepth + 1, recurseDepth, noBasePath);
                }
                else
                {
                    char child[1024];
                    if (basePath[dStrlen(basePath) - 1] == '/')
                        dStrcpy(child, d->d_name, 1024);
                    else
                        dSprintf(child, 1024, "/%s", d->d_name);
                    if (currentDepth < recurseDepth || recurseDepth == -1)
                        recurseDumpDirectories(basePath, child, directoryVector, currentDepth + 1, recurseDepth, noBasePath);
                }
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
    bool recurseDumpPath(const char* curPath, Vector<Platform::FileInfo>& fileVector, U32 depth)
    {
        DIR* dir = opendir(curPath);
        if (!dir)
            return false;

        dirent* entry;
        while ((entry = readdir(dir)))
        {
            const U32 len = dStrlen(curPath) + entry->d_namlen + 2;
            char pathbuf[len];
            dSprintf(pathbuf, len, "%s/%s", curPath, entry->d_name);
            pathbuf[len - 1] = '\0';

            if (entry->d_type == DT_DIR)
            {
                if (depth == 0)
                    continue;
                if (!isGoodDirectory(entry))
                    continue;
                recurseDumpPath(pathbuf, fileVector, depth - 1);
            }
            else
            {
                U32 fileSize = Platform::getFileSize(pathbuf);
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

    int len = dStrlen(path);
    char newpath[len + 1];

    dStrncpy(newpath, path, len);
    newpath[len] = '\0';
    if (newpath[len - 1] == '/')
        newpath[len - 1] = '\0';

    bool ret = recurseDumpPath(newpath, fileVector, depth);

    PROFILE_END();
    return ret;
}
