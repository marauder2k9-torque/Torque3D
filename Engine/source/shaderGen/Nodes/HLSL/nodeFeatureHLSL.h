#pragma once
#ifndef _NODEFEATURE_HLSL_H_

#ifndef _SHADERFEATURE_H_
#include "shaderGen/shaderFeature.h"
#endif

#ifndef _SHADERGEN_HLSL_SHADERFEATUREHLSL_H_
#include "shaderGen/HLSL/shaderFeatureHLSL.h"
#endif

// node feature types contains the structs that features use as arguments
// in their constructor. Structs should be the same for both glsl and hlsl variants.
// what each side does with those arugments is up to the feature.
#ifndef _NODEFEATURETYPES_H_
#include "shaderGen/Nodes/nodeFeatureTypes.h"
#endif

// forward def just in case
struct LangElement;
struct MaterialFeatureData;
struct RenderPassData;

/// <summary>
/// This class is mostly here to just initialize and register the node features
/// using module init. Similar method used for the terrain feature. 
/// </summary>
class NodeFeatureHLSL : public ShaderFeatureHLSL
{
public:
   NodeFeatureHLSL();
   Var* getNodeOutTexCoord(const char* name, const char* type, MultiLine* meta, Vector<ShaderComponent*>& componentList);
};

/// <summary>
/// Texture sampler feature for the node system. This feature just samples a texture
/// does not set its target or blend operation, those should be other nodes.
/// </summary>
class NodeTextureFeatureHLSL : public NodeFeatureHLSL
{
private:
   /// <summary>
   /// Parameters that this feature can use to change the shadergen output.
   /// </summary>
   NodeTextureFeatureParams* params;
public:

   /// <summary>
   /// Default constructor for register feature.
   /// </summary>
   NodeTextureFeatureHLSL()
   {
      params = new NodeTextureFeatureParams();
   }

   /// <summary>
   /// Constructor that takes params as an argument, for node features
   /// this should be the only constructor that gets called.
   /// </summary>
   /// <param name="inParams">The NodeTextureFeatureParams for this instance.</param>
   NodeTextureFeatureHLSL(NodeTextureFeatureParams* inParams)
   {
      params = inParams;
   }

   /// <summary>
   /// Create function called by shaderGen to create this instance.
   /// </summary>
   /// <param name="args">The arguments set for this feature.</param>
   /// <returns>ShaderFeature pointer for this instance.</returns>
   static ShaderFeature* createFunction(void* args)
   {
      NodeTextureFeatureParams* params = static_cast<NodeTextureFeatureParams*>(args);


      return new NodeTextureFeatureHLSL(params);
   }

   void processVert( Vector<ShaderComponent*>& componentList,
                     const MaterialFeatureData& fd) override;

   void processPix(  Vector<ShaderComponent*>& componentList,
                     const MaterialFeatureData& fd) override;

   ShaderFeature::Resources getResources(const MaterialFeatureData& fd) override;

   // Sets textures and texture flags for current pass
   void setTexData(  Material::StageData& stageDat,
                     const MaterialFeatureData& fd,
                     RenderPassData& passData,
                     U32& texIndex) override;

   String getName() override
   {
      return "Sampler Node:" + params->samplerName;
   }
};

#endif // !_NODEFEATURE_HLSL_H_
