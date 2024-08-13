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

#define YYSTYPE TSHADE_STYPE

class SimObject;
class SimGroup;

//-----------------------------------------------
// ENUMS 
//-----------------------------------------------

enum ShaderStageType {
   tSTAGE_VERTEX,
   tSTAGE_PIXEL,
   tSTAGE_GEOMETRY,
   tSTAGE_COMPUTE,
   tSTAGE_COUNT
};

enum ShaderVarType {
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
   tShadeNode();
   virtual ~tShadeNode();
};

struct tStructMemberNode {
   String name;
   ShaderVarType type;
   ShaderSemanticType semantic;

   tStructMemberNode(const String& memberName, ShaderVarType memberType, ShaderSemanticType memberSemantic)
      : name(memberName), type(memberType), semantic(memberSemantic) {}
};

struct tStructNode {
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

struct tVarDeclNode : public tShadeNode {
   String name;
   ShaderVarType type;
   tShadeNode* initExpr;

   tVarDeclNode(const String& inName, ShaderVarType vartype, tShadeNode* init = nullptr)
      : name(inName), type(vartype), initExpr(init) {}
};

struct tStageNode : tShadeNode
{
   ShaderStageType stage;
   tShadeNode* rootNode;

   tStageNode(ShaderStageType stage, tShadeNode* node) {
      rootNode = node;
   }
};


struct tShadeAst
{
   String shaderName;

   Vector<tStructNode*> mDataStructs;

   tStageNode* mVertStage;
   tStageNode* mPixStage;
   tStageNode* mGeoStage;
   tStageNode* mComputeStage;

   tShadeAst(const String& name)
      : shaderName(name), mVertStage(nullptr), mPixStage(nullptr), mGeoStage(nullptr),
      mComputeStage(nullptr) {}

   ~tShadeAst() {
      for (auto dataStruct : mDataStructs)
         delete dataStruct;

      delete mVertStage;
      delete mPixStage;
      delete mGeoStage;
      delete mComputeStage;
   }

   void addStageNode(ShaderStageType stageType, tStageNode* stageNode)
   {
      switch (stageType)
      {
      case tSTAGE_VERTEX:
         mVertStage = stageNode;
         break;
      case tSTAGE_PIXEL:
         mPixStage = stageNode;
         break;
      case tSTAGE_GEOMETRY:
         mGeoStage = stageNode;
         break;
      case tSTAGE_COMPUTE:
         mComputeStage = stageNode;
         break;
      default:
         break;
      }
   }

   void addStruct(tStructNode* structNode) {
      mDataStructs.push_back(structNode);
   }

};

//-----------------------------------------------
// AST NODES END
//-----------------------------------------------

#endif // !_TSHADEAST_H_

