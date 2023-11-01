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
#include "core/stream/fileStream.h"

Vector<GFXShaderMacro> GFXShader::smGlobalMacros;
bool GFXShader::smLogErrors = true;
bool GFXShader::smLogWarnings = true;


GFXShader::GFXShader()
   :  mPixVersion( 0.0f ),
      mReloadKey( 0 ),
      mInstancingFormat( NULL )
{
}

GFXShader::~GFXShader()
{
   Torque::FS::RemoveChangeNotification( mVertexFile, this, &GFXShader::_onFileChanged );
   Torque::FS::RemoveChangeNotification( mPixelFile, this, &GFXShader::_onFileChanged );

   SAFE_DELETE(mInstancingFormat);
}

bool GFXShader::init(   const Torque::Path &vertFile, 
                        const Torque::Path &pixFile, 
                        F32 pixVersion, 
                        const Vector<GFXShaderMacro> &macros,
                        const Vector<String> &samplerNames,
                        GFXVertexFormat *instanceFormat)
{
   // Take care of instancing
   if (instanceFormat)
   {
      mInstancingFormat = new GFXVertexFormat;
      mInstancingFormat->copy(*instanceFormat);
   }

   // Store the inputs for use in reloading.
   mVertexFile = vertFile;
   mPixelFile = pixFile;
   mPixVersion = pixVersion;
   mMacros = macros;
   mSamplerNamesOrdered = samplerNames;

   // Before we compile the shader make sure the
   // conditioner features have been updated.
   ConditionerFeature::updateConditioners();

   // Now do the real initialization.
   if ( !_init() )
      return false;

   _updateDesc();

   // Add file change notifications for reloads.
   Torque::FS::AddChangeNotification( mVertexFile, this, &GFXShader::_onFileChanged );
   Torque::FS::AddChangeNotification( mPixelFile, this, &GFXShader::_onFileChanged );

   return true;
}

bool GFXShader::reload()
{
   // Before we compile the shader make sure the
   // conditioner features have been updated.
   ConditionerFeature::updateConditioners();

   mReloadKey++;

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

//-------------------------------------------------------------------------------
// GFXSHADERPROGRAM :- this class holds all other shaders and handles consts.
//-------------------------------------------------------------------------------

GFXShaderProgram::GFXShaderProgram()
   : mLanguageVersion(0.0f),
   mReloadKey(0),
   mInstancingFormat(NULL)
{
   mHasVertex      = false;
   mHasPixel       = false;
   mHasGeometry    = false;
   mHasCompute     = false;
   mHasTessControl = false;
   mHasTessEval    = false;
}

GFXShaderProgram::GFXShaderProgram(F32 languageVersion, GFXVertexFormat* instancingFormat)
{
   mReloadKey = 0;
   mLanguageVersion = languageVersion;
   // Take care of instancing
   if (instancingFormat)
   {
      mInstancingFormat = new GFXVertexFormat;
      mInstancingFormat->copy(*instancingFormat);
   }

   mHasVertex      = false;
   mHasPixel       = false;
   mHasGeometry    = false;
   mHasCompute     = false;
   mHasTessControl = false;
   mHasTessEval    = false;
}

bool GFXShaderProgram::reload()
{
   // Before we compile the shader make sure the
   // conditioner features have been updated.
   ConditionerFeature::updateConditioners();

   mReloadKey++;

   // Init does the checks.
   bool success = false;
   if (mHasVertex)
   {

   }

   if (mHasPixel)
   {

   }

   if (mHasCompute)
   {

   }

   if (mHasGeometry)
   {

   }

   if (mHasTessControl)
   {

   }

   if (mHasTessEval)
   {

   }

   // Let anything that cares know that
   // this shader has reloaded
   mReloadSignal.trigger();

   return success;
}

bool GFXShaderProgram::init(const String shaderSource, ShaderTypes shaderType)
{
   switch (shaderType)
   {
   case ShaderTypes::VertexShader:
      if (!_initVertex())
      {
         return false;
      }
      else
      {
         mHasVertex = true;
         return true;
      }
      break;
   case ShaderTypes::PixelShader:
      if (!_initPixel())
      {
         return false;
      }
      else
      {
         mHasPixel = true;
         return true;
      }
      break;
   case ShaderTypes::ComputeShader:
      if (!_initCompute())
      {
         return false;
      }
      else
      {
         mHasCompute = true;
         return true;
      }
      break;
   case ShaderTypes::GeometryShader:
      if (!_initGeometry())
      {
         return false;
      }
      else
      {
         mHasGeometry = true;
         return true;
      }
      break;
   case ShaderTypes::TessellationControl:
      if (!_initTessControl())
      {
         return false;
      }
      else
      {
         mHasTessControl = true;
         return true;
      }
      break;
   case ShaderTypes::TessellationEvaluation:
      if (!_initTessEvaluation())
      {
         return false;
      }
      else
      {
         mHasTessEval = true;
         return true;
      }
      break;
   default:
      break;
   }
   return false;
}

bool GFXShaderProgram::init(const Torque::Path& shaderFile, ShaderTypes shaderType)
{

   FileStream f;
   if (!f.open(shaderFile.getFullPath(), Torque::FS::File::Read))
   {
      Con::errorf("GFXShaderProgram::init - couldn't read file %s", shaderFile.getFullPath().c_str());
      return false;
   }

   String data;
   f.read(f.getStreamSize(), &data);


   switch (shaderType)
   {
   case ShaderTypes::VertexShader:
      mVertexFile = shaderFile;
      if (!init(data, shaderType))
      {
         Con::errorf("GFXShaderProgram::init - could not compile Vertex shaer: %s", shaderFile.getFullPath().c_str());
         return false;
      }
      else
      {
         Torque::FS::AddChangeNotification(mVertexFile, this, &GFXShaderProgram::_onFileChanged);
         return true;
      }
      break;
   case ShaderTypes::PixelShader:
      mPixelFile = shaderFile;
      if (!init(data, shaderType))
      {
         Con::errorf("GFXShaderProgram::init - could not compile Pixel shaer: %s", shaderFile.getFullPath().c_str());
         return false;
      }
      else
      {
         Torque::FS::AddChangeNotification(mPixelFile, this, &GFXShaderProgram::_onFileChanged);
         return true;
      }
      break;
   case ShaderTypes::ComputeShader:
      mComputeFile = shaderFile;
      if (!init(data, shaderType))
      {
         Con::errorf("GFXShaderProgram::init - could not compile Compute shaer: %s", shaderFile.getFullPath().c_str());
         return false;
      }
      else
      {
         Torque::FS::AddChangeNotification(mComputeFile, this, &GFXShaderProgram::_onFileChanged);
         return true;
      }
      break;
   case ShaderTypes::GeometryShader:
      mGeometryFile = shaderFile;
      if (!init(data, shaderType))
      {
         Con::errorf("GFXShaderProgram::init - could not compile Geometry shaer: %s", shaderFile.getFullPath().c_str());
         return false;
      }
      else
      {
         Torque::FS::AddChangeNotification(mGeometryFile, this, &GFXShaderProgram::_onFileChanged);
         return true;
      }
      break;
   case ShaderTypes::TessellationControl:
      mTessControlFile = shaderFile;
      if (!init(data, shaderType))
      {
         Con::errorf("GFXShaderProgram::init - could not compile Tessellation Control shaer: %s", shaderFile.getFullPath().c_str());
         return false;
      }
      else
      {
         Torque::FS::AddChangeNotification(mTessControlFile, this, &GFXShaderProgram::_onFileChanged);
         return true;
      }
      break;
   case ShaderTypes::TessellationEvaluation:
      mTessEvalFile = shaderFile;
      if (!init(data, shaderType))
      {
         Con::errorf("GFXShaderProgram::init - could not compile Tessellation Evaluation shaer: %s", shaderFile.getFullPath().c_str());
         return false;
      }
      else
      {
         Torque::FS::AddChangeNotification(mTessEvalFile, this, &GFXShaderProgram::_onFileChanged);
         return true;
      }
      break;
   default:
      break;
   }
   return false;
}

