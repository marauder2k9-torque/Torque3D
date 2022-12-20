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
#include "shaderGen/HLSL/depthHLSL.h"

#include "materials/materialFeatureTypes.h"
#include "materials/materialFeatureData.h"
#include "terrain/terrFeatureTypes.h"
#include "shaderGen/shaderGen.h"

void EyeSpaceDepthOutHLSL::processVert(   Vector<ShaderComponent*> &componentList, 
                                          const MaterialFeatureData &fd )
{
   MultiLine *meta = new MultiLine;
   output = meta;

   // grab output
   ShaderConnector *connectComp = dynamic_cast<ShaderConnector *>( componentList[C_CONNECTOR] );
   Var *outWSEyeVec = connectComp->getElement( RT_TEXCOORD );
   outWSEyeVec->setName( "wsEyeVec" );
   outWSEyeVec->setStructName( "OUT" );

   // grab incoming vert position   
   Var *wsPosition = new Var( "depthPos", "float3" );
   getWsPosition( componentList, fd.features[MFT_UseInstancing], meta, new DecOp( wsPosition ) );

   Var *eyePos = (Var*)LangElement::find( "eyePosWorld" );
   if( !eyePos )
   {
      eyePos = new Var;
      eyePos->setType("float3");
      eyePos->setName("eyePosWorld");
      eyePos->uniform = true;
      eyePos->constSortPos = cspPass;
   }

   meta->addStatement( new GenOp( "   @ = float4( @.xyz - @, 1 );\r\n", outWSEyeVec, wsPosition, eyePos ) );
}

void EyeSpaceDepthOutHLSL::processPix( Vector<ShaderComponent*> &componentList, 
                                       const MaterialFeatureData &fd )
{      
   MultiLine *meta = new MultiLine;

   // grab connector position
   ShaderConnector *connectComp = dynamic_cast<ShaderConnector *>( componentList[C_CONNECTOR] );
   Var *wsEyeVec = connectComp->getElement( RT_TEXCOORD );
   wsEyeVec->setName( "wsEyeVec" );
   wsEyeVec->setStructName( "IN" );
   wsEyeVec->setType( "float4" );
   wsEyeVec->uniform = false;

   // get shader constants
   Var *vEye = new Var;
   vEye->setType("float3");
   vEye->setName("vEye");
   vEye->uniform = true;
   vEye->constSortPos = cspPass;

   // Expose the depth to the depth format feature
   Var *depthOut = new Var;
   depthOut->setType("float");
   depthOut->setName(getOutputVarName());

   LangElement *depthOutDecl = new DecOp( depthOut );

   meta->addStatement( new GenOp( "#ifndef CUBE_SHADOW_MAP\r\n" ) );

   if (fd.features.hasFeature(MFT_TerrainBaseMap))
      meta->addStatement(new GenOp("   @ =min(0.9999, dot(@, (@.xyz / @.w)));\r\n", depthOutDecl, vEye, wsEyeVec, wsEyeVec));
   else
      meta->addStatement(new GenOp("   @ = dot(@, (@.xyz / @.w));\r\n", depthOutDecl, vEye, wsEyeVec, wsEyeVec));

   meta->addStatement( new GenOp( "#else\r\n" ) );

   Var *farDist = (Var*)Var::find( "oneOverFarplane" );
   if ( !farDist )
   {
      farDist = new Var;
      farDist->setType("float4");
      farDist->setName("oneOverFarplane");
      farDist->uniform = true;
      farDist->constSortPos = cspPass;
   }

   meta->addStatement( new GenOp( "   @ = length( @.xyz / @.w ) * @.x;\r\n", depthOutDecl, wsEyeVec, wsEyeVec, farDist ) );      
   meta->addStatement( new GenOp( "#endif\r\n" ) );

   // If there isn't an output conditioner for the pre-pass, than just write
   // out the depth to rgba and return.
   if( !fd.features[MFT_DeferredConditioner] )
      meta->addStatement( new GenOp( "   @;\r\n", assignColor( new GenOp( "float4(@.r,0,0,1)", depthOut ), Material::None ) ) );
   
   output = meta;
}

ShaderFeature::Resources EyeSpaceDepthOutHLSL::getResources( const MaterialFeatureData &fd )
{
   Resources temp;

   // Passing from VS->PS:
   // - world space position (wsPos)
   temp.numTexReg = 1; 

   return temp; 
}

ShadowDepthOutHLSL::ShadowDepthOutHLSL()
   :mComputeDepth(ShaderGen::smCommonShaderPath + String("/computeDepth.hlsl"))
{
   addDependency(&mComputeDepth);
}

void ShadowDepthOutHLSL::processVert(Vector<ShaderComponent*>& componentList,
   const MaterialFeatureData& fd)
{
   MultiLine* meta = new MultiLine;
   output = meta;

   // grab output
   ShaderConnector* connectComp = dynamic_cast<ShaderConnector*>(componentList[C_CONNECTOR]);
   Var* outWSEyeVec = connectComp->getElement(RT_TEXCOORD);
   outWSEyeVec->setName("wsEyeVec");
   outWSEyeVec->setStructName("OUT");

   // grab incoming vert position   
   Var* wsPosition = new Var("depthPos", "float3");
   getWsPosition(componentList, fd.features[MFT_UseInstancing], meta, new DecOp(wsPosition));

   Var* eyePos = (Var*)LangElement::find("eyePosWorld");
   if (!eyePos)
   {
      eyePos = new Var;
      eyePos->setType("float3");
      eyePos->setName("eyePosWorld");
      eyePos->uniform = true;
      eyePos->constSortPos = cspPass;
   }

   meta->addStatement(new GenOp("   @ = float4( @.xyz - @, 1 );\r\n", outWSEyeVec, wsPosition, eyePos));
}

void ShadowDepthOutHLSL::processPix(Vector<ShaderComponent*>& componentList,
   const MaterialFeatureData& fd)
{
   MultiLine* meta = new MultiLine;

   // grab connector position
   ShaderConnector* connectComp = dynamic_cast<ShaderConnector*>(componentList[C_CONNECTOR]);
   Var* wsEyeVec = connectComp->getElement(RT_TEXCOORD);
   wsEyeVec->setName("wsEyeVec");
   wsEyeVec->setStructName("IN");
   wsEyeVec->setType("float4");
   wsEyeVec->uniform = false;

   // get shader constants
   Var* vEye = new Var;
   vEye->setType("float3");
   vEye->setName("vEye");
   vEye->uniform = true;
   vEye->constSortPos = cspPass;

   // Expose the depth to the depth format feature
   Var* depthOut = new Var;
   depthOut->setType("float");
   depthOut->setName(getOutputVarName());

   LangElement* depthOutDecl = new DecOp(depthOut);

   meta->addStatement(new GenOp("#ifndef CUBE_SHADOW_MAP\r\n"));

   if (fd.features.hasFeature(MFT_TerrainBaseMap))
      meta->addStatement(new GenOp("   @ =min(0.9999, dot(@, (@.xyz / @.w)));\r\n", depthOutDecl, vEye, wsEyeVec, wsEyeVec));
   else
      meta->addStatement(new GenOp("   @ = dot(@, (@.xyz / @.w));\r\n", depthOutDecl, vEye, wsEyeVec, wsEyeVec));

   meta->addStatement(new GenOp("#else\r\n"));

   Var* farDist = (Var*)Var::find("oneOverFarplane");
   if (!farDist)
   {
      farDist = new Var;
      farDist->setType("float4");
      farDist->setName("oneOverFarplane");
      farDist->uniform = true;
      farDist->constSortPos = cspPass;
   }

   meta->addStatement(new GenOp("   @ = length( @.xyz / @.w ) * @.x;\r\n", depthOutDecl, wsEyeVec, wsEyeVec, farDist));
   meta->addStatement(new GenOp("#endif\r\n"));

   // VSM
   Var* compDepth = (Var*)Var::find("computeDepth");
   if (!compDepth)
   {
      compDepth = new Var;
      compDepth->setType("float2");
      compDepth->setName("computeDepth");
      compDepth->uniform = false;
   }
   meta->addStatement(new GenOp("   float2 @ = ComputeMoments(@);\r\n", compDepth, depthOut));
   if (!fd.features[MFT_DeferredConditioner])
       meta->addStatement(new GenOp("   @;\r\n", assignColor(new GenOp("float4(@,0,0)", compDepth), Material::None)));
   

   //EVSM
   /*
   Var* avg = (Var*)Var::find("average");
   if (!avg)
   {
      avg = new Var;
      avg->setType("float4");
      avg->setName("average");
      avg->uniform = false;
   }
   LangElement* avgDecl = new DecOp(avg);
   meta->addStatement(new GenOp("   @ = float4(0.0f,0.0f,0.0f,0.0f);\r\n", avgDecl));

   meta->addStatement(new GenOp("   float2 exponents = GetEVSMExponent(40.0f, 5.0f);\r\n"));
   Var* vsmDepth = (Var*)Var::find("vsmDepth");
   if (!vsmDepth)
   {
      vsmDepth = new Var;
      vsmDepth->setType("float2");
      vsmDepth->setName("vsmDepth");
      vsmDepth->uniform = false;
   }

   LangElement* vsmDepthDecl = new DecOp(vsmDepth);

   meta->addStatement(new GenOp("   @ = WarpDepth(@, exponents);\r\n", vsmDepthDecl, depthOut));
   meta->addStatement(new GenOp("   @ = float4(@.xy,@.xy*@.xy);\r\n", avg, vsmDepth, vsmDepth, vsmDepth));

   // If there isn't an output conditioner for the pre-pass, than just write
   // out the depth to rgba and return.
   if (!fd.features[MFT_DeferredConditioner])
      meta->addStatement(new GenOp("   @;\r\n", assignColor(new GenOp("float4(@)", avg), Material::None)));
   */

   output = meta;
}

ShaderFeature::Resources ShadowDepthOutHLSL::getResources(const MaterialFeatureData& fd)
{
   Resources temp;

   // Passing from VS->PS:
   // - world space position (wsPos)
   temp.numTexReg = 1;

   return temp;
}

DepthOutHLSL::DepthOutHLSL()
   :mComputeDepth(ShaderGen::smCommonShaderPath + String("/computeDepth.hlsl"))
{
   addDependency(&mComputeDepth);
}

void DepthOutHLSL::processVert(  Vector<ShaderComponent*> &componentList,
                                 const MaterialFeatureData &fd )
{
   ShaderConnector *connectComp = dynamic_cast<ShaderConnector *>( componentList[C_CONNECTOR] );

   // Grab the output vert.
   Var *outPosition = (Var*)LangElement::find( "hpos" );

   // Grab our output depth.
   Var *outDepth = connectComp->getElement( RT_TEXCOORD );
   outDepth->setName( "depth" );
   outDepth->setStructName( "OUT" );
   outDepth->setType( "float" );

   output = new GenOp( "   @ = @.z / @.w;\r\n", outDepth, outPosition, outPosition );
}

void DepthOutHLSL::processPix(   Vector<ShaderComponent*> &componentList, 
                                 const MaterialFeatureData &fd )
{
   MultiLine* meta = new MultiLine;
   ShaderConnector *connectComp = dynamic_cast<ShaderConnector *>( componentList[C_CONNECTOR] );

   // grab connector position
   Var *depthVar = connectComp->getElement( RT_TEXCOORD );
   depthVar->setName( "depth" );
   depthVar->setStructName( "IN" );
   depthVar->setType( "float" );
   depthVar->uniform = false;

   /*
   // Expose the depth to the depth format feature
   Var *depthOut = new Var;
   depthOut->setType("float");
   depthOut->setName(getOutputVarName());
   */

   // VSM
   Var* compDepth = (Var*)Var::find("computeDepth");
   if (!compDepth)
   {
      compDepth = new Var;
      compDepth->setType("float2");
      compDepth->setName("computeDepth");
      compDepth->uniform = false;
   }
   meta->addStatement(new GenOp("   float2 @ = ComputeMoments(@);\r\n", compDepth, depthVar));
   if (!fd.features[MFT_DeferredConditioner])
       meta->addStatement(new GenOp("   @;\r\n", assignColor(new GenOp("float4(@,0,0)", compDepth), Material::None)));
   

   //EVSM
   /*Var* avg = (Var*)Var::find("average");
   if (!avg)
   {
      avg = new Var;
      avg->setType("float4");
      avg->setName("average");
      avg->uniform = false;
   }
   LangElement* avgDecl = new DecOp(avg);
   meta->addStatement(new GenOp("   @ = float4(0.0f,0.0f,0.0f,0.0f);\r\n", avgDecl));

   meta->addStatement(new GenOp("   float2 exponents = GetEVSMExponent(40.0f, 5.0f);\r\n"));
   Var* vsmDepth = (Var*)Var::find("vsmDepth");
   if (!vsmDepth)
   {
      vsmDepth = new Var;
      vsmDepth->setType("float2");
      vsmDepth->setName("vsmDepth");
      vsmDepth->uniform = false;
   }

   LangElement* vsmDepthDecl = new DecOp(vsmDepth);

   meta->addStatement(new GenOp("   @ = WarpDepth(@, exponents);\r\n", vsmDepthDecl, depthVar));
   meta->addStatement(new GenOp("   @ = float4(@.xy,@.xy*@.xy);\r\n", avg, vsmDepth, vsmDepth, vsmDepth));

   LangElement *depthOut = new GenOp( "float4( @ )", avg);

   meta->addStatement(new GenOp("   @;\r\n", assignColor(depthOut, Material::None)));

   output = meta;
   */
   // DEFAULT
   //LangElement* depthOut = new GenOp("float4( @, 0, 0, 1 )", depthVar);
   //meta->addStatement(new GenOp("   @;\r\n", assignColor(depthOut, Material::None)));

   output = meta;
}

ShaderFeature::Resources DepthOutHLSL::getResources( const MaterialFeatureData &fd )
{
   // We pass the depth to the pixel shader.
   Resources temp;
   temp.numTexReg = 1; 

   return temp; 
}
