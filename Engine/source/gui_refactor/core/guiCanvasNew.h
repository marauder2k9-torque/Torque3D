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

#ifndef _GUICANVASNEW_H_
#define _GUICANVASNEW_H_

#ifndef _SIMBASE_H_
#include "sim/simBase.h"
#endif
#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif
#ifndef _PLATFORMINPUT_H_
#include "platform/platformInput.h"
#endif

#ifndef _SIGNAL_H_
#include "core/util/tSignal.h"
#endif

#include "platform/input/IProcessInput.h"
#include "windowManager/platformWindowMgr.h"
#include "gfx/gfxFence.h"
#ifndef _GUIRENDERBATCH_H_
#include "gui_refactor/core/guiRenderBatch.h"
#endif

/// A canvas on which rendering occurs.
///
///
/// @section GuiCanvas_contents What a GUICanvas Can Contain...
///
/// @subsection GuiCanvas_content_contentcontrol Content Control
/// A content control is the top level GuiControlNew for a screen. This GuiControlNew
/// will be the parent control for all other GuiControls on that particular
/// screen.
///
/// @subsection GuiCanvas_content_dialogs Dialogs
///
/// A dialog is essentially another screen, only it gets overlaid on top of the
/// current content control, and all input goes to the dialog. This is most akin
/// to the "Open File" dialog box found in most operating systems. When you
/// choose to open a file, and the "Open File" dialog pops up, you can no longer
/// send input to the application, and must complete or cancel the open file
/// request. Torque keeps track of layers of dialogs. The dialog with the highest
/// layer is on top and will get all the input, unless the dialog is
/// modeless, which is a profile option.
///
/// @see GuiControlProfile
///
/// @section GuiCanvas_dirty Dirty Rectangles
///
/// The GuiCanvasNew is based on dirty regions.
///
/// Every frame the canvas paints only the areas of the canvas that are 'dirty'
/// or need updating. In most cases, this only is the area under the mouse cursor.
/// This is why if you look in guiCanvasNew.cc the call to glClear is commented out.
/// If you want a really good idea of what exactly dirty regions are and how they
/// work, un-comment that glClear line in the renderFrame method of guiCanvasNew.cc
///
/// What you will see is a black screen, except in the dirty regions, where the
/// screen will be painted normally. If you are making an animated GuiControlNew
/// you need to add your control to the dirty areas of the canvas.
///
class guiCanvasNew;
class Point2I;
typedef Signal<void(GuiCanvasNew* canvas)> CanvasNewSizeChangeSignal;
typedef Signal<void(GuiCanvasNew* canvas, bool isActive)> CanvasNewSetActiveSignal;

class GuiCanvasNew : public GuiControlNew, public IProcessInput
{

protected:
   typedef GuiControlNew Parent;

   /// @name Rendering
   /// @{
   RectI      mOldUpdateRects[2];
   RectI      mCurUpdateRect;
   U32        mLastRenderMs;
   /// @}

   /// @name Design-size / logical scale
   ///
   /// A gui is authored at a specific resolution -- the size of whatever
   /// control is passed to setContentControl() at the moment it's set is
   /// treated as that gui's "design size," the reference resolution every
   /// logical unit in the whole control tree is ultimately expressed
   /// relative to. Re-captured every time setContentControl() is called
   /// with a new control, so switching to a differently-designed screen
   /// (e.g. a menu authored at 1280x720 to a HUD authored at 1920x1080)
   /// re-anchors correctly rather than inheriting the previous screen's
   /// baseline.
   /// @{

   /// The content control's logical extent at the moment it was set --
   /// see setContentControl(). (0,0) means no design size captured yet
   /// (e.g. before any content control has ever been set), in which case
   /// getEffectiveScaleX()/Y() return 1.0 and no scaling is applied.
   Point2I mDesignSize;

   /// The window's DPI scale (see PlatformWindow::getDPIScale()) at the
   /// moment mDesignSize was captured -- lets getEffectiveScaleX()/Y()
   /// separate "the window got bigger/smaller" from "this monitor has
   /// different pixel density than whatever monitor the gui was designed
   /// on."
   F32 mDesignDPIScale;

   /// When true (the default), getEffectiveScaleX()/Y() are forced equal
   /// (the more-constrained axis wins) so the content control's aspect
   /// ratio is preserved, letterboxing/pillarboxing into the window
   /// rather than stretching independently per axis. When false, X and Y
   /// scale independently to always exactly fill the window.
   bool mLockAspectRatio;

   /// Recomputes and returns the combined design-size x DPI scale factor
   /// every control under this canvas should use to convert logical units
   /// to device pixels this frame -- see GuiControlNew::getDeviceBounds().
   /// Also updates mContentDeviceRect (the content control's device-pixel
   /// placement within the window, accounting for letterbox centering
   /// when mLockAspectRatio is set).
   void _updateEffectiveScale();

   /// The content control's resolved device-pixel rect within the actual
   /// window -- when mLockAspectRatio letterboxes, this is smaller than
   /// (and centered within) the full window rect. Recomputed by
   /// _updateEffectiveScale(); see getContentDeviceRect().
   RectI mContentDeviceRect;

   /// Current per-axis scale factors -- see _updateEffectiveScale(). Equal
   /// to each other when mLockAspectRatio is true; independent when false.
   F32 mEffectiveScaleX;
   F32 mEffectiveScaleY;

   /// @}

   /// @name Cursor Properties
   /// @{

   bool        mCursorEnabled;
   bool        mShowCursor;
   bool        mRenderFront;
   Point2F     mCursorPt;                             ///< Current cursor position in local coordinates.
   Point2I     mLastCursorPt;
   GuiCursor* mDefaultCursor;
   GuiCursor* mLastCursor;
   bool        mLastCursorEnabled;
   bool        mForceMouseToGUI;
   bool        mClampTorqueCursor;
   bool        mAlwaysHandleMouseButtons;

   bool        mDisplayWindow;

   /// @}

   /// @name Mouse Input
   /// @{

   SimObjectPtr<GuiControlNew>   mMouseCapturedControl;  ///< All mouse events will go to this ctrl only
   SimObjectPtr<GuiControlNew>   mMouseControl;          ///< the control the mouse was last seen in unless some other one captured it
   bool                       mMouseControlClicked;   ///< whether the current ctrl has been clicked - used by helpctrl
   U32                        mPrevMouseTime;         ///< this determines how long the mouse has been in the same control
   bool                       mMouseButtonDown;       ///< Flag to determine if the button is depressed
   bool                       mMouseRightButtonDown;  ///< bool to determine if the right button is depressed
   bool                       mMouseMiddleButtonDown; ///< Middle button flag
   GuiEvent                   mLastEvent;

   U8                         mLastMouseClickCount;
   S32                        mLastMouseDownTime;
   bool                       mLeftMouseLast;
   bool                       mMiddleMouseLast;
   bool                       mRightMouseLast;
   Point2F                    mMouseDownPoint;

   /// Processes keyboard input events. Helper method for processInputEvent
   ///
   /// \param inputEvent Information on the input even to be processed.
   /// \return True if the event was handled or false if it was not.
   virtual bool processKeyboardEvent(InputEventInfo& inputEvent);

   /// Processes mouse input events. Helper method for processInputEvent
   ///
   /// \param inputEvent Information on the input even to be processed.
   /// \return True if the event was handled or false if it was not.
   virtual bool processMouseEvent(InputEventInfo& inputEvent);

   /// Processes gamepad input events. Helper method for processInputEvent
   ///
   /// \param inputEvent Information on the input even to be processed.
   /// \return True if the event was handled or false if it was not.
   virtual bool processGamepadEvent(InputEventInfo& inputEvent);

   virtual void findMouseControl(const GuiEvent& event);
   virtual void refreshMouseControl();
   /// @}

   /// @name Keyboard Input
   /// @{

   /// Accelerator key map
   struct AccKeyMap
   {
      GuiControlNew* ctrl;
      U32 index;
      U32 keyCode;
      U32 modifier;
   };
   Vector <AccKeyMap> mAcceleratorMap;

   //for tooltip rendering
   U32            mHoverControlStart;
   GuiControlNew* mHoverControl;
   Point2I        mHoverPosition;
   bool           mHoverPositionSet;
   U32            mHoverLeftControlTime;

public:
   /// Setting for how to handle 'enableKeyboardTranslation' and 'setNativeAcceleratorsEnabled' requests.
   enum KeyTranslationMode
   {
      TranslationMode_Platform,
      TranslationMode_Callback,
      TranslationMode_Ignore,
   };

protected:
   KeyTranslationMode mKeyTranslationMode;
   KeyTranslationMode mNativeAcceleratorMode;

   /// @}

   // Internal event handling callbacks for use with PlatformWindow.
   void handleResize(WindowId did, S32 width, S32 height);
   void handleAppEvent(WindowId did, S32 event);
   void handlePaintEvent(WindowId did);

   PlatformWindow* mPlatformWindow;
   GFXFence** mFences;
   S32 mNextFenceIdx;
   S32 mNumFences;

   static bool setProtectedNumFences(void* object, const char* index, const char* data);
   virtual void setupFences();

   void checkLockMouseMove(const GuiEvent& event);
   //Signal used to let others know this canvas has changed size.
   static CanvasNewSizeChangeSignal smCanvasNewSizeChangeSignal;
   static CanvasNewSetActiveSignal smCanvasNewSetActiveSignal;

   GuiControlNew* mMenuBarCtrl;
   GuiControlNew* mMenuBackground;
   bool mConstrainMouse;

   /// The per-frame batch collector every control's onRender() submits
   /// draw primitives into instead of calling GFXDrawUtil directly -- see
   /// guiRenderBatch.h. Owned here (one per canvas, matching how
   /// mDefaultGuiSB is already one-per-canvas) since batching is
   /// inherently a per-frame, whole-tree concern that belongs at the
   /// root, not on any individual control.
   GuiRenderBatch mRenderBatch;

   typedef Signal< void(SetModification modification, SimSet* set, SimObject* object) > SetModificationSignal;
public:
   DECLARE_CONOBJECT(GuiCanvasNew);
   DECLARE_CATEGORY("Gui Core");

   GuiCanvasNew();
   virtual ~GuiCanvasNew();

   bool onAdd() override;
   void onRemove() override;
#ifdef TORQUE_TOOLS
   void setMenuBar(SimObject* obj);
   SimObject* getMenuBar() { return mMenuBarCtrl; }
#endif
   static void initPersistFields();

   /// The current frame's render batch -- every control's onRender()
   /// should submit into this (via getRoot()->getRenderBatch() or an
   /// equivalent path down from whatever canvas it belongs to) rather
   /// than calling GFXDrawUtil directly. See guiRenderBatch.h.
   GuiRenderBatch& getRenderBatch() { return mRenderBatch; }

   static CanvasNewSizeChangeSignal& getCanvasSizeChangeSignal() { return smCanvasNewSizeChangeSignal; }
   static CanvasNewSetActiveSignal& getCanvasSetActiveSignal() { return smCanvasNewSetActiveSignal; }

   /// @name Rendering methods
   ///
   /// @{
   void constrainMouse(bool constrained) { mConstrainMouse = constrained; };
   void constrainMouseCoords(Point2I mousePoint);

   /// Repaints the dirty regions of the canvas
   /// @param   preRenderOnly   If set to true, only the onPreRender methods of all the GuiControls will be called
   /// @param   bufferSwap      If set to true, it will swap buffers at the end. This is to support canvas-subclassing.
   virtual void renderFrame(bool preRenderOnly, bool bufferSwap = true);


   /// Repaints the canvas by calling the platform window display event.
   virtual void paint();

   /// Repaints the canvas skipping rendering if the target time
   /// has not yet elapsed.
   /// @param  elapsedMS The time since the last frame.
   virtual void repaint(U32 elapsedMS);

   /// This signal is triggered at the beginning and end of each render frame
   ///
   /// @param beginFrame true at the beginning of the frame, false at the end
   ///
   typedef Signal <void(bool beginFrame)> GuiCanvasFrameSignal;

   static GuiCanvasFrameSignal& getGuiCanvasFrameSignal();

   /// Adds a dirty area to the canvas so it will be updated on the next frame
   /// @param   pos   Screen-coordinates of the upper-left hand corner of the dirty area
   /// @param   ext   Width/height of the dirty area
   virtual void addUpdateRegion(Point2I pos, Point2I ext);

   /// Resets the update regions so that the next call to renderFrame will
   /// repaint the whole canvas
   virtual void resetUpdateRegions();

   /// Resizes the content control to match the canvas size.
   void maintainSizing();

   /// This builds a rectangle which encompasses all of the dirty regions to be
   /// repainted
   /// @param   updateUnion   (out) Rectangle which surrounds all dirty areas
   virtual void buildUpdateUnion(RectI* updateUnion);

   /// This will swap the buffers at the end of renderFrame. It was added for canvas
   /// sub-classes in case they wanted to do some custom code before the buffer
   /// flip occured.
   virtual void swapBuffers();

   /// @}

   /// @name Canvas Content Management
   /// @{

   /// This returns the PlatformWindow owned by this Canvas
   virtual PlatformWindow* getPlatformWindow()
   {
      return mPlatformWindow;
   }

   /// This sets the content control to something different
   /// @param   gui   New content control
   virtual void setContentControl(GuiControlNew* gui);

   /// Returns the content control
   virtual GuiControlNew* getContentControl();

   /// Adds a dialog control onto the stack of dialogs
   /// @param   gui   Dialog to add
   /// @param   layer   Layer to put dialog on
   /// @param   center  Center dialog on canvas.
   virtual void pushDialogControl(GuiControlNew* gui, S32 layer = 0, bool center = false);

   /// Removes a specific layer of dialogs
   /// @param   layer   Layer to pop off from
   virtual void popDialogControl(S32 layer = 0);

   /// Removes a specific dialog control
   /// @param   gui   Dialog to remove from the dialog stack
   virtual void popDialogControl(GuiControlNew* gui);
   ///@}

   /// This turns on/off front-buffer rendering
   /// @param   front   True if all rendering should be done to the front buffer
   virtual void setRenderFront(bool front) { mRenderFront = front; }

   /// @name Cursor commands
   /// A cursor can be on, but not be shown. If a cursor is not on, than it does not
   /// process input.
   /// @{

   /// Sets the cursor for the canvas.
   /// @param   cursor   New cursor to use.
   virtual void setCursor(GuiCursor* cursor);
   S32 mCursorChanged;

   /// Returns true if the cursor is on.
   virtual bool isCursorON() { return mCursorEnabled; }

   /// Sets if mouse events should be passed to the GUI even if the cursor is off.
   /// @param   onOff   True if events should be passed to the GUI if the cursor is off
   virtual void setForceMouseToGUI(bool onOff);

   /// Sets if the Torque cursor should be clamped to the window.
   /// @param  onOff    True if the Torque cursor should be clamped against the window
   virtual void setClampTorqueCursor(bool onOff);

   /// Returns if the Torque cursor is clamped to the window
   virtual bool getClampTorqueCursor() { return mClampTorqueCursor; }

   /// Turns the cursor on or off.
   /// @param   onOff   True if the cursor should be on.
   virtual void setCursorON(bool onOff);

   /// Sets the position of the cursor
   /// @param   pt   Point, in screenspace for the cursor
   virtual void setCursorPos(const Point2I& pt);

   /// Returns the point, in screenspace, at which the cursor is located.
   virtual Point2I getCursorPos();

   /// Returns the point, in local coordinates, at which the cursor is located
   virtual Point2I getCursorPosLocal() { return Point2I(S32(mCursorPt.x), S32(mCursorPt.y)); }

   /// Enable/disable rendering of the cursor.
   /// @param   state    True if we should render cursor
   virtual void showCursor(bool state);

   /// Returns true if the cursor is being rendered.
   virtual bool isCursorShown();

   void cursorClick(S32 buttonId, bool isDown);

   void cursorNudge(F32 x, F32 y);
   /// @}

   ///used by the tooltip resource
   Point2I getCursorExtent() { return mDefaultCursor->getExtent(); }

   /// @name Input Processing
   /// @{

   /// Processes an input event
   /// @see InputEvent
   /// @param   event   Input event to process
   bool processInputEvent(InputEventInfo& inputEvent) override;
   /// @}

   /// @name Mouse Methods
   /// @{

   /// When a control gets the mouse lock this means that that control gets
   /// ALL mouse input and no other control receives any input.
   /// @param   lockingControl   Control to lock mouse to
   virtual void mouseLock(GuiControlNew* lockingControl);

   /// Unlocks the mouse from a control
   /// @param   lockingControl   Control to unlock from
   virtual void mouseUnlock(GuiControlNew* lockingControl);

   /// Returns the control which the mouse is over
   virtual GuiControlNew* getMouseControl() { return mMouseControl; }

   /// Returns the control which the mouse is locked to if any
   virtual GuiControlNew* getMouseLockedControl() { return mMouseCapturedControl; }

   /// Returns true if the left mouse button is down
   virtual bool mouseButtonDown(void) { return mMouseButtonDown; }

   /// Returns true if the right mouse button is down
   virtual bool mouseRightButtonDown(void) { return mMouseRightButtonDown; }

   /// @}

   /// @name Mouse input methods
   /// These events process the events before passing them down to the
   /// controls they effect. This allows for things such as the input
   /// locking and such.
   ///
   /// Each of these methods corresponds to the action in it's method name
   /// and processes the GuiEvent passed as a parameter
   /// @{
   virtual void rootMouseUp(const GuiEvent& event);
   virtual void rootMouseDown(const GuiEvent& event);
   virtual void rootMouseMove(const GuiEvent& event);
   virtual void rootMouseDragged(const GuiEvent& event);

   virtual void rootRightMouseDown(const GuiEvent& event);
   virtual void rootRightMouseUp(const GuiEvent& event);
   virtual void rootRightMouseDragged(const GuiEvent& event);

   virtual void rootMiddleMouseDown(const GuiEvent& event);
   virtual void rootMiddleMouseUp(const GuiEvent& event);
   virtual void rootMiddleMouseDragged(const GuiEvent& event);

   virtual bool rootMouseWheelUp(const GuiEvent& event);
   virtual bool rootMouseWheelDown(const GuiEvent& event);
   /// @}

   /// @name Keyboard input methods
   /// First responders
   ///
   /// A first responder is a the GuiControlNew which responds first to input events
   /// before passing them off for further processing.
   /// @{

   /// Moves the first responder to the next tabable controle
   virtual bool tabNext(void);

   /// Moves the first responder to the previous tabable control
   virtual bool tabPrev(void);

   /// Setups a keyboard accelerator which maps to a GuiControlNew.
   ///
   /// @param   ctrl       GuiControlNew to map to.
   /// @param   index
   /// @param   keyCode    Key code.
   /// @param   modifier   Shift, ctrl, etc.
   virtual void addAcceleratorKey(GuiControlNew* ctrl, U32 index, U32 keyCode, U32 modifier);

   /// Sets the first responder.
   /// @param   firstResponder    Control to designate as first responder
   void setFirstResponder(GuiControlNew* firstResponder) override;

   /// This is used to toggle processing of native OS accelerators, not
   /// to be confused with the Torque accelerator key system, to keep them
   /// from swallowing up keystrokes.  Both GuiTextEditCtrl and GuiTextPadCtrl
   /// use this method.
   virtual void setNativeAcceleratorsEnabled(bool enabled);
   /// @}

   /// 
   virtual Point2I getWindowSize();

   /// Returns the per-axis design-size x DPI scale factor every control
   /// under this canvas should use to convert its own logical units to
   /// device pixels this frame. 1.0 if no content control has been set
   /// yet (see mDesignSize). Equal to each other when mLockAspectRatio is
   /// true; independent when false (see _updateEffectiveScale()). See
   /// GuiControlNew::getDeviceBounds(), which is what actually consumes
   /// these.
   F32 getEffectiveScaleX() const { return mEffectiveScaleX; }
   F32 getEffectiveScaleY() const { return mEffectiveScaleY; }

   /// Returns the design (reference) logical size captured from the
   /// content control the last time setContentControl() was called.
   /// (0,0) if no content control has ever been set.
   const Point2I& getDesignSize() const { return mDesignSize; }

   /// Returns the content control's resolved device-pixel placement
   /// within the actual window -- see mContentDeviceRect.
   const RectI& getContentDeviceRect() const { return mContentDeviceRect; }

   /// When true, the content control's aspect ratio is preserved
   /// (letterboxed/pillarboxed) rather than stretched independently per
   /// axis to fill the window. See mLockAspectRatio.
   bool getLockAspectRatio() const { return mLockAspectRatio; }
   void setLockAspectRatio(bool value) { mLockAspectRatio = value; _updateEffectiveScale(); }

   /// Diagnostic utility -- prints this canvas's live design-size/scale
   /// state to the console. Callable from script as Canvas.dumpScaleDebug().
   void dumpScaleDebug();

   /// Converts a point in DEVICE (raw window/screen) pixels -- e.g.
   /// mCursorPt, or anything else sourced directly from platform mouse
   /// input -- into LOGICAL coordinates, relative to the content
   /// control's design-space origin. This is the single conversion point
   /// every raw input coordinate should pass through exactly once, right
   /// where it's captured (see processMouseEvent()), so everything
   /// downstream -- GuiEvent::mousePoint, findHitControl(), individual
   /// controls' event handlers -- can consistently assume logical units,
   /// matching how those controls already think about their own bounds.
   /// Uses independent X/Y scale (see mEffectiveScaleX/Y) so this stays
   /// correct even when mLockAspectRatio is false.
   Point2I deviceToLogicalPoint(const Point2I& devicePoint) const
   {
      if (mEffectiveScaleX <= 0.0f || mEffectiveScaleY <= 0.0f)
         return devicePoint;

      return Point2I(
         (S32)((devicePoint.x - mContentDeviceRect.point.x) / mEffectiveScaleX),
         (S32)((devicePoint.y - mContentDeviceRect.point.y) / mEffectiveScaleY));
   }

   /// Inverse of deviceToLogicalPoint(): projects a point already
   /// expressed in CANVAS-GLOBAL logical coordinates (i.e. the output of
   /// GuiControlNew::localToGlobalCoord()) into device (physical screen)
   /// pixels, by applying mContentDeviceRect and the effective
   /// logical->device scale. This is the one place that projection
   /// happens -- every control's GuiControlNew::getDeviceBounds() and
   /// setUpdateRegion() route through this rather than each re-deriving
   /// the same scale/offset math.
   Point2I localToGlobal(const Point2I& logicalPoint) const
   {
      return Point2I(
         mContentDeviceRect.point.x + (S32)(logicalPoint.x * mEffectiveScaleX),
         mContentDeviceRect.point.y + (S32)(logicalPoint.y * mEffectiveScaleY));
   }

   virtual void enableKeyboardTranslation();
   virtual void disableKeyboardTranslation();

   virtual void setWindowTitle(const char* newTitle);

   DECLARE_CALLBACK(bool, onSetKeyboardTranslationEnabled, (bool enable));
   DECLARE_CALLBACK(bool, onSetNativeAcceleratorsEnabled, (bool enable));


private:
   static const U32 MAX_GAMEPADS = 4; ///< The maximum number of supported gamepads
protected:
   bool   mConsumeLastInputEvent;
   S32    mLastInputDeviceType;
public:
   void clearMouseRightButtonDown(void) { mMouseRightButtonDown = false; }
   void clearMouseButtonDown(void) { mMouseButtonDown = false; }
   void setConsumeLastInputEvent(bool flag) { mConsumeLastInputEvent = flag; }
   bool getLastCursorPoint(Point2I& pt) const { pt = mLastCursorPt; return mLastCursorEnabled; }

   StringTableEntry getLastInputDeviceType();

   void setActive(bool value) override;
};
typedef GuiCanvasNew::KeyTranslationMode NewKeyboardTranslationMode;
DefineEnumType(NewKeyboardTranslationMode);

#endif
