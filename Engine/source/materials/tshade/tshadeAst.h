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
#define YYLTYPE TSHADE_LTYPE

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
   tTYPE_SAMPLER2D,
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
   SEMANTIC_NONE,
   SEMANTIC_SV_POSITION,
   SEMANTIC_POSITION,
   SEMANTIC_COLOR,
   SEMANTIC_SV_DEPTH,
   SEMANTIC_SV_TARGET,
   SEMANTIC_TEXCOORD,
   SEMANTIC_FOG,
   SEMANTIC_ISFRONTFACE,
   SEMANTIC_TESSFACTOR,
   SEMANTIC_BINORMAL,
   SEMANTIC_BLENDWEIGHT,
   SEMANTIC_BLENDINDICES,
   SEMANTIC_NORMAL,
   SEMANTIC_TANGENT,
   SEMANTIC_POSITIONT,
   SEMANTIC_PSIZE,
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
struct tShadeAst;

struct tShadeNode
{
public:
   tShadeNode() {}
   virtual ~tShadeNode() {}
   
   virtual void print(tShadeAst* ast, String& out) = 0;
};

struct tStatementListNode : public tShadeNode {
   Vector<tShadeNode*> statements;

   tStatementListNode() {}

   ~tStatementListNode() {
      for (auto stmt : statements)
         delete stmt;
   }
   
   void print(tShadeAst *ast, String &out) override {
      for(auto statment : statements){
         statement->print(ast, out);
         out += ";\n\n";
      }
   }
   

   void addStatement(tShadeNode* stmt) {
      statements.push_back(stmt);
   }
};

struct tStructNode : public tShadeNode {
   String structName;
   tStatementListNode* members;

   tStructNode(const String& name, tStatementListNode* memeberList)
      : structName(name), members(memeberList) {}

   ~tStructNode() {
      delete members;
   }
   
   void print(tShadeAst *ast, String &out) override {
      out += "struct " + structName + " {\n";
      members->print(ast, out);
      out += "\n};";
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
   
   void print(tShadeAst *ast, String &out) override {
      for(auto expression : mExpressions)
      {
         expression->print(ast, out);
      }
   }
   
};

struct tFunctionParamNode : public tShadeNode {
   String name;
   ShaderVarType type;
   ParamModifier modifier;
   String structName;
   tFunctionParamNode(const String& paramName, ShaderVarType paramType, ParamModifier paramModifier)
      : name(paramName), type(paramType), modifier(paramModifier), structName(String::EmptyString) {}
   
   void print(tShadeAst *ast, String &out) override {}
   
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
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tFunctionNode : public tShadeNode {
   String name;
   ShaderVarType returnType;
   String structName;
   tFunctionNode(const String& funcName, ShaderVarType returnType)
      :name(funcName), returnType(returnType), structName(String::EmptyString) {}

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
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tFunctionDeclNode : public tFunctionNode {
   tFunctionParamListNode* paramList;

   tFunctionDeclNode(const String& funcName, ShaderVarType returnType, tFunctionParamListNode* params)
      : tFunctionNode(funcName, returnType), paramList(params) {}

   ~tFunctionDeclNode() {
      delete paramList;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tFunctionRefNode : public tShadeNode {
   tFunctionNode* funcDecl;
   tExpressionListNode* expr;

   tFunctionRefNode(tFunctionNode* decl, tExpressionListNode* exprList)
      : funcDecl(decl), expr(exprList) {}
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tTypeRefNode : public tShadeNode {
   ShaderVarType returnType;
   tExpressionListNode* expr;

   tTypeRefNode(ShaderVarType type, tExpressionListNode* exprList)
      : returnType(type), expr(exprList) {}
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tAccessNode : public tShadeNode {
   String accessString;

   tAccessNode(const String& inAccess): accessString(inAccess){}
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tVarDeclNode : public tShadeNode {
   String name;
   String structName;
   ShaderVarType type;
   tShadeNode* initExpr;
   tShadeNode* arraySize; // arrays can be set by an expression/macros etc
   bool isStruct;
   bool isUniform;
   bool isStatic;
   bool isConst;

   tVarDeclNode(
      const String& inName,
      ShaderVarType vartype,
      tShadeNode* init = nullptr,
      tShadeNode* initArray = nullptr,
      bool inStruct = false)
      : name(inName),
      structName(String::EmptyString),
      type(vartype),
      initExpr(init),
      arraySize(initArray),
      isStruct(inStruct),
      isUniform(false),
      isStatic(false),
      isConst(false)
   {}

   ~tVarDeclNode() {
      if(initExpr)
         delete initExpr;

      if (arraySize)
         delete arraySize;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tStructMemberNode : public tShadeNode {
   tVarDeclNode* varDecl;
   ShaderSemanticType semantic;
   U32 semNumber;

   tStructMemberNode(tVarDeclNode* varDecl, ShaderSemanticType memberSemantic = ShaderSemanticType::SEMANTIC_NONE, U32 semNum = 0)
      : varDecl(varDecl), semantic(memberSemantic), semNumber(semNum) {}
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tVarRefNode : public tShadeNode {
   tVarDeclNode* varDecl;
   tVarDeclNode* memberVar;

   tVarRefNode(tVarDeclNode* decl, tVarDeclNode* memberDecl = nullptr)
      : varDecl(decl), memberVar(memberDecl) {}
   
   void print(tShadeAst *ast, String &out) override {}
   
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
   
   void print(tShadeAst *ast, String &out) override {}

};

struct tUnaryOpNode : public tShadeNode {
   String op;
   tShadeNode* expr;
   bool prefix;

   tUnaryOpNode(const String& inOp, tShadeNode* inExpr, bool isPrefix = true)
   : op(inOp), expr(inExpr), prefix(isPrefix) {}

   ~tUnaryOpNode() {
      if(expr)
         delete expr;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tIntLiteralNode : public tShadeNode {
   S32 value;

   tIntLiteralNode(S32 val) : value(val) {}
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tFloatLiteralNode : public tShadeNode {
   F64 value;

   tFloatLiteralNode(F64 val) : value(val) {}
   
   void print(tShadeAst *ast, String &out) override {}
   
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
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tSampleNode : public tShadeNode {
   tShadeNode* left;
   tShadeNode* right;

   tSampleNode(tShadeNode* inLeft, tShadeNode* inRight)
      : left(inLeft), right(inRight) {}

   ~tSampleNode()
   {
      delete left;
      delete right;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tMulNode : public tShadeNode {
   tShadeNode* left;
   tShadeNode* right;

   tMulNode(tShadeNode* inLeft, tShadeNode* inRight)
      : left(inLeft), right(inRight) {}

   ~tMulNode()
   {
      delete left;
      delete right;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tLerpNode : public tShadeNode {
   tShadeNode* a;
   tShadeNode* b;
   tShadeNode* c;

   tLerpNode(tShadeNode* a, tShadeNode* b, tShadeNode* c)
      : a(a), b(b), c(c) {}

   ~tLerpNode()
   {
      delete a;
      delete b;
      delete c;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tFracNode : public tShadeNode {
   tShadeNode* val;

   tFracNode(tShadeNode* a)
      : val(a){}

   ~tFracNode()
   {
      delete val;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
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
   
   void print(tShadeAst *ast, String &out) override {}
   
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
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tSwitchNode : public tShadeNode {
   tShadeNode* expr;
   tStatementListNode* trueBranch;
   bool isDo;

   tSwitchNode(tShadeNode* inExpr, tStatementListNode* inTrue, bool doLoop = false)
      : expr(inExpr), trueBranch(inTrue), isDo(doLoop) {}

   ~tSwitchNode() {
      delete expr;
      delete trueBranch;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tCaseNode : public tShadeNode {
   tShadeNode* expr;
   tStatementListNode* trueBranch;
   bool isDefault;

   tCaseNode(tShadeNode* inExpr, tStatementListNode* inTrue, bool def = false)
      : expr(inExpr), trueBranch(inTrue), isDefault(def) {}

   ~tCaseNode() {
      delete expr;
      delete trueBranch;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tContinueNode : public tShadeNode {
   tContinueNode() {}
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tBreakNode : public tShadeNode {
   tBreakNode() {}
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tDiscardNode : public tShadeNode {
   tDiscardNode() {}
   
   void print(tShadeAst *ast, String &out) override {}
   
};


struct tReturnNode : public tShadeNode {
   tShadeNode* expr;

   tReturnNode(tShadeNode* inExpr = nullptr)
      : expr(inExpr){}

   ~tReturnNode() {
      delete expr;
   }
   
   void print(tShadeAst *ast, String &out) override {}
   
};

struct tShadeAst
{
   // map string to var type and shaderStage.
   typedef Map<String, tStructNode*> StructMap;
   typedef Map<String, tVarDeclNode*> VarMap;
   typedef Map<String, tFunctionNode*> FuncMap;

   StructMap mStructMap;
   VarMap mLocalVarMap;
   VarMap mGlobalVarMap;
   FuncMap mFuncMap;

   ShaderStageType currentStage;

   String shaderName;

   Vector<tStructNode*> mDataStructs; // global data structs (vert pix connect etc)

   tStageNode* mVertStage;
   tStageNode* mPixStage;
   tStageNode* mGeoStage;
   tStageNode* mComputeStage;

   tShadeAst()
      : shaderName(String::EmptyString), mVertStage(nullptr), mPixStage(nullptr), mGeoStage(nullptr),
      mComputeStage(nullptr), currentStage(ShaderStageType::tSTAGE_GLOBAL) {}

   ~tShadeAst() {
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

      mLocalVarMap.clear();
      mGlobalVarMap.clear();
      mStructMap.clear();
      mFuncMap.clear();
   }

   void addDataStruct(tStructNode* structNode) {
      mDataStructs.push_back(structNode);
      mStructMap[structNode->structName] = structNode;
   }

   void addStruct(tStructNode* structNode, ShaderStageType stage) {
      mStructMap[structNode->structName] = structNode;
   }

   void addfunction(tFunctionNode* func) {
      if (findFunction(func->name))
         return;

      mFuncMap[func->name] = func;
   }

   void addVarDecl(tVarDeclNode* varDecl) {
      if(currentStage == ShaderStageType::tSTAGE_GLOBAL)
         mGlobalVarMap[varDecl->name] = varDecl;
      else
         mLocalVarMap[varDecl->name] = varDecl;
   }

   void clearVarDecls() {
      mLocalVarMap.clear();
   }

   tVarDeclNode* findVar(const String& name) {
      auto var = mLocalVarMap.find(name);
      if (var != mLocalVarMap.end())
         return var->value;
      else
      {
         // not in local, check global.
         var = mGlobalVarMap.find(name);
         return var != mGlobalVarMap.end() ? var->value : nullptr;
      }
   }

   tFunctionNode* findFunction(const String& name) {
      auto func = mFuncMap.find(name);
      return func != mFuncMap.end() ? func->value : nullptr;
   }

   tStructNode* findStruct(const String& name) {
      auto strct = mStructMap.find(name);
      return strct != mStructMap.end() ? strct->value : nullptr;
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

