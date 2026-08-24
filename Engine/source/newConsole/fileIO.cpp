#include "newConsole/fileIO.h"

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

bool readScriptFile(const char* filename, String& outSource, String* outError)
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

   // +1 for a null terminator - a lexer operating over this buffer via a
   // string_view still expects a real destination buffer for the raw
   // Stream::read call; String owns the copy from here on, this buffer
   // is temporary.
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

} // namespace newConsole
