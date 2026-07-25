//-----------------------------------------------------------------------------
// macFont.h — macOS CoreText-based font implementation (OSXFont).
// Ported from macFont.h. Real, native, dependency-free (CoreText is part
// of the OS) — kept as-is.
//-----------------------------------------------------------------------------
#pragma once

#include "platform/platformFont.h"
#include <CoreText/CoreText.h>

class OSXFont : public PlatformFont
{
private:
    // Font reference.
    CTFontRef mFontRef = nullptr;

    // Distance from drawing point to typographic baseline. Think of the
    // drawing point as the upper-left corner of a text box. NOTE:
    // 'baseline' is synonymous with 'ascent' in Torque.
    U32 mBaseline = 0;

    // Distance between lines.
    U32 mHeight = 0;

    // Glyph rendering color-space.
    CGColorSpaceRef mColorSpace = nullptr;

public:
    OSXFont();
    ~OSXFont() override;

    /// Look up the requested font, cache style, layout, colorspace, and metrics.
    bool create(const char* name, dsize_t size, U32 charset = TGE_ANSI_CHARSET) override;

    /// Determine if the requested character is drawable, or should be ignored.
    bool isValidChar(const UTF16 character) const override;
    bool isValidChar(const UTF8* str) const override;

    U32 getFontHeight() const override { return mHeight; }
    U32 getFontBaseLine() const override { return mBaseline; }

    // Draw the character to a temporary bitmap and fill CharInfo with metrics.
    PlatformFont::CharInfo& getCharInfo(const UTF16 character) const override;
    PlatformFont::CharInfo& getCharInfo(const UTF8* str) const override;
};
