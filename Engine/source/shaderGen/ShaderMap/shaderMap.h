#ifndef _SHADERMAP_H_
#define _SHADERMAP_H_

#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif
#ifndef _SIMSET_H_
#include "console/simSet.h"
#endif
#ifndef _SHADERFEATURE_H_
#include "shaderGen/shaderFeature.h"
#endif
#ifndef _FEATURESET_H_
#include "shaderGen/featureSet.h"
#endif
#ifndef _MATERIALFEATUREDATA_H_
#include "materials/materialFeatureData.h"
#endif

class BaseShaderNodeFeature : public ShaderFeature
{
   // ShaderFeature
   Var* getVertTexCoord(const String& name) override;
   LangElement* setupTexSpaceMat(Vector<ShaderComponent*>& componentList, Var** texSpaceMat) override;
   LangElement* assignColor(LangElement* elem, Material::BlendOp blend, LangElement* lerpElem = NULL, ShaderFeature::OutputTarget outputTarget = ShaderFeature::DefaultTarget) override;
   LangElement* expandNormalMap(LangElement* sampleNormalOp, LangElement* normalDecl, LangElement* normalVar, const MaterialFeatureData& fd) override;
};


//-------------------------------------------------------------------------
// ShaderMap
//-------------------------------------------------------------------------

class ShaderNode : public SimObject
{
   typedef SimObject Parent;
   DECLARE_CONOBJECT(ShaderNode);

public:
   String varName;

   ShaderNode();
   static void initPersistFields();

   // Each node must implement this to add its linked feature to the FeatureSet
   virtual bool processNode(FeatureSet* outFeatures) = 0;

};

//-------------------------------------------------------------------------
// ShaderMap
//-------------------------------------------------------------------------

class ShaderMap : public SimGroup
{
   typedef SimGroup Parent;

public:
   ShaderMap();

   DECLARE_CONOBJECT(ShaderMap);

   bool onAdd() override;
   void addObject(SimObject*) override;
};


#endif // !_SHADERMAP_H_
