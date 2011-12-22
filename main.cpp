#define NOMINMAX // don't let windows screw up min()
#include <Windows.h> // for critical sections

#include "sound.h"
#include "asio_helper.h"
#include "vst_helper.h"

#include <algorithm> // for std::min
#include <fstream> // for if/ofstream
#include <random> // for good random numbers
#include <iostream> // for cout

// globals
static CRITICAL_SECTION snd_crit_;

static Sound<float> snd_;                             // our loaded sound
static std::vector< std::vector<float> > best_match_; // our best match
static float best_diff_ = FLT_MAX;

static int buf_idx_ = 0;
static ASIOHelper asio_;
static VSTHelper vst_;

static std::tr1::mt19937 eng_; // our random engine
static std::tr1::uniform_real<float> unif_(0, 1); // a random number distribution between 0 and 1

// put our good stuff here -- write out what we want when ASIO requests it
static void MyAudioCallback(int index)
{
  EnterCriticalSection(&snd_crit_);

  const std::vector< std::vector<float> >& s = best_match_;

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
          int channel = std::min(static_cast<unsigned int>(j), s.size() - 1);
          float smp = s[channel][(buf_idx_ + j) % s[channel].size()];
          
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
  buf_idx_ %= s[0].size();
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
  std::vector< std::vector<float> > generated = std::vector< std::vector<float> >(2, std::vector<float>(snd_len, 0));
  
  // get pointers to generate the output
  float* output[2];
  output[0] = &(*generated[0].begin());
  output[1] = &(*generated[1].begin());

  // for all eternity...
  for (;;)
  {
    // make a sound given the current values
    vst_.GenerateOutput(snd_len, output);

    // check the difference
    float diff = CalculateSonicDifference(snd_, output);
    
    // copy into best match
    if (diff < best_diff_)
    {
      EnterCriticalSection(&snd_crit_);
      best_match_ = generated;
      LeaveCriticalSection(&snd_crit_);
      best_diff_  = diff;

      std::cout << best_diff_ << std::endl;
    }

    // set all parameters randomly!
    for (int i = 0; i < vst_.GetNumParams(); ++i)
      vst_.SetParam(i, unif_(eng_));
  }

  /*vst_.SaveCurrentProgram("pgm.fxp");

  float val_b = vst_.GetParam(0);
  vst_.SetParam(0, 0.123456f);
  float val_a = vst_.GetParam(0);

  vst_.LoadProgram("pgm.fxp");
  val_a = vst_.GetParam(0);*/

  DeleteCriticalSection(&snd_crit_);
}