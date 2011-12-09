#include "asio_helper.h"

static ASIOHelper asio_;

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
  asio_.Init(MyAudioCallback);
}