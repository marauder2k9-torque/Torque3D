//-----------------------------------------------------------------------------
// macFont.mm — macOS CoreText-based font implementation.
// Ported from macFont.mm. Real, native, dependency-free — kept as-is.
//-----------------------------------------------------------------------------
#import <Cocoa/Cocoa.h>
#import "platform/platform.h"
#import "core/util/tVector.h"
#import "math/mMathFn.h"
#import "platform/macos/macFont.h"
#import "core/stringTable.h"
#import "core/strings/unicode.h"
#import "console/console.h"

//------------------------------------------------------------------------------
PlatformFont* createPlatformFont(const char* name, dsize_t size, U32 charset)
{
    PlatformFont* pFont = new OSXFont();

    if (pFont->create(name, size, charset))
        return pFont;

    delete pFont;
    return nullptr;
}

//------------------------------------------------------------------------------
void PlatformFont::enumeratePlatformFonts(Vector<StringTableEntry>& fonts, UTF16* fontFamily)
{
    @autoreleasepool {
        NSArray* availableFonts = [[NSFontManager sharedFontManager] availableFontNamesWithTraits:0];

        for (NSString* fontName in availableFonts)
        {
            fonts.push_back(StringTable->insert([fontName UTF8String]));
        }
    }
}

//------------------------------------------------------------------------------
OSXFont::OSXFont()
{
}

OSXFont::~OSXFont()
{
    if (mColorSpace)
        CGColorSpaceRelease(mColorSpace);
    if (mFontRef)
        CFRelease(mFontRef);
}

//------------------------------------------------------------------------------
bool OSXFont::create(const char* name, dsize_t size, U32 charset)
{
    AssertFatal(name != nullptr, "Cannot create a NULL font name.");

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

    NSString* fontName = [NSString stringWithUTF8String:nameStr.c_str()];

    if (!fontName)
    {
        Con::errorf("Could not handle font name of '%s'.", name);
        return false;
    }

    NSMutableDictionary* fontAttributes = [NSMutableDictionary dictionaryWithObjectsAndKeys:
        fontName, (NSString*)kCTFontFamilyNameAttribute,
        [NSNumber numberWithFloat:(float)size], (NSString*)kCTFontSizeAttribute,
        nil];

    CTFontSymbolicTraits traits = 0x0;
    if (doBold)   traits |= kCTFontBoldTrait;
    if (doItalic) traits |= kCTFontItalicTrait;

    CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes((CFDictionaryRef)fontAttributes);

    mFontRef = CTFontCreateWithFontDescriptor(descriptor, 0.0, nullptr);
    CFRelease(descriptor);

    if (!mFontRef)
    {
        Con::errorf("Could not generate a font reference to font name '%s' of size '%llu'", name, static_cast<unsigned long long>(size));
        return false;
    }

    // Apply symbolic traits, if any, by creating a modified copy of the font.
    if (traits != 0x0)
    {
        CTFontRef styledFont = CTFontCreateCopyWithSymbolicTraits(mFontRef, (float)size, nullptr, traits, traits);
        if (styledFont)
        {
            CFRelease(mFontRef);
            mFontRef = styledFont;
        }
    }

    const CGFloat ascent = CTFontGetAscent(mFontRef);
    const CGFloat descent = CTFontGetDescent(mFontRef);

    mBaseline = static_cast<U32>(mRound(ascent));
    mHeight = static_cast<U32>(mRound(ascent + descent));

    mColorSpace = CGColorSpaceCreateDeviceGray();

    return true;
}

//------------------------------------------------------------------------------
bool OSXFont::isValidChar(const UTF8* str) const
{
    // Only low-order characters are invalid, and those are single
    // codeunits in UTF8, so this cast is safe.
    return isValidChar(static_cast<UTF16>(*str));
}

bool OSXFont::isValidChar(const UTF16 character) const
{
    // ASCII control characters (< 0x20 / space) are excluded; only
    // printable characters are valid.
    if (character < 0x20)
        return false;

    return true;
}

//------------------------------------------------------------------------------
PlatformFont::CharInfo& OSXFont::getCharInfo(const UTF8* str) const
{
    return getCharInfo(oneUTF32toUTF16(oneUTF8toUTF32(str, nullptr)));
}

PlatformFont::CharInfo& OSXFont::getCharInfo(const UTF16 character) const
{
    static PlatformFont::CharInfo characterInfo;
    dMemset(&characterInfo, 0, sizeof(characterInfo));

    characterInfo.bitmapIndex = 0;
    characterInfo.xOffset = 0;
    characterInfo.yOffset = 0;

    CGGlyph characterGlyph;
    CGRect characterBounds;
    CGSize characterAdvances;
    UniChar unicodeCharacter = character;

    if (!CTFontGetGlyphsForCharacters(mFontRef, &unicodeCharacter, &characterGlyph, (CFIndex)1))
    {
        Con::warnf("Font glyph is messed up. Some characters may render incorrectly.");
    }

    CTFontGetBoundingRectsForGlyphs(mFontRef, kCTFontHorizontalOrientation, &characterGlyph, &characterBounds, (CFIndex)1);
    CTFontGetAdvancesForGlyphs(mFontRef, kCTFontHorizontalOrientation, &characterGlyph, &characterAdvances, (CFIndex)1);

    characterInfo.xOrigin = static_cast<S32>(mRound(characterBounds.origin.x));
    characterInfo.yOrigin = static_cast<S32>(mRound(characterBounds.origin.y));
    characterInfo.width = static_cast<U32>(mCeil(characterBounds.size.width)) + 2;
    characterInfo.height = static_cast<U32>(mCeil(characterBounds.size.height)) + 2;
    characterInfo.xIncrement = static_cast<S32>(mRound(characterAdvances.width));

    if (characterInfo.width == 0 && characterInfo.height == 0)
        return characterInfo;

    if (characterInfo.width == 0)
        characterInfo.width = 2;
    if (characterInfo.height == 0)
        characterInfo.height = 1;

    const U32 bitmapSize = characterInfo.width * characterInfo.height;
    characterInfo.bitmapData = new U8[bitmapSize];
    dMemset(characterInfo.bitmapData, 0x00, bitmapSize);

    CGContextRef bitmapContext = CGBitmapContextCreate(
        characterInfo.bitmapData, characterInfo.width, characterInfo.height,
        8, characterInfo.width, mColorSpace, kCGImageAlphaNone);

    AssertFatal(bitmapContext != nullptr, "Cannot create font context.");

    CGContextSetShouldAntialias(bitmapContext, true);
    CGContextSetShouldSmoothFonts(bitmapContext, true);
    CGContextSetRenderingIntent(bitmapContext, kCGRenderingIntentAbsoluteColorimetric);
    CGContextSetInterpolationQuality(bitmapContext, kCGInterpolationNone);
    CGContextSetGrayFillColor(bitmapContext, 1.0, 1.0);
    CGContextSetTextDrawingMode(bitmapContext, kCGTextFill);

    CGPoint renderOrigin;
    renderOrigin.x = -characterInfo.xOrigin;
    renderOrigin.y = -characterInfo.yOrigin;

    CTFontDrawGlyphs(mFontRef, &characterGlyph, &renderOrigin, 1, bitmapContext);

    // Adjust the y origin for the glyph size.
    characterInfo.yOrigin += characterInfo.height;

    CGContextRelease(bitmapContext);

    return characterInfo;
}
