#include "platform/platform.h"
#include "shaderGen/Nodes/nodeFeatureTypes.h"

#include "materials/materialFeatureTypes.h"

// all node features should be implemented with misc grouping and -1 as the order
// this way when we come to using the feature set no order change should happen.
// ordering of nodes is handled by the shader node editor.
ImplementFeatureType(SNF_, MFG_Misc, -1, false);
ImplementFeatureType(SNF_TextureFeature, MFG_Misc, -1, false);
