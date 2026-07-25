//-----------------------------------------------------------------------------
// win32Font.cpp — Windows GDI-based font rasterization (PlatformFont
// implementation), createFontInit/createFontShutdown, and font enumeration.
//
// Rewritten from winFont.cpp. This is real, working, dependency-free GDI
// code (no external font/rendering library involved) — kept largely as-is
// rather than replaced, since it already matches the "boot and run without
// external dependencies" direction for this rewrite.
//
// One dependency changed: createFontInit() previously wrote
// winState.appInstance = GetModuleHandle(NULL) — reaching into
// Win32PlatState (platformWin32.h), the native window/render stack's
// global state. Per the same decoupling already applied to win32MsgBox.cpp
// and win32Main.cpp, this file no longer touches winState at all. The
// HINSTANCE this used to stash into winState.appInstance is not actually
// needed here — CreateCompatibleDC(NULL)/EnumFontFamilies don't require an
// HINSTANCE — so that assignment is simply dropped rather than routed
// through getProcessInstance(); it was writing state for some OTHER
// subsystem to read later, which is out of scope for a font-rasterization
// file to be responsible for. If some other subsystem still needs the
// process HINSTANCE, it should call getProcessInstance() (win32Main.cpp)
// directly rather than expect this file to have cached it as a side effect.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformFont.h"

#include "gfx/gFont.h"
#include "gfx/bitmap/gBitmap.h"
#include "math/mRect.h"
#include "console/console.h"
#include "core/strings/unicode.h"
#include "core/strings/stringFunctions.h"
#include "core/stringTable.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>

namespace
{
    HDC sFontHDC = nullptr;
    HBITMAP sFontBMP = nullptr;

    const U32 sCharsetMap[] =
    {
        ANSI_CHARSET,
        SYMBOL_CHARSET,
        SHIFTJIS_CHARSET,
        HANGEUL_CHARSET,
        HANGUL_CHARSET,
        GB2312_CHARSET,
        CHINESEBIG5_CHARSET,
        OEM_CHARSET,
        JOHAB_CHARSET,
        HEBREW_CHARSET,
        ARABIC_CHARSET,
        GREEK_CHARSET,
        TURKISH_CHARSET,
        VIETNAMESE_CHARSET,
        THAI_CHARSET,
        EASTEUROPE_CHARSET,
        RUSSIAN_CHARSET,
        MAC_CHARSET,
        BALTIC_CHARSET,
    };
    constexpr U32 kNumCharsetMap = sizeof(sCharsetMap) / sizeof(U32);
}

void createFontInit()
{
    sFontHDC = ::CreateCompatibleDC(nullptr);
    sFontBMP = ::CreateCompatibleBitmap(sFontHDC, 256, 256);
}

void createFontShutdown()
{
    if (sFontBMP) ::DeleteObject(sFontBMP);
    if (sFontHDC) ::DeleteObject(sFontHDC);
    sFontBMP = nullptr;
    sFontHDC = nullptr;
}

//-----------------------------------------------------------------------------
// WinFont class
//-----------------------------------------------------------------------------
class WinFont final : public PlatformFont
{
    HFONT mFont = nullptr;
    TEXTMETRIC mTextMetric{};

public:
    WinFont() = default;

    ~WinFont() override
    {
        if (mFont)
            ::DeleteObject(mFont);
    }

    bool isValidChar(const UTF16 ch) const override
    {
        return ch != 0;
    }

    bool isValidChar(const UTF8 *str) const override
    {
        return isValidChar(static_cast<UTF16>(oneUTF8toUTF32(str)));
    }

    U32 getFontHeight() const override { return mTextMetric.tmHeight; }
    U32 getFontBaseLine() const override { return mTextMetric.tmAscent; }

    bool create(const char *name, dsize_t size, U32 charset = TGE_ANSI_CHARSET) override
    {
        if (!name || size < 1)
            return false;

        if (charset > kNumCharsetMap)
            charset = TGE_ANSI_CHARSET;

        U32 weight = 0;
        U32 doItalic = 0;

        String nameStr = String(name).trim();

        bool haveModifier;
        do
        {
            haveModifier = false;
            if (nameStr.compare("Bold", 4, String::NoCase | String::Right) == 0)
            {
                weight = 700;
                nameStr = nameStr.substr(0, nameStr.length() - 4).trim();
                haveModifier = true;
            }
            if (nameStr.compare("Italic", 6, String::NoCase | String::Right) == 0)
            {
                doItalic = 1;
                nameStr = nameStr.substr(0, nameStr.length() - 6).trim();
                haveModifier = true;
            }
        } while (haveModifier);

        const UTF16* n = nameStr.utf16();
        mFont = ::CreateFontW(static_cast<int>(size), 0, 0, 0, weight, doItalic, 0, 0,
                               DEFAULT_CHARSET, OUT_TT_PRECIS, 0, PROOF_QUALITY, 0,
                               reinterpret_cast<LPCWSTR>(n));

        if (!mFont)
            return false;

        ::SelectObject(sFontHDC, sFontBMP);
        ::SelectObject(sFontHDC, mFont);
        ::GetTextMetricsW(sFontHDC, &mTextMetric);

        return true;
    }

    PlatformFont::CharInfo &getCharInfo(const UTF16 ch) const override
    {
        static PlatformFont::CharInfo c;
        static U8 scratchPad[65536];

        std::memset(&c, 0, sizeof(c));
        c.bitmapIndex = -1;

        const COLORREF backgroundColorRef = RGB(0, 0, 0);
        const COLORREF foregroundColorRef = RGB(255, 255, 255);
        ::SelectObject(sFontHDC, sFontBMP);
        ::SelectObject(sFontHDC, mFont);
        ::SetBkColor(sFontHDC, backgroundColorRef);
        ::SetTextColor(sFontHDC, foregroundColorRef);

        MAT2 matrix{};
        GLYPHMETRICS metrics{};

        FIXED zero{}; zero.fract = 0; zero.value = 0;
        FIXED one{};  one.fract = 0;  one.value = 1;

        matrix.eM11 = one;
        matrix.eM12 = zero;
        matrix.eM21 = zero;
        matrix.eM22 = one;

        if (::GetGlyphOutlineW(sFontHDC, ch, GGO_GRAY8_BITMAP, &metrics,
                                sizeof(scratchPad), scratchPad, &matrix) != GDI_ERROR)
        {
            const U32 rowStride = (metrics.gmBlackBoxX + 3) & ~3u; // DWORD aligned
            const U32 size = rowStride * metrics.gmBlackBoxY;

            // NOTE (preserved from original): if rowStride * gmBlackBoxY
            // exceeds scratchPad's size, large font sizes could overrun —
            // the range checks below (both here and in the copy loop)
            // guard against writing past scratchPad rather than
            // pre-querying the real required size. A cleaner fix (query
            // GetGlyphOutline with a null buffer first to get the exact
            // size, then allocate exactly that) is a reasonable follow-up
            // but preserves the original's behavior/limits for this port.
            for (U32 j = 0; j < size && j < sizeof(scratchPad); j++)
            {
                U32 pad = static_cast<U32>(scratchPad[j]) << 2;
                if (pad > 255) pad = 255;
                scratchPad[j] = static_cast<U8>(pad);
            }

            c.xOffset = 0;
            c.yOffset = 0;
            c.width = metrics.gmBlackBoxX;
            c.height = metrics.gmBlackBoxY;
            c.xOrigin = metrics.gmptGlyphOrigin.x;
            c.yOrigin = metrics.gmptGlyphOrigin.y;
            c.xIncrement = metrics.gmCellIncX;

            c.bitmapData = new U8[static_cast<size_t>(c.width) * c.height];
            AssertFatal(c.bitmapData != nullptr, "Could not allocate memory for font bitmap data!");

            for (U32 y = 0; static_cast<S32>(y) < c.height; y++)
            {
                for (U32 x = 0; x < c.width; x++)
                {
                    const S32 spi = static_cast<S32>(y * rowStride + x);
                    if (spi >= static_cast<S32>(sizeof(scratchPad)))
                        return c;

                    c.bitmapData[y * c.width + x] = scratchPad[spi];
                }
            }
        }
        else
        {
            SIZE size{};
            ::GetTextExtentPoint32W(sFontHDC, reinterpret_cast<LPCWSTR>(&ch), 1, &size);
            if (size.cx)
            {
                c.xIncrement = size.cx;
                c.bitmapIndex = 0;
            }
        }

        return c;
    }

    PlatformFont::CharInfo &getCharInfo(const UTF8 *str) const override
    {
        return getCharInfo(static_cast<UTF16>(oneUTF8toUTF32(str)));
    }
};

//-----------------------------------------------------------------------------
namespace
{
    BOOL CALLBACK enumFamCallback(LPLOGFONTW logFont, LPNEWTEXTMETRICW, DWORD fontType, LPARAM lParam)
    {
        if (!(fontType & TRUETYPE_FONTTYPE))
            return TRUE;

        auto* fonts = reinterpret_cast<Vector<StringTableEntry>*>(lParam);

        const U32 len = static_cast<U32>(wcslen(logFont->lfFaceName)) * 3 + 1;
        FrameTemp<UTF8> buffer(len);
        convertUTF16toUTF8N(reinterpret_cast<const UTF16*>(logFont->lfFaceName), buffer, len);

        fonts->push_back(StringTable->insert(buffer));

        return TRUE;
    }
}

void PlatformFont::enumeratePlatformFonts(Vector<StringTableEntry>& fonts, UTF16* fontFamily)
{
    ::EnumFontFamiliesW(sFontHDC, reinterpret_cast<LPCWSTR>(fontFamily),
                         reinterpret_cast<FONTENUMPROCW>(enumFamCallback),
                         reinterpret_cast<LPARAM>(&fonts));
}

PlatformFont *createPlatformFont(const char *name, dsize_t size, U32 charset)
{
    PlatformFont* font = new WinFont;

    if (font->create(name, size, charset))
        return font;

    delete font;
    return nullptr;
}
