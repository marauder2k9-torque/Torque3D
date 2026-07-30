#include "gui_refactor/core/guiOffscreenCanvasNew.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTextureManager.h"
#include "gfx/gfxAPI.h"
#include "gfx/gfxDebugEvent.h"

#include "console/consoleTypes.h"
#include "console/console.h"

IMPLEMENT_CONOBJECT(GuiOffscreenCanvasNew);

GuiOffscreenCanvasNew* GuiOffscreenCanvasNew::sActiveOffscreenCanvas = NULL;
Vector<GuiOffscreenCanvasNew*> GuiOffscreenCanvasNew::sList;

GuiOffscreenCanvasNew::GuiOffscreenCanvasNew()
{
   mTargetFormat = GFXFormatR8G8B8A8;
   mTargetSize = Point2I(256,256);
   mTargetName = "offscreenCanvas";
   mTargetDirty = true;
   mDynamicTarget = false;
   mUseDepth = false;
}

GuiOffscreenCanvasNew::~GuiOffscreenCanvasNew()
{
}

void GuiOffscreenCanvasNew::initPersistFields()
{
   docsURL;
   addField( "targetSize", TypePoint2I, Offset( mTargetSize, GuiOffscreenCanvasNew ),"" );
   addField( "targetFormat", TypeGFXFormat, Offset( mTargetFormat, GuiOffscreenCanvasNew ), "");
   addField( "targetName", TypeRealString, Offset( mTargetName, GuiOffscreenCanvasNew ), "");
   addField( "dynamicTarget", TypeBool, Offset( mDynamicTarget, GuiOffscreenCanvasNew ), "");
   addField( "useDepth", TypeBool, Offset( mUseDepth, GuiOffscreenCanvasNew ), "");
   addField("canInteract", TypeBool, Offset(mCanInteract, GuiOffscreenCanvasNew), "");
   addField("maxInteractDistance", TypeF32, Offset(mMaxInteractDistance, GuiOffscreenCanvasNew), "");
   Parent::initPersistFields();
}

bool GuiOffscreenCanvasNew::onAdd()
{
   if (GuiControlNew::onAdd()) // jamesu - skip GuiCanvasNew onAdd since it sets up GFX which is bad
   {
      // ensure that we have a cursor
      setCursor(dynamic_cast<GuiCursor*>(Sim::findObject("DefaultCursor")));
      
      mRenderFront = true;
      sList.push_back(this);

      //Con::printf("Registering target %s...", mTargetName.c_str());
      mNamedTarget.registerWithName( mTargetName );

      _setupTargets();

      GFXTextureManager::addEventDelegate( this, &GuiOffscreenCanvasNew::_onTextureEvent );

      return true;
   }
   return false;
}

void GuiOffscreenCanvasNew::onRemove()
{
   GFXTextureManager::removeEventDelegate( this, &GuiOffscreenCanvasNew::_onTextureEvent );

   _teardownTargets();

   U32 idx = sList.find_next(this);
   if (idx != (U32)-1)
   {
      sList.erase(idx);
   }

   mTarget = NULL;
   mTargetTexture = NULL;
   mTargetDepth = NULL;

   Parent::onRemove();
}

void GuiOffscreenCanvasNew::_setupTargets()
{
   _teardownTargets();

   if (!mTarget.isValid())
   {
      mTarget = GFX->allocRenderToTextureTarget();
   }

   // Update color
   if (!mTargetTexture.isValid() || mTargetSize != mTargetTexture.getWidthHeight())
   {
      mTargetTexture.set( mTargetSize.x, mTargetSize.y, mTargetFormat, &GFXRenderTargetSRGBProfile, avar( "%s() - (line %d)", __FUNCTION__, __LINE__ ), 1, 0 );
   }

   // Update depth if needed
   if (mUseDepth && (!mTargetDepth.isValid() || mTargetSize != mTargetDepth.getWidthHeight()))
   {
      mTargetDepth.set( mTargetSize.x, mTargetSize.y, GFXFormatD24S8, &GFXRenderTargetProfile, avar( "%s() - (line %d)", __FUNCTION__, __LINE__ ), 1, 0 );
      mTarget->attachTexture( GFXTextureTarget::RenderSlot(GFXTextureTarget::DepthStencil), mTargetDepth );
   }

   mTarget->attachTexture( GFXTextureTarget::RenderSlot(GFXTextureTarget::Color0), mTargetTexture );
   mNamedTarget.setTexture(0, mTargetTexture);
}

void GuiOffscreenCanvasNew::_teardownTargets()
{
   mNamedTarget.release();
   mTargetTexture = NULL;
   mTargetDepth = NULL;
   mTargetDirty = true;
}

void GuiOffscreenCanvasNew::renderFrame(bool preRenderOnly, bool bufferSwap /* = true */)
{
   if (!mTargetDirty)
      return;
   
#ifdef TORQUE_ENABLE_GFXDEBUGEVENTS
   char buf[256];
   dSprintf(buf, sizeof(buf), "OffsceenCanvas %s", getName() ? getName() : getIdString());
   GFXDEBUGEVENT_SCOPE_EX(GuiOffscreenCanvas_renderFrame, ColorI::GREEN, buf);
#endif
   
   PROFILE_START(OffscreenCanvasPreRender);

#ifdef TORQUE_GFX_STATE_DEBUG
   GFX->getDebugStateManager()->startFrame();
#endif

   if (mTarget->getSize() != mTargetSize)
   {
      _setupTargets();
      mNamedTarget.setViewport( RectI( Point2I::Zero, mTargetSize ) );
   }

   // Make sure the root control is the size of the canvas.
   Point2I size = mTarget->getSize();

   if(size.x == 0 || size.y == 0)
   {
      PROFILE_END();
      return;
   }

   RectI screenRect(0, 0, size.x, size.y);

   maintainSizing();

   //preRender (recursive) all controls
   preRender();

   PROFILE_END();

   // Are we just doing pre-render?
   if(preRenderOnly)
   {
      return;
   }

   resetUpdateRegions();

   PROFILE_START(OffscreenCanvasRenderControls);

   GuiCursor *mouseCursor = NULL;
   bool cursorVisible = true;

   Point2I cursorPos((S32)mCursorPt.x, (S32)mCursorPt.y);
   mouseCursor = mDefaultCursor;

	mLastCursorEnabled = cursorVisible;
	mLastCursor = mouseCursor;
	mLastCursorPt = cursorPos;

   // Set active target
   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget(mTarget);

   // Clear the current viewport area
   GFX->setViewport( screenRect );
   GFX->clear( GFXClearTarget, LinearColorF(0,0,0,0), 1.0f, 0 );

   resetUpdateRegions();

	// Make sure we have a clean matrix state 
   // before we start rendering anything!   
   GFX->setWorldMatrix( MatrixF::Identity );
   GFX->setViewMatrix( MatrixF::Identity );
   GFX->setProjectionMatrix( MatrixF::Identity );
   
   RectI contentRect(Point2I(0,0), mTargetSize);
   {
      // Same batching wrap as GuiCanvasNew::renderFrame() -- see that
      // function's equivalent comment in guiCanvasNew.cpp. This canvas has
      // its own dialog-stack render loop (it doesn't call the base
      // implementation), so it needs its own begin()/flush() pair rather
      // than inheriting one automatically.
      mRenderBatch.begin(contentRect);

      // Render active GUI Dialogs
      for(iterator i = begin(); i != end(); i++)
      {
         // Get the control
         GuiControlNew *contentCtrl = static_cast<GuiControlNew*>(*i);
         
         GFX->setClipRect( contentRect );
         GFX->setStateBlock(mDefaultGuiSB);
         
         contentCtrl->onRender(contentCtrl->getPosition(), contentRect);
      }

      mRenderBatch.flush( GFX );

      // Fill Blue if no Dialogs
      if(this->size() == 0)
         GFX->clear( GFXClearTarget, LinearColorF(0,0,0,1), 1.0f, 0 );

      GFX->setClipRect( contentRect );

      // Draw cursor
      // 
      if (mCursorEnabled && mouseCursor && mShowCursor)
      {
         Point2I pos((S32)mCursorPt.x, (S32)mCursorPt.y);
         Point2I spot = mouseCursor->getHotSpot();

         pos -= spot;
         mouseCursor->render(pos);
      }

      GFX->getDrawUtil()->clearBitmapModulation();
   }

   mTarget->resolve();
   GFX->popActiveRenderTarget();

   PROFILE_END();

   // Keep track of the last time we rendered.
   mLastRenderMs = Platform::getRealMilliseconds();
   mTargetDirty = mDynamicTarget;

   onFrameRendered();
}

void GuiOffscreenCanvasNew::onFrameRendered()
{

}

Point2I GuiOffscreenCanvasNew::getWindowSize()
{
   return mTargetSize;
}

Point2I GuiOffscreenCanvasNew::getCursorPos()
{
   return Point2I(mCursorPt.x, mCursorPt.y);
}

void GuiOffscreenCanvasNew::setCursorPos(const Point2I &pt)
{
   mCursorPt.x = F32( pt.x );
   mCursorPt.y = F32( pt.y );
}

void GuiOffscreenCanvasNew::showCursor(bool state)
{ 
   mShowCursor = state;
}

bool GuiOffscreenCanvasNew::isCursorShown()
{
   return mShowCursor;
}

void GuiOffscreenCanvasNew::_onTextureEvent( GFXTexCallbackCode code )
{
   switch(code)
   {
      case GFXZombify:
         _teardownTargets();
         break;

      case GFXResurrect:
         _setupTargets();
         break;
   }
}

DefineEngineMethod(GuiOffscreenCanvasNew, resetTarget, void, (), , "")
{
   object->_setupTargets();
}

DefineEngineMethod(GuiOffscreenCanvasNew, markDirty, void, (), , "")
{
   object->markDirty();
}

