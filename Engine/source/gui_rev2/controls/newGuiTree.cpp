//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiTree.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiTree.h"
#include "gui_rev2/controls/newGuiTextEdit.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gui_rev2/core/newGuiCanvas.h"
#include "sim/simSet.h"

static const S32 kTreeDragThresholdPx = 4;
static const S32 kTreeArrowWidth = 16;
static const S32 kTreeIconWidth = 16;
static const S32 kTreeIconGap = 4;
static const S32 kTreeLabelPadding = 2;

//=============================================================================
// NewGuiTreeSimGroupSource
//=============================================================================

NewGuiTreeSimGroupSource::NewGuiTreeSimGroupSource(SimObject* singleRoot)
{
   if (singleRoot)
      mRoots.push_back(singleRoot);
}

NewGuiTreeSimGroupSource::NewGuiTreeSimGroupSource(const Vector<SimObject*>& roots)
{
   for (U32 i = 0; i < roots.size(); ++i)
   {
      if (roots[i])
         mRoots.push_back(roots[i]);
   }
}

void NewGuiTreeSimGroupSource::getRoots(Vector<SimObject*>& outRoots)
{
   for (U32 i = 0; i < mRoots.size(); ++i)
   {
      if (mRoots[i])
         outRoots.push_back(mRoots[i]);
   }
}

void NewGuiTreeSimGroupSource::getChildren(SimObject* item, Vector<SimObject*>& outChildren)
{
   SimGroup* group = dynamic_cast<SimGroup*>(item);
   if (!group)
      return;

   for (U32 i = 0; i < group->size(); ++i)
      outChildren.push_back(group->at(i));
}

bool NewGuiTreeSimGroupSource::hasChildren(SimObject* item)
{
   SimGroup* group = dynamic_cast<SimGroup*>(item);
   return group && group->size() > 0;
}

void NewGuiTreeSimGroupSource::registerInterest(SimObject* item, SimObject* notifyTarget)
{
   if (item && notifyTarget)
      notifyTarget->deleteNotify(item);
}

void NewGuiTreeSimGroupSource::unregisterInterest(SimObject* item, SimObject* notifyTarget)
{
   if (item && notifyTarget)
      notifyTarget->clearNotify(item);
}

const char* NewGuiTreeSimGroupSource::getDisplayName(SimObject* item)
{
   if (!item)
      return "";
   return item->getName() ? item->getName() : item->getIdString();
}

bool NewGuiTreeSimGroupSource::canAcceptChild(SimObject* item, SimObject* draggedItem)
{
   return dynamic_cast<SimGroup*>(item) != NULL;
}

bool NewGuiTreeSimGroupSource::moveItem(SimObject* draggedItem, SimObject* targetItem, bool asChild, SimObject* reorderBeforeSibling)
{
   if (!draggedItem || !targetItem)
      return false;

   SimGroup* oldParent = dynamic_cast<SimGroup*>(draggedItem->getGroup());

   SimGroup* newParent;
   if (asChild)
   {
      newParent = dynamic_cast<SimGroup*>(targetItem);
      if (!newParent)
         return false;
   }
   else
   {
      newParent = dynamic_cast<SimGroup*>(targetItem->getGroup());
      if (!newParent)
         return false;
   }

   if (oldParent != newParent)
   {
      if (oldParent)
         oldParent->removeObject(draggedItem);
      newParent->addObject(draggedItem);
   }

   if (!asChild && reorderBeforeSibling)
      newParent->reOrder(draggedItem, reorderBeforeSibling);

   return true;
}

//=============================================================================
// NewGuiTreeFileProxy
//=============================================================================

IMPLEMENT_CONOBJECT(NewGuiTreeFileProxy);

NewGuiTreeFileProxy::NewGuiTreeFileProxy()
{
   mFullPath = String::EmptyString;
   mDisplayName = String::EmptyString;
   mIsDirectory = false;
}

NewGuiTreeFileProxy::NewGuiTreeFileProxy(const String& fullPath, const String& displayName, bool isDirectory)
   : mFullPath(fullPath),
   mDisplayName(displayName),
   mIsDirectory(isDirectory)
{
}

NewGuiTreeFileProxy::~NewGuiTreeFileProxy()
{
}

//=============================================================================
// NewGuiTreeFileSystemSource
//=============================================================================

NewGuiTreeFileSystemSource::NewGuiTreeFileSystemSource(const String& singleRootPath)
{
   mRootPaths.push_back(singleRootPath);
}

NewGuiTreeFileSystemSource::NewGuiTreeFileSystemSource(const Vector<String>& rootPaths)
   : mRootPaths(rootPaths)
{
}

NewGuiTreeFileSystemSource::~NewGuiTreeFileSystemSource()
{
   for (U32 i = 0; i < mProxyCache.size(); ++i)
   {
      if (mProxyCache[i])
         mProxyCache[i]->deleteObject();
   }
}

// THE SEAM - see class doc comment in the header. Left unimplemented (returns no entries)
// rather than guessed, since this engine's real Torque::FS directory-listing call isn't present
// anywhere in the files available to write this against. Wire this one method to the real API
// and NewGuiTreeFileSystemSource is otherwise complete.
void NewGuiTreeFileSystemSource::_listDirectory(const String& fullVolumePath, Vector<DirEntry>& outEntries)
{
   // TODO: wire to this engine's real Torque::FS directory-enumeration entry point.
   // Expected shape: for each immediate child of fullVolumePath, push_back a DirEntry with
   // its full path (volume prefix included, e.g. "shadercache:/foo.bin") and whether it's a
   // directory. fullVolumePath itself already has any "root + relative" join applied by the
   // caller (getChildren() below) - this method only needs to resolve ONE level.
}

NewGuiTreeFileProxy* NewGuiTreeFileSystemSource::_getOrCreateProxy(const String& fullPath, bool isDirectory)
{
   for (U32 i = 0; i < mProxyCache.size(); ++i)
   {
      if (mProxyCache[i] && mProxyCache[i]->getFullPath() == fullPath)
         return mProxyCache[i];
   }

   // Display name is the path's leaf segment - last component after '/', or the whole path if
   // there's no separator (a bare volume root like "home:").
   String displayName = fullPath;
   S32 lastSlash = -1;
   for (S32 i = (S32)fullPath.length() - 1; i >= 0; --i)
   {
      if (fullPath[i] == '/')
      {
         lastSlash = i;
         break;
      }
   }
   if (lastSlash >= 0 && lastSlash + 1 < (S32)fullPath.length())
      displayName = fullPath.substr(lastSlash + 1, fullPath.length() - lastSlash - 1);

   NewGuiTreeFileProxy* proxy = new NewGuiTreeFileProxy(fullPath, displayName, isDirectory);
   proxy->registerObject();
   mProxyCache.push_back(proxy);
   return proxy;
}

void NewGuiTreeFileSystemSource::getRoots(Vector<SimObject*>& outRoots)
{
   for (U32 i = 0; i < mRootPaths.size(); ++i)
      outRoots.push_back(_getOrCreateProxy(mRootPaths[i], true));
}

void NewGuiTreeFileSystemSource::getChildren(SimObject* item, Vector<SimObject*>& outChildren)
{
   NewGuiTreeFileProxy* proxy = dynamic_cast<NewGuiTreeFileProxy*>(item);
   if (!proxy || !proxy->isDirectory())
      return;

   Vector<DirEntry> entries;
   _listDirectory(proxy->getFullPath(), entries);

   for (U32 i = 0; i < entries.size(); ++i)
      outChildren.push_back(_getOrCreateProxy(entries[i].path, entries[i].isDirectory));
}

bool NewGuiTreeFileSystemSource::hasChildren(SimObject* item)
{
   NewGuiTreeFileProxy* proxy = dynamic_cast<NewGuiTreeFileProxy*>(item);
   if (!proxy || !proxy->isDirectory())
      return false;

   Vector<DirEntry> entries;
   _listDirectory(proxy->getFullPath(), entries);
   return !entries.empty();
}

const char* NewGuiTreeFileSystemSource::getDisplayName(SimObject* item)
{
   NewGuiTreeFileProxy* proxy = dynamic_cast<NewGuiTreeFileProxy*>(item);
   return proxy ? proxy->getDisplayName().c_str() : "";
}

//=============================================================================
// NewGuiTreeRow
//=============================================================================

IMPLEMENT_CONOBJECT(NewGuiTreeRow);

IMPLEMENT_CALLBACK(NewGuiTreeRow, onRowClick, void, (S32 mouseClickCount), (mouseClickCount),
   "Called for every click on this row's label/icon area (not the expand arrow), single or double.");

NewGuiTreeRow::NewGuiTreeRow()
   : mOwningTree(NULL),
   mOwningGroup(NULL),
   mDepth(0),
   mHasChildren(false),
   mExpanded(false),
   mCachedFontFamily(NULL),
   mCachedFontSize(0.0f),
   mIconName(NULL),
   mRenameEdit(NULL),
   mPressArmed(false),
   mPressStartedOnArrow(false),
   mPressStartScreenPoint(0, 0),
   mDragInProgress(false),
   mDropTargetRow(NULL),
   mDropAsChild(false)
{
   mText.setAlignVertical(NewGuiTextAlignVertical::Middle);
}

NewGuiTreeRow::~NewGuiTreeRow()
{
}

void NewGuiTreeRow::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("TreeRow");

   ADD_FIELD("text", TypeString, 0)
      .onSet(_setText)
      .doc("Display label for this row. Normally driven by NewGuiTreeGroup::syncChildren() from the payload's display name rather than set directly.");

   GROUP_END("TreeRow");
}

bool NewGuiTreeRow::_setText(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTreeRow*>(obj)->mText.setText(data);
   static_cast<NewGuiTreeRow*>(obj)->setContentDirty();
   return false;
}

bool NewGuiTreeRow::onAdd()
{
   return Parent::onAdd();
}

void NewGuiTreeRow::onRemove()
{
   if (mRenameEdit)
      cancelRename();

   if (mPayload && mOwningTree && mOwningTree->getDataSource())
      mOwningTree->getDataSource()->unregisterInterest(mPayload, this);

   Parent::onRemove();
}

void NewGuiTreeRow::onDeleteNotify(SimObject* object)
{
   if (object == mPayload)
   {
      // Do not prune tree structure here - the owning tree/group's own onDeleteNotify()
      // (registered separately against the same payload, per NewGuiTreeGroup::syncChildren())
      // is the one place that actually re-derives ChildSlots and destroys this row as part of
      // that. This override exists only so this row never dereferences a mid-deletion payload
      // pointer from EmitDrawCommands()/onMouseUp() before that rebuild has run.
      mPayload = NULL;
   }

   Parent::onDeleteNotify(object);
}

void NewGuiTreeRow::configure(NewGuiTree* tree, NewGuiTreeGroup* group, SimObject* payload, S32 depth, bool hasChildren, bool expanded)
{
   AssertFatal(mOwningTree == NULL || mOwningTree == tree, "NewGuiTreeRow::configure - a row must never move between owning trees.");
   AssertFatal(mOwningGroup == NULL || mOwningGroup == group, "NewGuiTreeRow::configure - a row must never move between owning groups.");

   NewGuiTreeDataSource* source = tree ? tree->getDataSource() : NULL;

   if (mPayload && mPayload != payload && source)
      source->unregisterInterest(mPayload, this);

   mOwningTree = tree;
   mOwningGroup = group;

   if (payload && payload != mPayload && source)
      source->registerInterest(payload, this);

   mPayload = payload;
   mDepth = depth;
   mHasChildren = hasChildren;
   mExpanded = expanded;

   mIconName = tree ? NewGuiTree::resolveClassIcon(payload) : NULL;

   const char* label = (payload && source) ? source->getDisplayName(payload) : "";
   mText.setText(label);

   setContentDirty();
   setArrangementDirty();
}

void NewGuiTreeRow::resolveFont()
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

RectI NewGuiTreeRow::getExpandArrowRect() const
{
   if (!mHasChildren)
      return RectI(0, 0, 0, 0);

   S32 indent = mOwningTree ? mDepth * mOwningTree->getIndentWidth() : 0;
   return RectI(indent, 0, kTreeArrowWidth, mBounds.extent.y);
}

RectI NewGuiTreeRow::getIconRect() const
{
   S32 indent = mOwningTree ? mDepth * mOwningTree->getIndentWidth() : 0;
   S32 iconX = indent + kTreeArrowWidth;
   return RectI(iconX, (mBounds.extent.y - kTreeIconWidth) / 2, kTreeIconWidth, kTreeIconWidth);
}

void NewGuiTreeRow::beginRename()
{
   if (mRenameEdit || !mPayload || !mOwningTree || !mOwningTree->getAllowRename())
      return;

   mRenameEdit = new NewGuiTextEdit();
   mRenameEdit->registerObject();
   mRenameEdit->setText(mText.getText().c_str());
   mRenameEdit->setMultiLine(false);
   addObject(mRenameEdit);

   setArrangementDirty();
   mRenameEdit->setFirstResponder(true);
}

void NewGuiTreeRow::commitRename()
{
   if (!mRenameEdit)
      return;

   // Copied out (String, not const char*) BEFORE mRenameEdit is destroyed below - the source
   // buffer this pointer would otherwise reference does not outlive this function.
   String newText = mRenameEdit->getText();
   SimObjectPtr<SimObject> payload = mPayload;
   NewGuiTree* tree = mOwningTree;

   removeObject(mRenameEdit);
   mRenameEdit->deleteObject();
   mRenameEdit = NULL;

   // requestApplyRename(), not applyRename() directly - onRename_callback() reaches script,
   // which can delete anything while this handler is still on the stack. See
   // NewGuiTree::mPendingActionType's doc comment.
   if (payload && tree)
      tree->requestApplyRename(payload, newText.c_str());

   setContentDirty();
   setArrangementDirty();
}

void NewGuiTreeRow::cancelRename()
{
   if (!mRenameEdit)
      return;

   removeObject(mRenameEdit);
   mRenameEdit->deleteObject();
   mRenameEdit = NULL;

   setArrangementDirty();
}

Point2I NewGuiTreeRow::ComputePreferredSize()
{
   resolveFont();

   const NewGuiResolvedStyle& style = getResolvedStyle();

   mText.setBoxExtent(Point2I(0, 0));
   const NewGuiTextLayoutResult& result = mText.layout();

   S32 indent = mOwningTree ? mDepth * mOwningTree->getIndentWidth() : 0;
   S32 rowHeight = mOwningTree ? mOwningTree->getRowHeight() : 20;

   S32 width = indent + kTreeArrowWidth + kTreeIconWidth + kTreeIconGap + kTreeLabelPadding * 2 + result.blockBounds.extent.x;
   width += style.padding.left + style.padding.right;

   return Point2I(width, rowHeight);
}

void NewGuiTreeRow::EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer)
{
   Parent::EmitDrawCommands(batch, bounds, style, layer);

   bool selected = mOwningTree && mOwningTree->getSelectedItem() == mPayload && mPayload != NULL;
   if (selected)
   {
      ColorI highlight = style.secondaryColor;
      highlight.alpha = 96;
      batch->pushQuad(bounds, highlight, layer);
   }

   resolveFont();

   // Expand arrow - two-stroke chevron (no filled-triangle primitive on NewGuiRenderBatch;
   // pushLine() is what every other glyph-shaped affordance in this system reduces to).
   if (mHasChildren)
   {
      RectI arrowLocal = getExpandArrowRect();
      Point2I center(bounds.point.x + arrowLocal.point.x + arrowLocal.extent.x / 2,
         bounds.point.y + arrowLocal.point.y + arrowLocal.extent.y / 2);
      S32 r = 3;

      if (mExpanded)
      {
         batch->pushLine(Point2I(center.x - r, center.y - r / 2), Point2I(center.x, center.y + r / 2), style.textColor, 1.5f, layer);
         batch->pushLine(Point2I(center.x, center.y + r / 2), Point2I(center.x + r, center.y - r / 2), style.textColor, 1.5f, layer);
      }
      else
      {
         batch->pushLine(Point2I(center.x - r / 2, center.y - r), Point2I(center.x + r / 2, center.y), style.textColor, 1.5f, layer);
         batch->pushLine(Point2I(center.x + r / 2, center.y), Point2I(center.x - r / 2, center.y + r), style.textColor, 1.5f, layer);
      }
   }

   // Class icon - via skin images keyed by mIconName, flat-color placeholder fallback.
   {
      RectI iconLocal = getIconRect();
      RectI iconScreen(bounds.point.x + iconLocal.point.x, bounds.point.y + iconLocal.point.y, iconLocal.extent.x, iconLocal.extent.y);

      const NewGuiSkinImage* icon = mIconName ? style.findSkinImage(mIconName) : NULL;
      if (icon && icon->hasImage())
         NewGuiStyleDrawSkinImage(batch, iconScreen, *icon, style.opacity, layer);
      else
         batch->pushQuad(iconScreen, style.secondaryColor, layer);
   }

   // Label - suppressed while mRenameEdit is live (drawn as this row's own child instead).
   if (!mRenameEdit)
   {
      RectI iconLocal = getIconRect();
      S32 textX = iconLocal.point.x + iconLocal.extent.x + kTreeIconGap;
      Point2I textPos(bounds.point.x + textX, bounds.point.y);

      mText.setBoxExtent(Point2I(bounds.extent.x - textX - kTreeLabelPadding, bounds.extent.y));
      mText.submit(*batch, textPos, style.textColor, layer);
   }

   // Drop-indicator feedback, drawn by whichever row is actually dragging, against
   // mDropTargetRow's bounds.
   if (mDragInProgress && mDropTargetRow)
   {
      RectI targetBounds = mDropTargetRow->getBounds();
      if (mDropAsChild)
      {
         ColorI childHighlight = style.secondaryColor;
         childHighlight.alpha = 128;
         batch->pushQuad(targetBounds, childHighlight, layer + 1);
      }
      else
      {
         S32 lineY = targetBounds.point.y + targetBounds.extent.y;
         batch->pushLine(Point2I(targetBounds.point.x, lineY), Point2I(targetBounds.point.x + targetBounds.extent.x, lineY), style.secondaryColor, 2.0f, layer + 1);
      }
   }
}

void NewGuiTreeRow::onMouseDown(NewGuiInputEvent& event)
{
   // Arm-only, same shape as NewGuiButton::onMouseDown() - records where the press landed and
   // claims the event, performs no action itself. Destructive/script-reaching work only ever
   // happens from onMouseUp(), once the release point is known - see mPressArmed's doc comment.
   mPressArmed = true;
   mPressStartedOnArrow = mHasChildren && getExpandArrowRect().pointInRect(event.localPoint);
   mPressStartScreenPoint = event.screenPoint;
   mDragInProgress = false;
   mDropTargetRow = NULL;

   event.handled = true;
}

void NewGuiTreeRow::onInputEvent(NewGuiInputEvent& event)
{
   if (event.action != NewGuiInputAction::Move || !mPressArmed)
   {
      Parent::onInputEvent(event);
      return;
   }

   // A press that started on the arrow never becomes a drag - only a press on the row body
   // (label/icon area) can turn into a reparent drag. Falls through to Parent::onInputEvent()
   // (a no-op default) rather than claiming the event, same as the drag-threshold-not-yet-met
   // case just below.
   if (mPressStartedOnArrow)
      return;

   Point2I delta = event.screenPoint - mPressStartScreenPoint;
   S32 distSq = delta.x * delta.x + delta.y * delta.y;

   if (!mDragInProgress)
   {
      if (!mOwningTree || !mOwningTree->getAllowReparenting())
         return;

      if (distSq < kTreeDragThresholdPx * kTreeDragThresholdPx)
         return;

      mDragInProgress = true;
      pushCursor(NewGuiCursorShape::Pointer);
   }

   NewGuiTreeRow* target = NULL;
   bool asChild = false;
   if (mOwningTree)
      mOwningTree->resolveDropAtPoint(event.screenPoint, target, asChild);

   mDropTargetRow = target;
   mDropAsChild = asChild;

   bool valid = target && mOwningTree && mOwningTree->isValidDropTarget(mPayload, target, asChild);

   popCursor();
   pushCursor(valid ? NewGuiCursorShape::Pointer : NewGuiCursorShape::NotAllowed);

   event.handled = true;
}

void NewGuiTreeRow::onMouseUp(NewGuiInputEvent& event)
{
   if (mDragInProgress)
   {
      popCursor();

      // requestCommitDrop(), NOT commitDrop() - committing a drop can destroy this very row
      // (reparenting/reordering triggers NewGuiTree::rebuildRoots(), which can tear down and
      // recreate arbitrary parts of the visible tree, including `this`) - see
      // NewGuiTree::mPendingActionType's own doc comment for the crash calling it directly from
      // here produces. Everything read below (mPayload, mDropTargetRow->getPayload(),
      // mDropAsChild) is captured/queued BEFORE any of this row's own fields are cleared, and
      // none of these reads/writes can themselves trigger destruction, so they're still safe to
      // do directly here.
      if (mOwningTree && mPayload && mDropTargetRow && mDropTargetRow->getPayload()
         && mOwningTree->isValidDropTarget(mPayload, mDropTargetRow, mDropAsChild))
      {
         mOwningTree->requestCommitDrop(mPayload, mDropTargetRow->getPayload(), mDropAsChild);
      }

      mDragInProgress = false;
      mDropTargetRow = NULL;
      mPressArmed = false;
      event.handled = true;
      return;
   }

   // Release-point re-check, same as NewGuiButton::onMouseUp()'s own
   // "wasArmed && localBounds.pointInRect(event.localPoint)" - a press that started on the arrow
   // but was dragged off and released over the label (or vice versa) does neither action.
   bool wasArmed = mPressArmed;
   bool startedOnArrow = mPressStartedOnArrow;
   mPressArmed = false;

   if (!wasArmed || !mOwningTree || !mPayload)
   {
      event.handled = true;
      return;
   }

   const RectI localBounds(Point2I(0, 0), mBounds.extent);
   if (!localBounds.pointInRect(event.localPoint))
   {
      event.handled = true;
      return;
   }

   if (startedOnArrow)
   {
      if (mHasChildren && getExpandArrowRect().pointInRect(event.localPoint))
      {
         // requestExpandedChange(), NOT setExpanded() - toggling can destroy this very row (see
         // NewGuiTree::mPendingActionType's own doc comment for the crash calling setExpanded()
         // directly from a handler still on the stack produces). Deferred to the next
         // MeasurePass() instead.
         mOwningTree->requestExpandedChange(mPayload, !mExpanded);
      }
   }
   else if (!getExpandArrowRect().pointInRect(event.localPoint))
   {
      if (mRenameEdit)
      {
         // Click elsewhere on the row while renaming commits first - commitRename() only tears
         // down this row's OWN mRenameEdit child directly (safe: a fresh, single-owner child,
         // not tree structure reachable from anywhere else), then queues the actual
         // applyRename()/onRename_callback() the same deferred way - see commitRename()'s own
         // comment for why even that part can't run synchronously from here either.
         commitRename();
      }
      else
      {
         // requestRowClick(), NOT onRowClick_callback()/setSelectedItem()/beginRenameItem()
         // called directly - onRowClick_callback() reaches script, and setSelectedItem() reaches
         // script via onSelect_callback() - either can delete anything, including this row's own
         // payload or ancestry, from inside this handler. See
         // NewGuiTree::mPendingActionType's own doc comment.
         mOwningTree->requestRowClick(mPayload, event.clickCount);
      }
   }

   event.handled = true;
}

//=============================================================================
// NewGuiTreeGroup
//=============================================================================

IMPLEMENT_CONOBJECT(NewGuiTreeGroup);

NewGuiTreeGroup::NewGuiTreeGroup()
   : mOwningTree(NULL),
   mParentGroup(NULL),
   mDepth(0)
{
   // Fixed configuration, not authorable - this control only ever uses one Stack setup. Matches
   // NewGuiConsole::NewGuiConsole()'s own "mAxis = StackAxis_Vertical;" pattern for a Stack
   // subclass that isn't meant to be reconfigured from script.
   mAxis = StackAxis_Vertical;
   mSpacing = 0;
}

NewGuiTreeGroup::~NewGuiTreeGroup()
{
}

bool NewGuiTreeGroup::onAdd()
{
   return Parent::onAdd();
}

void NewGuiTreeGroup::onRemove()
{
   if (mGroupItem && mOwningTree && mOwningTree->getDataSource())
      mOwningTree->getDataSource()->unregisterInterest(mGroupItem, mOwningTree);

   Parent::onRemove();
}

void NewGuiTreeGroup::configure(NewGuiTree* tree, NewGuiTreeGroup* parentGroup, SimObject* groupItem, S32 depth)
{
   mOwningTree = tree;
   mParentGroup = parentGroup;
   mGroupItem = groupItem;

   // depth here is the depth this group's OWN CHILDREN's rows render at - NOT groupItem's own
   // depth (groupItem already has its own row, one level up, in parentGroup's ChildSlot - see
   // this class's own doc comment on why this group never draws a redundant row for groupItem
   // itself). Callers pass childDepth already incremented past groupItem's own row's depth - see
   // expandSlot()'s own comment for exactly where that +1 happens.
   mDepth = depth;

   if (groupItem && tree && tree->getDataSource())
   {
      // Interest in the group's OWN item is registered against the tree, not this group -
      // NewGuiTree::onDeleteNotify() is the single place that reacts to a group-item deletion
      // (see that method), so every group-item watch funnels through the same object
      // regardless of nesting depth, rather than each nested NewGuiTreeGroup needing its own
      // onDeleteNotify() override duplicating the same pruning logic. Still needed even though
      // this group draws no row of its own for groupItem - the WATCH is independent of whether
      // a row exists to display it.
      tree->getDataSource()->registerInterest(groupItem, tree);
   }

   syncChildren();
}

void NewGuiTreeGroup::syncChildren()
{
   if (!mOwningTree || !mOwningTree->getDataSource())
      return;

   NewGuiTreeDataSource* source = mOwningTree->getDataSource();

   Vector<SimObject*> currentItems;
   if (mGroupItem)
      source->getChildren(mGroupItem, currentItems);
   else
      source->getRoots(currentItems);   // Top-level synthetic group - see class doc comment.

   // Vector<T> has no swap() - snapshot the current contents by copy, then clear mChildSlots so
   // the reconciliation loop below can push_back the rebuilt list fresh. oldSlots is only ever
   // read from here on (reused rows/groups are re-parented into mChildSlots, not written back
   // into oldSlots), so the extra copy costs nothing beyond this one rebuild.
   Vector<ChildSlot> oldSlots = mChildSlots;
   mChildSlots.clear();

   // childDepth is simply mDepth now - mDepth already means "this group's own children's row
   // depth" directly (see configure()'s own comment).
   S32 childDepth = mDepth;

   for (U32 i = 0; i < currentItems.size(); ++i)
   {
      SimObject* item = currentItems[i];

      // Reuse an existing slot for this item, preserving its row/childGroup/expanded state -
      // see ChildSlot's own doc comment on why a slot's row and childGroup are never destroyed
      // and recreated just because a resync happened; only a genuinely REMOVED item's slot
      // content is destroyed, in the cleanup pass below.
      S32 oldIndex = -1;
      for (U32 j = 0; j < oldSlots.size(); ++j)
      {
         if (oldSlots[j].item == item)
         {
            oldIndex = (S32)j;
            break;
         }
      }

      bool hasChildren = source->hasChildren(item);

      ChildSlot slot;
      slot.item = item;
      slot.row = NULL;
      slot.childGroup = NULL;
      slot.expanded = false;

      if (oldIndex >= 0)
      {
         // Already existed - reuse its row (re-configure in place, same object, same identity)
         // and whatever childGroup it already had (untouched either way; visibility is what
         // setChildExpanded()/collapseSlot()/expandSlot() toggle, not existence).
         slot.row = oldSlots[oldIndex].row;
         slot.childGroup = oldSlots[oldIndex].childGroup;
         slot.expanded = oldSlots[oldIndex].expanded;

         oldSlots[oldIndex].row = NULL;         // Claimed - the cleanup pass below skips it.
         oldSlots[oldIndex].childGroup = NULL;   // Claimed - see above.

         slot.row->configure(mOwningTree, this, item, childDepth, hasChildren, slot.expanded);

         if (slot.childGroup)
         {
            addObject(slot.childGroup);   // No-op if already a child; safe either way.
            slot.childGroup->syncChildren();
            slot.childGroup->setVisible(slot.expanded);
         }
      }
      else
      {
         // New item - not present in the old slot list at all. Only the row is created now;
         // childGroup is created lazily on first expand (see expandSlot()) rather than for
         // every item up front, since most items in a real tree are never expanded.
         NewGuiTreeRow* row = new NewGuiTreeRow();
         row->registerObject();
         addObject(row);
         row->configure(mOwningTree, this, item, childDepth, hasChildren, false);
         slot.row = row;
      }

      mChildSlots.push_back(slot);
   }

   // Destroy whatever old slot content wasn't claimed above - items no longer present in
   // currentItems at all (a real structural removal, not a UI collapse). Never a bare `delete`;
   // always removeObject() + deleteObject().
   for (U32 j = 0; j < oldSlots.size(); ++j)
   {
      if (oldSlots[j].row)
      {
         removeObject(oldSlots[j].row);
         oldSlots[j].row->deleteObject();
      }
      if (oldSlots[j].childGroup)
      {
         removeObject(oldSlots[j].childGroup);
         oldSlots[j].childGroup->deleteObject();   // Recursively tears down everything under it via ordinary SimGroup teardown.
      }
   }

   setContentDirty();
   setArrangementDirty();
}

void NewGuiTreeGroup::expandSlot(ChildSlot& slot)
{
   if (!slot.item || slot.expanded)
      return;

   // slot.item's own row lives at mDepth (this group's own children's row depth - see
   // configure()'s own comment). Its childGroup's CHILDREN therefore render one level deeper.
   S32 childDepth = mDepth + 1;

   if (!slot.childGroup)
   {
      // First-ever expand for this item - create the group now (lazily, not up front in
      // syncChildren(), since most items in a real tree are never expanded). NEVER destroyed
      // by collapseSlot() afterward - see ChildSlot's own doc comment for why: destroying and
      // recreating a control that could currently be NewGuiCanvas::mMouseOverControl (a raw
      // pointer, invalidated with no notification when its target is destroyed) is what
      // produced a real dangling-pointer crash on ordinary hover after a collapse. Every other
      // dynamic-child control in this system (NewGuiStack, NewGuiScroll) avoids this simply by
      // never destroying a child as a side effect of ordinary interaction at all - this matches
      // that by only ever destroying a slot's row/childGroup when the ITEM itself is actually
      // gone (see syncChildren()'s cleanup pass), never for a UI-only collapse.
      NewGuiTreeGroup* group = new NewGuiTreeGroup();
      group->registerObject();
      addObject(group);
      group->configure(mOwningTree, this, slot.item, childDepth);
      slot.childGroup = group;
   }
   else
   {
      // Already exists from a previous expand - just re-sync (the underlying data may have
      // changed while collapsed) and show it again.
      slot.childGroup->syncChildren();
      slot.childGroup->setVisible(true);
   }

   // addObject() (above, first-time only) always appends to the END of this Stack's child list
   // (SimSet::addObject() -> pushBack()) - which puts childGroup in the wrong visual position
   // for any item that isn't the last one ever expanded. reOrder() it to sit immediately after
   // its own slot's row (and therefore before whatever slot comes next), so Stack's own child-
   // order-driven layout places it directly under its row every time, not at the bottom of the
   // whole list.
   S32 slotIndex = -1;
   for (U32 i = 0; i < mChildSlots.size(); ++i)
   {
      if (&mChildSlots[i] == &slot)
      {
         slotIndex = (S32)i;
         break;
      }
   }

   SimObject* reorderTarget = NULL;   // NULL = move to the very end, reOrder()'s own "last object" case.
   if (slotIndex >= 0)
   {
      for (U32 i = slotIndex + 1; i < mChildSlots.size(); ++i)
      {
         if (mChildSlots[i].row)
         {
            reorderTarget = mChildSlots[i].row;
            break;
         }
      }
   }
   reOrder(slot.childGroup, reorderTarget);

   slot.expanded = true;

   // mDepth, NOT childDepth - slot.row's own depth never changes on expand/collapse; only
   // hasChildren/expanded (the arrow's state) does. childDepth above is one level deeper, for
   // childGroup's OWN children, not for this row.
   if (slot.row)
      slot.row->configure(mOwningTree, this, slot.item, mDepth, true, true);
}

void NewGuiTreeGroup::collapseSlot(ChildSlot& slot)
{
   if (!slot.childGroup || !slot.expanded)
      return;

   // setVisible(false), NOT destruction - see expandSlot()'s own comment on why a collapse must
   // never destroy slot.childGroup. The whole subtree stays alive, off-screen, unhittable
   // (NewGuiControl::findHitControl() early-outs on !mVisible) and unrendered
   // (NewGuiControl::RenderPass() early-outs the same way) until expanded again.
   slot.childGroup->setVisible(false);
   slot.expanded = false;

   // mDepth directly - slot.row's own depth never changes on collapse, only hasChildren/
   // expanded does (see expandSlot()'s matching comment).
   bool hasChildren = mOwningTree && mOwningTree->getDataSource() ? mOwningTree->getDataSource()->hasChildren(slot.item) : false;

   if (slot.row)
      slot.row->configure(mOwningTree, this, slot.item, mDepth, hasChildren, false);
}


bool NewGuiTreeGroup::setChildExpanded(SimObject* item, bool expanded)
{
   for (U32 i = 0; i < mChildSlots.size(); ++i)
   {
      if (mChildSlots[i].item != item)
         continue;

      bool currentlyExpanded = mChildSlots[i].expanded;
      if (currentlyExpanded == expanded)
         return true;

      if (expanded)
         expandSlot(mChildSlots[i]);
      else
         collapseSlot(mChildSlots[i]);

      setContentDirty();
      setArrangementDirty();
      return true;
   }

   return false;
}

bool NewGuiTreeGroup::setExpandedRecursive(SimObject* item, bool expanded)
{
   if (setChildExpanded(item, expanded))
      return true;

   for (U32 i = 0; i < mChildSlots.size(); ++i)
   {
      if (mChildSlots[i].childGroup && mChildSlots[i].childGroup->setExpandedRecursive(item, expanded))
         return true;
   }

   return false;
}

bool NewGuiTreeGroup::isDescendantOrSelf(SimObject* draggedItem, SimObject* item)
{
   if (item == draggedItem)
      return true;

   // Find draggedItem's own slot (at any depth) and, if it's currently expanded, check whether
   // item falls anywhere within that nested group's subtree.
   for (U32 i = 0; i < mChildSlots.size(); ++i)
   {
      if (mChildSlots[i].item == draggedItem)
      {
         if (!mChildSlots[i].expanded)
            return false;   // draggedItem isn't expanded - it has no visible descendants to collide with.

         Vector<NewGuiTreeRow*> descendantRows;
         mOwningTree->collectVisibleRows(mChildSlots[i].childGroup, descendantRows);
         for (U32 d = 0; d < descendantRows.size(); ++d)
         {
            if (descendantRows[d]->getPayload() == item)
               return true;
         }
         return false;
      }

      if (mChildSlots[i].childGroup && mChildSlots[i].childGroup->isDescendantOrSelf(draggedItem, item))
         return true;
   }

   return false;
}

NewGuiTreeRow* NewGuiTreeGroup::findVisibleRow(SimObject* item)
{
   for (U32 i = 0; i < mChildSlots.size(); ++i)
   {
      if (mChildSlots[i].row && mChildSlots[i].row->getPayload() == item)
         return mChildSlots[i].row;

      // Only recurse into an EXPANDED (visible) childGroup - one that exists but is currently
      // collapsed/hidden has no visible rows, even though the group object itself still exists
      // (see ChildSlot's own doc comment on why it's never destroyed on collapse).
      if (mChildSlots[i].expanded && mChildSlots[i].childGroup)
      {
         NewGuiTreeRow* found = mChildSlots[i].childGroup->findVisibleRow(item);
         if (found)
            return found;
      }
   }

   return NULL;
}

//=============================================================================
// NewGuiTree
//=============================================================================

IMPLEMENT_CONOBJECT(NewGuiTree);

IMPLEMENT_CALLBACK(NewGuiTree, onSelect, void, (SimObject* item), (item),
   "Called whenever the selected item changes, including to no selection (item == NULL).");

IMPLEMENT_CALLBACK(NewGuiTree, onReparent, void, (SimObjectId item, SimObjectId newParent), (item, newParent),
   "Called after a drag-drop successfully reparents/reorders item under newParent.");

IMPLEMENT_CALLBACK(NewGuiTree, onRename, void, (SimObject* item, const char* newName), (item, newName),
   "Called after an inline rename has already been applied to item.");

NewGuiTree::NewGuiTree()
   : mDataSource(NULL),
   mRowHeight(20),
   mIndentWidth(16),
   mAllowReparenting(true),
   mAllowRename(true),
   mLastClickTimeMS(0),
   mPendingActionType(PendingAction_None),
   mPendingActionBool(false),
   mPendingActionClickCount(0)
{
   // Same reasoning as NewGuiTreeGroup::NewGuiTreeGroup() - fixed configuration, not authorable.
   mAxis = StackAxis_Vertical;
   mSpacing = 0;
}

NewGuiTree::~NewGuiTree()
{
   delete mDataSource;
}

void NewGuiTree::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Tree");

   ADD_FIELD("rowHeight", TypeS32, Offset(mRowHeight, NewGuiTree))
      .onSet(_setRowHeight)
      .doc("Fixed pixel height per row.");

   ADD_FIELD("indentWidth", TypeS32, Offset(mIndentWidth, NewGuiTree))
      .onSet(_setIndentWidth)
      .doc("Pixels of indent per depth level.");

   ADD_FIELD("allowReparenting", TypeBool, Offset(mAllowReparenting, NewGuiTree))
      .onSet(_setAllowReparenting)
      .doc("Whether rows can be dragged to reparent/reorder. Rows stay selectable/renameable either way.");

   ADD_FIELD("allowRename", TypeBool, Offset(mAllowRename, NewGuiTree))
      .onSet(_setAllowRename)
      .doc("Whether double-click (or a second click on an already-selected row) starts an inline rename.");

   GROUP_END("Tree");
}

bool NewGuiTree::_setRowHeight(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTree*>(obj)->mRowHeight = dAtoi(data);
   static_cast<NewGuiTree*>(obj)->setContentDirty();
   static_cast<NewGuiTree*>(obj)->setArrangementDirty();
   return false;
}

bool NewGuiTree::_setIndentWidth(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTree*>(obj)->mIndentWidth = dAtoi(data);
   static_cast<NewGuiTree*>(obj)->setContentDirty();
   static_cast<NewGuiTree*>(obj)->setArrangementDirty();
   return false;
}

bool NewGuiTree::_setAllowReparenting(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTree*>(obj)->mAllowReparenting = dAtob(data);
   return false;
}

bool NewGuiTree::_setAllowRename(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiTree*>(obj)->mAllowRename = dAtob(data);
   return false;
}

bool NewGuiTree::onAdd()
{
   return Parent::onAdd();
}

void NewGuiTree::onRemove()
{
   // mTopLevelGroups are this control's own SimGroup children - none of NewGuiTree/NewGuiStack/
   // NewGuiControl override SimGroup::onRemove(), so Parent::onRemove() reaches
   // SimGroup::onRemove() -> clear(), which deleteObject()s every child, recursing depth-first
   // through every nested NewGuiTreeGroup. Each one's own onRemove() (unregistering its
   // mGroupItem interest against `this`) already runs before this method returns, so only
   // mSelectedItem - the one notify this object holds directly - needs handling here.
   if (mSelectedItem)
      clearNotify(mSelectedItem);

   Parent::onRemove();
}

void NewGuiTree::onDeleteNotify(SimObject* object)
{
   if (object == mSelectedItem)
   {
      mSelectedItem = NULL;
      onSelect_callback(NULL);
   }

   // A group-item deletion (registered against `this` by NewGuiTreeGroup::configure()) means
   // some NewGuiTreeGroup's mGroupItem == object and its whole slot needs pruning from its
   // parent's mChildSlots. Rather than search for the exact group, the simplest correct response
   // is the same one setRoot()/setFileSystemRoots() already use: re-derive from the data source,
   // which naturally excludes the now-deleted object from whatever it queries next.
   rebuildRoots();

   Parent::onDeleteNotify(object);
}

void NewGuiTree::setDataSource(NewGuiTreeDataSource* source)
{
   // Tear down the old source's tree first, THEN swap - a stale mTopLevelGroups entry must
   // never be re-synced against a source it wasn't built from.
   for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
   {
      if (mTopLevelGroups[i])
      {
         removeObject(mTopLevelGroups[i]);
         mTopLevelGroups[i]->deleteObject();
      }
   }
   mTopLevelGroups.clear();

   delete mDataSource;
   mDataSource = source;

   rebuildRoots();
}

void NewGuiTree::setRoot(SimObject* root)
{
   setDataSource(new NewGuiTreeSimGroupSource(root));
}

void NewGuiTree::setRoots(const Vector<SimObject*>& roots)
{
   setDataSource(new NewGuiTreeSimGroupSource(roots));
}

void NewGuiTree::setFileSystemRoots(const Vector<String>& rootPaths)
{
   setDataSource(new NewGuiTreeFileSystemSource(rootPaths));
}

void NewGuiTree::rebuildRoots()
{
   if (!mDataSource)
   {
      for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
      {
         if (mTopLevelGroups[i])
         {
            removeObject(mTopLevelGroups[i]);
            mTopLevelGroups[i]->deleteObject();
         }
      }
      mTopLevelGroups.clear();
      setContentDirty();
      setArrangementDirty();
      return;
   }

   Vector<SimObject*> roots;
   mDataSource->getRoots(roots);

   // Same reasoning as NewGuiTreeGroup::syncChildren() above - Vector<T> has no swap().
   Vector<SimObjectPtr<NewGuiTreeGroup> > oldGroups = mTopLevelGroups;
   mTopLevelGroups.clear();

   for (U32 i = 0; i < roots.size(); ++i)
   {
      SimObject* root = roots[i];

      S32 oldIndex = -1;
      for (U32 j = 0; j < oldGroups.size(); ++j)
      {
         if (oldGroups[j] && oldGroups[j]->getGroupItem() == root)
         {
            oldIndex = (S32)j;
            break;
         }
      }

      if (oldIndex >= 0)
      {
         NewGuiTreeGroup* group = oldGroups[oldIndex];
         oldGroups[oldIndex] = NULL;
         group->syncChildren();
         mTopLevelGroups.push_back(group);

         // reOrder() to this position - a reused group otherwise stays wherever it already was
         // in Stack child order, which can be visually wrong if getRoots() order changed since
         // the last rebuild (same reasoning as NewGuiTreeGroup::expandSlot()'s own reOrder()
         // call). NULL target = move to the end; a real target is filled in on the NEXT loop
         // iteration if this isn't the last root, via the same mechanism used there.
         reOrder(group, NULL);
      }
      else
      {
         NewGuiTreeGroup* group = new NewGuiTreeGroup();
         group->registerObject();
         addObject(group);
         group->configure(this, NULL, root, 0);
         mTopLevelGroups.push_back(group);
      }
   }

   for (U32 j = 0; j < oldGroups.size(); ++j)
   {
      if (oldGroups[j])
      {
         removeObject(oldGroups[j]);
         oldGroups[j]->deleteObject();
      }
   }

   setContentDirty();
   setArrangementDirty();
}

StringTableEntry NewGuiTree::resolveClassIcon(SimObject* object)
{
   if (!object)
      return StringTable->insert("default");

   if (object->isMethod("getClassIconName"))
   {
      const char* iconName = Con::executef(object, "getClassIconName");
      if (iconName && iconName[0])
         return StringTable->insert(iconName);
   }

   return StringTable->insert(object->getClassName());
}

void NewGuiTree::setExpanded(SimObject* item, bool expanded)
{
   for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
   {
      if (mTopLevelGroups[i] && mTopLevelGroups[i]->setExpandedRecursive(item, expanded))
         return;
   }
}

// Marks this tree dirty so the driver actually revisits it (and therefore calls MeasurePass()
// again) next frame - without this, a queued request()* call sits in mPendingActionType forever
// if nothing else happens to dirty this subtree first, since the driver skips a subtree entirely
// when none of its dirty flags are set (see this system's own frame-loop contract, §3.3: "if
// none are set, that subtree is skipped entirely for that frame"). This was the actual cause of
// "only dragging works" - drag's own visual feedback is read directly by EmitDrawCommands() every
// RenderPass (which always runs, dirty or not), but selecting/expanding/renaming only take
// effect via flushPendingAction(), called from MeasurePass() only - so without marking dirty
// here, a click's requestRowClick() was queued correctly but never actually got processed.
void NewGuiTree::markPendingActionDirty()
{
   setContentDirty();
   setArrangementDirty();
}

void NewGuiTree::requestExpandedChange(SimObject* item, bool expanded)
{
   // Last request wins - see mPendingActionType's own doc comment. Only one pending action of
   // any kind at a time; a second request before the first flushes simply replaces it.
   mPendingActionType = PendingAction_SetExpanded;
   mPendingActionItem = item;
   mPendingActionItem2 = NULL;
   mPendingActionBool = expanded;
   mPendingActionClickCount = 0;
   mPendingActionText = String::EmptyString;
   markPendingActionDirty();
}

void NewGuiTree::requestCommitDrop(SimObject* draggedItem, SimObject* targetItem, bool asChild)
{
   mPendingActionType = PendingAction_CommitDrop;
   mPendingActionItem = draggedItem;
   mPendingActionItem2 = targetItem;
   mPendingActionBool = asChild;
   mPendingActionClickCount = 0;
   mPendingActionText = String::EmptyString;
   markPendingActionDirty();
}

void NewGuiTree::requestRowClick(SimObject* item, S32 clickCount)
{
   mPendingActionType = PendingAction_RowClick;
   mPendingActionItem = item;
   mPendingActionItem2 = NULL;
   mPendingActionBool = false;
   mPendingActionClickCount = clickCount;
   mPendingActionText = String::EmptyString;
   markPendingActionDirty();
}

void NewGuiTree::requestApplyRename(SimObject* item, const char* newName)
{
   mPendingActionType = PendingAction_ApplyRename;
   mPendingActionItem = item;
   mPendingActionItem2 = NULL;
   mPendingActionBool = false;
   mPendingActionClickCount = 0;
   mPendingActionText = newName ? newName : "";
   markPendingActionDirty();
}

void NewGuiTree::flushPendingAction()
{
   if (mPendingActionType == PendingAction_None)
      return;

   // Snapshot and clear BEFORE applying - see mPendingActionType's own doc comment on why this
   // ordering matters (a re-entrant request() call during the apply below must queue for the
   // NEXT flush, not be silently dropped by a flag this call is about to clear anyway).
   PendingActionType type = mPendingActionType;
   SimObjectPtr<SimObject> item = mPendingActionItem;
   SimObjectPtr<SimObject> item2 = mPendingActionItem2;
   bool boolArg = mPendingActionBool;
   S32 clickCount = mPendingActionClickCount;
   String textArg = mPendingActionText;

   mPendingActionType = PendingAction_None;
   mPendingActionItem = NULL;
   mPendingActionItem2 = NULL;
   mPendingActionBool = false;
   mPendingActionClickCount = 0;
   mPendingActionText = String::EmptyString;

   switch (type)
   {
   case PendingAction_SetExpanded:
      if (item)
         setExpanded(item, boolArg);
      break;

   case PendingAction_CommitDrop:
      if (item && item2)
         commitDrop(item, item2, boolArg);
      break;

   case PendingAction_RowClick:
      // Mirrors what NewGuiTreeRow::onMouseUp() used to do directly, inline, before it became
      // unsafe to run synchronously (onRowClick_callback() reaches script, which can delete
      // anything) - see mPendingActionType's own doc comment. onRowClick is a per-ROW callback
      // (IMPLEMENT_CALLBACK is on NewGuiTreeRow, not NewGuiTree), so the row displaying item is
      // looked up fresh here rather than held onto from onMouseUp() - by the time this flush
      // runs, structure may already differ from what it was when the click happened, and this
      // is exactly the re-lookup that keeps this callback from ever firing on a stale/dangling
      // row pointer.
      if (item)
      {
         for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
         {
            if (!mTopLevelGroups[i])
               continue;

            NewGuiTreeRow* row = mTopLevelGroups[i]->findVisibleRow(item);
            if (row)
            {
               row->onRowClick_callback(clickCount);
               break;
            }
         }

         bool alreadySelected = getSelectedItem() == item;
         setSelectedItem(item);

         if (clickCount >= 2 || (alreadySelected && clickCount == 1))
            beginRenameItem(item);
      }
      break;

   case PendingAction_ApplyRename:
      // applyRename()'s own onRename_callback() reaches script, same risk class as
      // onRowClick_callback()/onSelect_callback() above - already safe to call from here since
      // this whole method only ever runs from MeasurePass(), outside any row's own input
      // dispatch.
      if (item)
         applyRename(item, textArg.c_str());
      break;

   default:
      break;
   }
}

Point2I NewGuiTree::MeasurePass()
{
   flushPendingAction();
   return Parent::MeasurePass();
}

bool NewGuiTree::isExpanded(SimObject* item) const
{
   for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
   {
      if (mTopLevelGroups[i] && mTopLevelGroups[i]->findVisibleRow(item))
      {
         // A nested group's own header row's mExpanded is always true by construction (see
         // NewGuiTreeGroup::configure()); a leaf row's mExpanded reflects the real state.
         NewGuiTreeRow* row = mTopLevelGroups[i]->findVisibleRow(item);
         return row->getExpanded();
      }
   }
   return false;
}

void NewGuiTree::setSelectedItem(SimObject* item)
{
   if (mSelectedItem == item)
      return;

   if (mSelectedItem)
      clearNotify(mSelectedItem);

   mSelectedItem = item;

   if (mSelectedItem)
      deleteNotify(mSelectedItem);

   onSelect_callback(item);
}

void NewGuiTree::beginRenameItem(SimObject* item)
{
   if (!mAllowRename)
      return;

   for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
   {
      if (!mTopLevelGroups[i])
         continue;

      NewGuiTreeRow* row = mTopLevelGroups[i]->findVisibleRow(item);
      if (row)
      {
         row->beginRename();
         return;
      }
   }
}

void NewGuiTree::applyRename(SimObject* item, const char* newName)
{
   if (!item || !newName)
      return;

   bool legal = dIsalpha(newName[0]) || newName[0] == '_';
   if (legal)
   {
      for (const char* c = newName; *c; ++c)
      {
         if (!dIsalnum(*c) && *c != '_')
         {
            legal = false;
            break;
         }
      }
   }

   if (legal)
      item->assignName(newName);
   else
      item->setDataField(StringTable->insert("label"), NULL, newName);

   onRename_callback(item, newName);
}

void NewGuiTree::collectVisibleRows(NewGuiTreeGroup* group, Vector<NewGuiTreeRow*>& outRows)
{
   if (!group)
      return;

   for (U32 i = 0; i < group->mChildSlots.size(); ++i)
   {
      if (group->mChildSlots[i].row)
         outRows.push_back(group->mChildSlots[i].row);

      // Only recurse into an EXPANDED (visible) childGroup - a collapsed one still exists (see
      // ChildSlot's own doc comment) but its rows aren't currently visible/hittable, matching
      // NewGuiTreeGroup::findVisibleRow()'s own same rule.
      if (group->mChildSlots[i].expanded && group->mChildSlots[i].childGroup)
         collectVisibleRows(group->mChildSlots[i].childGroup, outRows);
   }
}

void NewGuiTree::resolveDropAtPoint(const Point2I& screenPoint, NewGuiTreeRow*& outRow, bool& outAsChild) const
{
   outRow = NULL;
   outAsChild = false;

   Vector<NewGuiTreeRow*> allRows;
   for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
   {
      if (mTopLevelGroups[i])
         collectVisibleRows(mTopLevelGroups[i], allRows);
   }

   for (U32 i = 0; i < allRows.size(); ++i)
   {
      NewGuiTreeRow* row = allRows[i];
      const RectI& bounds = row->getBounds();

      if (screenPoint.x < bounds.point.x || screenPoint.x >= bounds.point.x + bounds.extent.x)
         continue;
      if (screenPoint.y < bounds.point.y || screenPoint.y >= bounds.point.y + bounds.extent.y)
         continue;

      outRow = row;

      S32 localY = screenPoint.y - bounds.point.y;
      S32 third = bounds.extent.y / 3;
      outAsChild = (localY >= third && localY < bounds.extent.y - third);
      return;
   }
}

bool NewGuiTree::isValidDropTarget(SimObject* draggedItem, NewGuiTreeRow* targetRow, bool asChild) const
{
   if (!draggedItem || !targetRow || !targetRow->getPayload() || !mDataSource)
      return false;

   SimObject* targetItem = targetRow->getPayload();

   if (targetItem == draggedItem)
      return false;

   if (asChild && !mDataSource->canAcceptChild(targetItem, draggedItem))
      return false;

   // Cycle check: reject if targetItem is draggedItem itself or falls anywhere within
   // draggedItem's own currently-visible expanded subtree - delegated to NewGuiTreeGroup's own
   // recursive traversal rather than this class reaching into group internals directly.
   for (U32 i = 0; i < mTopLevelGroups.size(); ++i)
   {
      if (mTopLevelGroups[i] && mTopLevelGroups[i]->isDescendantOrSelf(draggedItem, targetItem))
         return false;
   }

   return true;
}

void NewGuiTree::commitDrop(SimObject* draggedItem, SimObject* targetItem, bool asChild)
{
   if (!draggedItem || !targetItem || !mDataSource)
      return;

   SimObject* reorderBefore = asChild ? NULL : targetItem;
   bool moved = mDataSource->moveItem(draggedItem, targetItem, asChild, reorderBefore);

   if (!moved)
      return;

   rebuildRoots();

   SimObject* newParent = asChild ? targetItem : NULL;
   onReparent_callback(draggedItem->getId(), newParent ?newParent->getId() : 0);
}

//=============================================================================
// Script (console) API - NewGuiTree
//=============================================================================
// initPersistFields()'s ADD_FIELD() calls (setRowHeight/indentWidth/etc via authored fields
// higher in this file) cover PROPERTIES set at object-creation time or via %obj.field = value;
// none of that machinery exposes ordinary C++ METHODS to script - those need their own
// DefineEngineMethod binding, one per method a .tscript file should be able to call directly
// (see NewGuiConsole::setDisplayFilters()/clear() etc. for the same split in an existing
// control). Every method below is a thin call-through to the already-implemented C++ method of
// the same name/shape - no new logic lives here.

DefineEngineMethod(NewGuiTree, setRoot, void, (SimObject* root), ,
   "Wraps a single already-existing SimObject/SimGroup as this tree's root (scene group, "
   "GuiControl hierarchy, or any other SimGroup). Pass 0 to empty the tree.\n"
   "@ingroup GuiCore")
{
   object->setRoot(root);
}

DefineEngineMethod(NewGuiTree, setRoots, void, (SimSet* roots), ,
   "Wraps several already-existing SimObject/SimGroup roots as this tree's top-level items - "
   "several independent groups shown as sibling top-level blocks with no shared parent row "
   "(e.g. several watched asset directories, each already represented as a SimGroup). Pass "
   "the roots as members of a SimSet (build one with `new SimSet() { }; %set.add(%obj);` and "
   "pass it here) rather than as separate arguments, since the underlying C++ setRoots() takes "
   "a variable-length list with no fixed arity a console method signature can express directly.\n"
   "@ingroup GuiCore")
{
   Vector<SimObject*> rootVec;
   if (roots)
   {
      for (U32 i = 0; i < roots->size(); ++i)
         rootVec.push_back(roots->at(i));
   }

   object->setRoots(rootVec);
}

DefineEngineMethod(NewGuiTree, setFileSystemRoots, void, (const char* tabSeparatedPaths), ,
   "Wraps one or more Torque::FS volume-relative paths (tab-separated in one string) as this "
   "tree's top-level roots - the asset-browser case. Volume-prefixed paths like \"shadercache:\" "
   "or \"home:\" are passed through to Torque::FS unmodified.\n"
   "@param tabSeparatedPaths One or more paths, separated by tab characters, e.g. "
   "\"art/shapes\\tart/gui\\tshadercache:\\thome:\".\n"
   "@ingroup GuiCore")
{
   Vector<String> paths;

   char buffer[1024];
   dStrncpy(buffer, tabSeparatedPaths, sizeof(buffer) - 1);
   buffer[sizeof(buffer) - 1] = '\0';

   char* token = dStrtok(buffer, "\t");
   while (token)
   {
      paths.push_back(String(token));
      token = dStrtok(NULL, "\t");
   }

   object->setFileSystemRoots(paths);
}

DefineEngineMethod(NewGuiTree, setExpanded, void, (SimObject* item, bool expanded), (true),
   "Expands or collapses item (must currently be a visible tree item). No-op if item isn't "
   "found or has no children.\n"
   "@ingroup GuiCore")
{
   if (item)
      object->setExpanded(item, expanded);
}

DefineEngineMethod(NewGuiTree, isExpanded, bool, (SimObject* item), ,
   "@return True if item is currently a visible, expanded tree item.\n"
   "@ingroup GuiCore")
{
   return item ? object->isExpanded(item) : false;
}

DefineEngineMethod(NewGuiTree, setSelectedItem, void, (SimObject* item), ,
   "Selects item (must currently be visible somewhere in the tree). Pass 0 to clear the "
   "current selection. Fires onSelect().\n"
   "@ingroup GuiCore")
{
   object->setSelectedItem(item);
}

DefineEngineMethod(NewGuiTree, getSelectedItem, SimObject*, (), ,
   "@return The currently-selected item, or 0 if nothing is selected.\n"
   "@ingroup GuiCore")
{
   return object->getSelectedItem();
}

DefineEngineMethod(NewGuiTree, beginRenameItem, void, (SimObject* item), ,
   "Starts an inline rename on item's row, if it's currently visible and allowRename is true.\n"
   "@ingroup GuiCore")
{
   if (item)
      object->beginRenameItem(item);
}
