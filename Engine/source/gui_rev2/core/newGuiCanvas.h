//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiCanvas.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUICANVAS_H_
#define _NEWGUICANVAS_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#ifndef _NEWGUIINPUTEVENT_H_
#include "gui_rev2/core/newGuiInputEvent.h"
#endif
#ifndef _NEWGUICURSOR_H_
#include "gui_rev2/core/newGuiCursor.h"
#endif
#ifndef _I_PROCESSINPUT_H_
#include "platform/input/IProcessInput.h"
#endif
#ifndef _EVENT_H_
#include "platform/input/event.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif

class PlatformWindow;
class NewGuiRenderBatch;
class GFXDevice;

/// The root of a GUI tree. Owns the frame driver (StylePass ->
/// MeasurePass -> ArrangePass -> RenderPass, once per dirty frame) and
/// the platform input bridge (IProcessInput - translates raw
/// InputEventInfo into NewGuiInputEvent and dispatches it). A canvas is
/// itself a NewGuiControl - the root of the tree is just a control with
/// no parent.
///
/// @code
/// new NewGuiCanvas( Canvas );
/// Canvas.setContent( MyRootPanel );
/// @endcode
class NewGuiCanvas : public NewGuiControl, public IProcessInput
{
public:

   typedef NewGuiControl Parent;

protected:

   PlatformWindow* mPlatformWindow;     ///< Window this canvas is bound to. May be NULL (headless canvas).
   bool mOwnsPlatformWindow;            ///< True if this canvas created mPlatformWindow itself, rather than being handed one.
   GFXDevice* mGFXDevice;               ///< Device created/bound during onAdd()'s bootstrap.
   bool mAutoRenderRegistered;          ///< True if subscribed to Process::notify() - see setAutoRender().

   /// Every currently-registered NewGuiCanvas, added/removed in onAdd()/
   /// onRemove(). Nothing in this system needed a canvas registry before
   /// - this exists solely so NewLang::setLanguage() has a way to reach
   /// "every GUI tree" without inventing a separate manager singleton.
   static Vector<NewGuiCanvas*> smAllCanvases;

public:

   /// Calls setStyleDirty() on every currently-registered canvas.
   static void setAllCanvasesStyleDirty();

protected:

   /// One render batch per canvas; begin()/flush() bracket RenderPass() once per renderFrame() call.
   NewGuiRenderBatch* mRenderBatch;

   /// Design-resolution scale, independent per axis (see mPreserveAspect
   /// for the per-control uniform-scale opt-out). Derived from the
   /// content control's own authored Pixels-mode width/height; excludes
   /// DPI scale, which is combined in fresh at ArrangePass() time.
   F32 mDesignScaleX;
   F32 mDesignScaleY;

   /// Recomputes mDesignScaleX/Y against actualExtent from the content
   /// control's own authored width/height. An axis stays 1.0 (unscaled)
   /// unless the content control's dimension on that axis is Pixels-mode.
   /// @param actualExtent Real window client extent to scale against.
   void recomputeDesignScale(const Point2I& actualExtent);

   /// Window dpiChangeEvent handler - DPI can change without a resize
   /// (e.g. dragging to a different monitor), so this needs its own
   /// subscription. Marks arrangement dirty; mDesignScaleX/Y themselves
   /// don't depend on DPI.
   void onWindowDPIChange(PlatformWindow* window, F32 newScale);

   SimObjectPtr<NewGuiCursor> mCurrentCursor;      ///< Currently-bound cursor image set, or NULL.
   Vector<NewGuiCursorShape> mControlCursorStack;  ///< Real stack of pushed shapes, so nested pushCursor() calls unwind correctly.

   /// Applies whatever mControlCursorStack's current top resolves to (or the platform baseline if empty).
   void applyCurrentCursor();

   bool mHasPushedPlatformCursor;   ///< Whether this canvas currently has a shape pushed onto the platform cursor stack.

   /// @return mControlCursorStack's current top, or Arrow if empty.
   NewGuiCursorShape getCurrentCursorShape() const { return mControlCursorStack.empty() ? NewGuiCursorShape::Arrow : mControlCursorStack.last(); }

   // Tree-wide interaction tracking - inherently canvas-owned, since dispatch (enter/leave, capture) spans controls.
   NewGuiControl* mMouseOverControl;      ///< Control the cursor was over as of the last processed mouse event.
   NewGuiControl* mMouseCapturedControl;  ///< Control that owns mouse events until button-up.
   NewGuiControl* mFirstResponder;        ///< Control with keyboard focus.

   /// Activate binding - authored rather than hardcoded so a project can
   /// rebind "activate" without touching engine code. Two keyboard slots
   /// since Enter and Space are both conventional; -1 disables a slot.
   S32 mActivateKeyCode1;
   S32 mActivateKeyCode2;
   S32 mActivateGamepadButton;

   /// One control-authored keyboard accelerator (e.g. NewGuiButton's
   /// "accelerator" field). Fires regardless of what currently has
   /// keyboard focus - see registerAccelerator()/checkAccelerators().
   struct NewGuiAcceleratorBinding
   {
      NewGuiControl* control;
      U16 keyCode;
      U32 modifier;     ///< Already normalized - see NormalizeModifiers().

      NewGuiAcceleratorBinding() : control(NULL), keyCode(0), modifier(0) {}
   };
   Vector<NewGuiAcceleratorBinding> mAccelerators;

   /// Collapses left/right modifier pairs (SI_LSHIFT/SI_RSHIFT, etc)
   /// into the engine's existing logical SI_SHIFT/SI_CTRL/SI_ALT bits,
   /// so "ctrl s" matches either physical Ctrl key. Reuses the same
   /// collapse ActionMap::translateModifiers() already performs
   /// (actionMap.cpp) rather than a second convention. Used identically
   /// by registration (via NewGuiButton::ParseAcceleratorString() ->
   /// ActionMap::createEventDescriptor(), already SI_SHIFT/SI_CTRL/
   /// SI_ALT-flavored) and matching (a live event's raw modifier field),
   /// so the two can never drift apart on which bits mean what.
   /// @param raw Modifier bitmask as delivered on GuiEvent/NewGuiInputEvent.
   /// @return Normalized bitmask - one bit per logical modifier (Shift/Ctrl/Alt), left+right collapsed.
   static U32 NormalizeModifiers(U32 raw);

   /// Checks event against every registered accelerator; on a match,
   /// calls the bound control's onAccelerator() and returns true.
   /// Only ever called for action == Down (see dispatchKeyEvent()).
   /// Skipped entirely while the current first responder wants raw
   /// keyboard input itself (see NewGuiControl::wantsRawKeyboardInput())
   /// - this is what lets a focused text field's own Ctrl+C/X/V reach
   /// it instead of being intercepted as some unrelated control's
   /// accelerator binding.
   /// @param event Event to test.
   /// @return True if a binding matched and consumed the event.
   bool checkAccelerators(const NewGuiInputEvent& event);

   Point2I mLastCursorPos;

   // Tooltip - canvas owns the lifecycle (hover-delay timer, show/hide, positioning, rendering) since it's cross-control state.
   S32 mDefaultTooltipDelayMS;               ///< Default hover delay for a control leaving its own delay at -1.
   NewGuiControl* mTooltipHoverControl;      ///< Control under the mouse that has a tooltip authored (distinct from mMouseOverControl).
   U32 mTooltipHoverStartMS;                 ///< Sim time mTooltipHoverControl was last set to a new control.
   bool mTooltipVisible;                     ///< True once the current tooltip has actually been shown.
   NewGuiControl* mActiveTooltipContent;     ///< Content currently shown - either the control's own mTooltipContent, or mDefaultTooltipBox.

   NewGuiControl* mDefaultTooltipBox;             ///< Owned, reused box for the plain-text tooltip case. Built lazily.
   class NewGuiLabel* mDefaultTooltipLabel;       ///< Child of mDefaultTooltipBox.

   /// Builds mDefaultTooltipBox/mDefaultTooltipLabel on first call, sets the label's text, and returns the box.
   /// @param text Tooltip text to display.
   /// @return mDefaultTooltipBox.
   NewGuiControl* buildDefaultTooltipContent(const char* text);

   SimObjectPtr<NewGuiStyle> mDefaultTooltipStyle;   ///< Style applied to the default plain-text tooltip.

   // --- Debug hit-region overlay --------------------------------------------------------------
   // Draws every control's mBounds - the EXACT rect findHitControl() tests a point against - as an
   // outline, plus its name/class. This is deliberately driven from the canvas rather than each
   // control's own EmitDrawCommands(): findHitControl() is a canvas-owned tree walk (see
   // dispatchMouseEvent()), and mBounds is otherwise only ever used INDIRECTLY by a control's own
   // render (EmitDrawCommands(batch, mBounds, ...)) - drawing it here, once, straight from the
   // canvas after the real render pass, guarantees the overlay shows literally the rect input
   // dispatch will test, with no risk of a hand-copied rect drifting from the real one over time.
   bool mDebugShowHitRegions;   ///< Authored/toggled via debugShowHitRegions - off by default.

   /// Case-insensitive substring filter against a control's name/class/id (same fallback order
   /// the label itself uses - see debugDrawHitRegions()). Empty/NULL (the default) draws
   /// everything. Set via debugHitRegionFilter or setDebugHitRegionFilter(); intended for pulling
   /// one subsystem (e.g. "KSNav", "Popup") out of a busy tree without the rest of the tree's
   /// outlines/labels overlapping it into illegibility.
   StringTableEntry mDebugHitRegionFilter;

   /// When true, draws only the ANCESTOR CHAIN from the tree root down to mMouseOverControl (not
   /// that control's own children/siblings) - "what's under my cursor right now," which stays
   /// readable even over a densely nested/overlapping area (e.g. a stack of open popups) where
   /// drawing the whole tree would just be a wall of overlapping boxes. Takes precedence over
   /// mDebugHitRegionFilter when both are set, since the two are more useful as alternatives
   /// (name search vs live cursor probe) than combined.
   bool mDebugHitRegionHoverOnly;

   Resource<GFont> mDebugFont;   ///< Lazily created on first debug draw; independent of any control's own resolved style.

   /// Ensures mDebugFont is loaded. Safe to call every frame; no-ops once loaded.
   void ensureDebugFont();

   /// Script-field setter for debugHitRegionFilter (see initPersistFields()'s onSet) - routes
   /// through setDebugHitRegionFilter() so script authoring and the C++ setter stay in sync.
   static bool _setDebugHitRegionFilter(void* obj, const char* index, const char* data);

   /// @return True if control's name/class/id contains mDebugHitRegionFilter (case-insensitive),
   /// or mDebugHitRegionFilter is unset. Used by debugDrawHitRegions() to decide whether to draw
   /// a given control at all.
   bool debugHitRegionPasses(NewGuiControl* control) const;

   /// Recurses the same tree RenderPass() just walked, drawing one outline (plus a name label,
   /// space permitting) per control from its OWN mBounds - never a hand-recomputed rect. Runs
   /// AFTER the real render pass and its own clip-rect pushes/pops have fully unwound (see
   /// renderFrame()), so every outline draws unclipped, at a fixed very-high layer, regardless of
   /// what any individual control's own clip region was during its real render.
   /// @param control Subtree root to draw (recurses into every child, visible or not).
   /// @param batch Canvas's own render batch - already mid-frame (begin() called, flush() pending).
   void debugDrawHitRegions(NewGuiControl* control, NewGuiRenderBatch* batch);

   /// mDebugHitRegionHoverOnly's draw path: walks mMouseOverControl's OWN ancestor chain upward
   /// via getGroup() (cheap - chain depth, not tree size) rather than recursing the whole tree and
   /// checking membership per-node, then draws that chain root-to-leaf so a nested popup still
   /// draws over its parent, same paint order the whole-tree walk would have produced for that
   /// same chain.
   /// @param batch Canvas's own render batch.
   void debugDrawHoverChain(NewGuiRenderBatch* batch);

   /// Shared by debugDrawHitRegions()/debugDrawHoverChain() - draws exactly one control's outline
   /// plus (space permitting) its name label. No recursion, no filter check - callers decide which
   /// controls reach this.
   /// @param control Control to draw.
   /// @param batch Canvas's own render batch.
   void debugDrawOneHitRegion(NewGuiControl* control, NewGuiRenderBatch* batch);

   /// Walks up from mMouseOverControl to find the nearest ancestor with
   /// a tooltip authored, updates hover-start tracking on a change, and
   /// shows the tooltip once the resolved delay has elapsed.
   void updateTooltipState();

   /// Positions mActiveTooltipContent near mLastCursorPos, clamped to stay within the window.
   void positionTooltip();

   /// @return True if this canvas or any descendant has a pending dirty flag.
   bool hasAnyDirtyFlag() const;

   // InputEventInfo -> NewGuiInputEvent translation. Each returns false (event ignored) or fills out and returns true.
   bool translateMouseButtonEvent(const InputEventInfo& in, NewGuiInputEvent& out);
   bool translateMouseMoveEvent(const InputEventInfo& in, NewGuiInputEvent& out);
   bool translateMouseWheelEvent(const InputEventInfo& in, NewGuiInputEvent& out);
   bool translateKeyEvent(const InputEventInfo& in, NewGuiInputEvent& out);

   /// Translates a decoded character-input event (WindowInputGenerator::handleCharInput(), fed
   /// through as its own InputEventInfo - see setPlatformWindow()) into a NewGuiInputEvent with
   /// isCharInput == true. Distinct from translateKeyEvent(): this carries a real, insertable,
   /// post-layout/IME-decoded character, not a raw keycode.
   bool translateCharInputEvent(const InputEventInfo& in, NewGuiInputEvent& out);

   /// Which finger slot is currently driving mouse-equivalent dispatch -
   /// only ever the first finger to touch down while none other is primary.
   InputObjectInstances mPrimaryTouchFinger;
   bool mHasPrimaryTouchFinger;

   /// Translates one touch InputEventInfo into a NewGuiInputEvent (deviceKind = Touch).
   /// @return False for any finger other than the primary one.
   bool translateTouchEvent(const InputEventInfo& in, NewGuiInputEvent& out);

   /// Translates one gamepad InputEventInfo into a NewGuiInputEvent (deviceKind = Gamepad). XInput digital buttons only.
   bool translateGamepadEvent(const InputEventInfo& in, NewGuiInputEvent& out);

   /// Fires onMouseEnter/onMouseLeave pairs when the hit-tested control changes, then updates mMouseOverControl.
   void updateMouseOver(NewGuiControl* newOverControl, NewGuiInputEvent& event);

   /// Dispatches a translated mouse event: starts at the captured control
   /// (if any) or the hit-tested control, then bubbles up via getGroup()
   /// until handled (except Move, which never bubbles).
   void dispatchMouseEvent(NewGuiInputEvent& event);

   /// Dispatches a translated key event. Tab/Shift+Tab are intercepted
   /// here before reaching the first responder; every other key
   /// (including activate keys) bubbles via the normal path.
   void dispatchKeyEvent(NewGuiInputEvent& event);

   /// Dispatches a translated gamepad event to the first responder (no pointer position to hit-test against).
   void dispatchGamepadEvent(NewGuiInputEvent& event);

   /// @return True if event should invoke the first responder's onActivate() rather than onInputEvent().
   bool isActivateEvent(const NewGuiInputEvent& event) const;

   /// Moves keyboard focus to the next (or previous, if reverse) tab stop.
   /// @param reverse True to move backward through the tab order.
   void focusNextTabStop(bool reverse);

   // PlatformWindow lifecycle - subscribed in setPlatformWindow(), unsubscribed on teardown.
   void onWindowDisplayEvent(WindowId did);
   void onWindowResizeEvent(WindowId did, S32 width, S32 height);
   void onWindowAppEvent(WindowId did, S32 event);

public:

   NewGuiCanvas();
   virtual ~NewGuiCanvas();

   DECLARE_CONOBJECT(NewGuiCanvas);

   static void initPersistFields();

   bool onAdd() override;
   void onRemove() override;

   /// The canvas is a pure layout root and never paints its own background/border.
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override {}

   /// Binds this canvas to a window: sets mPlatformWindow, resizes to
   /// the window's client extent, registers as the window's input
   /// controller, and subscribes to its lifecycle signals. NULL unbinds.
   /// @param window Window to bind to, or NULL to unbind.
   void setPlatformWindow(PlatformWindow* window);
   PlatformWindow* getPlatformWindow() const { return mPlatformWindow; }

   /// Detaches the current content control (not deleted) and attaches control as the new one.
   /// @param control New content control, or NULL to just detach.
   void setContent(NewGuiControl* control);

   /// @return The control most recently passed to setContent(), or NULL.
   NewGuiControl* getContent() const;

   // PlatformWindow property passthroughs - no-ops when mPlatformWindow is NULL.
   void setWindowTitle(const char* title);
   const char* getWindowTitle() const;

   void setWindowClientExtent(const Point2I& extent);
   Point2I getWindowClientExtent() const;

   void setWindowPosition(const Point2I& position);
   Point2I getWindowPosition() const;

   void setWindowFullscreen(bool fullscreen);
   bool isWindowFullscreen() const;

   void minimizeWindow();
   void maximizeWindow();
   void restoreWindow();
   void hideWindow();
   void showWindow();

   /// Repaints the canvas by triggering the platform window's display event.
   virtual void paint();

   void setWindowCursorVisible(bool visible);
   bool isWindowCursorVisible() const;

   /// Passthrough to PlatformWindow::setAcceleratorsEnabled() - disables native OS keyboard
   /// accelerator translation while enabled == false. A control overriding setFirstResponder()
   /// (e.g. NewGuiTextEdit) calls this via getOwningCanvas() on focus gain/loss - see that
   /// control's class doc for why this is control-driven rather than a canvas-side special case.
   /// No-op if this canvas has no bound window.
   /// @param enabled True to enable native accelerators (the default/unfocused state).
   void enableKeyboardAccelerators(bool enabled);

   /// Passthrough to PlatformWindow::setKeyboardTranslation() - enables platform character-input
   /// (IME) translation. No-op if this canvas has no bound window.
   /// @param enabled True to enable keyboard/IME translation.
   void enableKeyboardTranslation(bool enabled);

   /// Binds cursor as the active image set for this canvas (NULL
   /// unbinds). Resolves its images immediately and re-applies the
   /// currently-requested shape.
   /// @param cursor Cursor image set to bind, or NULL.
   void setCursor(NewGuiCursor* cursor);
   NewGuiCursor* getCursor() const { return mCurrentCursor; }

   /// Pushes shape - called by NewGuiControl::pushCursor() via this control's owning canvas.
   void pushControlCursor(NewGuiCursorShape shape);

   /// Pops whatever the matching pushControlCursor() pushed.
   void popControlCursor();

   /// @return The design-resolution scale currently in effect (excludes DPI scale). (1,1) if there's no content control.
   F32 getDesignScaleX() const { return mDesignScaleX; }
   F32 getDesignScaleY() const { return mDesignScaleY; }

   void setDefaultTooltipDelay(S32 ms) { mDefaultTooltipDelayMS = ms; }
   S32 getDefaultTooltipDelay() const { return mDefaultTooltipDelayMS; }

   void setDefaultTooltipStyle(NewGuiStyle* style) { mDefaultTooltipStyle = style; }
   NewGuiStyle* getDefaultTooltipStyle() const { return mDefaultTooltipStyle; }

   /// Computes the real scale (design scale combined with live DPI) and substitutes it before recursing into children.
   void ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// The frame driver. Call once per application frame tick. No-op if nothing in the tree is dirty.
   virtual void renderFrame();

   /// Registers/unregisters this canvas's renderFrame() with Process::notify(), so it's driven every tick automatically.
   /// @param enable True to register, false to unregister.
   void setAutoRender(bool enable);
   bool isAutoRender() const { return mAutoRenderRegistered; }

   bool processInputEvent(InputEventInfo& inputEvent) override;

   NewGuiControl* getMouseCapturedControl() const { return mMouseCapturedControl; }
   NewGuiControl* getFirstResponder() const { return mFirstResponder; }

   /// Sets keyboard focus, running onFirstResponder gained/lost pairs the same way updateMouseOver() does for hover.
   /// @param control Control to focus, or NULL to clear focus.
   void setFirstResponderControl(NewGuiControl* control);

   /// Registers a keyboard accelerator bound to control: fires control->onAccelerator()
   /// whenever keyCode+modifier is pressed, regardless of what currently has keyboard focus
   /// (unless the current first responder wants raw keyboard input itself - see
   /// checkAccelerators()). modifier is normalized internally, so callers may pass raw
   /// SI_LSHIFT/SI_RSHIFT-style bits as delivered by whatever parsed the authored string.
   /// @param control Control to notify on a match. Not retained beyond removeAccelerators().
   /// @param keyCode Key code to match (e.g. KEY_S).
   /// @param modifier Modifier bitmask to match (e.g. SI_LCTRL) - left/right pairs normalized before storing.
   void registerAccelerator(NewGuiControl* control, U16 keyCode, U32 modifier);

   /// Removes every accelerator binding registered for control. Safe to call even if none were registered.
   /// @param control Control whose bindings should be removed.
   void removeAccelerators(NewGuiControl* control);

   /// Toggles the debug hit-region overlay: an outline (plus name label) drawn from every
   /// control's real mBounds, the exact rect findHitControl() tests a point against. Meant for
   /// diagnosing "this looks clickable but isn't" cases - a control whose visual box and actual
   /// hit box have diverged (e.g. an ancestor resolved narrower than what it renders/its children
   /// occupy) shows up immediately as an outline that stops short of where the content appears to
   /// end, rather than needing to be traced through ArrangePass()/layoutChildren() by hand.
   /// @param enable True to draw the overlay every frame, false to stop.
   void setDebugShowHitRegions(bool enable) { mDebugShowHitRegions = enable; }
   bool getDebugShowHitRegions() const { return mDebugShowHitRegions; }

   /// Sets the name/class/id substring filter (case-insensitive). Pass "" or NULL to clear it and
   /// draw every control again. Has no effect while getDebugHitRegionHoverOnly() is true - see
   /// mDebugHitRegionHoverOnly's doc comment on why the two modes take precedence rather than combining.
   void setDebugHitRegionFilter(const char* filter) { mDebugHitRegionFilter = (filter && filter[0]) ? StringTable->insert(filter) : NULL; }
   const char* getDebugHitRegionFilter() const { return mDebugHitRegionFilter ? mDebugHitRegionFilter : ""; }

   /// Toggles hover-chain mode: draws only mMouseOverControl's ancestor chain instead of the whole
   /// tree, so probing one busy area (e.g. a stack of open popups) doesn't draw every other
   /// control's outline over it. Overrides the name filter while on.
   void setDebugHitRegionHoverOnly(bool enable) { mDebugHitRegionHoverOnly = enable; }
   bool getDebugHitRegionHoverOnly() const { return mDebugHitRegionHoverOnly; }
};

#endif // _NEWGUICANVAS_H_
