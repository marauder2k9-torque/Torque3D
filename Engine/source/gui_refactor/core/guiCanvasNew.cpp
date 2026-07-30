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

//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//
// Arcane-FX for MIT Licensed Open Source version of Torque 3D from GarageGames
// Copyright (C) 2015 Faust Logic, Inc.
//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~//~~~~~~~~~~~~~~~~~~~~~//

#include "platform/platform.h"
#include "gui_refactor/core/guiCanvasNew.h"

#include "console/console.h"
#include "console/engineAPI.h"
#include "platform/profiler.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/core/guiTypes.h"
#include "gui_refactor/core/guiControlNew.h"
#include "gui_refactor/editor/guiMenuBarNew.h"
#include "console/consoleTypes.h"
#include "gfx/screenshot.h"
#include "gfx/video/videoCapture.h"
#include "lighting/lightManager.h"
#include "core/strings/stringUnit.h"
#include "gui_refactor/core/guiOffscreenCanvasNew.h"
#include "sfx/sfxSystem.h"

#ifndef TORQUE_TGB_ONLY
#include "scene/sceneObject.h"
#endif

#include "gfx/gfxInit.h"
#include "core/util/journal/process.h"

#ifdef TORQUE_GFX_STATE_DEBUG
#include "gfx/gfxDebugStateTracker.h"
#endif

IMPLEMENT_CONOBJECT(GuiCanvasNew);

ConsoleDocClass(GuiCanvasNew,
   "@brief A canvas on which rendering occurs.\n\n"

   "@section GuiCanvas_contents What a GUICanvas Can Contain...\n\n"

   "@subsection GuiCanvas_content_contentcontrol Content Control\n"
   "A content control is the top level GuiControlNew for a screen. This GuiControlNew "
   "will be the parent control for all other GuiControls on that particular "
   "screen.\n\n"

   "@subsection GuiCanvas_content_dialogs Dialogs\n\n"

   "A dialog is essentially another screen, only it gets overlaid on top of the "
   "current content control, and all input goes to the dialog. This is most akin "
   "to the \"Open File\" dialog box found in most operating systems. When you "
   "choose to open a file, and the \"Open File\" dialog pops up, you can no longer "
   "send input to the application, and must complete or cancel the open file "
   "request. Torque keeps track of layers of dialogs. The dialog with the highest "
   "layer is on top and will get all the input, unless the dialog is "
   "modeless, which is a profile option.\n\n"

   "@see GuiControlProfile\n\n"

   "@section GuiCanvas_dirty Dirty Rectangles\n\n"

   "The GuiCanvasNew is based on dirty regions. "
   "Every frame the canvas paints only the areas of the canvas that are 'dirty' "
   "or need updating. In most cases, this only is the area under the mouse cursor. "
   "This is why if you look in guiCanvasNew.cc the call to glClear is commented out. "

   "What you will see is a black screen, except in the dirty regions, where the "
   "screen will be painted normally. If you are making an animated GuiControlNew "
   "you need to add your control to the dirty areas of the canvas.\n\n"

   "@see GuiControlNew\n\n"

   "@ingroup GuiCore\n");

ImplementEnumType(NewKeyboardTranslationMode,
   "Modes for handling keyboard translation or native accelerator requests.\n\n")
{
   GuiCanvasNew::TranslationMode_Platform, "Platform",
      "Requests will be passed to the platform window for handling."
},
{ GuiCanvasNew::TranslationMode_Callback, "Callback",
   "Script callbacks will be issued to notify and allow override of these events." },
{ GuiCanvasNew::TranslationMode_Ignore, "Ignore",
   "Requsts to enable/disable keyboard translations or native accelerators will be ignored "
   "with no callback triggered." },
   EndImplementEnumType;

IMPLEMENT_CALLBACK(GuiCanvasNew, onSetKeyboardTranslationEnabled, bool, (bool enable), (enable),
   "Called when the canvas receives an enableKeyboardTranslation request. This is usually the "
   "result of a GuitTextInputCtrl gaining or losing focus. Return true to allow the request "
   "to be passed to the platform window. Return false to override the request and handle it in script.\n\n"
   "@note This callback is only issued if keyTranslationMode is set to \"Callback\" for this canvas.\n"
   "@param enable Requested keyboard translation state.\n"
   "@see KeyboardTranslationMode\n");

IMPLEMENT_CALLBACK(GuiCanvasNew, onSetNativeAcceleratorsEnabled, bool, (bool enable), (enable),
   "Called when the canvas receives a setNativeAcceleratorsEnabled request. This is usually the "
   "result of a GuitTextInputCtrl gaining or losing focus. Return true to allow the request to "
   "be passed to the platform window. Return false to override the request and handle it in script.\n\n"
   "@note This callback is only issued if nativeAcceleratorMode is set to \"Callback\" for this canvas.\n"
   "@param enable Requested accelerator state.\n"
   "@see KeyboardTranslationMode\n");

ColorI gNewCanvasClearColor(255, 0, 255); ///< For GFX->clear

extern InputModifiers convertModifierBits(const U32 in);

//-----------------------------------------------------------------------------

GuiCanvasNew::GuiCanvasNew() : GuiControlNew(),
                              mCurUpdateRect(0, 0, 0, 0),
                              mCursorEnabled(true),
                              mForceMouseToGUI(false),
                              mAlwaysHandleMouseButtons(false),
                              mShowCursor(true),
                              mClampTorqueCursor(true),
                              mCursorChanged(0),
                              mLastCursorEnabled(false),
                              mMouseCapturedControl(NULL),
                              mMouseControl(NULL),
                              mMouseControlClicked(false),
                              mMouseButtonDown(false),
                              mMouseRightButtonDown(false),
                              mDefaultCursor(NULL),
                              mMouseMiddleButtonDown(false),
                              mCursorPt(0, 0),
                              mLastCursorPt(0, 0),
                              mLastCursor(NULL),
                              mLastMouseClickCount(0),
                              mRenderFront(false),
                              mPrevMouseTime(0),
                              mLastMouseDownTime(0),
                              mHoverControl(NULL),
                              mHoverPositionSet(false),
                              mLeftMouseLast(false),
                              mHoverLeftControlTime(0),
                              mKeyTranslationMode(TranslationMode_Platform),
                              mNativeAcceleratorMode(TranslationMode_Platform),
                              mMiddleMouseLast(false),
                              mRightMouseLast(false),
                              mMouseDownPoint(0.0f, 0.0f),
                              mLastRenderMs(0),
                              mPlatformWindow(NULL),
                              mDisplayWindow(true),
                              mMenuBarCtrl(NULL),
                              mMenuBackground(NULL),
                              mConstrainMouse(false),
                              mDesignSize(0, 0),
                              mDesignDPIScale(1.0f),
                              mLockAspectRatio(false),
                              mContentDeviceRect(0, 0, 640, 480),
                              mEffectiveScaleX(1.0f),
                              mEffectiveScaleY(1.0f)
{
   setBounds(0, 0, 640, 480);
   mAwake = true;

   mHoverControlStart = Platform::getRealMilliseconds();
   mHoverPosition = getCursorPos();

   mFences = NULL;
   mNextFenceIdx = -1;

#ifndef _XBOX
   mNumFences = Con::getIntVariable("$pref::Video::defaultFenceCount", 0);
#else
   mNumFences = 0;
#endif
   mConsumeLastInputEvent = false;
   mLastInputDeviceType = -1;
}

GuiCanvasNew::~GuiCanvasNew()
{
   SAFE_DELETE(mPlatformWindow);
   SAFE_DELETE_ARRAY(mFences);
}

//------------------------------------------------------------------------------

bool GuiCanvasNew::setProtectedNumFences(void* object, const char* index, const char* data)
{
   GuiCanvasNew* canvas = reinterpret_cast<GuiCanvasNew*>(object);
   canvas->mNumFences = dAtoi(data);
   canvas->setupFences();

   return false;
}

void GuiCanvasNew::initPersistFields()
{
   docsURL;
   addGroup("Mouse Handling");
   addField("alwaysHandleMouseButtons", TypeBool, Offset(mAlwaysHandleMouseButtons, GuiCanvasNew),
      "Deal with mouse buttons, even if the cursor is hidden.");
   endGroup("Mouse Handling");

   addGroup("Canvas Rendering");
   addProtectedField("numFences", TypeS32, Offset(mNumFences, GuiCanvasNew), &setProtectedNumFences, &defaultProtectedGetFn, "The number of GFX fences to use.");

   addField("displayWindow", TypeBool, Offset(mDisplayWindow, GuiCanvasNew), "Controls if the canvas window is rendered or not.");
   addField("lockAspectRatio", TypeBool, Offset(mLockAspectRatio, GuiCanvasNew),
      "When true (default), the content control's design aspect ratio is preserved (letterboxed/pillarboxed) "
      "rather than stretched independently per axis to fill the window.");
   endGroup("Canvas Rendering");

   addGroup("KeyboardMode Callbacks");
   addField("keyTranslationMode", TYPEID< KeyTranslationMode >(), Offset(mKeyTranslationMode, GuiCanvasNew),
      "How to handle enable/disable keyboard translation requests. \"Platform\", \"Callback\" or \"Ignore\".\n");
   addField("nativeAcceleratorMode", TYPEID< KeyTranslationMode >(), Offset(mNativeAcceleratorMode, GuiCanvasNew),
      "How to handle enable/disable native accelerator requests. \"Platform\", \"Callback\" or \"Ignore\".\n");
   endGroup("KeyboardMode Callbacks");

   Parent::initPersistFields();
}

//------------------------------------------------------------------------------

bool GuiCanvasNew::onAdd()
{
   // ensure that we have a cursor
   setCursor(dynamic_cast<GuiCursor*>(Sim::findObject("DefaultCursor")));

   SFXSystem::enumerateProviders();
   SFXProvider* p = SFXSystem::getBestProviderChoice();

   if (p)
      SFX->createDevice(p);

   // Enumerate things for GFX before we have an active device.
   GFXInit::enumerateAdapters();

   // Create a device.
   GFXAdapter* a = GFXInit::getBestAdapterChoice();

   // Do we have a global device already? (This is the site if you want
   // to start rendering to multiple devices simultaneously)
   GFXDevice* newDevice = GFX;
   if (newDevice == NULL)
      newDevice = GFXInit::createDevice(a);

   newDevice->setAllowRender(false);

   // Disable starting a new journal recording or playback from here on
   Journal::Disable();

   // Initialize the window...
   GFXVideoMode vm = GFXInit::getInitialVideoMode();

   //If we're recording, store the intial video resolution
   if (Journal::IsRecording())
   {
      Journal::Write(vm.resolution.x);
      Journal::Write(vm.resolution.y);
      Journal::Write(vm.fullScreen);
   }

   //If we're playing, read the intial video resolution from the journal
   if (Journal::IsPlaying())
   {
      Journal::Read(&vm.resolution.x);
      Journal::Read(&vm.resolution.y);
      Journal::Read(&vm.fullScreen);
   }

   if (a && a->mType != NullDevice)
   {
      mPlatformWindow = WindowManager->createWindow(newDevice, vm);

      //Disable window resizing if recording ir playing a journal
      if (Journal::IsRecording() || Journal::IsPlaying())
         mPlatformWindow->lockSize(true);

      // Set a minimum on the window size so people can't break us by resizing tiny.
      mPlatformWindow->setMinimumWindowSize(Point2I(Con::getIntVariable("$Video::minimumXResolution", 1024),
         Con::getIntVariable("$Video::minimumYResolution", 720)));

      // Now, we have to hook in our event callbacks so we'll get
      // appropriate events from the window.
      mPlatformWindow->resizeEvent.notify(this, &GuiCanvasNew::handleResize);
      mPlatformWindow->appEvent.notify(this, &GuiCanvasNew::handleAppEvent);
      mPlatformWindow->displayEvent.notify(this, &GuiCanvasNew::handlePaintEvent);
      mPlatformWindow->setInputController(dynamic_cast<IProcessInput*>(this));
   }

   // Need to get painted, too! :)
   Process::notify(this, &GuiCanvasNew::paint, PROCESS_RENDER_ORDER);

   // Set up the fences
   setupFences();

   // Make sure we're able to render.
   newDevice->setAllowRender(true);

   // NULL device returns a nullptr for getPlatformWindow
   PlatformWindow* window = getPlatformWindow();
   if (mDisplayWindow && window)
   {
      window->show();
      WindowManager->setDisplayWindow(true);
      window->setDisplayWindow(true);
   }
   else if (window)
   {
      window->hide();
      WindowManager->setDisplayWindow(false);
      window->setDisplayWindow(false);
   }

   // Propagate add to parents.
   // CodeReview - if GuiCanvasNew fails to add for whatever reason, what happens to
   // all the event registration above?
   bool parentRet = Parent::onAdd();

   // Define the menu bar for this canvas (if any)
   Con::executef(this, "onCreateMenu");

   return parentRet;
}

void GuiCanvasNew::onRemove()
{
   // And the process list
   Process::remove(this, &GuiCanvasNew::paint);

   // Destroy the menu bar for this canvas (if any)
   Con::executef(this, "onDestroyMenu");

   Parent::onRemove();
}
#ifdef TORQUE_TOOLS
void GuiCanvasNew::setMenuBar(SimObject* obj)
{
   GuiControlNew* oldMenuBar = mMenuBarCtrl;
   mMenuBarCtrl = dynamic_cast<GuiControlNew*>(obj);

   //remove old menubar
   if (oldMenuBar)
   {
      Parent::removeObject(oldMenuBar);
      Parent::removeObject(mMenuBackground); //also remove the modeless wrapper
   }

   // set new menubar    
   if (mMenuBarCtrl)
   {
      //Add a wrapper control so that the menubar sizes correctly
      if (mMenuBackground == NULL)
      {
         mMenuBackground = new GuiControlNew();
         mMenuBackground->registerObject();

         GuiStyle* style = NULL;
         if (Sim::findObject("GuiModelessDialogStyle", style) && style)
            mMenuBackground->setStyle(style);
         else
            Con::warnf("GuiCanvasNew::setMenuBar: GuiModelessDialogStyle not found -- menu bar wrapper will use default styling.");
      }

      mMenuBackground->addObject(mMenuBarCtrl);

      Parent::addObject(mMenuBackground);
   }

   // update accelerator keys for the new menu bar, using the same
   // standard GuiControlNew accelerator mechanism setContentControl()/
   // pushDialogControl()/popDialogControl() already use elsewhere in
   // this class (mAcceleratorMap rebuilt from every top-level canvas
   // child) -- see guiControlNew.h's buildAcceleratorMap()/addAcceleratorKey().
   // The old GuiMenuBarNew::buildWindowAcceleratorMap()/
   // removeWindowAcceleratorMap(WindowInputGenerator&) pair this replaced
   // was a separate, OS-window-level accelerator hook tied to a type
   // that doesn't exist in the rewritten GUI stack (see
   // gui-migration-plan.md) -- the new GuiMenuBarNew relies entirely on the
   // standard mechanism instead, so no menu-bar-specific accelerator API
   // is needed here at all.
   if (oldMenuBar != mMenuBarCtrl)
   {
      mAcceleratorMap.clear();

      for (iterator i = end(); i != begin(); )
      {
         i--;
         GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
         ctrl->buildAcceleratorMap();

         if (ctrl->getCapturesInput())
            break;
      }
   }
}
#endif
void GuiCanvasNew::setWindowTitle(const char* newTitle)
{
   if (mPlatformWindow)
      mPlatformWindow->setCaption(newTitle);
}

CanvasNewSizeChangeSignal GuiCanvasNew::smCanvasNewSizeChangeSignal;
CanvasNewSetActiveSignal GuiCanvasNew::smCanvasNewSetActiveSignal;

void GuiCanvasNew::handleResize(WindowId did, S32 width, S32 height)
{
   getCanvasSizeChangeSignal().trigger(this);
   if (Journal::IsPlaying() && mPlatformWindow)
   {
      mPlatformWindow->lockSize(false);
      mPlatformWindow->setSize(Point2I(width, height));
      mPlatformWindow->lockSize(true);
   }

   // Notify the scripts
   if (isMethod("onResize"))
      Con::executef(this, "onResize", Con::getIntArg(width), Con::getIntArg(height));
}

void GuiCanvasNew::handlePaintEvent(WindowId did)
{
   bool canRender = mPlatformWindow->isVisible() && GFX->allowRender() && !GFX->canCurrentlyRender();

   //// Do the screenshot first.
   //if ( gScreenShot != NULL && gScreenShot->isPending() && canRender )
   //   gScreenShot->capture( this );

   //// If the video capture is waiting for a canvas, start the capture
   //if ( VIDCAP->isWaitingForCanvas() && canRender )
   //   VIDCAP->begin( this );

   //// Now capture the video   
   //if ( VIDCAP->isRecording() && canRender )
   //   VIDCAP->capture();

   renderFrame(false);
}

void GuiCanvasNew::handleAppEvent(WindowId did, S32 event)
{
   // Notify script if we gain or lose focus.
   if (event == LoseFocus)
   {
      if (isMethod("onLoseFocus"))
         Con::executef(this, "onLoseFocus");
   }

   if (event == GainFocus)
   {
      if (isMethod("onGainFocus"))
         Con::executef(this, "onGainFocus");
   }

   if (event == WindowClose || event == WindowDestroy)
   {

      if (isMethod("onWindowClose"))
      {
         // First see if there is a method on this window to handle 
         //  it's closure
         Con::executef(this, "onWindowClose");
      }
      else if (Con::isFunction("onWindowClose"))
      {
         // otherwise check to see if there is a global function handling it
         Con::executef("onWindowClose", getIdString());
      }
      else
      {
         // Else just shutdown
         Process::requestShutdown();
      }
   }
}

Point2I GuiCanvasNew::getWindowSize()
{
   // CodeReview Asserting on this breaks previous logic
   // and code assumptions.  It seems logical that we would
   // handle this and return an error value rather than implementing
   // if(!mPlatformWindow) whenever we need to call getWindowSize.
   // This should help keep our API error free and easy to use, while
   // cutting down on code duplication for sanity checking.  [5/5/2007 justind]
   if (!mPlatformWindow)
      return Point2I(-1, -1);

   return mPlatformWindow->getClientExtent();
}

void GuiCanvasNew::enableKeyboardTranslation()
{
   if ((mKeyTranslationMode == TranslationMode_Platform) ||
      ((mKeyTranslationMode == TranslationMode_Callback) && onSetKeyboardTranslationEnabled_callback(true)))
   {
      AssertISV(mPlatformWindow, "GuiCanvasNew::enableKeyboardTranslation - no window present!");
      mPlatformWindow->setKeyboardTranslation(true);
   }
}

void GuiCanvasNew::disableKeyboardTranslation()
{
   if ((mKeyTranslationMode == TranslationMode_Platform) ||
      ((mKeyTranslationMode == TranslationMode_Callback) && onSetKeyboardTranslationEnabled_callback(false)))
   {
      AssertISV(mPlatformWindow, "GuiCanvasNew::disableKeyboardTranslation - no window present!");
      mPlatformWindow->setKeyboardTranslation(false);
   }
}

void GuiCanvasNew::setNativeAcceleratorsEnabled(bool enabled)
{
   if ((mNativeAcceleratorMode == TranslationMode_Platform) ||
      ((mNativeAcceleratorMode == TranslationMode_Callback) && onSetNativeAcceleratorsEnabled_callback(enabled)))
   {
      AssertISV(mPlatformWindow, "GuiCanvasNew::setNativeAcceleratorsEnabled - no window present!");
      mPlatformWindow->setAcceleratorsEnabled(enabled);
   }
}

void GuiCanvasNew::setForceMouseToGUI(bool onOff)
{
   mForceMouseToGUI = onOff;
}

void GuiCanvasNew::setClampTorqueCursor(bool onOff)
{
   mClampTorqueCursor = onOff;
}

void GuiCanvasNew::setCursor(GuiCursor* curs)
{
   mDefaultCursor = curs;
}

void GuiCanvasNew::setCursorON(bool onOff)
{
   mCursorEnabled = onOff;
   if (!mCursorEnabled)
      mMouseControl = NULL;
}

Point2I GuiCanvasNew::getCursorPos()
{
   Point2I p(0, 0);

   if (mPlatformWindow)
      mPlatformWindow->getCursorPosition(p);

   return p;
}

void GuiCanvasNew::setCursorPos(const Point2I& pt)
{
   AssertISV(mPlatformWindow, "GuiCanvasNew::setCursorPos - no window present!");

   if (mPlatformWindow->isMouseLocked())
   {
      mCursorPt.x = F32(pt.x);
      mCursorPt.y = F32(pt.y);
   }
   else
   {
      mPlatformWindow->setCursorPosition(pt.x, pt.y);
   }
}

void GuiCanvasNew::showCursor(bool state)
{
   mShowCursor = state;
   mPlatformWindow->setCursorVisible(state);
}

bool GuiCanvasNew::isCursorShown()
{
   if (!mPlatformWindow->getCursorController())
   {
      return mShowCursor;
   }

   return mPlatformWindow->isCursorVisible();
}

void GuiCanvasNew::cursorClick(S32 buttonId, bool isDown)
{
   InputEventInfo inputEvent;
   inputEvent.deviceType = MouseDeviceType;
   inputEvent.deviceInst = 0;
   inputEvent.objType = SI_BUTTON;
   inputEvent.objInst = (InputObjectInstances)(KEY_BUTTON0 + buttonId);
   inputEvent.modifier = (InputModifiers)0;
   inputEvent.ascii = 0;
   inputEvent.action = isDown ? SI_MAKE : SI_BREAK;
   inputEvent.fValue = isDown ? 1.0 : 0.0;

   processMouseEvent(inputEvent);
}

void GuiCanvasNew::cursorNudge(F32 x, F32 y)
{
   // Generate a base Movement along and Axis event
   InputEventInfo inputEvent;
   inputEvent.deviceType = MouseDeviceType;
   inputEvent.deviceInst = 0;
   inputEvent.objType = SI_AXIS;
   inputEvent.modifier = (InputModifiers)0;
   inputEvent.ascii = 0;

   // Generate delta movement along each axis
   Point2F cursDelta(x, y);

   // If X axis changed, generate a relative event
   if (mFabs(cursDelta.x) > 0.1)
   {
      inputEvent.objInst = SI_XAXIS;
      inputEvent.action = SI_MOVE;
      inputEvent.fValue = cursDelta.x;
      processMouseEvent(inputEvent);
   }

   // If Y axis changed, generate a relative event
   if (mFabs(cursDelta.y) > 0.1)
   {
      inputEvent.objInst = SI_YAXIS;
      inputEvent.action = SI_MOVE;
      inputEvent.fValue = cursDelta.y;
      processMouseEvent(inputEvent);
   }

   processMouseEvent(inputEvent);
}

void GuiCanvasNew::addAcceleratorKey(GuiControlNew* ctrl, U32 index, U32 keyCode, U32 modifier)
{
   if (keyCode > 0 && ctrl)
   {
      AccKeyMap newMap;
      newMap.ctrl = ctrl;
      newMap.index = index;
      newMap.keyCode = keyCode;
      newMap.modifier = modifier;
      mAcceleratorMap.push_back(newMap);
   }
}

bool GuiCanvasNew::tabNext(void)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(last());
   if (ctrl)
   {
      //save the old
      GuiControlNew* oldResponder = mFirstResponder;

      GuiControlNew* newResponder = ctrl->findNextTabable(mFirstResponder);
      if (!newResponder)
         newResponder = ctrl->findFirstTabable();

      if (newResponder && newResponder != oldResponder)
      {
         newResponder->setFirstResponder();

         // CodeReview Can this get killed? Note tabPrev code. BJG - 3/25/07
//       if ( oldResponder )
//          oldResponder->onLoseFirstResponder();
         return true;
      }
   }
   return false;
}

bool GuiCanvasNew::tabPrev(void)
{
   GuiControlNew* ctrl = static_cast<GuiControlNew*>(last());
   if (ctrl)
   {
      //save the old
      GuiControlNew* oldResponder = mFirstResponder;

      GuiControlNew* newResponder = ctrl->findPrevTabable(mFirstResponder);
      if (!newResponder)
         newResponder = ctrl->findLastTabable();

      if (newResponder && newResponder != oldResponder)
      {
         newResponder->setFirstResponder();

         // CodeReview As with tabNext() above, looks like this can now go. DAW - 7/05/09
         //if ( oldResponder )
         //   oldResponder->onLoseFirstResponder();

         return true;
      }
   }
   return false;
}

bool GuiCanvasNew::processInputEvent(InputEventInfo& inputEvent)
{
   mConsumeLastInputEvent = true;
   mLastInputDeviceType = inputEvent.deviceType;

   // If we have an active offscreen canvas, give it the input
   if (GuiOffscreenCanvasNew::sActiveOffscreenCanvas &&
      (GuiOffscreenCanvasNew::sActiveOffscreenCanvas != this) &&
      GuiOffscreenCanvasNew::sActiveOffscreenCanvas->processInputEvent(inputEvent))
   {
      GuiOffscreenCanvasNew::sActiveOffscreenCanvas = NULL;
      return mConsumeLastInputEvent;
   }

   // First call the general input handler (on the extremely off-chance that it will be handled):
   if (mFirstResponder && mFirstResponder->onInputEvent(inputEvent))
   {
      return mConsumeLastInputEvent;
   }

   switch (inputEvent.deviceType)
   {
   case KeyboardDeviceType:
      return processKeyboardEvent(inputEvent);
      break;

   case GamepadDeviceType:
      return processGamepadEvent(inputEvent);
      break;

   case MouseDeviceType:
      if (mCursorEnabled || mForceMouseToGUI ||
         (mAlwaysHandleMouseButtons && inputEvent.objType == SI_BUTTON))
      {
         return processMouseEvent(inputEvent);
      }
      break;
   default:
      break;
   }

   return false;
}

bool GuiCanvasNew::processKeyboardEvent(InputEventInfo& inputEvent)
{
   mLastEvent.ascii = inputEvent.ascii;
   mLastEvent.modifier = inputEvent.modifier;
   mLastEvent.keyCode = inputEvent.objInst;

   // Combine left/right shift bits - if one shift modifier key
   // bit is set, then set the other one. This way we can simplify
   // our processing logic by treating the keys identically.
   U32 eventModifier = inputEvent.modifier;
   if (eventModifier & SI_SHIFT)
   {
      eventModifier |= SI_SHIFT;
   }
   if (eventModifier & SI_CTRL)
   {
      eventModifier |= SI_CTRL;
   }
   if (eventModifier & SI_ALT)
   {
      eventModifier |= SI_ALT;
   }

   if (inputEvent.action == SI_MAKE)
   {
      //see if we should now pass the event to the first responder
      if (mFirstResponder)
      {
         if (mFirstResponder->onKeyDown(mLastEvent))
            return mConsumeLastInputEvent;
      }

      //see if we should tab next/prev
      if (isCursorON() && (inputEvent.objInst == KEY_TAB))
      {
         if (size() > 0)
         {
            if (inputEvent.modifier & SI_SHIFT)
            {
               if (tabPrev())
                  return mConsumeLastInputEvent;
            }
            else if (inputEvent.modifier == 0)
            {
               if (tabNext())
                  return mConsumeLastInputEvent;
            }
         }
      }

      //if not handled, search for an accelerator
      for (U32 i = 0; i < mAcceleratorMap.size(); i++)
      {
         if ((U32)mAcceleratorMap[i].keyCode == (U32)inputEvent.objInst && (U32)mAcceleratorMap[i].modifier == eventModifier)
         {
            mAcceleratorMap[i].ctrl->acceleratorKeyPress(mAcceleratorMap[i].index);
            return mConsumeLastInputEvent;
         }
      }
   }
   else if (inputEvent.action == SI_BREAK)
   {
      if (mFirstResponder && mFirstResponder->onKeyUp(mLastEvent))
         return mConsumeLastInputEvent;

      //see if there's an accelerator
      for (U32 i = 0; i < mAcceleratorMap.size(); i++)
      {
         if ((U32)mAcceleratorMap[i].keyCode == (U32)inputEvent.objInst && (U32)mAcceleratorMap[i].modifier == eventModifier)
         {
            mAcceleratorMap[i].ctrl->acceleratorKeyRelease(mAcceleratorMap[i].index);
            return mConsumeLastInputEvent;
         }
      }
   }
   else if (inputEvent.action == SI_REPEAT)
   {
      //if not handled, search for an accelerator
      for (U32 i = 0; i < mAcceleratorMap.size(); i++)
      {
         if ((U32)mAcceleratorMap[i].keyCode == (U32)inputEvent.objInst && (U32)mAcceleratorMap[i].modifier == eventModifier)
         {
            mAcceleratorMap[i].ctrl->acceleratorKeyPress(mAcceleratorMap[i].index);
            return mConsumeLastInputEvent;
         }
      }

      if (mFirstResponder)
      {
         bool ret = mFirstResponder->onKeyRepeat(mLastEvent);
         return ret && mConsumeLastInputEvent;
      }
   }
   return false;
}

bool GuiCanvasNew::processMouseEvent(InputEventInfo& inputEvent)
{
   // [rene 09/09/10] This custom mouse cursor tracking that is happening here is bad.  It will frequently
   //    get ouf of step with where the cursor actually is.  We really should *not* track the cursor; it's
   //    just another thing that can/will go wrong.  Let the input system pass us absolute screen coordinates
   //    for every mouse event instead and work off that.
   //
   //    'mCursorPt' basically is an accumulation of errors and the number of bugs that have cropped up with
   //    the GUI clicking stuff where it is not supposed to are probably all to blame on this.

   S32 mouseDoubleClickWidth = 12;
   S32 mouseDoubleClickHeight = 12;
   U32 mouseDoubleClickTime = 500;

   // Query platform for mouse info if its available
   PlatformCursorController* pController = mPlatformWindow ? mPlatformWindow->getCursorController() : NULL;
   if (pController)
   {
      mouseDoubleClickWidth = pController->getDoubleClickWidth();
      mouseDoubleClickHeight = pController->getDoubleClickHeight();
      mouseDoubleClickTime = pController->getDoubleClickTime();
   }

   //copy the modifier into the new event
   mLastEvent.modifier = inputEvent.modifier;

   if (inputEvent.objType == SI_AXIS &&
      (inputEvent.objInst == SI_XAXIS || inputEvent.objInst == SI_YAXIS))
   {

      // Set the absolute position if we get an SI_MAKE on an axis
      if (inputEvent.objInst == SI_XAXIS)
      {
         if (inputEvent.action == SI_MAKE)
            mCursorPt.x = (S32)inputEvent.fValue;
         else if (inputEvent.action == SI_MOVE)
            mCursorPt.x += (S32)inputEvent.fValue;
         mCursorPt.x = getMax(0, getMin((S32)mCursorPt.x, getBounds().extent.x - 1));
      }
      else if (inputEvent.objInst == SI_YAXIS)
      {
         if (inputEvent.action == SI_MAKE)
            mCursorPt.y = (S32)inputEvent.fValue;
         else if (inputEvent.action == SI_MOVE)
            mCursorPt.y += (S32)inputEvent.fValue;
         mCursorPt.y = getMax(0, getMin((S32)mCursorPt.y, getBounds().extent.y - 1));
      }

      // Store new cursor position. mLastEvent.mousePoint is LOGICAL from
      // here on -- convert once here, at the point this raw device
      // coordinate is captured, so everything downstream (findHitControl,
      // individual controls' event handlers) can consistently assume
      // logical units. mCursorPt itself stays in device pixels (it's also
      // used for double-click distance thresholding just below, a
      // physical-motion concern).
      mLastEvent.mousePoint = deviceToLogicalPoint(Point2I(S32(mCursorPt.x), S32(mCursorPt.y)));

      // See if we need to invalidate a possible dbl click due to the cursor
      // moving too much.
      Point2F movement = mMouseDownPoint - mCursorPt;

      if ((mAbs((S32)movement.x) > mouseDoubleClickWidth) || (mAbs((S32)movement.y) > mouseDoubleClickHeight))
      {
         mLeftMouseLast = false;
         mMiddleMouseLast = false;
         mRightMouseLast = false;
      }

      if (mMouseButtonDown)
         rootMouseDragged(mLastEvent);
      else if (mMouseRightButtonDown)
         rootRightMouseDragged(mLastEvent);
      else if (mMouseMiddleButtonDown)
         rootMiddleMouseDragged(mLastEvent);
      else
         rootMouseMove(mLastEvent);
      return mConsumeLastInputEvent;
   }
   else if (inputEvent.objInst == SI_ZAXIS
      || inputEvent.objInst == SI_RZAXIS)
   {
      mLastEvent.mousePoint = deviceToLogicalPoint(Point2I(S32(mCursorPt.x), S32(mCursorPt.y)));
      mLastEvent.fval = inputEvent.fValue;

      if (inputEvent.objInst == SI_ZAXIS)
         mLastEvent.mouseAxis = 1;
      else
         mLastEvent.mouseAxis = 0;

      if (inputEvent.fValue < 0.0f)
         return rootMouseWheelDown(mLastEvent);
      else
         return rootMouseWheelUp(mLastEvent);
   }
   else if (inputEvent.objType == SI_BUTTON)
   {
      //copy the cursor point into the event, converted to logical space
      mLastEvent.mousePoint = deviceToLogicalPoint(Point2I(S32(mCursorPt.x), S32(mCursorPt.y)));
      mMouseDownPoint = mCursorPt; // stays device-space; see double-click distance check above

      if (inputEvent.objInst == KEY_BUTTON0) // left button
      {
         //see if button was pressed
         if (inputEvent.action == SI_MAKE)
         {
            U32 curTime = Platform::getVirtualMilliseconds();

            //if the last button pressed was the left...
            if (mLeftMouseLast)
            {
               //if it was within the double click time count the clicks
               if (curTime - mLastMouseDownTime <= mouseDoubleClickTime)
                  mLastMouseClickCount++;
               else
                  mLastMouseClickCount = 1;
            }
            else
            {
               mLeftMouseLast = true;
               mLastMouseClickCount = 1;
            }

            mLastMouseDownTime = curTime;
            mLastEvent.mouseClickCount = mLastMouseClickCount;

            rootMouseDown(mLastEvent);
         }
         //else button was released
         else
         {
            rootMouseUp(mLastEvent);
         }

         return mConsumeLastInputEvent;
      }
      else if (inputEvent.objInst == KEY_BUTTON1) // right button
      {
         if (inputEvent.action == SI_MAKE)
         {
            U32 curTime = Platform::getVirtualMilliseconds();

            //if the last button pressed was the right...
            if (mRightMouseLast)
            {
               //if it was within the double click time count the clicks
               if (curTime - mLastMouseDownTime <= mouseDoubleClickTime)
                  mLastMouseClickCount++;
               else
                  mLastMouseClickCount = 1;
            }
            else
            {
               mRightMouseLast = true;
               mLastMouseClickCount = 1;
            }

            mLastMouseDownTime = curTime;
            mLastEvent.mouseClickCount = mLastMouseClickCount;

            rootRightMouseDown(mLastEvent);
         }
         else // it was a mouse up
            rootRightMouseUp(mLastEvent);

         return mConsumeLastInputEvent;
      }
      else if (inputEvent.objInst == KEY_BUTTON2) // middle button
      {
         if (inputEvent.action == SI_MAKE)
         {
            U32 curTime = Platform::getVirtualMilliseconds();

            //if the last button pressed was the right...
            if (mMiddleMouseLast)
            {
               //if it was within the double click time count the clicks
               if (curTime - mLastMouseDownTime <= mouseDoubleClickTime)
                  mLastMouseClickCount++;
               else
                  mLastMouseClickCount = 1;
            }
            else
            {
               mMiddleMouseLast = true;
               mLastMouseClickCount = 1;
            }

            mLastMouseDownTime = curTime;
            mLastEvent.mouseClickCount = mLastMouseClickCount;

            rootMiddleMouseDown(mLastEvent);
         }
         else // it was a mouse up
            rootMiddleMouseUp(mLastEvent);

         return mConsumeLastInputEvent;
      }
   }
   return false;
}

bool GuiCanvasNew::processGamepadEvent(InputEventInfo& inputEvent)
{
   if (!mFirstResponder)
   {
      // early out, no first responder to receive gamepad input
      return false;
   }

   if (inputEvent.deviceInst >= MAX_GAMEPADS)
   {
      // early out, we only support the first MAX_GAMEPADS gamepads
      return false;
   }

   mLastEvent.keyCode = inputEvent.objInst;

   if (inputEvent.objType == SI_BUTTON)
   {
      switch (inputEvent.action)
      {
      case SI_MAKE:
         switch (inputEvent.objInst)
         {
         case SI_UPOV:
            return mFirstResponder->onGamepadAxisUp(mLastEvent);

         case SI_DPOV:
            return mFirstResponder->onGamepadAxisDown(mLastEvent);

         case SI_LPOV:
            return mFirstResponder->onGamepadAxisLeft(mLastEvent);

         case SI_RPOV:
            return mFirstResponder->onGamepadAxisRight(mLastEvent);

         default:
            return mFirstResponder->onGamepadButtonDown(mLastEvent);
         }
         break;

      case SI_BREAK:
         return mFirstResponder->onGamepadButtonUp(mLastEvent);

      default:
         return false;
      }
   }
   else if (inputEvent.objType == SI_AXIS)
   {
      F32 incomingValue = mFabs(inputEvent.fValue);
      static const F32 DEAD_ZONE = 0.5f;
      static const F32 MIN_CLICK_TIME = 500.0f;
      static const F32 MAX_CLICK_TIME = 1000.0f;
      static F32 xDecay[] = { 1.0f, 1.0f, 1.0f, 1.0f };
      static F32 yDecay[] = { 1.0f, 1.0f, 1.0f, 1.0f };
      static F32 zDecay[] = { 1.0f, 1.0f, 1.0f, 1.0f };
      static U32 xLastClickTime[] = { 0, 0, 0, 0 };
      static U32 yLastClickTime[] = { 0, 0, 0, 0 };
      static U32 zLastClickTime[] = { 0, 0, 0, 0 };
      U32 curTime = Platform::getRealMilliseconds();
      F32* decay;
      U32* lastClickTime;

      switch (inputEvent.objInst)
      {
      case SI_ZAXIS:
      case XI_LEFT_TRIGGER:
      case XI_RIGHT_TRIGGER:
         decay = &zDecay[inputEvent.deviceInst];
         lastClickTime = &zLastClickTime[inputEvent.deviceInst];
         break;

      case SI_YAXIS:
      case XI_THUMBLY:
      case XI_THUMBRY:
         decay = &yDecay[inputEvent.deviceInst];
         lastClickTime = &yLastClickTime[inputEvent.deviceInst];
         break;

      case SI_XAXIS:
      case XI_THUMBLX:
      case XI_THUMBRX:
      default:
         decay = &xDecay[inputEvent.deviceInst];
         lastClickTime = &xLastClickTime[inputEvent.deviceInst];
         break;
      }

      if (incomingValue < DEAD_ZONE)
      {
         // early out, control movement is within the deadzone
         *decay = 1.0f;
         *lastClickTime = 0;
         return false;
      }

      // Rescales the input between 0.0 and 1.0
      incomingValue = (incomingValue - DEAD_ZONE) * (1.0f / (1.0f - DEAD_ZONE));

      F32 clickTime = MIN_CLICK_TIME + (MAX_CLICK_TIME - MIN_CLICK_TIME) * (1.0f - incomingValue);
      clickTime *= *decay;

      if (clickTime < (curTime - *lastClickTime))
      {
         *decay *= 0.9f;
         if (*decay < 0.2f)
         {
            *decay = 0.2f;
         }

         *lastClickTime = curTime;

         bool negative = (inputEvent.fValue < 0.0f);

         switch (inputEvent.objInst)
         {
         case SI_ZAXIS:
         case SI_RZAXIS:
            return mFirstResponder->onGamepadTrigger(mLastEvent);

         case SI_YAXIS:
         case SI_RYAXIS:
            if (!negative)
            {
               return mFirstResponder->onGamepadAxisDown(mLastEvent);
            }
            else
            {
               return mFirstResponder->onGamepadAxisUp(mLastEvent);
            }

         case SI_XAXIS:
         case SI_RXAXIS:
         default:
            if (negative)
            {
               return mFirstResponder->onGamepadAxisLeft(mLastEvent);
            }
            else
            {
               return mFirstResponder->onGamepadAxisRight(mLastEvent);
            }
         }
      }
   }
   return false;
}

void GuiCanvasNew::rootMouseDown(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();
   mMouseButtonDown = true;

   //pass the event to the mouse locked control
   if (bool(mMouseCapturedControl))
      mMouseCapturedControl->onMouseDown(event);
   else
   {
      //else pass it to whoever is underneath the cursor
      iterator i;
      i = end();
      while (i != begin())
      {
         i--;
         GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
         GuiControlNew* controlHit = ctrl->findHitControl(event.mousePoint - ctrl->getPosition());

         // Deliver to the topmost layer's hit control unconditionally --
         // NOT gated on getCapturesInput(). That flag means "blocks
         // mouse input from reaching whatever's BEHIND this layer" (see
         // guiControlNew.h's mCapturesInput doc comment), which is a
         // real, correct thing to check when deciding whether iteration
         // should continue on to a LOWER layer -- but this was
         // (mis)using it as a gate on delivering the event to THIS,
         // the topmost, layer at all. mCapturesInput defaults to false
         // for every ordinary (non-modal-dialog) control -- a plain
         // content screen never sets it -- so this unconditionally
         // skipped delivering every click to ordinary content and fell
         // through to the next-lower layer (usually none), meaning
         // mouse clicks were silently never delivered anywhere. This is
         // the direct cause of "clicking the checkbox/text field does
         // nothing" -- findHitControl() (see its own recent fix in
         // guiControlNew.cpp for the identical root issue, one level
         // deeper in the recursion) could correctly identify the
         // checkbox/text field as the hit control, but this check threw
         // that correct answer away before ever calling onMouseDown() on
         // it.
         //
         // controlHit is only ever NULL if this layer's own mCanHit is
         // false with nothing hit beneath it either (see
         // findHitControl()'s final fallback) -- guard for that, then
         // always deliver and stop: whatever's topmost under the cursor
         // gets the click, exactly like every other GUI system's normal
         // (non-modal) hit-testing behaves. A genuinely modal dialog
         // layer still "wins" here simply by virtue of being iterated
         // first (topmost, since it was pushed later -- see
         // pushDialogControl()), without needing this check at all.
         if (controlHit)
         {
            controlHit->onMouseDown(event);
            break;
         }
      }
   }

   if (bool(mMouseControl))
      mMouseControlClicked = true;
}

void GuiCanvasNew::findMouseControl(const GuiEvent& event)
{
   // Any children at all?
   if (size() == 0)
   {
      mMouseControl = NULL;
      return;
   }

   // Otherwise, check the point and find the overlapped control.
   GuiControlNew* controlHit = findHitControl(event.mousePoint);
   if (controlHit != static_cast<GuiControlNew*>(mMouseControl))
   {
      if (bool(mMouseControl))
         mMouseControl->onMouseLeave(event);
      mMouseControl = controlHit;
      mMouseControl->onMouseEnter(event);
   }
}

void GuiCanvasNew::refreshMouseControl()
{
   GuiEvent evt;
   evt.mousePoint = deviceToLogicalPoint(Point2I(S32(mCursorPt.x), S32(mCursorPt.y)));
   findMouseControl(evt);
}

void GuiCanvasNew::checkLockMouseMove(const GuiEvent& event)
{
   GuiControlNew* controlHit = findHitControl(event.mousePoint);
   if (controlHit != mMouseControl)
   {
      if (mMouseControl == mMouseCapturedControl)
         mMouseCapturedControl->onMouseLeave(event);
      else if (controlHit == mMouseCapturedControl)
         mMouseCapturedControl->onMouseEnter(event);

      mMouseControl = controlHit;
   }
}

void GuiCanvasNew::rootMouseUp(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();
   mMouseButtonDown = false;

   // pass the event to the mouse locked control
   if (bool(mMouseCapturedControl))
   {
      checkLockMouseMove(event);
      mMouseCapturedControl->onMouseUp(event);
   }
   else
   {
      findMouseControl(event);
      if (bool(mMouseControl))
         mMouseControl->onMouseUp(event);
   }
}

void GuiCanvasNew::rootMouseDragged(const GuiEvent& event)
{
   //pass the event to the mouse locked control
   if (bool(mMouseCapturedControl))
   {
      checkLockMouseMove(event);
      mMouseCapturedControl->onMouseDragged(event);
   }
   else
   {
      findMouseControl(event);
      if (bool(mMouseControl))
         mMouseControl->onMouseDragged(event);
   }
}

void GuiCanvasNew::rootMouseMove(const GuiEvent& event)
{
   if (bool(mMouseCapturedControl))
   {
      mMouseCapturedControl->onMouseMove(event);
   }
   else
   {
      findMouseControl(event);
      if (bool(mMouseControl))
         mMouseControl->onMouseMove(event);
   }
}

void GuiCanvasNew::rootRightMouseDown(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();
   mMouseRightButtonDown = true;

   if (bool(mMouseCapturedControl))
   {
      checkLockMouseMove(event);
      mMouseCapturedControl->onRightMouseDown(event);
   }
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
      {
         mMouseControl->onRightMouseDown(event);
      }
   }
}

void GuiCanvasNew::rootRightMouseUp(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();
   mMouseRightButtonDown = false;

   if (bool(mMouseCapturedControl))
      mMouseCapturedControl->onRightMouseUp(event);
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
         mMouseControl->onRightMouseUp(event);
   }
}

void GuiCanvasNew::rootRightMouseDragged(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();

   if (bool(mMouseCapturedControl))
   {
      mMouseCapturedControl->onRightMouseDragged(event);
   }
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
         mMouseControl->onRightMouseDragged(event);
   }
}

void GuiCanvasNew::rootMiddleMouseDown(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();
   mMouseMiddleButtonDown = true;

   if (bool(mMouseCapturedControl))
      mMouseCapturedControl->onMiddleMouseDown(event);
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
      {
         mMouseControl->onMiddleMouseDown(event);
      }
   }
}

void GuiCanvasNew::rootMiddleMouseUp(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();
   mMouseMiddleButtonDown = false;

   if (bool(mMouseCapturedControl))
      mMouseCapturedControl->onMiddleMouseUp(event);
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
         mMouseControl->onMiddleMouseUp(event);
   }
}

void GuiCanvasNew::rootMiddleMouseDragged(const GuiEvent& event)
{
   mPrevMouseTime = Platform::getVirtualMilliseconds();

   if (bool(mMouseCapturedControl))
   {
      checkLockMouseMove(event);
      mMouseCapturedControl->onMiddleMouseDragged(event);
   }
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
         mMouseControl->onMiddleMouseDragged(event);
   }
}

bool GuiCanvasNew::rootMouseWheelUp(const GuiEvent& event)
{
   if (bool(mMouseCapturedControl))
      return mMouseCapturedControl->onMouseWheelUp(event);
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
         return mMouseControl->onMouseWheelUp(event);
   }

   return false;
}

bool GuiCanvasNew::rootMouseWheelDown(const GuiEvent& event)
{
   if (bool(mMouseCapturedControl))
      return mMouseCapturedControl->onMouseWheelDown(event);
   else
   {
      findMouseControl(event);

      if (bool(mMouseControl))
         return mMouseControl->onMouseWheelDown(event);
   }

   return false;
}

void GuiCanvasNew::setContentControl(GuiControlNew* gui)
{
   // Skip out if we got passed NULL (why would that happen?)
   if (!gui)
      return;

   GuiControlNew* oldContent = getContentControl();
   if (oldContent)
      Con::executef(oldContent, "onUnsetContent", Con::getIntArg(gui->getId()));

   //remove all dialogs on layer 0
   U32 index = 0;
   while (size() > index)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>((*this)[index]);
      if (ctrl == gui || ctrl->mLayer != 0)
         index++;

      Sim::getGuiGroup()->addObject(ctrl);
   }
#ifdef TORQUE_TOOLS
   // set current menu bar
   setMenuBar(mMenuBarCtrl);
#endif
   // lose the first responder from the old GUI
   GuiControlNew* responder = gui->findFirstTabable();
   if (responder)
      responder->setFirstResponder();

   // Sync this canvas's own bounds to the real platform window size (if
   // available) BEFORE addObject(gui) lets a width="100%"/height="100%"
   // content control resolve against it -- otherwise a content control set
   // before the first real maintainSizing() pass would resolve against
   // the constructor's placeholder bounds, and mDesignSize would
   // permanently capture that wrong size. getWindowSize() returns (-1,-1)
   // if no window is attached yet, matching maintainSizing()'s own guard.
   const Point2I realWindowSize = getWindowSize();
   if (realWindowSize.x != -1 && realWindowSize.y != -1)
      setBounds(RectI(0, 0, realWindowSize.x, realWindowSize.y));

   //add the gui to the front
   if (!size() || gui != (*this)[0])
   {
      // automatically wakes objects in GuiControlNew::onWake
      addObject(gui);
      if (size() >= 2)
         reOrder(gui, *begin());
   }

   // UPDATED: this explicit call used to be REQUIRED to force gui's
   // width="100%"-style fields to resolve against this canvas's real
   // extent before reading gui->getExtent() below -- see this file's
   // history for the original bug this worked around (a percent field set
   // before addObject() would otherwise sit unresolved against the
   // constructor's raw 64x64 placeholder, since resolveLayout() was never
   // called synchronously from addObject() itself).
   //
   // That's no longer the mechanism: GuiControlNew's width/height/left/
   // top/right/bottom/centerHorizontal/centerVertical setters now resolve
   // and apply to mBounds IMMEDIATELY at set-time, and any percent value
   // that couldn't resolve yet (no live parent at set-time) is recorded in
   // mPendingPercentAxes and resolved by addObject() itself, synchronously,
   // as soon as a live parent exists -- see GuiControlNew::addObject()'s
   // doc comment. By the time addObject(gui) returns above, gui->getExtent()
   // already reflects whatever its fields resolved to against this canvas's
   // real current extent; there is no remaining stale-64x64 window this
   // call was closing.
   //
   // resolveLayout() itself no longer does any field-driven sizing/
   // positioning at all (see GuiControlNew::resolveLayout()'s updated doc
   // comment) -- what's left there (containment clamp, aspect-ratio
   // preservation) is harmless to call again here, but no longer serves
   // the purpose this comment used to describe, so the call is removed
   // rather than kept as a no-longer-accurate safety net.

   // Capture this gui's design size -- the resolution it was authored
   mDesignSize = gui->getExtent();
   mDesignDPIScale = mPlatformWindow ? mPlatformWindow->getDPIScale() : 1.0f;

   //refresh the entire gui
   resetUpdateRegions();

   //rebuild the accelerator map
   mAcceleratorMap.clear();

   for (iterator i = end(); i != begin(); )
   {
      i--;
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      ctrl->buildAcceleratorMap();

      if (ctrl->getCapturesInput())
         break;
   }
   refreshMouseControl();

   // Force the canvas to update the sizing of the new content control
   maintainSizing();

   // Do this last so onWake gets called first
   Con::executef(gui, "onSetContent", Con::getIntArg(oldContent ? oldContent->getId() : 0));
}

GuiControlNew* GuiCanvasNew::getContentControl()
{
   if (size() > 0)
      return (GuiControlNew*)first();
   return NULL;
}

void GuiCanvasNew::pushDialogControl(GuiControlNew* gui, S32 layer, bool center)
{
   if (center)
      gui->setPosition(getExtent().x / 2 - gui->getExtent().x / 2,
         getExtent().y / 2 - gui->getExtent().y / 2);

   //add the gui
   gui->mLayer = layer;

   // GuiControlNew::addObject wakes the object
   addObject(gui);

   //reorder it to the correct layer 
   iterator i;
   for (i = begin(); i != end(); i++)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl->mLayer > gui->mLayer)
      {
         reOrder(gui, ctrl);
         break;
      }
   }

   //call the dialog push method
   gui->onDialogPush();

   //find the first responder
   GuiControlNew* responder = gui->findFirstTabable();
   if (responder)
      responder->setFirstResponder();

   // call the 'onWake' method?
   //if(wakedGui)
   //   Con::executef(gui, 1, "onWake");

   //refresh the entire gui
   resetUpdateRegions();

   //rebuild the accelerator map
   mAcceleratorMap.clear();
   if (size() > 0)
   {
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(last());
      ctrl->buildAcceleratorMap();
   }

   refreshMouseControl();

   // I don't see the purpose of this, and it's causing issues when showing, for instance the 
   //  metrics dialog while in a 3d scene, causing the cursor to be shown even when the mouse
   //  is locked [4/25/2007 justind]
   //if(gui->mProfile && gui->mProfile->mModal)
   //   mPlatformWindow->getCursorController()->pushCursor(PlatformCursorController::curArrow);
}

void GuiCanvasNew::popDialogControl(GuiControlNew* gui)
{
   if (size() < 1)
      return;

   //first, find the dialog, and call the "onDialogPop()" method
   GuiControlNew* ctrl = NULL;
   if (gui)
   {
      //make sure the gui really exists on the stack
      iterator i;
      bool found = false;
      for (i = begin(); i != end(); i++)
      {
         GuiControlNew* check = static_cast<GuiControlNew*>(*i);
         if (check == gui)
         {
            ctrl = check;
            found = true;
         }
      }

      if (!found)
         return;
   }
   else
      ctrl = static_cast<GuiControlNew*>(last());

   //call the "on pop" function
   ctrl->onDialogPop();

   //now pop the last child (will sleep if awake)
   Sim::getGuiGroup()->addObject(ctrl);

   if (size() > 0)
   {
      GuiControlNew* lastCtrl = static_cast<GuiControlNew*>(last());
      if (lastCtrl->getFirstResponder())
         lastCtrl->getFirstResponder()->setFirstResponder();
   }
   else
   {
      setFirstResponder(NULL);
   }

   //refresh the entire gui
   resetUpdateRegions();

   //rebuild the accelerator map
   mAcceleratorMap.clear();

   if (size() > 0)
   {
      GuiControlNew* lastCtrl = static_cast<GuiControlNew*>(last());
      lastCtrl->buildAcceleratorMap();
   }
   refreshMouseControl();
}

void GuiCanvasNew::popDialogControl(S32 layer)
{
   if (size() < 1)
      return;

   GuiControlNew* ctrl = NULL;
   iterator i = end(); // find in z order (last to first)
   while (i != begin())
   {
      i--;
      ctrl = static_cast<GuiControlNew*>(*i);
      if (ctrl->mLayer == layer)
         break;
   }
   if (ctrl)
      popDialogControl(ctrl);
}

void GuiCanvasNew::mouseLock(GuiControlNew* lockingControl)
{
   if (bool(mMouseCapturedControl))
      return;

   mMouseCapturedControl = lockingControl;

   if (mMouseControl && mMouseControl != mMouseCapturedControl)
   {
      GuiEvent evt;
      evt.mousePoint = deviceToLogicalPoint(Point2I(S32(mCursorPt.x), S32(mCursorPt.y)));

      mMouseControl->onMouseLeave(evt);
   }
}

void GuiCanvasNew::mouseUnlock(GuiControlNew* lockingControl)
{
   if (static_cast<GuiControlNew*>(mMouseCapturedControl) != lockingControl)
      return;

   GuiEvent evt;
   evt.mousePoint = deviceToLogicalPoint(Point2I(S32(mCursorPt.x), S32(mCursorPt.y)));

   GuiControlNew* controlHit = findHitControl(evt.mousePoint);
   if (controlHit != mMouseCapturedControl)
   {
      mMouseControl = controlHit;
      mMouseControlClicked = false;
      if (bool(mMouseControl))
         mMouseControl->onMouseEnter(evt);
   }
   mMouseCapturedControl = NULL;
}

void GuiCanvasNew::paint()
{
   resetUpdateRegions();

   // inhibit explicit refreshes in the case we're swapped out
   if (mPlatformWindow && mPlatformWindow->isVisible() && GFX->allowRender())
      mPlatformWindow->displayEvent.trigger(mPlatformWindow->getWindowId());
}

void GuiCanvasNew::repaint(U32 elapsedMS)
{
   // Make sure we have a window.
   if (!mPlatformWindow)
      return;

   // Has enough time elapsed?
   U32 elapsed = Platform::getRealMilliseconds() - mLastRenderMs;
   if (elapsed < elapsedMS)
      return;

   // Do the render.
   resetUpdateRegions();
   handlePaintEvent(mPlatformWindow->getWindowId());
}

void GuiCanvasNew::_updateEffectiveScale()
{
   const F32 currentDPIScale = mPlatformWindow ? mPlatformWindow->getDPIScale() : 1.0f;

   // No design size captured yet (no content control ever set) -- nothing
   // to scale against
   if (mDesignSize.x <= 0 || mDesignSize.y <= 0)
   {
      mEffectiveScaleX = 1.0f;
      mEffectiveScaleY = 1.0f;
      mContentDeviceRect = RectI(getPosition(), getExtent());
      return;
   }

   const Point2I windowSize = getExtent(); // canvas's own bounds are always raw window/device pixels
   const F32 scaleX = (F32)windowSize.x / (F32)mDesignSize.x;
   const F32 scaleY = (F32)windowSize.y / (F32)mDesignSize.y;

   // DPI correction: if this gui was designed on a monitor reporting a
   // different DPI scale than the current one
   const F32 dpiCorrection = (mDesignDPIScale > 0.0f) ? (currentDPIScale / mDesignDPIScale) : 1.0f;

   F32 finalScaleX = scaleX * dpiCorrection;
   F32 finalScaleY = scaleY * dpiCorrection;

   if (mLockAspectRatio)
   {
      // Uniform scale -- the more-constrained axis wins, so the content
      // control never overflows the window on either axis; the other
      // axis' leftover space becomes the letterbox/pillarbox bars.
      const F32 uniformScale = getMin(finalScaleX, finalScaleY);
      finalScaleX = finalScaleY = uniformScale;
   }

   mEffectiveScaleX = finalScaleX;
   mEffectiveScaleY = finalScaleY;

   const Point2I deviceExtent(
      (S32)(mDesignSize.x * finalScaleX),
      (S32)(mDesignSize.y * finalScaleY));

   const Point2I deviceOffset(
      (windowSize.x - deviceExtent.x) / 2,
      (windowSize.y - deviceExtent.y) / 2);

   mContentDeviceRect = RectI(deviceOffset, deviceExtent);
}

//-----------------------------------------------------------------------------

void GuiCanvasNew::maintainSizing()
{
   Point2I size = getWindowSize();

   if (size.x == -1 || size.y == -1)
      return;

   RectI screenRect(0, 0, size.x, size.y);

   // Reset dirty-rect tracking if the canvas's own extent didn't already
   // match the real window size (e.g. still seeded from the constructor
   // placeholder).
   const bool windowSizeChanged = (getExtent() != screenRect.extent);

   setBounds(screenRect);

   if (windowSizeChanged)
      resetUpdateRegions();

   // Canvas bounds are always the raw window rect; the content control's
   // LOGICAL extent stays pinned to mDesignSize regardless -- only the
   // derived device-pixel projection (mContentDeviceRect) tracks the
   // real window. This is what lets child layout fields resolve against
   // a stable reference size no matter the window's actual dimensions.
   const F32 prevEffectiveScaleX = mEffectiveScaleX;
   const F32 prevEffectiveScaleY = mEffectiveScaleY;

   _updateEffectiveScale();

   // A scale change alone (with the content control's own logical bounds
   // unchanged) still needs to mark descendants dirty, since resolveLayout()
   // reacts to a parent's resolved size, not just its own field changes.
   const bool effectiveScaleChanged =
      (mEffectiveScaleX != prevEffectiveScaleX) ||
      (mEffectiveScaleY != prevEffectiveScaleY);

   iterator i;
   for (i = begin(); i != end(); i++)
   {
      AssertFatal(static_cast<GuiControlNew*>((*i))->isAwake(), "GuiCanvasNew::maintainSizing - ctrl is not awake");
      GuiControlNew* ctrl = static_cast<GuiControlNew*>(*i);

      if (ctrl == getContentControl() && mDesignSize.x > 0 && mDesignSize.y > 0)
      {
         // Content control: logical size pinned to mDesignSize, position
         // to (0,0) -- getDeviceBounds() handles placing/scaling it into
         // the real window.
         Point2I designPos(0, 0);

         // Menu bar displacement is in DESIGN/logical units.
         Point2I designExt = mDesignSize;
         if (mMenuBarCtrl)
         {
            const U32 yOffset = (U32)mMenuBarCtrl->getExtent().y;
            designPos.y += yOffset;
            designExt.y -= yOffset;
         }

         // Reads raw, non-resolving bounds here -- using the resolving
         // accessors would let resolveLayout()'s containment clamp run
         // against the canvas's raw window extent (the wrong reference
         // frame) before the resize() below can re-pin this control back
         // to its correct design-size box.
         const RectI rawBounds = ctrl->getRawBounds();
         if (rawBounds.point != designPos || rawBounds.extent != designExt)
         {
            ctrl->resize(designPos, designExt);
            resetUpdateRegions();
         }
         else if (effectiveScaleChanged)
         {
            // Content control's own box didn't move, but every descendant's
            // containment clamp still depends on parent size, regardless of
            // whether its own fields are percent or fixed-pixel -- give
            // every descendant a chance to re-resolve.
            iterator ci;
            for (ci = ctrl->begin(); ci != ctrl->end(); ci++)
               static_cast<GuiControlNew*>(*ci)->markLayoutDirty();

            resetUpdateRegions();
         }

         continue;
      }

      // Dialogs (and the content control before any design size has been
      // captured) keep the legacy behavior of being force-stretched to
      // the full raw window rect -- dialogs don't currently participate
      // in the design-size system; see gui-migration-plan.md for this as
      // a tracked follow-up once dialog behavior under the new model is
      // designed.
      Point2I ext = ctrl->getExtent();
      Point2I pos = ctrl->getPosition();
      Point2I newExt = screenRect.extent;
      Point2I newPos = screenRect.point;

      if (pos != newPos || ext != newExt)
      {
         ctrl->resize(newPos, newExt);
         resetUpdateRegions();
      }
   }
}

void GuiCanvasNew::setupFences()
{
   // Destroy old fences
   SAFE_DELETE_ARRAY(mFences);

   // Now create the new ones
   if (mNumFences > 0)
   {
      mFences = new GFXFence * [mNumFences];

      // Allocate the new fences
      for (S32 i = 0; i < mNumFences; i++)
         mFences[i] = GFX->createFence();
   }

   // Reset state
   mNextFenceIdx = 0;
}

void GuiCanvasNew::constrainMouseCoords(Point2I mousePoint)
{
   if (mConstrainMouse == false) return;
   Point2I windowPos = getPlatformWindow()->getPosition();//this is the offset
   Point2I winSize = getWindowSize();//window size too!

   S32 newDisplay = getPlatformWindow()->getCurrentMonitorIndex();
   SDL_DisplayMode displayM;
   if (0 == SDL_GetDesktopDisplayMode(newDisplay, &displayM))
   {
      S32 width = displayM.w;
      S32 height = displayM.h;

      if (winSize.x < width || winSize.y < height)
      {
         //we must be windowed
         //find the diference and half it
         S32 offX = (width - winSize.x) * 0.5f;
         S32 offY = (height - winSize.y) * 0.5f;
         S32 maxX = winSize.x + offX;
         S32 maxY = winSize.y + offY;

         Point2I newPos; //using 8px as a safety margin
         newPos.x = mClamp(mousePoint.x, 0, maxX);
         newPos.y = mClamp(mousePoint.y, 0, maxY);
         setCursorPos(windowPos + newPos);
      }
   }
}

void GuiCanvasNew::renderFrame(bool preRenderOnly, bool bufferSwap /* = true */)
{
   AssertISV(mPlatformWindow, "GuiCanvasNew::renderFrame - no window present!");
   if (!mPlatformWindow->isVisible() || !GFX->allowRender() || GFX->canCurrentlyRender())
      return;

   PROFILE_START(CanvasPreRender);

   // Set our window as the current render target so we can see outputs.
   GFX->setActiveRenderTarget(mPlatformWindow->getGFXTarget());

   if (!GFX->getActiveRenderTarget())
   {
      PROFILE_END();
      return;
   }

#ifdef TORQUE_GFX_STATE_DEBUG
   GFX->getDebugStateManager()->startFrame();
#endif

   GFXTarget* renderTarget = GFX->getActiveRenderTarget();
   if (renderTarget == NULL)
   {
      PROFILE_END();
      return;
   }

   // Make sure the root control is the size of the canvas.
   Point2I size = renderTarget->getSize();

   if (size.x == 0 || size.y == 0)
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
   if (preRenderOnly)
      return;

   // Signal the interested parties.
   GuiCanvasNew::getGuiCanvasFrameSignal().trigger(true);

   // Gross hack to make sure we don't end up with advanced lighting and msaa 
   // at the same time, which causes artifacts. At the same time we don't 
   // want to just throw the settings the user has chosen if the light manager 
   // changes at a later time.

   GFXVideoMode mode = mPlatformWindow->getVideoMode();
   if (dStricmp(LIGHTMGR->getId(), "ADVLM") == 0 && mode.antialiasLevel > 0)
   {
      const char* pref = Con::getVariable("$pref::Video::mode");
      mode.parseFromString(pref);
      mode.antialiasLevel = 0;
      Point2I winPos = mPlatformWindow->getPosition(); // Save position so we can put window back.
      mPlatformWindow->setVideoMode(mode);
      // setVideoMode (above) will center the window on the display device. If the window had been positioned
      // by the user or from script, put it back where it was before the light manager change.
      mPlatformWindow->setPosition(winPos);

      Con::printf("AntiAliasing has been disabled; it is not compatible with AdvancedLighting.");
   }
   else if (dStricmp(LIGHTMGR->getId(), "BLM") == 0)
   {
      const char* pref = Con::getVariable("$pref::Video::mode");

      U32 prefAA = dAtoi(StringUnit::getUnit(pref, 5, " "));
      if (prefAA != mode.antialiasLevel)
      {
         mode.parseFromString(pref);
         Point2I winPos = mPlatformWindow->getPosition(); // Save position so we can put window back.
         mPlatformWindow->setVideoMode(mode);
         // setVideoMode (above) will center the window on the display device. If the window had been positioned
         // by the user or from script, put it back where it was before the light manager change.
         mPlatformWindow->setPosition(winPos);

         Con::printf("AntiAliasing has been enabled while running BasicLighting.");
      }
   }

   // for now, just always reset the update regions - this is a
   // fix for FSAA on ATI cards
   resetUpdateRegions();

   PROFILE_START(CanvasRenderControls);

   // Draw the mouse
   GuiCursor* mouseCursor = NULL;
   bool cursorVisible = true;

   if (bool(mMouseCapturedControl))
      mMouseCapturedControl->getCursor(mouseCursor, cursorVisible, mLastEvent);
   else if (bool(mMouseControl))
      mMouseControl->getCursor(mouseCursor, cursorVisible, mLastEvent);

   Point2I cursorPos((S32)mCursorPt.x, (S32)mCursorPt.y);
   if (!mouseCursor)
      mouseCursor = mDefaultCursor;

   if (mLastCursorEnabled && mLastCursor)
   {
      Point2I spot = mLastCursor->getHotSpot();
      Point2I cext = mLastCursor->getExtent();
      Point2I pos = mLastCursorPt - spot;
      addUpdateRegion(pos - Point2I(2, 2), Point2I(cext.x + 4, cext.y + 4));
   }

   if (cursorVisible && mouseCursor)
   {
      Point2I spot = mouseCursor->getHotSpot();
      Point2I cext = mouseCursor->getExtent();
      Point2I pos = cursorPos - spot;

      addUpdateRegion(pos - Point2I(2, 2), Point2I(cext.x + 4, cext.y + 4));
   }
   constrainMouseCoords(cursorPos);

   mLastCursorEnabled = cursorVisible;
   mLastCursor = mouseCursor;
   mLastCursorPt = cursorPos;

   // Begin GFX
   PROFILE_START(GFXBeginScene);

   bool beginSceneRes = GFX->beginScene();

   PROFILE_END();

   // Render all offscreen canvas objects here since we may need them in the render loop
   if (GuiOffscreenCanvasNew::sList.size() != 0)
   {
      // Reset the entire state since oculus shit will have barfed it.

      GFX->updateStates(true);

      for (Vector<GuiOffscreenCanvasNew*>::iterator itr = GuiOffscreenCanvasNew::sList.begin(); itr != GuiOffscreenCanvasNew::sList.end(); itr++)
      {
         (*itr)->renderFrame(false, false);
      }

      GFX->setActiveRenderTarget(renderTarget);
   }

   // Can't render if waiting for device to reset.   
   if (!beginSceneRes)
   {
      PROFILE_END(); // CanvasRenderControls

      // Since we already triggered the signal once for begin-of-frame,
      // we should be consistent and trigger it again for end-of-frame.
      GuiCanvasNew::getGuiCanvasFrameSignal().trigger(false);

      return;
   }

   // Clear the current viewport area
   GFX->setViewport(screenRect);
   GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, gNewCanvasClearColor, 0.0f, 0);

   resetUpdateRegions();

   // Make sure we have a clean matrix state 
   // before we start rendering anything!   
   GFX->setWorldMatrix(MatrixF::Identity);
   GFX->setViewMatrix(MatrixF::Identity);
   GFX->setProjectionMatrix(MatrixF::Identity);

   // If we're taking a screenshot then let it have
   // a chance at altering the view matrix.
   if (gScreenShot && gScreenShot->isPending())
      gScreenShot->tileGui(size);

   RectI updateUnion;
   buildUpdateUnion(&updateUnion);
   if (updateUnion.intersect(screenRect))
   {
      // Start a fresh batch for this frame -- every control's onRender()
      // below submits its draw primitives (quads/lines/text) into this
      // instead of calling GFXDrawUtil directly (see guiRenderBatch.h).
      // One begin()/flush() pair per frame, wrapping the WHOLE dialog-
      // stack render pass below, is what collapses however many
      // controls exist in the tree down to a small, roughly-fixed number
      // of draw calls (gui-rewrite-design.md §7.1's actual payoff) rather
      // than one per control.
      mRenderBatch.begin(updateUnion);

      // Render active GUI Dialogs
      for (iterator i = begin(); i != end(); i++)
      {
         // Get the control
         GuiControlNew* contentCtrl = static_cast<GuiControlNew*>(*i);

         GFX->setClipRect(updateUnion);
         GFX->setStateBlock(mDefaultGuiSB);

         // onRender()'s 'offset' parameter is DEVICE-pixel space (see
         // GuiControlNew::onRender()/renderChildControls() doc comments) --
         // getDeviceBounds().point is this control's absolute device
         // position (for the content control, this is
         // getContentDeviceRect().point; for a dialog, which isn't
         // design-scaled, its device bounds mirror its logical bounds).
         contentCtrl->onRender(contentCtrl->getDeviceBounds().point, updateUnion);
      }

      // FIX: tooltip rendering moved to HERE, before mRenderBatch.flush()
      // below -- it was previously AFTER the flush call, which is a real
      // bug: defaultTooltipRender() (see guiControlNew.cpp's own doc
      // comment on it: "Every draw below goes through the canvas's
      // per-frame batch ... instead of GFXDrawUtil") submits its
      // quads/text via the EXACT SAME mRenderBatch every other control's
      // onRender() uses, via root->getRenderBatch() -- it has no
      // separate/immediate-mode draw path of its own. Calling it after
      // flush() had already run meant every tooltip quad/text got queued
      // into a batch that had just been drained (flush() clears mQuads/
      // mTextRuns -- see guiRenderBatch.cpp) and would only be cleared
      // again, unflushed, by next frame's begin() -- so a tooltip was
      // built and queued correctly every single frame, but its draw
      // calls never actually reached the GPU. Moving this block above
      // the flush() call means its output rides along in the SAME single
      // flush pass as the rest of the frame's GUI content, matching this
      // function's own documented "one begin()/flush() pair per frame"
      // design instead of needing a second, separate flush call.
      if (bool(mMouseControl))
      {
         U32 curTime = Platform::getRealMilliseconds();
         if (mHoverControl == mMouseControl)
         {
            if (mHoverPositionSet || (curTime - mHoverControlStart) >= mHoverControl->mTipHoverTime || (curTime - mHoverLeftControlTime) <= mHoverControl->mTipHoverTime)
            {
               if (!mHoverPositionSet)
               {
                  mHoverPosition = cursorPos;
               }
               mHoverPositionSet = mMouseControl->mRenderTooltipDelegate(mHoverPosition, cursorPos, NULL);
            }

         }
         else
         {
            if (mHoverPositionSet)
            {
               mHoverLeftControlTime = curTime;
               mHoverPositionSet = false;
            }
            mHoverControl = mMouseControl;
            mHoverControlStart = curTime;
         }
      }

      // Every onRender() call above (and the tooltip delegate just
      // above, if it fired) only QUEUED primitives into mRenderBatch --
      // nothing has actually reached the GPU yet for the GUI tree
      // itself. Flush once, here, after the whole dialog stack (and any
      // tooltip) has had a chance to submit.
      mRenderBatch.flush(GFX);

      // Fill Black if no Dialogs
      if (this->size() == 0)
         GFX->clear(GFXClearTarget, ColorI(0, 0, 0, 0), 1.0f, 0);

      GFX->setClipRect(updateUnion);

      // Draw an ugly box if we don't have a cursor available...
      //if (mCursorEnabled && mShowCursor && !mouseCursor)
      //{
      //   GFX->drawRectFill( RectI( mCursorPt.x, mCursorPt.y, mCursorPt.x + 2, mCursorPt.y + 2 ), ColorI( 255, 0, 0 ) );
      //}


      // CodeReview - Make sure our bitmap modulation is clear or else there's a black modulation
      // that ruins rendering of textures at startup.. This was done in mouseCursor 
      // onRender and so at startup when it wasn't called the modulation was black, ruining
      // the loading screen display. This fixes the issue, but is it only masking a deeper issue
      // in GFX with regard to gui rendering? [5/3/2007 justind]
      GFX->getDrawUtil()->clearBitmapModulation();

      // Really draw the cursor. :)
      // Only if the platform cursor controller is missing or the platform cursor
      // isn't visible.
      if (!mPlatformWindow->getCursorController() || (mCursorEnabled && mouseCursor && mShowCursor &&
         !mPlatformWindow->getCursorController()->isCursorVisible()))
      {
         Point2I pos((S32)mCursorPt.x, (S32)mCursorPt.y);
         Point2I spot = mouseCursor->getHotSpot();

         pos -= spot;
         mouseCursor->render(pos);
      }
   }

   // Render all RTT end of frame updates HERE
   //DynamicTexture::updateScreenTextures();
   //DynamicTexture::updateEndOfFrameTextures();
   // mPending is set when the console function "screenShot()" is called
   // this situation is necessary because it needs to take the screenshot
   // before the buffers swap

   PROFILE_END();

   // Fence logic here, because this is where endScene is called.
   if (mNumFences > 0)
   {
      // Issue next fence
      mFences[mNextFenceIdx]->issue();

      mNextFenceIdx++;

      // Wrap the next fence around to first if we're maxxed
      if (mNextFenceIdx >= mNumFences)
         mNextFenceIdx = 0;

      // Block on previous fence
      mFences[mNextFenceIdx]->block();
   }

   PROFILE_START(GFXEndScene);
   GFX->endScene();
   PROFILE_END();

   GFX->getDeviceEventSignal().trigger(GFXDevice::dePostFrame);
   swapBuffers();

   GuiCanvasNew::getGuiCanvasFrameSignal().trigger(false);

#ifdef TORQUE_GFX_STATE_DEBUG
   GFX->getDebugStateManager()->endFrame();
#endif

   // Keep track of the last time we rendered.
   mLastRenderMs = Platform::getRealMilliseconds();
}

GuiCanvasNew::GuiCanvasFrameSignal& GuiCanvasNew::getGuiCanvasFrameSignal()
{
   static GuiCanvasFrameSignal theSignal;
   return theSignal;
}

void GuiCanvasNew::swapBuffers()
{
   AssertISV(mPlatformWindow, "GuiCanvasNew::swapBuffers - no window present!");
   if (!mPlatformWindow->isVisible())
      return;

   PROFILE_START(SwapBuffers);
   mPlatformWindow->getGFXTarget()->present();
   PROFILE_END();
}

void GuiCanvasNew::buildUpdateUnion(RectI* updateUnion)
{
   *updateUnion = mOldUpdateRects[0];

   //the update region should encompass the oldUpdateRects, and the curUpdateRect
   Point2I upperL;
   Point2I lowerR;

   upperL.x = getMin(mOldUpdateRects[0].point.x, mOldUpdateRects[1].point.x);
   upperL.x = getMin(upperL.x, mCurUpdateRect.point.x);

   upperL.y = getMin(mOldUpdateRects[0].point.y, mOldUpdateRects[1].point.y);
   upperL.y = getMin(upperL.y, mCurUpdateRect.point.y);

   lowerR.x = getMax(mOldUpdateRects[0].point.x + mOldUpdateRects[0].extent.x, mOldUpdateRects[1].point.x + mOldUpdateRects[1].extent.x);
   lowerR.x = getMax(lowerR.x, mCurUpdateRect.point.x + mCurUpdateRect.extent.x);

   lowerR.y = getMax(mOldUpdateRects[0].point.y + mOldUpdateRects[0].extent.y, mOldUpdateRects[1].point.y + mOldUpdateRects[1].extent.y);
   lowerR.y = getMax(lowerR.y, mCurUpdateRect.point.y + mCurUpdateRect.extent.y);

   updateUnion->point = upperL;
   updateUnion->extent = lowerR - upperL;

   //shift the oldUpdateRects
   mOldUpdateRects[0] = mOldUpdateRects[1];
   mOldUpdateRects[1] = mCurUpdateRect;

   mCurUpdateRect.point.set(0, 0);
   mCurUpdateRect.extent.set(0, 0);
}

void GuiCanvasNew::addUpdateRegion(Point2I pos, Point2I ext)
{
   if (mCurUpdateRect.extent.x == 0)
   {
      mCurUpdateRect.point = pos;
      mCurUpdateRect.extent = ext;
   }
   else
   {
      Point2I upperL;
      upperL.x = getMin(mCurUpdateRect.point.x, pos.x);
      upperL.y = getMin(mCurUpdateRect.point.y, pos.y);
      Point2I lowerR;
      lowerR.x = getMax(mCurUpdateRect.point.x + mCurUpdateRect.extent.x, pos.x + ext.x);
      lowerR.y = getMax(mCurUpdateRect.point.y + mCurUpdateRect.extent.y, pos.y + ext.y);
      mCurUpdateRect.point = upperL;
      mCurUpdateRect.extent = lowerR - upperL;
   }
}

void GuiCanvasNew::resetUpdateRegions()
{
   //DEBUG - get surface width and height
   mOldUpdateRects[0] = getBounds();
   mOldUpdateRects[1] = mOldUpdateRects[0];
   mCurUpdateRect = mOldUpdateRects[0];
}

void GuiCanvasNew::setFirstResponder(GuiControlNew* newResponder)
{
   GuiControlNew* oldResponder = mFirstResponder;
   Parent::setFirstResponder(newResponder);

   if (oldResponder == mFirstResponder)
      return;

   if (oldResponder && (oldResponder != newResponder))
      oldResponder->onLoseFirstResponder();

   if (newResponder && (newResponder != oldResponder) && newResponder->isProperlyAdded())
      newResponder->onGainFirstResponder();
}

StringTableEntry GuiCanvasNew::getLastInputDeviceType()
{
   switch (mLastInputDeviceType)
   {
   case KeyboardDeviceType:
      return StringTable->insert("Keyboard");
      break;

   case GamepadDeviceType:
      return StringTable->insert("Gamepad");
      break;

   case MouseDeviceType:
      return StringTable->insert("Mouse");
      break;
   }

   return StringTable->EmptyString();
}

void GuiCanvasNew::setActive(bool value)
{
   Parent::setActive(value);

   GuiCanvasNew::getCanvasSetActiveSignal().trigger(this, value);
}

DefineEngineMethod(GuiCanvasNew, getContent, S32, (), ,
   "@brief Get the GuiControlNew which is being used as the content.\n\n"

   "@tsexample\n"
   "Canvas.getContent();\n"
   "@endtsexample\n\n"

   "@return ID of current content control")
{
   GuiControlNew* ctrl = object->getContentControl();
   if (ctrl)
      return ctrl->getId();
   return -1;
}

DefineEngineMethod(GuiCanvasNew, setContent, void, (GuiControlNew* ctrl), ,
   "@brief Set the content of the canvas to a specified control.\n\n"

   "@param ctrl ID or name of GuiControlNew to set content to\n\n"

   "@tsexample\n"
   "Canvas.setContent(PlayGui);\n"
   "@endtsexample\n\n")
{
   // Not using old error reporting until we modify the engineAPI - mperry

   //GuiControlNew *gui = NULL;
 //  if(argv[2][0])
 //  {
 //     if (!Sim::findObject(argv[2], gui))
 //     {
 //        Con::printf("%s(): Invalid control: %s", argv[0], argv[2]);
 //        return;
 //     }
 //  }

   if (!ctrl)
   {
      Con::errorf("GuiCanvasNew::setContent - Invalid control specified')");
      return;
   }

   //set the new content control
   object->setContentControl(ctrl);
}

//ConsoleDocFragment _pushDialog(
//   "@brief Adds a dialog control onto the stack of dialogs\n\n"
//   "@param ctrl Dialog to add\n"
//   "@param layer Layer to put dialog on (optional)\n"
//   "@param center True to center dialog on canvas (optional)\n\n"
//   "@tsexample\n"
//   "Canvas.pushDialog(RecordingsDlg);\n"
//   "@endtsexample\n\n",
//   "GuiCanvasNew",
//   "void pushDialog( GuiControlNew ctrl, int layer=0, bool center=false);"
//);

DefineEngineMethod(GuiCanvasNew, pushDialog, void, (const char* ctrlName, S32 layer, bool center), (0, false), "(GuiControlNew ctrl, int layer=0, bool center=false)"
   "@hide")
{
   GuiControlNew* gui;

   if (!Sim::findObject(ctrlName, gui))
   {
      Con::printf("pushDialog(): Invalid control: %s", ctrlName);
      return;
   }

   //find the layer

   //set the new content control
   object->pushDialogControl(gui, layer, center);
}
//
//ConsoleDocFragment _popDialog1(
//   "@brief Removes a specific dialog control\n\n"
//   "@param ctrl Dialog to pop\n"
//   "@tsexample\n"
//   "Canvas.popDialog(RecordingsDlg);\n"
//   "@endtsexample\n\n",
//   "GuiCanvasNew",
//   "void popDialog( GuiControlNew ctrl);"
//);
//
//ConsoleDocFragment _popDialog2(
//   "@brief Removes a dialog at the front most layer\n\n"
//   "@tsexample\n"
//   "// Pops whatever is on layer 0\n"
//   "Canvas.popDialog();\n"
//   "@endtsexample\n\n",
//   "GuiCanvasNew",
//   "void popDialog();"
//);

DefineEngineMethod(GuiCanvasNew, popDialog, void, (GuiControlNew* gui), (nullAsType<GuiControlNew*>()), "(GuiControlNew ctrl=NULL)"
   "@hide")
{
   if (gui)
      object->popDialogControl(gui);
   else
      object->popDialogControl();
}

//ConsoleDocFragment _popLayer1(
//   "@brief Removes the top most layer of dialogs\n\n"
//   "@tsexample\n"
//   "Canvas.popLayer();\n"
//   "@endtsexample\n\n",
//   "GuiCanvasNew",
//   "void popLayer();"
//);
//
//ConsoleDocFragment _popLayer2(
//   "@brief Removes a specified layer of dialogs\n\n"
//   "@param layer Number of the layer to pop\n\n"
//   "@tsexample\n"
//   "Canvas.popLayer(1);\n"
//   "@endtsexample\n\n",
//   "GuiCanvasNew",
//   "void popLayer(S32 layer);"
//);

DefineEngineMethod(GuiCanvasNew, popLayer, void, (S32 layer), (0), "(int layer)"
   "@hide")
{

   object->popDialogControl(layer);
}

DefineEngineMethod(GuiCanvasNew, cursorOn, void, (), ,
   "@brief Turns on the mouse cursor.\n\n"
   "@tsexample\n"
   "Canvas.cursorOn();\n"
   "@endtsexample\n\n")
{
   object->setCursorON(true);
}

DefineEngineMethod(GuiCanvasNew, cursorOff, void, (), ,
   "@brief Turns on the mouse off.\n\n"
   "@tsexample\n"
   "Canvas.cursorOff();\n"
   "@endtsexample\n\n")
{
   object->setCursorON(false);
}


DefineEngineMethod(GuiCanvasNew, setCursor, void, (GuiCursor* cursor), ,
   "@brief Sets the cursor for the canvas.\n\n"

   "@param cursor Name of the GuiCursor to use\n\n"

   "@tsexample\n"
   "Canvas.setCursor(\"DefaultCursor\");\n"
   "@endtsexample\n\n")
{
   if (!cursor)
   {
      Con::errorf("GuiCanvasNew::setCursor - Invalid GuiCursor name or ID");
      return;
   }
   object->setCursor(cursor);
}

DefineEngineMethod(GuiCanvasNew, renderFront, void, (bool enable), ,
   "@brief This turns on/off front-buffer rendering.\n\n"

   "@param enable True if all rendering should be done to the front buffer\n\n"

   "@tsexample\n"
   "Canvas.renderFront(false);\n"
   "@endtsexample\n\n")
{
   object->setRenderFront(enable);
}

DefineEngineMethod(GuiCanvasNew, showCursor, void, (), ,
   "@brief Enable rendering of the cursor.\n\n"

   "@tsexample\n"
   "Canvas.showCursor();\n"
   "@endtsexample\n\n")
{
   object->showCursor(true);
}

DefineEngineMethod(GuiCanvasNew, hideCursor, void, (), ,
   "@brief Disable rendering of the cursor.\n\n"

   "@tsexample\n"
   "Canvas.hideCursor();\n"
   "@endtsexample\n\n")
{
   object->showCursor(false);
}

DefineEngineMethod(GuiCanvasNew, isCursorOn, bool, (), ,
   "@brief Determines if mouse cursor is enabled.\n\n"

   "@tsexample\n"
   "// Is cursor on?\n"
   "if(Canvas.isCursorOn())\n"
   "  echo(\"Canvas cursor is on\");\n"
   "@endtsexample\n\n"
   "@return Returns true if the cursor is on.\n\n")
{
   return object->isCursorON();
}

DefineEngineMethod(GuiCanvasNew, isCursorShown, bool, (), ,
   "@brief Determines if mouse cursor is rendering.\n\n"

   "@tsexample\n"
   "// Is cursor rendering?\n"
   "if(Canvas.isCursorShown())\n"
   "  echo(\"Canvas cursor is rendering\");\n"
   "@endtsexample\n\n"
   "@return Returns true if the cursor is rendering.\n\n")
{
   return object->isCursorShown();
}

DefineEngineMethod(GuiCanvasNew, repaint, void, (S32 elapsedMS), (0),
   "@brief Force canvas to redraw.\n"
   "If the elapsed time is greater than the time since the last paint "
   "then the repaint will be skipped.\n"
   "@param elapsedMS The optional elapsed time in milliseconds.\n\n"

   "@tsexample\n"
   "Canvas.repaint();\n"
   "@endtsexample\n\n")
{
   object->repaint(elapsedMS < 0 ? 0 : elapsedMS);
}

DefineEngineMethod(GuiCanvasNew, reset, void, (), ,
   "@brief Reset the update regions for the canvas.\n\n"

   "@tsexample\n"
   "Canvas.reset();\n"
   "@endtsexample\n\n")
{
   object->resetUpdateRegions();
}

DefineEngineMethod(GuiCanvasNew, getCursorPos, Point2I, (), ,
   "@brief Get the current position of the cursor in screen-space. Note that this position"
   " might be outside the Torque window. If you want to get the position within the Canvas,"
   " call screenToClient on the result.\n\n"
   "@see Canvas::screenToClient()\n\n"
   "@param param Description\n\n"
   "@tsexample\n"
   "%cursorPos = Canvas.getCursorPos();\n"
   "@endtsexample\n\n"
   "@return Screen coordinates of mouse cursor, in format \"X Y\"")
{
   return object->getCursorPos();
}
//
//ConsoleDocFragment _setCursorPos1(
//   "@brief Sets the position of the cursor\n\n"
//   "@param pos Point, in screenspace for the cursor. Formatted as (\"x y\")\n\n"
//   "@tsexample\n"
//   "Canvas.setCursorPos(\"0 0\");\n"
//   "@endtsexample\n\n",
//   "GuiCanvasNew",
//   "bool setCursorPos( Point2I pos );"
//);
//ConsoleDocFragment _setCursorPos2(
//   "@brief Sets the position of the cursor\n\n"
//   "@param posX X-coordinate, in screenspace for the cursor.\n"
//   "@param posY Y-coordinate, in screenspace for the cursor.\n\n"
//   "@tsexample\n"
//   "Canvas.setCursorPos(0,0);\n"
//   "@endtsexample\n\n",
//   "GuiCanvasNew",
//   "bool setCursorPos( F32 posX, F32 posY);"
//);

DefineEngineMethod(GuiCanvasNew, setCursorPos, void, (Point2I pos), , "(Point2I pos)"
   "@hide")
{

   object->setCursorPos(pos);
}

DefineEngineMethod(GuiCanvasNew, getMouseControl, S32, (), ,
   "@brief Gets the gui control under the mouse.\n\n"
   "@tsexample\n"
   "%underMouse = Canvas.getMouseControl();\n"
   "@endtsexample\n\n"

   "@return ID of the gui control, if one was found. NULL otherwise")
{
   GuiControlNew* control = object->getMouseControl();
   if (control)
      return control->getId();

   return 0;
}

DefineEngineFunction(excludeOtherInstanceNew, bool, (const char* appIdentifer), ,
   "@brief Used to exclude/prevent all other instances using the same identifier specified\n\n"

   "@note Not used on OSX, Xbox, or in Win debug builds\n\n"

   "@param appIdentifier Name of the app set up for exclusive use.\n"

   "@return False if another app is running that specified the same appIdentifier\n\n"

   "@ingroup Platform\n"
   "@ingroup GuiCore")
{
   // mac can only run one instance in general.
#if !defined(TORQUE_OS_MAC) && !defined(TORQUE_DEBUG) && !defined(TORQUE_OS_LINUX)
   return Platform::excludeOtherInstances(appIdentifer);
#else
   // We can just return true if we get here.
   return true;
#endif
}

DefineEngineMethod(GuiCanvasNew, getExtent, Point2I, (), ,
   "@brief Returns the dimensions of the canvas\n\n"

   "@tsexample\n"
   "%extent = Canvas.getExtent();\n"
   "@endtsexample\n\n"

   "@return Width and height of canvas. Formatted as numerical values in a single string \"# #\"")
{
   return object->getExtent();
}

void GuiCanvasNew::dumpScaleDebug()
{
   Con::printf("GuiCanvasNew scale debug:");
   Con::printf("  canvas extent (raw window): %d %d", getExtent().x, getExtent().y);
   Con::printf("  designSize: %d %d", getDesignSize().x, getDesignSize().y);
   Con::printf("  lockAspectRatio: %d", getLockAspectRatio() ? 1 : 0);
   Con::printf("  effectiveScaleX: %f", getEffectiveScaleX());
   Con::printf("  effectiveScaleY: %f", getEffectiveScaleY());
   const RectI& cdr = getContentDeviceRect();
   Con::printf("  contentDeviceRect: pos(%d %d) ext(%d %d)", cdr.point.x, cdr.point.y, cdr.extent.x, cdr.extent.y);

   GuiControlNew* content = getContentControl();
   if (content)
   {
      Con::printf("  content control logical: pos(%d %d) ext(%d %d)",
         content->getPosition().x, content->getPosition().y,
         content->getExtent().x, content->getExtent().y);
      const RectI db = content->getDeviceBounds();
      Con::printf("  content control device:   pos(%d %d) ext(%d %d)",
         db.point.x, db.point.y, db.extent.x, db.extent.y);
   }
   else
   {
      Con::printf("  content control: NULL");
   }
}

//-----------------------------------------------------------------------------

DefineEngineMethod(GuiCanvasNew, dumpScaleDebug, void, (), ,
   "@brief Diagnostic utility -- prints the canvas's live design-size/scale state to the console.")
{
   object->dumpScaleDebug();
}


DefineEngineMethod(GuiCanvasNew, setWindowTitle, void, (const char* newTitle), ,
   "@brief Change the title of the OS window.\n\n"

   "@param newTitle String containing the new name\n\n"

   "@tsexample\n"
   "Canvas.setWindowTitle(\"Documentation Rocks!\");\n"
   "@endtsexample\n\n")
{
   object->setWindowTitle(newTitle);
}


DefineEngineMethod(GuiCanvasNew, findFirstMatchingMonitor, S32, (const char* name), ,
   "@brief Find the first monitor index that matches the given name.\n\n"
   "The actual match algorithm depends on the implementation.\n"
   "@param name The name to search for.\n\n"
   "@return The number of monitors attached to the system, including the default monoitor.")
{
   return PlatformWindowManager::get()->findFirstMatchingMonitor(name);
}

DefineEngineMethod(GuiCanvasNew, getMonitorCount, S32, (), ,
   "@brief Gets the number of monitors attached to the system.\n\n"

   "@return The number of monitors attached to the system, including the default monoitor.")
{
   return PlatformWindowManager::get()->getMonitorCount();
}

DefineEngineMethod(GuiCanvasNew, getMonitorName, const char*, (S32 index), ,
   "@brief Gets the name of the requested monitor.\n\n"
   "@param index The monitor index.\n\n"
   "@return The name of the requested monitor.")
{
   return PlatformWindowManager::get()->getMonitorName(index);
}

DefineEngineMethod(GuiCanvasNew, getMonitorRect, RectI, (S32 index), ,
   "@brief Gets the region of the requested monitor.\n\n"
   "@param index The monitor index.\n\n"
   "@return The rectangular region of the requested monitor.")
{
   return PlatformWindowManager::get()->getMonitorRect(index);
}

DefineEngineMethod(GuiCanvasNew, getMonitorUsableRect, RectI, (S32 index), ,
   "@brief Use this function to get the usable desktop area represented by a display, with the primary display located at 0,0.\n\n"
   "This is the same area as Canvas.getMonitorRect() reports, but with portions reserved by the system removed. "
   "For example, on Apple Mac OS X, this subtracts the area occupied by the menu bar and dock.\n"
   "Setting a window to be fullscreen generally bypasses these unusable areas, so these are good guidelines for "
   "the maximum space available to a non - fullscreen window."
   "@param index The monitor index.\n\n"
   "@return The rectangular region of the requested monitor.")
{
   return PlatformWindowManager::get()->getMonitorUsableRect(index);
}

DefineEngineMethod(GuiCanvasNew, getMonitorModeCount, S32, (S32 monitorIndex), (0),
   "Gets the number of video modes available on the selected monitor.\n\n")
{
   return PlatformWindowManager::get()->getMonitorModeCount(monitorIndex);
}
DefineEngineMethod(GuiCanvasNew, getMonitorMode, const char*, (S32 monitorIndex, S32 modeIndex), (0),
   "Gets a video mode string from the selected monitor.\n\n")
{
   char* buf = Con::getReturnBuffer(PlatformWindowManager::get()->getMonitorMode(monitorIndex, modeIndex));
   return buf;
}
DefineEngineMethod(GuiCanvasNew, getMonitorDesktopMode, const char*, (S32 monitorIndex), (0),
   "Gets the current desktop mode for the selected monitor.\n\n")
{
   char* buf = Con::getReturnBuffer(PlatformWindowManager::get()->getMonitorDesktopMode(monitorIndex));
   return buf;
}

DefineEngineMethod(GuiCanvasNew, getVideoMode, const char*, (), ,
   "@brief Gets the current screen mode as a string.\n\n"

   "The return string will contain 5 values (width, height, fullscreen, bitdepth, refreshRate). "
   "You will need to parse out each one for individual use.\n\n"

   "@tsexample\n"
   "%screenWidth = getWord(Canvas.getVideoMode(), 0);\n"
   "%screenHeight = getWord(Canvas.getVideoMode(), 1);\n"
   "%isFullscreen = getWord(Canvas.getVideoMode(), 2);\n"
   "%bitdepth = getWord(Canvas.getVideoMode(), 3);\n"
   "%refreshRate = getWord(Canvas.getVideoMode(), 4);\n"
   "@endtsexample\n\n"

   "@return String formatted with screen width, screen height, screen mode, bit depth, and refresh rate.")
{
   // Grab the video mode.
   if (!object->getPlatformWindow())
      return "";

   GFXVideoMode vm = object->getPlatformWindow()->getVideoMode();
   char* buf = Con::getReturnBuffer(vm.toString());
   return buf;
}


DefineEngineMethod(GuiCanvasNew, getModeCount, S32, (), ,
   "@brief Gets the number of modes available on this device.\n\n"

   "@param param Description\n\n"

   "@tsexample\n"
   "%modeCount = Canvas.getModeCount()\n"
   "@endtsexample\n\n"

   "@return The number of video modes supported by the device")
{
   if (!object->getPlatformWindow())
      return 0;

   // Grab the available mode list from the device.
   const Vector<GFXVideoMode>* const modeList =
      object->getPlatformWindow()->getGFXDevice()->getVideoModeList();

   // Return the number of resolutions.
   return modeList->size();
}

DefineEngineMethod(GuiCanvasNew, getMode, const char*, (S32 modeId), ,
   "@brief Gets information on the specified mode of this device.\n\n"
   "@param modeId Index of the mode to get data from.\n"
   "@return A video mode string given an adapter and mode index.\n\n"
   "@see GuiCanvasNew::getVideoMode()")
{
   if (!object->getPlatformWindow())
      return 0;

   // Grab the available mode list from the device.
   const Vector<GFXVideoMode>* const modeList =
      object->getPlatformWindow()->getGFXDevice()->getVideoModeList();

   // Get the desired index and confirm it's valid.
   S32 idx = modeId;
   if ((idx < 0) || (idx >= modeList->size()))
   {
      Con::errorf("GuiCanvasNew::getResolution - You requested an out of range index of %d. Please specify an index in the range [0, %d).", idx, modeList->size());
      return "";
   }

   // Great - we got something valid, so convert the videomode into a 
   // string and return to the user.
   GFXVideoMode vm = (*modeList)[idx];

   char* retString = Con::getReturnBuffer(vm.toString());
   return retString;
}


DefineEngineMethod(GuiCanvasNew, toggleFullscreen, void, (), ,
   "@brief toggle canvas from fullscreen to windowed mode or back.\n\n"

   "@tsexample\n"
   "// If we are in windowed mode, the following will put is in fullscreen\n"
   "Canvas.toggleFullscreen();"
   "@endtsexample\n\n")
{
   if (Platform::getWebDeployment())
      return;

   if (!object->getPlatformWindow())
      return;

   if (Journal::IsRecording() || Journal::IsPlaying())
      return;

   // Get the window's video mode.
   GFXVideoMode origMode = object->getPlatformWindow()->getVideoMode();

   // And grab the device its using.
   GFXDevice* device = object->getPlatformWindow()->getGFXDevice();

   // Toggle the fullscreen bit.
   GFXVideoMode newMode = origMode;
   newMode.fullScreen = !origMode.fullScreen;

   // CodeReview Toggling might be better served by reading the fullscreen
   //            or windowed video mode pref and setting that instead [bjg 5/2/07]

   if (newMode.fullScreen == true)
   {
      // Are we going to fullscreen? If so find the first matching resolution that
      // is equal to or bigger in size, and has same BPP - windows
      // are often strangely sized and will need to be adjusted to a viable
      // fullscreen res.

      for (S32 i = 0; i < device->getVideoModeList()->size(); i++)
      {
         const GFXVideoMode& newVm = (*(device->getVideoModeList()))[i];

         if (newMode.resolution.x > newVm.resolution.x)
            continue;

         if (newMode.resolution.y > newVm.resolution.y)
            continue;

         if (newMode.bitDepth != newVm.bitDepth)
            continue;

         // Great - got a match.
         newMode = newVm;
         newMode.fullScreen = true;
         break;
      }
   }

   // Ok, we have new video mode. Set it!
   object->getPlatformWindow()->setVideoMode(newMode);
}


DefineEngineMethod(GuiCanvasNew, clientToScreen, Point2I, (Point2I coordinate), ,
   "Translate a coordinate from canvas window-space to screen-space.\n"
   "@param coordinate The coordinate in window-space.\n"
   "@return The given coordinate translated to screen-space.")
{
   if (!object->getPlatformWindow())
      return coordinate;

   return object->getPlatformWindow()->clientToScreen(coordinate);
}

DefineEngineMethod(GuiCanvasNew, screenToClient, Point2I, (Point2I coordinate), ,
   "Translate a coordinate from screen-space to canvas window-space.\n"
   "@param coordinate The coordinate in screen-space.\n"
   "@return The given coordinate translated to window-space.")
{
   if (!object->getPlatformWindow())
      return coordinate;

   return object->getPlatformWindow()->screenToClient(coordinate);
}

DefineEngineMethod(GuiCanvasNew, getWindowPosition, Point2I, (), ,
   "Get the current position of the platform window associated with the canvas.\n"
   "@return The window position of the canvas in screen-space.")
{
   if (!object->getPlatformWindow())
      return Point2I(0, 0);

   return object->getPlatformWindow()->getPosition();
}

DefineEngineMethod(GuiCanvasNew, setWindowPosition, void, (Point2I position), ,
   "Set the position of the platform window associated with the canvas.\n"
   "@param position The new position of the window in screen-space.")
{
   if (!object->getPlatformWindow())
      return;

   object->getPlatformWindow()->setPosition(position);
}

DefineEngineMethod(GuiCanvasNew, isFullscreen, bool, (), , "() - Is this canvas currently fullscreen?")
{
   if (Platform::getWebDeployment())
      return false;

   if (!object->getPlatformWindow())
      return false;

   return object->getPlatformWindow()->getVideoMode().fullScreen;
}

DefineEngineMethod(GuiCanvasNew, minimizeWindow, void, (), , "() - minimize this canvas' window.")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
      window->minimize();
}

DefineEngineMethod(GuiCanvasNew, isMinimized, bool, (), , "()")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
      return window->isMinimized();

   return false;
}

DefineEngineMethod(GuiCanvasNew, isMaximized, bool, (), , "()")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
      return window->isMaximized();

   return false;
}

DefineEngineMethod(GuiCanvasNew, maximizeWindow, void, (), , "() - maximize this canvas' window.")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
      window->maximize();
}

DefineEngineMethod(GuiCanvasNew, restoreWindow, void, (), , "() - restore this canvas' window.")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
      window->restore();
}

DefineEngineMethod(GuiCanvasNew, setFocus, void, (), , "() - Claim OS input focus for this canvas' window.")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
   {
      window->setFocus();
      window->appEvent.trigger(window->getWindowId(), GainFocus);
   }
}

#ifdef TORQUE_TOOLS
DefineEngineMethod(GuiCanvasNew, setMenuBar, void, (GuiControlNew* menu), ,
   "Translate a coordinate from canvas window-space to screen-space.\n"
   "@param coordinate The coordinate in window-space.\n"
   "@return The given coordinate translated to screen-space.")
{
   return object->setMenuBar(menu);
}
#endif

DefineEngineMethod(GuiCanvasNew, setVideoMode, void,
   (U32 width, U32 height, bool fullscreen, U32 bitDepth, U32 refreshRate, U32 antialiasLevel),
   (false, 0, 0, 0),
   "(int width, int height, bool fullscreen, [int bitDepth], [int refreshRate], [int antialiasLevel] )\n"
   "Change the video mode of this canvas. This method has the side effect of setting the $pref::Video::mode to the new values.\n\n"
   "\\param width The screen width to set.\n"
   "\\param height The screen height to set.\n"
   "\\param fullscreen Specify true to run fullscreen or false to run in a window\n"
   "\\param bitDepth [optional] The desired bit-depth. Defaults to the current setting. This parameter is ignored if you are running in a window.\n"
   "\\param refreshRate [optional] The desired refresh rate. Defaults to the current setting. This parameter is ignored if you are running in a window"
   "\\param antialiasLevel [optional] The level of anti-aliasing to apply 0 = none")
{
   if (!object->getPlatformWindow())
      return;

   if (Journal::IsRecording() || Journal::IsPlaying())
      return;

   // Update the video mode and tell the window to reset.
   GFXVideoMode vm = object->getPlatformWindow()->getVideoMode();


   bool changed = false;
   if (width == 0 && height > 0)
   {
      // Our width is 0 but our height isn't...
      // Try to find a matching width
      for (S32 i = 0; i < object->getPlatformWindow()->getGFXDevice()->getVideoModeList()->size(); i++)
      {
         const GFXVideoMode& newVm = (*(object->getPlatformWindow()->getGFXDevice()->getVideoModeList()))[i];

         if (newVm.resolution.y == height)
         {
            width = newVm.resolution.x;
            changed = true;
            break;
         }
      }
   }
   else if (height == 0 && width > 0)
   {
      // Our height is 0 but our width isn't...
      // Try to find a matching height
      for (S32 i = 0; i < object->getPlatformWindow()->getGFXDevice()->getVideoModeList()->size(); i++)
      {
         const GFXVideoMode& newVm = (*(object->getPlatformWindow()->getGFXDevice()->getVideoModeList()))[i];

         if (newVm.resolution.x == width)
         {
            height = newVm.resolution.y;
            changed = true;
            break;
         }
      }
   }

   if (width == 0 || height == 0)
   {
      // Got a bad size for both of our dimensions or one of our dimensions and
      // didn't get a match for the other default back to our current resolution
      width = vm.resolution.x;
      height = vm.resolution.y;

      changed = true;
   }

   if (changed)
   {
      Con::errorf("GuiCanvasNew::setVideoMode(): Error - Invalid resolution of (%d, %d) - attempting (%d, %d)", width, height, width, height);
   }

   vm.resolution = Point2I(width, height);
   vm.fullScreen = fullscreen;

   if (Platform::getWebDeployment())
      vm.fullScreen = false;

   // These optional params are set to default at construction of vm. If they
   // aren't specified, just leave them at whatever they were set to.
   if (bitDepth > 0)
   {
      vm.bitDepth = bitDepth;
   }

   if (refreshRate > 0)
   {
      vm.refreshRate = refreshRate;
   }

   if (antialiasLevel > 0)
   {
      vm.antialiasLevel = antialiasLevel;
   }

   object->getPlatformWindow()->setVideoMode(vm);

   // Store the new mode into a pref.
   Con::setVariable("$pref::Video::mode", vm.toString());
}

DefineEngineMethod(GuiCanvasNew, showWindow, void, (), , "")
{
   if (!object->getPlatformWindow())
      return;

   object->getPlatformWindow()->show();
   WindowManager->setDisplayWindow(true);
   object->getPlatformWindow()->setDisplayWindow(true);
}

DefineEngineMethod(GuiCanvasNew, hideWindow, void, (), , "")
{
   if (!object->getPlatformWindow())
      return;

   object->getPlatformWindow()->hide();
   WindowManager->setDisplayWindow(false);
   object->getPlatformWindow()->setDisplayWindow(false);
}

DefineEngineMethod(GuiCanvasNew, cursorClick, void, (S32 buttonId, bool isDown), , "")
{
   object->cursorClick(buttonId, isDown);
}

DefineEngineMethod(GuiCanvasNew, cursorNudge, void, (F32 x, F32 y), , "")
{
   object->cursorNudge(x, y);
}

DefineEngineMethod(GuiCanvasNew, constrainMouse, void, (bool constrained), , "constrain Mouse to the window")
{
   object->constrainMouse(constrained);
}

// This function allows resetting of the video-mode from script. It was motivated by
// the need to temporarily disable vsync during datablock cache load to avoid a 
// significant slowdown.
bool AFXNew_forceVideoReset = false;


DefineEngineMethod(GuiCanvasNew, resetVideoMode, void, (), , "")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
   {
      GFXWindowTarget* gfx_target = window->getGFXTarget();
      if (gfx_target)
      {
         AFXNew_forceVideoReset = true;
         gfx_target->resetMode();
         AFXNew_forceVideoReset = false;
      }
   }
}

DefineEngineMethod(GuiCanvasNew, getLastInputDevice, const char*, (), , "Returns the name of the last input device that the GuiCanvasNew consumed.")
{
   return object->getLastInputDeviceType();
}

DefineEngineMethod(GuiCanvasNew, getActiveOffscreenCanvas, S32, (), , "Returns the SimID of the active offscreen canvas, if one exists. If not, returns 0")
{
   if (GuiOffscreenCanvasNew::sActiveOffscreenCanvas && GuiOffscreenCanvasNew::sActiveOffscreenCanvas->isActive())
      return GuiOffscreenCanvasNew::sActiveOffscreenCanvas->getId();

   return 0;
}
