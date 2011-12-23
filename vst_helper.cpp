#include "vst_helper.h"

#include <fstream> // for saving programs
#include <list> // for reading from file

typedef AEffect* (*PluginEntryProc)(audioMasterCallback audioMaster);

// We use this, because it looks like technically the parameter string lengths
// are limited to 8, but at least one implementation (Synth1) uses more
// than this length and ends up crashing stuff.
static const int kVstExtMaxParamStrLen = kVstMaxParamStrLen<<5;
static VstTimeInfo time_info_;

// in case we're using arpeggiators, etc.
static int tempo_ = 120;
static int time_sig_num_ = 4;
static int time_sig_den_ = 4;

VstIntPtr VSTCALLBACK HostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
switch (opcode)
  {
  case audioMasterVersion:
    return kVstVersion;
  case audioMasterGetTime:
    
    // set basic data
    // everything else is 0
    time_info_.tempo              = tempo_;
    time_info_.timeSigNumerator   = time_sig_num_;
    time_info_.timeSigDenominator = time_sig_den_;

    time_info_.flags |= kVstTransportChanged;
    time_info_.flags |= kVstTransportPlaying;
    time_info_.flags |= kVstNanosValid;
    time_info_.flags |= kVstPpqPosValid;
    time_info_.flags |= kVstTempoValid;
    time_info_.flags |= kVstBarsValid;
    time_info_.flags |= kVstCyclePosValid;
    time_info_.flags |= kVstTimeSigValid;
    time_info_.flags |= kVstSmpteValid;
    time_info_.flags |= kVstClockValid;

    return (VstIntPtr)&time_info_;
  }

  return NULL;
}

void VSTHelper::Init(const std::string& vst_path)
{
  // load the library
  if (module_ == NULL)
    module_ = LoadLibrary(vst_path.c_str());

  // get the function pointer
  PluginEntryProc addr_ = reinterpret_cast<PluginEntryProc>(GetProcAddress(module_, "VSTPluginMain"));
  if (addr_ == NULL)
    addr_ = reinterpret_cast<PluginEntryProc>(GetProcAddress(module_, "main"));
  if (addr_ == NULL) exit(EXIT_FAILURE);

  // create the effect
  effect_ = addr_(HostCallback);

  //// get the version
  //int version = effect_->dispatcher(effect_, effGetVstVersion, 0, 0, 0, 0);

  // open
  VstIntPtr ret = effect_->dispatcher(effect_, effOpen, 0, 0, 0, 0);

  //// set to program 0
  //effect_->dispatcher(effect_, effSetProgram, 0, 0, 0, 0);

  //bool can_replacing = (effect_->flags & effFlagsCanReplacing) != 0;

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
  //{
  //}

  //output.WriteWav("output.wav");
}

void VSTHelper::Deinit()
{
  effect_->dispatcher(effect_, effClose, 0, 0, 0, 0);

  //FreeLibrary(module_);
}

void VSTHelper::GenerateOutput(float sample_rate, VstInt32 block_size, float* output[])
{  
  // our vst events
  VstEvents ev = { 0 };

  // set sample rate
  effect_->dispatcher(effect_, effSetSampleRate, 0, 0, 0, sample_rate);

  // set the block size
  effect_->dispatcher(effect_, effSetBlockSize, 0, block_size, NULL, 0.f);

  // create a note on
  VstMidiEvent note_on = { 0 };
  note_on.type         = kVstMidiType;
  note_on.byteSize     = sizeof(VstMidiEvent);
  note_on.midiData[0]  = (char)0x90; // channel 1 note on
  note_on.midiData[1]  = 60;   // C4
  note_on.midiData[2]  = 127;  // max velocity
    
  //note_on.flags = kVstMidiEventIsRealtime;

  // resume
	effect_->dispatcher(effect_, effMainsChanged, 0, 1, 0, 0);

  // process the ON events
  ev.numEvents = 1;
  ev.events[0] = reinterpret_cast<VstEvent*>(&note_on);
  effect_->dispatcher(effect_, effProcessEvents, 0, 0, &ev, 0);

  // run processReplacing
  memset(output[0], 0, sizeof(float) * block_size);
  memset(output[1], 0, sizeof(float) * block_size);
  effect_->processReplacing(effect_, output, output, block_size);

  // suspend
	effect_->dispatcher(effect_, effMainsChanged, 0, 0, 0, 0);
}

void VSTHelper::LoadProgram(const std::string& path)
{
  std::ifstream program(path, std::ios::binary);
  if (program.fail()) return;

  // get length of file:
  program.seekg(0, std::ios::end);
  int length = static_cast<int>(program.tellg());
  program.seekg(0, std::ios::beg);

  char* data = new char[length];
  program.read(data, length);
  
  // 0 for bank, 1 for program
  //effect_->dispatcher(effect_, effBeginSetProgram, 0, 0, 0, 0);
  int ret = effect_->dispatcher(effect_, effSetChunk, 1, length, data, 0);
  //effect_->dispatcher(effect_, effEndSetProgram, 0, 0, 0, 0);

  delete[] data;

  //void* chunk;
  //int byte_size = effect_->dispatcher(effect_, effGetChunk, 1, 0, &chunk, 0);

  program.close();
}

void VSTHelper::SaveCurrentProgram(const std::string& path)
{
  // are programs in chunks?
  if (!(effect_->flags & effFlagsProgramChunks))
    return;

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
