//
//  iosVolume.m
//  TestProject
//
//  Created by ctrlintelligence on 30/07/2023.
//

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "core/crc.h"
#include "core/frameAllocator.h"

#include "core/util/str.h"
#include "core/strings/stringFunctions.h"

#include "platform/platformVolume.h"
#include "platformIOS/iosVolume.h"

#ifndef PATH_MAX
#include <sys/syslimits.h>
#endif

#ifndef NGROUPS_UMAX
#define NGROUPS_UMAX 32
#endif

extern bool ResolvePathCaseInsensitive(char* pathName, S32 pathNameSize, bool requiredAbsolute);

namespace Torque
{
namespace IOS
{

static String buildFilename(const String& prefix, const Path& path)
{
   String file = prefix;
   file = Path::Join(file, '/', path.getPath());
   file = Path::Join(file, '/', path.getFileName());
   file = Path::Join(file, '.', path.getExtension());
   return file;
}

static uid_t _uid;
static int _GroupCount;
static gid_t _Groups[NGROUPS_MAX+1];

static void copyStatAttributes(const struct stat& info, FileNode::Attributes* attr)
{
   if(!_uid)
   {
      _uid = getuid();
      _GroupCount = getgroups(NGROUPS_MAX, _Groups);
      _Groups[_GroupCount++] = getegid();
   }
   
   attr->flags = 0;
   if(S_ISDIR(info.st_mode))
      attr->flags |= FileNode::Directory;
   
   if(S_ISREG(info.st_mode))
      attr->flags |= FileNode::File;
   
   if(info.st_uid == _uid)
   {
      if(!(info.st_mode & S_IWUSR))
         attr->flags |= FileNode::ReadOnly;
   }
   else
   {
      S32 i = 0;
      for(; i < _GroupCount; i++)
      {
         if(_Groups[i] == info.st_gid)
            break;
      }
      
      if(i != _GroupCount)
      {
         if(!(info.st_mode & S_IWGRP))
            attr->flags |= FileNode::ReadOnly;
      }
      else
      {
         if(!(info.st_mode & S_IWOTH))
            attr->flags |= FileNode::ReadOnly;
      }
   }
   
   attr->size = info.st_size;
   attr->mtime = UnixTimeToTime(info.st_mtime);
   attr->atime = UnixTimeToTime(info.st_atime);
   attr->ctime = UnixTimeToTime(info.st_ctime);
}

//---------------------------------------------
// IOSFileSystem
//---------------------------------------------

IOSFileSystem::IOSFileSystem(String volume)
{
   _volume = volume;
}

IOSFileSystem::~IOSFileSystem()
{
}

FileNodeRef IOSFileSystem::resolve(const Path& path)
{
   String file = buildFilename(_volume, path);
   struct stat info;
   
   FileNodeRef res = 0;
   
   if(stat(file.c_str(), &info) == 0)
   {
      if(S_ISREG(info.st_mode))
         res = new IOSFile(path, file);
      
      if(S_ISDIR(info.st_mode))
         res = new IOSDirectory(path, file);
   }
   
   return res;
   
}

FileNodeRef IOSFileSystem::create(const Path &path, FileNode::Mode mode)
{
   if(mode & FileNode::File)
      return new IOSFile(path, buildFilename(_volume, path));
   
   if(mode & FileNode::Directory)
   {
      String file = buildFilename(_volume, path);
      
      if(mkdir(file.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0)
         return new IOSDirectory(path, file);
   }
   
   return 0;
}

bool IOSFileSystem::remove(const Path &path)
{
   String file = buildFilename(_volume, path);
   
   struct stat info;
   int error = stat(file.c_str(), &info);
   if(error < 0)
      return false;
   
   if(S_ISDIR(info.st_mode))
      return !rmdir(file);
   
   return !unlink(file);
}

bool IOSFileSystem::rename(const Path &from, const Path &to)
{
   String fa = buildFilename(_volume, from);
   String fb = buildFilename(_volume, to);
   
   if(!::rename(fa.c_str(), fb.c_str()))
      return true;
   
   return false;
}

Path IOSFileSystem::mapTo(const Path &path)
{
   return buildFilename(_volume, path);
}

Path IOSFileSystem::mapFrom(const Path &path)
{
   const String::SizeType volumePathLen = _volume.length();
   
   String pathStr = path.getFullPath();
   
   if(_volume.compare(pathStr,volumePathLen, String::NoCase))
      return Path();
   
   return pathStr.substr(volumePathLen, pathStr.length() - volumePathLen);
}

//---------------------------------------------
// IOSFile
//---------------------------------------------

IOSFile::IOSFile(const Path& path, String name)
{
   _path = path;
   _name = name;
   _status = Closed;
   _handle = 0;
}

IOSFile::~IOSFile()
{
   if(_handle)
      close();
}

Path IOSFile::getName() const
{
   return _path;
}

FileNode::NodeStatus IOSFile::getStatus() const
{
   return _status;
}

bool IOSFile::getAttributes(Attributes *attr)
{
   struct stat info;
   int error = _handle ? fstat(fileno(_handle), &info) : stat(_name.c_str(), &info);
   
   if(error < 0)
   {
      _updateStatus();
      return false;
   }
   
   copyStatAttributes(info, attr);
   attr->name = _path;
   
   return true;
}

U32 IOSFile::calculateChecksum()
{
   if(!open(Read))
      return 0;
   
   U64 fileSize = getSize();
   U32 bufSize = 1024 * 1024 * 4;
   FrameTemp<U8> buf(bufSize);
   
   U32 crc = CRC::INITIAL_CRC_VALUE;
   
   while(fileSize > 0 )
   {
      U32 bytesRead = getMin(fileSize, bufSize);
      if(read(buf, bytesRead) != bytesRead)
      {
         close();
         return 0;
      }
      
      fileSize -= bytesRead;
      crc = CRC::calculateCRC(buf, bytesRead, crc);
   }
   
   close();
   
   return crc;
}

bool IOSFile::open(AccessMode mode)
{
   close();
   
   if(_name.isEmpty())
   {
      return _status;
   }
   
#ifdef DEBUG_SPEW
   Platform::outputDebugString("[IOSFile] opening '%s'", _name.c_str() );
#endif
   
   const char* fmode = "r";
   
   switch (mode)
   {
      case Read: fmode = "r"; break;
      case Write: fmode = "w"; break;
      case ReadWrite:
      {
         fmode = "r+";
         FILE* temp = fopen(_name.c_str(), "a+");
         fclose(temp);
         break;
      }
      case WriteAppend: fmode = "a"; break;
         break;
         
      default:
         break;
   }
   
   if(!(_handle = fopen(_name.c_str(), fmode)))
   {
      _updateStatus();
      return false;
   }
   
   _status = Open;
   return true;
}

bool IOSFile::close()
{
   if(_handle)
   {
#ifdef DEBUG_SPEW
      Platform::outputDebugString("[IOSFile] closing '%s'", _name.c_str() );
#endif
      
      fflush(_handle);
      fclose(_handle);
      _handle = 0;
   }
   
   _status = Closed;
   return true;
}

U32 IOSFile::getPosition()
{
   if(_status == Open || _status == EndOfFile)
      return ftell(_handle);
   
   return 0;
}

U32 IOSFile::setPosition(U32 delta, SeekMode mode)
{
   if(_status != Open && _status != EndOfFile)
      return 0;
   
   S32 fmode = 0;
   switch (mode)
   {
      case Begin:    fmode = SEEK_SET; break;
      case Current:  fmode = SEEK_CUR; break;
      case End:      fmode = SEEK_END; break;
       default:      break;
   }
   
   if(fseek(_handle, delta, fmode))
   {
      _status = UnknownError;
      return 0;
   }
   
   _status = Open;
   
   return ftell(_handle);
}

U32 IOSFile::read(void *dst, U32 size)
{
   if(_status != Open && _status != EndOfFile)
      return 0;
   
   U32 bytesRead = fread(dst, 1, size, _handle);
   
   if(bytesRead != size)
   {
      if(feof(_handle))
         _status = EndOfFile;
      else
         _updateStatus();
   }
   
   return bytesRead;
}

U32 IOSFile::write(const void *src, U32 size)
{
   if((_status != Open && _status != EndOfFile) || !size)
      return 0;
   
   U32 bytesWritten = fwrite(src, 1, size, _handle);
   
   if(bytesWritten != size)
      _updateStatus();
   
   return bytesWritten;
}

void IOSFile::_updateStatus()
{
   switch (errno) {
      case EACCES:   _status = AccessDenied;    break;
      case ENOSPC:   _status = FileSystemFull;  break;
      case ENOTDIR:  _status = NoSuchFile;      break;
      case ENOENT:   _status = NoSuchFile;      break;
      case EISDIR:   _status = AccessDenied;    break;
      case EROFS:    _status = AccessDenied;    break;
      default:       _status = UnknownError;    break;
   }
}

//---------------------------------------------
// IOSDirectory
//---------------------------------------------

IOSDirectory::IOSDirectory(const Path& path, String name)
{
   _path = path;
   _name = name;
   _status = Closed;
   _handle = 0;
}

IOSDirectory::~IOSDirectory()
{
   if(_handle)
      close();
}

Path IOSDirectory::getName() const
{
   return _path;
}

bool IOSDirectory::open()
{
   if((_handle = opendir(_name)) == 0)
   {
      _updateStatus();
      return false;
   }
   
   _status = Open;
   return true;
}

bool IOSDirectory::close()
{
   if(_handle)
   {
      closedir(_handle);
      _handle = NULL;
      return true;
   }
   
   return false;
}

bool IOSDirectory::read(Attributes *entry)
{
   if(_status != Open)
      return false;
   
   struct dirent* de = readdir(_handle);
   
   if(!de)
   {
      _status = EndOfFile;
      return false;
   }
   
   
   if(de->d_name[0] == '.' && (de->d_name[1] == '\0' ||
      (de->d_name[1] == '.' && de->d_name[2] == '\0')))
      return read(entry);
   
   struct stat info;
   String file = _name + "/" + de->d_name;
   
   int error = stat(file.c_str(), &info);
   
   if(error < 0)
   {
      _updateStatus();
      return false;
   }
   
   copyStatAttributes(info, entry);
   entry->name = de->d_name;
   return true;
}

U32 IOSDirectory::calculateChecksum()
{
   return 0;
}

bool IOSDirectory::getAttributes(Attributes* attr)
{
   struct stat info;
   if(stat(_name.c_str(), &info))
   {
      _updateStatus();
      return false;
   }
   
   copyStatAttributes(info, attr);
   attr->name = _path;
   return true;
}

FileNode::NodeStatus IOSDirectory::getStatus() const
{
   return _status;
}

void IOSDirectory::_updateStatus()
{
   switch (errno) {
      case EACCES:   _status = AccessDenied; break;
      case ENOTDIR:  _status = NoSuchFile;   break;
      case ENOENT:   _status = NoSuchFile;   break;
      default:       _status = UnknownError; break;
   }
}

}// namespace ios

}// namespace torque

Torque::FS::FileSystemRef Platform::FS::createNativeFS(const String &volume)
{
   return new IOS::IOSFileSystem(volume);
}

String Platform::FS::getAssetDir()
{
   return Platform::getExecutablePath();
}

bool Platform::FS::InstallFileSystems()
{
   Platform::FS::Mount("/",  Platform::FS::createNativeFS(String() ) );
   
   char buffer[PATH_MAX];
   if(::getcwd(buffer, sizeof(buffer)))
   {
      if(buffer[dStrlen(buffer) - 1] != '/')
         dStrcat(buffer, "/", PATH_MAX);
      
      Platform::FS::SetCwd(buffer);
   }
   
   if(char* home = getenv("HOME"))
      Platform::FS::Mount("home", Platform::FS::createNativeFS(home));
   
   return true;
}
