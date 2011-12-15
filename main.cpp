#define NOMINMAX // don't let windows screw up min()

#include "sound.h"
#include "asio_helper.h"

// for LoadLibrary
#include <Windows.h>

// this is static global so we can access it inside the audio callback
#include <algorithm> // for std::min

// VST
#define WIN32 // to import the right types
#include "pluginterfaces/vst2.x/aeffectx.h"

typedef AEffect* (*PluginEntryProc)(audioMasterCallback audioMaster);

// globals
static Sound<float> snd_;
static ASIOHelper asio_;
static HMODULE module_;
static PluginEntryProc addr_;
static AEffect* effect_;

// put our good stuff here -- write out what we want when ASIO requests it
static void MyAudioCallback(int index)
{
  //// our location within the sound to play back
  //static int buf_idx = 0;

  //// go through the buffers, only using outputs
  //ASIOBufferInfo*  b = asio_.buffer_infos_  + asio_.input_channels_;
  //ASIOChannelInfo* c = asio_.channel_infos_ + asio_.input_channels_;
  //for (int i = 0; i < asio_.output_channels_; ++i, ++b, ++c)
  //{
  //  switch (c->type)
  //  {
  //  case ASIOSTInt32LSB:
  //    {
  //      int32_t* int32_p = static_cast<int32_t*>(b->buffers[index]);
  //      for (int j = 0; j < asio_.buffer_size_; ++j)
  //      {
  //        // random data
  //        float smp = static_cast<float>(rand()) / RAND_MAX;

  //        // read from sound file
  //        // take the corresponding channel index if it exists
  //        int channel = std::min(static_cast<unsigned int>(j), snd_.data.size() - 1);
  //        //float smp = snd_.data[channel][(buf_idx + j) % snd_.data[channel].size()];
  //        
  //        // place the output
  //        *int32_p++ = static_cast<int32_t>(smp * (smp >= 0 ? INT32_MAX : INT32_MIN));
  //      }
  //    }
  //    break;
  //  default:
  //    break;
  //  }
  //}

  //// adjust the location within the sound for next output
  //// assume that each channel contains the same amount of info
  //buf_idx += asio_.buffer_size_;
  ////buf_idx %= snd_.data[0].size();
}

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

int main(int argc, char* argv[])
{
  // read a wav file
  //snd_.ReadWav("C:/Users/clarkson/Desktop/test.wav");

  // gets ASIO going and uses our callback as the audio callback
  // use the callback to play it
  asio_.Init(MyAudioCallback);

  // load the library
  module_ = LoadLibrary("Synth1 VST.dll");

  // get the function pointer
  addr_ = reinterpret_cast<PluginEntryProc>(GetProcAddress(module_, "VSTPluginMain"));
  if (addr_ == NULL)
    addr_ = reinterpret_cast<PluginEntryProc>(GetProcAddress(module_, "main"));
  if (addr_ == NULL) exit(EXIT_FAILURE);

  // create the effect
  effect_ = addr_(HostCallback);

  // open
  VstIntPtr ret = effect_->dispatcher(effect_, effOpen, 0, 0, 0, 0);
  if (ret != 0) exit(EXIT_FAILURE);

  // set sample rate to be the asio sample rate
  ret = effect_->dispatcher(effect_, effSetSampleRate, 0, 0, 0, static_cast<float>(asio_.sample_rate_));
  if (ret != 0) exit(EXIT_FAILURE);

  // set block size to be the asio buffer size
  ret = effect_->dispatcher(effect_, effSetBlockSize, 0, asio_.buffer_size_, 0, 0);
  if (ret != 0) exit(EXIT_FAILURE);

  // iterate parameters
	for (VstInt32 i = 0; i < effect_->numParams; i++)
	{
		char paramName[256] = {0};
		char paramLabel[256] = {0};
		char paramDisplay[256] = {0};

		effect_->dispatcher(effect_, effGetParamName, i, 0, paramName, 0);
		effect_->dispatcher(effect_, effGetParamLabel, i, 0, paramLabel, 0);
		effect_->dispatcher(effect_, effGetParamDisplay, i, 0, paramDisplay, 0);
		float value = effect_->getParameter(effect_, i);

		printf("Param %03d: %s [%s %s] (normalized = %f)\n", i, paramName, paramDisplay, paramLabel, value);
	}
 
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
  float    sample_rate = 44100;
  VstInt32 block_size  = 10*sample_rate;

  effect_->dispatcher(effect_, effSetSampleRate, 0, NULL, NULL, sample_rate);
  effect_->dispatcher(effect_, effSetBlockSize, 0, block_size, NULL, 0.f);

	effect_->dispatcher(effect_, effMainsChanged, 0, 1, 0, 0);

  // set up outputs
  Sound<float> output;
  output.data.resize(2);
  output.data[0] = output.data[1] = std::vector<float>(block_size, 0);

  float* output_pass[2];
  output_pass[0] = &(*output.data[0].begin());
  output_pass[1] = &(*output.data[1].begin());

  // call once to clear buffers
  effect_->processReplacing(effect_, NULL, output_pass, block_size);

  // try to call it

  //output.ReadWav("output.wav");

  // write our output

  // infinite loop!
  //for (;;)
  {
    effect_->dispatcher(effect_, effProcessEvents, 0, 0, &ev, 0.f);
    effect_->processReplacing(effect_, NULL, output_pass, block_size);
  }

  output.WriteWav("output.wav");

  // suspend
	effect_->dispatcher(effect_, effMainsChanged, 0, 0, 0, 0);
}