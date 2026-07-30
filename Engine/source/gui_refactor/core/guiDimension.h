//-----------------------------------------------------------------------------
// guiDimension.h
//
// Replaces GuiControlNew's old horizSizingOptions/vertSizingOptions enums and
// ratio-based parentResized() math (see guiControlNew.h/.cpp pre-rewrite) with
// a small CSS-style value type used for every layout-relevant field on
// GuiControlNew: width, height, minWidth, maxWidth, minHeight, maxHeight,
// left, top, right, bottom.
//
// A GuiDimension is one of:
//   Auto     -- "auto" -- no explicit value; meaning depends on which field
//               it's used for (see guiControlNew.h's resolveLayout() doc
//               comments -- e.g. auto width/height means "keep current
//               size," auto left/top/right/bottom means "not used for
//               positioning on this axis")
//   Pixels   -- an explicit logical-unit value, e.g. "120" or "120px"
//   Percent  -- a fraction of the immediate parent's current resolved size
//               on the matching axis, e.g. "50%"
//
// Parsed ONCE from a TorqueScript string at field-set time (via the
// GuiDimension console type's ConsoleSetType, see guiControlNew.cpp), not
// re-parsed on every layout resolve -- resolveLayout() only ever works with
// already-parsed GuiDimension values.
//-----------------------------------------------------------------------------

#ifndef _GUIDIMENSION_H_
#define _GUIDIMENSION_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STRINGTABLE_H_
#include "console/stringTable.h"
#endif

/// A single CSS-style layout value: auto, an explicit logical-pixel amount,
/// or a percentage of the parent's current size on the relevant axis.
/// See file header for the full explanation.
struct GuiDimension
{
   enum Mode : U8
   {
      Auto = 0,
      Pixels,
      Percent
   };

   Mode mMode;
   F32  mValue; ///< meaningless when mMode == Auto; logical px when Pixels; 0-100 (not 0-1) when Percent

   GuiDimension() : mMode( Auto ), mValue( 0.0f ) {}
   GuiDimension( Mode mode, F32 value ) : mMode( mode ), mValue( value ) {}

   static GuiDimension fromPixels( F32 px ) { return GuiDimension( Pixels, px ); }
   static GuiDimension fromPercent( F32 pct ) { return GuiDimension( Percent, pct ); }
   static GuiDimension autoValue() { return GuiDimension( Auto, 0.0f ); }

   bool isAuto() const { return mMode == Auto; }
   bool isPixels() const { return mMode == Pixels; }
   bool isPercent() const { return mMode == Percent; }

   /// Resolves this value against the given reference length (e.g. the
   /// parent's current width when resolving a horizontal field). Only
   /// meaningful when NOT auto -- callers must check isAuto() themselves
   /// first, since "auto" has field-specific meaning that this generic
   /// resolver can't know about (see guiControlNew.cpp's resolveLayout()).
   F32 resolve( F32 referenceLength ) const
   {
      switch ( mMode )
      {
         case Pixels:  return mValue;
         case Percent: return referenceLength * ( mValue * 0.01f );
         default:      return 0.0f;
      }
   }

   /// Parses a TorqueScript string ("auto", "120", "120px", "50%") into a
   /// GuiDimension. Called once, at field-set time -- see this type's
   /// ConsoleSetType in guiControlNew.cpp.
   static GuiDimension parse( const char *str )
   {
      if ( !str || !str[ 0 ] )
         return autoValue();

      // Skip leading whitespace.
      while ( *str == ' ' || *str == '\t' )
         str++;

      if ( dStricmp( str, "auto" ) == 0 )
         return autoValue();

      // dAtof() itself stops at the first non-numeric character, so the
      // numeric value comes from it directly; we separately scan for a
      // trailing '%' to decide Percent vs Pixels (a trailing "px" suffix,
      // or no suffix at all, both mean Pixels).
      F32 value = dAtof( str );

      const char *scan = str;
      while ( *scan == ' ' || *scan == '\t' || *scan == '-' || *scan == '+' ||
              *scan == '.' || ( *scan >= '0' && *scan <= '9' ) )
         scan++;

      while ( *scan == ' ' || *scan == '\t' )
         scan++;

      if ( *scan == '%' )
         return fromPercent( value );

      // Anything else (bare number, or explicit "px" suffix) is Pixels.
      return fromPixels( value );
   }

   /// Inverse of parse() -- used by this type's ConsoleGetType.
   const char *toString() const
   {
      static const U32 bufSize = 32;
      char *buf = Con::getReturnBuffer( bufSize );
      switch ( mMode )
      {
         case Auto:
            dSprintf( buf, bufSize, "auto" );
            break;
         case Percent:
            dSprintf( buf, bufSize, "%g%%", mValue );
            break;
         case Pixels:
         default:
            dSprintf( buf, bufSize, "%g", mValue );
            break;
      }
      return buf;
   }
};

DECLARE_STRUCT( GuiDimension );
DefineConsoleType( TypeGuiDimension, GuiDimension );

#endif // _GUIDIMENSION_H_
