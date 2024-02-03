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
#include "platform/platform.h"
#include "materials/shaderBlueprint.h"

#include "core/volume.h"

#include <unordered_map>
#include <string>

// used to create tokens from input buffer.
// note compiler complains about anything other than std::string
// const char* does not work as no comparitor.
const std::unordered_map<std::string, ShaderToken> WordMap
{
   {"Blueprint",        ShaderToken::Blueprint },
   // is this stage supposed to be an included in multiple shaders?
   {"ShaderInclude",    ShaderToken::Include },
   //Shader stages.
   {"VertexShader",     ShaderToken::Vertex },
   {"PixelShader",      ShaderToken::Pixel },
   {"ComputeShader",    ShaderToken::Compute },
   {"GeometryShader",   ShaderToken::Geometry },
   {"DomainShader",     ShaderToken::Domain },
   {"HullShader",       ShaderToken::Hull },
   
   //Shader Attribute blocks.
   {"VertexData",    ShaderToken::VertexData },
   {"PixelData",     ShaderToken::PixelData },
   {"ComputeData",   ShaderToken::ComputeData },
   {"GeometryData",  ShaderToken::GeometryData },
   {"DomainData",    ShaderToken::DomainData },
   {"HullData",      ShaderToken::HullData },

   {"void",    ShaderToken::Void },
   // Scalar
   {"float",   ShaderToken::Float},
   {"int",     ShaderToken::Int},
   {"uint",    ShaderToken::UInt},
   {"bool",    ShaderToken::Bool},

   // HLSL Float vector.
   {"float2",  ShaderToken::Float2},
   {"float3",  ShaderToken::Float3},
   {"float4",  ShaderToken::Float4},

   // GLSL Float vector.
   {"vec2",    ShaderToken::Float2},
   {"vec3",    ShaderToken::Float3},
   {"vec4",    ShaderToken::Float4},

   // HLSL Int vector.
   {"int2",    ShaderToken::Int2},
   {"int3",    ShaderToken::Int3},
   {"int4",    ShaderToken::Int4},

   // GLSL Int vector.
   {"ivec2",   ShaderToken::Int2},
   {"ivec3",   ShaderToken::Int3},
   {"ivec4",   ShaderToken::Int4},

   // HLSL UInt vector.
   {"uint2",   ShaderToken::UInt2},
   {"uint3",   ShaderToken::UInt3},
   {"uint4",   ShaderToken::UInt4},

   // GLSL UInt vector.
   {"uvec2",   ShaderToken::UInt2},
   {"uvec3",   ShaderToken::UInt3},
   {"uvec4",   ShaderToken::UInt4},

   // HLSL Matrices.
   {"float2x2", ShaderToken::Float2x2 },
   {"float3x3", ShaderToken::Float3x3 },
   {"float3x4", ShaderToken::Float3x4 },
   {"float4x3", ShaderToken::Float4x3 },
   {"float4x4", ShaderToken::Float4x4 },

   // GLSL Matrices.
   {"mat2x2",  ShaderToken::Float2x2 },
   {"mat2",    ShaderToken::Float2x2 },
   {"mat3x3",  ShaderToken::Float3x3 },
   {"mat3",    ShaderToken::Float3x3 },
   {"mat3x4",  ShaderToken::Float3x4 },
   {"mat4x3",  ShaderToken::Float4x3 },
   {"mat4x4",  ShaderToken::Float4x4 },
   {"mat4",    ShaderToken::Float4x4 },

   // We use glsl type samplers.
   {"sampler2D",        ShaderToken::Sampler2D },
   {"sampler2DArray",   ShaderToken::Sampler2DArray },
   {"sampler3D",        ShaderToken::Sampler3D },
   {"samplerCube",      ShaderToken::SamplerCube },
   {"samplerCubeArray", ShaderToken::SamplerCubeArray },

   // conditionals/loops/statements.
   {"if",      ShaderToken::If},
   {"else",    ShaderToken::Else},
   {"for",     ShaderToken::For},
   {"while",   ShaderToken::While},
   {"do",      ShaderToken::Do},
   {"break",   ShaderToken::Break},
   {"true",    ShaderToken::True},
   {"false",   ShaderToken::False},
   {"return",  ShaderToken::Return},
   {"contine", ShaderToken::Continue},
   {"discard", ShaderToken::Discard},

   // Shader data specifiers.
   {"uniform", ShaderToken::Uniform },
   {"struct",  ShaderToken::StructBlock },

   // Modifiers
   {"const",   ShaderToken::Const },
   {"static",  ShaderToken::Static },
   {"inline",  ShaderToken::Inline },
   {"in",      ShaderToken::In },
   {"out",     ShaderToken::Out },
   {"inout",   ShaderToken::InOut }
};

const std::unordered_map<std::string, StructVarSemantic> SemanticMap
{
   {"POSITION",      StructVarSemantic::Position},
   {"POSITIONT",     StructVarSemantic::PositionT},
   {"NORMAL",        StructVarSemantic::Normal},
   {"BINORMAL",      StructVarSemantic::Binormal},
   {"COLOR",         StructVarSemantic::Color},
   {"TANGENT",       StructVarSemantic::Tangent},
   {"TESCOORD",      StructVarSemantic::TexCoord},
   {"PSIZE",         StructVarSemantic::PSize},
   {"BLENDINDICES",  StructVarSemantic::BlendIndices},
   {"BLENDWEIGHT",   StructVarSemantic::BlendWeight},
   {"SV_Position",   StructVarSemantic::SV_Position},
   {"SV_Target",     StructVarSemantic::SV_Target},
   {"SV_Depth",      StructVarSemantic::SV_Depth},
};

// used to identify tokens, mostly for error messages when expected.
const std::unordered_map<ShaderToken, const char*> ExpectedTokenError
{
   {ShaderToken::Blueprint,   "Blueprint"},
   {ShaderToken::VariableName,"Variable Declaration"},

};

IMPLEMENT_CONOBJECT(ShaderBlueprint);

ConsoleDocClass(ShaderBlueprint,
   "@brief Special type of data block that stores information for a shader blueprint.\n\n"

   "@tsexample\n"
   "singleton ShaderBlueprint( DiffuseShader )\n"
   "{\n"
   "	BlueprintFile = $Core::CommonShaderPath @ \"/diffuse.tlsl\";\n"
   "};\n"
   "@endtsexample\n\n"

   "@ingroup Shaders\n");

bool ShaderBlueprint::isOpOrSymbol(char c)
{
   switch (c)
   {
   case ';':case ':':case '.':case ',':
   case '-':case '/':case '*':case '+':case '=':
   case '!':case '?':case '|':case '&':case '~':case '@':case '<':case '>':case '^':
   case '{':case '}':case '[':case ']':case '(':case ')':
   case '"':case'\'':
      return true;
   }

   return false;
}

bool ShaderBlueprint::isNumber(char c)
{
   switch (c)
   {
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      return true;
   }

   return false;
}

bool ShaderBlueprint::isWhiteSpace()
{
   bool ret = false;
   while (mBuffer < mBufferEnd && dIsspace(mBuffer[0]))
   {
      ret = true;
      if (mBuffer[0] == '\n' || mBuffer[0] == '\r')
         mLine++;

      mBuffer++;
   }

   return ret;
}

bool ShaderBlueprint::isComment()
{
   bool ret = false;

   if (mBuffer[0] == '/')
   {
      if (mBuffer[1] == '/')
      {
         ret = true;

         mBuffer += 2;
         while (mBuffer < mBufferEnd)
         {
            if (*(mBuffer++) == '\n')
            {
               mLine++;
               break;
            }
         }
      }
   }
   else if (mBuffer[1] == '*')
   {
      ret = true;
      mBuffer += 2;
      while (mBuffer < mBufferEnd)
      {
         if (mBuffer[0] == '\n')
         {
            mLine++;
         }

         if (mBuffer[0] == '*' && mBuffer[1] == '/')
         {
            break;
         }

         if (mBuffer < mBufferEnd)
            mBuffer += 2;
      }
   }

   return ret;
}

bool ShaderBlueprint::getToken(U32 tokenId, bool force)
{
   if (mCurToken == tokenId)
   {
      nextToken();
      return true;
   }

   // only need to check no match when forced.
   if (force && mCurToken != tokenId)
   {
      mError = true;

      // if we find a type in the map, print the error with the expected.
      auto MapFind = ExpectedTokenError.find((ShaderToken)tokenId);
      if (MapFind != ExpectedTokenError.end())
      {
         // always report line from the blueprint file rather than the current shader stage.
         Con::printf("ShaderBlueprint Error - Expected: '%s', could not be found on line: %d", MapFind->second, mLine);
      }
      
      return false;
   }

   return false;
}

bool ShaderBlueprint::getToken(const char* tokenName, bool force)
{
   if (dStrcmp(tokenName, mTokenIdentifier) == 0)
   {
      nextToken();
      return true;
   }

   // only need to check no match when forced.
   if (force && dStrcmp(tokenName, mTokenIdentifier) != 0)
   {
      mError = true;
      // always report line from the blueprint file rather than the current shader stage.
      Con::printf("ShaderBlueprint Error - Expected: '%s', but was '%s', on line: %d", tokenName, mTokenIdentifier.c_str(), mLine);
      return false;
   }

   return false;
}

bool ShaderBlueprint::isTokenInRange(U32 tokenStart, U32 tokenEnd)
{
   if (mCurToken >= tokenStart || mCurToken <= tokenEnd)
   {
      return true;
   }

   return false;
}

bool ShaderBlueprint::getTokenRange(U32 tokenStart, U32 tokenEnd, ShaderToken& outToken, bool force)
{
   bool success = false;
   if (mCurToken >= tokenStart || mCurToken <= tokenEnd)
   {
      outToken = (ShaderToken)mCurToken;
      success = true;
      nextToken();
   }

   if (force && success == false)
   {
      mError = true;
      // always report line from the blueprint file rather than the current shader stage.
      Con::printf("ShaderBlueprint Error - Unexpected: '%s', on line: %d", mTokenIdentifier.c_str(), mLine);
      return success;
   }

   return success;
}

bool ShaderBlueprint::getVariableName(String& outVar, bool force)
{
   if (mCurToken != ShaderToken::VariableName)
   {
      if (force)
      {
         mError = true;
         Con::printf("ShaderBlueprint Error - Variable expected.");
      }

      return false;
   }
   else
   {
      outVar = mTokenIdentifier;
      nextToken();
      return true;
   }
}

ShaderBlueprint::ShaderBlueprint()
{
   mLine = 1;
   mCurTokenLine = 0;
   mTokenIdentifier = String::EmptyString;
   mShaderName = String::EmptyString;
   mCurToken = ShaderToken::EndOfFile;
   mError = false;
   mBlueprintFile = String::EmptyString;

   for (U32 i = 0; i < 6; i++)
   {
      mBlueprintStructs[i] = NULL;
      mShaderStages[i] = NULL;
   };
}

ShaderBlueprint::~ShaderBlueprint()
{
}

void ShaderBlueprint::initPersistFields()
{
   docsURL;

   addField("BlueprintFile", TypeStringFilename, Offset(mBlueprintFile, ShaderBlueprint),
      "@brief %Path to the blueprint file.\n\n");

   Parent::initPersistFields();

}

bool ShaderBlueprint::onAdd()
{
   if (!Parent::onAdd())
      return false;

   void* data = NULL;
   U32 dataSize = 0;

   Torque::FS::ReadFile(mBlueprintFile, data, dataSize, true);
   if (data == NULL)
   {
      Con::printf("ShaderBlueprint::onAdd() - unable to open file %s.", mBlueprintFile);
      return false;
   }

   // kick off parsing and set mShaderName
   if (!initParser(static_cast<const char*>(data), dataSize))
      return false;

   // now parse the entire blueprint..
   if (!parseShaderBlueprint())
      return false;

   Torque::FS::AddChangeNotification(mBlueprintFile, this, &ShaderBlueprint::_onFileChanged);

   return true;
}

void ShaderBlueprint::onRemove()
{

   Torque::FS::RemoveChangeNotification(mBlueprintFile, this, &ShaderBlueprint::_onFileChanged);

   Parent::onRemove();
}

bool ShaderBlueprint::_reload()
{
   void* data = NULL;
   U32 dataSize = 0;
   Torque::FS::ReadFile(mBlueprintFile, data, dataSize, true);
   if (data == NULL)
   {
      Con::printf("ShaderBlueprint::_reload() - unable to open file %s.", mBlueprintFile);
      return false;
   }

   if (!initParser(static_cast<const char*>(data), dataSize))
      return false;

   return true;
}


void ShaderBlueprint::nextToken()
{
   // skip white space, increment line on \n.
   // skip comments, we probably dont want to always do this.. might want comments in output shaders.
   while(isWhiteSpace() || isComment())
   {
   }

   // if we have an error do not continue.
   if (mError)
   {
      mCurToken = ShaderToken::EndOfFile;
      return;
   }

   mCurTokenLine = mLine;

   if (mBuffer >= mBufferEnd || mBuffer == '\0')
   {
      mCurToken = ShaderToken::EndOfFile;
      return;
   }

   if (mBuffer[0] == ';')
   {
      mTokenIdentifier = ";";
      mCurToken = ShaderToken::VariableName;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == ':')
   {
      mTokenIdentifier = ":";
      mCurToken = ShaderToken::VariableName;
      mBuffer += 1;
      return;
   }
   // should probably put all these symbol checks into their own function.....
   // Check for Bracket symbols and set identifier.
   if (mBuffer[0] == '(')
   {
      mTokenIdentifier = "(";
      mCurToken = ShaderToken::OpenParenthese;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == ')')
   {
      mTokenIdentifier = ")";
      mCurToken = ShaderToken::CloseParenthese;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '{')
   {
      mTokenIdentifier = "{";
      mCurToken = ShaderToken::OpenBrace;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '}')
   {
      mTokenIdentifier = "}";
      mCurToken = ShaderToken::CloseBrace;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '[')
   {
      mTokenIdentifier = "[";
      mCurToken = ShaderToken::OpenSquare;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == ']')
   {
      mTokenIdentifier = "]";
      mCurToken = ShaderToken::CloseSquare;
      mBuffer += 1;
      return;
   }

   // check for symbols and set our identifier to match.
   if (mBuffer[0] == '+')
   {
      mTokenIdentifier = "+";
      if (mBuffer[1] == '=')
      {
         mCurToken = ShaderToken::PlusEqual;
         mTokenIdentifier += "=";
         mBuffer += 2;
         return;
      }

      if (mBuffer[1] == '+')
      {
         mCurToken = ShaderToken::PlusPlus;
         mTokenIdentifier += "+";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::Plus;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '-')
   {
      mTokenIdentifier = "-";
      if (mBuffer[1] == '=')
      {
         mCurToken = ShaderToken::MinusEqual;
         mTokenIdentifier += "=";
         mBuffer += 2;
         return;
      }

      if (mBuffer[1] == '-')
      {
         mCurToken = ShaderToken::MinusMinus;
         mTokenIdentifier += "-";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::Minus;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '*')
   {
      mTokenIdentifier = "*";
      if (mBuffer[1] == '=')
      {
         mCurToken = ShaderToken::TimesEqual;
         mTokenIdentifier += "=";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::Times;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '/')
   {
      mTokenIdentifier = "/";
      if (mBuffer[1] == '=')
      {
         mCurToken = ShaderToken::DivideEqual;
         mTokenIdentifier += "=";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::Divide;
      mBuffer += 1;
      return;
   }

   // check for conditionals.
   if (mBuffer[0] == '!')
   {
      mTokenIdentifier = "!";

      if (mBuffer[1] == '=')
      {
         mCurToken = ShaderToken::NotEqual;
         mTokenIdentifier += "=";
         mBuffer += 2;
         return;
      }
   }

   if (mBuffer[0] == '<')
   {
      mTokenIdentifier = "<";
      if (mBuffer[1] == '=')
      {
         mCurToken = ShaderToken::LessEqual;
         mTokenIdentifier += "=";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::LessThan;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '>')
   {
      mTokenIdentifier = "<";
      if (mBuffer[1] == '=')
      {
         mCurToken = ShaderToken::GreaterEqual;
         mTokenIdentifier += "=";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::GreaterThan;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '&')
   {
      mTokenIdentifier = "&";
      if (mBuffer[1] == '&')
      {
         mCurToken = ShaderToken::AndAnd;
         mTokenIdentifier += "&";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::And;
      mBuffer += 1;
      return;
   }

   if (mBuffer[0] == '|')
   {
      mTokenIdentifier = "|";
      if (mBuffer[1] == '|')
      {
         mCurToken = ShaderToken::OrOr;
         mTokenIdentifier += "|";
         mBuffer += 2;
         return;
      }

      mCurToken = ShaderToken::Or;
      mBuffer += 1;
      return;
   }

   // just skip these, not used in shaders. Blueprint identifiers can use these though and that will be handled next.
   if (mBuffer[0] == '"' || mBuffer[0] == '\'')
   {
      mBuffer++;
      nextToken();
      return;
   }

   // keep track of where we started.
   const char* first = mBuffer;

   // start of a number?
   if (isNumber(mBuffer[0]))
   {
      // keep going until we get to a space.
      while (!dIsspace(mBuffer[0]))
      {
         mBuffer++;
      }

      U32 len = mBuffer - first;
      char temp[256];
      dMemcpy(temp, first, len);
      temp[len] = '\0';
      mTokenIdentifier = temp;

      // Numbers get cast to a variable name type so we can pull the string
      // out later.
      mCurToken = ShaderToken::VariableName;
      return;
   }

   // if we have reached here it must be a variable. 
   while (mBuffer < mBufferEnd && mBuffer[0] != 0 && !dIsspace(mBuffer[0]) && !isOpOrSymbol(mBuffer[0]))
   {
      mBuffer++;
   }

   U32 len = mBuffer - first;
   char temp[256];
   dMemcpy(temp, first, len);
   temp[len] = '\0';
   mTokenIdentifier = temp;
    
   // make sure its not a keyword, if it is set the token.
   auto MapFind = WordMap.find(temp);
   if (MapFind != WordMap.end())
   {
      mCurToken = MapFind->second;
      return;
   }

   // else this is a variable.
   mCurToken = ShaderToken::VariableName;
}

bool ShaderBlueprint::initParser(const char* buffer, U32 buffLen)
{
   mBuffer = buffer;
   mBufferEnd = buffer + buffLen;
   mError = false;
   mLine = 1;

   // kick off the next token.
   nextToken();

   // have we found an error already?
   if (mError)
      return false;

   if (!getToken(ShaderToken::Blueprint, true))
      return false;

   // next token should be a variable identifier for naming our Shader files.
   if (!getVariableName(mShaderName))
   {
      Con::printf("ShaderBlueprint Error - Name expected but was not found.");
      return false;
   }

   return true;
}

bool ShaderBlueprint::parseVariableDefinition(String& name, ShaderVarType*& type)
{
   if (!getTokenRange(ShaderToken::Void, ShaderToken::SamplerCubeArray, type->type, true))
   {
      return false;
   }

   // next token should be a variable identifier for naming the variable.
   if (!getVariableName(name))
   {
      return false;
   }

   // this is an array.
   if (getToken("["))
   {
      type->isArray = true;

      // is this an identifier because this could be set by a macro.
      if (getVariableName(type->size))
      {
         if (!getToken("]", true))
            return false;
      }
   }

   // is this variable being set to a default value?
   if (getToken("="))
   {
      if (type->isArray)
      {
         // arrays that are initialized should have a size as an integer, not a macro.
         S32 arraySize = -1;
         arraySize = dAtoi(type->size.c_str());

         if (arraySize == -1)
         {
            Con::printf("ShaderBlueprint - Error: array no initialized properly.");
            return false;
         }

         // check for open
         if (!getToken("{"))
         {
            return false;
         }

         // loop over our expected array entries.
         for (S32 i = 0; i < arraySize; i++)
         {
            // is this a scalar?
            if (type->type == ShaderToken::Float ||
               type->type == ShaderToken::Int ||
               type->type == ShaderToken::UInt)
            {
               // keep track of where we started.
               const char* first = mBuffer;

               while (!getToken(","))
               {
                  mBuffer++;
               }

               U32 len = mBuffer - first;
               char temp[256];
               dMemcpy(temp, first, len);
               temp[len] = '\0';
               type->arrayValues.push_back(temp);
            }
            else
            {
               // we are a vector type, a bit more complicated....
               if (!getToken(type->type))
               {
                  Con::printf("ShaderBlueprint - Error: array of vector type requires type before each value.");
                  return false;
               }

               if (!getToken("("))
               {
                  return false;
               }

               const char* first = mBuffer;

               while (!getToken(")"))
               {
                  mBuffer++;
               }

               U32 len = mBuffer - first;
               char temp[256];
               dMemcpy(temp, first, len);
               temp[len] = '\0';
               type->arrayValues.push_back(temp);

               // get our separator, go to the next token to be ready for the next loop
               if (i != arraySize - 1)
               {
                  if (!getToken(","))
                  {
                     return false;
                  }
               }

            }
         }

         // check for close, false if this doesnt exist.
         if (!getToken("}"))
         {
            return false;
         }

         // check for a line ending, if it doesn't exist we will add one anyway..
         // this is mostly to get onto the next token.
         if (!getToken(";"))
         {
            return true;
         }
      }
      else
      {
         // keep track of where we started.
         const char* first = mBuffer;

         while (!getToken(";"))
         {
            mBuffer++;
         }

         U32 len = mBuffer - first;
         char temp[256];
         dMemcpy(temp, first, len);
         temp[len] = '\0';
         type->initValue = temp;
      }
   }

   return true;
}

bool ShaderBlueprint::parseStructVariable(ShaderStructVar*& var, bool semantic)
{
   if (!parseVariableDefinition(var->name, var->type))
   {
      return false;
   }

   if (semantic)
   {
      if (!getToken(":", true))
      {
         return false;
      }

      String semanticName;
      if (!getVariableName(semanticName, true))
      {
         return false;
      }

      S32 semanticRegister = -1;
      semanticName = String::GetTrailingNumber(semanticName, semanticRegister);

      // make sure its not a keyword, if it is set the token.
      auto MapFind = SemanticMap.find(semanticName.c_str());
      if (MapFind != SemanticMap.end())
      {
         var->semantic = MapFind->second;
      }

      if (var->semantic == StructVarSemantic::SV_Target && semanticRegister > 7)
      {
         Con::printf("ShaderBlueprint - Error: SV_Target can only have a max register of 7");
         semanticRegister = 7;
      }

      var->semanticRegister = semanticRegister;
   }

   if (!getToken(";", true))
   {
      return false;
   }

   return true;
}

bool ShaderBlueprint::parseShaderBlueprint()
{
   if (getToken("{", true))
   {
      while (!getToken("}"))
      {
         // is this an include?
         if (getToken(ShaderToken::Include))
         {

         }

         // Parse our structure shader datat blocks.
         if (getToken(ShaderToken::StructBlock))
         {
            // is this within the ranges of the shaderstage data.
            if (isTokenInRange(ShaderToken::VertexData, ShaderToken::HullData))
            {
               U32 dataIdx = 0;

               // are we an expected struct.
               switch (mCurToken)
               {
               case ShaderToken::VertexData:
                  dataIdx = 0;
                  break;
               case ShaderToken::PixelData:
                  dataIdx = 1;
                  break;
               case ShaderToken::ComputeData:
                  dataIdx = 2;
                  break;
               case ShaderToken::GeometryData:
                  dataIdx = 3;
                  break;
               case ShaderToken::DomainData:
                  dataIdx = 4;
                  break;
               case ShaderToken::HullData:
                  dataIdx = 5;
                  break;
               default:
                  Con::printf("ShaderBlueprint - Error: User Defined struct not expected: '%s'!", mTokenIdentifier.c_str());
                  return false;
               }

               // create a struct for this.
               mBlueprintStructs[dataIdx] = new ShaderStruct();
               // Set name and ready next token.
               mBlueprintStructs[dataIdx]->name = mTokenIdentifier;
               nextToken();

               if (!getToken("{", true))
               {
                  // error 
                  return false;
               }

               // Lets get the fields.
               ShaderStructVar* prevVar = NULL; // prev var
               while (!getToken("}"))
               {
                  // unexpected end of struct.
                  if (getToken("}"))
                     return false;

                  // parse struct variabes, since this is shaderdata they require
                  // semantic specifiers.
                  ShaderStructVar* var = new ShaderStructVar();
                  if (!parseStructVariable(var, true))
                  {
                     return false;
                  }

                  if (prevVar == NULL)
                  {
                     mBlueprintStructs[dataIdx]->firstVar = var;
                  }
                  else
                  {
                     prevVar->nextVar = var;
                  }

                  prevVar = var;
               }
            }
            //else
            //{
            //   // if not this is a user defined data type that will be a global, these structs
            //   // will get copied into every shader stage upon generation.

            //}
         }

         // is this within the ranges of the shader stages.
         if (isTokenInRange(ShaderToken::Vertex, ShaderToken::Hull))
         {
            // now get the shaderstage token and move to the next token.
            ShaderToken shaderStage;
            getTokenRange(ShaderToken::Vertex, ShaderToken::Hull, shaderStage);
         }
      }
   }

   return true;
}


