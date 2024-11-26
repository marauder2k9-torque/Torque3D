#pragma once
#ifndef _GFXSTRUCTS_H_
#define _GFXSTRUCTS_H_

#ifndef _GFXENUMS_H_
#include "gfx2/gfxEnums.h"
#endif

#ifndef _CRC_H_
#include "core/crc.h"
#endif

#ifndef _COLOR_H_
#include "core/color.h"
#endif

//-----------------------------------------------------
// STATE BLOCK DESCRIPTORS:
// Render State Desc
//-----------------------------------------------------

/// <summary>
/// GFXRenderStateDesc describes a render state. These will be applied to the
/// rasterizer when the draw call is executed.
/// </summary>
struct GFXRenderStateDesc {

   // Color Blending
   /// <summary>
   /// Enable Color blending default: false
   /// </summary>
   bool blendEnable                       = false;

   /// <summary>
   /// Color Source blending factor default: One
   /// </summary>
   GFXBlendFactor srcBlend                = GFXBlendFactor::One;

   /// <summary>
   /// Color Destination blending factor default: Zero
   /// </summary>
   GFXBlendFactor destBlend               = GFXBlendFactor::Zero;

   /// <summary>
   /// Color blending operation default: Add
   /// </summary>
   GFXBlendOp blendOp                     = GFXBlendOp::Add;

   // Seperate Alpha Blending
   /// <summary>
   /// Enable Separate Alpha blending default: false
   /// </summary>
   bool separateAlphaBlendEnable          = false;

   /// <summary>
   /// Alpha Source blending factor default: One
   /// </summary>
   GFXBlendFactor separateAlphaBlendSrc   = GFXBlendFactor::One;

   /// <summary>
   /// Alpha Destination blending factor default: Zero
   /// </summary>
   GFXBlendFactor separateAlphaBlendDest  = GFXBlendFactor::Zero;

   /// <summary>
   /// Alpha blending operation default: Add
   /// </summary>
   GFXBlendOp separateAlphaBlendOp        = GFXBlendOp::Add;

   // Color Writes
   /// <summary>
   /// Enable Color writing default: true
   /// </summary>
   bool colorWriteEnable                  = true;

   /// <summary>
   /// Enable Color Red writing default: true
   /// </summary>
   bool colorWriteRed                     = true;

   /// <summary>
   /// Enable Color Blue writing default: true
   /// </summary>
   bool colorWriteBlue                    = true;

   /// <summary>
   /// Enable Color Green writing default: true
   /// </summary>
   bool colorWriteGreen                   = true;

   /// <summary>
   /// Enable Color Alpha writing default: true
   /// </summary>
   bool colorWriteAlpha                   = true;

   // Depth
   /// <summary>
   /// Enable Depth default: true
   /// </summary>
   bool depthEnable                       = true;

   /// <summary>
   /// Enable Depth Writing default: true
   /// </summary>
   bool depthWriteEnable                  = true;

   /// <summary>
   /// Depth comparison function default: GreaterEqual
   /// </summary>
   GFXComparisonFunc depthFunc            = GFXComparisonFunc::GreaterEqual;

   /// <summary>
   /// Depth bias default: 0.0f
   /// </summary>
   F32 depthBias                          = 0.0f;

   /// <summary>
   /// Depth slope bias default: 0.0f
   /// </summary>
   F32 depthSlopeBias                     = 0.0f;

   // Stencil settings for front face
   /// <summary>
   /// Enable Stencil default: false
   /// </summary>
   bool stencilEnable                     = false;

   /// <summary>
   /// Stencil front face fail operation default: Keep
   /// </summary>
   GFXStencilOp stencilFailOp             = GFXStencilOp::Keep;

   /// <summary>
   /// Stencil front face depth fail operation default: Keep
   /// </summary>
   GFXStencilOp stencilDepthFailOp        = GFXStencilOp::Keep;

   /// <summary>
   /// Stencil front face pass operation default: Keep
   /// </summary>
   GFXStencilOp stencilPassOp             = GFXStencilOp::Keep;

   /// <summary>
   /// Stencil front face comparison function default: Always
   /// </summary>
   GFXComparisonFunc stencilFunc          = GFXComparisonFunc::Always;

   // Stencil settings for back face
   /// <summary>
   /// Stencil back face fail operation default: Keep
   /// </summary>
   GFXStencilOp stencilFailOpBack         = GFXStencilOp::Keep;

   /// <summary>
   /// Stencil back face depth fail operation default: Keep
   /// </summary>
   GFXStencilOp stencilDepthFailOpBack    = GFXStencilOp::Keep;

   /// <summary>
   /// Stencil back face pass operation default: Keep
   /// </summary>
   GFXStencilOp stencilPassOpBack         = GFXStencilOp::Keep;

   /// <summary>
   /// Stencil back face comparison function default: Always
   /// </summary>
   GFXComparisonFunc stencilFuncBack      = GFXComparisonFunc::Always;

   // Stencil mask
   U32 stencilRef                         = 0;
   U32 stencilMask                        = 0xFFFFFFFF;
   U32 stencilWriteMask                   = 0xFFFFFFFF;

   // Rasterizer state
   /// <summary>
   /// Culling defined default: false
   /// </summary>
   bool cullDefined                       = false;

   /// <summary>
   /// Culling mode default: Back
   /// </summary>
   GFXCullMode cullMode                   = GFXCullMode::Back;

   /// <summary>
   /// Fill mode default: Solid
   /// </summary>
   GFXFillMode fillMode                   = GFXFillMode::Solid;
   bool scissorEnable                     = false;
   bool vertexColorEnable                 = false;

   /// <summary>
   /// Creates a hash using all the variables in the struct. This
   /// is then used for comparisons later.
   /// </summary>
   /// <returns> <c>U32</c> hash of all the values.</returns>
   U32 getHashValue() const {
      U32 crc = 0;

      // Blending
      crc = CRC::calculateCRC(&blendEnable, sizeof(blendEnable), crc);
      crc = CRC::calculateCRC(&srcBlend, sizeof(srcBlend), crc);
      crc = CRC::calculateCRC(&destBlend, sizeof(destBlend), crc);
      crc = CRC::calculateCRC(&blendOp, sizeof(blendOp), crc);

      // Separate Alpha Blending
      crc = CRC::calculateCRC(&separateAlphaBlendEnable, sizeof(separateAlphaBlendEnable), crc);
      crc = CRC::calculateCRC(&separateAlphaBlendSrc, sizeof(separateAlphaBlendSrc), crc);
      crc = CRC::calculateCRC(&separateAlphaBlendDest, sizeof(separateAlphaBlendDest), crc);
      crc = CRC::calculateCRC(&separateAlphaBlendOp, sizeof(separateAlphaBlendOp), crc);

      // Color Writes
      crc = CRC::calculateCRC(&colorWriteEnable, sizeof(colorWriteEnable), crc);
      crc = CRC::calculateCRC(&colorWriteRed, sizeof(colorWriteRed), crc);
      crc = CRC::calculateCRC(&colorWriteBlue, sizeof(colorWriteBlue), crc);
      crc = CRC::calculateCRC(&colorWriteGreen, sizeof(colorWriteGreen), crc);
      crc = CRC::calculateCRC(&colorWriteAlpha, sizeof(colorWriteAlpha), crc);

      // Depth
      crc = CRC::calculateCRC(&depthEnable, sizeof(depthEnable), crc);
      crc = CRC::calculateCRC(&depthWriteEnable, sizeof(depthWriteEnable), crc);
      crc = CRC::calculateCRC(&depthFunc, sizeof(depthFunc), crc);
      crc = CRC::calculateCRC(&depthBias, sizeof(depthBias), crc);
      crc = CRC::calculateCRC(&depthSlopeBias, sizeof(depthSlopeBias), crc);

      // Stencil Front Face
      crc = CRC::calculateCRC(&stencilEnable, sizeof(stencilEnable), crc);
      crc = CRC::calculateCRC(&stencilFailOp, sizeof(stencilFailOp), crc);
      crc = CRC::calculateCRC(&stencilDepthFailOp, sizeof(stencilDepthFailOp), crc);
      crc = CRC::calculateCRC(&stencilPassOp, sizeof(stencilPassOp), crc);
      crc = CRC::calculateCRC(&stencilFunc, sizeof(stencilFunc), crc);

      // Stencil Back Face
      crc = CRC::calculateCRC(&stencilFailOpBack, sizeof(stencilFailOpBack), crc);
      crc = CRC::calculateCRC(&stencilDepthFailOpBack, sizeof(stencilDepthFailOpBack), crc);
      crc = CRC::calculateCRC(&stencilPassOpBack, sizeof(stencilPassOpBack), crc);
      crc = CRC::calculateCRC(&stencilFuncBack, sizeof(stencilFuncBack), crc);

      // Rasterizer State
      crc = CRC::calculateCRC(&cullDefined, sizeof(cullDefined), crc);
      crc = CRC::calculateCRC(&cullMode, sizeof(cullMode), crc);
      crc = CRC::calculateCRC(&fillMode, sizeof(fillMode), crc);
      crc = CRC::calculateCRC(&scissorEnable, sizeof(scissorEnable), crc);
      crc = CRC::calculateCRC(&vertexColorEnable, sizeof(vertexColorEnable), crc);

      return crc;
   }
};

//-----------------------------------------------------
// Sampler State Desc
//-----------------------------------------------------

/// <summary>
/// GFXSamplerStateDesc describes a sampler state used for textures.
/// </summary>
struct GFXSamplerStateDesc {

   /// <summary>
   /// Mip filter for the sampler default: Linear
   /// </summary>
   GFXFilterMode mipFilter = GFXFilterMode::Linear;

   /// <summary>
   /// Min filter for the sampler default: Linear
   /// </summary>
   GFXFilterMode minFilter = GFXFilterMode::Linear;

   /// <summary>
   /// Mag filter for the sampler default: Linear
   /// </summary>
   GFXFilterMode magFilter = GFXFilterMode::Linear;

   /// <summary>
   /// Texture Address mode for U dimension in the sampler default: Wrap
   /// </summary>
   GFXAddressMode addressU = GFXAddressMode::Wrap;

   /// <summary>
   /// Texture Address mode for V dimension in the sampler default: Wrap
   /// </summary>
   GFXAddressMode addressV = GFXAddressMode::Wrap;

   /// <summary>
   /// Texture Address mode for W dimension in the sampler default: Wrap
   /// </summary>
   GFXAddressMode addressW = GFXAddressMode::Wrap;

   /// <summary>
   /// Texture mip LOD bias default: 0.0f
   /// </summary>
   F32 mipLODBias = 0.0f;

   /// <summary>
   /// Texture max anisotropy default: 1
   /// </summary>
   U32 maxAnisotropy = 1;

   /// <summary>
   /// Texture comparison function default: Always
   /// </summary>
   GFXComparisonFunc comparisonFunc = GFXComparisonFunc::Always;

   /// <summary>
   /// Border color default: White
   /// </summary>
   LinearColorF borderColor = LinearColorF::WHITE;

   /// <summary>
   /// Creates a hash using all the variables in the struct. This
   /// is then used for comparisons later.
   /// </summary>
   /// <returns>U32 hash with all the values.</returns>
   U32 getHashValue() const {
      U32 crc = 0;

      // Filtering modes
      crc = CRC::calculateCRC(&mipFilter, sizeof(mipFilter), crc);
      crc = CRC::calculateCRC(&minFilter, sizeof(minFilter), crc);
      crc = CRC::calculateCRC(&magFilter, sizeof(magFilter), crc);

      // Address modes
      crc = CRC::calculateCRC(&addressU, sizeof(addressU), crc);
      crc = CRC::calculateCRC(&addressV, sizeof(addressV), crc);
      crc = CRC::calculateCRC(&addressW, sizeof(addressW), crc);

      // LOD bias and anisotropy
      crc = CRC::calculateCRC(&mipLODBias, sizeof(mipLODBias), crc);
      crc = CRC::calculateCRC(&maxAnisotropy, sizeof(maxAnisotropy), crc);

      // Comparison function
      crc = CRC::calculateCRC(&comparisonFunc, sizeof(comparisonFunc), crc);

      // Border color
      crc = CRC::calculateCRC(&borderColor, sizeof(borderColor), crc);

      return crc;
   }
};
#endif // !_GFXSTRUCTS_H_
