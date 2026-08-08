#include "newConsole/torquescript2/sourceFile.h"

#ifndef _FILESTREAM_H_
#include "core/stream/fileStream.h"
#endif
#ifndef _VOLUME_H_
#include "core/volume.h"
#endif
#ifndef _STRINGFUNCTIONS_H_
#include "core/strings/stringFunctions.h"
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

bool loadSourceFile(const char* filename, String& outSource, String* outError)
{
   FileStream* stream = FileStream::createAndOpen(filename, Torque::FS::File::Read);
   if (!stream)
   {
      if (outError)
         *outError = "could not open file";
      return false;
   }

   U32 size = stream->getStreamSize();
   if (size == 0)
   {
      outSource = String();
      delete stream;
      return true;
   }

   // +1 for a null terminator - the lexer (see lexer.h) operates over a
   // std::string_view sliced from this String's own buffer, but reading
   // raw bytes off a Stream needs a real destination buffer first; String
   // owns the copy from here on, this buffer is temporary.
   char* buffer = new char[size + 1];
   bool ok = stream->read(size, buffer);
   buffer[size] = '\0';

   if (ok)
      outSource = String(buffer, size);
   else if (outError)
      *outError = "read failed partway through file";

   delete[] buffer;
   delete stream;
   return ok;
}

} // namespace ts2
} // namespace newConsole
