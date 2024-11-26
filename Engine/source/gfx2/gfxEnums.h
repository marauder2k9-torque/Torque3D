#pragma once
#ifndef _GFXENUMS_H_
#define _GFXENUMS_H_

//-----------------------------------------------------
// RENDER STATE SPECIFIC:
// BLEND OPERATION ENUMS
//-----------------------------------------------------
enum class GFXBlendOp
{
   FIRST = 0,
   Add,                // Source + Destination
   Subtract,           // Source - Destination
   ReverseSubtract,    // Destination - Source
   Min,                // Min(Source, Destination)
   Max,                // Max(Source, Destination)
   COUNT
};

//-----------------------------------------------------
// BLEND FACTOR ENUMS
//-----------------------------------------------------
enum class GFXBlendFactor
{
   FIRST = 0,
   Zero,               // (0, 0, 0, 0)
   One,                // (1, 1, 1, 1)
   SrcColor,           // Source Color
   OneMinusSrcColor,   // (1, 1, 1, 1) - Source Color
   DstColor,           // Destination Color
   OneMinusDstColor,   // (1, 1, 1, 1) - Destination Color
   SrcAlpha,           // Source Alpha
   OneMinusSrcAlpha,   // (1, 1, 1, 1) - Source Alpha
   DstAlpha,           // Destination Alpha
   OneMinusDstAlpha,   // (1, 1, 1, 1) - Destination Alpha
   ConstantColor,      // Constant Color
   OneMinusConstantColor, // (1, 1, 1, 1) - Constant Color
   ConstantAlpha,      // Constant Alpha
   OneMinusConstantAlpha, // (1, 1, 1, 1) - Constant Alpha
   SrcAlphaSaturate,   // Min(Source Alpha, 1 - Destination Alpha)
   COUNT
};

//-----------------------------------------------------
// COMPARE FUNCTION ENUMS
//-----------------------------------------------------
enum class GFXComparisonFunc
{
   FIRST = 0,
   Never,              // Never passes
   Less,               // Passes if Source < Destination
   Equal,              // Passes if Source == Destination
   LessEqual,          // Passes if Source <= Destination
   Greater,            // Passes if Source > Destination
   NotEqual,           // Passes if Source != Destination
   GreaterEqual,       // Passes if Source >= Destination
   Always,             // Always passes
   COUNT
};

//-----------------------------------------------------
// CULL MODE ENUMS
//-----------------------------------------------------
enum class GFXCullMode
{
   FIRST = 0,
   None,               // No culling
   Front,              // Cull front faces (Clockwise)
   Back,               // Cull back faces (Counter Clockwise)
   FrontAndBack,       // Cull both front and back faces
   COUNT
};

//-----------------------------------------------------
// FILL MODE ENUMS
//-----------------------------------------------------
enum class GFXFillMode {
   FIRST = 0,
   Solid,              // Solid fill
   Wireframe,          // Wireframe (outline) fill
   COUNT
};

//-----------------------------------------------------
// STENCIL OPERATIONS ENUMS
//-----------------------------------------------------
enum class GFXStencilOp {
   FIRST = 0,
   Keep,               // Keep the current stencil value
   Zero,               // Set the stencil value to zero
   Replace,            // Replace the stencil value with the reference value
   IncrementSaturate,  // Increment the stencil value and clamp to the maximum value
   DecrementSaturate,  // Decrement the stencil value and clamp to the minimum value
   Invert,             // Invert the stencil value (bitwise NOT)
   Increment,          // Increment the stencil value (no clamp)
   Decrement,          // Decrement the stencil value (no clamp)
   COUNT
};

//-----------------------------------------------------
// TEXTURE/SAMPLER SPECIFIC:
// FILTER MODE ENUMS
//-----------------------------------------------------
enum class GFXFilterMode {
   FIRST = 0,
   Nearest,            // Nearest neighbor filtering
   Linear,             // Bilinear filtering
   Anisotropic,        // Anisotropic filtering
   COUNT
};

//-----------------------------------------------------
// ADDRESS MODE ENUMS
//-----------------------------------------------------
enum class GFXAddressMode {
   FIRST = 0,
   Wrap,               // Wrap the texture (repeat)
   Clamp,              // Clamp to the edge of the texture
   Mirror,             // Mirror the texture (i.e., flip every other repetition)
   Border,             // Clamp to a border color
   COUNT
};

//-----------------------------------------------------
// TEXTURE FORMAT ENUMS
//-----------------------------------------------------
enum class GFXTextureFormat {
   FIRST = 0,
   RGBA8,               // 8-bit RGBA format
   BGRA8,               // 8-bit BGRA format
   BGRX8,               // 8-bit BGRX format (without alpha channel)
   R8,                  // 8-bit red channel
   RG8,                 // 8-bit red-green channels
   RGB8,                // 8-bit RGB format
   RGBA16F,             // 16-bit float RGBA format
   RGBA32F,             // 32-bit float RGBA format
   D16,                 // 16-bit depth format
   D32,                 // 32-bit depth format
   D24S8,               // 24-bit depth, 8-bit stencil format
   DepthStencil,        // Generic depth-stencil format
   RGB10A2,             // 10-bit RGB + 2-bit alpha format
   RGBA4,               // 4-bit RGBA format
   R32F,                // 32-bit float red channel
   RG32F,               // 32-bit float RG format
   R11G11B10F,          // 11-bit float RGBA format (high dynamic range)
   // sRGB formats
   SRGB8,               // 8-bit sRGB format (gamma corrected)
   SRGB8_A8,            // 8-bit sRGB format with alpha (gamma corrected)
   SRGB16F,             // 16-bit sRGB float format
   SRGB32F,             // 32-bit sRGB float format
   COUNT
};

//-----------------------------------------------------
// SHADER SPECIFIC:
// SHADER STAGE ENUMS
//-----------------------------------------------------
enum class GFXShaderStage : U32 {
   Unknown  = 0x00,     // Unknown stage
   Vertex   = 0x01,     // Vertex Shader
   Pixel    = 0x02,     // Pixel/Fragment Shader
   Geometry = 0x04,     // Geometry Shader
   Domain   = 0x08,     // Domain Shader (Tessellation evaluation)
   Hull     = 0x10,     // Hull Shader (Tessellation control)
   Compute  = 0x20      // Compute shader
};

//-----------------------------------------------------
// SHADER CONST TYPE ENUMS
//-----------------------------------------------------
enum class GFXShaderConstType {
   Float,               // Single float
   Float2,              // vec2 (float2)
   Float3,              // vec3 (float3)
   Float4,              // vec4 (float4)
   Double,              // Single double
   Double2,             // dvec2 (double2)
   Double3,             // dvec3 (double3)
   Double4,             // dvec4 (double4)
   Matrix3x3,           // 3x3 matrix
   Matrix3x4,           // 3x4 matrix
   Matrix4x3,           // 4x3 matrix
   Matrix4x4,           // 4x4 matrix
   Int,                 // Single integer
   Int2,                // ivec2 (int2)
   Int3,                // ivec3 (int3)
   Int4,                // ivec4 (int4)
   UInt,                // Single unsigned integer
   UInt2,               // uvec2 (uint2)
   UInt3,               // uvec3 (uint3)
   UInt4,               // uvec4 (uint4)
   Bool,                // Single boolean
   Bool2,               // bvec2
   Bool3,               // bvec3
   Bool4,               // bvec4
   Struct,              // Struct (custom types from shaders)
   Unknown              // Fallback for unsupported or unknown types
};

enum class GFXResourceType {
   // Regular textures

    // 2D texture
    // HLSL: Texture2D<T>
    // GLSL: sampler2D
   Texture2D,

   // Array of 2D textures
   // HLSL: Texture2DArray<T>
   // GLSL: sampler2DArray
   Texture2DArray,

   // 3D texture
   // HLSL: Texture3D<T>
   // GLSL: sampler3D
   Texture3D,

   // Cube map texture
   // HLSL: TextureCube<T>
   // GLSL: samplerCube
   TextureCube,

   // Array of cube maps
   // HLSL: TextureCubeArray<T>
   // GLSL: samplerCubeArray
   TextureCubeArray,

   // 1D texture
   // HLSL: Texture1D<T>
   // GLSL: sampler1D
   Texture1D,

   // Array of 1D textures
   // HLSL: Texture1DArray<T>
   // GLSL: sampler1DArray
   Texture1DArray,

   // Read-Write textures

   // Read-write 2D texture (Pixel and Compute ONLY)
   // HLSL: RWTexture2D<T>
   // GLSL: image2D
   RWTexture2D,

   // Read-write array of 2D textures (Pixel and Compute ONLY)
   // HLSL: RWTexture2DArray<T>
   // GLSL: image2DArray
   RWTexture2DArray,

   // Read-write 3D texture (Pixel and Compute ONLY)
   // HLSL: RWTexture3D<T>
   // GLSL: image3D
   RWTexture3D,

   // Read-write cube map texture (Pixel and Compute ONLY)
   // HLSL: RWTextureCube<T>
   // GLSL: imageCube
   RWTextureCube,

   // Read-write array of cube maps (Pixel and Compute ONLY)
   // HLSL: RWTextureCubeArray<T>
   // GLSL: imageCubeArray
   RWTextureCubeArray,

   // Read-write 1D texture (Pixel and Compute ONLY)
   // HLSL: RWTexture1D<T>
   // GLSL: image1D
   RWTexture1D,

   // Read-write array of 1D textures (Pixel and Compute ONLY)
   // HLSL: RWTexture1DArray<T>
   // GLSL: image1DArray
   RWTexture1DArray,

   // Other resource types

   // Sampler state (sample can only be invoked in pixel shaders)
   // HLSL: SamplerState, SamplerComparisonState
   // GLSL: Integrated with sampler2D, samplerCube, etc.
   Sampler,

   // Read-write structured buffer (Pixel and Compute ONLY)
   // HLSL: RWStructuredBuffer<T>, AppendStructuredBuffer<T>, ConsumeStructuredBuffer<T>
   // GLSL: buffer block (SSBO - Shader Storage Buffer Object)
   RWStructuredBuffer,

   // Raw or byte-address buffer (Pixel and Compute ONLY)
   // HLSL: ByteAddressBuffer, StructuredBuffer<T>
   // GLSL: buffer block (SSBO - Shader Storage Buffer Object)
   Buffer,

   // Uniform buffer
   // HLSL: cbuffer or ConstantBuffer<T>
   // GLSL: uniform block
   ConstantBuffer,

   Unknown
};
#endif
