//-----------------------------------------------------------------------------
// linuxFont.cpp — Linux FreeType/Fontconfig-based font implementation.
// See linuxFont.h for the dependency rationale.
//
// FT_Load_Char with FT_LOAD_RENDER (8-bit anti-aliased grayscale bitmap,
// FT_RENDER_MODE_NORMAL) plays the same role here that GetGlyphOutlineW
// (GGO_GRAY8_BITMAP) plays in win32Font.cpp and CTFontDrawGlyphs plays in
// macFont.mm — each produces an 8-bit coverage bitmap for the glyph that
// gets copied into PlatformFont::CharInfo::bitmapData.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/linux/linuxFont.h"

#include "gfx/gFont.h"
#include "core/util/tVector.h"
#include "core/stringTable.h"
#include "core/strings/unicode.h"
#include "core/strings/stringFunctions.h"
#include "console/console.h"

#include <cstring>
#include <cstdlib>

FT_Library LinuxFont::smLibrary = nullptr;
U32 LinuxFont::smLibraryRefCount = 0;

//-----------------------------------------------------------------------------
bool LinuxFont::acquireLibrary()
{
    if (smLibraryRefCount == 0)
    {
        if (FT_Init_FreeType(&smLibrary) != 0)
        {
            Con::errorf("LinuxFont: FT_Init_FreeType failed.");
            smLibrary = nullptr;
            return false;
        }
    }
    ++smLibraryRefCount;
    return true;
}

void LinuxFont::releaseLibrary()
{
    if (smLibraryRefCount == 0)
        return;

    if (--smLibraryRefCount == 0 && smLibrary)
    {
        FT_Done_FreeType(smLibrary);
        smLibrary = nullptr;
    }
}

//-----------------------------------------------------------------------------
bool LinuxFont::resolveFontFile(const String& baseName, bool bold, bool italic, String* outPath, S32* outFaceIndex)
{
    if (!FcInit())
    {
        // FcInit is safe to call repeatedly (it no-ops if already
        // initialized) — a hard failure here means Fontconfig's own
        // config couldn't be loaded at all.
        Con::errorf("LinuxFont: FcInit failed.");
        return false;
    }

    FcPattern* pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>(baseName.c_str()));
    FcPatternAddInteger(pattern, FC_WEIGHT, bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(pattern, FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);

    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern* match = FcFontMatch(nullptr, pattern, &result);
    FcPatternDestroy(pattern);

    if (!match)
        return false;

    FcChar8* fcFile = nullptr;
    S32 fcIndex = 0;
    const bool haveFile = FcPatternGetString(match, FC_FILE, 0, &fcFile) == FcResultMatch;
    FcPatternGetInteger(match, FC_INDEX, 0, &fcIndex);

    if (haveFile && fcFile)
    {
        *outPath = String(reinterpret_cast<const char*>(fcFile));
        *outFaceIndex = fcIndex;
    }

    FcPatternDestroy(match);
    return haveFile;
}

//-----------------------------------------------------------------------------
LinuxFont::~LinuxFont()
{
    if (mFace)
        FT_Done_Face(mFace);
    releaseLibrary();
}

bool LinuxFont::create(const char* name, dsize_t size, U32 charset)
{
    AssertFatal(name != nullptr, "Cannot create a NULL font name.");
    TORQUE_UNUSED(charset); // Fontconfig/FreeType select by family+style, not a Win32-style charset id.

    bool doBold = false;
    bool doItalic = false;

    String nameStr = String(name).trim();

    bool haveModifier;
    do
    {
        haveModifier = false;
        if (nameStr.compare("Bold", 4, String::NoCase | String::Right) == 0)
        {
            doBold = true;
            nameStr = nameStr.substr(0, nameStr.length() - 4).trim();
            haveModifier = true;
        }
        if (nameStr.compare("Italic", 6, String::NoCase | String::Right) == 0)
        {
            doItalic = true;
            nameStr = nameStr.substr(0, nameStr.length() - 6).trim();
            haveModifier = true;
        }
    } while (haveModifier);

    if (!acquireLibrary())
        return false;

    String fontFile;
    S32 faceIndex = 0;
    if (!resolveFontFile(nameStr, doBold, doItalic, &fontFile, &faceIndex))
    {
        Con::errorf("LinuxFont: Fontconfig could not resolve a file for font '%s'.", name);
        releaseLibrary();
        return false;
    }

    if (FT_New_Face(smLibrary, fontFile.c_str(), faceIndex, &mFace) != 0)
    {
        Con::errorf("LinuxFont: FT_New_Face failed for '%s' (resolved to %s).", name, fontFile.c_str());
        releaseLibrary();
        return false;
    }

    // FreeType sizes are in 26.6 fixed-point; 0 for horizontal/vertical
    // resolution defaults to the face's native 72 DPI assumption, which
    // matches the point-size-as-pixel-size convention win32Font.cpp/
    // macFont.mm both use (CreateFontW's nHeight, CTFontCreate's size).
    FT_Set_Char_Size(mFace, 0, static_cast<FT_F26Dot6>(size) * 64, 72, 72);

    const FT_Size_Metrics& metrics = mFace->size->metrics;
    mBaseline = static_cast<U32>(metrics.ascender >> 6);
    mHeight   = static_cast<U32>((metrics.ascender - metrics.descender) >> 6);

    return true;
}

//-----------------------------------------------------------------------------
bool LinuxFont::isValidChar(const UTF16 ch) const
{
    // ASCII control characters (< 0x20 / space) are excluded, matching
    // macFont.mm's convention; everything else is considered valid and
    // simply renders as a missing-glyph box if the face lacks it.
    return ch >= 0x20;
}

bool LinuxFont::isValidChar(const UTF8* str) const
{
    return isValidChar(static_cast<UTF16>(oneUTF8toUTF32(str, nullptr)));
}

//-----------------------------------------------------------------------------
PlatformFont::CharInfo& LinuxFont::getCharInfo(const UTF16 ch) const
{
    static PlatformFont::CharInfo c;
    std::memset(&c, 0, sizeof(c));
    c.bitmapIndex = -1;

    if (!mFace)
        return c;

    if (FT_Load_Char(mFace, ch, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL) != 0)
        return c;

    const FT_GlyphSlot slot = mFace->glyph;
    const FT_Bitmap& bmp = slot->bitmap;

    c.width  = bmp.width;
    c.height = bmp.rows;
    c.xOffset = 0;
    c.yOffset = 0;
    c.xOrigin = slot->bitmap_left;
    // FreeType's bitmap_top is the distance from the baseline UP to the
    // top of the glyph; Torque's yOrigin convention (matching win32/mac's
    // gmptGlyphOrigin/renderOrigin usage) expects the same sense.
    c.yOrigin = slot->bitmap_top;
    c.xIncrement = static_cast<S32>(slot->advance.x >> 6);

    if (c.width == 0 || c.height == 0)
        return c;

    c.bitmapIndex = 0;
    c.bitmapData = new U8[static_cast<size_t>(c.width) * c.height];

    // FreeType's grayscale bitmap is already 0..255 coverage, row-major,
    // with pitch possibly wider than width (padding) — copy row by row
    // rather than assuming pitch == width.
    for (U32 y = 0; y < c.height; ++y)
    {
        const U8* srcRow = bmp.buffer + static_cast<size_t>(y) * static_cast<size_t>(std::abs(bmp.pitch));
        std::memcpy(c.bitmapData + static_cast<size_t>(y) * c.width, srcRow, c.width);
    }

    return c;
}

PlatformFont::CharInfo& LinuxFont::getCharInfo(const UTF8* str) const
{
    return getCharInfo(static_cast<UTF16>(oneUTF8toUTF32(str, nullptr)));
}

//-----------------------------------------------------------------------------
void createFontInit()
{
    // No global font-DC/bitmap setup is needed the way GDI requires
    // (CreateCompatibleDC/CreateCompatibleBitmap) — FreeType's library
    // handle is acquired lazily and refcounted per-LinuxFont instance
    // instead (see acquireLibrary()/releaseLibrary()).
}

void createFontShutdown()
{
}

//-----------------------------------------------------------------------------
void PlatformFont::enumeratePlatformFonts(Vector<StringTableEntry>& fonts, UTF16* fontFamily)
{
    if (!FcInit())
        return;

    FcPattern* pattern = FcPatternCreate();
    if (fontFamily)
    {
        // Convert the (possibly non-null-terminated-by-convention) UTF16
        // family filter to UTF8 for Fontconfig's API.
        const U32 bufLen = static_cast<U32>(dStrlen(reinterpret_cast<const char*>(fontFamily))) * 3 + 1;
        FrameTemp<UTF8> buffer(bufLen);
        convertUTF16toUTF8N(fontFamily, buffer, bufLen);
        FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>(static_cast<UTF8*>(buffer)));
    }

    FcObjectSet* objectSet = FcObjectSetBuild(FC_FAMILY, nullptr);
    FcFontSet* fontSet = FcFontList(nullptr, pattern, objectSet);

    if (fontSet)
    {
        for (int i = 0; i < fontSet->nfont; ++i)
        {
            FcChar8* familyName = nullptr;
            if (FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &familyName) == FcResultMatch && familyName)
                fonts.push_back(StringTable->insert(reinterpret_cast<const char*>(familyName)));
        }
        FcFontSetDestroy(fontSet);
    }

    FcObjectSetDestroy(objectSet);
    FcPatternDestroy(pattern);
}

//-----------------------------------------------------------------------------
PlatformFont* createPlatformFont(const char* name, dsize_t size, U32 charset)
{
    PlatformFont* font = new LinuxFont;

    if (font->create(name, size, charset))
        return font;

    delete font;
    return nullptr;
}
