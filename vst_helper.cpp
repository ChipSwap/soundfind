#include "vst_helper.h"

#include <fstream> // for saving programs
#include <list> // for reading from file

typedef AEffect* (*PluginEntryProc)(audioMasterCallback audioMaster);

// We use this, because it looks like technically the parameter string lengths
// are limited to 8, but at least one implementation (Synth1) uses more
// than this length and ends up crashing stuff.
static const int kVstExtMaxParamStrLen = kVstMaxParamStrLen<<5;

VstIntPtr VSTCALLBACK HostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
  VstIntPtr result = 0;

  switch (opcode)
  {
  case audioMasterVersion :
    result = kVstVersion;
    break;
  }

  return result;
}

void VSTHelper::Init(const std::string& vst_path, float sample_rate)
{
  // load the library
  module_ = LoadLibrary(vst_path.c_str());

  // get the function pointer
  PluginEntryProc addr_ = reinterpret_cast<PluginEntryProc>(GetProcAddress(module_, "VSTPluginMain"));
  if (addr_ == NULL)
    addr_ = reinterpret_cast<PluginEntryProc>(GetProcAddress(module_, "main"));
  if (addr_ == NULL) exit(EXIT_FAILURE);

  // create the effect
  effect_ = addr_(HostCallback);

  // open
  VstIntPtr ret = effect_->dispatcher(effect_, effOpen, 0, 0, 0, 0);
  if (ret != 0) exit(EXIT_FAILURE);

  // are programs in chunks?
  in_chunks_ = (effect_->flags & effFlagsProgramChunks) != 0;

 // // get parameter info
 // params_.clear();
	//for (VstInt32 i = 0; i < effect_->numParams; i++)
	//{
 //   char param_info[kVstExtMaxParamStrLen];

 //   // our param to stick on
 //   Param p;

 //   // name
	//	effect_->dispatcher(effect_, effGetParamName, i, 0, param_info, 0);
 //   p.name_ = param_info;

 //   // label
	//	effect_->dispatcher(effect_, effGetParamLabel, i, 0, param_info, 0);
 //   p.label_ = param_info;

 //   // display
	//	effect_->dispatcher(effect_, effGetParamDisplay, i, 0, param_info, 0);
 //   p.display_ = param_info;

 //   // value
	//	p.value = effect_->getParameter(effect_, i);

 //   // isn't necessarily supported
 //   VstParameterProperties props = { 0 };
 //   VstIntPtr ret = effect_->dispatcher(effect_, effGetParameterProperties, i, 0, &props, 0);

	//	//printf("Param %03d: %s [%s %s] (normalized = %f)\n", i, paramName, paramDisplay, paramLabel, value);
 //   
 //   // add our param
 //   params_.push_back(p);
	//}

  //effect_->setParameter(effect_, 0, 1.f);

  // set sample rate
  ret = effect_->dispatcher(effect_, effSetSampleRate, 0, 0, 0, sample_rate);
  if (ret != 0) exit(EXIT_FAILURE);

  //// set block size to be the asio buffer size
  //ret = effect_->dispatcher(effect_, effSetBlockSize, 0, buffer_size, 0, 0);
  //if (ret != 0) exit(EXIT_FAILURE);


  // resume
  //float    sample_rate = 44100;
  //VstInt32 block_size  = static_cast<VstInt32>(10*sample_rate);

  //effect_->dispatcher(effect_, effSetSampleRate, 0, NULL, NULL, sample_rate);

  // set up outputs
  //Sound<float> output;
  //output.data.resize(2);
  //output.data[0] = output.data[1] = std::vector<float>(block_size, 0);

  //float* output_pass[2];
  //output_pass[0] = &(*output.data[0].begin());
  //output_pass[1] = &(*output.data[1].begin());

  // call once to clear buffers
  //effect_->processReplacing(effect_, NULL, output_pass, block_size);

  // try to call it

  //output.ReadWav("output.wav");

  // write our output

  // infinite loop!
  //for (;;)
  {
  }

  //output.WriteWav("output.wav");
}

void VSTHelper::GenerateOutput(VstInt32 block_size, float* output[])
{
  // set the block size
  effect_->dispatcher(effect_, effSetBlockSize, 0, block_size, NULL, 0.f);

  // create a midi event
  VstMidiEvent midi_event = { 0 };
  midi_event.type = kVstMidiType;
  midi_event.byteSize = sizeof(VstMidiEvent);
  
  // set the midi data
  midi_event.midiData[0] = (char)0x90; // channel 1 note on
  midi_event.midiData[1] = 60;   // C4
  midi_event.midiData[2] = 127;  // max velocity
    
  midi_event.flags = kVstMidiEventIsRealtime;

  VstEvents ev = { 0 };
  ev.numEvents = 1;
  ev.events[0] = reinterpret_cast<VstEvent*>(&midi_event);

  // resume
	effect_->dispatcher(effect_, effMainsChanged, 0, 1, 0, 0);

  // process the midi event
  effect_->dispatcher(effect_, effProcessEvents, 0, 0, &ev, 0.f);

  // get the output
  effect_->processReplacing(effect_, NULL, output, block_size);

  // suspend
	effect_->dispatcher(effect_, effMainsChanged, 0, 0, 0, 0);
}

void VSTHelper::LoadProgram(const std::string& path)
{
  std::ifstream program(path, std::ios::binary);

  // get length of file:
  program.seekg(0, std::ios::end);
  int length = static_cast<int>(program.tellg());
  program.seekg(0, std::ios::beg);

  char* data = new char[length];
  program.read(data, length);
  
  // 0 for bank, 1 for program
  int ret = effect_->dispatcher(effect_, effSetChunk, 1, length, data, 0);

  delete[] data;

  //void* chunk;
  //int byte_size = effect_->dispatcher(effect_, effGetChunk, 1, 0, &chunk, 0);

  program.close();
}

void VSTHelper::SaveCurrentProgram(const std::string& path)
{
  void* chunk;

  // 0 for bank, 1 for program
  int byte_size = effect_->dispatcher(effect_, effGetChunk, 1, 0, &chunk, 0);

  std::ofstream program(path, std::ios::binary);
  program.write(reinterpret_cast<char*>(chunk), byte_size);
  program.close();
}

void VSTHelper::SetParam(unsigned int index, float value)
{
  if (index >= static_cast<unsigned int>(effect_->numParams))
    return;

  effect_->setParameter(effect_, index, value);
}

float VSTHelper::GetParam(unsigned int index)
{
  if (index >= static_cast<unsigned int>(effect_->numParams))
    return 0.f;
  
  return effect_->getParameter(effect_, index);
}
