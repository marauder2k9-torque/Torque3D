//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiLabel.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUILABEL_H_
#define _NEWGUILABEL_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#ifndef _NEWGUITEXT_H_
#include "gui_rev2/core/newGuiText.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif

/// A static text leaf control, built on NewGuiText for measurement/layout/drawing.
///
/// @code
/// new NewGuiLabel( MyLabel )
/// {
///    width = "200"; height = "auto";
///    text = "Hello, world!";
///    wrap = "true";
/// };
/// @endcode
class NewGuiLabel : public NewGuiControl
{
public:

   typedef NewGuiControl Parent;

protected:

   NewGuiText mText;   ///< Owns all text configuration/layout.

   /// Optional translation key. When set, StylePass() re-resolves this
   /// through the active NewLangTable and pushes the result into mText
   /// every time StylePass runs.
   StringTableEntry mTextKey;

   Resource<GFont> mFont;   ///< Cached resolved font, kept in step with the resolved style - see resolveFont().
   StringTableEntry mCachedFontFamily;
   F32 mCachedFontSize;

   /// Kept separately so letterSpacing/wordSpacing can be set independently from script,
   /// without either setter clobbering the other's last value passed to NewGuiText::setSpacing().
   S32 mLetterSpacing;
   S32 mWordSpacing;

   static bool _setText(void* obj, const char* index, const char* data);
   static bool _setTextKey(void* obj, const char* index, const char* data);
   static bool _setAlignment(void* obj, const char* index, const char* data);
   static bool _setWrap(void* obj, const char* index, const char* data);
   static bool _setOverflow(void* obj, const char* index, const char* data);
   static bool _setLetterSpacing(void* obj, const char* index, const char* data);
   static bool _setWordSpacing(void* obj, const char* index, const char* data);

   /// Ensures mFont matches the resolved style's fontFamily/fontSize, reloading only when changed.
   void resolveFont();

   /// Shared by ArrangePass() and ArrangePassWithFixedExtent(). Requires mBounds.extent.x to
   /// already be final. Re-measures wrapped height against it and corrects mBounds.extent.y/
   /// mPreferredSize.y in place if wrap="true" and the result differs from the unbounded guess
   /// ComputePreferredSize() made.
   void _reflowWrappedHeight();

public:

   NewGuiLabel();
   virtual ~NewGuiLabel();

   DECLARE_CONOBJECT(NewGuiLabel);

   static void initPersistFields();

   void setText(const char* text);
   const char* getText() const { return mText.getText().c_str(); }

   /// Sets/clears the translation key. Passing NULL or "" reverts to
   /// whatever was last set via setText() being treated as literal again.
   void setTextKey(const char* key);
   const char* getTextKey() const { return mTextKey; }

   /// Re-resolves mTextKey (if set) through the active NewLangTable before
   /// falling into Parent::StylePass() for everything else - see mTextKey's
   /// own doc comment for why this is the hook point.
   void StylePass(const NewGuiResolvedStyle& inherited, U32 inheritedGeneration) override;

   /// Always measures unbounded (natural, single-line size) - MeasurePass() runs before any
   /// control's width is resolved, so there's no box to wrap against yet. A wrap="true" label's
   /// real wrapped height is instead re-derived in ArrangePass(), once its own width is known.
   Point2I ComputePreferredSize() override;

   /// Resolves mBounds normally via Parent::, then corrects mBounds.extent.y for wrap="true" via _reflowWrappedHeight().
   void ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// REQUIRED override - a Stack-managed label is placed via this
   /// entry point, never ArrangePass(), so without this wrap="true"
   /// would silently stop working for any label inside a Stack.
   void ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// Draws background/border, then the text run via NewGuiText::submit().
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override;
};

#endif // _NEWGUILABEL_H_
