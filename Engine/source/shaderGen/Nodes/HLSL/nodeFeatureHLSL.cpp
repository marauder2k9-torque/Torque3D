#include "platform/platform.h"
#include "shaderGen/Nodes/HLSL/nodeFeatureHLSL.h"

#include "shaderGen/langElement.h"
#include "shaderGen/shaderOp.h"
#include "shaderGen/featureType.h"
#include "shaderGen/featureMgr.h"
#include "shaderGen/shaderGen.h"
#include "core/module.h"
#include "materials/materialFeatureTypes.h"

namespace
{
   void register_hlsl_shader_features_for_nodes(GFXAdapterType type)
   {
      if (type != Direct3D11)
         return;

      FEATUREMGR->registerFeature(SNF_TextureFeature, new NodeTextureFeatureHLSL, NodeTextureFeatureHLSL::createFunction);
   }
};

MODULE_BEGIN(NodeFeatureHLSL)

   MODULE_INIT_AFTER(ShaderGen)

   MODULE_INIT
   {
      SHADERGEN->getFeatureInitSignal().notify(&register_hlsl_shader_features_for_nodes);
   }

MODULE_END;

NodeFeatureHLSL::NodeFeatureHLSL()
{}

Var* NodeFeatureHLSL::getNodeOutTexCoord( const char* name,
                                          const char* type,
                                          MultiLine* meta,
                                          Vector<ShaderComponent*>& componentList)
{
   // The default input texcoord from torque needs to exist.
   Var* inTex = getVertTexCoord("texCoord");
   AssertFatal(inTex, "ShaderFeatureHLSL::getOutTexCoord - Unknown vertex input coord!");

   // TODO: if we use spirv in future names for in and out must match so it can parse the structs
   // correctly, they also have to match across all stages.
   String outTexName = String::ToString("out_%s", name);
   Var* texCoord = (Var*)LangElement::find(outTexName);

   if (!texCoord)
   {
      ShaderConnector* connectComp = dynamic_cast<ShaderConnector*>(componentList[C_CONNECTOR]);

      texCoord = connectComp->getElement(RT_TEXCOORD);
      texCoord->setName(outTexName);
      texCoord->setStructName("OUT");
      texCoord->setType(type);

      // Statement allows for casting of different types which
      // eliminates vector truncation problems.
      String statement = String::ToString("   @ = (%s)@;\r\n", type);
      meta->addStatement(new GenOp(statement, texCoord, inTex));
   }

   return texCoord;
}

//------------------------------------------------------------------
// Node Texture Feature.
//------------------------------------------------------------------

void NodeTextureFeatureHLSL::processVert(Vector<ShaderComponent*>& componentList, const MaterialFeatureData& fd)
{
   MultiLine* meta = new MultiLine;
   getNodeOutTexCoord(params->uvName,
      params->uvType,
      meta,
      componentList);
   output = meta;
}

void NodeTextureFeatureHLSL::processPix(Vector<ShaderComponent*>& componentList, const MaterialFeatureData& fd)
{
   // This is a very simplified version of the diffuseMapFeat
   // since we dont know what this texture is to be used for we
   // just sample it. Atlasing is handled in another node.
   // TODO: Add support for texture types at the moment this just
   // does a Texture2D but we require arrays 3d and cube.

   // grab connector texcoord register
   Var* inTex = getInTexCoord(params->uvName, params->uvType, componentList);

   Var* texSampler = new Var;
   texSampler->setName(params->samplerName + "_sampler");
   texSampler->setType("SamplerState");
   texSampler->uniform = true;
   texSampler->sampler = true;
   texSampler->constNum = Var::getTexUnitNum();

   Var* texTexture = new Var;
   texTexture->setName(params->samplerName + "_tex");
   texTexture->setType("Texture2D");
   texTexture->uniform = true;
   texTexture->texture = true;
   texTexture->constNum = texSampler->constNum;

   // Set the var to the samplerName so other nodes
   // know what it is.
   Var* texColor = new Var;
   texColor->setName(params->samplerName);
   texColor->setType("float4");
   LangElement* colorDecl = new DecOp(texColor);

   MultiLine* meta = new MultiLine;

   // handle mips, this is set from the parameters at the moment.
   // TODO: params should include mip bias as an input.
   if (params->hasMips)
   {
      const bool is_sm3 = (GFX->getPixelShaderVersion() > 2.0f);
      if (is_sm3)
      {
         // Figure out the mip level. (note only 1 should exist)
         Var* mipLod = (Var*)LangElement::find("mipLod");
         if (!mipLod)
         {
            mipLod = new Var;
            mipLod->setName("mipLoad");
            mipLod->setType("float");
            LangElement* mipLodDecl = new DecOp(mipLod);

            meta->addStatement(new GenOp("   // Calculate mip level.\r\n"));
            meta->addStatement(new GenOp("   float2 _dx = ddx(@);\r\n", inTex));
            meta->addStatement(new GenOp("   float2 _dy = ddy(@);\r\n", inTex));
            meta->addStatement(new GenOp("   @ = 0.5 * log2(max(dot(_dx, _dx), dot(_dy, _dy)));\r\n", mipLodDecl));
         }

         meta->addStatement(new GenOp("   @ = @.SampleLevel(@, @, @);\r\n", colorDecl, texTexture, texSampler, inTex, mipLod));
      }
   }
   else
   {
      meta->addStatement(new GenOp("   @ = @.Sample(@, @);\r\n", colorDecl, texTexture, texSampler, inTex));
   }
   output = meta;
}

ShaderFeature::Resources NodeTextureFeatureHLSL::getResources(const MaterialFeatureData& fd)
{
   Resources res;
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}
