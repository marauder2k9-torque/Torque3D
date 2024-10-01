//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "gfx/gfxShader.h"

#include "shaderGen/conditionerFeature.h"
#include "core/volume.h"
#include "console/engineAPI.h"

#include <SPIRV/GlslangToSpv.h>
#include <spirv-cross/spirv_cross.hpp>
#include <spirv-cross/spirv_glsl.hpp>
#include <spirv-cross/spirv_hlsl.hpp>
#include <glslang/MachineIndependent/Initialize.h>

Vector<GFXShaderMacro> GFXShader::smGlobalMacros;
bool GFXShader::smLogErrors = true;
bool GFXShader::smLogWarnings = true;

// CustomIncluder to handle #include directives, with support for embedded includes
class CustomIncluder : public glslang::TShader::Includer
{
private:
   Vector<String> mLastPath;

public:
   CustomIncluder(const String& path) {
      mLastPath.clear();
      mLastPath.push_back(path);
   }

   // Handles system-style includes (<file.h>)
   IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override {
      return resolveInclude(headerName);
   }

   // Handles local-style includes ("file.h")
   IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override {
      return resolveInclude(headerName);
   }

   // Release the memory of the included file's content
   void releaseInclude(IncludeResult* result) override {
      if (result) {
         delete[] result->headerData;  // Free file content memory
         delete result;  // Free the IncludeResult itself
         mLastPath.pop_back();
      }
   }

private:
   IncludeResult* resolveInclude(const String& pFileName)
   {
      using namespace Torque;
      // First try making the path relative to the parent.
      Torque::Path path = Torque::Path::Join(mLastPath.last(), '/', pFileName);
      path = Torque::Path::CompressPath(path);

      void* data;
      U32 size;

      if (!Torque::FS::ReadFile(path, data, size, true))
      {
         // Ok... now try using the path as is.
         path = String(pFileName);
         path = Torque::Path::CompressPath(path);

         if (!Torque::FS::ReadFile(path, data, size, true))
         {
            AssertISV(false, avar("Failed to open include '%s'.", pFileName.c_str()));
            return nullptr;
         }
      }

      const char* file = (const char*)data;

      // If the data was of zero size then we cannot recurse
      if (size > 0)
         mLastPath.push_back(path.getRootAndPath());

      return new IncludeResult(pFileName.c_str(), file, size, nullptr);
   }
};

static TBuiltInResource defaultResources() {
   TBuiltInResource resource;
   resource.limits.generalVariableIndexing = 1;
   resource.limits.generalUniformIndexing = 1;
   resource.maxGeometryOutputVertices = 256;
   resource.maxGeometryTotalOutputComponents = 1024;
   resource.maxGeometryInputComponents = 64;
   resource.maxGeometryOutputComponents = 128;
   return resource;
}

GFXShader::GFXShader()
   :  mPixVersion( 0.0f ),
      mReloadKey( 0 ),
      mInstancingFormat( NULL ),
      mStages(0)
{
   mGenSPV = false;
}

GFXShader::~GFXShader()
{
   if (!mVertexBaseFile.isEmpty() && mVertexBaseFile.getExtension().equal("hlsl"))
      Torque::FS::RemoveChangeNotification( mVertexBaseFile, this, &GFXShader::_onFileChanged );

   if (!mPixelBaseFile.isEmpty() && mPixelBaseFile.getExtension().equal("hlsl"))
      Torque::FS::RemoveChangeNotification( mPixelBaseFile, this, &GFXShader::_onFileChanged );

   if (!mGeometryBaseFile.isEmpty() && mGeometryBaseFile.getExtension().equal("hlsl"))
      Torque::FS::RemoveChangeNotification( mGeometryBaseFile, this, &GFXShader::_onFileChanged );

   SAFE_DELETE(mInstancingFormat);
}

#ifndef TORQUE_OPENGL
bool GFXShader::init(   const Torque::Path &vertFile,
                        const Torque::Path &pixFile,
                        F32 pixVersion,
                        const Vector<GFXShaderMacro> &macros )
{
   Vector<String> samplerNames;
   return init( vertFile, pixFile, pixVersion, macros, samplerNames );
}
#endif

/// <summary>
/// Checks the incoming files for whether a spv recompile is required
/// this will be true if spv file is older than the shaderFile.
/// </summary>
/// <param name="shaderFile">The incoming design file (hlsl/glsl).</param>
/// <returns>True if recompile is required, otherwise false.</returns>
bool GFXShader::requireSPIRVRecompile(Torque::Path& shaderFile)
{
   // if our incoming file is spv, we dont need to recompile.
   if (shaderFile.getExtension().equal("spv"))
      return false;

   Torque::Path spv = shaderFile;
   spv.setExtension("spv");

   // check if spv existst, if newer we dont need to recompile spv.
   if (Torque::FS::IsFile(spv))
   {
      S32 ret = checkFile(spv, shaderFile);
      if (ret == 1)
         return false;
   }

   return true;
}

bool GFXShader::requireAPIRecompile(Torque::Path& apiFile, Torque::Path& spvFile)
{
   if (!Torque::FS::IsFile(spvFile))
   {
      // no spv file to compile something bad happened
      return false;
   }

   // check if apifile exists and is newer than the spv file.
   if (Torque::FS::IsFile(apiFile))
   {
      S32 ret = checkFile(apiFile, spvFile);
      if (ret == 1)
         return false;
   }

   return true;
}

/// <summary>
/// Checks all files set to this shader to see if spv recompile is required.
/// </summary>
/// <returns>True if any file requires recompile, otherwise false.</returns>
bool GFXShader::checkSpirvRecompile()
{
   if (!mVertexBaseFile.isEmpty())
   {
      if (requireSPIRVRecompile(mVertexBaseFile))
         return true;
   }

   if (!mPixelBaseFile.isEmpty())
   {
      if (requireSPIRVRecompile(mPixelBaseFile))
         return true;
   }

   if (!mGeometryBaseFile.isEmpty())
   {
      if (requireSPIRVRecompile(mGeometryBaseFile))
         return true;
   }

   return false;
}

bool GFXShader::checkApiRecompile()
{
   Torque::Path spvPath, apiPath;
   String ext;

   switch (GFX->getAdapterType())
   {
   case Direct3D11:
      ext = "hlsl";
      break;
   case OpenGL:
      ext = "glsl";
      break;
   default:
      return false;
      break;
   }

   if (!mVertexBaseFile.isEmpty())
   {
      spvPath = mVertexBaseFile;
      spvPath.setExtension("spv");

      apiPath = "data:/" + ext + "/" + mVertexBaseFile.getFileName();
      apiPath.setExtension(ext);

      if (requireAPIRecompile(apiPath, spvPath))
         return true;
   }

   if (!mPixelBaseFile.isEmpty())
   {
      spvPath = mPixelBaseFile;
      spvPath.setExtension("spv");

      apiPath = "data:/" + mPixelBaseFile.getFileName();
      apiPath.setExtension(ext);

      if (requireAPIRecompile(apiPath, spvPath))
         return true;
   }

   if (!mGeometryBaseFile.isEmpty())
   {
      spvPath = mGeometryBaseFile;
      spvPath.setExtension("spv");

      apiPath = "data:/" + mGeometryBaseFile.getFileName();
      apiPath.setExtension(ext);

      if (requireAPIRecompile(apiPath, spvPath))
         return true;
   }

   return false;
}

/// <summary>
/// Converts all files to spirv.
/// </summary>
/// <returns>True if successful, otherwise false.</returns>
bool GFXShader::convertToSpirv()
{
   // initialize glslang for program compilation
   if (!glslang::InitializeProcess())
   {
      Con::printf("Failed to initialize glslang.");
      return false;
   }

   glslang::TProgram* program(new glslang::TProgram);

   // check each stage and parse for glslang
   if (mStages & (U32)GFXShaderStage::VERTEX_SHADER)
   {
      glslang::TShader* vert(new glslang::TShader(EShLanguage::EShLangVertex));
      if (!parseShader(mVertexBaseFile, EShLanguage::EShLangVertex, *vert))
      {
         return false;
      }
      else
      {
         program->addShader(vert);
      }
   }

   if (mStages & (U32)GFXShaderStage::PIXEL_SHADER)
   {
      glslang::TShader* pixel(new glslang::TShader(EShLanguage::EShLangFragment));
      if (!parseShader(mPixelBaseFile, EShLanguage::EShLangFragment, *pixel))
      {
         return false;
      }
      else
      {
         program->addShader(pixel);
      }
   }

   if (mStages & (U32)GFXShaderStage::GEOMETRY_SHADER)
   {
      glslang::TShader* geo(new glslang::TShader(EShLanguage::EShLangGeometry));
      if (!parseShader(mGeometryBaseFile, EShLanguage::EShLangGeometry, *geo))
      {
         return false;
      }
      else
      {
         program->addShader(geo);
      }
   }

   // Link the program after all shaders have been added
   if (!program->link(static_cast<EShMessages>(EShMessages::EShMsgReadHlsl | EShMessages::EShMsgSpvRules | EShMessages::EShMsgVulkanRules)))
   {
      Con::warnf("%s", program->getInfoLog());
   }

   // now recompile each stage to spv
   for (int stage = 0; stage < EShLangCount; ++stage)
   {
      EShLanguage langStage = (EShLanguage)stage;
      if (program->getIntermediate((EShLanguage)langStage))
      {
         // for now we only do pix,vert and geo.
         // TODO: add a method to map this to a string for the
         // filename in spv.
         Torque::Path path;
         switch (langStage)
         {
         case EShLanguage::EShLangVertex:
            path = mVertexBaseFile;
            break;
         case EShLanguage::EShLangFragment:
            path = mPixelBaseFile;
            break;
         case EShLanguage::EShLangGeometry:
            path = mGeometryBaseFile;
            break;
         default:
            break;
         }

         path.setExtension("spv");
         std::vector<uint32_t> spirv;
         glslang::GlslangToSpv(*program->getIntermediate(langStage), spirv);
         if (!saveSPIRV(spirv, path))
         {
            Con::printf("Failed to save SPIR-V output file: %s", path.getFullFileName().c_str());
            return false;
         }
      }
   }

   glslang::FinalizeProcess();

   return true;
}

bool GFXShader::compileAPIFile(const Torque::Path& inputBaseFile)
{
   // at this point we should definitely have a spv file. just load it.
   Torque::Path spv;
   
   U32 stream_sz = 0;
   FileStream shaderStream;
   // check each stage and parse for glslang
   spv = inputBaseFile;
   spv.setExtension("spv");

   if (!shaderStream.open(spv, Torque::FS::File::Read))
   {
      Con::printf("Failed to load SPV: %s", spv.getFullFileName().c_str());
   }

   stream_sz = shaderStream.getStreamSize();

   std::vector<uint32_t> spirvStream(stream_sz);

   shaderStream.read(stream_sz, reinterpret_cast<char*>(spirvStream.data()));

   if (GFX->getAdapterType() == Direct3D11)
   {
      Torque::Path apiPath;
      String ext("hlsl");
      apiPath = "data:/" + ext + "/" + spv.getFileName();
      apiPath.setExtension(ext);

      if(!saveAPI(compileSPIRVtoHLSL(spirvStream), apiPath))
      {
         Con::printf("Failed to compile API: %s", apiPath.getFullFileName().c_str());
      }
   }
   else if (GFX->getAdapterType() == OpenGL)
   {

   }

   return false;
}

String GFXShader::compileSPIRVtoHLSL(const std::vector<uint32_t>& spirv)
{
   spirv_cross::CompilerHLSL hlslCompiler(reinterpret_cast<const uint32_t*>(spirv.data()), spirv.size() / 4);

   // Set HLSL options (optional)
   spirv_cross::CompilerHLSL::Options options;
   options.shader_model = 51;  // Set the HLSL shader model (e.g., 50 for SM5.0)
   options.point_size_compat = true;  // Enable point size compatibility for geometry shaders
   options.point_coord_compat = true; // Ensure point coord compatibility for geometry shaders
   options.use_entry_point_name = true;

   hlslCompiler.set_hlsl_options(options);

   // Perform conversion
   String hlslCode = String::ToString(hlslCompiler.compile().c_str());
   return hlslCode;
}

/// <summary>
/// Sets up our shaders for cross api compilation using glslang as intermediate.
/// </summary>
/// <returns>True if successful, otherwise false.</returns>
bool GFXShader::_setupShaders()
{
   bool recompSPV = checkSpirvRecompile();
   
   // if any file needs recompiled we need to recompile the whole program.
   if (recompSPV)
   {
      if (!convertToSpirv())
      {
         //return false;
      }
   }

   bool recompAPI = checkApiRecompile();

   if (recompAPI)
   {
      // check each stage and parse for glslang
      if (mStages & (U32)GFXShaderStage::VERTEX_SHADER)
      {
         if (!compileAPIFile(mVertexBaseFile))
         {

         }
      }

      if (mStages & (U32)GFXShaderStage::PIXEL_SHADER)
      {
         if (!compileAPIFile(mPixelBaseFile))
         {

         }
      }

      if (mStages & (U32)GFXShaderStage::GEOMETRY_SHADER)
      {
         if (!compileAPIFile(mGeometryBaseFile))
         {

         }
      }
   }

   return true;
}

bool GFXShader::init(   F32 pixVersion,
                        const Vector<GFXShaderMacro> &macros,
                        const Vector<String> &samplerNames,
                        bool genSPV,
                        GFXVertexFormat *instanceFormat)
{
   // early out.
   if (mVertexBaseFile.isEmpty() && mPixelBaseFile.isEmpty() && mGeometryBaseFile.isEmpty())
   {
      Con::errorf("Shader files empty, please call setShaderStageFile from shaderData");
      return false;
   }

   // Take care of instancing
   if (instanceFormat)
   {
      mInstancingFormat = new GFXVertexFormat;
      mInstancingFormat->copy(*instanceFormat);
   }

   // Store the inputs for use in reloading.
   mPixVersion = pixVersion;
   mMacros = macros;
   mSamplerNamesOrdered = samplerNames;

   // Before we compile the shader make sure the
   // conditioner features have been updated.
   ConditionerFeature::updateConditioners();

   // call before init, this will setup spv and our files.
   // for now set these to the input files.
   mVertexFile = mVertexBaseFile;
   mPixelFile = mPixelBaseFile;
   mGeometryFile = mGeometryBaseFile;

   mGenSPV = genSPV;
   if (genSPV)
   {
      if (!_setupShaders())
         return false;
   }

   // Now do the real initialization.
   if ( !_init() )
      return false;

   _updateDesc();

   // Add file change notifications for reloads.
   // note only for the hlsl design file, if this is spv this is release and we dont listen for reloads.
   if(!mVertexBaseFile.isEmpty() && mVertexBaseFile.getExtension().equal("hlsl"))
      Torque::FS::AddChangeNotification(mVertexBaseFile, this, &GFXShader::_onFileChanged );
   if(!mPixelBaseFile.isEmpty() && mPixelBaseFile.getExtension().equal("hlsl"))
      Torque::FS::AddChangeNotification(mPixelBaseFile, this, &GFXShader::_onFileChanged );
   if(!mGeometryBaseFile.isEmpty() && mGeometryBaseFile.getExtension().equal("hlsl"))
      Torque::FS::AddChangeNotification(mGeometryBaseFile, this, &GFXShader::_onFileChanged);

   return true;
}

bool GFXShader::reload()
{
   // Before we compile the shader make sure the
   // conditioner features have been updated.
   ConditionerFeature::updateConditioners();

   mReloadKey++;

   // call before init, this will setup spv and our files.
   if (mGenSPV)
   {
      if (!_setupShaders())
         return false;
   }

   // Init does the work.
   bool success = _init();
   if ( success )
      _updateDesc();

   // Let anything that cares know that
   // this shader has reloaded
   mReloadSignal.trigger();

   return success;
}

void GFXShader::_updateDesc()
{
   mDescription = String::ToString( "Files: %s, %s Pix Version: %0.2f\nMacros: ",
      mVertexFile.getFullPath().c_str(), mPixelFile.getFullPath().c_str(), mPixVersion );

   GFXShaderMacro::stringize( smGlobalMacros, &mDescription );
   GFXShaderMacro::stringize( mMacros, &mDescription );
}

/// <summary>
/// Checks fileA against fileB
/// </summary>
/// <param name="fileA">The fileA.</param>
/// <param name="fileB">the fileB</param>
/// <returns>1 if FileA is newer. -1 if FileB is newer. 0 if error.</returns>
S32 GFXShader::checkFile(Torque::Path& fileA, Torque::Path& fileB)
{
   Torque::FS::FileNodeRef nodeA = Torque::FS::GetFileNode(fileA);
   Torque::FS::FileNodeRef nodeB = Torque::FS::GetFileNode(fileB);

   // Can't do anything if either file doesn't exist
   if (!nodeA || !nodeB)
   {
      return 0;
   }

   Torque::FS::FileNode::Attributes fileAAttributes;
   Torque::FS::FileNode::Attributes fileBAttributes;

   // If retrieval of attributes fails, we can't compare   
   if (!nodeA->getAttributes(&fileAAttributes) || !nodeB->getAttributes(&fileBAttributes))
   {
      return 0;
   }

   if (fileAAttributes.mtime > fileBAttributes.mtime)
   {
      return 1;
   }
   else if (fileAAttributes.mtime < fileBAttributes.mtime)
   {
      return -1;
   }
   return 0;
}

/// <summary>
/// Parses incoming shader (hlsl/glsl) with glslang.
/// </summary>
/// <param name="shaderFile">The incoming shader file to parse.</param>
/// <param name="stage">The shader stage to parse.</param>
/// <param name="shader">The pointer to the shader to be created by this parse.</param>
/// <returns>True if the parse is successful, else returns false.</returns>
bool GFXShader::parseShader(const String& shaderFile, EShLanguage stage, glslang::TShader& shader)
{
   Torque::Path curFile = shaderFile;

   // TODO: eventually we will want to support glsl -> spv -> hlsl, for now though hlsl -> spv is the best route.
   glslang::EShSource sourceLang = curFile.getExtension().equal("hlsl") ? glslang::EShSourceHlsl : glslang::EShSourceGlsl;

   FileStream shaderStream;
   if (!shaderStream.open(curFile, Torque::FS::File::Read))
   {
      Con::printf("Failed to load Shader: %s", curFile.getFullFileName().c_str());
   }

   U32 stream_sz = shaderStream.getStreamSize();
   char* buffer = new char[stream_sz + 1];
   shaderStream.read(stream_sz, buffer);
   buffer[stream_sz] = '\0';

   const char* str = buffer;

   // stringify our macros
   String macros;
   GFXShaderMacro::stringize(smGlobalMacros, &macros, true);
   GFXShaderMacro::stringize(mMacros, &macros, true);
   macros.replace(";", "\n");

   shader.setStrings(&str, 1);
   // add macros to the shader code.
   shader.setEntryPoint("main");
   shader.setPreamble(macros.c_str());
   shader.setEnvInput(sourceLang, stage, glslang::EShClientVulkan, 100);
   shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
   shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

   TBuiltInResource res = defaultResources();

   glslang::SpvVersion spv_version{};
   spv_version.spv = SPV_VERSION;
   spv_version.vulkanRelaxed = true; // be maximally permissive, allowing certain OpenGL in Vulkan

   glslang::TBuiltIns builtins{};
   builtins.initialize(460, ECompatibilityProfile, spv_version);
   builtins.initialize(res, 460, ECompatibilityProfile, spv_version, stage);

   CustomIncluder includer(curFile.getRootAndPath());

   if (!shader.parse(&res, 100, false, static_cast<EShMessages>(EShMessages::EShMsgReadHlsl | EShMessages::EShMsgSpvRules | EShMessages::EShMsgVulkanRules), includer))
   {
      Con::errorf("Failed to Parse Shader: %s", curFile.getFullFileName().c_str());
      Con::errorf("%s", shader.getInfoLog());
      Con::errorf("%s", shader.getInfoDebugLog());
      return false;
   }

   delete[] buffer;
   return true;
}

/// <summary>
/// Saves the incoming code to a spv file.
/// </summary>
/// <param name="spirv">The incoming spirv code.</param>
/// <param name="output">The file path to output to.</param>
/// <returns>True if successful, else returns false.</returns>
bool GFXShader::saveSPIRV(const std::vector<uint32_t>& spirv, Torque::Path& output)
{
   FileStream stream;
   if (!stream.open(output, Torque::FS::File::Write))
   {
      Con::printf("Failed to open SPIR-V output file for writing: %s", output.getFullFileName().c_str());
      return false;
   }

   stream.write(spirv.size() * sizeof(uint32_t), spirv.data());
   stream.close();

   return true;
}

bool GFXShader::saveAPI(const String& apiCode, Torque::Path& output)
{
   FileStream stream;
   if (!stream.open(output, Torque::FS::File::Write))
   {
      Con::printf("Failed to open SPIR-V output file for writing: %s", output.getFullFileName().c_str());
      return false;
   }
   stream.writeText(apiCode.c_str());
   stream.close();

   return true;
}

void GFXShader::addGlobalMacro( const String &name, const String &value )
{
   // Check to see if we already have this macro.
   Vector<GFXShaderMacro>::iterator iter = smGlobalMacros.begin();
   for ( ; iter != smGlobalMacros.end(); iter++ )
   {
      if ( iter->name == name )
      {
         if ( iter->value != value )
            iter->value = value;
         return;
      }
   }

   // Add a new macro.
   smGlobalMacros.increment();
   smGlobalMacros.last().name = name;
   smGlobalMacros.last().value = value;
}

bool GFXShader::removeGlobalMacro( const String &name )
{
   Vector<GFXShaderMacro>::iterator iter = smGlobalMacros.begin();
   for ( ; iter != smGlobalMacros.end(); iter++ )
   {
      if ( iter->name == name )
      {
         smGlobalMacros.erase( iter );
         return true;
      }
   }

   return false;
}

void GFXShader::setShaderStageFile(const GFXShaderStage stage, const Torque::Path& filePath)
{
   switch (stage)
   {
   case GFXShaderStage::VERTEX_SHADER:
      mVertexBaseFile = filePath;
      if ((mStages & GFXShaderStage::VERTEX_SHADER) == 0)
         mStages |= (U32)GFXShaderStage::VERTEX_SHADER;
      break;
   case GFXShaderStage::PIXEL_SHADER:
      mPixelBaseFile = filePath;
      if ((mStages & GFXShaderStage::PIXEL_SHADER) == 0)
         mStages |= (U32)GFXShaderStage::PIXEL_SHADER;
      break;
   case GFXShaderStage::GEOMETRY_SHADER:
      mGeometryBaseFile = filePath;
      if ((mStages & GFXShaderStage::GEOMETRY_SHADER) == 0)
         mStages |= (U32)GFXShaderStage::GEOMETRY_SHADER;
      break;
   default:
      break;
   }
}

void GFXShader::_unlinkBuffer( GFXShaderConstBuffer *buf )
{
   Vector<GFXShaderConstBuffer*>::iterator iter = mActiveBuffers.begin();
   for ( ; iter != mActiveBuffers.end(); iter++ )
   {
      if ( *iter == buf )
      {
         mActiveBuffers.erase_fast( iter );
         return;
      }
   }

   AssertFatal( false, "GFXShader::_unlinkBuffer - buffer was not found?" );
}


DefineEngineFunction( addGlobalShaderMacro, void,
   ( const char *name, const char *value ), ( nullAsType<const char*>() ),
   "Adds a global shader macro which will be merged with the script defined "
   "macros on every shader.  The macro will replace the value of an existing "
   "macro of the same name.  For the new macro to take effect all the shaders "
   "in the system need to be reloaded.\n"
   "@see resetLightManager, removeGlobalShaderMacro\n"
   "@ingroup Rendering\n" )
{
   GFXShader::addGlobalMacro( name, value );
}

DefineEngineFunction( removeGlobalShaderMacro, void, ( const char *name ),,
   "Removes an existing global macro by name.\n"
   "@see addGlobalShaderMacro\n"
   "@ingroup Rendering\n" )
{
   GFXShader::removeGlobalMacro( name );
}
