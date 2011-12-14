#ifndef ASIO_HELPER_H
#define ASIO_HELPER_H

#include "asiodrivers.h" // for getDriverNames, loadDriver, etc.
#include "asio.h"        // for all the ASIO types, etc.

typedef void (*AudioCallback)(int index);

class ASIOHelper
{
public:

  void Init(AudioCallback callback);

  // these are accessible because audio callbacks need them
  ASIOBufferInfo* buffer_infos_;
  ASIOChannelInfo* channel_infos_;
  long input_channels_;
  long output_channels_;
  long buffer_size_;

private:

  static const int kMaxAsioDriverLen     = 8;
  static const int kMaxAsioDriverNameLen = MAXDRVNAMELEN;

  AsioDrivers asio_drivers;
};

#endif