//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiTree.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUITREE_H_
#define _NEWGUITREE_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#ifndef _NEWGUISTACK_H_
#include "gui_rev2/controls/newGuiStack.h"
#endif
#ifndef _NEWGUITEXT_H_
#include "gui_rev2/core/newGuiText.h"
#endif
#ifndef _GFONT_H_
#include "gfx/gFont.h"
#endif

class NewGuiTree;
class NewGuiTreeGroup;
class NewGuiTreeRow;
class NewGuiTextEdit;

//=============================================================================
// NewGuiTreeDataSource
//=============================================================================

/// Where a NewGuiTree's items actually come from - NOT a SimObject, NOT owned via the
/// register/addObject chain (it has no visual presence and no place in a SimGroup; it's a plain
/// interface object the tree holds a raw pointer to and destroys itself, same as e.g. a
/// NewGuiTextFontAtSizeDelegate is configuration rather than a tree citizen). Exists so
/// NewGuiTree/NewGuiTreeGroup never need to know whether an item is a live SimGroup child, a
/// directory entry under a Torque::FS volume (shadercache:, home:, ...), or anything else -
/// every item the tree displays is represented to it as a SimObject* either way (see
/// NewGuiTreeFileSystemSource's own comment on how a raw file becomes one).
///
/// A tree has exactly one data source, but that source may itself expose multiple top-level
/// roots (see getRoots()) - this is what makes an asset browser watching several directories, or
/// several volumes, a single NewGuiTree rather than several unrelated ones.
class NewGuiTreeDataSource
{
public:

   virtual ~NewGuiTreeDataSource() {}

   /// @param outRoots Receives every top-level item, in display order. One entry for an
   /// ordinary single-root tree (a scene group, a GuiControl hierarchy); multiple entries for
   /// e.g. several watched asset directories shown as sibling top-level blocks.
   virtual void getRoots(Vector<SimObject*>& outRoots) = 0;

   /// @param item Item to expand.
   /// @param outChildren Receives item's immediate children, in display order.
   virtual void getChildren(SimObject* item, Vector<SimObject*>& outChildren) = 0;

   /// @return True if item has at least one child - queried separately from getChildren() so a
   /// collapsed group can show/hide its expand arrow without the source paying for a full child
   /// enumeration (relevant for the filesystem source, where getChildren() means an actual
   /// directory read).
   virtual bool hasChildren(SimObject* item) = 0;

   /// Registers the tree's interest in item's lifetime/identity, however this source's backing
   /// data can express that. A SimObject-backed source wires this straight to
   /// notifyTarget->deleteNotify(item)/clearNotify(item); a source with no real deleteNotify
   /// chain behind it (a raw filesystem entry) is free to no-op - see
   /// NewGuiTreeFileSystemSource's own comment on why that's still safe.
   /// @param item Item to watch.
   /// @param notifyTarget Object whose onDeleteNotify(item) should fire, if this source supports it.
   virtual void registerInterest(SimObject* item, SimObject* notifyTarget) = 0;

   /// Inverse of registerInterest() - called when the tree no longer needs item watched
   /// (item's row/group is being destroyed, or the tree itself is going away).
   virtual void unregisterInterest(SimObject* item, SimObject* notifyTarget) = 0;

   /// @return A stable display name for item - the source's call, since a filesystem entry's
   /// "name" is a path leaf, not necessarily item->getName() the way a SimObject's is.
   virtual const char* getDisplayName(SimObject* item) = 0;

   /// @return True if item can accept draggedItem as a new child (asChild-drop target
   /// validity) - a SimObject-backed source defers to dynamic_cast<SimGroup*>; the filesystem
   /// source defers to "is item a directory, and is the volume writable" once that layer exists
   /// (see NewGuiTreeFileSystemSource's class doc comment - the drop/write side is not yet wired
   /// against a confirmed FS API and is flagged there, not silently assumed).
   virtual bool canAcceptChild(SimObject* item, SimObject* draggedItem) = 0;

   /// Performs an actual reparent/reorder of draggedItem to become a child of (or sibling
   /// adjacent to, depending on asChild) targetItem, against whatever real backing structure
   /// this source wraps. Returns false (and does nothing) if unsupported - e.g. a read-only
   /// filesystem source until real move/rename FS calls are wired in.
   virtual bool moveItem(SimObject* draggedItem, SimObject* targetItem, bool asChild, SimObject* reorderBeforeSibling) = 0;
};

/// Wraps one or more already-existing SimObjects/SimGroups as tree roots - the scene-editor
/// case (root = the scene's SimGroup), the GuiControl-editor case (root = a GuiControl
/// hierarchy), and the "several top-level groups already live in the Sim" case, all in one
/// source. getChildren() walks SimGroup::at()/size() directly; registerInterest()/
/// unregisterInterest() are thin wrappers over SimObject::deleteNotify()/clearNotify(), so
/// deletion of any watched object is caught the normal way. moveItem() performs a real
/// removeObject()/addObject()/reOrder() against the live SimGroup structure.
class NewGuiTreeSimGroupSource : public NewGuiTreeDataSource
{
protected:

   Vector<SimObjectPtr<SimObject> > mRoots;

public:

   /// @param singleRoot Convenience constructor for the common one-root case.
   explicit NewGuiTreeSimGroupSource(SimObject* singleRoot);

   /// @param roots One or more already-existing top-level SimObjects.
   explicit NewGuiTreeSimGroupSource(const Vector<SimObject*>& roots);

   void getRoots(Vector<SimObject*>& outRoots) override;
   void getChildren(SimObject* item, Vector<SimObject*>& outChildren) override;
   bool hasChildren(SimObject* item) override;
   void registerInterest(SimObject* item, SimObject* notifyTarget) override;
   void unregisterInterest(SimObject* item, SimObject* notifyTarget) override;
   const char* getDisplayName(SimObject* item) override;
   bool canAcceptChild(SimObject* item, SimObject* draggedItem) override;
   bool moveItem(SimObject* draggedItem, SimObject* targetItem, bool asChild, SimObject* reorderBeforeSibling) override;
};

/// Wraps one or more Torque::FS volume-relative root paths (a plain OS directory, or a
/// volume-prefixed path like "shadercache:" or "home:" - resolution of that prefix is entirely
/// Torque::FS's own job, this class just passes paths through to it unmodified) as tree roots -
/// the asset-browser case.
///
/// @note IMPLEMENTATION GAP, called out deliberately rather than guessed at: the actual
/// directory-enumeration and file-identity calls (whatever this engine's Torque::FS exposes for
/// "list entries under path" and "does this path still exist") are not present anywhere in the
/// files available to write this class against, so getChildren()/hasChildren() below are left as
/// a single, clearly-marked seam (_listDirectory()) rather than invented against a guessed API
/// that would silently fail to compile or, worse, compile against the wrong overload. Wire
/// _listDirectory() to this engine's real volume-listing entry point and the rest of this class
/// (item identity via NewGuiTreeFileProxy, sorting, registerInterest() no-op reasoning below)
/// does not need to change.
///
/// Each returned "item" is a NewGuiTreeFileProxy (see that class) - a lightweight, tree-owned
/// SimObject synthesized to represent one file/directory entry, since a raw file on disk is not
/// itself a SimObject and the rest of NewGuiTree/NewGuiTreeRow's contract (payload is always a
/// SimObject*, deleteNotify-able) needs something real to point at either way.
///
/// registerInterest()/unregisterInterest() are no-ops here: a NewGuiTreeFileProxy's lifetime is
/// owned entirely by this source (created fresh each _listDirectory() call, replacing the
/// previous batch), not by anything the tree needs to deleteNotify() against - if a file
/// disappears from disk between one rebuild and the next, the corresponding proxy simply isn't
/// re-created next rebuild, which NewGuiTree's ordinary "item vanished from getChildren()" path
/// (not a live-deletion notification) already handles. This is a real, deliberate difference
/// from the SimObject source, not an oversight - there is no OS-level "notify me when this file
/// is deleted" hook wired here yet either, so no interest to register even in principle.
class NewGuiTreeFileSystemSource : public NewGuiTreeDataSource
{
protected:

   Vector<String> mRootPaths;   ///< Volume-relative root paths, e.g. "art/shapes", "shadercache:", "home:".

   /// Every proxy this source has ever handed out, keyed by full path, so repeated
   /// getChildren() calls for the same directory return the SAME NewGuiTreeFileProxy instance
   /// (identity-stable across rebuilds) rather than a fresh one each time - needed so
   /// NewGuiTree's selection/expand-state tracking (keyed by SimObject* identity) survives a
   /// rebuild at all.
   Vector<SimObjectPtr<class NewGuiTreeFileProxy> > mProxyCache;

   /// THE SEAM - see class doc comment. Populate outEntries with this directory's immediate
   /// children; real implementation wires to this engine's actual Torque::FS listing call.
   struct DirEntry { String path; bool isDirectory; };
   /// @param fullVolumePath Already-resolved (root + relative) path, volume prefix included.
   /// @param outEntries Receives one DirEntry per immediate child.
   void _listDirectory(const String& fullVolumePath, Vector<DirEntry>& outEntries);

   /// Finds or creates (registering it the normal chain) the NewGuiTreeFileProxy for fullPath.
   class NewGuiTreeFileProxy* _getOrCreateProxy(const String& fullPath, bool isDirectory);

public:

   explicit NewGuiTreeFileSystemSource(const String& singleRootPath);
   explicit NewGuiTreeFileSystemSource(const Vector<String>& rootPaths);
   virtual ~NewGuiTreeFileSystemSource();

   void getRoots(Vector<SimObject*>& outRoots) override;
   void getChildren(SimObject* item, Vector<SimObject*>& outChildren) override;
   bool hasChildren(SimObject* item) override;
   void registerInterest(SimObject* item, SimObject* notifyTarget) override {}
   void unregisterInterest(SimObject* item, SimObject* notifyTarget) override {}
   const char* getDisplayName(SimObject* item) override;

   /// Currently always false - see class doc comment; drag-to-move against real files needs
   /// confirmed FS move/rename calls before this can honestly return true for any pair.
   bool canAcceptChild(SimObject* item, SimObject* draggedItem) override { return false; }
   bool moveItem(SimObject* draggedItem, SimObject* targetItem, bool asChild, SimObject* reorderBeforeSibling) override { return false; }
};

/// A minimal registered SimObject standing in for one filesystem entry, so
/// NewGuiTreeFileSystemSource can hand the rest of the tree a real SimObject* identity per file/
/// directory (see that class's doc comment). Holds only what a row needs to display: path,
/// leaf display name, and whether it's a directory (-> hasChildren proxy). Never holds an open
/// file handle or any live OS resource - it is pure metadata, safe to create/destroy freely
/// across rebuilds.
class NewGuiTreeFileProxy : public SimObject
{
public:

   typedef SimObject Parent;

protected:

   String mFullPath;
   String mDisplayName;
   bool mIsDirectory;

public:

   NewGuiTreeFileProxy();

   NewGuiTreeFileProxy(const String& fullPath, const String& displayName, bool isDirectory);
   virtual ~NewGuiTreeFileProxy();

   DECLARE_CONOBJECT(NewGuiTreeFileProxy);

   const String& getFullPath() const { return mFullPath; }
   const String& getDisplayName() const { return mDisplayName; }
   bool isDirectory() const { return mIsDirectory; }
};

//=============================================================================
// NewGuiTreeRow
//=============================================================================

/// One visual row - the expand arrow (if hasChildren), class icon, and (editable) label for a
/// single tree item. A real, registered SimObject (a NewGuiControl child of its owning
/// NewGuiTreeGroup - NOT of NewGuiTree directly; see NewGuiTreeGroup's own class doc comment for
/// why ownership sits at the group level now), created/destroyed by
/// NewGuiTreeGroup::syncChildren() alongside that group's own structural changes.
///
/// mPayload is the arbitrary SimObject (or NewGuiTreeFileProxy) this row represents. A row
/// registers interest in its payload via the owning tree's data source
/// (NewGuiTreeDataSource::registerInterest()) rather than calling deleteNotify() directly, so
/// notification wiring stays correct regardless of which source backs the tree.
class NewGuiTreeRow : public NewGuiControl
{
public:

   typedef NewGuiControl Parent;

protected:

   NewGuiTree* mOwningTree;                  ///< Set once at creation; never reassigned.
   NewGuiTreeGroup* mOwningGroup;             ///< This row's direct parent group - set once at creation; never reassigned.
   SimObjectPtr<SimObject> mPayload;         ///< The tree item this row displays. May go NULL via onDeleteNotify() - see class comment.
   S32 mDepth;                               ///< Indent level, cached at configure() time - purely cosmetic now (nesting itself comes from mOwningGroup's real nesting), used only to size the indent guide.
   bool mHasChildren;
   bool mExpanded;

   NewGuiText mText;                         ///< Label display when NOT being inline-edited.
   Resource<GFont> mFont;
   StringTableEntry mCachedFontFamily;
   F32 mCachedFontSize;

   StringTableEntry mIconName;               ///< Resolved once per configure() via NewGuiTree::resolveClassIcon(mPayload).

   NewGuiTextEdit* mRenameEdit;              ///< NULL except during an active inline rename.

   /// True from a bounds-landing onMouseDown() until the matching onMouseUp() - same
   /// arm-on-down/act-on-up split every other control in this system uses (see
   /// NewGuiButton::onMouseDown()/onMouseUp() for the reference shape onMouseDown()/onMouseUp()
   /// below are deliberately matched against). onMouseDown() ONLY ever sets this (plus
   /// mPressStartedOnArrow/mPressStartScreenPoint) and claims the event - it never itself
   /// toggles expand state, commits a rename, or selects anything. All of that happens in
   /// onMouseUp(), gated on the release point still being valid, exactly like NewGuiButton's own
   /// wasArmed + localBounds.pointInRect(event.localPoint) check.
   bool mPressArmed;

   /// Which part of the row mPressArmed's onMouseDown() landed on - decided once, at press time,
   /// and re-checked against the RELEASE point in onMouseUp() before acting, so a press that
   /// started on the arrow but was dragged off and released over the label (or vice versa) does
   /// neither action, matching the same "only the release point decides" rule
   /// NewGuiButton::onMouseUp()'s own comment states for its click.
   bool mPressStartedOnArrow;

   Point2I mPressStartScreenPoint;
   bool mDragInProgress;
   NewGuiTreeRow* mDropTargetRow;
   bool mDropAsChild;

   static bool _setText(void* obj, const char* index, const char* data);

   void resolveFont();
   RectI getExpandArrowRect() const;
   RectI getIconRect() const;

public:

   NewGuiTreeRow();
   virtual ~NewGuiTreeRow();

   DECLARE_CONOBJECT(NewGuiTreeRow);

   static void initPersistFields();

   bool onAdd() override;
   void onRemove() override;
   void onDeleteNotify(SimObject* object) override;

   /// @param tree Owning tree.
   /// @param group Owning group (this row's real SimGroup/NewGuiControl parent).
   /// @param payload Item this row displays. Registers interest via the tree's data source.
   /// @param depth Indent level (cosmetic - see mDepth's own doc comment).
   /// @param hasChildren Whether to draw/hit-test an expand arrow.
   /// @param expanded Current expand state, for arrow orientation.
   void configure(NewGuiTree* tree, NewGuiTreeGroup* group, SimObject* payload, S32 depth, bool hasChildren, bool expanded);

   SimObject* getPayload() const { return mPayload; }
   S32 getDepth() const { return mDepth; }
   NewGuiTreeGroup* getOwningGroup() const { return mOwningGroup; }
   bool getHasChildren() const { return mHasChildren; }
   bool getExpanded() const { return mExpanded; }

   void beginRename();
   void commitRename();
   void cancelRename();
   bool isRenaming() const { return mRenameEdit != NULL; }

   Point2I ComputePreferredSize() override;
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override;

   void onMouseDown(NewGuiInputEvent& event) override;
   void onInputEvent(NewGuiInputEvent& event) override;
   void onMouseUp(NewGuiInputEvent& event) override;

   DECLARE_CALLBACK(void, onRowClick, (S32 mouseClickCount));
};

//=============================================================================
// NewGuiTreeGroup
//=============================================================================

/// One EXPANDED level of the tree, rendered as a real nested visual block rather than a run of
/// indented siblings - this is the piece that makes "expand/collapse a subgroup" mean "show/hide
/// one child control" instead of NewGuiTree hand-computing a visible-row subrange every rebuild.
///
/// Owns NO header row of its own - the row a user actually clicks to expand/collapse a given
/// item is that item's ChildSlot::row, living one level up, in whichever NewGuiTreeGroup
/// contains it as a child (a NewGuiTree::mTopLevelGroups entry is itself just a NewGuiTreeGroup
/// with mGroupItem == NULL - "top-level" isn't a structurally different case, only a semantic
/// one: its own ChildSlots ARE the tree's roots, per syncChildren()'s own getRoots() branch). A
/// NewGuiTreeGroup created for mGroupItem therefore only ever contains mGroupItem's CHILDREN's
/// rows/nested groups, never a row for mGroupItem itself - an earlier version of this class
/// created its own redundant header row here, which produced a real, visible bug: every expanded
/// item's own name appeared twice, once as its real ChildSlot::row in the parent and again as
/// this group's own mHeaderRow one level deeper, looking like the item had become a child of
/// itself.
///
/// Children, in document order: for each of mGroupItem's (or, for a top-level group, each
/// data-source root's) children, a permanent NewGuiTreeRow (always present, never destroyed
/// while the item exists - see ChildSlot's own doc comment) immediately followed by that item's
/// own NewGuiTreeGroup child block, created lazily on first expand and, from then on, kept
/// resident and toggled via setVisible() rather than destroyed on every collapse.
///
/// Derives from NewGuiStack (vertical axis, fixed in the constructor - not authorable, this
/// control only ever uses one configuration) rather than reimplementing vertical flow by hand,
/// for the same reason NewGuiConsole and NewGuiPopupGroup already do the same thing: Stack
/// already correctly measures/arranges while SKIPPING invisible children (see
/// NewGuiStack::ComputePreferredSize()'s own isVisible() checks) - which is exactly what a
/// collapsed-but-resident childGroup needs, and hand-rolling that visibility-aware accounting a
/// second time here was a real, unforced mistake corrected by this rewrite: NewGuiStack already
/// solves it, tested and working, and duplicating it only risked getting some case (e.g. a
/// hidden child still contributing to preferred height) subtly wrong.
class NewGuiTreeGroup : public NewGuiStack
{
public:

   typedef NewGuiStack Parent;

   friend class NewGuiTree;

protected:

   NewGuiTree* mOwningTree;
   NewGuiTreeGroup* mParentGroup;       ///< NULL for one of NewGuiTree's top-level groups.
   SimObjectPtr<SimObject> mGroupItem;  ///< The item this group is the expansion of. NULL for a top-level synthetic group (see mOwningTree's mTopLevelGroups).
   S32 mDepth;                          ///< 0 for a top-level group; header row indent derives from this.

   /// One entry per CURRENT child item of mGroupItem (or per data-source root, for a top-level
   /// group), in display order. Kept in step with the data source's current getChildren()
   /// result by syncChildren().
   struct ChildSlot
   {
      SimObjectPtr<SimObject> item;
      SimObjectPtr<NewGuiTreeRow> row;   ///< This slot's own row - ALWAYS present, never destroyed while item is present (only when item itself is removed - see syncChildren()'s cleanup pass). A NewGuiStack child like any other; Stack's own layout places it whether or not it's the item being renamed/etc.
      NewGuiTreeGroup* childGroup;        ///< This slot's nested-children container. NULL until the first expand; once created, NEVER destroyed while item is present - only setVisible(false/true) toggles it (Stack skips it in layout while hidden - see class doc comment). Owned as a normal SimGroup/Stack child, not SimObjectPtr'd separately - see syncChildren().
      bool expanded;                      ///< Current expand state - independent of whether childGroup exists yet (it may exist but be hidden, or not exist yet at all if never expanded).
   };
   Vector<ChildSlot> mChildSlots;

   /// Re-derives mChildSlots from mOwningTree's data source's current getChildren(mGroupItem)
   /// (or getRoots(), for a top-level group), reusing any existing row/childGroup whose item is
   /// still present at the same identity (preserving them - and childGroup's own expanded
   /// subtree - untouched), creating new ones via the normal register/addObject chain for new
   /// items, and destroying (removeObject + deleteObject, never `delete`) any row/childGroup
   /// whose item is no longer present at all. Recurses into every existing childGroup's own
   /// syncChildren() (expanded or not - a collapsed group's underlying data can still change
   /// while hidden), so one call at the tree root brings the whole tracked subtree back in sync.
   void syncChildren();

   /// Expands slot's item in place: creates slot.childGroup on first use (kept forever
   /// afterward - see ChildSlot's own doc comment), or just re-syncs and shows it if it already
   /// existed from a previous expand.
   void expandSlot(ChildSlot& slot);

   /// Inverse of expandSlot() - setVisible(false) on slot.childGroup. Never destroys it - see
   /// ChildSlot's own doc comment for why.
   void collapseSlot(ChildSlot& slot);

public:

   NewGuiTreeGroup();
   virtual ~NewGuiTreeGroup();

   DECLARE_CONOBJECT(NewGuiTreeGroup);

   bool onAdd() override;
   void onRemove() override;

   /// @param tree Owning tree.
   /// @param parentGroup NULL for a top-level group.
   /// @param groupItem NULL for a top-level synthetic group; otherwise the item being expanded.
   /// @param depth Indent level for this group's own header row.
   void configure(NewGuiTree* tree, NewGuiTreeGroup* parentGroup, SimObject* groupItem, S32 depth);

   SimObject* getGroupItem() const { return mGroupItem; }

   /// Finds item among this group's OWN child slots (not recursive) and expands/collapses it.
   /// Returns true if item was found at this level.
   bool setChildExpanded(SimObject* item, bool expanded);

   /// Recursively finds whichever NewGuiTreeRow currently displays item - only rows under an
   /// EXPANDED (visible) childGroup count, matching NewGuiTree::beginRenameItem()'s existing
   /// "must already be visible" contract, even though a collapsed childGroup's rows still exist.
   NewGuiTreeRow* findVisibleRow(SimObject* item);

   /// Recursive form of setChildExpanded() - tries this group's own slots first, then every
   /// child's own childGroup slots in turn, so a caller doesn't need to know which level item
   /// lives at. Returns true once found and applied, at whatever depth.
   bool setExpandedRecursive(SimObject* item, bool expanded);

   /// @return True if item is draggedItem itself, or currently visible anywhere within
   /// draggedItem's own EXPANDED subtree (i.e. dropping item's row onto it would be a cycle).
   /// Searches this group's ENTIRE subtree for draggedItem first (it may be at any depth, not
   /// just this level), then, once found, checks whether item falls under it.
   bool isDescendantOrSelf(SimObject* draggedItem, SimObject* item);
};

//=============================================================================
// NewGuiTree
//=============================================================================

/// A tree control for folders, scene hierarchies, GuiControl hierarchies, or an asset browser
/// spanning several watched directories/volumes - class icons per row, click-to-select,
/// double-click inline rename, and drag-to-reparent/reorder (where the active
/// NewGuiTreeDataSource supports it - see NewGuiTreeDataSource::canAcceptChild()/moveItem()).
///
/// NewGuiTree itself owns exactly N top-level NewGuiTreeGroup children (N == data source's
/// getRoots() count - one for a single-root scene/GuiControl tree, several for a multi-
/// directory asset browser), stacked vertically. All actual row/nested-group creation,
/// destruction, and visibility lives in NewGuiTreeGroup::syncChildren() - see that class's doc
/// comment - so NewGuiTree itself no longer computes a flat visible-row list by hand; expand/
/// collapse is real SimGroup structure (a NewGuiTreeGroup appearing/disappearing), and
/// MeasurePass/ArrangePass's existing bottom-up/top-down passes size everything correctly with
/// no separate row-count bookkeeping.
///
/// @code
/// new NewGuiTree( SceneTree )
/// {
///    width = "100%"; height = "100%";
///    rowHeight = "20";
///    indentWidth = "16";
///    allowReparenting = "true";
/// };
/// %SceneTree.setRoot(%mySceneGroup);
///
/// // Asset browser, several watched locations:
/// %AssetTree.setFileSystemRoots("art/shapes\tart/gui\tshadercache:\thome:");
/// @endcode
class NewGuiTree : public NewGuiStack
{
public:

   typedef NewGuiStack Parent;

   friend class NewGuiTreeGroup;
   friend class NewGuiTreeRow;

protected:

   NewGuiTreeDataSource* mDataSource;   ///< Owned - destroyed in the destructor/whenever replaced. NULL means an empty tree.

   /// This tree's own top-level NewGuiTreeGroup children - one per mDataSource->getRoots()
   /// entry, in that order. Populated/destroyed by rebuildRoots(), the top-level analogue of
   /// NewGuiTreeGroup::syncChildren() (there is no single top-level SimObject to hang a group
   /// under, so this one level is handled directly rather than via a synthetic zero-th item).
   Vector<SimObjectPtr<NewGuiTreeGroup> > mTopLevelGroups;

   S32 mRowHeight;
   S32 mIndentWidth;
   bool mAllowReparenting;
   bool mAllowRename;

   SimObjectPtr<SimObject> mSelectedItem;

   SimObjectPtr<SimObject> mLastClickedItem;
   U32 mLastClickTimeMS;
   static const U32 smDoubleClickIntervalMS = 400;

   /// A structure-mutating action requested by a row while its own onMouseDown()/onMouseUp() was
   /// still on the call stack, applied by flushPendingAction() at the start of the next
   /// MeasurePass() instead of immediately.
   ///
   /// Do not "simplify" setExpanded()/commitDrop()/onRowClick_callback() back to a direct call
   /// from NewGuiTreeRow::onMouseDown()/onMouseUp(): any of them can destroy the very
   /// NewGuiTreeRow whose handler is still executing (collapse/reparent both recursively
   /// deleteObject() rows via ordinary SimGroup teardown), which leaves
   /// NewGuiCanvas::mMouseCapturedControl/mMouseOverControl (raw pointers, never cleared on
   /// delete) dangling. Deferring to MeasurePass() (never called from input dispatch) guarantees
   /// every row's own handler has fully returned before anything is destroyed.
   enum PendingActionType : U8
   {
      PendingAction_None = 0,
      PendingAction_SetExpanded,
      PendingAction_CommitDrop,
      PendingAction_RowClick,      ///< Fires onRowClick_callback() + the select/rename logic that follows it in onMouseUp() - see flushPendingAction().
      PendingAction_ApplyRename,   ///< Fires applyRename()/onRename_callback() - see requestApplyRename().
   };
   PendingActionType mPendingActionType;
   SimObjectPtr<SimObject> mPendingActionItem;      ///< setExpanded's item, commitDrop's draggedItem, RowClick's clicked payload, or ApplyRename's item.
   SimObjectPtr<SimObject> mPendingActionItem2;      ///< commitDrop's targetItem only; unused otherwise.
   bool mPendingActionBool;                          ///< setExpanded's expanded, or commitDrop's asChild.
   S32 mPendingActionClickCount;                     ///< RowClick's event.clickCount only; unused otherwise.
   String mPendingActionText;                        ///< ApplyRename's newName only; unused otherwise. A String (not const char*) since the source buffer (a NewGuiTextEdit about to be destroyed) won't outlive the request.

   /// Queues item/expanded for a deferred setExpanded() - see mPendingActionType's doc comment.
   /// The only safe way to request a toggle from inside a NewGuiTreeRow's own onMouseDown().
   void requestExpandedChange(SimObject* item, bool expanded);

   /// Queues a deferred commitDrop() - see mPendingActionType's doc comment. The only safe way
   /// to request a drop-commit from inside a NewGuiTreeRow's own onMouseUp().
   void requestCommitDrop(SimObject* draggedItem, SimObject* targetItem, bool asChild);

   /// Queues a deferred "this row was clicked" notification (onRowClick_callback() + select/
   /// rename) - see mPendingActionType's doc comment. The only safe way for
   /// NewGuiTreeRow::onMouseUp() to report a plain click, since even a plain select can
   /// indirectly reach script (via onSelect_callback()) which could itself delete something.
   void requestRowClick(SimObject* item, S32 clickCount);

   /// Queues a deferred applyRename()/onRename_callback() - see mPendingActionType's doc
   /// comment. The only safe way for NewGuiTreeRow::commitRename() to apply an edited name,
   /// since onRename_callback() reaches script the same as onRowClick/onSelect do.
   void requestApplyRename(SimObject* item, const char* newName);

   /// Applies whichever mPendingActionType is currently queued (no-op if PendingAction_None),
   /// then clears it. Called once, at the very start of MeasurePass(), before
   /// Parent::MeasurePass() touches any child.
   void flushPendingAction();

   /// Marks this control dirty so the driver actually revisits it next frame - called by every
   /// requestX() method above. See its own doc comment in the .cpp for why this was the real
   /// cause of queued clicks/expands/renames never taking effect.
   void markPendingActionDirty();

   static bool _setRowHeight(void* obj, const char* index, const char* data);
   static bool _setIndentWidth(void* obj, const char* index, const char* data);
   static bool _setAllowReparenting(void* obj, const char* index, const char* data);
   static bool _setAllowRename(void* obj, const char* index, const char* data);

   /// Re-derives mTopLevelGroups from mDataSource->getRoots(), reusing any group whose item is
   /// still a root at the same identity (preserving its whole expanded subtree), creating new
   /// ones (register/addObject) for new roots, and destroying (removeObject + deleteObject) any
   /// group whose root is no longer present. Calls each surviving/new group's syncChildren().
   void rebuildRoots();

   /// @return The class icon name for object, via object->getClassIconName() (if the object's
   /// class defines that console method), else object->getClassName(), else "default" if object
   /// is NULL. A NewGuiTreeFileProxy's class name ("NewGuiTreeFileProxy") is deliberately NOT
   /// icon-distinctive on its own - a script-side getClassIconName() override on that class (or
   /// a per-extension lookup keyed off getDisplayName()'s suffix) is how a file/folder gets a
   /// distinct icon from a SimObject; that mapping belongs in skin/script data, not hardcoded
   /// here, same reasoning as the original resolveClassIcon() design.
   static StringTableEntry resolveClassIcon(SimObject* object);

   void resolveDropAtPoint(const Point2I& screenPoint, NewGuiTreeRow*& outRow, bool& outAsChild) const;
   bool isValidDropTarget(SimObject* draggedItem, NewGuiTreeRow* targetRow, bool asChild) const;

   /// Depth-first collects every currently-materialized NewGuiTreeRow under group (recursing
   /// into nested groups) - shared by resolveDropAtPoint() and anything else that needs to walk
   /// all visible rows without NewGuiTree keeping its own separate flat list anymore.
   static void collectVisibleRows(NewGuiTreeGroup* group, Vector<NewGuiTreeRow*>& outRows);

public:

   NewGuiTree();
   virtual ~NewGuiTree();

   DECLARE_CONOBJECT(NewGuiTree);

   static void initPersistFields();

   bool onAdd() override;
   void onRemove() override;
   void onDeleteNotify(SimObject* object) override;

   /// Convenience: wraps a single already-existing SimObject/SimGroup root in a
   /// NewGuiTreeSimGroupSource and installs it - the scene-editor/GuiControl-editor case.
   /// @param root New root object, or NULL to empty the tree.
   void setRoot(SimObject* root);

   /// Wraps several already-existing SimObject roots in one NewGuiTreeSimGroupSource - the
   /// "several top-level groups already live in the Sim" case.
   void setRoots(const Vector<SimObject*>& roots);

   /// Wraps one or more Torque::FS volume-relative paths in a NewGuiTreeFileSystemSource - the
   /// asset-browser case, including volume-prefixed paths like "shadercache:"/"home:". See
   /// NewGuiTreeFileSystemSource's own class doc comment for the one implementation gap
   /// (directory-listing call) that needs wiring against this engine's confirmed Torque::FS API.
   void setFileSystemRoots(const Vector<String>& rootPaths);

   /// Installs an arbitrary custom data source (taking ownership) - the general form setRoot()/
   /// setRoots()/setFileSystemRoots() are convenience wrappers around, for a caller with its own
   /// NewGuiTreeDataSource implementation.
   void setDataSource(NewGuiTreeDataSource* source);
   NewGuiTreeDataSource* getDataSource() const { return mDataSource; }

   /// Applies immediately - safe to call from script or anywhere else OUTSIDE this tree's own
   /// input-event dispatch (the "Expand All"/"Collapse All" test buttons, etc). A NewGuiTreeRow's
   /// own arrow click does NOT call this directly - see requestExpandedChange() (protected,
   /// NewGuiTreeRow-only) and mPendingActionType's doc comment for why.
   void setExpanded(SimObject* item, bool expanded);
   bool isExpanded(SimObject* item) const;

   void setSelectedItem(SimObject* item);
   SimObject* getSelectedItem() const { return mSelectedItem; }

   void beginRenameItem(SimObject* item);
   virtual void applyRename(SimObject* item, const char* newName);

   /// Applies immediately - safe to call from script or anywhere else OUTSIDE this tree's own
   /// input-event dispatch. A NewGuiTreeRow's own drag-drop does NOT call this directly - see
   /// requestCommitDrop() (protected, NewGuiTreeRow-only) and mPendingActionType's doc comment.
   void commitDrop(SimObject* draggedItem, SimObject* targetItem, bool asChild);

   S32 getRowHeight() const { return mRowHeight; }
   S32 getIndentWidth() const { return mIndentWidth; }
   bool getAllowReparenting() const { return mAllowReparenting; }
   bool getAllowRename() const { return mAllowRename; }

   /// Flushes any pending requestExpandedChange()/requestCommitDrop()/requestRowClick() (see
   /// mPendingActionType's doc comment) before deferring to Parent::MeasurePass() - so a
   /// just-applied structural change is already reflected in mTopLevelGroups/mChildSlots by the
   /// time this same MeasurePass() call measures them, with no extra frame of lag.
   Point2I MeasurePass() override;

   DECLARE_CALLBACK(void, onSelect, (SimObject* item));
   DECLARE_CALLBACK(void, onReparent, (SimObjectId item, SimObjectId newParent));
   DECLARE_CALLBACK(void, onRename, (SimObject* item, const char* newName));
};

#endif // _NEWGUITREE_H_
