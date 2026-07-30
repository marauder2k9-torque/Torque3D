//-----------------------------------------------------------------------------
// guiStyleValue.h
//
// A small "was this explicitly set, or should it cascade from a lower
// layer" wrapper, used by every field in GuiStyleProperties (see
// guiStyle.h). This is the mechanism that makes the whole style system's
// CSS-like cascade possible: a GuiStyleValue<T> that was never assigned
// contributes nothing when resolving a property, letting a lower layer
// (a less-specific state, or the style's base block) supply the value
// instead -- exactly like an unset CSS property falling through to
// whatever the cascade would otherwise produce.
//
// Layers, most to least specific (see guiStyle.h/guiStyleable.h for where
// each of these actually lives):
//   1. A control's own inline override (GuiStyleable::mInlineOverrides)
//   2. The current interaction state's block on the referenced GuiStyle
//      (e.g. ":hover")
//   3. That GuiStyle's base block
//-----------------------------------------------------------------------------

#ifndef _GUISTYLEVALUE_H_
#define _GUISTYLEVALUE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

/// Wraps a single style property value with an explicit "is this set"
/// flag, so GuiStyleProperties fields can distinguish "never touched" from
/// "explicitly set to the zero/default value of T." See file header for
/// how this drives the style cascade.
template< typename T >
struct GuiStyleValue
{
   bool mIsSet;
   T    mValue;

   GuiStyleValue() : mIsSet( false ), mValue() {}

   bool isSet() const { return mIsSet; }

   void set( const T &value )
   {
      mValue = value;
      mIsSet = true;
   }

   void clear()
   {
      mIsSet = false;
      mValue = T();
   }

   /// Returns mValue if set, otherwise falls back to whatever the next
   /// less-specific layer already resolved to (fallback). This is the
   /// single-property cascade step; GuiStyleProperties::resolveOnto()
   /// applies it field-by-field for a whole property block at once.
   const T &resolve( const T &fallback ) const
   {
      return mIsSet ? mValue : fallback;
   }

   /// Convenience for the common "layer B on top of A" cascade step: if
   /// this value is set, overwrite target with it; otherwise leave target
   /// untouched (it already holds whatever a less-specific layer produced).
   void cascadeOnto( T &target ) const
   {
      if ( mIsSet )
         target = mValue;
   }
};

#endif // _GUISTYLEVALUE_H_
