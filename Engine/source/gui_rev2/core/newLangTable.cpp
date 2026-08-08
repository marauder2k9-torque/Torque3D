//-----------------------------------------------------------------------------
// gui_rev2/core/newLangTable.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "core/stream/stream.h"
#include "core/stream/fileStream.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "gui_rev2/core/newLangTable.h"
#include "gui_rev2/core/newGuiCanvas.h"

//-----------------------------------------------------------------------------
// NewLangFile
//-----------------------------------------------------------------------------

U32 NewLangFile::addString(const String& str)
{
   mStrings.push_back(str);
   return mStrings.size() - 1;
}

const char* NewLangFile::getString(U32 id) const
{
   if (id >= mStrings.size())
      return NULL;
   return mStrings[id].c_str();
}

bool NewLangFile::load(Stream* stream, Vector<String>& outNames)
{
   if (!stream)
      return false;

   // Read the whole file into memory first - see this method's header
   // doc comment for why (packed-file decryption needs to happen on a
   // byte buffer, in between "read from disk" and "parse text", and
   // that's much simpler as two separate steps than as one Stream
   // wrapping a decrypt filter).
   U32 size = stream->getStreamSize();
   if (size == 0)
      return true;   // Empty file is not an error - just no strings.

   Vector<char> raw;
   raw.setSize(size);

   if (!stream->read(size, raw.address()))
   {
      Con::errorf("NewLangFile::load - failed reading %u bytes from stream.", size);
      return false;
   }

   return loadFromText(raw.address(), size, outNames);
}

bool NewLangFile::loadFromText(const char* text, U32 length, Vector<String>& outNames)
{
   if (!text)
      return false;

   char line[2048];
   U32 lineNum = 0;

   U32 pos = 0;
   while (pos < length)
   {
      // Copy one line (up to '\n' or end of buffer) into the fixed
      // working buffer - same per-line parsing as before, just fed from
      // an in-memory buffer instead of Stream::readLine().
      U32 lineLen = 0;
      while (pos < length && text[pos] != '\n' && lineLen < sizeof(line) - 1)
         line[lineLen++] = text[pos++];

      while (pos < length && text[pos] != '\n')
         pos++;   // Discard any remainder past the working buffer's capacity.
      if (pos < length)
         pos++;   // Skip the '\n' itself.

      // Strip a trailing '\r' (CRLF source files), same as Stream::readLine() would.
      if (lineLen > 0 && line[lineLen - 1] == '\r')
         lineLen--;
      line[lineLen] = 0;

      lineNum++;

      // Trim leading whitespace so indentation/comments are tolerant of authoring style.
      char* start = line;
      while (*start == ' ' || *start == '\t')
         start++;

      if (*start == 0 || *start == '#' || *start == ';')
         continue;   // Blank line or comment.

      char* sep = dStrstr(start, "=");
      if (!sep)
      {
         Con::warnf("NewLangFile::load - line %u has no '=' separator, skipping: %s", lineNum, start);
         continue;
      }

      // NAME is everything before '=', trimmed; value is everything after, trimmed of one leading space.
      char* nameEnd = sep;
      while (nameEnd > start && (*(nameEnd - 1) == ' ' || *(nameEnd - 1) == '\t'))
         nameEnd--;

      if (nameEnd == start)
      {
         Con::warnf("NewLangFile::load - line %u has an empty name, skipping.", lineNum);
         continue;
      }

      String name(start, nameEnd - start);

      char* valueStart = sep + 1;
      while (*valueStart == ' ' || *valueStart == '\t')
         valueStart++;

      String value(valueStart);

      outNames.push_back(name);
      addString(value);
   }

   return true;
}

//-----------------------------------------------------------------------------
// .langpack format - light obfuscation, NOT real encryption. See
// NewLangTable's class doc comment (newLangTable.h) for the threat model
// this is and isn't meant to cover.
//-----------------------------------------------------------------------------

namespace
{
   // 4-byte magic + 1-byte version. A file starting with anything else
   // is treated as plain text - this is the entire "auto-detect" logic
   // in addLanguage().
   const U8 kLangPackMagic[4] = { 'N', 'L', 'P', 'K' };
   const U8 kLangPackVersion = 1;
   const U32 kLangPackHeaderSize = 5;

   // Compiled into the binary, not authored/configurable - see this
   // file's top-of-section doc comment. Any fixed byte sequence works
   // for XOR obfuscation; length matters more than content (a longer key
   // means less periodic repetition visible to a hex-editor skim).
   const U8 kLangPackKey[16] =
   {
      0x4E, 0x65, 0x77, 0x47, 0x75, 0x69, 0x4C, 0x61,
      0x6E, 0x67, 0x21, 0x9F, 0x3C, 0x71, 0xE2, 0x08
   };

   // Symmetric - the same call encodes and decodes.
   void xorBuffer(U8* data, U32 length)
   {
      for (U32 i = 0; i < length; i++)
         data[i] ^= kLangPackKey[i % sizeof(kLangPackKey)];
   }
}

//-----------------------------------------------------------------------------
// NewLangTable - static class, see newLangTable.h's class doc comment for why.
//-----------------------------------------------------------------------------

IMPLEMENT_STATIC_CLASS(NewLangTable, ,
   "Loads and switches between GUI localization string tables."
);

ConsoleDoc(
   "@class NewLangTable\n"
   "@ingroup GuiCore\n"
   "@brief Loads and switches between GUI localization string tables.\n"
);

Vector<NewLangFile*> NewLangTable::smLanguages;
Vector<String> NewLangTable::smNames;
Vector<U32> NewLangTable::smIds;
S32 NewLangTable::smDefaultLanguage = -1;
S32 NewLangTable::smCurrentLanguage = -1;

S32 NewLangTable::addLanguage(const char* filename, const char* languageName)
{
   FileStream* stream = FileStream::createAndOpen(filename, Torque::FS::File::Read);
   if (!stream)
   {
      Con::errorf("NewLangTable::addLanguage - couldn't open '%s'", filename);
      return -1;
   }

   U32 fileSize = stream->getStreamSize();
   Vector<U8> raw;
   raw.setSize(fileSize);

   bool readOk = (fileSize == 0) || stream->read(fileSize, raw.address());
   delete stream;

   if (!readOk)
   {
      Con::errorf("NewLangTable::addLanguage - failed reading '%s'.", filename);
      return -1;
   }

   // Auto-detect: a packed file starts with kLangPackMagic + version;
   // anything else (including a too-short file) is treated as plain text.
   bool isPacked = (fileSize >= kLangPackHeaderSize) &&
      dMemcmp(raw.address(), kLangPackMagic, sizeof(kLangPackMagic)) == 0;

   const char* textStart = (const char*)raw.address();
   U32 textLength = fileSize;

   if (isPacked)
   {
      if (raw[(U32)sizeof(kLangPackMagic)] != kLangPackVersion)
      {
         Con::errorf("NewLangTable::addLanguage - '%s' is a .langpack of an unsupported version.", filename);
         return -1;
      }

      // Decrypt in place, after the header - loud failure (above) rather
      // than silently misinterpreting a version we don't understand.
      textStart = (const char*)(raw.address() + kLangPackHeaderSize);
      textLength = fileSize - kLangPackHeaderSize;
      xorBuffer((U8*)textStart, textLength);
   }

   NewLangFile* file = new NewLangFile((languageName && languageName[0]) ? languageName : filename);

   Vector<String> names;
   bool ok = file->loadFromText(textStart, textLength, names);

   if (!ok)
   {
      delete file;
      return -1;
   }

   bool isFirstLanguage = smLanguages.empty();

   for (U32 i = 0; i < names.size(); i++)
   {
      if (getIdForName(names[i].c_str()) == NewLang_InvalidId)
      {
         // First time this NAME has been seen - its id is whatever slot
         // it landed in in the file that defined it. This is only
         // guaranteed to match across languages when it's the FIRST
         // language (see addLanguage()'s header doc comment) - a name
         // introduced by a later language just gets appended here.
         smNames.push_back(names[i]);
         smIds.push_back(i);
      }
      else if (isFirstLanguage)
      {
         // Can't happen within a single well-formed file (each name maps
         // 1:1 to its own line's id), but guard against a duplicate NAME
         // authored twice in the same file rather than silently aliasing.
         Con::warnf("NewLangTable::addLanguage - duplicate name '%s' in '%s', keeping first occurrence.",
            names[i].c_str(), filename);
      }
   }

   smLanguages.push_back(file);
   S32 index = smLanguages.size() - 1;

   if (smDefaultLanguage == -1)
      setDefaultLanguage(index);
   if (smCurrentLanguage == -1)
      smCurrentLanguage = index;   // Not through setLanguage() - no canvases likely exist yet at startup, and there's nothing to invalidate the FIRST time a language is set.

   return index;
}

void NewLangTable::setDefaultLanguage(S32 index)
{
   if (index >= 0 && index < smLanguages.size())
      smDefaultLanguage = index;
}

bool NewLangTable::setLanguage(S32 index)
{
   if (index < 0 || index >= smLanguages.size())
      return false;

   smCurrentLanguage = index;

   // The entire invalidation step - see NewGuiCanvas::setAllCanvasesStyleDirty()'s
   // and NewGuiLabel::StylePass()'s own doc comments for why this alone is enough.
   NewGuiCanvas::setAllCanvasesStyleDirty();

   return true;
}

const char* NewLangTable::getLanguageName(S32 index)
{
   if (index < 0 || index >= smLanguages.size())
      return NULL;
   return smLanguages[index]->getLanguageName().c_str();
}

U32 NewLangTable::getIdForName(const char* name)
{
   if (!name || !name[0])
      return NewLang_InvalidId;

   // Case-sensitive: translation keys are authored constants (e.g.
   // "TXT_HELLO_WORLD"), not user-facing text, so there's no reason to
   // tolerate/hide a typo'd case mismatch - it should fail loudly (see
   // translate()'s "??name??" placeholder) rather than silently match.
   // dStrcmp() rather than a String comparison operator - only
   // dStricmp()/dStrnicmp() are confirmed in use elsewhere in this
   // codebase (see newGuiCanvas.cpp's debugCaseInsensitiveContains()),
   // and dStrcmp() is its near-certain case-sensitive sibling.
   for (U32 i = 0; i < smNames.size(); i++)
   {
      if (dStrcmp(smNames[i].c_str(), name) == 0)
         return smIds[i];
   }

   return NewLang_InvalidId;
}

const char* NewLangTable::getString(U32 id)
{
   const char* s = NULL;

   if (smCurrentLanguage >= 0)
      s = smLanguages[smCurrentLanguage]->getString(id);

   if (!s && smDefaultLanguage >= 0 && smDefaultLanguage != smCurrentLanguage)
      s = smLanguages[smDefaultLanguage]->getString(id);

   if (s)
      return s;

   // Loud, visible failure rather than blank text - see this class's header doc comment.
   // Con::getReturnBuffer() rather than a static/local buffer: this can be called several
   // times in one expression (e.g. building a composite string from multiple translate()
   // calls), and a shared static would let a later call silently overwrite an earlier
   // caller's still-in-use result.
   char* buf = Con::getReturnBuffer(32);
   dSprintf(buf, 32, "??%u??", id);
   return buf;
}

const char* NewLangTable::translate(const char* name)
{
   U32 id = getIdForName(name);
   if (id == NewLang_InvalidId)
   {
      char* buf = Con::getReturnBuffer(256);
      dSprintf(buf, 256, "??%s??", name ? name : "(null)");
      return buf;
   }

   return getString(id);
}

bool NewLangTable::packLanguageFile(const char* srcFilename, const char* destFilename)
{
   FileStream* srcStream = FileStream::createAndOpen(srcFilename, Torque::FS::File::Read);
   if (!srcStream)
   {
      Con::errorf("NewLangTable::packLanguageFile - couldn't open '%s' for reading.", srcFilename);
      return false;
   }

   U32 srcSize = srcStream->getStreamSize();
   Vector<U8> text;
   text.setSize(srcSize);

   bool readOk = (srcSize == 0) || srcStream->read(srcSize, text.address());
   delete srcStream;

   if (!readOk)
   {
      Con::errorf("NewLangTable::packLanguageFile - failed reading '%s'.", srcFilename);
      return false;
   }

   // Encrypt AFTER the full source read succeeds, and only open destFilename
   // once we have the complete packed buffer ready - so a failed/partial
   // read never truncates an existing dest, including the src == dest case.
   xorBuffer(text.address(), srcSize);

   FileStream* destStream = FileStream::createAndOpen(destFilename, Torque::FS::File::Write);
   if (!destStream)
   {
      Con::errorf("NewLangTable::packLanguageFile - couldn't open '%s' for writing.", destFilename);
      return false;
   }

   bool writeOk = destStream->write(sizeof(kLangPackMagic), kLangPackMagic)
      && destStream->write((U32)1, &kLangPackVersion)
      && (srcSize == 0 || destStream->write(srcSize, text.address()));

   delete destStream;

   if (!writeOk)
      Con::errorf("NewLangTable::packLanguageFile - failed writing '%s'.", destFilename);

   return writeOk;
}

bool NewLangTable::unpackLanguageFile(const char* srcFilename, const char* destFilename)
{
   FileStream* srcStream = FileStream::createAndOpen(srcFilename, Torque::FS::File::Read);
   if (!srcStream)
   {
      Con::errorf("NewLangTable::unpackLanguageFile - couldn't open '%s' for reading.", srcFilename);
      return false;
   }

   U32 fileSize = srcStream->getStreamSize();
   Vector<U8> raw;
   raw.setSize(fileSize);

   bool readOk = (fileSize == 0) || srcStream->read(fileSize, raw.address());
   delete srcStream;

   if (!readOk)
   {
      Con::errorf("NewLangTable::unpackLanguageFile - failed reading '%s'.", srcFilename);
      return false;
   }

   if (fileSize < kLangPackHeaderSize || dMemcmp(raw.address(), kLangPackMagic, sizeof(kLangPackMagic)) != 0)
   {
      Con::errorf("NewLangTable::unpackLanguageFile - '%s' is not a .langpack file.", srcFilename);
      return false;
   }

   if (raw[(U32)sizeof(kLangPackMagic)] != kLangPackVersion)
   {
      Con::errorf("NewLangTable::unpackLanguageFile - '%s' is a .langpack of an unsupported version.", srcFilename);
      return false;
   }

   U8* textStart = raw.address() + kLangPackHeaderSize;
   U32 textLength = fileSize - kLangPackHeaderSize;
   xorBuffer(textStart, textLength);

   FileStream* destStream = FileStream::createAndOpen(destFilename, Torque::FS::File::Write);
   if (!destStream)
   {
      Con::errorf("NewLangTable::unpackLanguageFile - couldn't open '%s' for writing.", destFilename);
      return false;
   }

   bool writeOk = (textLength == 0) || destStream->write(textLength, textStart);
   delete destStream;

   if (!writeOk)
      Con::errorf("NewLangTable::unpackLanguageFile - failed writing '%s'.", destFilename);

   return writeOk;
}

//-----------------------------------------------------------------------------
// Console methods - script calls these exactly like GFXInit::getAdapterCount().
//-----------------------------------------------------------------------------

DefineEngineStaticMethod(NewLangTable, addLanguage, S32, (const char* filename, const char* languageName), (""),
   "Loads a language from filename and adds it to the table.\n\n"
   "@param filename Path to a plain-text 'NAME = value' language file.\n"
   "@param languageName Human-readable display name for this language; defaults to filename if omitted.\n"
   "@return The language's index (>= 0), or -1 if the file couldn't be read.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::addLanguage(filename, languageName);
}

DefineEngineStaticMethod(NewLangTable, setDefaultLanguage, void, (S32 index), ,
   "Sets which loaded language is read when the current language doesn't define a given string.\n"
   "@ingroup GuiCore")
{
   NewLangTable::setDefaultLanguage(index);
}

DefineEngineStaticMethod(NewLangTable, getDefaultLanguage, S32, (), ,
   "@return The index of the default language, or -1 if none is set.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::getDefaultLanguage();
}

DefineEngineStaticMethod(NewLangTable, setLanguage, bool, (S32 index), ,
   "Switches the active language and restyles every open GUI canvas so all "
   "translated text updates immediately.\n"
   "@return False if index is out of range.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::setLanguage(index);
}

DefineEngineStaticMethod(NewLangTable, getCurrentLanguage, S32, (), ,
   "@return The index of the currently active language, or -1 if none is set.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::getCurrentLanguage();
}

DefineEngineStaticMethod(NewLangTable, getNumLanguages, S32, (), ,
   "@return The number of loaded languages.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::getNumLanguages();
}

DefineEngineStaticMethod(NewLangTable, getLanguageName, const char*, (S32 index), ,
   "@param index Index of a loaded language.\n"
   "@return That language's display name, or an empty string if index is out of range.\n"
   "@ingroup GuiCore")
{
   const char* name = NewLangTable::getLanguageName(index);
   return name ? name : "";
}

DefineEngineStaticMethod(NewLangTable, getIdForName, S32, (const char* name), ,
   "@param name A translation key, e.g. 'TXT_HELLO_WORLD'.\n"
   "@return That key's numeric id, or -1 if no loaded language defines it.\n"
   "@ingroup GuiCore")
{
   U32 id = NewLangTable::getIdForName(name);
   return (id == NewLang_InvalidId) ? -1 : (S32)id;
}

DefineEngineStaticMethod(NewLangTable, translate, const char*, (const char* name), ,
   "Looks up name in the current language (falling back to the default language). "
   "Never fails silently - an unrecognized key returns a visibly-wrong placeholder.\n"
   "@param name A translation key, e.g. 'TXT_HELLO_WORLD'.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::translate(name);
}

DefineEngineStaticMethod(NewLangTable, packLanguageFile, bool, (const char* srcFilename, const char* destFilename), ,
   "Packs a plain-text source language file into an obfuscated .langpack file. "
   "This is light XOR obfuscation, not real encryption - see NewLangTable's class "
   "documentation for what it does and doesn't protect against.\n"
   "@param srcFilename Plain-text 'NAME = value' source file.\n"
   "@param destFilename Where to write the packed .langpack file. May be the same path as srcFilename.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::packLanguageFile(srcFilename, destFilename);
}

DefineEngineStaticMethod(NewLangTable, unpackLanguageFile, bool, (const char* srcFilename, const char* destFilename), ,
   "Reverses packLanguageFile() - writes a .langpack file's original plain text back out.\n"
   "@param srcFilename A .langpack file previously written by packLanguageFile().\n"
   "@param destFilename Where to write the recovered plain-text file.\n"
   "@ingroup GuiCore")
{
   return NewLangTable::unpackLanguageFile(srcFilename, destFilename);
}
