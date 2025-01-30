
#pragma once
#ifndef _NODEFEATURETYPES_H_
#define _NODEFEATURETYPES_H_

#ifndef _FEATURETYPE_H_
#include "shaderGen/featureType.h"
#endif

#ifndef _SHADERFEATURE_H_
#include "shaderGen/shaderFeature.h"
#endif

//------------------------------------------------------------------
// Structs for features.
//------------------------------------------------------------------

struct NodeTextureFeatureParams
{
   String samplerName;
   String uvName;
   String uvType;
   NodeTextureFeatureParams()
   {
      uvName = "texCoord"; // torques default input uv.
   }
};

// All node features follow a simple naming convention S:Shader N:Node F:Feature
// to differentiate from MFT material feature type. 
// DeclareFeatureType(SNF_<featureName>);

DeclareFeatureType(SNF_TextureFeature);

//------------------------------------------------------------------
// Shader var for cross api (write shadergen once).
//------------------------------------------------------------------

#endif // !_NODEFEATURETYPES_H_
