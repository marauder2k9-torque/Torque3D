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
#ifndef _SHADERBLUEPRINT_H_
#define _SHADERBLUEPRINT_H_

#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif
#ifndef _GFXSHADER_H_
#include "gfx/gfxShader.h"
#endif
#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif

enum ShaderToken
{
   // Shader Stages, these need to be first for our 0-5 array
   Vertex,
   Pixel,
   Compute,
   Geometry,
   Domain,
   Hull,
   // Main Specifier used to verify file type.
   Blueprint,
   // Shader Include
   Include,

   // Shader Attribute Blocks
   VertexData,
   PixelData,
   ComputeData,
   GeometryData,
   DomainData,
   HullData,
   PixelOut,

   Void,
   // Const types
   // Scalar.
   Float,
   Int,
   UInt,
   Bool,
   // Float vector.
   Float2,
   Float3,
   Float4,

   // Int Vector.
   Int2,
   Int3,
   Int4,

   // UInt Vector.
   UInt2,
   UInt3,
   UInt4,

   // Matrices.
   Float2x2,
   Float3x3,
   Float3x4,
   Float4x3,
   Float4x4,

   // Samplers.
   Sampler2D,
   Sampler2DArray,
   Sampler3D,
   SamplerCube,
   SamplerCubeArray,

   // Brackets.
   OpenParenthese,
   CloseParenthese,
   OpenBrace,
   CloseBrace,
   OpenSquare,
   CloseSquare,

   // Conditionals.
   If,
   Else,
   For,
   While,
   Do,
   Break,
   True,
   False,
   Return,
   Continue,
   Discard,

   // Conditional symbols.
   LessEqual,
   LessThan,
   GreaterEqual,
   GreaterThan,
   EqualEqual,
   NotEqual,
   AndAnd,
   OrOr,

   // Binary symbols
   Plus,
   Minus,
   Times,
   Divide,
   Or,
   And,

   // iterators
   PlusPlus,
   MinusMinus,
   PlusEqual,
   MinusEqual,
   TimesEqual,
   DivideEqual,

   // Shader Data Specifiers.
   Uniform,
   StructBlock,

   // Modifiers.
   Const,
   Static,
   Inline,
   In,
   Out,
   InOut,

   // Variable Name.
   VariableName,

   // end out.
   EndOfFile,
};

enum ShaderDataBlockType
{
   First,
   Stage,
   DataBlock,
   VariableDeclaration,
   Struct,
   StructVar,
   Function,
   Argument
};

// the order of these are important as they become the
// layout binding for glsl, as defined in gfxGLShader
enum StructVarSemantic
{
   // application vertex attributes, integer values are important.
   Position = 0,
   Normal,
   Color,
   Tangent,
   TangentW,
   Binormal,
   TexCoord, // this + trailing number become layout location.
   // pixel shader data, the integer values here dont matter.
   SV_Position,
   SV_Target,
   SV_Depth,
   PositionT,
   PSize,
   BlendIndices,
   BlendWeight,
};

struct ShaderDataBlock;
struct ShaderStructVar;

struct ShaderVarType
{
   // type of this var, set to void, a var cant be void.
   ShaderToken type = ShaderToken::Void;
   bool isArray = false;

   // is this an array and its size is set from a macro?
   String size = String::EmptyString;

   // we dont need to convert values all apis use the
   // same grammar eg(0x for hex, e and decimal mean float/half/double)
   // if there are errors here, let the api compiler sort it out.
   // initial values can also be set by a macro.
   String initValue = String::EmptyString;

   // arrays are initialized differently in glsl as to hlsl
   // array should be written in the hlsl style, the generators
   // will automatically change this.
   Vector<String> arrayValues;
};

struct ShaderRoot
{
   ShaderDataBlockType blockType;
};

struct ShaderBlock : public ShaderRoot
{
   // first block of data for a shader.
   ShaderDataBlock* dataBlock;

   ShaderBlock() {
      blockType = ShaderDataBlockType::First;
      dataBlock = NULL;
   }
};

// base class for all shader data blocks, these can be single lines
// attributes, uniforms, declarations etc. basically everything.
struct ShaderDataBlock : public ShaderRoot
{
   ShaderDataBlock* nextBlock;

   ShaderDataBlock() {
      blockType = ShaderDataBlockType::DataBlock;
      nextBlock = NULL;
   }
};

struct ShaderVarDeclare : public ShaderDataBlock
{
   String name;
   bool uniform;
   ShaderVarType* type;
   ShaderVarDeclare* nextVarDeclare;

   ShaderVarDeclare()
   {
      blockType = ShaderDataBlockType::VariableDeclaration;
      name = String::EmptyString;
      uniform = false;
      type = NULL;
      nextVarDeclare = NULL;
   }
};

struct ShaderStruct : public ShaderDataBlock
{
   String name;
   ShaderStructVar* firstVar;

   ShaderStruct()
   {
      blockType = ShaderDataBlockType::Struct;
      name = String::EmptyString;
      firstVar = NULL;
   }
};

struct ShaderStructVar : public ShaderRoot
{
   String name;
   ShaderStructVar* nextVar;
   StructVarSemantic semantic;
   ShaderVarType* type;
   S32 semanticRegister;

   ShaderStructVar()
   {
      blockType = ShaderDataBlockType::StructVar;
      name = String::EmptyString;
      nextVar = NULL;
      semantic = StructVarSemantic::Position;
      type = new ShaderVarType();
      semanticRegister = -1;
   }
};

struct ShaderFunctionArgument : public ShaderRoot
{
   String name;
   ShaderVarType* type;
   ShaderToken modifier;
   ShaderFunctionArgument* nextArg;

   ShaderFunctionArgument()
   {
      blockType = ShaderDataBlockType::Argument;

      name = String::EmptyString;
      type = NULL;
      // just set to void as a negation.
      modifier = ShaderToken::Void;
      nextArg = NULL;
   }
};

struct ShaderFunction : public ShaderDataBlock
{
   String name;
   ShaderFunctionArgument* firstArgument;
   ShaderToken modifier;
   ShaderToken returnType;
   ShaderFunction()
   {
      blockType = ShaderDataBlockType::Function;

      name = String::EmptyString;
      firstArgument = NULL;
      // just set to void as a negation.
      modifier = ShaderToken::Void;
      returnType = ShaderToken::Void;
   }
};

struct ShaderStage : public ShaderDataBlock
{
   ShaderToken shaderStage;
   ShaderDataBlock* firstBlock;
   ShaderStage()
   {
      blockType = ShaderDataBlockType::Stage;

      shaderStage = ShaderToken::Vertex;
      firstBlock = NULL;
   }
};

class ShaderBlueprint : public SimObject
{
   typedef SimObject Parent;
protected:
   StringTableEntry mBlueprintFile;
   // we can only ever have a max of 6 of these.
   ShaderStruct* mBlueprintStructs[7];
   ShaderStage* mShaderStages[6];

   void _onFileChanged(const Torque::Path& path) {  }

public:
   ShaderBlueprint();
   virtual ~ShaderBlueprint();

   // SimObject
   virtual bool onAdd();
   virtual void onRemove();

   // ConsoleObject
   static void initPersistFields();
   DECLARE_CONOBJECT(ShaderBlueprint);

private:
   bool _reload();

   const char* mBuffer;
   const char* mBufferEnd;
   U32 mLine;
   bool mError;
   String mTokenIdentifier;

   //
   String mShaderName;

   // token vars
   ShaderToken mCurToken;
   U32 mCurTokenLine;

   // Check for types
   bool isWhiteSpace();
   bool isComment();
   bool isOpOrSymbol(char c);
   bool isNumber(char c);
   bool isTokenInRange(U32 tokenStart, U32 tokenEnd);

   // Token functions
   void nextToken();

   // get token data, these are used to make sure the token is what is expected,
   // and then moves to the next token.
   bool getToken(U32 tokenId, bool force = false);
   bool getToken(const char* tokenName, bool force = false);
   bool getTokenRange(U32 tokenStart, U32 tokenEnd, ShaderToken& outToken, bool force = false);
   bool getVariableName(String& outVar, bool force = false); // get variable name and move to next token.

   // parse functions
   bool initParser(const char* buffer, U32 buffLen);

   bool parseVariableDefinition(String& name, ShaderVarType*& type);
   bool parseStructVariable(ShaderStructVar*& var, bool semantic = false);
   bool parseShaderStage(ShaderStage*&);

   // main parse function that kicks it all off.
   bool parseShaderBlueprint();
};

#endif
