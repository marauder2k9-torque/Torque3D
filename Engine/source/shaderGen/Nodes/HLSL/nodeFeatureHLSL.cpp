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
   String outTexName = String::ToString("out_%s", name);
   Var* texCoord = (Var*)LangElement::find(outTexName);

   if (!texCoord)
   {
      Var* inTex = getVertTexCoord(name);
      AssertFatal(inTex, "ShaderFeatureHLSL::getOutTexCoord - Unknown vertex input coord!");

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

   AssertFatal(String::compare(type, (const char*)texCoord->type) == 0,
      "ShaderFeatureHLSL::getOutTexCoord - Type mismatch!");

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
   meta->addStatement(new GenOp("@ = @.Sample(@, @);\r\n", colorDecl, texTexture, texSampler, inTex));
   output = meta;
}

ShaderFeature::Resources NodeTextureFeatureHLSL::getResources(const MaterialFeatureData& fd)
{
   Resources res;
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}
