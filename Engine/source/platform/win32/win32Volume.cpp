//-----------------------------------------------------------------------------
// win32Volume.cpp — Windows native Torque::FS::FileSystem implementation,
// plus Platform::FS::InstallFileSystems() (VFS mount bring-up).
//
// Fresh C++17 rewrite against the confirmed core/volume.h contract.
// Preserves two pieces of real, working functionality from the original
// winVolume.cpp that aren't legacy noise:
//   - Win32FileSystemChangeNotifier (live directory-change notifications
//     via FindFirstChangeNotificationW) — Linux's volume layer has no
//     equivalent of this; it's a genuine Windows-only feature, not
//     something to drop.
//   - Win32File::getSize() querying the live handle size (via
//     GetFileSizeEx) when the file is currently open, rather than only
//     the on-disk size, which correctly accounts for unflushed write
//     buffers.
//
// SECURE VFS FIX: same class of bug as Linux's InstallFileSystems() — the
// original wrapped its ENTIRE body in "#ifndef TORQUE_SECURE_VFS", so
// defining that flag skipped cwd setup entirely, not just the (genuinely
// insecure) drive-mounting step. Only the raw drive-mounting loop is now
// skipped under TORQUE_SECURE_VFS; cwd setup still happens unconditionally.
//
// NOTES ON THIS REVISION:
//   - Added "using namespace FS;" inside namespace Torque. FileSystem,
//     FileNode, Path, Directory, etc. all live in Torque::FS, and every
//     class below references them unqualified — without this the file
//     does not compile.
//   - Every Win32 API call is now explicitly global-namespace-qualified
//     (::CreateFileW, ::ReadFile, etc.). This code lives inside
//     namespace Torque::Win32 — a namespace literally named Win32 — so an
//     unqualified call risks ambiguous/incorrect name lookup once
//     "using namespace FS" is in scope, and that risk varies by
//     translation unit and include order rather than failing
//     consistently. The :: prefix removes the ambiguity outright.
//-----------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "platform/platform.h"
#include "platform/platformVolume.h"
#include "core/volume.h"
#include "core/crc.h"
#include "core/frameAllocator.h"
#include "core/util/str.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"

#include <string>
#include <vector>

namespace Torque
{
   using namespace FS;

   namespace Win32
   {

      namespace
      {
         // A directory/system/offline/temporary attribute rules a file OUT of
         // being a plain regular file, mirroring the original's S_ISREG macro
         // but as inline functions rather than macros (macros named S_ISREG/
         // S_ISDIR are prone to colliding with any POSIX-compat header that
         // might also be transitively included).
         bool isRegularFile(DWORD attrs)
         {
            return !(attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE |
               FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY));
         }

         bool isDirectoryAttr(DWORD attrs)
         {
            return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
         }

         String buildFileName(const String& prefix, const Path& path)
         {
            String file = prefix;
            file = Path::Join(file, '/', path.getPath());

            if (path.getFileName().isEmpty() && path.getExtension().isNotEmpty())
               file += String("/");
            else
               file = Path::Join(file, '/', path.getFileName());

            file = Path::Join(file, '.', path.getExtension());
            return file;
         }

         bool queryIsDirectory(const String& file)
         {
            WIN32_FIND_DATAW info;
            HANDLE h = ::FindFirstFileW(PathToOS(file).utf16(), &info);
            ::FindClose(h);
            if (h == INVALID_HANDLE_VALUE)
               return false;
            return isDirectoryAttr(info.dwFileAttributes);
         }

         void copyStatAttributes(const WIN32_FIND_DATAW& info, FileNode::Attributes* attr)
         {
            attr->flags = 0;
            if (isDirectoryAttr(info.dwFileAttributes)) attr->flags |= FileNode::Directory;
            if (isRegularFile(info.dwFileAttributes))   attr->flags |= FileNode::File;
            if (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) attr->flags |= FileNode::ReadOnly;

            attr->size = (static_cast<U64>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;

            // Convert to local time before handing off to Win32FileTimeToTime,
            // matching the original's behavior (attr times are reported in
            // local time, not UTC).
            auto toLocalTorqueTime = [](const FILETIME& utc) -> Torque::Time
            {
               SYSTEMTIME st, stLocal;
               FILETIME localFt;
               ::FileTimeToSystemTime(&utc, &st);
               ::SystemTimeToTzSpecificLocalTime(nullptr, &st, &stLocal);
               ::SystemTimeToFileTime(&stLocal, &localFt);
               return Win32FileTimeToTime(localFt.dwLowDateTime, localFt.dwHighDateTime);
            };

            attr->mtime = toLocalTorqueTime(info.ftLastWriteTime);
            attr->atime = toLocalTorqueTime(info.ftLastAccessTime);
            attr->ctime = toLocalTorqueTime(info.ftCreationTime);
         }
      }

      //-----------------------------------------------------------------------------
      class Win32FileSystemChangeNotifier : public FileSystemChangeNotifier
      {
      public:
         explicit Win32FileSystemChangeNotifier(FileSystem* fs) : FileSystemChangeNotifier(fs) {}

      private:
         void internalProcessOnce() override
         {
            // WaitForMultipleObjects caps out at MAXIMUM_WAIT_OBJECTS, so loop
            // over the handle list in chunks.
            for (U32 i = 0; i < mHandleList.size(); i += MAXIMUM_WAIT_OBJECTS)
            {
               const U32 numHandles = getMin<U32>(MAXIMUM_WAIT_OBJECTS, static_cast<U32>(mHandleList.size()) - i);

               const DWORD waitStatus = ::WaitForMultipleObjects(numHandles, mHandleList.address() + i, FALSE, 0);
               if (waitStatus == WAIT_FAILED || waitStatus == WAIT_TIMEOUT)
                  continue;

               if (waitStatus >= WAIT_OBJECT_0 && waitStatus <= (WAIT_OBJECT_0 + numHandles - 1))
               {
                  const U32 index = i + waitStatus;
                  // Reset the notification before checking mod times, so we
                  // don't miss a change that occurs mid-check.
                  ::FindNextChangeNotification(mHandleList[index]);
                  internalNotifyDirChanged(mDirs[index]);
               }
            }
         }

         bool internalAddNotification(const Path& dir) override
         {
            for (const auto& existing : mDirs)
               if (existing == dir)
                  return false;

            const Path fullFSPath = mFS->mapTo(dir);
            const String osPath = PathToOS(fullFSPath);

            HANDLE h = ::FindFirstChangeNotificationW(
               osPath.utf16(), FALSE,
               FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES);

            if (h == INVALID_HANDLE_VALUE || h == nullptr)
            {
               Con::errorf("Win32FileSystemChangeNotifier::internalAddNotification: failed on '%s' (err=%lu)",
                  osPath.c_str(), ::GetLastError());
               return false;
            }

            mDirs.push_back(dir);
            mHandleList.push_back(h);
            return true;
         }

         bool internalRemoveNotification(const Path& dir) override
         {
            for (U32 i = 0; i < mDirs.size(); ++i)
            {
               if (mDirs[i] != dir)
                  continue;
               ::FindCloseChangeNotification(mHandleList[i]);
               mDirs.erase(i);
               mHandleList.erase(i);
               return true;
            }
            return false;
         }

         Vector<Path>   mDirs;
         Vector<HANDLE> mHandleList;
      };

      //-----------------------------------------------------------------------------
      class Win32File final : public Torque::FS::File
      {
         friend class Win32FileSystem;

         Path _path;
         String _name;
         HANDLE _handle = nullptr;
         NodeStatus _status = Closed;

         Win32File(const Path& path, String name) : _path(path), _name(std::move(name)) {}

         void _updateStatus()
         {
            switch (::GetLastError())
            {
            case ERROR_INVALID_ACCESS:      _status = AccessDenied;     break;
            case ERROR_TOO_MANY_OPEN_FILES: _status = UnknownError;     break;
            case ERROR_PATH_NOT_FOUND:      _status = NoSuchFile;       break;
            case ERROR_FILE_NOT_FOUND:      _status = NoSuchFile;       break;
            case ERROR_SHARING_VIOLATION:   _status = SharingViolation; break;
            case ERROR_HANDLE_DISK_FULL:    _status = FileSystemFull;   break;
            case ERROR_ACCESS_DENIED:       _status = AccessDenied;     break;
            default:                        _status = UnknownError;    break;
            }
         }

         U32 calculateChecksum() override
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

      public:
         ~Win32File() override { if (_handle) close(); }

         Path getName() const override { return _path; }
         NodeStatus getStatus() const override { return _status; }

         bool getAttributes(Attributes* attr) override
         {
            WIN32_FIND_DATAW info;
            HANDLE h = ::FindFirstFileW(PathToOS(_name).utf16(), &info);
            ::FindClose(h);
            if (h == INVALID_HANDLE_VALUE)
               return false;

            copyStatAttributes(info, attr);
            attr->name = _path;
            return true;
         }

         // Preserves the original's live-handle-size optimization: while open,
         // query the actual handle size (accounts for unflushed write buffers)
         // rather than the on-disk size the base FileNode::getSize() would see.
         U64 getSize() override
         {
            if (_status == Open)
            {
               LARGE_INTEGER size;
               if (!::GetFileSizeEx(_handle, &size))
                  return 0;
               return static_cast<U64>(size.QuadPart);
            }
            return FileNode::getSize();
         }

         bool open(AccessMode mode) override
         {
            close();

            if (_name.isEmpty())
               return false;

            DWORD access, share, disposition;
            switch (mode)
            {
            case Read:        access = GENERIC_READ;  share = FILE_SHARE_READ; disposition = OPEN_EXISTING; break;
            case Write:       access = GENERIC_WRITE; share = 0; disposition = CREATE_ALWAYS; break;
            case ReadWrite:   access = GENERIC_READ | GENERIC_WRITE; share = 0; disposition = OPEN_ALWAYS; break;
            case WriteAppend: access = GENERIC_WRITE; share = 0; disposition = OPEN_ALWAYS; break;
            default:          access = GENERIC_READ;  share = FILE_SHARE_READ; disposition = OPEN_EXISTING; break;
            }

            _handle = ::CreateFileW(PathToOS(_name).utf16(), access, share, nullptr, disposition,
               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

            if (_handle == INVALID_HANDLE_VALUE)
            {
               _updateStatus();
               _handle = nullptr;
               return false;
            }

            if (mode == WriteAppend)
               ::SetFilePointer(_handle, 0, nullptr, FILE_END);

            _status = Open;
            return true;
         }

         bool close() override
         {
            if (_handle)
            {
               ::CloseHandle(_handle);
               _handle = nullptr;
            }
            _status = Closed;
            return true;
         }

         U32 getPosition() override
         {
            return (_status == Open || _status == EndOfFile) ? ::SetFilePointer(_handle, 0, nullptr, FILE_CURRENT) : 0;
         }

         U32 setPosition(U32 delta, SeekMode mode) override
         {
            if (_status != Open && _status != EndOfFile)
               return 0;

            DWORD winMode = FILE_BEGIN;
            switch (mode)
            {
            case Begin:   winMode = FILE_BEGIN;   break;
            case Current: winMode = FILE_CURRENT; break;
            case End:     winMode = FILE_END;     break;
            }

            const DWORD pos = ::SetFilePointer(_handle, static_cast<LONG>(delta), nullptr, winMode);
            if (pos == INVALID_SET_FILE_POINTER)
            {
               _status = UnknownError;
               return 0;
            }

            _status = Open;
            return pos;
         }

         U32 read(void* dst, U32 size) override
         {
            if (_status != Open && _status != EndOfFile)
               return 0;

            DWORD bytesRead = 0;
            if (!::ReadFile(_handle, dst, size, &bytesRead, nullptr))
               _updateStatus();
            else if (bytesRead != size)
               _status = EndOfFile;

            return bytesRead;
         }

         U32 write(const void* src, U32 size) override
         {
            if ((_status != Open && _status != EndOfFile) || size == 0)
               return 0;

            DWORD bytesWritten = 0;
            if (!::WriteFile(_handle, src, size, &bytesWritten, nullptr))
               _updateStatus();

            return bytesWritten;
         }
      };

      //-----------------------------------------------------------------------------
      class Win32Directory final : public Torque::FS::Directory
      {
         friend class Win32FileSystem;

         Path _path;
         String _name;
         HANDLE _handle = nullptr;
         NodeStatus _status = Closed;

         Win32Directory(const Path& path, String name) : _path(path), _name(std::move(name)) {}

         void _updateStatus()
         {
            switch (::GetLastError())
            {
            case ERROR_NO_MORE_FILES:     _status = EndOfFile;        break;
            case ERROR_INVALID_ACCESS:    _status = AccessDenied;     break;
            case ERROR_PATH_NOT_FOUND:    _status = NoSuchFile;       break;
            case ERROR_SHARING_VIOLATION: _status = SharingViolation; break;
            case ERROR_ACCESS_DENIED:     _status = AccessDenied;     break;
            default:                      _status = UnknownError;    break;
            }
         }

         U32 calculateChecksum() override { return 0; }

      public:
         ~Win32Directory() override { if (_handle) close(); }

         Path getName() const override { return _path; }
         NodeStatus getStatus() const override { return _status; }

         bool getAttributes(Attributes* attr) override
         {
            WIN32_FIND_DATAW info;
            HANDLE h = ::FindFirstFileW(PathToOS(_name).utf16(), &info);
            ::FindClose(h);
            if (h == INVALID_HANDLE_VALUE)
            {
               _updateStatus();
               return false;
            }
            copyStatAttributes(info, attr);
            attr->name = _path;
            return true;
         }

         bool open() override
         {
            if (!queryIsDirectory(_name))
            {
               _status = NoSuchFile;
               return false;
            }
            _status = Open;
            return true;
         }

         bool close() override
         {
            if (_handle)
            {
               ::FindClose(_handle);
               _handle = nullptr;
               return true;
            }
            return false;
         }

         bool read(Attributes* entry) override
         {
            if (_status != Open)
               return false;

            WIN32_FIND_DATAW info;
            if (!_handle)
            {
               const String searchPath = PathToOS(_name) + "\\*";
               _handle = ::FindFirstFileW(searchPath.utf16(), &info);
               if (_handle == nullptr || _handle == INVALID_HANDLE_VALUE)
               {
                  _updateStatus();
                  return false;
               }
            }
            else if (!::FindNextFileW(_handle, &info))
            {
               _updateStatus();
               return false;
            }

            if (info.cFileName[0] == L'.' && (info.cFileName[1] == L'\0' ||
               (info.cFileName[1] == L'.' && info.cFileName[2] == L'\0')))
               return read(entry);

            copyStatAttributes(info, entry);
            entry->name = String(reinterpret_cast<const UTF16*>(info.cFileName));
            return true;
         }
      };

      //-----------------------------------------------------------------------------
      class Win32FileSystem final : public Torque::FS::FileSystem
      {
         String _volume;

      public:
         explicit Win32FileSystem(String volume) : _volume(std::move(volume))
         {
            mChangeNotifier = new Win32FileSystemChangeNotifier(this);
         }

         String getTypeStr() const override { return "Win32"; }

         FileNodeRef resolve(const Path& path) override
         {
            const String file = buildFileName(_volume, path);

            WIN32_FIND_DATAW info;
            HANDLE h = ::FindFirstFileW(PathToOS(file).utf16(), &info);
            ::FindClose(h);
            if (h == INVALID_HANDLE_VALUE)
               return nullptr;

            if (isRegularFile(info.dwFileAttributes))
               return new Win32File(path, file);
            if (isDirectoryAttr(info.dwFileAttributes))
               return new Win32Directory(path, file);

            return nullptr;
         }

         FileNodeRef create(const Path& path, FileNode::Mode mode) override
         {
            if (mode & FileNode::File)
               return new Win32File(path, buildFileName(_volume, path));

            if (mode & FileNode::Directory)
            {
               const String file = PathToOS(buildFileName(_volume, path));
               if (::CreateDirectoryW(file.utf16(), nullptr))
                  return new Win32Directory(path, file);
            }

            return nullptr;
         }

         bool remove(const Path& path) override
         {
            const String file = PathToOS(buildFileName(_volume, path));

            WIN32_FIND_DATAW info;
            HANDLE h = ::FindFirstFileW(file.utf16(), &info);
            ::FindClose(h);
            if (h == INVALID_HANDLE_VALUE)
               return false;

            if (isDirectoryAttr(info.dwFileAttributes))
               return ::RemoveDirectoryW(file.utf16()) != 0;

            return ::DeleteFileW(file.utf16()) != 0;
         }

         bool rename(const Path& from, const Path& to) override
         {
            const String fa = PathToOS(buildFileName(_volume, from));
            const String fb = PathToOS(buildFileName(_volume, to));
            return ::MoveFileW(fa.utf16(), fb.utf16()) != 0;
         }

         Path mapTo(const Path& path) override { return buildFileName(_volume, path); }

         Path mapFrom(const Path& path) override
         {
            const String full = path.getFullPath();
            const String::SizeType volumeLen = _volume.length();

            if (_volume.compare(full, volumeLen, String::NoCase) != 0)
               return Path();

            return Path(full.substr(volumeLen, full.length() - volumeLen));
         }
      };

   } // namespace Win32
} // namespace Torque

//-----------------------------------------------------------------------------
bool Torque::FS::VerifyWriteAccess(const Path& path)
{
   // UAC can silently redirect writes into a per-user "virtual store"
   // rather than actually failing, so the only reliable check is a real
   // create/write/read/verify/delete round-trip against the target
   // directory rather than trusting file permission bits.
   String temp = path.getFullPath();
   temp += "\\torque_writetest.tmp";

   ::DeleteFileW(temp.utf16());

   HANDLE hFile = ::CreateFileW(PathToOS(temp).utf16(), GENERIC_WRITE, 0, nullptr,
      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
   if (hFile == INVALID_HANDLE_VALUE)
      return false;

   const U32 t = Platform::getTime();
   DWORD bytesWritten = 0;
   if (!::WriteFile(hFile, &t, sizeof(t), &bytesWritten, nullptr))
   {
      ::CloseHandle(hFile);
      ::DeleteFileW(temp.utf16());
      return false;
   }
   ::CloseHandle(hFile);

   hFile = ::CreateFileW(PathToOS(temp).utf16(), GENERIC_READ, FILE_SHARE_READ, nullptr,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
   if (hFile == INVALID_HANDLE_VALUE)
      return false;

   U32 t2 = 0;
   DWORD bytesRead = 0;
   const bool readOk = ::ReadFile(hFile, &t2, sizeof(t2), &bytesRead, nullptr) != 0;
   ::CloseHandle(hFile);
   ::DeleteFileW(temp.utf16());

   return readOk && (t == t2);
}

//-----------------------------------------------------------------------------
Torque::FS::FileSystemRef Platform::FS::createNativeFS(const String& volume)
{
   return new Torque::Win32::Win32FileSystem(volume);
}

String Platform::FS::getAssetDir()
{
   wchar_t buf[2048];
   ::GetModuleFileNameW(nullptr, buf, 2048);

   std::wstring wpath(buf);
   const auto lastSlash = wpath.find_last_of(L'\\');
   if (lastSlash != std::wstring::npos)
      wpath.resize(lastSlash);

   return Path::CleanSeparators(String(reinterpret_cast<const UTF16*>(wpath.c_str())));
}

bool Platform::FS::InstallFileSystems()
{
   // See file header comment: only the raw drive-mounting loop (mounting
   // every local drive letter as its own root — the genuinely insecure
   // operation TORQUE_SECURE_VFS exists to prevent) is skipped. cwd setup
   // still runs unconditionally, since other engine code depends on it
   // having happened by this point regardless of secure-VFS mode.
#ifndef TORQUE_SECURE_VFS
    // Suppress the OS error dialog for drives with no media present
    // (e.g. an empty optical/card reader) while we enumerate them.
   ::SetErrorMode(SEM_FAILCRITICALERRORS);

   DWORD driveMask = ::GetLogicalDrives();
   char driveLetter = 'A';
   while (driveMask)
   {
      if (driveMask & 1)
      {
         char driveRoot[4] = { driveLetter, ':', '/', '\0' };
         char driveName[2] = { driveLetter, '\0' };
         Platform::FS::Mount(driveName, Platform::FS::createNativeFS(driveRoot));
      }
      driveMask >>= 1;
      ++driveLetter;
   }
#endif

   // Windows sometimes reports drive letters in different case depending
   // on the calling shell (cygwin/bash in particular); force uppercase to
   // stay consistent with the drive-letter mounts above.
   wchar_t cwdBuf[1024];
   ::GetCurrentDirectoryW(1024, cwdBuf);

   std::wstring cwd(cwdBuf);
   if (cwd.size() > 1 && cwd[1] == L':')
      cwd[0] = towupper(cwd[0]);

   String wd(cwd.c_str());
   wd += '/';
   Platform::FS::SetCwd(wd);

   return true;
}
