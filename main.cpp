#include "asio_helper.h"

// this is static global so we can access it inside the audio callback
static ASIOHelper asio_;

// put our good stuff here -- write out what we want when ASIO requests it
void MyAudioCallback(int index)
{
  // go through the buffers, only using outputs
  ASIOBufferInfo*  b = asio_.buffer_infos_  + asio_.input_channels_;
  ASIOChannelInfo* c = asio_.channel_infos_ + asio_.input_channels_;
  for (int i = 0; i < asio_.output_channels_; ++i, ++b, ++c)
  {
    unsigned int* uint32_p;

    switch (c->type)
    {
    case ASIOSTInt32LSB:
      uint32_p = static_cast<unsigned int*>(b->buffers[index]);
      for (int j = 0; j < asio_.buffer_size_; ++j)
      {
        float rnd = static_cast<float>(rand()) * UINT_MAX / RAND_MAX;
        *uint32_p++ = static_cast<unsigned int>(rnd);
      }
      break;
    default:
      break;
    }
  }
}

int main(int argc, char* argv[])
{
  // gets ASIO going and uses our callback as the audio callback
  asio_.Init(MyAudioCallback);
}