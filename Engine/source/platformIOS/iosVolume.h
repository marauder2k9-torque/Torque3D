//
//  iosVolume.h
//  TestProject
//
//  Created by ctrlintelligence on 30/07/2023.
//

#ifndef iosVolume_h
#define iosVolume_h

#ifndef _VOLUME_H_
#include "core/volume.h"
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

namespace Torque
{
using namespace FS;

   namespace IOS
   {
      class IOSFileSystem : public FileSystem
      {
         String _volume;
      public:
         IOSFileSystem(String volume);
         ~IOSFileSystem();
         
         String getTypeStr() const {return "IOS";}
         
         FileNodeRef resolve(const Path& path);
         FileNodeRef create(const Path& path, FileNode::Mode);
         bool remove(const Path& path);
         bool rename(const Path& from, const Path& to);
         Path mapTo(const Path& path);
         Path mapFrom(const Path& path);
      };
   
      class IOSFile : public File
      {
         friend class IOSFileSyatem;
         Path _path;
         String _name;
         FILE* _handle;
         NodeStatus _status;
         
         bool _updateInfo();
         void _updateStatus();
         
      public:
         IOSFile(const Path& path, String name);
         ~IOSFile();
         
         Path getName() const;
         NodeStatus getStatus() const;
         bool getAttributes(Attributes*);
         
         
         U32 getPosition();
         U32 setPosition(U32,SeekMode);
         
         bool open(AccessMode);
         bool close();
         
         U32 read(void* dst, U32 size);
         U32 write(const void* src, U32 size);
         
      private:
         U32 calculateChecksum();
      };
   
      class IOSDirectory : public Directory
      {
         friend class IOSFileSystem;
         Path _path;
         String _name;
         DIR* _handle;
         NodeStatus _status;
         
         void _updateStatus();
         
      public:
         IOSDirectory(const Path& path, String name);
         ~IOSDirectory();
         
         Path getName() const;
         NodeStatus getStatus() const;
         bool getAttributes(Attributes*);
         
         bool open();
         bool close();
         bool read(Attributes*);
         
      private:
         U32 calculateChecksum();
      };
   
   }

}

#endif /* iosVolume_h */
