//-----------------------------------------------------------------------------
// win32FileIO.cpp — Windows implementation of the File class (core/fileio.h)
// and the file/directory functions declared in platform.h.
//
// Fresh C++17 rewrite. Uses std::filesystem for directory traversal,
// existence/size/time queries, and path manipulation (which correctly
// handles UTF-8 <-> UTF-16 conversion internally via std::filesystem::path's
// wchar_t construction on Windows); uses real Win32 CreateFileW-based
// handles for the File class itself, since its contract (capability flags,
// explicit AccessMode) maps most directly onto native Win32 file handles.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "core/fileio.h"
#include "core/util/tVector.h"
#include "core/stringTable.h"
#include "console/console.h"
#include "core/strings/stringFunctions.h"
#include "core/strings/unicode.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h> // SHGetKnownFolderPath, FOLDERID_RoamingAppData — for
                    // Platform::getUserDataDirectory(). Requires linking
                    // against Shell32.lib and Ole32.lib (the latter for
                    // CoTaskMemFree) — add both to the target's link
                    // libraries if not already present.

#include <filesystem>
#include <system_error>
#include <string>
#include <codecvt>
#include <locale>
#include <functional>
#include <cstdlib>

namespace fs = std::filesystem;

namespace
{

   // Converting explicitly avoids depending on the process ACP.
   std::wstring utf8ToWide(const char* utf8)
   {
      if (!utf8 || !*utf8)
         return std::wstring();
      const int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
      std::wstring wide(static_cast<size_t>(wideLen > 0 ? wideLen - 1 : 0), L'\0');
      if (wideLen > 0)
         MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide.data(), wideLen);
      return wide;
   }

   std::string wideToUtf8(const std::wstring& wide)
   {
      if (wide.empty())
         return std::string();
      const int narrowLen = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
      std::string narrow(static_cast<size_t>(narrowLen > 0 ? narrowLen - 1 : 0), '\0');
      if (narrowLen > 0)
         WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, narrow.data(), narrowLen, nullptr, nullptr);
      return narrow;
   }

   fs::path toFsPath(const char* utf8)
   {
      return fs::path(utf8ToWide(utf8));
   }

   void toWinFileTime(const FileTime& ft, FILETIME& out)
   {
      out.dwLowDateTime = ft.v1;
      out.dwHighDateTime = ft.v2;
   }

   FileTime fromWinFileTime(const FILETIME& ft)
   {
      FileTime out;
      out.v1 = ft.dwLowDateTime;
      out.v2 = ft.dwHighDateTime;
      return out;
   }

   U64 fileTimeTo64(const FileTime& ft)
   {
      return (static_cast<U64>(ft.v2) << 32) | ft.v1;
   }

   DWORD toWinAccessFlags(File::AccessMode mode, DWORD& creationDisposition)
   {
      switch (mode)
      {
      case File::Read:
         creationDisposition = OPEN_EXISTING;
         return GENERIC_READ;
      case File::Write:
         creationDisposition = CREATE_ALWAYS;
         return GENERIC_WRITE;
      case File::ReadWrite:
         creationDisposition = OPEN_ALWAYS;
         return GENERIC_READ | GENERIC_WRITE;
      case File::WriteAppend:
         creationDisposition = OPEN_ALWAYS;
         return GENERIC_WRITE;
      }
      creationDisposition = OPEN_EXISTING;
      return GENERIC_READ;
   }
}

//-----------------------------------------------------------------------------
// File
//-----------------------------------------------------------------------------
File::File()
   : handle(reinterpret_cast<void*>(INVALID_HANDLE_VALUE)), currentStatus(Closed), capability(0)
{
   static_assert(sizeof(HANDLE) == sizeof(void*), "File::File: cannot cast void* to HANDLE");
}

File::~File()
{
   close();
}

File::FileStatus File::open(const char* filename, const AccessMode openMode)
{
   AssertFatal(filename != nullptr, "File::open: NULL filename");

   if (currentStatus != Closed)
      close();

   const std::wstring wpath = utf8ToWide(filename);

   DWORD creationDisposition;
   const DWORD access = toWinAccessFlags(openMode, creationDisposition);

   HANDLE h = CreateFileW(wpath.c_str(), access,
      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);

   if (h == INVALID_HANDLE_VALUE)
   {
      Con::errorf("File::open: could not open '%s' (err=%lu)", filename, GetLastError());
      return setStatus();
   }

   if (openMode == WriteAppend)
      SetFilePointer(h, 0, nullptr, FILE_END);

   handle = reinterpret_cast<void*>(h);

   switch (openMode)
   {
   case Read:        capability = FileRead; break;
   case Write:
   case WriteAppend: capability = FileWrite; break;
   case ReadWrite:   capability = FileRead | FileWrite; break;
   }

   return currentStatus = Ok;
}

U32 File::getPosition() const
{
   AssertFatal(currentStatus != Closed, "File::getPosition: file closed");
   return SetFilePointer(reinterpret_cast<HANDLE>(handle), 0, nullptr, FILE_CURRENT);
}

File::FileStatus File::setPosition(S32 position, bool absolutePos)
{
   AssertFatal(currentStatus != Closed, "File::setPosition: file closed");

   if (currentStatus != Ok && currentStatus != EOS)
      return currentStatus;

   const DWORD result = SetFilePointer(reinterpret_cast<HANDLE>(handle), position, nullptr,
      absolutePos ? FILE_BEGIN : FILE_CURRENT);
   if (result == INVALID_SET_FILE_POINTER)
      return setStatus();

   if (result >= getSize())
      return currentStatus = EOS;

   return currentStatus = Ok;
}

U32 File::getSize() const
{
   if (currentStatus != Ok && currentStatus != EOS)
      return 0;
   return GetFileSize(reinterpret_cast<HANDLE>(handle), nullptr);
}

File::FileStatus File::flush()
{
   AssertFatal(currentStatus != Closed, "File::flush: file closed");
   AssertFatal(hasCapability(FileWrite), "File::flush: cannot flush a read-only file");
   return FlushFileBuffers(reinterpret_cast<HANDLE>(handle)) ? (currentStatus = Ok) : setStatus();
}

File::FileStatus File::close()
{
   HANDLE h = reinterpret_cast<HANDLE>(handle);
   if (h != INVALID_HANDLE_VALUE && h != nullptr)
   {
      const bool ok = CloseHandle(h) != 0;
      handle = reinterpret_cast<void*>(INVALID_HANDLE_VALUE);
      if (!ok)
         return setStatus();
   }
   return currentStatus = Closed;
}

File::FileStatus File::getStatus() const
{
   return currentStatus;
}

File::FileStatus File::setStatus()
{
   Con::errorf("File IO error (Win32 err=%lu)", GetLastError());
   return currentStatus = IOError;
}

File::FileStatus File::setStatus(FileStatus status)
{
   return currentStatus = status;
}

File::FileStatus File::read(U32 size, char* dst, U32* bytesRead)
{
   AssertFatal(currentStatus != Closed, "File::read: file closed");
   AssertFatal(dst != nullptr, "File::read: NULL destination pointer");
   AssertFatal(hasCapability(FileRead), "File::read: file lacks read capability");

   if (currentStatus != Ok || size == 0)
      return currentStatus;

   DWORD actuallyRead = 0;
   if (!ReadFile(reinterpret_cast<HANDLE>(handle), dst, size, &actuallyRead, nullptr))
      return setStatus();

   if (bytesRead)
      *bytesRead = actuallyRead;

   if (actuallyRead != size)
      return currentStatus = EOS;

   return currentStatus = Ok;
}

File::FileStatus File::write(U32 size, const char* src, U32* bytesWritten)
{
   AssertFatal(currentStatus != Closed, "File::write: file closed");
   AssertFatal(src != nullptr, "File::write: NULL source pointer");
   AssertFatal(hasCapability(FileWrite), "File::write: file lacks write capability");

   if ((currentStatus != Ok && currentStatus != EOS) || size == 0)
      return currentStatus;

   DWORD actuallyWritten = 0;
   if (!WriteFile(reinterpret_cast<HANDLE>(handle), src, size, &actuallyWritten, nullptr))
      return setStatus();

   if (bytesWritten)
      *bytesWritten = actuallyWritten;

   return currentStatus = Ok;
}

bool File::hasCapability(Capability cap) const
{
   return (capability & static_cast<U32>(cap)) != 0;
}

//-----------------------------------------------------------------------------
// Platform:: free functions
//-----------------------------------------------------------------------------
StringTableEntry Platform::createPlatformFriendlyFilename(const char* filename)
{
   return StringTable->insert(filename);
}

bool dFileDelete(const char* name)
{
   std::error_code ec;
   return fs::remove(toFsPath(name), ec);
}

bool Platform::fileDelete(const char* name)
{
   return dFileDelete(name);
}

bool dFileRename(const char* oldName, const char* newName)
{
   std::error_code ec;
   fs::rename(toFsPath(oldName), toFsPath(newName), ec);
   return !ec;
}

bool dFileTouch(const char* name)
{
   const std::wstring wpath = utf8ToWide(name);
   HANDLE h = CreateFileW(wpath.c_str(), FILE_WRITE_ATTRIBUTES,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr, OPEN_EXISTING, 0, nullptr);
   if (h == INVALID_HANDLE_VALUE)
      return false;

   FILETIME now;
   GetSystemTimeAsFileTime(&now);
   const bool ok = SetFileTime(h, nullptr, nullptr, &now) != 0;
   CloseHandle(h);
   return ok;
}

bool dPathCopy(const char* fromName, const char* toName, bool nooverwrite)
{
   std::error_code ec;
   const fs::path from = toFsPath(fromName);
   const fs::path to = toFsPath(toName);

   if (fs::is_directory(from, ec))
   {
      const auto options = fs::copy_options::recursive |
         (nooverwrite ? fs::copy_options::skip_existing : fs::copy_options::overwrite_existing);
      fs::copy(from, to, options, ec);
      return !ec;
   }

   const auto options = nooverwrite ? fs::copy_options::skip_existing : fs::copy_options::overwrite_existing;
   fs::copy_file(from, to, options, ec);
   return !ec;
}

StringTableEntry osGetTemporaryDirectory()
{
   std::error_code ec;
   fs::path tempDir = fs::temp_directory_path(ec);
   if (ec)
      return StringTable->insert("");

   std::string utf8 = wideToUtf8(tempDir.wstring());
   for (char& c : utf8) if (c == '\\') c = '/';
   if (!utf8.empty() && utf8.back() != '/')
      utf8 += '/';
   return StringTable->insert(utf8.c_str());
}

void Platform::getVolumeNamesList( Vector<const char*>& out_rNameVector, bool bOnlyFixedDrives )
{
	DWORD dwDrives = GetLogicalDrives();
	DWORD dwMask = 1;
	char driveLetter[12];

   out_rNameVector.clear();
		
	for(S32 i = 0; i < 32; i++ )
	{
		dMemset(driveLetter,0,12);
		if( dwDrives & dwMask )
		{
			dSprintf(driveLetter, 12, "%c:", (i + 'A'));

			if( bOnlyFixedDrives && GetDriveTypeA(driveLetter) == DRIVE_FIXED )
            out_rNameVector.push_back( StringTable->insert( driveLetter ) );
         else if ( !bOnlyFixedDrives )
            out_rNameVector.push_back( StringTable->insert( driveLetter ) );
		}
		dwMask <<= 1;
	}
}

void Platform::getVolumeInformationList( Vector<VolumeInformation>& out_rVolumeInfoVector, bool bOnlyFixedDrives )
{
   Vector<const char*> drives;

   getVolumeNamesList( drives, bOnlyFixedDrives );

   if( ! drives.empty() )
   {
      Vector<StringTableEntry>::iterator i;
      for( i = drives.begin(); i != drives.end(); i++ )
      {
         VolumeInformation info;
         TCHAR lpszVolumeName[ 256 ];
         TCHAR lpszFileSystem[ 256 ];
         DWORD dwSerial = 0;
         DWORD dwMaxComponentLength = 0;
         DWORD dwFileSystemFlags = 0;

         dMemset( lpszVolumeName, 0, sizeof( lpszVolumeName ) );
         dMemset( lpszFileSystem, 0, sizeof( lpszFileSystem ) );
         dMemset( &info, 0, sizeof( VolumeInformation ) );

         // More volume information
         UINT uDriveType = GetDriveTypeA( (*i) );
         if( uDriveType == DRIVE_UNKNOWN )
            info.Type = DRIVETYPE_UNKNOWN;
         else if( uDriveType == DRIVE_REMOVABLE )
            info.Type = DRIVETYPE_REMOVABLE;
         else if( uDriveType == DRIVE_FIXED )
            info.Type = DRIVETYPE_FIXED;
         else if( uDriveType == DRIVE_CDROM )
            info.Type = DRIVETYPE_CDROM;
         else if( uDriveType == DRIVE_RAMDISK )
            info.Type = DRIVETYPE_RAMDISK;
         else if( uDriveType == DRIVE_REMOTE )
            info.Type = DRIVETYPE_REMOTE;

         info.RootPath = StringTable->insert( (*i) );

         // We don't retrieve drive volume info for removable drives, because it's loud :(
         if( info.Type != DRIVETYPE_REMOVABLE )
         {
#ifdef UNICODE
            WCHAR ibuf[ 3 ];
            ibuf[ 0 ] = ( *i )[ 0 ];
            ibuf[ 1 ] = ':';
            ibuf[ 2 ] = '\0';
#else
            char* ibuf = *i;
#endif
            // Standard volume information
            GetVolumeInformation( ibuf, lpszVolumeName, sizeof( lpszVolumeName ) / sizeof( lpszVolumeName[ 0 ] ),
               &dwSerial, &dwMaxComponentLength, &dwFileSystemFlags, lpszFileSystem,
               sizeof( lpszFileSystem ) / sizeof( lpszFileSystem[ 0 ] ) );

#ifdef UNICODE
            char buf[ sizeof( lpszFileSystem ) / sizeof( lpszFileSystem[ 0 ] ) * 3 + 1 ];
            convertUTF16toUTF8( lpszFileSystem, buf );
            info.FileSystem = StringTable->insert( buf );

            convertUTF16toUTF8( lpszVolumeName, buf );
            info.Name = StringTable->insert( buf );
#else
            info.FileSystem = StringTable->insert( lpszFileSystem );
            info.Name = StringTable->insert( lpszVolumeName );
#endif
            info.SerialNumber = dwSerial;
            // Won't compile on something prior to XP.
            info.ReadOnly = dwFileSystemFlags & FILE_READ_ONLY_VOLUME;
         }
         out_rVolumeInfoVector.push_back( info );

         // I opted not to get free disk space because of the overhead of the calculations required for it

      }
   }
}


bool Platform::isFile(const char* pFilePath)
{
   if (!pFilePath || !*pFilePath)
      return false;
   std::error_code ec;
   return fs::is_regular_file(toFsPath(pFilePath), ec) && !ec;
}

S32 Platform::getFileSize(const char* pFilePath)
{
   if (!pFilePath || !*pFilePath)
      return -1;
   std::error_code ec;
   const auto size = fs::file_size(toFsPath(pFilePath), ec);
   return ec ? -1 : static_cast<S32>(size);
}

bool Platform::isDirectory(const char* pDirPath)
{
   if (!pDirPath || !*pDirPath)
      return false;
   std::error_code ec;
   return fs::is_directory(toFsPath(pDirPath), ec) && !ec;
}

bool Platform::isSubDirectory(const char* pParent, const char* pDir)
{
   if (!pParent || !pDir || !*pDir)
      return false;

   std::error_code ec;
   for (const auto& entry : fs::directory_iterator(toFsPath(pParent), ec))
   {
      if (ec) break;
      if (entry.is_directory())
      {
         const std::string name = wideToUtf8(entry.path().filename().wstring());
         if (String(name.c_str()).equal(pDir, String::NoCase))
            return true;
      }
   }
   return false;
}

bool Platform::hasSubDirectory(const char* pPath)
{
   if (!pPath)
      return false;

   std::error_code ec;
   for (const auto& entry : fs::directory_iterator(toFsPath(pPath), ec))
   {
      if (ec) break;
      if (!entry.is_directory())
         continue;

      const std::string name = wideToUtf8(entry.path().filename().wstring());
      if (Platform::isExcludedDirectory(name.c_str()))
         continue;
      return true;
   }
   return false;
}

bool Platform::createPath(const char* path)
{
   std::error_code ec;
   fs::path p = toFsPath(path);
   fs::path dir = p.has_extension() ? p.parent_path() : p;
   if (dir.empty())
      return true;
   fs::create_directories(dir, ec);
   return !ec || fs::is_directory(dir);
}

// NOTE: Platform::deleteDirectory is already defined generically in
// platformFileIO.cpp (pure recursive-delete-with-warning logic, no OS-
// specific content) — do not redefine it here. Same applies to
// Platform::compareModifiedTimes (platform.cpp) and
// Platform::addExcludedDirectory/clearExcludedDirectories/
// isExcludedDirectory (platformFileIO.cpp), removed further down in this
// file for the same reason.

bool Platform::getFileTimes(const char* filePath, FileTime* createTime, FileTime* modifyTime)
{
   const std::wstring wpath = utf8ToWide(filePath);

   WIN32_FILE_ATTRIBUTE_DATA data;
   if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &data))
      return false;

   if (createTime)
      *createTime = fromWinFileTime(data.ftCreationTime);
   if (modifyTime)
      *modifyTime = fromWinFileTime(data.ftLastWriteTime);

   return true;
}

S32 Platform::compareFileTimes(const FileTime& a, const FileTime& b)
{
   const U64 av = fileTimeTo64(a);
   const U64 bv = fileTimeTo64(b);
   if (av > bv) return 1;
   if (av < bv) return -1;
   return 0;
}

bool Platform::stringToFileTime(const char* string, FileTime* time)
{
   if (!time || !string)
      return false;
   // Using std::strtoull directly rather than a d*-prefixed 64-bit parse
   // helper, since a confirmed dAtoi64 (or equivalent) signature wasn't
   // available to verify against core/strings/stringFunctions.h.
   const U64 v = static_cast<U64>(std::strtoull(string, nullptr, 10));
   time->v1 = static_cast<U32>(v & 0xFFFFFFFFu);
   time->v2 = static_cast<U32>(v >> 32);
   return true;
}

bool Platform::fileTimeToString(FileTime* time, char* string, U32 strLen)
{
   if (!time || !string)
      return false;
   dSprintf(string, strLen, "%llu", static_cast<unsigned long long>(fileTimeTo64(*time)));
   return true;
}

StringTableEntry Platform::getCurrentDirectory()
{
   std::error_code ec;
   fs::path cwd = fs::current_path(ec);
   if (ec)
      return StringTable->insert("");
   std::string utf8 = wideToUtf8(cwd.wstring());
   for (char& c : utf8) if (c == '\\') c = '/';
   return StringTable->insert(utf8.c_str());
}

bool Platform::setCurrentDirectory(StringTableEntry newDir)
{
   if (Platform::getWebDeployment())
      return true;

   std::error_code ec;
   fs::current_path(toFsPath(newDir), ec);
   return !ec;
}

StringTableEntry Platform::getUserHomeDirectory()
{
   wchar_t* userProfile = nullptr;
   size_t len = 0;
   if (_wdupenv_s(&userProfile, &len, L"USERPROFILE") == 0 && userProfile)
   {
      std::string utf8 = wideToUtf8(std::wstring(userProfile));
      free(userProfile);
      for (char& c : utf8) if (c == '\\') c = '/';
      return StringTable->insert(utf8.c_str());
   }
   return StringTable->insert("");
}

// Distinct from getUserHomeDirectory(): this returns the per-user
// "roaming application data" folder (%APPDATA%, typically
// C:/Users/<name>/AppData/Roaming), which is the conventional Windows
// location for application settings/save data — the same role
// ~/Library/Application Support/ plays on macOS and ~/.local/share (or
// similar) plays on Linux, as opposed to the user's home/profile root.
StringTableEntry Platform::getUserDataDirectory()
{
   PWSTR path = nullptr;
   HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);

   if (SUCCEEDED(hr) && path)
   {
      std::string utf8 = wideToUtf8(std::wstring(path));
      CoTaskMemFree(path);
      for (char& c : utf8) if (c == '\\') c = '/';
      return StringTable->insert(utf8.c_str());
   }

   if (path)
      CoTaskMemFree(path);

   // Fall back to %APPDATA% environment variable if the Known Folder API
   // call fails for any reason.
   wchar_t* appData = nullptr;
   size_t len = 0;
   if (_wdupenv_s(&appData, &len, L"APPDATA") == 0 && appData)
   {
      std::string utf8 = wideToUtf8(std::wstring(appData));
      free(appData);
      for (char& c : utf8) if (c == '\\') c = '/';
      return StringTable->insert(utf8.c_str());
   }

   return StringTable->insert("");
}

namespace
{
   char sExecutablePath[MAX_PATH] = "";
   char sExecutableName[MAX_PATH] = "";
   bool sResolved = false;

   void resolveExecutablePath()
   {
      if (sResolved)
         return;
      sResolved = true;

      wchar_t wbuf[MAX_PATH];
      const DWORD len = GetModuleFileNameW(nullptr, wbuf, MAX_PATH);
      if (len == 0)
         return;

      fs::path p(std::wstring(wbuf, len));
      std::string dir = wideToUtf8(p.parent_path().wstring());
      std::string name = wideToUtf8(p.filename().wstring());
      for (char& c : dir) if (c == '\\') c = '/';

      dStrncpy(sExecutablePath, dir.c_str(), sizeof(sExecutablePath));
      dStrncpy(sExecutableName, name.c_str(), sizeof(sExecutableName));
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

// NOTE: Platform::addExcludedDirectory/clearExcludedDirectories/
// isExcludedDirectory are already defined generically in
// platformFileIO.cpp (plain in-memory list bookkeeping, no OS-specific
// content) — do not redefine them here. The sExcludedDirectories
// Vector<String> that used to back a per-OS copy of these has been
// removed along with them.

//-----------------------------------------------------------------------------
bool Platform::dumpPath(const char* in_pBasePath, Vector<Platform::FileInfo>& out_rFileVector, S32 recurseDepth)
{
   std::error_code ec;
   const fs::path base = toFsPath(in_pBasePath);
   if (!fs::is_directory(base, ec) || ec)
      return false;

   std::function<void(const fs::path&, S32)> walk = [&](const fs::path& dir, S32 depth)
   {
      for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec))
      {
         if (ec) break;

         const std::string name = wideToUtf8(entry.path().filename().wstring());

         if (entry.is_directory())
         {
            if (Platform::isExcludedDirectory(name.c_str()))
               continue;
            if (recurseDepth < 0 || depth < recurseDepth)
               walk(entry.path(), depth + 1);
         }
         else if (entry.is_regular_file())
         {
            out_rFileVector.increment();
            Platform::FileInfo& info = out_rFileVector.last();
            info.pFullPath = StringTable->insert(wideToUtf8(entry.path().parent_path().wstring()).c_str());
            info.pFileName = StringTable->insert(name.c_str());

            std::error_code sizeEc;
            info.fileSize = static_cast<U32>(fs::file_size(entry.path(), sizeEc));
         }
      }
   };

   walk(base, 0);
   return true;
}

bool Platform::dumpDirectories(const char* path, Vector<StringTableEntry>& directoryVector, S32 depth, bool noBasePath)
{
   std::error_code ec;
   const fs::path base = toFsPath(path);
   if (!fs::is_directory(base, ec) || ec)
      return false;

   if (!noBasePath)
      directoryVector.push_back(StringTable->insert(path));

   std::function<void(const fs::path&, S32)> walk = [&](const fs::path& dir, S32 currentDepth)
   {
      for (const auto& entry : fs::directory_iterator(dir, ec))
      {
         if (ec) break;
         if (!entry.is_directory())
            continue;

         const std::string name = wideToUtf8(entry.path().filename().wstring());
         if (Platform::isExcludedDirectory(name.c_str()))
            continue;

         std::string full = wideToUtf8(entry.path().wstring());
         for (char& c : full) if (c == '\\') c = '/';
         directoryVector.push_back(StringTable->insert(full.c_str()));

         if (depth < 0 || currentDepth < depth)
            walk(entry.path(), currentDepth + 1);
      }
   };

   walk(base, 0);
   return true;
}
