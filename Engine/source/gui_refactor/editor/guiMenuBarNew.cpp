//-----------------------------------------------------------------------------
// guiMenuBarNew.cpp
// See guiMenuBarNew.h for the full design writeup.
//-----------------------------------------------------------------------------

#include "gui_refactor/editor/guiMenuBarNew.h"

#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT( GuiMenuBarNew );

//-----------------------------------------------------------------------------

GuiMenuBarNew::GuiMenuBarNew()
   : mHighlightIndex( -1 ),
     mOpenIndex( -1 ),
     mFieldPaddingH( 12 )
{
   mCapturesInput = false; // a plain content-area control, not an overlay/dialog -- see guiPopupMenuNew.h's contrasting doc comment
   mFocusable = true;
   mTabable = false;
   mIsContainer = false;

   // Menu bars are always a fixed, short strip -- default to a sensible
   // height; width is left to whatever layout fields the .gui/script
   // sets (typically stretched to the parent's full width).
   setExtent( 200, 24 );
}

//-----------------------------------------------------------------------------

GuiMenuBarNew::~GuiMenuBarNew()
{
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::initPersistFields()
{
   docsURL;
   addGroup( "Menu Bar" );

      addField( "fieldPadding", TypeS32, Offset( mFieldPaddingH, GuiMenuBarNew ),
         "Horizontal padding, in logical units, on either side of each top-level menu field's text." );

   endGroup( "Menu Bar" );

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------
// Content
//-----------------------------------------------------------------------------

GuiPopupMenuNew* GuiMenuBarNew::addMenu( const String &text, bool enabled )
{
   GuiMenuBarEntry entry;
   entry.mText = text;
   entry.mEnabled = enabled;

   // The dropdown is created and registered up front (not lazily on
   // first click) so script can populate it via addMenuItem()/
   // addMenuSeparator() immediately after addMenu() returns, the same
   // turn -- see file header's usage example.
   GuiPopupMenuNew *dropdown = new GuiPopupMenuNew();
   dropdown->registerObject();
   entry.mDropdown = dropdown;

   mMenus.push_back( entry );
   _layoutMenus();

   return dropdown;
}

//-----------------------------------------------------------------------------

S32 GuiMenuBarNew::addMenuItem( S32 menuIndex, const String &text, const String &consoleCommand, bool enabled )
{
   GuiPopupMenuNew *menu = getMenu( menuIndex );
   if ( !menu )
      return -1;

   return menu->addItem( text, consoleCommand, enabled );
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::addMenuSeparator( S32 menuIndex )
{
   GuiPopupMenuNew *menu = getMenu( menuIndex );
   if ( menu )
      menu->addSeparator();
}

//-----------------------------------------------------------------------------

GuiPopupMenuNew* GuiMenuBarNew::getMenu( S32 menuIndex ) const
{
   if ( menuIndex < 0 || (U32)menuIndex >= mMenus.size() )
      return NULL;

   return mMenus[menuIndex].mDropdown;
}

//-----------------------------------------------------------------------------

S32 GuiMenuBarNew::findMenu( const String &text ) const
{
   for ( U32 i = 0; i < mMenus.size(); i++ )
      if ( mMenus[i].mText.equal( text, String::NoCase ) )
         return (S32)i;

   return -1;
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::clearMenus()
{
   if ( mOpenIndex >= 0 )
      _openOrCloseMenu( mOpenIndex ); // closes it, since it's already open

   for ( U32 i = 0; i < mMenus.size(); i++ )
   {
      if ( mMenus[i].mDropdown )
         mMenus[i].mDropdown->deleteObject();
   }

   mMenus.clear();
   mHighlightIndex = -1;
   mOpenIndex = -1;
}

//-----------------------------------------------------------------------------
// Layout
//-----------------------------------------------------------------------------

void GuiMenuBarNew::_layoutMenus()
{
   Resource<GFont> fontRes = mStyle ? mStyle->getResolvedFont( 0 ) : Resource<GFont>();
   GFont *font = fontRes;

   S32 x = 0;
   for ( U32 i = 0; i < mMenus.size(); i++ )
   {
      S32 textWidth = font ? font->getStrWidthPrecise( (const UTF8*)mMenus[i].mText.c_str() ) : mMenus[i].mText.length() * 8;
      S32 fieldWidth = textWidth + mFieldPaddingH * 2;

      mMenus[i].mFieldRect.set( x, 0, fieldWidth, getExtent().y );
      x += fieldWidth;
   }
}

//-----------------------------------------------------------------------------

bool GuiMenuBarNew::resize( const Point2I &newPosition, const Point2I &newExtent )
{
   bool ret = Parent::resize( newPosition, newExtent );
   _layoutMenus(); // field rects' height tracks the bar's own height -- see _layoutMenus()'s use of getExtent().y
   return ret;
}

//-----------------------------------------------------------------------------

S32 GuiMenuBarNew::_findMenuAt( const Point2I &localPoint ) const
{
   for ( U32 i = 0; i < mMenus.size(); i++ )
      if ( mMenus[i].mFieldRect.pointInRect( localPoint ) )
         return (S32)i;

   return -1;
}

//-----------------------------------------------------------------------------

bool GuiMenuBarNew::_isDropdownOpen( S32 index ) const
{
   if ( index < 0 || (U32)index >= mMenus.size() || !mMenus[index].mDropdown )
      return false;

   // A GuiPopupMenuNew deletes itself on close (see guiPopupMenuNew.h's
   // closeMenu()) -- for a MENU BAR's top-level dropdown specifically,
   // that would destroy the very GuiPopupMenuNew script populated via
   // addMenu()'s returned pointer, which is a real problem here (unlike
   // a submenu, which is genuinely transient). To keep the bar's own
   // top-level dropdown alive/reusable across opens, GuiMenuBarNew checks
   // isProperlyAdded() as its "is this dropdown currently the open one"
   // signal instead of trusting mOpenIndex alone -- see
   // _openOrCloseMenu()'s doc comment for how this interacts with
   // showAt()/closeMenu().
   return mMenus[index].mDropdown->isProperlyAdded();
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::_openOrCloseMenu( S32 index )
{
   if ( index < 0 || (U32)index >= mMenus.size() || !mMenus[index].mEnabled )
      return;

   GuiCanvasNew *canvas = getRoot();
   if ( !canvas )
      return;

   // Clicking the field whose dropdown is ALREADY open just closes it --
   // standard menu-bar toggle behavior.
   if ( mOpenIndex == index && _isDropdownOpen( index ) )
   {
      mMenus[index].mDropdown->closeMenu( false );
      mOpenIndex = -1;
      setUpdate();
      return;
   }

   // A different dropdown is open -- close it first (at most one open at
   // a time, matching every OS menu bar).
   if ( mOpenIndex >= 0 && _isDropdownOpen( mOpenIndex ) )
      mMenus[mOpenIndex].mDropdown->closeMenu( false );

   mOpenIndex = index;

   RectI fieldGlobalRect( localToGlobalCoord( mMenus[index].mFieldRect.point ), mMenus[index].mFieldRect.extent );
   Point2I anchor( fieldGlobalRect.point.x, fieldGlobalRect.point.y + fieldGlobalRect.extent.y );

   mMenus[index].mDropdown->showAt( canvas, anchor );

   setUpdate();
}

//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------

void GuiMenuBarNew::onRender( Point2I offset, const RectI &updateRect )
{
   GuiCanvasNew *canvas = getRoot();
   if ( !canvas )
      return;
   GuiRenderBatch &batch = canvas->getRenderBatch();

   RectI ctrlRect( offset, getDeviceBounds().extent );

   const GuiStyleProperties style = resolveStyle();

   if ( style.backgroundColor.isSet() )
      batch.pushQuad( ctrlRect, style.backgroundColor.mValue );

   if ( style.borderWidth.isSet() && style.borderWidth.mValue > 0 && style.borderColor.isSet() )
   {
      // Bottom edge only -- a menu bar conventionally has just a
      // dividing line under it, not a full box outline (matching every
      // OS menu bar's own look); the flat-four-quad full border
      // GuiPopupMenuNew's own box uses would look wrong here.
      const S32 bw = style.borderWidth.mValue;
      batch.pushQuad( RectI( Point2I( ctrlRect.point.x, ctrlRect.point.y + ctrlRect.extent.y - bw ), Point2I( ctrlRect.extent.x, bw ) ), style.borderColor.mValue );
   }

   const F32 scaleX = getEffectiveScaleX();
   const F32 scaleY = getEffectiveScaleY();

   Resource<GFont> fontRes = mStyle ? mStyle->getResolvedFont( 0 ) : Resource<GFont>();
   GFont *font = fontRes;

   for ( U32 i = 0; i < mMenus.size(); i++ )
   {
      const GuiMenuBarEntry &entry = mMenus[i];
      RectI fieldDeviceRect(
         offset + Point2I( (S32)( entry.mFieldRect.point.x * scaleX ), (S32)( entry.mFieldRect.point.y * scaleY ) ),
         Point2I( (S32)( entry.mFieldRect.extent.x * scaleX ), (S32)( entry.mFieldRect.extent.y * scaleY ) ) );

      // Highlight the hovered field, and separately keep the OPEN
      // field visibly "pressed" even after the mouse has moved onto the
      // dropdown itself (where mHighlightIndex on the BAR no longer
      // tracks it) -- both cases share the same highlight color, matching
      // every OS menu bar's look of "the active menu's own field stays
      // lit while its dropdown is showing."
      if ( ( (S32)i == mHighlightIndex || (S32)i == mOpenIndex ) && entry.mEnabled )
      {
         ColorI hoverColor = style.backgroundColor.isSet()
            ? ColorI( style.backgroundColor.mValue.red + 24, style.backgroundColor.mValue.green + 24, style.backgroundColor.mValue.blue + 24 )
            : ColorI( 96, 96, 96 );
         batch.pushQuad( fieldDeviceRect, hoverColor );
      }

      if ( font )
      {
         ColorI textColor = entry.mEnabled
            ? ( style.textColor.isSet() ? style.textColor.mValue : ColorI( 255, 255, 255 ) )
            : ColorI( 128, 128, 128 );

         Point2I textPos( fieldDeviceRect.point.x + (S32)( mFieldPaddingH * scaleX ), fieldDeviceRect.point.y + ( fieldDeviceRect.extent.y - font->getHeight() ) / 2 );
         // Pass fontRes (the Resource<GFont> resolved above), not the raw
         // font pointer -- see guiRenderBatch.h's GuiBatchTextRun::font
         // doc comment: only a real Resource<GFont> held by the batch
         // keeps the font alive until the deferred flush() that actually
         // reads it; fontRes as a local variable would otherwise be
         // destroyed once this function returns.
         batch.pushText( fontRes, textPos, entry.mText.c_str(), textColor );
      }
   }
}

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------

bool GuiMenuBarNew::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mHighlightIndex = -1;
   _layoutMenus();

   return true;
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::onSleep()
{
   // Close any open dropdown rather than leaving it orphaned on the
   // canvas's dialog stack once this bar itself goes to sleep (e.g. the
   // canvas's menu bar is being swapped out from under it -- see
   // GuiCanvasNew::setMenuBar()).
   if ( mOpenIndex >= 0 && _isDropdownOpen( mOpenIndex ) )
      mMenus[mOpenIndex].mDropdown->closeMenu( false );

   mOpenIndex = -1;
   mHighlightIndex = -1;

   Parent::onSleep();
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::onMouseMove( const GuiEvent &event )
{
   Point2I localPt = globalToLocalCoord( event.mousePoint );
   S32 newHighlight = _findMenuAt( localPt );

   if ( newHighlight != mHighlightIndex )
   {
      mHighlightIndex = newHighlight;
      setUpdate();

      // If a different top-level dropdown is already open, hovering
      // across the bar to another field swaps which one shows --
      // standard "mouse-over while menu bar is active" behavior, same
      // as clicking would do, just without requiring a fresh click.
      if ( mOpenIndex >= 0 && newHighlight >= 0 && newHighlight != mOpenIndex && mMenus[newHighlight].mEnabled )
         _openOrCloseMenu( newHighlight );
   }
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::onMouseLeave( const GuiEvent &event )
{
   mHighlightIndex = -1;
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiMenuBarNew::onMouseDown( const GuiEvent &event )
{
   Point2I localPt = globalToLocalCoord( event.mousePoint );
   S32 idx = _findMenuAt( localPt );

   if ( idx >= 0 )
      _openOrCloseMenu( idx );
}
