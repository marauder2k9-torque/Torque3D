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
      /// @note Transient by design: built by Parser::parse(), consumed by
      ///   the emit pass, then discarded. Not a module/import unit, no
      ///   cross-file identity - see ast::ExprHandle/StmtHandle for why
      ///   nodes reference each other by index rather than pointer.
      ///
      /// @note List storage: ast::ListHandle values (argument lists, slot
      ///   assignments, switch cases, params, ...) index into the arenas
      ///   below (exprList/stmtList/paramList/caseList/slotList), not
      ///   exprs/stmts directly - index is the starting offset, count is
      ///   how many contiguous entries follow. One arena per element type
      ///   keeps each concrete (no variant needed) and makes "which arena"
      ///   answerable from the field name (e.g. CallExpr::args is always exprList).
      class CompilationUnit
      {
      public:
         /// Origin file name, for diagnostics only - never used in AST
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

         /// Root of the parsed file - top-level statement list, same
         /// shape as any other ListHandle over StmtHandle. Empty for an
         /// empty file, never for a parse failure (that's reported
         /// through Parser's diagnostics instead).
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
         /// ListHandle describing it. The only way a ListHandle is ever
         /// produced - callers never hand-construct one, so this class
         /// alone upholds "index/count is a valid range in the arena".
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

         /// Read-only view of the entries a ListHandle points into.
         /// Bounds are asserted, not clamped - out-of-range means
         /// something violated this class's invariant, and silent
         /// truncation would hide that.
         template<typename T>
         static const T* listData(const Vector<T>& arena, ast::ListHandle handle)
         {
            // count == 0 is valid regardless of index - a default
            // ListHandle{} (index == ~0u sentinel, count == 0) is what
            // an omitted optional list produces.
            if (handle.count == 0)
               return nullptr;

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
