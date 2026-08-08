//-----------------------------------------------------------------------------
// gui_rev2/core/newLangTable.h
//-----------------------------------------------------------------------------
#ifndef _NEWLANGTABLE_H_
#define _NEWLANGTABLE_H_

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif
#ifndef _ENGINEOBJECT_H_
#include "console/engineObject.h"
#endif

class Stream;

/// Invalid string ID - returned by name lookups that fail, and safe to pass
/// to NewLangTable::translate()/getString() (both treat it as "always missing").
static const U32 NewLang_InvalidId = 0xFFFFFFFF;

/// One language's ID-indexed string table.
///
/// IDs are dense array indices, not hashes - the lookup NewLangTable::
/// translate() does every StylePass for every keyed control is a vector
/// index, deliberately kept as cheap as mCachedInheritedGeneration's own
/// comparison in NewGuiControl::StylePass(). String NAMES (the authored
/// "TXT_HELLO_WORLD" form) are resolved to IDs once, at load time, by
/// NewLangTable::getIdForName() - never re-hashed per StylePass.
class NewLangFile
{
protected:

   Vector<String> mStrings;   ///< Dense, ID-indexed. Empty string at an index is a valid ("string is blank in this language") entry.
   String mLanguageName;      ///< Human-readable ("English", "Deutsch") - display only, never used as a lookup key.

public:

   NewLangFile() {}
   explicit NewLangFile(const String& languageName) : mLanguageName(languageName) {}

   /// Parses a plain-text source file - one entry per line, "NAME = value",
   /// blank lines and lines starting with '#' or ';' ignored. Deliberately
   /// NOT a compiled/binary format: a few thousand lines of text parses in
   /// microseconds, and keeping the shipped format identical to the
   /// translator-edited format means there's no separate build step that
   /// can drift out of sync with what's actually loaded (the old LangFile/
   /// CompileLanguage .lso+.cs pair's failure mode. Names are handed to the caller
   /// via outNames as they're encountered, so NewLangTable can build its
   /// name->id map without re-parsing the file itself.
   /// @param stream Already-open stream to read from. Reads the whole
   /// stream into memory first (via loadFromText()) rather than parsing
   /// line-by-line directly off it, so NewLangTable::addLanguage() can
   /// decrypt a packed file's bytes in between reading and parsing
   /// without needing a second Stream implementation.
   /// @param outNames Filled with each entry's NAME, in file order (parallel to the ids addString() handed out).
   /// @return True if the stream was read without a fatal error (a malformed individual line is skipped with a console warning, not a failure).
   bool load(Stream* stream, Vector<String>& outNames);

   /// Same parsing as load(), but from an already-in-memory text buffer
   /// rather than a Stream - this is what NewLangTable::addLanguage()
   /// actually calls after a packed file has been decrypted in memory,
   /// and what load(Stream*) itself calls after reading a plain file
   /// whole. text need not be null-terminated; length is authoritative.
   bool loadFromText(const char* text, U32 length, Vector<String>& outNames);

   const String& getLanguageName() const { return mLanguageName; }
   void setLanguageName(const String& name) { mLanguageName = name; }

   /// @return The string at id, or NULL if id is out of range (never NewLang_InvalidId
   /// - callers are expected to check that before calling, same as LangFile::getString()'s
   /// old id-range check).
   const char* getString(U32 id) const;

   U32 getNumStrings() const { return mStrings.size(); }

   /// Appends str as a new entry, returning its id. Used only while loading.
   U32 addString(const String& str);
};

/// The single global GUI localization table - a static class in the same
/// shape as GFXInit (see gfxInit.h), not a SimObject. A static class is
/// inherently one instance, which is the point: script (or engine code)
/// can't accidentally construct a second, competing NewLangTable the way
/// two script-created LangTable SimObjects could previously coexist with
/// nothing picking which one a control actually reads. There is exactly
/// one table, the same way there is exactly one GFXInit.
///
/// addLanguage() accepts either a plain-text source file OR a packed
/// .langpack file (see packLanguageFile()) - it auto-detects by magic
/// header, so nothing else in this class needs to know which form a
/// given file is in. Packing is a light XOR-class obfuscation, not real
/// encryption: it stops a curious player double-clicking a .lang file
/// in a text editor and spoiling story content, nothing more. Anyone
/// willing to disassemble the shipped binary can recover the key. If
/// stronger protection is ever needed, that's a different, heavier
/// mechanism than this class provides.
///
/// Every public method below has a DefineEngineStaticMethod(NewLangTable, ...)
/// counterpart in newLangTable.cpp, so script calls this exactly like
/// GFXInit::getAdapterCount(): NewLangTable::addLanguage("en.lang", "English");
/// NewLangTable::setLanguage(1); etc.
class NewLangTable
{
   DECLARE_STATIC_CLASS(NewLangTable);

protected:

   static Vector<NewLangFile*> smLanguages;

   /// NAME -> id, parallel Vector pair rather than a hash map - this
   /// codebase has no confirmed HashTable/dictionary type in active use
   /// (see newLangTable.cpp's note at the top), and lookups here only
   /// happen at addLanguage()/getIdForName() call time, never per-frame
   /// (translate() takes a NAME once per call and resolves to id via this,
   /// but callers - see NewGuiLabel::StylePass() - are expected to intern
   /// their key once at authoring time, not re-resolve it every pass in
   /// a hot loop). A linear scan over a few hundred/thousand names at
   /// load time or on an occasional translate() call is not worth a
   /// dependency on an unverified container type.
   static Vector<String> smNames;
   static Vector<U32> smIds;   ///< Parallel to smNames.

   static S32 smDefaultLanguage;   ///< Index into smLanguages; -1 if none set.
   static S32 smCurrentLanguage;   ///< Index into smLanguages; -1 if none set.

public:

   /// Loads a language from a file and adds it to the table - the file
   /// may be plain-text source OR a packed .langpack (auto-detected, see
   /// this class's own doc comment). The FIRST language ever added
   /// defines the id space (its NAME->id assignments become
   /// authoritative); every language added afterward is expected to
   /// define the same set of NAMEs. A name present in a later file but
   /// not the first is still accepted (appended, gets a new id) but
   /// won't have a translation in any language that doesn't also define
   /// it - translate() falls back to the default language per-id in
   /// that case (see translate()).
   /// @return The language's index (>= 0), or -1 if the file couldn't be read.
   static S32 addLanguage(const char* filename, const char* languageName = "");

   /// Sets which loaded language getString()/translate() reads by default when
   /// the current language doesn't define an id. Does not itself change what's displayed.
   static void setDefaultLanguage(S32 index);
   static S32 getDefaultLanguage() { return smDefaultLanguage; }

   /// Switches the current language, THEN marks every control in every
   /// canvas style-dirty in one call - this is the entire language-switch
   /// operation: no separate generation counter, no separate invalidation pass. The StylePass
   /// sweep that follows re-resolves every mTextKey the same way any
   /// other style change already does.
   /// @return False if index is out of range (nothing changed, nothing invalidated).
   static bool setLanguage(S32 index);
   static S32 getCurrentLanguage() { return smCurrentLanguage; }

   static S32 getNumLanguages() { return smLanguages.size(); }
   static const char* getLanguageName(S32 index);

   /// @return The NAME's id, or NewLang_InvalidId if no loaded language ever defined it.
   static U32 getIdForName(const char* name);

   /// Looks up id in the current language, falling back to the default
   /// language if the current language doesn't define that id (matches
   /// the old LangTable::getString()'s fallback behavior). Never returns
   /// NULL - an id with no string anywhere returns a visibly-wrong
   /// placeholder ("??<id>??"), never blank, per this project's "loud
   /// failure over silent incorrectness" rule.
   static const char* getString(U32 id);

   /// Convenience wrapper: getIdForName() + getString() in one call. This
   /// is what a control resolving mTextKey each StylePass actually calls.
   /// Also never returns NULL, for the same reason as getString().
   static const char* translate(const char* name);

   /// Packs a plain-text source language file into an obfuscated
   /// .langpack file - see this class's .cpp for exactly what
   /// "obfuscated" means here and why it's deliberately light (XOR
   /// against a compiled-in key, not real encryption). Source and dest
   /// may be the same path; the packed bytes are only written after the
   /// full source is read successfully, so a bad srcFilename never
   /// truncates an existing dest.
   /// @return True if srcFilename was read and destFilename was written successfully.
   static bool packLanguageFile(const char* srcFilename, const char* destFilename);

   /// Reverses packLanguageFile() - writes the original plain text back
   /// out. Exists mainly for verifying a pack round-trips correctly and
   /// for a translator wanting to edit an already-packed file; not
   /// needed by addLanguage() itself, which decodes packed files
   /// in-memory without ever writing the plain text back to disk.
   /// @return True if srcFilename was a valid .langpack and destFilename was written.
   static bool unpackLanguageFile(const char* srcFilename, const char* destFilename);
};

#endif // _NEWLANGTABLE_H_
