//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiCanvas.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "platform/input/event.h"
#include "windowManager/platformWindow.h"
#include "windowManager/platformWindowMgr.h"
#include "windowManager/platformCursorController.h"
#include "windowManager/windowInputGenerator.h"
#include "gfx/gfxInit.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxTarget.h"
#include "math/mMatrix.h"
#include "sfx/sfxSystem.h"
#include "core/util/journal/process.h"
#include "core/util/journal/journal.h"
#include "gui_rev2/core/newGuiCanvas.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gui_rev2/controls/newGuiLabel.h"
#include "sim/sim.h"
#include <utility>   // std::pair - focusNextTabStop()'s sort keys.

IMPLEMENT_CONOBJECT(NewGuiCanvas);

NewGuiCanvas::NewGuiCanvas()
   : mPlatformWindow(NULL),
   mOwnsPlatformWindow(false),
   mGFXDevice(NULL),
   mAutoRenderRegistered(false),
   mRenderBatch(new NewGuiRenderBatch()),
   mDesignScaleX(1.0f),
   mDesignScaleY(1.0f),
   mHasPushedPlatformCursor(false),
   mMouseOverControl(NULL),
   mMouseCapturedControl(NULL),
   mFirstResponder(NULL),
   mActivateKeyCode1(KEY_RETURN),
   mActivateKeyCode2(KEY_SPACE),
   mActivateGamepadButton(XI_A),
   mLastCursorPos(0, 0),
   mDefaultTooltipDelayMS(500),
   mTooltipHoverControl(NULL),
   mTooltipHoverStartMS(0),
   mTooltipVisible(false),
   mActiveTooltipContent(NULL),
   mDefaultTooltipBox(NULL),
   mDefaultTooltipLabel(NULL),
   mDebugShowHitRegions(false),
   mDebugHitRegionFilter(NULL),
   mDebugHitRegionHoverOnly(false),
   mPrimaryTouchFinger((InputObjectInstances)0),
   mHasPrimaryTouchFinger(false)
{
   mActive = true;
   mVisible = true;
}

NewGuiCanvas::~NewGuiCanvas()
{
   delete mRenderBatch;
   mRenderBatch = NULL;

   // mDefaultTooltipBox owns mDefaultTooltipLabel as an ordinary child; deleting the box is sufficient.
   delete mDefaultTooltipBox;
   mDefaultTooltipBox = NULL;
   mDefaultTooltipLabel = NULL;
}

// Bootstraps SFX + GFX + the platform window this canvas drives, then creates and binds the OS window.
Vector<NewGuiCanvas*> NewGuiCanvas::smAllCanvases;

void NewGuiCanvas::setAllCanvasesStyleDirty()
{
   for (U32 i = 0; i < smAllCanvases.size(); i++)
      smAllCanvases[i]->setStyleDirty();
}

bool NewGuiCanvas::onAdd()
{
   if (!Parent::onAdd())
      return false;

   smAllCanvases.push_back(this);

   // --- SFX ---
   SFXSystem::enumerateProviders();
   SFXProvider* p = SFXSystem::getBestProviderChoice();
   if (p)
      SFX->createDevice(p);

   // --- GFX ---
   GFXInit::enumerateAdapters();

   GFXAdapter* a = GFXInit::getBestAdapterChoice();

   GFXDevice* newDevice = GFX;
   if (newDevice == NULL)
      newDevice = GFXInit::createDevice(a);

   if (!newDevice)
   {
      Con::errorf("NewGuiCanvas::onAdd - GFXInit::createDevice() returned NULL, no GFX device available.");
      return false;
   }

   newDevice->setAllowRender(false);
   mGFXDevice = newDevice;

   Journal::Disable();

   // --- Window ---
   GFXVideoMode vm = GFXInit::getInitialVideoMode();

   if (Journal::IsRecording())
   {
      Journal::Write(vm.resolution.x);
      Journal::Write(vm.resolution.y);
      Journal::Write(vm.fullScreen);
   }

   if (Journal::IsPlaying())
   {
      Journal::Read(&vm.resolution.x);
      Journal::Read(&vm.resolution.y);
      Journal::Read(&vm.fullScreen);
   }

   // A NullDevice adapter (headless/dedicated-server run) still gets a device, but no window - nothing to show.
   if (a && a->mType != NullDevice)
   {
      PlatformWindow* window = WindowManager->createWindow(newDevice, vm);
      if (!window)
      {
         Con::errorf("NewGuiCanvas::onAdd - WindowManager->createWindow() returned NULL.");
         return false;
      }

      mOwnsPlatformWindow = true;
      setPlatformWindow(window);

      mPlatformWindow->setCaption("");
      // only show window after the first frame renders.
      //mPlatformWindow->show();
      WindowManager->setDisplayWindow(true);
      mPlatformWindow->setDisplayWindow(true);
   }

   newDevice->setAllowRender(true);

   setAutoRender(true);

   return true;
}

// Mirror of onAdd(): stop ticking, unbind/close the window if owned, release our device reference.
// Does not tear down SFX or the GFXDevice itself - those are process-wide shared state.
void NewGuiCanvas::onRemove()
{
   Con::executef(this, "onDestroyMenu");

   for (U32 i = 0; i < smAllCanvases.size(); i++)
   {
      if (smAllCanvases[i] == this)
      {
         smAllCanvases.erase(i);
         break;
      }
   }

   setAutoRender(false);

   setPlatformWindow(NULL);   // Unsubscribes every window signal.

   mGFXDevice = NULL;

   mMouseOverControl = NULL;
   mMouseCapturedControl = NULL;
   mFirstResponder = NULL;

   Parent::onRemove();
}

void NewGuiCanvas::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Input");

   ADD_FIELD("activateKeyCode1", TypeS32, Offset(mActivateKeyCode1, NewGuiCanvas))
      .doc("Keyboard key code (see event.h's KEY_* constants) that activates the current first responder. Default KEY_RETURN. -1 disables this slot.");

   ADD_FIELD("activateKeyCode2", TypeS32, Offset(mActivateKeyCode2, NewGuiCanvas))
      .doc("Second keyboard key code that activates the current first responder, independent of activateKeyCode1. Default KEY_SPACE. -1 disables this slot.");

   ADD_FIELD("activateGamepadButton", TypeS32, Offset(mActivateGamepadButton, NewGuiCanvas))
      .doc("XInput button constant (see event.h's XI_* constants) that activates the current first responder. Default XI_A. -1 disables gamepad activation.");

   GROUP_END("Input");

   GROUP_BEGIN("Debug");

   ADD_FIELD("debugShowHitRegions", TypeBool, Offset(mDebugShowHitRegions, NewGuiCanvas))
      .doc("When true, every control's real mBounds (the exact rect findHitControl() tests a "
         "point against) is outlined and labeled with its name/class each frame, drawn after "
         "the normal render pass. Useful for diagnosing a control that renders somewhere it "
         "can't actually be clicked - an ancestor whose resolved bounds are narrower than what "
         "it visually renders shows up as an outline stopping short of the drawn content. "
         "Off by default.");

   ADD_FIELD("debugHitRegionFilter", TypeString, 0)
      .onSet(_setDebugHitRegionFilter)
      .doc("Case-insensitive substring filter for the debugShowHitRegions overlay - only controls "
         "whose name, class, or Sim id contains this string are drawn. Empty (default) draws "
         "everything. Ignored while debugHitRegionHoverOnly is true.");

   ADD_FIELD("debugHitRegionHoverOnly", TypeBool, Offset(mDebugHitRegionHoverOnly, NewGuiCanvas))
      .doc("When true (and debugShowHitRegions is also true), the overlay draws only the ANCESTOR "
         "CHAIN down to whatever control the mouse is currently over, instead of the whole tree "
         "- useful for probing one specific spot in a busy/overlapping area (e.g. a stack of "
         "open popups) without every other control's outline cluttering the same view. "
         "Overrides debugHitRegionFilter while on. Off by default.");

   GROUP_END("Debug");

   // mPlatformWindow is a runtime binding, not authored script state.
}

// Binds (or, given NULL, unbinds) this canvas to a window: input controller, size sync, and lifecycle signal subscriptions.
void NewGuiCanvas::setPlatformWindow(PlatformWindow* window)
{
   if (mPlatformWindow == window)
      return;

   if (mPlatformWindow)
   {
      mPlatformWindow->setInputController(NULL);

      mPlatformWindow->displayEvent.remove(this, &NewGuiCanvas::onWindowDisplayEvent);
      mPlatformWindow->resizeEvent.remove(this, &NewGuiCanvas::onWindowResizeEvent);
      mPlatformWindow->appEvent.remove(this, &NewGuiCanvas::onWindowAppEvent);
      mPlatformWindow->dpiChangeEvent.remove(this, &NewGuiCanvas::onWindowDPIChange);

      // A push against this window's own cursor controller means nothing once that controller is no longer ours.
      mHasPushedPlatformCursor = false;

      // Only close a window this canvas created - one merely attached remains the caller's responsibility.
      if (mOwnsPlatformWindow)
      {
         mPlatformWindow->close();
         mOwnsPlatformWindow = false;
      }
   }

   mPlatformWindow = window;

   if (mPlatformWindow)
   {
      mPlatformWindow->setInputController(dynamic_cast<IProcessInput*>(this));

      mPlatformWindow->displayEvent.notify(this, &NewGuiCanvas::onWindowDisplayEvent);
      mPlatformWindow->resizeEvent.notify(this, &NewGuiCanvas::onWindowResizeEvent);
      mPlatformWindow->appEvent.notify(this, &NewGuiCanvas::onWindowAppEvent);
      mPlatformWindow->dpiChangeEvent.notify(this, &NewGuiCanvas::onWindowDPIChange);

      Point2I extent = mPlatformWindow->getClientExtent();

      // Percent(100), not Pixels(extent) - the canvas's own extent always equals its root slot;
      // a Pixels-mode value here would double-scale against ArrangePass()'s own scale substitution.
      mWidth = NewGuiDimension::fromPercent(100.0f);
      mHeight = NewGuiDimension::fromPercent(100.0f);

      recomputeDesignScale(extent);
      setContentDirty();
      setArrangementDirty();

      // Establish the baseline cursor explicitly rather than relying on the platform controller's own default.
      applyCurrentCursor();
   }
}

void NewGuiCanvas::setContent(NewGuiControl* control)
{
   if (!control)
      return;

   NewGuiControl* old = getContent();
   if (old)
      Con::executef(old, "onUnsetContent", Con::getIntArg(control->getId()));

   // Remove all dialogs on layer 0.
   U32 index = 0;
   while (size() > index)
   {
      NewGuiControl* ctrl = static_cast<NewGuiControl*>((*this)[index]);
      if (ctrl == control)
         index++;

      Sim::getGuiGroup()->addObject(ctrl);
   }

   // Add the gui to the front.
   if (!size() || control != (*this)[0])
   {
      addObject(control);   // Automatically wakes objects in GuiControlNew::onWake.
      if (size() >= 2)
         reOrder(control, *begin());
   }

   if (mPlatformWindow)
      recomputeDesignScale(mPlatformWindow->getClientExtent());

   // Do this last so onWake gets called first.
   Con::executef(control, "onSetContent", Con::getIntArg(old ? old->getId() : 0));
}

NewGuiControl* NewGuiCanvas::getContent() const
{
   if (size() > 0)
      return dynamic_cast<NewGuiControl*>(const_cast<NewGuiCanvas*>(this)->first());

   return NULL;
}

// PlatformWindow property passthroughs.
void NewGuiCanvas::setWindowTitle(const char* title)
{
   if (mPlatformWindow)
      mPlatformWindow->setCaption(title ? title : "");
}

const char* NewGuiCanvas::getWindowTitle() const
{
   if (mPlatformWindow)
      return mPlatformWindow->getCaption();
   return "";
}

void NewGuiCanvas::setWindowClientExtent(const Point2I& extent)
{
   if (mPlatformWindow)
      mPlatformWindow->setClientExtent(extent);

   // Does not stamp dirty flags directly - the resulting resizeEvent (onWindowResizeEvent) handles that.
}

Point2I NewGuiCanvas::getWindowClientExtent() const
{
   if (mPlatformWindow)
      return mPlatformWindow->getClientExtent();
   return Point2I(0, 0);
}

void NewGuiCanvas::setWindowPosition(const Point2I& position)
{
   if (mPlatformWindow)
      mPlatformWindow->setPosition(position);
}

Point2I NewGuiCanvas::getWindowPosition() const
{
   if (mPlatformWindow)
      return mPlatformWindow->getPosition();
   return Point2I(0, 0);
}

void NewGuiCanvas::setWindowFullscreen(bool fullscreen)
{
   if (!mPlatformWindow)
      return;

   if (fullscreen)
      mPlatformWindow->setFullscreen(true);
   else
      mPlatformWindow->clearFullscreen();
}

bool NewGuiCanvas::isWindowFullscreen() const
{
   if (mPlatformWindow)
      return mPlatformWindow->isFullscreen();
   return false;
}

void NewGuiCanvas::minimizeWindow()
{
   if (mPlatformWindow)
      mPlatformWindow->minimize();
}

void NewGuiCanvas::maximizeWindow()
{
   if (mPlatformWindow)
      mPlatformWindow->maximize();
}

void NewGuiCanvas::restoreWindow()
{
   if (mPlatformWindow)
      mPlatformWindow->restore();
}

void NewGuiCanvas::hideWindow()
{
   if (mPlatformWindow)
      mPlatformWindow->hide();
}

void NewGuiCanvas::showWindow()
{
   if (mPlatformWindow)
      mPlatformWindow->show();
}

void NewGuiCanvas::setWindowCursorVisible(bool visible)
{
   if (mPlatformWindow)
      mPlatformWindow->setCursorVisible(visible);
}

bool NewGuiCanvas::isWindowCursorVisible() const
{
   if (mPlatformWindow)
      return mPlatformWindow->isCursorVisible();
   return false;
}

void NewGuiCanvas::setCursor(NewGuiCursor* cursor)
{
   if (mCurrentCursor.getObject() == cursor)
      return;

   mCurrentCursor = cursor;

   if (mCurrentCursor)
      mCurrentCursor->resolveImages();

   applyCurrentCursor();
}

void NewGuiCanvas::pushControlCursor(NewGuiCursorShape shape)
{
   mControlCursorStack.push_back(shape);
   applyCurrentCursor();
}

void NewGuiCanvas::popControlCursor()
{
   if (mControlCursorStack.empty())
      return;   // Unbalanced pop is a legitimate, recoverable case (e.g. already hovered when this canvas first bound).

   mControlCursorStack.pop_back();
   applyCurrentCursor();
}

// Resolves mControlCursorStack's current top (or Arrow if empty) and applies it: a drawn image if
// mCurrentCursor has one for this shape, otherwise the real platform shape.
void NewGuiCanvas::applyCurrentCursor()
{
   NewGuiCursorShape shape = getCurrentCursorShape();

   PlatformCursorController* controller = mPlatformWindow ? mPlatformWindow->getCursorController() : NULL;

   if (mCurrentCursor && mCurrentCursor->hasImageForShape(shape))
   {
      // Drawn-image path - hide the real OS cursor so it doesn't double up with the drawn quad.
      if (controller)
         controller->setCursorVisible(false);
      return;
   }

   // No drawn override - plain platform cursor. Pop whatever this canvas last pushed before pushing the new one.
   if (controller)
   {
      if (mHasPushedPlatformCursor)
         controller->popCursor();

      controller->pushCursor(NewGuiCursorShapeToPlatformId(shape));
      mHasPushedPlatformCursor = true;
      controller->setCursorVisible(true);
   }
}

void NewGuiCanvas::onWindowDisplayEvent(WindowId did)
{
   renderFrame();
}

void NewGuiCanvas::onWindowResizeEvent(WindowId did, S32 width, S32 height)
{
   mWidth = NewGuiDimension::fromPercent(100.0f);
   mHeight = NewGuiDimension::fromPercent(100.0f);
   recomputeDesignScale(Point2I(width, height));
   setContentDirty();
   setArrangementDirty();
}

// Reads the content control's own AUTHORED width/height (not resolved bounds) - each axis only
// contributes a non-1.0 scale if that axis is Pixels-mode; Percent/Auto (or no content) stays 1.0.
void NewGuiCanvas::recomputeDesignScale(const Point2I& actualExtent)
{
   mDesignScaleX = 1.0f;
   mDesignScaleY = 1.0f;

   NewGuiControl* content = getContent();
   if (!content)
      return;

   const NewGuiDimension& designWidth = content->getAuthoredWidth();
   const NewGuiDimension& designHeight = content->getAuthoredHeight();

   if (designWidth.isPixels() && designWidth.value > 0.0f)
      mDesignScaleX = F32(actualExtent.x) / designWidth.value;

   if (designHeight.isPixels() && designHeight.value > 0.0f)
      mDesignScaleY = F32(actualExtent.y) / designHeight.value;
}

// DPI can change without a resize (e.g. dragging to a different monitor) - needs its own subscription.
// mDesignScaleX/Y don't depend on DPI; only a re-arrange is needed so ArrangePass() re-reads it fresh.
void NewGuiCanvas::onWindowDPIChange(PlatformWindow* window, F32 newScale)
{
   setArrangementDirty();
}

// Computes the real scale (design scale combined with live DPI) and substitutes it for every
// descendant. Safe to also use for the canvas's own resolution, since mWidth/mHeight are always
// Percent(100) - Percent resolution never touches the scale factor at all.
void NewGuiCanvas::ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   F32 realScaleX = mDesignScaleX;
   F32 realScaleY = mDesignScaleY;

   if (mPlatformWindow)
   {
      const F32 dpiScale = mPlatformWindow->getDPIScale();
      realScaleX *= dpiScale;
      realScaleY *= dpiScale;
   }

   Parent::ArrangePass(slotRect, parentRenderLayer, realScaleX, realScaleY);
}

void NewGuiCanvas::onWindowAppEvent(WindowId did, S32 event)
{
   if (event == LoseFocus)
   {
      // Prevents a control from being left "stuck" owning the mouse/keyboard once the window
      // loses focus. Synthesizes a real onMouseUp() (not just a flag reset) so anything a control
      // did in its own onMouseDown() (e.g. pushCursor()) gets a real chance to symmetrically undo it.
      if (mMouseCapturedControl)
      {
         NewGuiInputEvent syntheticUp;
         syntheticUp.deviceKind = NewGuiDeviceKind::Mouse;
         syntheticUp.action = NewGuiInputAction::Up;
         syntheticUp.screenPoint = mLastCursorPos;
         syntheticUp.localPoint = mLastCursorPos - mMouseCapturedControl->getBounds().point;

         mMouseCapturedControl->onMouseUp(syntheticUp);
         mMouseCapturedControl = NULL;
      }

      if (mMouseOverControl)
      {
         mMouseOverControl->setMouseOver(false);
         mMouseOverControl = NULL;
      }
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
         Con::executef(this, "onWindowClose");
      }
      else if (Con::isFunction("onWindowClose"))
      {
         Con::executef("onWindowClose", getIdString());
      }
      else
      {
         Process::requestShutdown();
      }
   }
}

void NewGuiCanvas::paint()
{
   // Inhibit explicit refreshes if we're swapped out.
   if (mPlatformWindow && mPlatformWindow->isVisible() && GFX->allowRender())
      mPlatformWindow->displayEvent.trigger(mPlatformWindow->getWindowId());
}

void NewGuiCanvas::setAutoRender(bool enable)
{
   if (enable == mAutoRenderRegistered)
      return;

   if (enable)
      Process::notify(this, &NewGuiCanvas::paint, PROCESS_RENDER_ORDER);
   else
      Process::remove(this, &NewGuiCanvas::paint);

   mAutoRenderRegistered = enable;
}

bool NewGuiCanvas::hasAnyDirtyFlag() const
{
   return mStyleDirty || mContentDirty || mArrangementDirty;
}

// The frame driver: StylePass -> MeasurePass -> ArrangePass -> RenderPass, once per dirty frame.
void NewGuiCanvas::renderFrame()
{
   AssertISV(mPlatformWindow, "NewGuiCanvas::renderFrame - no window present!");
   if (!GFX->allowRender() || GFX->canCurrentlyRender())
      return;

   GFX->setActiveRenderTarget(mPlatformWindow->getGFXTarget());

   GFXTarget* renderTarget = GFX->getActiveRenderTarget();
   if (!renderTarget)
      return;

   Point2I extent = renderTarget->getSize();
   if (extent.x == 0 || extent.y == 0)
      return;

   RectI rootSlot(Point2I(0, 0), extent);

   // Catch up on a resize that happened since the last frame before deciding whether there's work to do.
   if (mBounds.extent != extent)
   {
      mWidth = NewGuiDimension::fromPercent(100.0f);
      mHeight = NewGuiDimension::fromPercent(100.0f);

      recomputeDesignScale(extent);
      setContentDirty();
      setArrangementDirty();
   }

   // Style/content/arrangement all propagate to the canvas, so checking its own three flags is
   // sufficient to know whether any layout/style work is needed this frame.
   if (hasAnyDirtyFlag())
   {
      if (mStyleDirty)
      {
         NewGuiResolvedStyle rootInherited;
         StylePass(rootInherited, 0);
      }

      if (mContentDirty)
         MeasurePass();

      if (mArrangementDirty)
      {
         // (1,1) here is a placeholder - NewGuiCanvas::ArrangePass() always recomputes and
         // substitutes the real scale (design scale x live DPI scale) regardless of what it's called with.
         ArrangePass(rootSlot, -1, 1.0f, 1.0f);
      }
   }

   // --- Begin the actual GFX frame ---
   if (!GFX->beginScene())
      return;   // Device waiting on a reset (alt-tab, resolution change, etc) - try again next tick.

   GFX->setViewport(rootSlot);
   GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, ColorI(0, 0, 0, 255), 0.0f, 0);

   GFX->setWorldMatrix(MatrixF::Identity);
   GFX->setViewMatrix(MatrixF::Identity);
   GFX->setProjectionMatrix(MatrixF::Identity);

   // Real GPU scissor, distinct from NewGuiRenderBatch's own CPU-side clip stack - established
   // once against the full root rect, since the whole tree shares one batch/flush pass.
   GFX->setClipRect(rootSlot);

   // Render always runs regardless of layout dirtiness - not every frame needs a re-layout, but every frame needs a render.
   mRenderBatch->begin(rootSlot);
   RenderPass(mRenderBatch, -1);

   // Debug hit-region overlay - drawn AFTER the real render pass so its own clip-rect pushes/pops
   // (NewGuiScroll is the only control that installs one) have fully unwound; every outline below
   // therefore draws unclipped, regardless of what any individual control's own clip region was
   // during its real render just above. Hover-chain mode takes precedence over the whole-tree walk
   // (with its name filter) - see mDebugHitRegionHoverOnly's doc comment.
   if (mDebugShowHitRegions)
   {
      if (mDebugHitRegionHoverOnly)
         debugDrawHoverChain(mRenderBatch);
      else
         debugDrawHitRegions(this, mRenderBatch);
   }

   // Tooltip state updated here (mMouseOverControl is current for this frame) then rendered
   // directly, since it's never a normal tree child - layered below the cursor but above everything else.
   updateTooltipState();

   if (mTooltipVisible && mActiveTooltipContent)
   {
      mActiveTooltipContent->RenderPass(mRenderBatch, mActiveTooltipContent->getRenderLayer());
   }

   // Drawn cursor (if any), submitted last so its layer sorts above everything else in flush().
   if (mCurrentCursor)
   {
      mCurrentCursor->submitDrawnCursor(mRenderBatch, getCurrentCursorShape(), mLastCursorPos);
   }

   mRenderBatch->flush(GFX);

   GFX->endScene();

   GFX->getDeviceEventSignal().trigger(GFXDevice::dePostFrame);

   // Present - without this the completed frame sits in the back buffer and is never shown.
   mPlatformWindow->getGFXTarget()->present();

   if (!mPlatformWindow->isVisible())
      mPlatformWindow->show();
}

// IProcessInput translation - each returns false (ignored) or fills out and returns true.
bool NewGuiCanvas::translateMouseMoveEvent(const InputEventInfo& in, NewGuiInputEvent& out)
{
   // SI_MAKE carries absolute position on SI_XAXIS/SI_YAXIS; SI_MOVE (relative delta) is unused.
   if (in.action != SI_MAKE)
      return false;

   if (in.objInst == SI_XAXIS)
      mLastCursorPos.x = S32(in.fValue);
   else if (in.objInst == SI_YAXIS)
      mLastCursorPos.y = S32(in.fValue);
   else
      return false;

   out.deviceKind = NewGuiDeviceKind::Mouse;
   out.action = NewGuiInputAction::Move;
   out.screenPoint = mLastCursorPos;
   out.modifier = in.modifier;
   return true;
}

bool NewGuiCanvas::translateMouseButtonEvent(const InputEventInfo& in, NewGuiInputEvent& out)
{
   if (in.objType != SI_BUTTON)
      return false;

   out.deviceKind = NewGuiDeviceKind::Mouse;
   out.action = (in.action == SI_MAKE) ? NewGuiInputAction::Down : NewGuiInputAction::Up;
   out.screenPoint = mLastCursorPos;
   out.modifier = in.modifier;
   return true;
}

bool NewGuiCanvas::translateMouseWheelEvent(const InputEventInfo& in, NewGuiInputEvent& out)
{
   if (in.action != SI_MOVE)
      return false;

   if (in.objInst == SI_ZAXIS)
      out.wheelAxis = 1;   // 1 = vertical.
   else if (in.objInst == SI_RZAXIS)
      out.wheelAxis = 0;   // 0 = horizontal.
   else
      return false;

   out.deviceKind = NewGuiDeviceKind::Mouse;
   out.action = NewGuiInputAction::Wheel;
   out.screenPoint = mLastCursorPos;
   out.modifier = in.modifier;

   // in.fValue arrives in raw platform wheel units, NOT already-normalized
   // step counts.
   static const F32 kWheelUnitsPerStep = 120.0f;
   out.wheelDelta = in.fValue / kWheelUnitsPerStep;

   return true;
}

bool NewGuiCanvas::translateKeyEvent(const InputEventInfo& in, NewGuiInputEvent& out)
{
   if (in.objType != SI_KEY || in.objInst == KEY_NULL)
      return false;

   out.deviceKind = NewGuiDeviceKind::Keyboard;
   out.action = (in.action == SI_REPEAT) ? NewGuiInputAction::Repeat
      : (in.action == SI_MAKE) ? NewGuiInputAction::Down
      : NewGuiInputAction::Up;
   out.ascii = in.ascii;
   out.keyCode = in.objInst;
   out.modifier = in.modifier;
   out.isCharInput = false;
   return true;
}

// Decoded character, off WindowInputGenerator::handleCharInput()
bool NewGuiCanvas::translateCharInputEvent(const InputEventInfo& in, NewGuiInputEvent& out)
{
   if (in.objType != SI_KEY || in.objInst != KEY_NULL || in.action != SI_MAKE)
      return false;

   out.deviceKind = NewGuiDeviceKind::Keyboard;
   out.action = NewGuiInputAction::Down;
   out.ascii = in.ascii;
   out.keyCode = 0;
   out.modifier = in.modifier;
   out.isCharInput = true;
   return true;
}

// Collapses SI_LSHIFT/SI_RSHIFT (and the Ctrl/Alt equivalents) into the engine's existing
// logical SI_SHIFT/SI_CTRL/SI_ALT bits
U32 NewGuiCanvas::NormalizeModifiers(U32 raw)
{
   U32 result = 0;

   if ((raw & SI_SHIFT) || (raw & SI_LSHIFT) || (raw & SI_RSHIFT))
      result |= SI_SHIFT;
   if ((raw & SI_CTRL) || (raw & SI_LCTRL) || (raw & SI_RCTRL))
      result |= SI_CTRL;
   if ((raw & SI_ALT) || (raw & SI_LALT) || (raw & SI_RALT))
      result |= SI_ALT;

   return result;
}

void NewGuiCanvas::registerAccelerator(NewGuiControl* control, U16 keyCode, U32 modifier)
{
   if (!control)
      return;

   NewGuiAcceleratorBinding binding;
   binding.control = control;
   binding.keyCode = keyCode;
   binding.modifier = NormalizeModifiers(modifier);

   mAccelerators.push_back(binding);
}

void NewGuiCanvas::removeAccelerators(NewGuiControl* control)
{
   for (S32 i = (S32)mAccelerators.size() - 1; i >= 0; --i)
   {
      if (mAccelerators[i].control == control)
         mAccelerators.erase(i);
   }
}

bool NewGuiCanvas::checkAccelerators(const NewGuiInputEvent& event)
{
   if (event.deviceKind != NewGuiDeviceKind::Keyboard)
      return false;

   if (mFirstResponder && mFirstResponder->wantsRawKeyboardInput())
      return false;

   U32 eventModifier = NormalizeModifiers(event.modifier);

   for (U32 i = 0; i < (U32)mAccelerators.size(); i++)
   {
      const NewGuiAcceleratorBinding& binding = mAccelerators[i];

      if (binding.control && binding.keyCode == event.keyCode && binding.modifier == eventModifier)
      {
         binding.control->onAccelerator();
         return true;
      }
   }

   return false;
}

void NewGuiCanvas::enableKeyboardAccelerators(bool enabled)
{
   if (mPlatformWindow)
      mPlatformWindow->setAcceleratorsEnabled(enabled);
}

void NewGuiCanvas::enableKeyboardTranslation(bool enabled)
{
   if (mPlatformWindow)
      mPlatformWindow->setKeyboardTranslation(enabled);
}

// SI_TOUCH: fValue/fValue2 are normalized (0..1) X/Y; objInst is the finger slot; action is SI_MAKE/SI_MOVE/SI_BREAK.
bool NewGuiCanvas::translateTouchEvent(const InputEventInfo& in, NewGuiInputEvent& out)
{
   if (in.objType != SI_TOUCH)
      return false;

   bool isBreak = false;

   if (in.action == SI_MAKE)
   {
      // First finger down while none is primary becomes the mouse-equivalent pointer;
      // additional simultaneous fingers are ignored by this translation layer.
      if (mHasPrimaryTouchFinger)
         return false;

      mHasPrimaryTouchFinger = true;
      mPrimaryTouchFinger = in.objInst;
      out.action = NewGuiInputAction::Down;
   }
   else if (in.action == SI_BREAK)
   {
      if (!mHasPrimaryTouchFinger || in.objInst != mPrimaryTouchFinger)
         return false;

      isBreak = true;
      out.action = NewGuiInputAction::Up;
   }
   else if (in.action == SI_MOVE)
   {
      if (!mHasPrimaryTouchFinger || in.objInst != mPrimaryTouchFinger)
         return false;

      out.action = NewGuiInputAction::Move;
   }
   else
   {
      return false;
   }

   // Normalized -> pixel, against this canvas's own current extent.
   Point2I extent = mBounds.extent;
   Point2I pixelPos(
      S32(in.fValue * F32(extent.x)),
      S32(in.fValue2 * F32(extent.y)));

   mLastCursorPos = pixelPos;
   out.deviceKind = NewGuiDeviceKind::Touch;
   out.screenPoint = pixelPos;
   out.modifier = 0;   // Touch carries no keyboard-modifier state.

   if (isBreak)
      mHasPrimaryTouchFinger = false;

   return true;
}

// A gamepad button arrives as objType == SI_BUTTON with an XI_* constant in objInst, deviceType
// XInputDeviceType, exactly like a mouse click with a different button space. Digital buttons only -
// analog sticks/triggers are not translated here. keyCode carries the XI_* button identity.
bool NewGuiCanvas::translateGamepadEvent(const InputEventInfo& in, NewGuiInputEvent& out)
{
   if (in.deviceType != XInputDeviceType)
      return false;

   if (in.objType != SI_BUTTON)
      return false;

   if (in.action != SI_MAKE && in.action != SI_BREAK)
      return false;

   out.deviceKind = NewGuiDeviceKind::Gamepad;
   out.action = (in.action == SI_MAKE) ? NewGuiInputAction::Down : NewGuiInputAction::Up;
   out.keyCode = (U16)in.objInst;
   out.modifier = in.modifier;
   return true;
}

// Mirrors dispatchKeyEvent() - routes to the first responder (no pointer position to hit-test), bubbling until handled.
void NewGuiCanvas::dispatchGamepadEvent(NewGuiInputEvent& event)
{
   bool activate = isActivateEvent(event);

   NewGuiControl* target = mFirstResponder;

   while (target && !event.handled)
   {
      if (activate)
         target->onActivate(event);
      else
         target->onInputEvent(event);

      if (event.handled)
         break;

      target = dynamic_cast<NewGuiControl*>(target->getGroup());
   }
}

// Reads the authored activate bindings rather than hardcoded literals, so a project can rebind
// activate without touching engine code. Only Down/Up qualify - Repeat is excluded so holding
// the key doesn't re-fire activate every auto-repeat tick.
bool NewGuiCanvas::isActivateEvent(const NewGuiInputEvent& event) const
{
   if (event.action != NewGuiInputAction::Down && event.action != NewGuiInputAction::Up)
      return false;

   if (event.deviceKind == NewGuiDeviceKind::Keyboard)
   {
      return (mActivateKeyCode1 >= 0 && event.keyCode == (U16)mActivateKeyCode1)
         || (mActivateKeyCode2 >= 0 && event.keyCode == (U16)mActivateKeyCode2);
   }

   if (event.deviceKind == NewGuiDeviceKind::Gamepad)
   {
      return mActivateGamepadButton >= 0 && event.keyCode == (U16)mActivateGamepadButton;
   }

   return false;
}

void NewGuiCanvas::updateMouseOver(NewGuiControl* newOverControl, NewGuiInputEvent& event)
{
   if (mMouseOverControl == newOverControl)
      return;

   if (mMouseOverControl)
   {
      // Reset handled between the two calls so onMouseLeave() setting it doesn't suppress onMouseEnter().
      event.handled = false;
      mMouseOverControl->onMouseLeave(event);
   }

   mMouseOverControl = newOverControl;

   if (mMouseOverControl)
   {
      event.handled = false;
      mMouseOverControl->onMouseEnter(event);
   }

   event.handled = false;
}

// Built lazily once, then repopulated (setText()) on every subsequent call.
NewGuiControl* NewGuiCanvas::buildDefaultTooltipContent(const char* text)
{
   if (!mDefaultTooltipBox)
   {
      mDefaultTooltipBox = new NewGuiControl();
      mDefaultTooltipBox->mWidth = NewGuiDimension::fromAuto();
      mDefaultTooltipBox->mHeight = NewGuiDimension::fromAuto();

      mDefaultTooltipLabel = new NewGuiLabel();
      mDefaultTooltipLabel->mWidth = NewGuiDimension::fromAuto();
      mDefaultTooltipLabel->mHeight = NewGuiDimension::fromAuto();

      // Ordinary parent/child pair, but never Sim-registered and never a child of the canvas itself.
      mDefaultTooltipBox->addObject(mDefaultTooltipLabel);

      if (mDefaultTooltipStyle)
         mDefaultTooltipBox->setStyleAsset(mDefaultTooltipStyle);
   }

   mDefaultTooltipLabel->setText(text);
   return mDefaultTooltipBox;
}

// Called once per renderFrame(), after mMouseOverControl is current for the frame.
void NewGuiCanvas::updateTooltipState()
{
   // Walk up from mMouseOverControl (deepest-first) to the nearest control that has a tooltip authored.
   NewGuiControl* hoverCandidate = NULL;
   for (NewGuiControl* c = mMouseOverControl; c; c = dynamic_cast<NewGuiControl*>(c->getGroup()))
   {
      if (c->hasTooltip())
      {
         hoverCandidate = c;
         break;
      }
   }

   if (hoverCandidate != mTooltipHoverControl)
   {
      // Hover target changed - reset the timer and hide whatever was showing; a new hover always
      // restarts from "not yet shown," matching standard tooltip UX.
      mTooltipHoverControl = hoverCandidate;
      mTooltipHoverStartMS = (U32)Sim::getCurrentTime();
      mTooltipVisible = false;
      mActiveTooltipContent = NULL;
      return;
   }

   if (!mTooltipHoverControl || mTooltipVisible)
      return;   // Nothing hovered, or already shown.

   const S32 delay = (mTooltipHoverControl->getTooltipDelay() >= 0) ? mTooltipHoverControl->getTooltipDelay() : mDefaultTooltipDelayMS;
   const U32 elapsed = (U32)Sim::getCurrentTime() - mTooltipHoverStartMS;

   if ((S32)elapsed < delay)
      return;   // Not yet.

   // Delay elapsed - show it. mTooltipContent takes priority over plain mTooltipText.
   NewGuiControl* content = mTooltipHoverControl->getTooltipContent();
   if (!content)
      content = buildDefaultTooltipContent(mTooltipHoverControl->getTooltipText());

   mActiveTooltipContent = content;
   mTooltipVisible = true;

   positionTooltip();
}

// Measures mActiveTooltipContent, then arranges it near mLastCursorPos, clamped to the window's client extent.
void NewGuiCanvas::positionTooltip()
{
   if (!mActiveTooltipContent || !mPlatformWindow)
      return;

   static const S32 kCursorOffset = 16;   // Pixels away from the cursor, so the tooltip doesn't sit under it.

   // A fresh StylePass - mActiveTooltipContent is never a child of this canvas's own tree, so it
   // never picks up a resolved style from the ordinary root-down walk.
   NewGuiResolvedStyle rootInherited;
   mActiveTooltipContent->StylePass(rootInherited, 0);

   mActiveTooltipContent->MeasurePass();
   const Point2I preferredSize = mActiveTooltipContent->getPreferredSize();

   Point2I windowExtent = mPlatformWindow->getClientExtent();

   Point2I pos = mLastCursorPos + Point2I(kCursorOffset, kCursorOffset);

   // Flip to the opposite side of the cursor on whichever axis would otherwise render off-window.
   if (pos.x + preferredSize.x > windowExtent.x)
      pos.x = mLastCursorPos.x - kCursorOffset - preferredSize.x;

   if (pos.y + preferredSize.y > windowExtent.y)
      pos.y = mLastCursorPos.y - kCursorOffset - preferredSize.y;

   pos.x = mClamp(pos.x, 0, getMax(0, windowExtent.x - preferredSize.x));
   pos.y = mClamp(pos.y, 0, getMax(0, windowExtent.y - preferredSize.y));

   RectI tooltipSlot(pos, preferredSize);

   F32 realScaleX = mDesignScaleX;
   F32 realScaleY = mDesignScaleY;
   const F32 dpiScale = mPlatformWindow->getDPIScale();
   realScaleX *= dpiScale;
   realScaleY *= dpiScale;

   // Sits below the cursor's own layer (100000) but above any realistic tree depth.
   static const S32 kTooltipLayer = 50000;
   mActiveTooltipContent->ArrangePass(tooltipSlot, kTooltipLayer, realScaleX, realScaleY);
}

void NewGuiCanvas::ensureDebugFont()
{
   if (mDebugFont != NULL)
      return;

   // Deliberately independent of any control's resolved style - the overlay has to stay legible
   // (and keep working at all) even in a tree with no styles set, or styles that make text
   // invisible/unreadably small, since diagnosing exactly that kind of broken tree is the point.
   mDebugFont = GFont::create("Arial", 12);
}

// Local case-insensitive substring search - NOT assumed to exist as a platform helper in this
// codebase (only dStricmp/dStrnicmp are confirmed elsewhere), so built from dStrnicmp instead of
// risking an undeclared dStristr.
static bool debugCaseInsensitiveContains(const char* haystack, const char* needle)
{
   if (!haystack || !needle || !needle[0])
      return false;

   const U32 needleLen = dStrlen(needle);
   for (const char* p = haystack; *p; ++p)
   {
      if (dStrnicmp(p, needle, needleLen) == 0)
         return true;
   }
   return false;
}

bool NewGuiCanvas::_setDebugHitRegionFilter(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiCanvas*>(obj)->setDebugHitRegionFilter(data);
   return false;
}

// Same name/class/id fallback debugDrawOneHitRegion() uses for its label, so "what a control is
// searchable by" always matches "what its label says" - a filter that matched on a field the
// overlay never actually shows would be confusing to use.
bool NewGuiCanvas::debugHitRegionPasses(NewGuiControl* control) const
{
   if (!mDebugHitRegionFilter || !mDebugHitRegionFilter[0])
      return true;

   if (debugCaseInsensitiveContains(control->getName(), mDebugHitRegionFilter))
      return true;

   if (debugCaseInsensitiveContains(control->getClassName(), mDebugHitRegionFilter))
      return true;

   if (debugCaseInsensitiveContains(control->getIdString(), mDebugHitRegionFilter))
      return true;

   return false;
}

// Draws exactly one control's outline (color-coded by its own visible/active/hitTestable state)
// plus, space permitting, its name label. Shared by the whole-tree walk (debugDrawHitRegions())
// and the hover-chain walk (debugDrawHoverChain()) - no recursion, no filter check here; both
// callers decide which controls reach this on their own.
//
//   - green  : normal - visible, active, hit-testable. This is what findHitControl() can return.
//   - yellow : hitTestable = false. Rendered, tested for arrangement, but pointInRect() checks on
//              CHILDREN can still hit through to something below this control's own layer -
//              findHitControl() will never return this control itself.
//   - red    : !visible or !active. findHitControl() bails out of this whole subtree immediately
//              (see the early-out at the top of findHitControl()) - nothing under here, however
//              correctly sized, can ever be hit while this is true.
void NewGuiCanvas::debugDrawOneHitRegion(NewGuiControl* control, NewGuiRenderBatch* batch)
{
   ensureDebugFont();

   // Fixed, very high layer - always paints above every real control and above the overlay's own
   // sibling outlines drawn earlier in this same walk, regardless of tree depth/render layer.
   // Kept below the tooltip (50000) and cursor (100000) layers so those remain legible over it.
   static const S32 kDebugOverlayLayer = 45000;

   const RectI& bounds = control->getBounds();

   const bool skipped = !control->isVisible() || !control->isActive();
   const bool passThrough = !control->isHitTestable();

   ColorI outlineColor = ColorI(60, 220, 90, 255);     // green - normal, can be hit directly.
   if (skipped)
      outlineColor = ColorI(230, 60, 60, 255);         // red - this whole subtree is unreachable.
   else if (passThrough)
      outlineColor = ColorI(230, 200, 60, 255);        // yellow - rendered, but never itself the hit result.

   // A skipped subtree still gets drawn (see the class doc above) but faded, so it reads as
   // "exists, can't be hit" rather than looking identical to a normal reachable control.
   const U8 alpha = skipped ? 110 : 255;
   outlineColor.alpha = alpha;

   static const F32 kOutlineThickness = 1.0f;

   const Point2I topLeft = bounds.point;
   const Point2I topRight(bounds.point.x + bounds.extent.x, bounds.point.y);
   const Point2I bottomLeft(bounds.point.x, bounds.point.y + bounds.extent.y);
   const Point2I bottomRight(bounds.point.x + bounds.extent.x, bounds.point.y + bounds.extent.y);

   batch->pushLine(topLeft, topRight, outlineColor, kOutlineThickness, kDebugOverlayLayer);
   batch->pushLine(topRight, bottomRight, outlineColor, kOutlineThickness, kDebugOverlayLayer);
   batch->pushLine(bottomRight, bottomLeft, outlineColor, kOutlineThickness, kDebugOverlayLayer);
   batch->pushLine(bottomLeft, topLeft, outlineColor, kOutlineThickness, kDebugOverlayLayer);

   // Name label - name if set, else class name, else the Sim id, same fallback order the rest of
   // the engine uses when referring to an unnamed object in diagnostics (and the same order
   // debugHitRegionPasses() searches) - followed by the control's actual resolved bounds
   // (x,y w x h, in device pixels, straight from the same mBounds the outline itself is drawn
   // from) so a collapsed/mispositioned box shows its own numbers directly rather than needing to
   // be inferred from how the outline looks on screen.
   const char* name = control->getName();
   if (!name || !name[0])
      name = control->getClassName();
   if (!name || !name[0])
      name = control->getIdString();

   char label[160];
   dSprintf(label, sizeof(label), "%s  %d,%d %dx%d", name, bounds.point.x, bounds.point.y, bounds.extent.x, bounds.extent.y);

   static const S32 kMinLabelWidth = 24;
   static const S32 kMinLabelHeight = 14;

   if (mDebugFont != NULL && bounds.extent.x >= kMinLabelWidth && bounds.extent.y >= kMinLabelHeight)
   {
      // Small inset from the outline's top-left corner, and a solid backing quad behind the text
      // so it stays legible over whatever busy content is already drawn underneath at this point.
      static const S32 kLabelInset = 2;
      const Point2I labelPos = topLeft + Point2I(kLabelInset, kLabelInset);

      const S32 backingWidth = (S32)dStrlen(label) * 7 + 4;
      const S32 backingHeight = 14;
      RectI labelBacking(labelPos, Point2I(getMax(0, backingWidth), backingHeight));

      ColorI backingColor(0, 0, 0, skipped ? 90 : 180);
      batch->pushQuad(labelBacking, backingColor, kDebugOverlayLayer);

      ColorI textColor = outlineColor;
      textColor.alpha = 255;   // Text itself always full-opacity, even over a faded/skipped outline - it's the part that has to stay readable.

      batch->pushText(mDebugFont, labelPos + Point2I(2, 1), label, textColor, 0, 0, kDebugOverlayLayer + 1);
   }
   else if (mDebugFont != NULL)
   {
      // Rect too small (or degenerate - a near-zero-width/height box is exactly the symptom of a
      // resolved-bounds bug, not a rare edge case) to hold the label INSIDE its own outline - drawn
      // instead as a small floating tag just above-right of the rect's top-left corner, so a
      // collapsed control is still fully diagnosable (name + real numbers) rather than silently
      // dropped from the overlay for being "too small to label."
      const Point2I tagPos = topLeft + Point2I(2, -16);

      const S32 backingWidth = (S32)dStrlen(label) * 7 + 4;
      const S32 backingHeight = 14;
      RectI labelBacking(tagPos, Point2I(getMax(0, backingWidth), backingHeight));

      ColorI backingColor(0, 0, 0, skipped ? 90 : 180);
      batch->pushQuad(labelBacking, backingColor, kDebugOverlayLayer);

      ColorI textColor = outlineColor;
      textColor.alpha = 255;

      batch->pushText(mDebugFont, tagPos + Point2I(2, 1), label, textColor, 0, 0, kDebugOverlayLayer + 1);
   }
}

// Recurses the same tree RenderPass() just walked, drawing one outline per control from its OWN
// mBounds - the same field RenderPass()/EmitDrawCommands() just read for real drawing, and the
// same field findHitControl() tests a point against (see newGuiControl.cpp) - so this overlay can
// never itself drift from what hit-testing actually does; there's only the one rect.
//
// Runs over the WHOLE tree (subject to mDebugHitRegionFilter - see debugHitRegionPasses()),
// including invisible/inactive/non-hit-testable controls, since those are exactly the cases worth
// being able to see: an invisible control still occupies layout space and can still be why a
// sibling landed somewhere unexpected, and a non-hit-testable one is a common, easy-to-forget
// reason a click passes through to whatever's behind it.
//
// A filtered-out control still recurses into its children even though it isn't itself drawn - the
// filter narrows WHICH controls get an outline, not which subtrees get walked, so e.g. filtering
// to "Button" still finds every matching button regardless of which unnamed/non-matching stacks
// and panels happen to contain it.
void NewGuiCanvas::debugDrawHitRegions(NewGuiControl* control, NewGuiRenderBatch* batch)
{
   if (!control)
      return;

   if (debugHitRegionPasses(control))
      debugDrawOneHitRegion(control, batch);

   for (SimSet::iterator itr = control->begin(); itr != control->end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         debugDrawHitRegions(child, batch);
   }
}

// mDebugHitRegionHoverOnly's draw path. Walks mMouseOverControl's ancestor chain upward via
// getGroup() - cheap (chain depth, not tree size) rather than recursing the whole tree and
// checking "is this an ancestor of mMouseOverControl" per node - then draws root-to-leaf (the
// order the chain was collected in reverse) so a nested control's outline/label still paints over
// its parent's, same paint order a normal whole-tree walk would have produced for that same chain.
// Draws nothing if the mouse isn't currently over anything (mMouseOverControl == NULL) - there's
// no chain to show, and drawing the whole tree as a fallback would defeat the point of this mode.
void NewGuiCanvas::debugDrawHoverChain(NewGuiRenderBatch* batch)
{
   if (!mMouseOverControl)
      return;

   Vector<NewGuiControl*> chain;
   for (NewGuiControl* c = mMouseOverControl; c; c = dynamic_cast<NewGuiControl*>(c->getGroup()))
      chain.push_back(c);

   for (S32 i = S32(chain.size()) - 1; i >= 0; --i)
      debugDrawOneHitRegion(chain[i], batch);
}

// Move goes only to the exact hit/captured control (non-bubbling); every other action bubbles up via getGroup().
void NewGuiCanvas::dispatchMouseEvent(NewGuiInputEvent& event)
{
   NewGuiControl* hit = findHitControl(event.screenPoint);

   if (event.action == NewGuiInputAction::Down)
   {
      for (S32 i = S32(size()) - 1; i >= 0; --i)
      {
         NewGuiControl* child = dynamic_cast<NewGuiControl*>(at(i));
         if (child)
            child->onOutsideHitTest(hit);
      }
   }

   // Hover tracking is suppressed while something has mouse capture - hover is meaningless during a drag.
   if (!mMouseCapturedControl)
      updateMouseOver(hit, event);

   NewGuiControl* target = mMouseCapturedControl ? mMouseCapturedControl : hit;
   if (!target)
      return;

   if (event.action == NewGuiInputAction::Down)
      mMouseCapturedControl = target;

   while (target)
   {
      event.localPoint = event.screenPoint - target->getBounds().point;

      switch (event.action)
      {
      case NewGuiInputAction::Down:
         target->onMouseDown(event);
         break;
      case NewGuiInputAction::Up:
         target->onMouseUp(event);
         break;
      case NewGuiInputAction::Move:
         target->onInputEvent(event);
         target = NULL;   // Move never bubbles.
         continue;
      default:
         target->onInputEvent(event);
         break;
      }

      if (event.handled)
         break;

      target = dynamic_cast<NewGuiControl*>(target->getGroup());
   }

   if (event.action == NewGuiInputAction::Up && mMouseCapturedControl)
   {
      mMouseCapturedControl = NULL;

      // Hover was suppressed for the whole duration of the capture - re-resolve immediately against
      // the pointer's current position rather than waiting for the next Move to fire.
      updateMouseOver(hit, event);
   }
}

void NewGuiCanvas::dispatchKeyEvent(NewGuiInputEvent& event)
{
   // A decoded character (isCharInput) is never an accelerator, Tab, or activate trigger
   if (event.isCharInput)
   {
      NewGuiControl* target = mFirstResponder;
      while (target && !event.handled)
      {
         target->onInputEvent(event);
         if (event.handled)
            break;
         target = dynamic_cast<NewGuiControl*>(target->getGroup());
      }
      return;
   }

   // Accelerators are checked first, ahead of Tab and the focus-scoped bubble walk
   if (event.action == NewGuiInputAction::Down && checkAccelerators(event))
   {
      event.handled = true;
      return;
   }

   if (event.keyCode == KEY_TAB && event.action == NewGuiInputAction::Down)
   {
      bool reverse = (event.modifier & (SI_LSHIFT | SI_RSHIFT)) != 0;
      focusNextTabStop(reverse);
      event.handled = true;
      return;
   }

   bool activate = isActivateEvent(event);

   NewGuiControl* target = mFirstResponder;

   while (target && !event.handled)
   {
      if (activate)
         target->onActivate(event);
      else
         target->onInputEvent(event);

      if (event.handled)
         break;

      target = dynamic_cast<NewGuiControl*>(target->getGroup());
   }
}

void NewGuiCanvas::setFirstResponderControl(NewGuiControl* control)
{
   if (mFirstResponder == control)
      return;

   if (mFirstResponder)
      mFirstResponder->setFirstResponder(false);

   mFirstResponder = control;

   if (mFirstResponder)
      mFirstResponder->setFirstResponder(true);
}

// Walks the whole tree via collectTabStops() (parent before children, matching paint order), stable-
// sorts by (effective tab index, natural walk position) so an override only ever moves that one
// stop, then steps forward/backward from mFirstResponder's own position, wrapping at either end.
void NewGuiCanvas::focusNextTabStop(bool reverse)
{
   Vector<NewGuiControl*> stops;
   collectTabStops(stops);

   if (stops.empty())
      return;

   // Pair each stop with its effective sort key, captured from its own pre-sort index.
   Vector< std::pair<S32, NewGuiControl*> > ordered;
   ordered.reserve(stops.size());

   for (U32 i = 0; i < (U32)stops.size(); ++i)
   {
      S32 effectiveIndex = (stops[i]->getTabIndex() >= 0) ? stops[i]->getTabIndex() : (S32)i;
      ordered.push_back(std::make_pair(effectiveIndex, stops[i]));
   }

   std::stable_sort(ordered.address(), ordered.address() + ordered.size(),
      [](const std::pair<S32, NewGuiControl*>& a, const std::pair<S32, NewGuiControl*>& b)
   {
      return a.first < b.first;
   });

   S32 currentPos = -1;
   for (U32 i = 0; i < (U32)ordered.size(); ++i)
   {
      if (ordered[i].second == mFirstResponder)
      {
         currentPos = (S32)i;
         break;
      }
   }

   S32 count = (S32)ordered.size();
   S32 nextPos;

   if (currentPos < 0)
   {
      // Nothing focused - Tab starts at the first stop, Shift+Tab at the last.
      nextPos = reverse ? (count - 1) : 0;
   }
   else
   {
      nextPos = reverse ? (currentPos - 1) : (currentPos + 1);
      if (nextPos < 0)
         nextPos = count - 1;
      else if (nextPos >= count)
         nextPos = 0;
   }

   setFirstResponderControl(ordered[nextPos].second);
}

// Single entry point raw platform input arrives through. Returns true if consumed (a false return
// lets input fall through to ActionMap).
bool NewGuiCanvas::processInputEvent(InputEventInfo& inputEvent)
{
   if (inputEvent.deviceType == MouseDeviceType)
   {
      NewGuiInputEvent event;

      if (inputEvent.objType == SI_BUTTON)
      {
         if (!translateMouseButtonEvent(inputEvent, event))
            return false;

         dispatchMouseEvent(event);
         return true;
      }

      if (translateMouseMoveEvent(inputEvent, event))
      {
         dispatchMouseEvent(event);
         return true;
      }

      if (translateMouseWheelEvent(inputEvent, event))
      {
         dispatchMouseEvent(event);
         return true;
      }

      return false;
   }

   if (inputEvent.deviceType == KeyboardDeviceType)
   {
      NewGuiInputEvent event;

      // Two distinct InputEventInfo shapes arrive on this same device type
      if (translateCharInputEvent(inputEvent, event))
      {
         dispatchKeyEvent(event);
         return true;
      }

      if (!translateKeyEvent(inputEvent, event))
         return false;

      dispatchKeyEvent(event);
      return true;
   }

   if (inputEvent.deviceType == TouchDeviceType)
   {
      NewGuiInputEvent event;
      if (!translateTouchEvent(inputEvent, event))
         return false;

      dispatchMouseEvent(event);
      return true;
   }

   if (inputEvent.deviceType == GamepadDeviceType || inputEvent.deviceType == XInputDeviceType)
   {
      NewGuiInputEvent event;
      if (!translateGamepadEvent(inputEvent, event))
         return false;

      dispatchGamepadEvent(event);
      return true;
   }

   // Any other device type falls through to the ActionMap, same as the legacy default.
   return false;
}

// Console method exposure - setContent()/getContent() and window property passthroughs.
DefineEngineMethod(NewGuiCanvas, setContent, void, (NewGuiControl* control), ,
   "Detaches the current content control (if any - not deleted) and attaches control as the new one. Pass an empty/NULL control to just clear the canvas.")
{
   object->setContent(control);
}

DefineEngineMethod(NewGuiCanvas, getContent, S32, (), ,
   "Returns the ID of this canvas's current content control, or 0 if none is set.")
{
   NewGuiControl* content = object->getContent();
   return content ? content->getId() : 0;
}

DefineEngineMethod(NewGuiCanvas, setWindowTitle, void, (const char* title), ,
   "Sets this canvas's platform window caption/title bar text.")
{
   object->setWindowTitle(title);
}

DefineEngineMethod(NewGuiCanvas, getWindowTitle, const char*, (), ,
   "Returns this canvas's platform window caption/title bar text.")
{
   return object->getWindowTitle();
}

DefineEngineMethod(NewGuiCanvas, setWindowClientExtent, void, (const char* extent), ,
   "Resizes this canvas's platform window's client (drawable) area. extent is \"width height\".")
{
   Point2I e(0, 0);
   dSscanf(extent, "%d %d", &e.x, &e.y);
   object->setWindowClientExtent(e);
}

DefineEngineMethod(NewGuiCanvas, getWindowClientExtent, const char*, (), ,
   "Returns this canvas's platform window's client (drawable) area size, as \"width height\".")
{
   Point2I e = object->getWindowClientExtent();
   static const U32 bufSize = 32;
   char* buf = Con::getReturnBuffer(bufSize);
   dSprintf(buf, bufSize, "%d %d", e.x, e.y);
   return buf;
}

DefineEngineMethod(NewGuiCanvas, setWindowPosition, void, (const char* position), ,
   "Moves this canvas's platform window to the given screen position. position is \"x y\".")
{
   Point2I p(0, 0);
   dSscanf(position, "%d %d", &p.x, &p.y);
   object->setWindowPosition(p);
}

DefineEngineMethod(NewGuiCanvas, getWindowPosition, const char*, (), ,
   "Returns this canvas's platform window's current screen position, as \"x y\".")
{
   Point2I p = object->getWindowPosition();
   static const U32 bufSize = 32;
   char* buf = Con::getReturnBuffer(bufSize);
   dSprintf(buf, bufSize, "%d %d", p.x, p.y);
   return buf;
}

DefineEngineMethod(NewGuiCanvas, setWindowFullscreen, void, (bool fullscreen), ,
   "Switches this canvas's platform window into or out of fullscreen mode.")
{
   object->setWindowFullscreen(fullscreen);
}

DefineEngineMethod(NewGuiCanvas, isWindowFullscreen, bool, (), ,
   "Returns true if this canvas's platform window is currently fullscreen.")
{
   return object->isWindowFullscreen();
}

DefineEngineMethod(NewGuiCanvas, minimizeWindow, void, (), ,
   "Minimizes this canvas's platform window.")
{
   object->minimizeWindow();
}

DefineEngineMethod(NewGuiCanvas, maximizeWindow, void, (), ,
   "Maximizes this canvas's platform window.")
{
   object->maximizeWindow();
}

DefineEngineMethod(NewGuiCanvas, restoreWindow, void, (), ,
   "Restores this canvas's platform window from a minimized/maximized state.")
{
   object->restoreWindow();
}

DefineEngineMethod(NewGuiCanvas, hideWindow, void, (), ,
   "Hides this canvas's platform window.")
{
   object->hideWindow();
}

DefineEngineMethod(NewGuiCanvas, showWindow, void, (), ,
   "Shows this canvas's platform window.")
{
   object->showWindow();
}

DefineEngineMethod(NewGuiCanvas, setWindowCursorVisible, void, (bool visible), ,
   "Sets whether the OS mouse cursor is visible over this canvas's platform window.")
{
   object->setWindowCursorVisible(visible);
}

DefineEngineMethod(NewGuiCanvas, isWindowCursorVisible, bool, (), ,
   "Returns true if the OS mouse cursor is currently visible over this canvas's platform window.")
{
   return object->isWindowCursorVisible();
}

DefineEngineMethod(NewGuiCanvas, setDebugShowHitRegions, void, (bool enable), ,
   "Toggles the debug hit-region overlay: outlines every control's real mBounds (the exact rect "
   "findHitControl() tests a point against), color-coded green (normal), yellow (hitTestable = "
   "false - passes clicks through), or red (invisible/inactive - this whole subtree can never be "
   "hit), plus each control's name. Useful for diagnosing a control that renders somewhere it "
   "can't actually be clicked.")
{
   object->setDebugShowHitRegions(enable);
}

DefineEngineMethod(NewGuiCanvas, getDebugShowHitRegions, bool, (), ,
   "Returns true if the debug hit-region overlay is currently enabled.")
{
   return object->getDebugShowHitRegions();
}

DefineEngineMethod(NewGuiCanvas, setDebugHitRegionFilter, void, (const char* filter), ,
   "Sets a case-insensitive substring filter for the debug hit-region overlay (see "
   "setDebugShowHitRegions) - only controls whose name, class, or Sim id contains this string are "
   "drawn. Pass an empty string to clear the filter and draw everything again. Ignored while hover "
   "chain mode (setDebugHitRegionHoverOnly) is on.")
{
   object->setDebugHitRegionFilter(filter);
}

DefineEngineMethod(NewGuiCanvas, getDebugHitRegionFilter, const char*, (), ,
   "Returns the debug hit-region overlay's current name/class/id filter string, or \"\" if unset.")
{
   return object->getDebugHitRegionFilter();
}

DefineEngineMethod(NewGuiCanvas, setDebugHitRegionHoverOnly, void, (bool enable), ,
   "Toggles hover-chain mode for the debug hit-region overlay: draws only the ancestor chain down "
   "to whatever control the mouse currently sits over, instead of the whole tree - useful for "
   "probing one spot in a busy/overlapping area (e.g. a stack of open popups) without every other "
   "control's outline cluttering the view. Overrides setDebugHitRegionFilter while on.")
{
   object->setDebugHitRegionHoverOnly(enable);
}

DefineEngineMethod(NewGuiCanvas, getDebugHitRegionHoverOnly, bool, (), ,
   "Returns true if the debug hit-region overlay's hover-chain mode is currently enabled.")
{
   return object->getDebugHitRegionHoverOnly();
}

DefineEngineMethod(NewGuiCanvas, setCursor, void, (NewGuiCursor* cursor), ,
   "Binds cursor as this canvas's active NewGuiCursor image set - any shape it has an image for is drawn instead of the real platform cursor from then on. Pass an empty/NULL cursor to unbind (plain platform cursors for every shape).")
{
   object->setCursor(cursor);
}

DefineEngineMethod(NewGuiCanvas, getCursor, S32, (), ,
   "Returns the ID of this canvas's currently-bound NewGuiCursor, or 0 if none is bound.")
{
   NewGuiCursor* cursor = object->getCursor();
   return cursor ? cursor->getId() : 0;
}

DefineEngineMethod(NewGuiCanvas, getDesignScale, const char*, (), ,
   "Returns the design-resolution scale currently in effect (derived from the content control's own authored Pixels-mode width/height - see setContent()), as \"scaleX scaleY\". Always \"1 1\" if there's no content control or its width/height aren't both Pixels-mode. Excludes DPI scale.")
{
   static const U32 bufSize = 64;
   char* buf = Con::getReturnBuffer(bufSize);
   dSprintf(buf, bufSize, "%g %g", object->getDesignScaleX(), object->getDesignScaleY());
   return buf;
}

DefineEngineMethod(NewGuiCanvas, setFocus, void, (), , "() - Claim OS input focus for this canvas' window.")
{
   PlatformWindow* window = object->getPlatformWindow();
   if (window)
   {
      window->setFocus();
      window->appEvent.trigger(window->getWindowId(), GainFocus);
   }
}

DefineEngineMethod(NewGuiCanvas, setVideoMode, void,
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
      // Width is 0 but height isn't - try to find a matching width.
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
      // Height is 0 but width isn't - try to find a matching height.
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
      // Got a bad size for both dimensions, or one with no match for the other - default back to current.
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

   // These optional params default at vm's construction - leave them alone if not specified.
   if (bitDepth > 0)
      vm.bitDepth = bitDepth;

   if (refreshRate > 0)
      vm.refreshRate = refreshRate;

   if (antialiasLevel > 0)
      vm.antialiasLevel = antialiasLevel;

   object->getPlatformWindow()->setVideoMode(vm);

   Con::setVariable("$pref::Video::mode", vm.toString());
}

// Monitor enumeration/query methods - global queries, not tied to any particular window.
DefineEngineMethod(NewGuiCanvas, getMonitorCount, S32, (), ,
   "Gets the number of monitors attached to the system.\n\n"
   "@return The number of monitors attached to the system, including the default monitor.")
{
   return PlatformWindowManager::get()->getMonitorCount();
}

DefineEngineMethod(NewGuiCanvas, getMonitorName, const char*, (S32 index), ,
   "Gets the name of the requested monitor.\n\n"
   "@param index The monitor index.\n"
   "@return The name of the requested monitor, or \"\" if the platform doesn't provide monitor names.")
{
   return PlatformWindowManager::get()->getMonitorName(index);
}

DefineEngineMethod(NewGuiCanvas, getMonitorRect, RectI, (S32 index), ,
   "Gets the region of the requested monitor, in desktop coordinates.\n\n"
   "@param index The monitor index.\n"
   "@return The rectangular region of the requested monitor.")
{
   return PlatformWindowManager::get()->getMonitorRect(index);
}

DefineEngineMethod(NewGuiCanvas, getMonitorUsableRect, RectI, (S32 index), ,
   "Gets the usable desktop area of the requested monitor (its full region minus any space reserved by the system - e.g. a taskbar or menu bar).\n\n"
   "@param index The monitor index.\n"
   "@return The rectangular usable region of the requested monitor.")
{
   return PlatformWindowManager::get()->getMonitorUsableRect(index);
}

DefineEngineMethod(NewGuiCanvas, findFirstMatchingMonitor, S32, (const char* name), ,
   "Finds the first monitor index whose name matches `name` (match algorithm is platform-defined).\n\n"
   "@param name The name to search for.\n"
   "@return The matching monitor index, or -1 if none matched.")
{
   return PlatformWindowManager::get()->findFirstMatchingMonitor(name);
}

// Per-monitor video mode queries - monitorIndex defaults to -1, meaning this canvas's window's current monitor.
DefineEngineMethod(NewGuiCanvas, getMonitorModeCount, S32, (S32 monitorIndex), (-1),
   "Gets the number of display modes available on a monitor.\n\n"
   "@param monitorIndex The monitor index, or -1 (default) to use this canvas's window's current monitor.\n"
   "@return The number of display modes, or 0 if the platform doesn't provide monitor mode information, or if -1 was passed and this canvas has no window yet.")
{
   if (monitorIndex < 0)
   {
      if (!object->getPlatformWindow())
         return 0;
      monitorIndex = object->getPlatformWindow()->getCurrentMonitorIndex();
      if (monitorIndex < 0)
         return 0;
   }

   return PlatformWindowManager::get()->getMonitorModeCount((U32)monitorIndex);
}

DefineEngineMethod(NewGuiCanvas, getMonitorMode, const char*, (S32 modeIndex, S32 monitorIndex), (-1),
   "Gets a display mode string for a specific monitor.\n\n"
   "@param modeIndex The mode index (0..getMonitorModeCount()-1).\n"
   "@param monitorIndex The monitor index, or -1 (default) to use this canvas's window's current monitor.\n"
   "@return The mode as a GFXVideoMode-style string, or \"\" if unavailable.")
{
   if (monitorIndex < 0)
   {
      if (!object->getPlatformWindow())
         return "";
      monitorIndex = object->getPlatformWindow()->getCurrentMonitorIndex();
      if (monitorIndex < 0)
         return "";
   }

   char* buf = Con::getReturnBuffer(PlatformWindowManager::get()->getMonitorMode((U32)monitorIndex, (U32)modeIndex));
   return buf;
}

DefineEngineMethod(NewGuiCanvas, getMonitorDesktopMode, const char*, (S32 monitorIndex), (-1),
   "Gets the current desktop display mode for a specific monitor (the mode the OS desktop itself is running at, independent of this canvas's own current video mode).\n\n"
   "@param monitorIndex The monitor index, or -1 (default) to use this canvas's window's current monitor.\n"
   "@return The desktop mode as a GFXVideoMode-style string, or \"\" if unavailable.")
{
   if (monitorIndex < 0)
   {
      if (!object->getPlatformWindow())
         return "";
      monitorIndex = object->getPlatformWindow()->getCurrentMonitorIndex();
      if (monitorIndex < 0)
         return "";
   }

   char* buf = Con::getReturnBuffer(PlatformWindowManager::get()->getMonitorDesktopMode((U32)monitorIndex));
   return buf;
}

// Device/adapter-wide mode queries - what the GPU can output at all, distinct from any monitor's own supported modes.
DefineEngineMethod(NewGuiCanvas, getVideoMode, const char*, (), ,
   "Gets the current video mode of this canvas's window as a string.\n\n"
   "@return \"width height fullscreen bitDepth refreshRate antialiasLevel\", or \"\" if this canvas has no window yet.")
{
   if (!object->getPlatformWindow())
      return "";

   GFXVideoMode vm = object->getPlatformWindow()->getVideoMode();
   char* buf = Con::getReturnBuffer(vm.toString());
   return buf;
}

DefineEngineMethod(NewGuiCanvas, getModeCount, S32, (), ,
   "Gets the number of video modes this canvas's window's GFX device supports.\n\n"
   "@return The number of supported video modes, or 0 if this canvas has no window yet.")
{
   if (!object->getPlatformWindow())
      return 0;

   const Vector<GFXVideoMode>* const modeList = object->getPlatformWindow()->getGFXDevice()->getVideoModeList();
   return modeList->size();
}

DefineEngineMethod(NewGuiCanvas, getMode, const char*, (S32 modeIndex), ,
   "Gets a specific video mode this canvas's window's GFX device supports.\n\n"
   "@param modeIndex The mode index (0..getModeCount()-1).\n"
   "@return The mode as a GFXVideoMode-style string, or \"\" if modeIndex is out of range or this canvas has no window yet.")
{
   if (!object->getPlatformWindow())
      return "";

   const Vector<GFXVideoMode>* const modeList = object->getPlatformWindow()->getGFXDevice()->getVideoModeList();

   if (modeIndex < 0 || modeIndex >= modeList->size())
   {
      Con::errorf("NewGuiCanvas::getMode - index %d out of range [0, %d).", modeIndex, modeList->size());
      return "";
   }

   GFXVideoMode vm = (*modeList)[modeIndex];
   char* buf = Con::getReturnBuffer(vm.toString());
   return buf;
}
