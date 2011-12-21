#define NOMINMAX // don't let windows screw up min()

#include "sound.h"
#include "asio_helper.h"
#include "vst_helper.h"

// this is static global so we can access it inside the audio callback
#include <algorithm> // for std::min
#include <fstream> // for if/ofstream
#include <Windows.h> // for mutexes

// globals
static CRITICAL_SECTION snd_crit_;

static Sound<float> snd_;        // our loaded sound
static Sound<float> best_match_; // our best match

static int buf_idx_ = 0;
static ASIOHelper asio_;
static VSTHelper vst_;

// put our good stuff here -- write out what we want when ASIO requests it
static void MyAudioCallback(int index)
{
  EnterCriticalSection(&snd_crit_);

  // choose which sound to play
  const Sound<float>& s = best_match_;

  // go through the buffers, only using outputs
  ASIOBufferInfo*  b = asio_.buffer_infos_  + asio_.input_channels_;
  ASIOChannelInfo* c = asio_.channel_infos_ + asio_.input_channels_;
  for (int i = 0; i < asio_.output_channels_; ++i, ++b, ++c)
  {
    switch (c->type)
    {
    case ASIOSTInt32LSB:
      {
        int32_t* int32_p = static_cast<int32_t*>(b->buffers[index]);
        for (int j = 0; j < asio_.buffer_size_; ++j)
        {
          // random data
          //float smp = static_cast<float>(rand()) / RAND_MAX;

          // read from sound file
          // take the corresponding channel index if it exists
          int channel = std::min(static_cast<unsigned int>(j), s.data_.size() - 1);
          float smp = s.data_[channel][(buf_idx_ + j) % s.data_[channel].size()];
          
          // place the output
          *int32_p++ = static_cast<int32_t>(smp * (smp >= 0 ? INT32_MAX : INT32_MIN));
        }
      }
      break;
    default:
      break;
    }
  }

  LeaveCriticalSection(&snd_crit_);

  // adjust the location within the sound for next output
  // assume that each channel contains the same amount of info
  buf_idx_ += asio_.buffer_size_;
  buf_idx_ %= s.data_[0].size();
}

template <class T>
float CalculateSonicDifference(Sound<T> snd, float* cmp[2])
{
  // SSD
  float diff = 0.f;
  for (unsigned int i = 0; i < std::min(snd.data_.size(), static_cast<unsigned int>(2)); ++i)
    for (unsigned int j = 0; j < snd.data_[i].size(); ++j)
      diff += (cmp[i][j] - snd.data_[i][j]) * (cmp[i][j] - snd.data_[i][j]);
  return diff;
}

int main(int argc, char* argv[])
{
  if (argc < 2)
    exit(EXIT_FAILURE);

  // make a critical section for sound playing
  InitializeCriticalSection(&snd_crit_);

  // need a synth to load
  std::string vst_path = argv[1];

  // read a wav file
  snd_.ReadWav("bwip.wav");

  // gets ASIO going and uses our callback as the audio callback
  // use the callback to play it
  asio_.Init(MyAudioCallback);

  // start up a VST given the path to the DLL
  vst_.Init(vst_path.c_str(), snd_.sample_rate_);

  // get our sound length
  unsigned int snd_len = snd_.data_[0].size();

  // get some output
  best_match_.data_ = std::vector< std::vector<float> >(2, std::vector<float>(snd_len, 0));
  
  // get pointers to generate the output
  float* output[2];
  output[0] = &(*best_match_.data_[0].begin());
  output[1] = &(*best_match_.data_[1].begin());

  vst_.GenerateOutput(snd_len, output);

  // check the difference
  float diff = CalculateSonicDifference(snd_, output);

  for (;;) {}

  DeleteCriticalSection(&snd_crit_);
}