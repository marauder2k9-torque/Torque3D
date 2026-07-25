//-----------------------------------------------------------------------------
// linuxFont.h — Linux font implementation (LinuxFont), using FreeType for
// rasterization and Fontconfig for name -> file resolution and family
// enumeration.
//
// This is the one place in the Linux platform layer that takes on a real
// external dependency (libfreetype2 + libfontconfig), rather than staying
// libc/kernel-only like every other file here. That's a deliberate,
// explicit exception: Win32 has GDI and macOS has CoreText, both shipped
// by the OS itself with zero extra linking — Linux has no equivalent
// system-provided rasterizer, and FreeType+Fontconfig is the de facto
// standard, ubiquitous (present on effectively every desktop distro and
// trivially available on headless/server distros via package manager)
// solution every other engine/toolkit on Linux uses for exactly this.
//
// Build note: link against `freetype2` and `fontconfig` (e.g. via
// pkg-config: `pkg-config --cflags --libs freetype2 fontconfig`).
//-----------------------------------------------------------------------------
#pragma once

#include "platform/platformFont.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

class LinuxFont : public PlatformFont
{
private:
    FT_Face mFace = nullptr;

    // Distance from drawing point to typographic baseline (== ascent,
    // matching the Torque convention documented in macFont.h).
    U32 mBaseline = 0;

    // Distance between lines.
    U32 mHeight = 0;

    // Owns the FreeType library handle used to load mFace; refcounted at
    // the process level so multiple LinuxFont instances share one
    // FT_Library rather than each initializing/finalizing FreeType
    // themselves (see .cpp for the refcounting helpers).
    static FT_Library smLibrary;
    static U32 smLibraryRefCount;

    static bool acquireLibrary();
    static void releaseLibrary();

    /// Resolves a Torque font name (e.g. "Arial Bold Italic") to an
    /// on-disk font file path via Fontconfig, stripping the Bold/Italic
    /// suffix modifiers the same way win32Font.cpp/macFont.mm do and
    /// translating them into Fontconfig slant/weight match criteria.
    static bool resolveFontFile(const String& baseName, bool bold, bool italic, String* outPath, S32* outFaceIndex);

public:
    LinuxFont() = default;
    ~LinuxFont() override;

    bool isValidChar(const UTF16 ch) const override;
    bool isValidChar(const UTF8* str) const override;

    U32 getFontHeight() const override { return mHeight; }
    U32 getFontBaseLine() const override { return mBaseline; }

    bool create(const char* name, dsize_t size, U32 charset = TGE_ANSI_CHARSET) override;

    PlatformFont::CharInfo& getCharInfo(const UTF16 ch) const override;
    PlatformFont::CharInfo& getCharInfo(const UTF8* str) const override;
};
