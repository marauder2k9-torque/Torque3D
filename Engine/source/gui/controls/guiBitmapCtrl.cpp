//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "gui/controls/guiBitmapCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"

#include "materials/matTextureTarget.h"


IMPLEMENT_CONOBJECT(GuiBitmapCtrl);

ConsoleDocClass( GuiBitmapCtrl,
   "@brief A gui control that is used to display an image.\n\n"
   
   "The image is stretched to the constraints of the control by default. However, the control can also\n"
   "tile the image as well.\n\n"

   "The image itself is stored inside the GuiBitmapCtrl::bitmap field. The boolean value that decides\n"
   "whether the image is stretched or tiled is stored inside the GuiBitmapCtrl::wrap field.\n"
   
   "@tsexample\n"
   "// Create a tiling GuiBitmapCtrl that displays \"myImage.png\"\n"
   "%bitmapCtrl = new GuiBitmapCtrl()\n"
   "{\n"
   "   bitmap = \"myImage.png\";\n"
   "   wrap = \"true\";\n"
   "};\n"
   "@endtsexample\n\n"
   
   "@ingroup GuiControls"
);

GuiBitmapCtrl::GuiBitmapCtrl(void)
 : mStartPoint( 0, 0 ),
   mColor(ColorI::WHITE),
   mAngle(0),
   mWrap( false )
{
   INIT_IMAGEASSET(Bitmap);
}

void GuiBitmapCtrl::setBitmap(const char* pAssetId)
{
   // Ignore no change.
   if (mBitmapAsset.getAssetId() == StringTable->insert(pAssetId))
      return;

   mBitmapAsset = pAssetId;

   mBitmapAsset->getTexture(mBitmapProfile);
}

void GuiBitmapCtrl::initPersistFields()
{
   docsURL;
   addGroup( "Bitmap" );

      INITPERSISTFIELD_IMAGEASSET(Bitmap, GuiBitmapCtrl, "The bitmap asset");

      addField("color", TypeColorI, Offset(mColor, GuiBitmapCtrl),"color mul");
      addField( "wrap",   TypeBool,     Offset( mWrap, GuiBitmapCtrl ),
         "If true, the bitmap is tiled inside the control rather than stretched to fit." );

      addField("angle", TypeF32, Offset(mAngle, GuiBitmapCtrl), "rotation");
      
   endGroup( "Bitmap" );

   Parent::initPersistFields();
}

bool GuiBitmapCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
   setActive(true);

   return true;
}

void GuiBitmapCtrl::onSleep()
{
   Parent::onSleep();
}

//-------------------------------------
void GuiBitmapCtrl::inspectPostApply()
{
   // if the extent is set to (0,0) in the gui editor and appy hit, this control will
   // set it's extent to be exactly the size of the bitmap (if present)
   Parent::inspectPostApply();

   if (!mWrap && (getExtent().x == 0) && (getExtent().y == 0) && mBitmapAsset.notNull())
   {
      setExtent(mBitmapAsset->getTextureBitmapWidth(), mBitmapAsset->getTextureBitmapHeight());
   }
}

void GuiBitmapCtrl::setBitmapResize( const char *name, bool resize )
{
   setBitmap(StringTable->insert(name));

   if (mBitmapAsset && resize)
   {
      setExtent(mBitmapAsset->getTextureBitmapWidth(), mBitmapAsset->getTextureBitmapHeight());
      updateSizing();
   }

   setUpdate();
}

void GuiBitmapCtrl::updateSizing()
{
   if(!getParent())
      return;
   // updates our bounds according to our horizSizing and verSizing rules
   RectI fakeBounds( getPosition(), getParent()->getExtent());
   parentResized( fakeBounds, fakeBounds);
}

void GuiBitmapCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   if (mBitmapAsset->isAssetValid())
   {
      GFX->getDrawUtil()->clearBitmapModulation();
      GFX->getDrawUtil()->setBitmapModulation(mColor);
		if(mWrap)
		{
         // We manually draw each repeat because non power of two textures will 
         // not tile correctly when rendered with GFX->drawBitmapTile(). The non POT
         // bitmap will be padded by the hardware, and we'll see lots of slack
         // in the texture. So... lets do what we must: draw each repeat by itself:
 			GFXTextureObject* texture = mBitmapAsset->getTexture(mBitmapProfile);
			RectI srcRegion;
			RectI dstRegion;
			F32 xdone = ((F32)getExtent().x/(F32)texture->mBitmapSize.x)+1;
			F32 ydone = ((F32)getExtent().y/(F32)texture->mBitmapSize.y)+1;

			S32 xshift = mStartPoint.x%texture->mBitmapSize.x;
			S32 yshift = mStartPoint.y%texture->mBitmapSize.y;
			for(S32 y = 0; y < ydone; ++y)
				for(S32 x = 0; x < xdone; ++x)
				{
		 			srcRegion.set(0,0,texture->mBitmapSize.x,texture->mBitmapSize.y);
  					dstRegion.set( ((texture->mBitmapSize.x*x)+offset.x)-xshift,
								      ((texture->mBitmapSize.y*y)+offset.y)-yshift,
								      texture->mBitmapSize.x,
								      texture->mBitmapSize.y);
               GFX->getDrawUtil()->drawBitmapStretchSR(texture, dstRegion, srcRegion, GFXBitmapFlip_None, GFXTextureFilterLinear, mAngle);
				}

		}
		else
      {
         RectI rect(offset, getExtent());
         GFX->getDrawUtil()->drawBitmapStretch(mBitmapAsset->getTexture(mBitmapProfile), rect, GFXBitmapFlip_None, GFXTextureFilterLinear, false, mAngle);
      }
   }

   if (mProfile->mBorder || !mBitmapAsset->isAssetValid())
   {
      RectI rect(offset.x, offset.y, getExtent().x, getExtent().y);
      GFX->getDrawUtil()->drawRect(rect, mProfile->mBorderColor);
   }

   renderChildControls(offset, updateRect);
}

void GuiBitmapCtrl::setValue(S32 x, S32 y)
{
   if (mBitmapAsset->isAssetValid())
   {
		x += mBitmapAsset->getTextureBitmapWidth() / 2;
		y += mBitmapAsset->getTextureBitmapHeight() / 2;
  	}
  	while (x < 0)
  		x += 256;
  	mStartPoint.x = x % 256;

  	while (y < 0)
  		y += 256;
  	mStartPoint.y = y % 256;
}

DefineEngineMethod( GuiBitmapCtrl, setValue, void, ( S32 x, S32 y ),,
   "Set the offset of the bitmap within the control.\n"
   "@param x The x-axis offset of the image.\n"
   "@param y The y-axis offset of the image.\n")
{
   object->setValue(x, y);
}

static ConsoleDocFragment _sGuiBitmapCtrlSetBitmap1(
   "@brief Assign an image to the control.\n\n"
   "Child controls with resize according to their layout settings.\n"
   "@param filename The filename of the image.\n"
   "@param resize Optional parameter. If true, the GUI will resize to fit the image.",
   "GuiBitmapCtrl", // The class to place the method in; use NULL for functions.
   "void setBitmap( String filename, bool resize );" ); // The definition string.

static ConsoleDocFragment _sGuiBitmapCtrlSetBitmap2(
   "@brief Assign an image to the control.\n\n"
   "Child controls will resize according to their layout settings.\n"
   "@param filename The filename of the image.\n"
   "@param resize A boolean value that decides whether the ctrl refreshes or not.",
   "GuiBitmapCtrl", // The class to place the method in; use NULL for functions.
   "void setBitmap( String filename );" ); // The definition string.


//"Set the bitmap displayed in the control. Note that it is limited in size, to 256x256."
DefineEngineMethod( GuiBitmapCtrl, setBitmap, void, ( const char * fileRoot, bool resize), ( false),
   "( String filename | String filename, bool resize ) Assign an image to the control.\n\n"
   "@hide" )
{
   char filename[1024];
   Con::expandScriptFilename(filename, sizeof(filename), fileRoot);
   object->setBitmapResize(filename, resize );
}

DefineEngineMethod(GuiBitmapCtrl, getBitmap, const char*, (),,
   "Gets the current bitmap set for this control.\n\n"
   "@hide")
{
   return object->mBitmapAsset->getAssetId();
}

