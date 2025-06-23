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
#include "shaderGen/HLSL/shaderCompHLSL.h"
#include "shaderGen/shaderComp.h"
#include "shaderGen/langElement.h"
#include "gfx/gfxDevice.h"


Var * ShaderConnectorHLSL::getElement( RegisterType type, 
                                       U32 numElements, 
                                       U32 numRegisters )
{
   Var *ret = NULL;
   
   if ( type == RT_BLENDINDICES )
   {
      ret = getIndexedElement( mCurBlendIndicesElem, type, numElements, numRegisters );
   }
   else if ( type == RT_BLENDWEIGHT )
   {
      ret = getIndexedElement( mCurBlendWeightsElem, type, numElements, numRegisters );
   }
   else
   {
      ret = getIndexedElement( mCurTexElem, type, numElements, numRegisters );
   }

   // Adjust texture offset if this is a texcoord type
   if( type == RT_TEXCOORD )
   {
      if ( numRegisters != -1 )
         mCurTexElem += numRegisters;
      else
         mCurTexElem += numElements;
   }
   else if ( type == RT_BLENDINDICES )
   {
      if ( numRegisters != -1 )
         mCurBlendIndicesElem += numRegisters;
      else
         mCurBlendIndicesElem += numElements;
   }
   else if ( type == RT_BLENDWEIGHT )
   {
      if ( numRegisters != -1 )
         mCurBlendWeightsElem += numRegisters;
      else
         mCurBlendWeightsElem += numElements;
   }

   return ret;
}

Var * ShaderConnectorHLSL::getIndexedElement( U32 index, RegisterType type, U32 numElements /*= 1*/, U32 numRegisters /*= -1 */ )
{
   switch( type )
   { 
   case RT_POSITION:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );
         newVar->setConnectName( "POSITION" );
         newVar->rank = 0;
         return newVar;
      }

   case RT_VPOS:
      {
         Var *newVar = new Var;
         mElementList.push_back(newVar);
         newVar->setConnectName("VPOS");
         newVar->rank = 0;
         return newVar;
      }

   case RT_SVPOSITION:
      {
         Var *newVar = new Var;
         mElementList.push_back(newVar);
         newVar->setConnectName("SV_Position");
         newVar->rank = 0;
         return newVar;
      }

   case RT_NORMAL:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );
         newVar->setConnectName( "NORMAL" );
         newVar->rank = 1;
         return newVar;
      }

   case RT_BINORMAL:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );
         newVar->setConnectName( "BINORMAL" );
         newVar->rank = 2;
         return newVar;
      }

   case RT_TANGENT:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );
         newVar->setConnectName( "TANGENT" );
         newVar->rank = 3;
         return newVar;
      }

   case RT_COLOR:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );
         newVar->setConnectName( "COLOR" );
         newVar->rank = 4;
         return newVar;
      }

   case RT_TEXCOORD:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );

         // This was needed for hardware instancing, but
         // i don't really remember why right now.
         if ( index > mCurTexElem )
            mCurTexElem = index + 1;

         char out[32];
         dSprintf( (char*)out, sizeof(out), "TEXCOORD%d", index );
         newVar->setConnectName( out );
         newVar->constNum = index;
         newVar->arraySize = numElements;
         newVar->rank = 5 + index;

         return newVar;
      }

   case RT_BLENDINDICES:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );

         // This was needed for hardware instancing, but
         // i don't really remember why right now.
         if ( index > mCurBlendIndicesElem )
            mCurBlendIndicesElem = index + 1;

         char out[32];
         dSprintf( (char*)out, sizeof(out), "BLENDINDICES%d", index );
         newVar->setConnectName( out );
         newVar->constNum = index;
         newVar->arraySize = numElements;

         return newVar;
      }

   case RT_BLENDWEIGHT:
      {
         Var *newVar = new Var;
         mElementList.push_back( newVar );

         // This was needed for hardware instancing, but
         // i don't really remember why right now.
         if ( index > mCurBlendWeightsElem )
            mCurBlendWeightsElem = index + 1;

         char out[32];
         dSprintf( (char*)out, sizeof(out), "BLENDWEIGHT%d", index );
         newVar->setConnectName( out );
         newVar->constNum = index;
         newVar->arraySize = numElements;

         return newVar;
      }



   default:
      break;
   }

   return NULL;
}



S32 QSORT_CALLBACK ShaderConnectorHLSL::_hlsl4VarSort(const void* e1, const void* e2)
{
   Var* a = *((Var **)e1);
   Var* b = *((Var **)e2);

   return a->rank - b->rank;
}

void ShaderConnectorHLSL::sortVars()
{

   // If shader model 4+ than we gotta sort the vars to make sure the order is consistent
   if (GFX->getPixelShaderVersion() >= 4.f)
   {
      dQsort((void *)&mElementList[0], mElementList.size(), sizeof(Var *), _hlsl4VarSort);
      return;
   }

   return;
}

void ShaderConnectorHLSL::setName( const char *newName )
{
   dStrcpy( (char*)mName, newName, 32 );
}

void ShaderConnectorHLSL::reset()
{
   for( U32 i=0; i<mElementList.size(); i++ )
   {
      mElementList[i] = NULL;
   }

   mElementList.setSize( 0 );
   mCurTexElem = 0;
   mCurBlendIndicesElem = 0;
   mCurBlendWeightsElem = 0;
}

void ShaderConnectorHLSL::print( Stream &stream, bool isVertexShader )
{
   const char * header = "struct ";
   const char * header2 = "\r\n{\r\n";
   const char * footer = "};\r\n\r\n\r\n";

   stream.write( dStrlen(header), header );
   stream.write( dStrlen((char*)mName), mName );
   stream.write( dStrlen(header2), header2 );


   // print out elements
   for( U32 i=0; i<mElementList.size(); i++ )
   {
      U8 output[256];

      Var *var = mElementList[i];
      if (var->arraySize <= 1)
         dSprintf( (char*)output, sizeof(output), "   %s %-15s : %s;\r\n", var->type, var->name, var->connectName );
      else
         dSprintf( (char*)output, sizeof(output), "   %s %s[%d] : %s;\r\n", var->type, var->name, var->arraySize, var->connectName );

      stream.write( dStrlen((char*)output), output );
   }

   stream.write( dStrlen(footer), footer );
}

void ParamsDefHLSL::assignConstantNumbers()
{

   // Here we assign constant number to uniform vars, sorted 
   // by their update frequency.

   U32 mCurrConst = 0;
   for (U32 bin = cspUninit+1; bin < csp_Count; bin++)
   {   
      // Find all the uniform variables that are part of this group and assign constant numbers
      for( U32 i=0; i<LangElement::elementList.size(); i++)
      {
         Var *var = dynamic_cast<Var*>(LangElement::elementList[i]);
         if( var && var->constBufferNum == -1)
         {            
            bool shaderConst = var->uniform && !var->sampler && !var->texture;
            AssertFatal((!shaderConst) || var->constSortPos != cspUninit, "Const sort position has not been set, variable will not receive a constant number!!");
            if( shaderConst && var->constSortPos == bin)
            {
               var->constNum = mCurrConst;
               // Increment our constant number based on the variable type
               if (String::compare((const char*)var->type, "float4x4") == 0)
               {
                  mCurrConst += (4 * var->arraySize);
               }
               else
               {
                  if (String::compare((const char*)var->type, "float3x3") == 0)
                  {
                     mCurrConst += (3 * var->arraySize);
                  }
                  else
                  {
                     if (String::compare((const char*)var->type, "float4x3") == 0)
                     {
                        mCurrConst += (3 * var->arraySize);
                     }
                     else
                     {
                        mCurrConst += var->arraySize;
                     }
                  }
               }
            }
         }
      }
   }
}

void VertexParamsDefHLSL::print( Stream &stream, bool isVerterShader )
{
   assignConstantNumbers();

   const char *opener = "ConnectData main( VertData IN";
   stream.write( dStrlen(opener), opener );

   // find all the uniform variables and print them out
   for( U32 i=0; i<LangElement::elementList.size(); i++)
   {
      Var *var = dynamic_cast<Var*>(LangElement::elementList[i]);
      if( var )
      {
         if( var->uniform && var->constBufferNum == -1 )
         {
            const char* nextVar = ",\r\n                  ";
            stream.write( dStrlen(nextVar), nextVar );            

            U8 varNum[64];
            dSprintf( (char*)varNum, sizeof(varNum), "register(C%d)", var->constNum );

            U8 output[256];
            if (var->arraySize <= 1)
               dSprintf( (char*)output, sizeof(output), "uniform %-8s %-15s : %s", var->type, var->name, varNum );
            else
               dSprintf( (char*)output, sizeof(output), "uniform %-8s %s[%d] : %s", var->type, var->name, var->arraySize, varNum );

            stream.write( dStrlen((char*)output), output );
         }
      }
   }

   const char *closer = "\r\n)\r\n{\r\n   ConnectData OUT;\r\n\r\n";
   stream.write( dStrlen(closer), closer );
}

void PixelParamsDefHLSL::print( Stream &stream, bool isVerterShader )
{
   assignConstantNumbers();

   const char * opener = "Fragout main( ConnectData IN";
   stream.write( dStrlen(opener), opener );

   // find all the sampler & uniform variables and print them out
   for( U32 i=0; i<LangElement::elementList.size(); i++)
   {
      Var *var = dynamic_cast<Var*>(LangElement::elementList[i]);
      if( var )
      {
         if( var->uniform && var->constBufferNum == -1)
         {
            WRITESTR( ",\r\n              " );

            U8 varNum[32];

            if( var->sampler )
            {
               dSprintf( (char*)varNum, sizeof(varNum), ": register(S%d)", var->constNum );
            }
            else if (var->texture)
            {
               dSprintf((char*)varNum, sizeof(varNum), ": register(T%d)", var->constNum);
            }
            else
            {
               dSprintf( (char*)varNum, sizeof(varNum), ": register(C%d)", var->constNum );
            }

            U8 output[256];
            if (var->arraySize <= 1)
               dSprintf( (char*)output, sizeof(output), "uniform %-9s %-15s %s", var->type, var->name, varNum );
            else
               dSprintf( (char*)output, sizeof(output), "uniform %-9s %s[%d] %s", var->type, var->name, var->arraySize, varNum );

            WRITESTR( (char*) output );
         }
      }
   }

   const char *closer = "\r\n)\r\n{\r\n   Fragout OUT;\r\n\r\n";
   stream.write( dStrlen(closer), closer );
}

void ConstBufferParamsHLSL::addSceneConstBufferVars()
{
   // 16 alignment so for simplicity these are spaced out based on that
   createCBufferVar("oneOverFarplane", "float4", 0);

   createCBufferVar("fogColor", "float4", 0);

   createCBufferVar("fogData", "float3", 0);
   createCBufferVar("dampness", "float", 0);

   //createCBufferVar("accumTime", "float", 0);

   createCBufferVar("vEye", "float3", 0);
   createCBufferVar("scene_pack_var0", "float", 0);
   
   //createCBufferVar("eyePos", "float3", 0);
   //createCBufferVar("visibility", "float", 0);
  
}

void ConstBufferParamsHLSL::addCameraConstBufferVars()
{
   createCBufferVar("viewProj", "float4x4", 1);

   createCBufferVar("worldToCamera", "float4x4", 1);

   createCBufferVar("cameraToWorld", "float4x4", 1);

}

void ConstBufferParamsHLSL::addMaterialConstBufferVars()
{
   // 16 alignment so for simplicity these are spaced out based on that
   createCBufferVar("diffuseMaterialColor", "float4", 2);

   createCBufferVar("diffuseAtlasParams", "float4", 2);

   createCBufferVar("diffuseAtlasTileParams", "float4", 2);

   createCBufferVar("bumpAtlasParams", "float4", 2);

   createCBufferVar("bumpAtlasTileParams", "float4", 2);

   createCBufferVar("subSurfaceParams", "float4", 2);

   createCBufferVar("targetSize", "float2", 2);
   createCBufferVar("oneOverTargetSize", "float2", 2);

   createCBufferVar("roughness", "float", 2);
   createCBufferVar("metalness", "float", 2);
   createCBufferVar("glowMul", "float", 2);
   createCBufferVar("alphaTestValue", "float", 2);

   createCBufferVar("matInfoFlags", "float", 2);
   createCBufferVar("detailBumpStrength", "float", 2);
   createCBufferVar("minnaertConstant", "float", 2);
   createCBufferVar("accuScale", "float", 2);

   createCBufferVar("accuDirection", "float", 2);
   createCBufferVar("accuStrength", "float", 2);
   createCBufferVar("accuCoverage", "float", 2);
   createCBufferVar("accuSpecular", "float", 2);

   createCBufferVar("imposterLimits", "float", 2);
   // this is to ensure the 16bit alignment, we can find something for this im sure.
   createCBufferVar("mat_pack_Var", "float3", 2);

}

void ConstBufferParamsHLSL::addObjectConstBufferVars()
{
   createCBufferVar("modelview", "float4x4", 3);

   createCBufferVar("worldToObj", "float4x4", 3);

   createCBufferVar("viewToObj", "float4x4", 3);

   createCBufferVar("objTrans", "float4x4", 3);

   createCBufferVar("worldViewOnly", "float4x4", 3);
}

void ConstBufferParamsHLSL::printSceneConstBuffer(Stream& stream)
{
   const char* opener = "cbuffer SceneData : register(b0)\r\n{\r\n";
   stream.write(dStrlen(opener), opener);

   for (U32 i = 0; i < LangElement::elementList.size(); i++)
   {
      Var* var = dynamic_cast<Var*>(LangElement::elementList[i]);
      if (var)
      {
         if (var->uniform && var->constBufferNum == 0)
         {
            char bufferLine[256];
            if (var->arraySize <= 1)
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s;\r\n", var->type, var->name);
            else
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s[%d];\r\n", var->type, var->name, var->arraySize);

            stream.write(dStrlen(bufferLine), bufferLine);
         }
      }
   }

   const char* closer = "};\r\n\r\n";
   stream.write(dStrlen(closer), closer);
}

void ConstBufferParamsHLSL::printCameraConstBuffer(Stream& stream)
{
   const char* opener = "cbuffer CameraMatrixData : register(b1)\r\n{\r\n";
   stream.write(dStrlen(opener), opener);

   for (U32 i = 0; i < LangElement::elementList.size(); i++)
   {
      Var* var = dynamic_cast<Var*>(LangElement::elementList[i]);
      if (var)
      {
         if (var->uniform && var->constBufferNum == 1)
         {
            char bufferLine[256];
            if (var->arraySize <= 1)
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s;\r\n", var->type, var->name);
            else
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s[%d];\r\n", var->type, var->name, var->arraySize);

            stream.write(dStrlen(bufferLine), bufferLine);
         }
      }
   }

   const char* closer = "};\r\n\r\n";
   stream.write(dStrlen(closer), closer);
}

void ConstBufferParamsHLSL::printMaterialConstBuffer(Stream& stream)
{
   const char* opener = "cbuffer MaterialData : register(b2)\r\n{\r\n";
   stream.write(dStrlen(opener), opener);

   for (U32 i = 0; i < LangElement::elementList.size(); i++)
   {
      Var* var = dynamic_cast<Var*>(LangElement::elementList[i]);
      if (var)
      {
         if (var->uniform && var->constBufferNum == 2)
         {
            char bufferLine[256];
            if (var->arraySize <= 1)
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s;\r\n", var->type, var->name);
            else
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s[%d];\r\n", var->type, var->name, var->arraySize);

            stream.write(dStrlen(bufferLine), bufferLine);
         }
      }
   }

   const char* closer = "};\r\n\r\n";
   stream.write(dStrlen(closer), closer);
}

void ConstBufferParamsHLSL::printObjectConstBuffer(Stream& stream)
{
   const char* opener = "cbuffer ObjectData : register(b3)\r\n{\r\n";
   stream.write(dStrlen(opener), opener);

   for (U32 i = 0; i < LangElement::elementList.size(); i++)
   {
      Var* var = dynamic_cast<Var*>(LangElement::elementList[i]);
      if (var)
      {
         if (var->uniform && var->constBufferNum == 3)
         {
            char bufferLine[256];
            if (var->arraySize <= 1)
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s;\r\n", var->type, var->name);
            else
               dSprintf(bufferLine, sizeof(bufferLine), "   %-8s %s[%d];\r\n", var->type, var->name, var->arraySize);

            stream.write(dStrlen(bufferLine), bufferLine);
         }
      }
   }

   const char* closer = "};\r\n\r\n";
   stream.write(dStrlen(closer), closer);
}

ConstBufferParamsHLSL::ConstBufferParamsHLSL()
{
   addSceneConstBufferVars();
   addCameraConstBufferVars();
   addMaterialConstBufferVars();
   addObjectConstBufferVars();
}

void ConstBufferParamsHLSL::print(Stream& stream, bool isVerterShader)
{
   printSceneConstBuffer(stream);
   printCameraConstBuffer(stream);
   printMaterialConstBuffer(stream);
   printObjectConstBuffer(stream);
}
