#ifndef _NEWCONSOLE_TS2_COMPILATIONUNIT_H_
#define _NEWCONSOLE_TS2_COMPILATIONUNIT_H_

#ifndef _NEWCONSOLE_TS2_ASTSTMTS_H_
#include "newConsole/torquescript2/astStmts.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif

namespace newConsole
{
namespace ts2
{

/// Owns the flat, arena-allocated AST produced by parsing one .ts2 file.
///
/// @note Transient by design: built fresh by Parser::parse(), consumed
///   by the emit pass, then discarded. Nothing about this struct persists
///   across a compile - it is not a module/import unit and has no
///   cross-file identity; see ast::ExprHandle/StmtHandle's own comments
///   for why nodes reference each other by index into this struct's
///   arrays rather than by pointer.
///
/// @note List storage: ast::ListHandle values (argument lists, slot
///   assignments, switch cases, function params, ...) index into the
///   three list arenas below (exprList/stmtList/paramList/caseList/
///   slotList), not into exprs/stmts directly - a ListHandle's `index`
///   is the starting offset into the matching arena and `count` is how
///   many contiguous entries follow. One arena per element type keeps
///   each arena's element type concrete rather than needing a variant,
///   and keeps a reader's "which arena does this ListHandle come from"
///   question answerable from the field name it was stored under (e.g.
///   ast::CallExpr::args is always exprList, never ambiguous).
class CompilationUnit
{
public:
   /// Origin file name, for diagnostics only - not used in any AST
   /// lookup or comparison.
   StringTableEntry originName = nullptr;

   Vector<ast::ExprNode> exprs;
   Vector<ast::StmtNode> stmts;

   // List arenas, one per element type a ListHandle can point into.
   Vector<ast::ExprHandle> exprList;
   Vector<ast::StmtHandle> stmtList;
   Vector<ast::Param> paramList;
   Vector<ast::SwitchCase> caseList;
   Vector<ast::SlotAssignment> slotList;

   /// Root of the parsed file - the top-level statement list, same shape
   /// as any other ListHandle over StmtHandle. Invalid (isEmpty()) for
   /// an empty file, never for a parse failure - a failed parse is
   /// reported through Parser's diagnostics, not represented as a
   /// distinguished CompilationUnit state.
   ast::ListHandle topLevel;

   ast::ExprHandle addExpr(ast::ExprNode node)
   {
      exprs.push_back(node);
      return ast::ExprHandle{ static_cast<U32>(exprs.size() - 1) };
   }

   ast::StmtHandle addStmt(ast::StmtNode node)
   {
      stmts.push_back(node);
      return ast::StmtHandle{ static_cast<U32>(stmts.size() - 1) };
   }

   ast::ExprNode& get(ast::ExprHandle h) { return exprs[h.index]; }
   const ast::ExprNode& get(ast::ExprHandle h) const { return exprs[h.index]; }
   ast::StmtNode& get(ast::StmtHandle h) { return stmts[h.index]; }
   const ast::StmtNode& get(ast::StmtHandle h) const { return stmts[h.index]; }

   /// Appends a contiguous run to the given arena and returns a
   /// ListHandle describing it. The four overloads below are the only
   /// way a ListHandle is ever produced - callers never construct one by
   /// hand, so "index/count describe a valid contiguous range in the
   /// matching arena" is an invariant this class alone is responsible
   /// for upholding.
   ast::ListHandle addExprList(const Vector<ast::ExprHandle>& items)
   {
      return appendList(exprList, items);
   }
   ast::ListHandle addStmtList(const Vector<ast::StmtHandle>& items)
   {
      return appendList(stmtList, items);
   }
   ast::ListHandle addParamList(const Vector<ast::Param>& items)
   {
      return appendList(paramList, items);
   }
   ast::ListHandle addCaseList(const Vector<ast::SwitchCase>& items)
   {
      return appendList(caseList, items);
   }
   ast::ListHandle addSlotList(const Vector<ast::SlotAssignment>& items)
   {
      return appendList(slotList, items);
   }

   /// Read-only view of the entries a ListHandle points into. Bounds are
   /// asserted, not silently clamped - an out-of-range ListHandle means
   /// something upstream (usually a hand-rolled append outside the
   /// add*List helpers above) violated this class's invariant, and
   /// silently truncating would hide that instead of surfacing it.
   template<typename T>
   static const T* listData(const Vector<T>& arena, ast::ListHandle handle)
   {
      AssertFatal(handle.index + handle.count <= static_cast<U32>(arena.size()),
         "CompilationUnit::listData - ListHandle out of range for this arena");
      return arena.address() + handle.index;
   }

private:
   template<typename T>
   static ast::ListHandle appendList(Vector<T>& arena, const Vector<T>& items)
   {
      ast::ListHandle handle;
      handle.index = static_cast<U32>(arena.size());
      handle.count = static_cast<U32>(items.size());
      for (U32 i = 0; i < items.size(); ++i)
         arena.push_back(items[i]);
      return handle;
   }
};

} // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_COMPILATIONUNIT_H_
