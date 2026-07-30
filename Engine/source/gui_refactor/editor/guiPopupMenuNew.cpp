//-----------------------------------------------------------------------------
// guiPopupMenuNew.cpp
// See guiPopupMenuNew.h for the full design writeup (click-away dismissal
// mechanics in particular).
//-----------------------------------------------------------------------------

#include "gui_refactor/editor/guiPopupMenuNew.h"

#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT( GuiPopupMenuNew );

//-----------------------------------------------------------------------------

GuiPopupMenuNew::GuiPopupMenuNew()
   : mHighlightIndex( -1 ),
     mItemHeight( 22 ),
     mSeparatorHeight( 7 ),
     mSubMenuArrowInset( 16 ),
     mOwningCanvas( NULL ),
     mVisibleRect( 0, 0, 0, 0 ),
     mClosing( false ),
     mParentMenu( NULL ),
     mOpenSubMenu( NULL )
{
   // A popup menu is never authored/edited directly and never wants tab
   // focus cycling through its own rows the normal way (Escape/arrow
   // keys are handled explicitly in onKeyDown() instead) -- see
   // mCapturesInput's doc comment on GuiControlNew for what this actually
   // controls.
   mCapturesInput = true;
   mFocusable = true;
   mTabable = false;
   mIsContainer = false;

   // Starts at zero size/position -- showAt()/showAsSubMenu() resize
   // this to the full canvas and compute mVisibleRect before the menu
   // is ever pushed onto the dialog stack, so nothing here needs a real
   // value yet.
   setExtent( 0, 0 );
}

//-----------------------------------------------------------------------------

GuiPopupMenuNew::~GuiPopupMenuNew()
{
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::initPersistFields()
{
   docsURL;
   addGroup( "Popup Menu" );

      addField( "itemHeight",       TypeS32, Offset( mItemHeight,       GuiPopupMenuNew ),
         "Height, in logical units, of one clickable item row." );
      addField( "separatorHeight",  TypeS32, Offset( mSeparatorHeight,  GuiPopupMenuNew ),
         "Height, in logical units, of a separator row." );
      addField( "subMenuArrowInset",TypeS32, Offset( mSubMenuArrowInset,GuiPopupMenuNew ),
         "Right-edge inset reserved for the submenu arrow glyph on rows with a submenu." );

   endGroup( "Popup Menu" );

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------
// Content
//-----------------------------------------------------------------------------

S32 GuiPopupMenuNew::addItem( const String &text, const String &consoleCommand, bool enabled )
{
   GuiPopupMenuItem item;
   item.mText = text;
   item.mConsoleCommand = consoleCommand;
   item.mEnabled = enabled;

   mItems.push_back( item );
   return mItems.size() - 1;
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::addSeparator()
{
   GuiPopupMenuItem item;
   item.mIsSeparator = true;
   item.mEnabled = false;

   mItems.push_back( item );
}

//-----------------------------------------------------------------------------

S32 GuiPopupMenuNew::addSubMenuItem( const String &text, GuiPopupMenuNew *childMenu, bool enabled )
{
   GuiPopupMenuItem item;
   item.mText = text;
   item.mEnabled = enabled;
   item.mSubMenu = childMenu;

   mItems.push_back( item );
   return mItems.size() - 1;
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::clearItems()
{
   mItems.clear();
   mHighlightIndex = -1;
}

//-----------------------------------------------------------------------------
// Layout helpers
//-----------------------------------------------------------------------------

void GuiPopupMenuNew::autoSizeToContent()
{
   // Width: widest item's text, plus left/right padding and room for the
   // submenu arrow column if anything in this menu has a submenu.
   S32 maxTextWidth = 0;
   bool anySubMenus = false;

   Resource<GFont> fontRes = mStyle ? mStyle->getResolvedFont( 0 ) : Resource<GFont>();
   GFont *font = fontRes;

   for ( U32 i = 0; i < mItems.size(); i++ )
   {
      if ( mItems[i].mIsSeparator )
         continue;

      if ( mItems[i].mSubMenu )
         anySubMenus = true;

      S32 w = font ? font->getStrWidthPrecise( (const UTF8*)mItems[i].mText.c_str() ) : mItems[i].mText.length() * 8;
      if ( w > maxTextWidth )
         maxTextWidth = w;
   }

   const S32 kHorizPadding = 24; // left inset + right inset before the arrow column
   S32 width = maxTextWidth + kHorizPadding + ( anySubMenus ? mSubMenuArrowInset : 0 );

   if ( width < 80 )
      width = 80; // sane minimum so a menu with only very short labels doesn't look clipped

   // Height: sum of every row.
   S32 height = 0;
   for ( U32 i = 0; i < mItems.size(); i++ )
      height += mItems[i].mIsSeparator ? mSeparatorHeight : mItemHeight;

   mVisibleRect.extent.set( width, height );
}

//-----------------------------------------------------------------------------

RectI GuiPopupMenuNew::getItemRect( S32 index ) const
{
   S32 y = 0;
   for ( S32 i = 0; i < index; i++ )
      y += mItems[i].mIsSeparator ? mSeparatorHeight : mItemHeight;

   S32 h = mItems[index].mIsSeparator ? mSeparatorHeight : mItemHeight;

   return RectI( 0, y, mVisibleRect.extent.x, h );
}

//-----------------------------------------------------------------------------

S32 GuiPopupMenuNew::getItemAt( const Point2I &visibleRectLocalPoint ) const
{
   if ( visibleRectLocalPoint.x < 0 || visibleRectLocalPoint.x >= mVisibleRect.extent.x )
      return -1;
   if ( visibleRectLocalPoint.y < 0 || visibleRectLocalPoint.y >= mVisibleRect.extent.y )
      return -1;

   S32 y = 0;
   for ( U32 i = 0; i < mItems.size(); i++ )
   {
      S32 h = mItems[i].mIsSeparator ? mSeparatorHeight : mItemHeight;
      if ( visibleRectLocalPoint.y >= y && visibleRectLocalPoint.y < y + h )
         return mItems[i].mIsSeparator ? -1 : (S32)i;
      y += h;
   }

   return -1;
}

//-----------------------------------------------------------------------------
// Showing / Closing
//-----------------------------------------------------------------------------

void GuiPopupMenuNew::showAt( GuiCanvasNew *canvas, const Point2I &screenPos )
{
   if ( !canvas )
      return;

   mOwningCanvas = canvas;
   mParentMenu = NULL;

   autoSizeToContent();

   // Resize THIS control to cover the whole canvas -- see file header's
   // click-away dismissal note. mVisibleRect.extent was just computed by
   // autoSizeToContent() above; mVisibleRect.point is set below once we
   // know the canvas's extent to clamp against.
   const Point2I canvasExtent = canvas->getExtent();
   resize( Point2I( 0, 0 ), canvasExtent );

   // Clamp so the visible box stays fully on-screen -- flip above/left
   // of the anchor point rather than letting it run off the far edge.
   Point2I pos = screenPos;
   if ( pos.x + mVisibleRect.extent.x > canvasExtent.x )
      pos.x = canvasExtent.x - mVisibleRect.extent.x;
   if ( pos.y + mVisibleRect.extent.y > canvasExtent.y )
      pos.y = screenPos.y - mVisibleRect.extent.y; // flip to above the anchor
   if ( pos.x < 0 )
      pos.x = 0;
   if ( pos.y < 0 )
      pos.y = 0;

   mVisibleRect.point = pos;

   canvas->pushDialogControl( this, 100, false );
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::showAsSubMenu( GuiPopupMenuNew *parentMenu, const RectI &parentItemGlobalRect )
{
   if ( !parentMenu )
      return;

   GuiCanvasNew *canvas = parentMenu->mOwningCanvas ? parentMenu->mOwningCanvas : parentMenu->getRoot();
   if ( !canvas )
      return;

   mOwningCanvas = canvas;
   mParentMenu = parentMenu;

   autoSizeToContent();

   const Point2I canvasExtent = canvas->getExtent();
   resize( Point2I( 0, 0 ), canvasExtent );

   // Anchor to the owning item's top-right corner; flip to the LEFT
   // side of the item instead if there isn't room on the right, and
   // flip up if there isn't room below -- same edge-avoidance idea as
   // showAt(), just anchored to a rect instead of a raw point.
   Point2I pos( parentItemGlobalRect.point.x + parentItemGlobalRect.extent.x, parentItemGlobalRect.point.y );

   if ( pos.x + mVisibleRect.extent.x > canvasExtent.x )
      pos.x = parentItemGlobalRect.point.x - mVisibleRect.extent.x;
   if ( pos.y + mVisibleRect.extent.y > canvasExtent.y )
      pos.y = canvasExtent.y - mVisibleRect.extent.y;
   if ( pos.x < 0 )
      pos.x = 0;
   if ( pos.y < 0 )
      pos.y = 0;

   mVisibleRect.point = pos;

   canvas->pushDialogControl( this, 100, false );
   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::closeMenu( bool closeWholeChain )
{
   if ( mClosing )
      return;
   mClosing = true;

   // Close any open submenu first (leaves are closed before their
   // parents so a submenu never outlives the menu that spawned it).
   if ( mOpenSubMenu )
   {
      GuiPopupMenuNew *sub = mOpenSubMenu;
      mOpenSubMenu = NULL;
      sub->closeMenu( false ); // false: we'll handle walking further up ourselves below
   }

   GuiPopupMenuNew *parent = mParentMenu;

   if ( mOwningCanvas )
      mOwningCanvas->popDialogControl( this );

   deleteObject();

   if ( closeWholeChain && parent )
      parent->closeMenu( true );
}

//-----------------------------------------------------------------------------
// Submenu hover management
//-----------------------------------------------------------------------------

void GuiPopupMenuNew::updateOpenSubMenu( S32 newHighlightIndex )
{
   const bool newHasSubMenu = ( newHighlightIndex >= 0 )
      && ( (U32)newHighlightIndex < mItems.size() )
      && ( mItems[newHighlightIndex].mSubMenu != NULL );

   if ( mOpenSubMenu )
   {
      // Already open, and hover is still on the same item -- nothing to do.
      bool stillSameItem = false;
      if ( newHasSubMenu && (U32)newHighlightIndex < mItems.size() )
         stillSameItem = ( mItems[newHighlightIndex].mSubMenu == mOpenSubMenu );

      if ( !stillSameItem )
      {
         GuiPopupMenuNew *old = mOpenSubMenu;
         mOpenSubMenu = NULL;
         old->closeMenu( false );
      }
   }

   if ( newHasSubMenu && !mOpenSubMenu )
   {
      mOpenSubMenu = mItems[newHighlightIndex].mSubMenu;

      RectI rowRect = getItemRect( newHighlightIndex );
      RectI globalRowRect( mVisibleRect.point + rowRect.point, rowRect.extent );

      mOpenSubMenu->showAsSubMenu( this, globalRowRect );
   }
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::selectItem( S32 index )
{
   if ( index < 0 || (U32)index >= mItems.size() )
      return;

   const GuiPopupMenuItem &item = mItems[index];
   if ( item.mIsSeparator || !item.mEnabled || item.mSubMenu )
      return;

   if ( item.mConsoleCommand.isNotEmpty() )
      evaluate( item.mConsoleCommand );

   closeMenu( true );
}

//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------

void GuiPopupMenuNew::onRender( Point2I offset, const RectI &updateRect )
{
   // offset is this control's DEVICE top-left (which is the whole
   // canvas, per showAt()) -- the actual visible box needs mVisibleRect
   // projected the same logical->device way the rest of the control
   // stack does. Since mVisibleRect is authored directly (not through
   // the mWidth/mLeft layout fields), project it manually here using
   // the same scale getDeviceBounds() already computed for our own
   // full-canvas mBounds.
   const RectI &deviceBounds = getDeviceBounds();
   const F32 scaleX = getExtent().x > 0 ? (F32)deviceBounds.extent.x / (F32)getExtent().x : 1.0f;
   const F32 scaleY = getExtent().y > 0 ? (F32)deviceBounds.extent.y / (F32)getExtent().y : 1.0f;

   Point2I boxDevicePos = offset + Point2I( (S32)( mVisibleRect.point.x * scaleX ), (S32)( mVisibleRect.point.y * scaleY ) );
   Point2I boxDeviceExtent( (S32)( mVisibleRect.extent.x * scaleX ), (S32)( mVisibleRect.extent.y * scaleY ) );

   RectI boxRect( boxDevicePos, boxDeviceExtent );

   // Every draw below goes through the canvas's per-frame batch (see
   // guiRenderBatch.h) instead of GFXDrawUtil -- this control no longer
   // issues any immediate draw calls of its own; it just queues
   // primitives that GuiCanvasNew::renderFrame() flushes once, for the
   // whole tree, after every control's onRender() has run.
   GuiCanvasNew *canvas = getRoot();
   if ( !canvas )
      return;
   GuiRenderBatch &batch = canvas->getRenderBatch();

   const GuiStyleProperties style = resolveStyle();

   if ( style.backgroundColor.isSet() )
      batch.pushQuad( boxRect, style.backgroundColor.mValue );

   if ( style.borderWidth.isSet() && style.borderWidth.mValue > 0 && style.borderColor.isSet() )
   {
      // Flat border drawn as four thin quads rather than an outline
      // primitive -- keeps it in the same solid-quad batch/draw call as
      // everything else this control submits, matching
      // GuiControlNew::onRender()'s own note that only a simple flat border
      // is supported for now (bevel/9-slice border styles are a later,
      // skin-system-dependent piece -- see gui-migration-plan.md Stage D).
      const S32 bw = style.borderWidth.mValue;
      const ColorI &bc = style.borderColor.mValue;
      batch.pushQuad( RectI( boxRect.point, Point2I( boxRect.extent.x, bw ) ), bc ); // top
      batch.pushQuad( RectI( Point2I( boxRect.point.x, boxRect.point.y + boxRect.extent.y - bw ), Point2I( boxRect.extent.x, bw ) ), bc ); // bottom
      batch.pushQuad( RectI( boxRect.point, Point2I( bw, boxRect.extent.y ) ), bc ); // left
      batch.pushQuad( RectI( Point2I( boxRect.point.x + boxRect.extent.x - bw, boxRect.point.y ), Point2I( bw, boxRect.extent.y ) ), bc ); // right
   }

   Resource<GFont> fontRes = mStyle ? mStyle->getResolvedFont( 0 ) : Resource<GFont>();
   GFont *font = fontRes;

   for ( U32 i = 0; i < mItems.size(); i++ )
   {
      const GuiPopupMenuItem &item = mItems[i];
      RectI rowRect = getItemRect( i );
      RectI rowDeviceRect(
         boxDevicePos + Point2I( (S32)( rowRect.point.x * scaleX ), (S32)( rowRect.point.y * scaleY ) ),
         Point2I( (S32)( rowRect.extent.x * scaleX ), (S32)( rowRect.extent.y * scaleY ) ) );

      if ( item.mIsSeparator )
      {
         // Thin centered divider line, one device pixel tall.
         Point2I lineStart( rowDeviceRect.point.x + 4, rowDeviceRect.point.y + rowDeviceRect.extent.y / 2 );
         Point2I lineEnd( rowDeviceRect.point.x + rowDeviceRect.extent.x - 4, lineStart.y );
         batch.pushLine( lineStart, lineEnd, style.borderColor.isSet() ? style.borderColor.mValue : ColorI( 128, 128, 128 ) );
         continue;
      }

      // Row hover highlight -- drawn as a simple fill; a real per-state
      // GuiStyle hover color would need a resolveStyle() call scoped to
      // just that row rather than this control's own state, which
      // GuiStyle doesn't support per-child today (out of scope for a
      // first pass -- see gui-migration-plan.md's "don't build
      // speculative primitives" guidance). A flat highlight color is
      // used instead, deliberately distinct from backgroundColor.
      if ( (S32)i == mHighlightIndex && item.mEnabled )
      {
         ColorI hoverColor = style.backgroundColor.isSet()
            ? ColorI( style.backgroundColor.mValue.red + 24, style.backgroundColor.mValue.green + 24, style.backgroundColor.mValue.blue + 24 )
            : ColorI( 96, 96, 96 );
         batch.pushQuad( rowDeviceRect, hoverColor );
      }

      if ( font )
      {
         ColorI textColor = item.mEnabled
            ? ( style.textColor.isSet() ? style.textColor.mValue : ColorI( 255, 255, 255 ) )
            : ColorI( 128, 128, 128 );

         Point2I textPos( rowDeviceRect.point.x + (S32)( 8 * scaleX ), rowDeviceRect.point.y + ( rowDeviceRect.extent.y - font->getHeight() ) / 2 );
         // Pass fontRes (the Resource<GFont> resolved above), not the raw
         // font pointer -- see guiRenderBatch.h's GuiBatchTextRun::font
         // doc comment: only a real Resource<GFont> held by the batch
         // keeps the font alive until the deferred flush() that actually
         // reads it; fontRes as a local variable would otherwise be
         // destroyed once this function returns.
         batch.pushText( fontRes, textPos, item.mText.c_str(), textColor );

         if ( item.mSubMenu )
         {
            // Simple ">" glyph in the reserved arrow column -- avoids
            // needing a dedicated arrow bitmap/atlas frame for a first
            // pass; a skinned icon can replace this once GuiBitmap (see
            // gui-migration-plan.md Stage D) has a real consumer.
            Point2I arrowPos( rowDeviceRect.point.x + rowDeviceRect.extent.x - (S32)( 14 * scaleX ), textPos.y );
            batch.pushText( fontRes, arrowPos, ">", textColor );
         }
      }
   }
}

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------

bool GuiPopupMenuNew::onWake()
{
   if ( !Parent::onWake() )
      return false;

   mHighlightIndex = -1;
   mClosing = false;

   return true;
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::onMouseMove( const GuiEvent &event )
{
   Point2I localPt = event.mousePoint - mVisibleRect.point;
   S32 newHighlight = getItemAt( localPt );

   if ( newHighlight != mHighlightIndex )
   {
      mHighlightIndex = newHighlight;
      updateOpenSubMenu( mHighlightIndex );
      setUpdate();
   }
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::onMouseLeave( const GuiEvent &event )
{
   // Deliberately do NOT clear mHighlightIndex/close mOpenSubMenu here --
   // this control covers the full canvas (see file header), so
   // "onMouseLeave" only fires when the mouse leaves the canvas
   // altogether, not when it moves from the visible box into the
   // surrounding capture area (that's still tracked frame-to-frame via
   // onMouseMove()'s own bounds check against mVisibleRect instead).
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::onMouseDown( const GuiEvent &event )
{
   // See file header's "click-away dismissal mechanics." A click inside
   // mVisibleRect is a genuine row press; anything else, anywhere in
   // this full-canvas capture area, dismisses the chain.
   Point2I localPt = event.mousePoint - mVisibleRect.point;

   if ( !mVisibleRect.pointInRect( event.mousePoint ) )
   {
      // Outside our own visible box. If we're a submenu, a click that's
      // still inside an ANCESTOR's visible box shouldn't close the
      // whole chain -- since only the topmost capturing control ever
      // receives rootMouseDown's event (see file header), we walk our
      // own ancestor chain explicitly here rather than relying on the
      // ancestor to somehow get the event itself.
      GuiPopupMenuNew *ancestor = mParentMenu;
      while ( ancestor )
      {
         if ( ancestor->mVisibleRect.pointInRect( event.mousePoint ) )
         {
            // Click landed on an ancestor menu's own visible box -- let
            // IT resolve the click (close everything below it, and
            // treat this as a normal row press on that ancestor).
            closeMenu( false );
            ancestor->onMouseDown( event );
            return;
         }
         ancestor = ancestor->mParentMenu;
      }

      closeMenu( true );
      return;
   }

   // Click is inside our own visible box -- selection itself happens on
   // mouse-UP (matching normal button/menu feel: press, drag to browse,
   // release over the item you want), so nothing further to do here
   // beyond making sure hover/submenu state matches where the press
   // landed.
   S32 idx = getItemAt( localPt );
   if ( idx != mHighlightIndex )
   {
      mHighlightIndex = idx;
      updateOpenSubMenu( mHighlightIndex );
      setUpdate();
   }
}

//-----------------------------------------------------------------------------

void GuiPopupMenuNew::onMouseUp( const GuiEvent &event )
{
   Point2I localPt = event.mousePoint - mVisibleRect.point;
   S32 idx = getItemAt( localPt );

   if ( idx >= 0 )
      selectItem( idx );
}

//-----------------------------------------------------------------------------

bool GuiPopupMenuNew::onKeyDown( const GuiEvent &event )
{
   if ( event.keyCode == KEY_ESCAPE )
   {
      closeMenu( true );
      return true;
   }

   if ( event.keyCode == KEY_RETURN || event.keyCode == KEY_NUMPADENTER )
   {
      if ( mHighlightIndex >= 0 )
         selectItem( mHighlightIndex );
      return true;
   }

   if ( event.keyCode == KEY_UP || event.keyCode == KEY_DOWN )
   {
      const S32 dir = ( event.keyCode == KEY_DOWN ) ? 1 : -1;
      S32 idx = mHighlightIndex;
      for ( U32 attempts = 0; attempts < mItems.size(); attempts++ )
      {
         idx += dir;
         if ( idx < 0 ) idx = mItems.size() - 1;
         if ( (U32)idx >= mItems.size() ) idx = 0;

         if ( !mItems[idx].mIsSeparator )
         {
            mHighlightIndex = idx;
            updateOpenSubMenu( mHighlightIndex );
            setUpdate();
            break;
         }
      }
      return true;
   }

   return Parent::onKeyDown( event );
}
