#include "newConsole/torquescript2/sourceFile.h"

#ifndef _VOLUME_H_
#include "core/volume.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      bool hasScriptExtension(const char* filename)
      {
         if (!filename)
            return false;

         Torque::Path path(filename);
         return path.getExtension().equal(kFileExtension, String::NoCase);
      }

   } // namespace ts2
} // namespace newConsole
