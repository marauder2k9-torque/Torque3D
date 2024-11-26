#pragma once
#ifndef _GFXSHADERBUFFEROBJECT_H_
#define _GFXSHADERBUFFEROBJECT_H_

#ifndef _GFXDEVICE_H_
#include "gfx2/gfxDevice.h"
#endif

/// <summary>
/// Abstract buffer object class, acts as the base class
/// data buffers for shaders. (primitve buffers and vertex buffers are separate)
/// </summary>
class GFXShaderBufferObject : public StrongRefBase
{

};
#endif
