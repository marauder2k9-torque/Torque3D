//-----------------------------------------------------------------------------
// guiStyleProperties.h
//
// One state's worth of style values -- what a GuiStyle's base block, or
// any one of its named state blocks (hover/active/focus/disabled/checked/
// error), or a control's own inline override block, actually holds. See
// guiStyle.h for how several of these combine into one resolved result,
// and guiStyleValue.h for the per-field "is this set" mechanism this
// entire cascade is built on.
//
//   backgroundColor, borderColor, borderWidth   -- box appearance
//   textColor, fontFamily, fontSize             -- text appearance
//   letterSpacing, wordSpacing                  -- text tracking/rhythm
//   bitmapAsset                                 -- background/foreground image
//   textAlignHorizontal, textAlignVertical      -- justification
//-----------------------------------------------------------------------------

#ifndef _GUISTYLEPROPERTIES_H_
#define _GUISTYLEPROPERTIES_H_

#ifndef _GUISTYLEVALUE_H_
#include "gui_refactor/core/guiStyleValue.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _IMAGE_ASSET_H_
#include "T3D/assets/ImageAsset.h"
#endif

/// Horizontal text alignment
enum GuiTextAlignHorizontal
{
   GuiTextAlignHorizontal_Left = 0,
   GuiTextAlignHorizontal_Center,
   GuiTextAlignHorizontal_Right
};

DefineEnumType(GuiTextAlignHorizontal);

/// Vertical text alignment
enum GuiTextAlignVertical
{
   GuiTextAlignVertical_Top = 0,
   GuiTextAlignVertical_Middle,
   GuiTextAlignVertical_Bottom
};

DefineEnumType(GuiTextAlignVertical);

/// Shared "R G B [A]" string parser used by every color-valued
/// GuiStyleProperties field setter
inline ColorI GuiStyleParseColor( const char *data )
{
   ColorI color( 255, 255, 255, 255 );
   S32 r = 255, g = 255, b = 255, a = 255;
   S32 count = dSscanf( data, "%d %d %d %d", &r, &g, &b, &a );
   if ( count >= 3 )
   {
      color.red = (U8)r;
      color.green = (U8)g;
      color.blue = (U8)b;
      color.alpha = ( count >= 4 ) ? (U8)a : 255;
   }
   return color;
}

/// One state's worth of style values. See file header.
struct GuiStyleProperties
{
   GuiStyleValue<ColorI> backgroundColor;
   GuiStyleValue<ColorI> borderColor;
   GuiStyleValue<S32>    borderWidth;

   GuiStyleValue<ColorI>           textColor;
   GuiStyleValue<StringTableEntry> fontFamily;
   GuiStyleValue<S32>              fontSize;

   /// Extra space, in logical units, added after every character's own
   /// natural advance width (GFont::getCharXIncrement())
   GuiStyleValue<S32>              letterSpacing;

   /// Extra space, in logical units, added specifically after a SPACE
   /// character (on top of letterSpacing, which still applies to every
   /// character including spaces)
   GuiStyleValue<S32>              wordSpacing;

   GuiStyleValue<GuiTextAlignHorizontal> textAlignHorizontal;
   GuiStyleValue<GuiTextAlignVertical>   textAlignVertical;

   /// Cascades every set field in this block onto target: wherever THIS
   /// block has a value set, it overwrites target's value
   void cascadeOnto( GuiStyleProperties &target ) const
   {
      if ( backgroundColor.isSet() ) target.backgroundColor.set( backgroundColor.mValue );
      if ( borderColor.isSet() )     target.borderColor.set( borderColor.mValue );
      if ( borderWidth.isSet() )     target.borderWidth.set( borderWidth.mValue );
      if ( textColor.isSet() )       target.textColor.set( textColor.mValue );
      if ( fontFamily.isSet() )      target.fontFamily.set( fontFamily.mValue );
      if ( fontSize.isSet() )        target.fontSize.set( fontSize.mValue );
      if ( letterSpacing.isSet() )   target.letterSpacing.set( letterSpacing.mValue );
      if ( wordSpacing.isSet() )     target.wordSpacing.set( wordSpacing.mValue );
      if ( textAlignHorizontal.isSet() ) target.textAlignHorizontal.set( textAlignHorizontal.mValue );
      if ( textAlignVertical.isSet() )   target.textAlignVertical.set( textAlignVertical.mValue );
   }
};

#endif // _GUISTYLEPROPERTIES_H_
