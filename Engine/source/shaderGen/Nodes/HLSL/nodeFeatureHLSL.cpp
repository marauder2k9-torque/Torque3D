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
   getNodeOutTexCoord("texCoord",
      "float2",
      meta,
      componentList);
   output = meta;
}

void NodeTextureFeatureHLSL::processPix(Vector<ShaderComponent*>& componentList, const MaterialFeatureData& fd)
{
   // grab connector texcoord register
   Var* inTex = getInTexCoord("texCoord", "float2", componentList);


}

ShaderFeature::Resources NodeTextureFeatureHLSL::getResources(const MaterialFeatureData& fd)
{
   Resources res;
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}

void NodeTextureFeatureHLSL::setTexData(Material::StageData& stageDat, const MaterialFeatureData& fd, RenderPassData& passData, U32& texIndex)
{
}
