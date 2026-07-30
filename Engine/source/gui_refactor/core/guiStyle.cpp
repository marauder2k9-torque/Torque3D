//-----------------------------------------------------------------------------
// guiStyle.cpp
// See guiStyle.h for the overall design.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "gui_refactor/core/guiStyle.h"


#include "console/consoleTypes.h"
#include "console/console.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT(GuiStyle);

ImplementEnumType(GuiTextAlignHorizontal,
   "Horizontal text alignment.\n\n")
{
   GuiTextAlignHorizontal_Left, "left", "Left-justified (default)."
},
{ GuiTextAlignHorizontal_Center, "center", "Centered." },
{ GuiTextAlignHorizontal_Right, "right", "Right-justified." },
EndImplementEnumType;

ImplementEnumType(GuiTextAlignVertical,
   "Vertical text alignment.\n\n")
{
   GuiTextAlignVertical_Top, "top", "Top-justified."
},
{ GuiTextAlignVertical_Middle, "middle", "Vertically centered (default)." },
{ GuiTextAlignVertical_Bottom, "bottom", "Bottom-justified." },
EndImplementEnumType;

//-----------------------------------------------------------------------------

GuiStyle::GuiStyle()
   : mLoadCount(0)
   , mUseSDF(false)
{
}

//-----------------------------------------------------------------------------

bool GuiStyle::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

//-----------------------------------------------------------------------------

void GuiStyle::onRemove()
{
   Parent::onRemove();
}

//-----------------------------------------------------------------------------

void GuiStyle::incLoadCount()
{
   mLoadCount++;
   if (mLoadCount == 1)
      _loadFonts();
}

//-----------------------------------------------------------------------------

void GuiStyle::decLoadCount()
{
   AssertFatal(mLoadCount > 0, "GuiStyle::decLoadCount - zero load count");
   mLoadCount--;
   // NOTE: fonts/bitmaps are Resource<>/AssetRef<>-managed and release
   // themselves when no longer referenced -- unlike GuiControlProfile's
   // mFont, we don't hold a raw font pointer that needs an explicit null-
   // out here. Nothing to do once mLoadCount reaches 0 beyond the
   // refcount itself dropping when this style's GuiStyleProperties blocks
   // eventually get destroyed or reassigned.
}

//-----------------------------------------------------------------------------

Resource<GFont> GuiStyle::_getFontFor(const GuiStyleProperties& merged) const
{
   StringTableEntry family = merged.fontFamily.isSet() ? merged.fontFamily.mValue : StringTable->insert("Arial");
   S32 size = merged.fontSize.isSet() ? merged.fontSize.mValue : 14;

   static StringTableEntry sFontCacheDirectory = StringTable->insert(Con::getVariable("$GUI::fontCacheDirectory"));
   Resource<GFont> font = GFont::create(family, size, sFontCacheDirectory, TGE_ANSI_CHARSET, mUseSDF);

   // See GFont::noteDisplaySizeUsed()'s doc comment -- create() already
   // records the size it was called with, but re-noting here as well
   // covers the case where a cached .uft (or an SDF font already shared
   // by an earlier _getFontFor() call at a different size, see
   // GFont::create()'s size-independent SDF cache path) is returned
   // without going through create()'s "just baked it" path at all.
   if (font != NULL)
      font->cacheDisplaySize(size);

   return font;
}

//-----------------------------------------------------------------------------

void GuiStyle::_loadFonts()
{
   // Load the base font, plus a distinct font for any state that
   // overrides fontFamily/fontSize -- most styles will only ever need
   // the base font since state blocks typically only touch colors.
   _getFontFor(mBase);

   for (U32 i = 0; i < (U32)GuiStyleState::Count; ++i)
   {
      if (mStates[i].fontFamily.isSet() || mStates[i].fontSize.isSet())
      {
         GuiStyleProperties merged = mBase;
         mStates[i].cascadeOnto(merged);
         _getFontFor(merged);
      }
   }
}

//-----------------------------------------------------------------------------
// Field registration.
//
// Naming convention: base properties are unprefixed (e.g.
// "backgroundColor"); state variants are prefixed with the state's name
// (e.g. "hoverBackgroundColor", "activeBorderWidth", "disabledTextColor").
// This is the closest equivalent to the old fillColorHL/fillColorSEL/etc
// naming that still reads unambiguously with a much larger property list.
//
// Every field below goes through a generated protected setter rather than
// a plain addField -- a plain addField would write straight into a
// GuiStyleValue<T>'s mValue without ever setting mIsSet, which would
// silently defeat the entire cascade (an unset property needs to stay
// distinguishable from one explicitly set to its zero/default value; see
// guiStyleValue.h). GUISTYLE_DEFINE_SETTER generates one small static
// setter per property x state combination that parses the incoming
// string with the same console-type machinery a plain field would have
// used, then calls .set() so mIsSet is correctly marked.
//-----------------------------------------------------------------------------

// Generates "bool GuiStyle::set_<member>Prot(...)" which parses `data`
// as ConsoleValueType and stores it via GuiStyleValue<T>::set(), correctly
// marking mIsSet. `member` is the full member-access expression (e.g.
// mBase.backgroundColor or mStates[(U32)GuiStyleState::Hover].backgroundColor).
#define GUISTYLE_DEFINE_SETTER( funcName, member, CType, parseExpr ) \
   bool GuiStyle::funcName( void *object, const char *index, const char *data ) \
   { \
      GuiStyle *style = static_cast<GuiStyle*>( object ); \
      style->member.set( (CType)( parseExpr ) ); \
      return false; \
   }

GUISTYLE_DEFINE_SETTER(setBaseBackgroundColorProt, mBase.backgroundColor, ColorI, GuiStyleParseColor(data))
GUISTYLE_DEFINE_SETTER(setBaseBorderColorProt, mBase.borderColor, ColorI, GuiStyleParseColor(data))
GUISTYLE_DEFINE_SETTER(setBaseBorderWidthProt, mBase.borderWidth, S32, dAtoi(data))
GUISTYLE_DEFINE_SETTER(setBaseTextColorProt, mBase.textColor, ColorI, GuiStyleParseColor(data))
GUISTYLE_DEFINE_SETTER(setBaseFontFamilyProt, mBase.fontFamily, StringTableEntry, StringTable->insert(data))
GUISTYLE_DEFINE_SETTER(setBaseFontSizeProt, mBase.fontSize, S32, dAtoi(data))
GUISTYLE_DEFINE_SETTER(setBaseLetterSpacingProt, mBase.letterSpacing, S32, dAtoi(data))
GUISTYLE_DEFINE_SETTER(setBaseWordSpacingProt, mBase.wordSpacing, S32, dAtoi(data))
GUISTYLE_DEFINE_SETTER(setBaseTextAlignHProt, mBase.textAlignHorizontal, GuiTextAlignHorizontal, (GuiTextAlignHorizontal)dAtoi(data))
GUISTYLE_DEFINE_SETTER(setBaseTextAlignVProt, mBase.textAlignVertical, GuiTextAlignVertical, (GuiTextAlignVertical)dAtoi(data))

#define GUISTYLE_DEFINE_STATE_SETTERS( stateName, stateEnum ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##BackgroundColorProt, mStates[(U32)GuiStyleState::stateEnum].backgroundColor, ColorI, GuiStyleParseColor( data ) ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##BorderColorProt,     mStates[(U32)GuiStyleState::stateEnum].borderColor,     ColorI, GuiStyleParseColor( data ) ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##BorderWidthProt,     mStates[(U32)GuiStyleState::stateEnum].borderWidth,     S32,    dAtoi( data ) ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##TextColorProt,       mStates[(U32)GuiStyleState::stateEnum].textColor,       ColorI, GuiStyleParseColor( data ) ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##FontFamilyProt,      mStates[(U32)GuiStyleState::stateEnum].fontFamily,      StringTableEntry, StringTable->insert( data ) ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##FontSizeProt,        mStates[(U32)GuiStyleState::stateEnum].fontSize,        S32,    dAtoi( data ) ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##LetterSpacingProt,   mStates[(U32)GuiStyleState::stateEnum].letterSpacing,   S32,    dAtoi( data ) ) \
   GUISTYLE_DEFINE_SETTER( set##stateName##WordSpacingProt,     mStates[(U32)GuiStyleState::stateEnum].wordSpacing,     S32,    dAtoi( data ) )

GUISTYLE_DEFINE_STATE_SETTERS(Hover, Hover)
GUISTYLE_DEFINE_STATE_SETTERS(Active, Active)
GUISTYLE_DEFINE_STATE_SETTERS(Focus, Focus)
GUISTYLE_DEFINE_STATE_SETTERS(Disabled, Disabled)
GUISTYLE_DEFINE_STATE_SETTERS(Checked, Checked)
GUISTYLE_DEFINE_STATE_SETTERS(Error, Error)

#undef GUISTYLE_DEFINE_STATE_SETTERS
#undef GUISTYLE_DEFINE_SETTER

//-----------------------------------------------------------------------------

void GuiStyle::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Base");

   addField("useSDF", TypeBool, Offset(mUseSDF, GuiStyle),
      "Requests signed-distance-field rendering for every font this style resolves to (base or any state's "
      "font override). Off by default -- opt-in, and only takes effect when the resolved font family is a "
      "direct .ttf/.otf/.ttc file (see GFont::create()'s useSDF parameter); requesting it for an "
      "OS-installed face name is silently ignored.");

   addProtectedField("backgroundColor", TypeColorI, Offset(mBase.backgroundColor.mValue, GuiStyle), &setBaseBackgroundColorProt, &defaultProtectedGetFn,
      "Base background/fill color.");
   addProtectedField("borderColor", TypeColorI, Offset(mBase.borderColor.mValue, GuiStyle), &setBaseBorderColorProt, &defaultProtectedGetFn,
      "Base border color.");
   addProtectedField("borderWidth", TypeS32, Offset(mBase.borderWidth.mValue, GuiStyle), &setBaseBorderWidthProt, &defaultProtectedGetFn,
      "Base border width in logical units. 0 means no border.");
   addProtectedField("textColor", TypeColorI, Offset(mBase.textColor.mValue, GuiStyle), &setBaseTextColorProt, &defaultProtectedGetFn,
      "Base text color.");
   addProtectedField("fontFamily", TypeString, Offset(mBase.fontFamily.mValue, GuiStyle), &setBaseFontFamilyProt, &defaultProtectedGetFn,
      "Base font face name.");
   addProtectedField("fontSize", TypeS32, Offset(mBase.fontSize.mValue, GuiStyle), &setBaseFontSizeProt, &defaultProtectedGetFn,
      "Base font size.");
   addProtectedField("letterSpacing", TypeS32, Offset(mBase.letterSpacing.mValue, GuiStyle), &setBaseLetterSpacingProt, &defaultProtectedGetFn,
      "Base extra space, in logical units, added after every character's own natural advance width.");
   addProtectedField("wordSpacing", TypeS32, Offset(mBase.wordSpacing.mValue, GuiStyle), &setBaseWordSpacingProt, &defaultProtectedGetFn,
      "Base extra space, in logical units, added specifically after a space character (on top of letterSpacing).");
   addProtectedField("textAlignHorizontal", TYPEID< GuiTextAlignHorizontal >(), Offset(mBase.textAlignHorizontal.mValue, GuiStyle), &setBaseTextAlignHProt, &defaultProtectedGetFn,
      "Base horizontal text alignment: 0=left, 1=center, 2=right.");
   addProtectedField("textAlignVertical", TYPEID< GuiTextAlignVertical >(), Offset(mBase.textAlignVertical.mValue, GuiStyle), &setBaseTextAlignVProt, &defaultProtectedGetFn,
      "Base vertical text alignment: 0=top, 1=middle, 2=bottom.");

   endGroup("Base");

#define ADD_STATE_GROUP( stateName, stateEnum ) \
      addGroup( #stateName ); \
         addProtectedField( #stateName "BackgroundColor", TypeColorI, Offset( mStates[(U32)GuiStyleState::stateEnum].backgroundColor.mValue, GuiStyle ), &set##stateName##BackgroundColorProt, &defaultProtectedGetFn, "" ); \
         addProtectedField( #stateName "BorderColor",     TypeColorI, Offset( mStates[(U32)GuiStyleState::stateEnum].borderColor.mValue, GuiStyle ), &set##stateName##BorderColorProt, &defaultProtectedGetFn, "" ); \
         addProtectedField( #stateName "BorderWidth",     TypeS32,    Offset( mStates[(U32)GuiStyleState::stateEnum].borderWidth.mValue, GuiStyle ), &set##stateName##BorderWidthProt, &defaultProtectedGetFn, "" ); \
         addProtectedField( #stateName "TextColor",       TypeColorI, Offset( mStates[(U32)GuiStyleState::stateEnum].textColor.mValue, GuiStyle ), &set##stateName##TextColorProt, &defaultProtectedGetFn, "" ); \
         addProtectedField( #stateName "FontFamily",      TypeString, Offset( mStates[(U32)GuiStyleState::stateEnum].fontFamily.mValue, GuiStyle ), &set##stateName##FontFamilyProt, &defaultProtectedGetFn, "" ); \
         addProtectedField( #stateName "FontSize",        TypeS32,    Offset( mStates[(U32)GuiStyleState::stateEnum].fontSize.mValue, GuiStyle ), &set##stateName##FontSizeProt, &defaultProtectedGetFn, "" ); \
         addProtectedField( #stateName "LetterSpacing",   TypeS32,    Offset( mStates[(U32)GuiStyleState::stateEnum].letterSpacing.mValue, GuiStyle ), &set##stateName##LetterSpacingProt, &defaultProtectedGetFn, "" ); \
         addProtectedField( #stateName "WordSpacing",     TypeS32,    Offset( mStates[(U32)GuiStyleState::stateEnum].wordSpacing.mValue, GuiStyle ), &set##stateName##WordSpacingProt, &defaultProtectedGetFn, "" ); \
      endGroup( #stateName );

   ADD_STATE_GROUP(Hover, Hover);
   ADD_STATE_GROUP(Active, Active);
   ADD_STATE_GROUP(Focus, Focus);
   ADD_STATE_GROUP(Disabled, Disabled);
   ADD_STATE_GROUP(Checked, Checked);
   ADD_STATE_GROUP(Error, Error);

#undef ADD_STATE_GROUP
}
