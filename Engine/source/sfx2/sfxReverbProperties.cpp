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

#include "sfx2/sfxSystem.h"
#include "sim/netConnection.h"
#include "core/stream/bitStream.h"
#include "core/module.h"

IMPLEMENT_CO_DATABLOCK_V1(SFXReverbProperties);

ConsoleDocClass(SFXReverbProperties,
   "@brief Description of a reverb environment.\n\n"

   "A reverb environment specifies how the audio mixer should render advanced environmental audio "
   "effects.  \n\n"

   "@see http://www.atc.creative.com/algorithms/eax20.pdf\n"
   "@see http://connect.creativelabs.com/developer/Gaming/Forms/AllItems.aspx\n"
   "@ref SFX_reverb\n"
   "@ingroup SFX\n"
);

void SFXReverbProperties::initPersistFields()
{
   addGroup("Reverb");
      addField("Density", TypeF32, Offset(flDensity, SFXReverbProperties), "Density of reverb environment.");
      addField("Diffusion", TypeF32, Offset(flDiffusion, SFXReverbProperties), "Environment diffusion.");
      addField("Gain", TypeF32, Offset(flGain, SFXReverbProperties), "Reverb Gain Level.");
      addField("GainHF", TypeF32, Offset(flGainHF, SFXReverbProperties), "Reverb Gain to high frequencies");
      addField("GainLF", TypeF32, Offset(flGainLF, SFXReverbProperties), "Reverb Gain to high frequencies");
      addField("DecayTime", TypeF32, Offset(flDecayTime, SFXReverbProperties), "Decay time for the reverb.");
      addField("DecayHFRatio", TypeF32, Offset(flDecayHFRatio, SFXReverbProperties), "High frequency decay time ratio.");
      addField("DecayLFRatio", TypeF32, Offset(flDecayLFRatio, SFXReverbProperties), "High frequency decay time ratio.");
      addField("reflectionDelay", TypeF32, Offset(flReflectionsDelay, SFXReverbProperties), "How long to delay reflections.");
      addField("lateReverbDelay", TypeF32, Offset(flLateReverbDelay, SFXReverbProperties), "Late reverb delay time.");
      addField("EchoTime", TypeF32, Offset(flEchoTime, SFXReverbProperties), "Reverb echo time.");
      addField("EchoDepth", TypeF32, Offset(flEchoDepth, SFXReverbProperties), "Reverb echo depth.");
      addField("ModTime", TypeF32, Offset(flModulationTime, SFXReverbProperties), "Reverb Modulation time.");
      addField("ModDepth", TypeF32, Offset(flModulationDepth, SFXReverbProperties), "Reverb Modulation time.");
      addField("airAbsorbtionGainHF", TypeF32, Offset(flAirAbsorptionGainHF, SFXReverbProperties), "High Frequency air absorbtion");
      addField("HFRef", TypeF32, Offset(flHFReference, SFXReverbProperties), "Reverb High Frequency Reference.");
      addField("LFRef", TypeF32, Offset(flLFReference, SFXReverbProperties), "Reverb Low Frequency Reference.");
      addField("roomRolloffFactor", TypeF32, Offset(flRoomRolloffFactor, SFXReverbProperties), "Rolloff factor for reverb.");
      addField("decayHFLimit", TypeS32, Offset(iDecayHFLimit, SFXReverbProperties), "High Frequency decay limit.");
   endGroup("Reverb");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool SFXReverbProperties::onAdd()
{
   if (!Parent::onAdd())
      return false;

   Sim::getSFXEnvironmentSet()->addObject(this);

   return true;
}

//-----------------------------------------------------------------------------

bool SFXReverbProperties::preload(bool server, String& errorStr)
{
   if (!Parent::preload(server, errorStr))
      return false;

   validate();

   return true;
}

//-----------------------------------------------------------------------------

void SFXReverbProperties::inspectPostApply()
{
   Parent::inspectPostApply();
   validate();
}

//-----------------------------------------------------------------------------

void SFXReverbProperties::packData(BitStream* stream)
{
   Parent::packData(stream);

   stream->write(flDensity);
   stream->write(flDiffusion);
   stream->write(flGain);
   stream->write(flGainHF);
   stream->write(flGainLF);
   stream->write(flDecayTime);
   stream->write(flDecayHFRatio);
   stream->write(flDecayLFRatio);
   stream->write(flReflectionsDelay);
   stream->write(flLateReverbDelay);
   stream->write(flEchoTime);
   stream->write(flEchoDepth);
   stream->write(flModulationTime);
   stream->write(flModulationDepth);
   stream->write(flAirAbsorptionGainHF);
   stream->write(flHFReference);
   stream->write(flLFReference);
   stream->write(flRoomRolloffFactor);
   stream->write(iDecayHFLimit);
}

//-----------------------------------------------------------------------------

void SFXReverbProperties::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   stream->read(&flDensity);
   stream->read(&flDiffusion);
   stream->read(&flGain);
   stream->read(&flGainHF);
   stream->read(&flGainLF);
   stream->read(&flDecayTime);
   stream->read(&flDecayHFRatio);
   stream->read(&flDecayLFRatio);
   stream->read(&flReflectionsDelay);
   stream->read(&flLateReverbDelay);
   stream->read(&flEchoTime);
   stream->read(&flEchoDepth);
   stream->read(&flModulationTime);
   stream->read(&flModulationDepth);
   stream->read(&flAirAbsorptionGainHF);
   stream->read(&flHFReference);
   stream->read(&flLFReference);
   stream->read(&flRoomRolloffFactor);
   stream->read(&iDecayHFLimit);
}
