//-----------------------------------------------------------------------------
// guiStyle.h
//
// Replaces GuiControlProfile's single flat set of fields (with parallel
// per-state suffixes like fillColorHL/fillColorSEL/fillColorNA/
// fillColorERR baked into the class itself) with a CSS-like model:
//
//   - A GuiStyle is a named, reusable SimObject (script-creatable via
//     `new GuiStyle(myButtonStyle) { ... };`, same authoring feel as
//     GuiControlProfile today) that many controls can reference by name,
//     like a CSS class.
//   - It holds one GuiStyleProperties "base" block, plus one optional
//     block per named interaction state: hover, active, focus, disabled,
//     checked, error. Each state block is sparse (see guiStyleValue.h) --
//     a state only needs to specify what's DIFFERENT from base.
//   - GuiControlNew (via GuiStyleable, see guiStyleable.h) additionally
//     holds a small per-control INLINE override block, resolved on top of
//     whatever the referenced GuiStyle produces -- exactly like inline
//     style="" beating a class in CSS.
//
// Resolution order for a single property, most to least specific:
//   1. Control's own inline override (if set)
//   2. The GuiStyle's block for the control's current interaction state
//      (if that block sets it)
//   3. The GuiStyle's base block
//   4. A hardcoded engine default (if nothing above set it at all)
//
// See GuiStyle::resolve().
//-----------------------------------------------------------------------------

#ifndef _GUISTYLE_H_
#define _GUISTYLE_H_

#ifndef _SIMBASE_H_
#include "sim/simBase.h"
#endif
#ifndef _GUISTYLEPROPERTIES_H_
#include "gui_refactor/core/guiStyleProperties.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif


/// The named interaction states a GuiStyle can hold a variant block for.
enum class GuiStyleState : U8
{
   Hover = 0,
   Active,
   Focus,
   Disabled,
   Checked,
   Error,

   Count ///< Not a real state; array-sizing sentinel.
};

/// A named, reusable style -- see file header.
class GuiStyle : public SimObject
{
   public:

      typedef SimObject Parent;

   private:

      GuiStyleProperties mBase;
      GuiStyleProperties mStates[ (U32)GuiStyleState::Count ];

      /// How many awake controls currently reference this style
      U32 mLoadCount;

      /// Opt-in, base-level-only (not per-state -- see this field's
      /// script-facing "useSDF" doc comment on initPersistFields()):
      bool mUseSDF;

      /// Loads (or reloads) every distinct font this style's base +
      /// state blocks reference.
      void _loadFonts();

      /// Resolves the (fontFamily, fontSize) pair a given already-merged
      /// GuiStyleProperties block specifies into a loaded font resource.
      Resource<GFont> _getFontFor( const GuiStyleProperties &merged ) const;

      // One generated protected setter per (property, state) 
      static bool setBaseBackgroundColorProt( void *object, const char *index, const char *data );
      static bool setBaseBorderColorProt( void *object, const char *index, const char *data );
      static bool setBaseBorderWidthProt( void *object, const char *index, const char *data );
      static bool setBaseTextColorProt( void *object, const char *index, const char *data );
      static bool setBaseFontFamilyProt( void *object, const char *index, const char *data );
      static bool setBaseFontSizeProt( void *object, const char *index, const char *data );
      static bool setBaseLetterSpacingProt( void *object, const char *index, const char *data );
      static bool setBaseWordSpacingProt( void *object, const char *index, const char *data );
      static bool setBaseTextAlignHProt( void *object, const char *index, const char *data );
      static bool setBaseTextAlignVProt( void *object, const char *index, const char *data );

      #define GUISTYLE_DECLARE_STATE_SETTERS( stateName ) \
         static bool set##stateName##BackgroundColorProt( void *object, const char *index, const char *data ); \
         static bool set##stateName##BorderColorProt( void *object, const char *index, const char *data ); \
         static bool set##stateName##BorderWidthProt( void *object, const char *index, const char *data ); \
         static bool set##stateName##TextColorProt( void *object, const char *index, const char *data ); \
         static bool set##stateName##FontFamilyProt( void *object, const char *index, const char *data ); \
         static bool set##stateName##FontSizeProt( void *object, const char *index, const char *data ); \
         static bool set##stateName##LetterSpacingProt( void *object, const char *index, const char *data ); \
         static bool set##stateName##WordSpacingProt( void *object, const char *index, const char *data );

      GUISTYLE_DECLARE_STATE_SETTERS( Hover )
      GUISTYLE_DECLARE_STATE_SETTERS( Active )
      GUISTYLE_DECLARE_STATE_SETTERS( Focus )
      GUISTYLE_DECLARE_STATE_SETTERS( Disabled )
      GUISTYLE_DECLARE_STATE_SETTERS( Checked )
      GUISTYLE_DECLARE_STATE_SETTERS( Error )

      #undef GUISTYLE_DECLARE_STATE_SETTERS

   public:

      DECLARE_CONOBJECT( GuiStyle );
      DECLARE_CATEGORY( "Gui Core" );
      DECLARE_DESCRIPTION( "A named, reusable CSS-like style: a base property set plus optional "
         "hover/active/focus/disabled/checked/error variant blocks." );

      GuiStyle();
      virtual ~GuiStyle() {}

      static void initPersistFields();

      bool onAdd() override;
      void onRemove() override;

      /// Called (indirectly, via GuiStyleable) whenever an awake control
      /// starts/stops referencing this style
      void incLoadCount();
      void decLoadCount();

      /// Direct, mutable access to the base block and each state block
      GuiStyleProperties &getBase() { return mBase; }
      GuiStyleProperties &getState( GuiStyleState state ) { return mStates[ (U32)state ]; }
      const GuiStyleProperties &getBase() const { return mBase; }
      const GuiStyleProperties &getState( GuiStyleState state ) const { return mStates[ (U32)state ]; }

      /// See mUseSDF's doc comment.
      bool getUseSDF() const { return mUseSDF; }

      /// Resolves this style's properties for a given interaction state
      GuiStyleProperties resolve(U32 activeStateMask) const
      {
         GuiStyleProperties result = mBase;

         // Priority order, LEAST to MOST
         static const GuiStyleState kResolvePriorityOrder[] =
         {
            GuiStyleState::Error,
            GuiStyleState::Checked,
            GuiStyleState::Disabled,
            GuiStyleState::Focus,
            GuiStyleState::Hover,
            GuiStyleState::Active,
         };

         for (U32 i = 0; i < sizeof(kResolvePriorityOrder) / sizeof(kResolvePriorityOrder[0]); i++)
         {
            const GuiStyleState state = kResolvePriorityOrder[i];
            if (activeStateMask & GuiStyle::bit(state))
               mStates[(U32)state].cascadeOnto(result);
         }

         return result;
      }

      /// Convenience bit for activeStateMask -- e.g. GuiStyle::bit(GuiStyleState::Hover).
      static U32 bit( GuiStyleState state ) { return 1u << (U32)state; }

      /// Resolves this style for the given interaction state (see
      /// resolve()) and returns the loaded font that resolution's
      /// fontFamily/fontSize should use.
      Resource<GFont> getResolvedFont( U32 activeStateMask ) const
      {
         return _getFontFor( resolve( activeStateMask ) );
      }

      /// Same as getResolvedFont(), except the resolved fontSize is
      /// overridden with explicitSizeOverride instead of using whatever
      /// this style's base/state blocks authored
      Resource<GFont> getResolvedFontAtSize( U32 activeStateMask, S32 explicitSizeOverride ) const
      {
         GuiStyleProperties merged = resolve( activeStateMask );
         merged.fontSize.set( explicitSizeOverride );
         return _getFontFor( merged );
      }
};

#endif // _GUISTYLE_H_
