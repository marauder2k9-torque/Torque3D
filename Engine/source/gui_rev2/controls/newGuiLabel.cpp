//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiLabel.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiLabel.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gui_rev2/core/newLangTable.h"

IMPLEMENT_CONOBJECT(NewGuiLabel);

NewGuiLabel::NewGuiLabel()
   : mCachedFontFamily(NULL),
   mCachedFontSize(0.0f),
   mLetterSpacing(0),
   mWordSpacing(0),
   mTextKey(NULL)
{
}

NewGuiLabel::~NewGuiLabel()
{
}

void NewGuiLabel::setText(const char* text)
{
   mText.setText(text ? text : "");

   // Text content can change this control's own preferred size.
   setContentDirty();
   setArrangementDirty();
}

bool NewGuiLabel::_setText(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiLabel*>(obj)->setText(data);
   return false;
}

void NewGuiLabel::setTextKey(const char* key)
{
   mTextKey = (key && key[0]) ? StringTable->insert(key) : NULL;

   // Resolve immediately rather than waiting for the next StylePass, so
   // scripted setTextKey() calls (e.g. in response to a script-driven
   // language switch of their own) see the new text right away, same as
   // setText() already does above.
   if (mTextKey)
      setText(NewLangTable::translate(mTextKey));
}

bool NewGuiLabel::_setTextKey(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiLabel*>(obj)->setTextKey(data);
   return false;
}

void NewGuiLabel::StylePass(const NewGuiResolvedStyle& inherited, U32 inheritedGeneration)
{
   // Re-resolve BEFORE Parent::StylePass() so mText already holds the
   // current-language string by the time anything downstream of style
   // resolution (font, size, wrap) looks at it this same pass.
   if (mTextKey)
      mText.setText(NewLangTable::translate(mTextKey));

   Parent::StylePass(inherited, inheritedGeneration);
}

bool NewGuiLabel::_setAlignment(void* obj, const char* index, const char* data)
{
   NewGuiLabel* label = static_cast<NewGuiLabel*>(obj);

   NewGuiTextAlignHorizontal newAlignment = NewGuiTextAlignHorizontal::Left;
   if (dStricmp(data, "center") == 0)
      newAlignment = NewGuiTextAlignHorizontal::Center;
   else if (dStricmp(data, "right") == 0)
      newAlignment = NewGuiTextAlignHorizontal::Right;

   label->mText.setAlignHorizontal(newAlignment);
   // Only shifts draw position within already-resolved bounds - no dirty flag needed.
   return false;
}

bool NewGuiLabel::_setWrap(void* obj, const char* index, const char* data)
{
   NewGuiLabel* label = static_cast<NewGuiLabel*>(obj);
   label->mText.setWrap(dAtob(data));

   label->setContentDirty();
   label->setArrangementDirty();
   return false;
}

bool NewGuiLabel::_setOverflow(void* obj, const char* index, const char* data)
{
   NewGuiLabel* label = static_cast<NewGuiLabel*>(obj);

   NewGuiTextOverflow overflow = NewGuiTextOverflow_Overflow;
   if (dStricmp(data, "clip") == 0)
      overflow = NewGuiTextOverflow_Clip;
   else if (dStricmp(data, "ellipsis") == 0)
      overflow = NewGuiTextOverflow_Ellipsis;

   label->mText.setOverflow(overflow);
   // Doesn't change preferred size - no dirty flag needed.
   return false;
}

bool NewGuiLabel::_setLetterSpacing(void* obj, const char* index, const char* data)
{
   NewGuiLabel* label = static_cast<NewGuiLabel*>(obj);
   label->mLetterSpacing = dAtoi(data);
   label->mText.setSpacing(label->mLetterSpacing, label->mWordSpacing);

   label->setContentDirty();
   label->setArrangementDirty();
   return false;
}

bool NewGuiLabel::_setWordSpacing(void* obj, const char* index, const char* data)
{
   NewGuiLabel* label = static_cast<NewGuiLabel*>(obj);
   label->mWordSpacing = dAtoi(data);
   label->mText.setSpacing(label->mLetterSpacing, label->mWordSpacing);

   label->setContentDirty();
   label->setArrangementDirty();
   return false;
}

void NewGuiLabel::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Text");

   ADD_FIELD("text", TypeString, 0)
      .onSet(_setText)
      .doc("The text this label displays. Ignored (overwritten every StylePass) if textKey is also set.");

   ADD_FIELD("textKey", TypeString, 0)
      .onSet(_setTextKey)
      .doc("Optional translation key. When set, this label's text is re-resolved through the "
         "active NewLangTable (see NewLangTable::setLanguage()) every StylePass, "
         "so it automatically updates on a language switch. Leave unset to author text literally.");

   ADD_FIELD("alignment", TypeString, 0)
      .onSet(_setAlignment)
      .doc("Horizontal text alignment within this label's bounds: left (default), center, right.");

   ADD_FIELD("wrap", TypeBool, 0)
      .onSet(_setWrap)
      .doc("Enables word-wrap across multiple lines within this label's width. Off by default (single line).");

   ADD_FIELD("overflow", TypeString, 0)
      .onSet(_setOverflow)
      .doc("How text wider than this label's box is handled: overflow (default, never clips), clip, ellipsis.");

   ADD_FIELD("letterSpacing", TypeS32, 0)
      .onSet(_setLetterSpacing)
      .doc("Extra pixels of spacing applied after every character.");

   ADD_FIELD("wordSpacing", TypeS32, 0)
      .onSet(_setWordSpacing)
      .doc("Extra pixels of spacing applied after every space, in addition to letterSpacing.");

   GROUP_END("Text");
}

// Re-resolves only when the resolved style's fontFamily/fontSize have actually changed.
void NewGuiLabel::resolveFont()
{
   const NewGuiResolvedStyle& style = getResolvedStyle();

   if (mFont != NULL && style.fontFamily == mCachedFontFamily && style.fontSize == mCachedFontSize)
      return;

   const char* faceName = style.fontFamily ? style.fontFamily : "Arial";
   U32 size = (U32)(style.fontSize > 0.0f ? style.fontSize : 14.0f);

   mFont = GFont::create(faceName, size);
   mCachedFontFamily = style.fontFamily;
   mCachedFontSize = style.fontSize;

   mText.setFont(mFont);
}

// Measures unbounded (natural width) - matches Auto's "size to preferred content" contract; the
// real wrapped height (once width is known) is derived separately in ArrangePass().
Point2I NewGuiLabel::ComputePreferredSize()
{
   resolveFont();

   const NewGuiResolvedStyle& style = getResolvedStyle();

   mText.setBoxExtent(Point2I(0, 0));   // Unbounded - natural size.
   const NewGuiTextLayoutResult& result = mText.layout();

   S32 textWidth = 0;
   S32 textHeight = (mFont != NULL) ? (S32)mFont->getHeight() : 0;

   if (!result.lines.empty())
   {
      textWidth = result.blockBounds.extent.x;
      textHeight = getMax(textHeight, result.blockBounds.extent.y);
   }

   S32 width = textWidth + (S32)style.padding.horizontal();
   S32 height = textHeight + (S32)style.padding.vertical();

   return Point2I(width, height);
}

// Matches the legacy GuiLabelCtrlNew::getPreferredContentExtent() approach: measure at the
// already-resolved width. Parent::ArrangePass() resolves mBounds.extent.x first; once that
// returns, _reflowWrappedHeight() can measure a genuine wrapped height against it.
void NewGuiLabel::ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   Parent::ArrangePass(slotRect, parentRenderLayer, uiScaleX, uiScaleY);
   _reflowWrappedHeight();
}

// REQUIRED - a Stack-managed label is placed via this entry point, never ArrangePass().
void NewGuiLabel::ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   Parent::ArrangePassWithFixedExtent(finalBounds, parentRenderLayer, uiScaleX, uiScaleY);
   _reflowWrappedHeight();
}

// Shared by both entry points above. No-ops unless wrap is on - an unwrapped label's height
// never depends on width.
void NewGuiLabel::_reflowWrappedHeight()
{
   if (!mText.getWrap())
      return;

   const NewGuiResolvedStyle& style = getResolvedStyle();

   S32 innerWidth = getMax(0, mBounds.extent.x - (S32)style.padding.horizontal());

   // Large-but-finite height keeps NewGuiText's hasBox check true (wrap applies) while still
   // measuring the full wrapped extent rather than truncating partway through.
   static const S32 kUnboundedHeightForMeasurement = 0x7fffff;
   mText.setBoxExtent(Point2I(innerWidth, kUnboundedHeightForMeasurement));

   const NewGuiTextLayoutResult& result = mText.layout();

   S32 wrappedTextHeight = (mFont != NULL) ? (S32)mFont->getHeight() : 0;
   if (!result.lines.empty())
      wrappedTextHeight = getMax(wrappedTextHeight, result.blockBounds.extent.y);

   S32 wrappedHeight = wrappedTextHeight + (S32)style.padding.vertical();

   if (wrappedHeight != mBounds.extent.y)
   {
      mBounds.extent.y = wrappedHeight;

      // Correct mPreferredSize too, so a parent reading it (e.g. a Stack summing child heights)
      // gets the truthful value starting next dirty frame - MeasurePass() already finished
      // tree-wide before ArrangePass() started, so a parent sizing around this THIS frame still
      // sees the old value regardless.
      mPreferredSize.y = wrappedHeight;
   }
}

void NewGuiLabel::EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer)
{
   Parent::EmitDrawCommands(batch, bounds, style, layer);

   if (!batch || mFont == NULL || style.opacity <= 0.0f)
      return;

   const RectI clientRect(
      Point2I(bounds.point.x + (S32)style.padding.left, bounds.point.y + (S32)style.padding.top),
      Point2I(getMax(0, bounds.extent.x - (S32)style.padding.horizontal()),
         getMax(0, bounds.extent.y - (S32)style.padding.vertical())));

   // Final word on box extent regardless of what ComputePreferredSize()/ArrangePass() measured against.
   mText.setBoxExtent(clientRect.extent);

   ColorI textColor(
      style.textColor.red,
      style.textColor.green,
      style.textColor.blue,
      (U8)((F32)style.textColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));

   mText.submit(*batch, clientRect.point, textColor, layer);
}

//-----------------------------------------------------------------------------
// Script (console) API
//-----------------------------------------------------------------------------
// setText()/getText() exist as ordinary C++ methods above, and "text" is authorable as a field
// (ADD_FIELD("text", ...).onSet(_setText) in initPersistFields()) - but a field assignment
// (%obj.text = "..."; ) is a distinct binding from a callable method (%obj.setText("...");),
// and only the field side was ever exposed. Added here so script can update a label's text at
// runtime via a method call, not just at authoring time via the field.

DefineEngineMethod(NewGuiLabel, setText, void, (const char* text), ,
   "Sets the label's text content.\n"
   "@ingroup GuiCore")
{
   object->setText(text);
}

DefineEngineMethod(NewGuiLabel, getText, const char*, (), ,
   "@return The label's current text content.\n"
   "@ingroup GuiCore")
{
   return object->getText();
}
