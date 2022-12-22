#ifndef _GFXGLTEXTUREARRAY_H_
#define _GFXGLTEXTUREARRAY_H_

#include <glad/glad.h>

#include "gfx/gfxTextureArray.h"
#include "gfx/gfxTextureManager.h"

class GFXGLTextureArray : public GFXTextureArray
{
public:
   GFXGLTextureArray();

   ~GFXGLTextureArray() { Release(); };

   void init() override;
   void initDynamic(U32 texSize, GFXFormat faceFormat = GFXFormatR8G8B8A8, U32 mipLevels = 0, U32 arraySize = 0) override {}
   void setToTexUnit(U32 tuNum) override;

   void bind(U32 textureUnit) const;

   // GFXResource interface
   void zombify() override;
   void resurrect() override;
   void Release() override;

protected:
   void _setTexture(const GFXTexHandle& texture, U32 slot) override;

private:
   GLuint mTextureArray;
};


#endif
