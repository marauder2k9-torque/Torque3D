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
#pragma once

#ifndef _TSHADEAST_H_
#define _TSHADEAST_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/types.h"
#endif

#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif // !#ifndef _TDICTIONARY_H_

#define YYSTYPE TSHADE_STYPE

class SimObject;
class SimGroup;

//-----------------------------------------------
// ENUMS 
//-----------------------------------------------

enum ShaderStageType {
   tSTAGE_GLOBAL, // for data that will be copied across all stages.
   tSTAGE_VERTEX,
   tSTAGE_PIXEL,
   tSTAGE_GEOMETRY,
   tSTAGE_COMPUTE,
   tSTAGE_COUNT
};

enum ShaderVarType {
   tTYPE_VOID,
   tTYPE_STRUCT,
   tTYPE_FLOAT,
   tTYPE_INT,
   tTYPE_UINT,
   tTYPE_BOOL,
   tTYPE_FLOAT2,
   tTYPE_FLOAT3,
   tTYPE_FLOAT4,
   tTYPE_INT2,
   tTYPE_INT3,
   tTYPE_INT4,
   tTYPE_UINT2,
   tTYPE_UINT3,
   tTYPE_UINT4,
   tTYPE_BOOL2,
   tTYPE_BOOL3,
   tTYPE_BOOL4,
   tTYPE_MAT33,
   tTYPE_MAT34,
   tTYPE_MAT43,
   tTYPE_MAT44,
   tTYPE_COUNT,
};

enum ShaderSemanticType {
   // vertex semantics
   SEMANTIC_VERT_POSITION,
   SEMANTIC_VERT_COLOR,
   SEMANTIC_VERT_NORMAL,
   SEMANTIC_VERT_BINORMAL,
   SEMANTIC_VERT_PSIZE,
   SEMANTIC_VERT_TANGENT,
   SEMANTIC_VERT_TEXCOORD,
   SEMANTIC_VERT_TESSFACTOR,
   // pixel shader semantics
   SEMANTIC_PIXIN_COLOR,
   SEMANTIC_PIXIN_TEXCOORD,
   SEMANTIC_PIXIN_ISFRONTFACE,
   SEMANTIC_PIXIN_POSITION,
   SEMANTIC_PIXOUT_DEPTH,
   SEMANTIC_PIXOUT_TARGET,
   SEMANTIC_COUNT
};

enum ParamModifier{
   PARAM_MOD_NONE,
   PARAM_MOD_IN,
   PARAM_MOD_OUT,
   PARAM_MOD_INOUT,
   PARAM_MOD_COUNT
};

//-----------------------------------------------
// ENUMS END
//-----------------------------------------------

//-----------------------------------------------
// AST NODES
//-----------------------------------------------

struct tShadeNode
{
public:
   tShadeNode() {}
   virtual ~tShadeNode() {}
};

struct tStatementListNode : public tShadeNode {
   Vector<tShadeNode*> statements;

   tStatementListNode() {}

   ~tStatementListNode() {
      for (auto stmt : statements)
         delete stmt;
   }

   void addStatement(tShadeNode* stmt) {
      statements.push_back(stmt);
   }
};
struct tStructMemberNode : public tShadeNode {
   String name;
   ShaderVarType type;
   ShaderSemanticType semantic;

   tStructMemberNode(const String& memberName, ShaderVarType memberType, ShaderSemanticType memberSemantic)
      : name(memberName), type(memberType), semantic(memberSemantic) {}
};

struct tStructNode : public tShadeNode {
   String structName;
   Vector<tStructMemberNode*> members;

   tStructNode(const String& name) : structName(name) {}

   ~tStructNode() {
      for (auto member : members)
         delete member;
   }

   void addMember(tStructMemberNode* member)
   {
      members.push_back(member);
   }
};

struct tExpressionListNode : public tShadeNode {
   Vector<tShadeNode*> mExpressions;

   tExpressionListNode() {}

   void addExpression(tShadeNode* expr) {
      mExpressions.push_back(expr);
   }

   ~tExpressionListNode() {
      for (auto expr : mExpressions) {
         delete expr;  // Assuming expressions are dynamically allocated
      }
   }
};

struct tFunctionParamNode : public tShadeNode {
   String name;
   ShaderVarType type;
   ParamModifier modifier;

   tFunctionParamNode(const String& paramName, ShaderVarType paramType, ParamModifier paramModifier)
      : name(paramName), type(paramType), modifier(paramModifier) {}
};

struct tFunctionParamListNode : public tShadeNode {
   Vector<tFunctionParamNode*> params;

   void addParam(tFunctionParamNode* param) {
      params.push_back(param);
   }

   ~tFunctionParamListNode() {
      for (auto param : params)
         delete param;
   }
};

struct tFunctionNode : public tShadeNode {
   String name;
   ShaderVarType returnType;
   tFunctionNode(const String& funcName, ShaderVarType returnType)
      :name(funcName), returnType(returnType) {}

   ~tFunctionNode() {}
};

struct tFunctionDefNode : public tFunctionNode {
   tFunctionParamListNode* paramList;
   tStatementListNode* body;

   tFunctionDefNode(const String& funcName, ShaderVarType returnType, tFunctionParamListNode* params, tStatementListNode* funcBody)
      : tFunctionNode(funcName, returnType), paramList(params), body(funcBody) {}

   ~tFunctionDefNode() {
      delete paramList;
      if (body)
         delete body;
   }
};

struct tFunctionDeclNode : public tFunctionNode {
   tFunctionParamListNode* paramList;

   tFunctionDeclNode(const String& funcName, ShaderVarType returnType, tFunctionParamListNode* params)
      : tFunctionNode(funcName, returnType), paramList(params) {}

   ~tFunctionDeclNode() {
      delete paramList;
   }
};

struct tFunctionRefNode : public tShadeNode {
   tFunctionNode* funcDecl;
   tExpressionListNode* expr;

   tFunctionRefNode(tFunctionNode* decl, tExpressionListNode* exprList)
      : funcDecl(decl), expr(exprList) {}
};

struct tTypeRefNode : public tShadeNode {
   ShaderVarType returnType;
   tExpressionListNode* expr;

   tTypeRefNode(ShaderVarType type, tExpressionListNode* exprList)
      : returnType(type), expr(exprList) {}
};

struct tVarDeclNode : public tShadeNode {
   String name;
   ShaderVarType type;
   tShadeNode* initExpr;
   tShadeNode* arraySize; // arrays can be set by an expression/macros etc
   bool isStruct;
   bool isUniform;

   tVarDeclNode(
      const String& inName,
      ShaderVarType vartype,
      tShadeNode* init = nullptr,
      tShadeNode* initArray = nullptr,
      bool inStruct = false)
      : name(inName),
      type(vartype),
      initExpr(init),
      arraySize(initArray),
      isStruct(inStruct),
      isUniform(false)
   {}

   ~tVarDeclNode() {
      if(initExpr)
         delete initExpr;

      if (arraySize)
         delete arraySize;
   }
};

struct tVarRefNode : public tShadeNode {
   tVarDeclNode* varDecl;
   String swizzle;

   tVarRefNode(tVarDeclNode* decl, const String& swiz = String::EmptyString)
      : varDecl(decl), swizzle(swiz) {}
};

struct tBinaryOpNode : public tShadeNode {
   String op;
   tShadeNode* left;
   tShadeNode* right;

   tBinaryOpNode(const String& inOp, tShadeNode* inLeft, tShadeNode* inRight)
      : op(inOp), left(inLeft), right(inRight) {}

   ~tBinaryOpNode() {
      delete left;
      delete right;
   }

};

struct tUnaryOpNode : public tShadeNode {
   String op;
   tShadeNode* expr;

   tUnaryOpNode(const String& inOp, tShadeNode* inExpr)
   : op(inOp), expr(inExpr) {}

   ~tUnaryOpNode() {
      if(expr)
         delete expr;
   }
};

struct tIntLiteralNode : public tShadeNode {
   S32 value;

   tIntLiteralNode(S32 val) : value(val) {}
};

struct tFloatLiteralNode : public tShadeNode {
   F64 value;

   tFloatLiteralNode(F64 val) : value(val) {}
};

struct tStageNode : tShadeNode
{
   ShaderStageType stage;
   tShadeNode* rootNode;

   tStageNode(ShaderStageType inStage, tShadeNode* node)
   : stage(inStage) {

      rootNode = node;
   }

   ~tStageNode() {
      if (rootNode)
         delete rootNode;
   }
};

struct tIfNode : public tShadeNode {
   tShadeNode* expr;
   tStatementListNode* trueBranch;
   tStatementListNode* elseBranch;

   tIfNode(tShadeNode* inExpr, tStatementListNode* inTrue, tStatementListNode* inElse = nullptr)
      : expr(inExpr), trueBranch(inTrue), elseBranch(inElse) {}

   ~tIfNode() {
      delete expr;
      delete trueBranch;

      if(elseBranch)
         delete elseBranch;
   }
};

struct tWhileNode : public tShadeNode {
   tShadeNode* expr;
   tStatementListNode* trueBranch;
   bool isDo;

   tWhileNode(tShadeNode* inExpr, tStatementListNode* inTrue, bool doLoop = false)
      : expr(inExpr), trueBranch(inTrue), isDo(doLoop) {}

   ~tWhileNode() {
      delete expr;
      delete trueBranch;
   }
};

struct tContinueNode : public tShadeNode {
   tContinueNode() {}
};

struct tBreakNode : public tShadeNode {
   tBreakNode() {}
};


struct tReturnNode : public tShadeNode {
   tShadeNode* expr;

   tReturnNode(tShadeNode* inExpr = nullptr)
      : expr(inExpr){}

   ~tReturnNode() {
      delete expr;
   }
};

struct tShadeAst
{
   // map string to var type and shaderStage.
   typedef Map<String, ShaderStageType> StructMap;
   typedef Map<String, tVarDeclNode*> VarMap;
   typedef Map<String, tFunctionNode*> FuncMap;

   StructMap mStructMap;
   VarMap mVarMap;
   FuncMap mFuncMap;

   String shaderName;

   Vector<tShadeNode*> mGlobalVars; // global vars, macros
   Vector<tStructNode*> mDataStructs; // data structs (vert pix connect etc)

   tStageNode* mVertStage;
   tStageNode* mPixStage;
   tStageNode* mGeoStage;
   tStageNode* mComputeStage;

   tShadeAst(const String& name)
      : shaderName(name), mVertStage(nullptr), mPixStage(nullptr), mGeoStage(nullptr),
      mComputeStage(nullptr) {}

   ~tShadeAst() {
      for (auto var : mGlobalVars)
         delete var;

      for (auto dataStruct : mDataStructs)
         delete dataStruct;

      if(mVertStage)
         delete mVertStage;

      if(mPixStage)
         delete mPixStage;

      if(mGeoStage)
         delete mGeoStage;

      if(mComputeStage)
         delete mComputeStage;

      mVarMap.clear();
      mStructMap.clear();
      mFuncMap.clear();
   }

   void addStruct(tStructNode* structNode) {
      mDataStructs.push_back(structNode);

      mStructMap[structNode->structName] = ShaderStageType::tSTAGE_GLOBAL;
   }

   void addStruct(tStructNode* structNode, ShaderStageType stage) {
      mDataStructs.push_back(structNode);

      mStructMap[structNode->structName] = stage;
   }

   void addfunction(tFunctionNode* func) {
      if (findFunction(func->name))
         return;

      mFuncMap[func->name] = func;
   }

   void addVarDecl(tVarDeclNode* varDecl) {
      mVarMap[varDecl->name] = varDecl;
   }

   tVarDeclNode* findVar(const String& name) {
      auto var = mVarMap.find(name);
      return var != mVarMap.end() ? var->value : nullptr;
   }

   tFunctionNode* findFunction(const String& name) {
      auto func = mFuncMap.find(name);
      return func != mFuncMap.end() ? func->value : nullptr;
   }

   bool isStruct(const String& name) const {
      auto strct = mStructMap.find(name);
      return (strct != mStructMap.end());
   }

};

//-----------------------------------------------
// AST NODES END
//-----------------------------------------------

#endif // !_TSHADEAST_H_

