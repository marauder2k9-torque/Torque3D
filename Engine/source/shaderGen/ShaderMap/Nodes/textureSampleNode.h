#ifndef _TEXTURESAMPLENODE_H_
#define _TEXTURESAMPLENODE_H_

#ifndef _SHADERMAP_H_
#include "shaderGen/ShaderMap/shaderMap.h"
#endif

struct TextureFeatureParams
{
   String samplerName;
   String uvName;
   GFXShaderConstType uvType;

   TextureFeatureParams()
   {
      samplerName = "sampler";
      uvName = "texCoord";
      uvType = GFXSCT_Float2;
   }
};

class TextureSampleFeature : public BaseShaderNodeFeature
{
private:
   /// Parameters that this feature can use to change the shadergen output.
   TextureFeatureParams* params;
public:
   TextureSampleFeature()
   {
      params = new TextureFeatureParams();
   }

   TextureSampleFeature(TextureFeatureParams* inParams)
   {
      params = inParams;
   }

   static ShaderFeature* createFunction(void* args)
   {
      TextureFeatureParams* params = static_cast<TextureFeatureParams*>(args);
      return new TextureSampleFeature(params);
   }

   void processVert(Vector<ShaderComponent*>& componentList,
      const MaterialFeatureData& fd) override;

   void processPix(Vector<ShaderComponent*>& componentList,
      const MaterialFeatureData& fd) override;

   Resources getResources(const MaterialFeatureData& fd) override;

   String getName() override
   {
      return "Texture Sampler: " + params->samplerName;
   }
};

#endif
