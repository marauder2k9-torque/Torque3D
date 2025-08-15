#include "platform/platform.h"
#include "shaderGen/ShaderMap/shaderMap.h"
#include "shaderGen/ShaderMap/Nodes/textureSampleNode.h"

#include "shaderGen/langElement.h"
#include "shaderGen/shaderOp.h"
#include "shaderGen/shaderGenVars.h"
#include "gfx/gfxDevice.h"
#include "materials/matInstance.h"
#include "materials/processedMaterial.h"
#include "materials/materialFeatureTypes.h"
#include "core/util/autoPtr.h"

#include "lighting/advanced/advancedLightBinManager.h"
#include "ts/tsShape.h"

#include "shaderGen/shaderGen.h"

void TextureSampleFeature::processVert(Vector<ShaderComponent*>& componentList, const MaterialFeatureData& fd)
{
   MultiLine* meta = new MultiLine;
   output = meta;

   // Add new method for uv coord retrieval
}

void TextureSampleFeature::processPix(Vector<ShaderComponent*>& componentList, const MaterialFeatureData& fd)
{
   MultiLine* meta = new MultiLine;
   output = meta;

   // Need to add a new shaderop for texture sampling. For opengl texture and for dx samplerstate and sample.

}

ShaderFeature::Resources TextureSampleFeature::getResources(const MaterialFeatureData& fd)
{
   Resources res;
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}
