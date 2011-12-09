#include "asiodrivers.h" // for getDriverNames, loadDriver, etc.
#include "asio.h"

static AsioDrivers asio_drivers;
static const int kMaxAsioDriverLen     = 8;
static const int kMaxAsioDriverNameLen = MAXDRVNAMELEN;

static ASIOBufferInfo* buffer_infos;
static ASIOChannelInfo* channel_infos;
static long input_channels;
static long output_channels;
static long buffer_size;

void SampleRateDidChange(ASIOSampleRate sRate)
{
	// do whatever you need to do if the sample rate changed
	// usually this only happens during external sync.
	// Audio processing is not stopped by the driver, actual sample rate
	// might not have even changed, maybe only the sample rate status of an
	// AES/EBU or S/PDIF digital input at the audio device.
	// You might have to update time/sample related conversion routines, etc.
}

long AsioMessage(long selector, long value, void* message, double* opt)
{
	// currently the parameters "value", "message" and "opt" are not used.
	long ret = 0;
	switch(selector)
	{
		case kAsioSelectorSupported:
			if(value == kAsioResetRequest
			|| value == kAsioEngineVersion
			|| value == kAsioResyncRequest
			|| value == kAsioLatenciesChanged
			// the following three were added for ASIO 2.0, you don't necessarily have to support them
			|| value == kAsioSupportsTimeInfo
			|| value == kAsioSupportsTimeCode
			|| value == kAsioSupportsInputMonitor)
				ret = 1L;
			break;
		case kAsioResetRequest:
			// defer the task and perform the reset of the driver during the next "safe" situation
			// You cannot reset the driver right now, as this code is called from the driver.
			// Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
			// Afterwards you initialize the driver again.
			ret = 1L;
			break;
		case kAsioResyncRequest:
			// This informs the application, that the driver encountered some non fatal data loss.
			// It is used for synchronization purposes of different media.
			// Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
			// Windows Multimedia system, which could loose data because the Mutex was hold too long
			// by another thread.
			// However a driver can issue it in other situations, too.
			ret = 1L;
			break;
		case kAsioLatenciesChanged:
			// This will inform the host application that the drivers were latencies changed.
			// Beware, it this does not mean that the buffer sizes have changed!
			// You might need to update internal delay data.
			ret = 1L;
			break;
		case kAsioEngineVersion:
			// return the supported ASIO version of the host application
			// If a host applications does not implement this selector, ASIO 1.0 is assumed
			// by the driver
			ret = 2L;
			break;
		case kAsioSupportsTimeInfo:
			// informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
			// is supported.
			// For compatibility with ASIO 1.0 drivers the host application should always support
			// the "old" bufferSwitch method, too.
			ret = 1;
			break;
		case kAsioSupportsTimeCode:
			// informs the driver wether application is interested in time code info.
			// If an application does not need to know about time code, the driver has less work
			// to do.
			ret = 0;
			break;
	}
	return ret;
}

ASIOTime* BufferSwitchTimeInfo(ASIOTime* time_info, long index, ASIOBool process_now)
{
  // go through the buffers, only using outputs
  ASIOBufferInfo*  b = buffer_infos  + input_channels;
  ASIOChannelInfo* c = channel_infos + input_channels;
  for (int i = 0; i < output_channels; ++i, ++b, ++c)
  {
    unsigned int* uint32_p;

    switch (c->type)
    {
    case ASIOSTInt32LSB:
      uint32_p = static_cast<unsigned int*>(b->buffers[index]);
      for (int j = 0; j < buffer_size; ++j)
      {
        float rnd = static_cast<float>(rand()) * UINT_MAX / RAND_MAX;
        *uint32_p++ = static_cast<unsigned int>(rnd);
      }
      break;
    default:
      break;
    }

  int n = 0; 
  }

  return NULL;
}

void BufferSwitch(long index, ASIOBool process_now) { BufferSwitchTimeInfo(NULL, index, process_now); }

int main(int argc, char* argv[])
{
  // to hold ASIO-related errors
  ASIOError error;

  char* names[kMaxAsioDriverLen];
  for (int i = 0; i < kMaxAsioDriverLen; ++i)
    names[i] = new char[kMaxAsioDriverNameLen];

  // get the driver names
  long drivers_found = asio_drivers.getDriverNames(
    names, 
    kMaxAsioDriverNameLen
    );

  if (drivers_found <= 0) exit(EXIT_FAILURE);

  // keep going till we find a good one
  int idx = 0;
  do
  {
    bool success = asio_drivers.loadDriver(names[idx++]);
    if (!success) exit(EXIT_FAILURE);

    // initialize and get driver info at the same time
    ASIODriverInfo driver_info;
    error = ASIOInit(&driver_info);
  } while (error != ASE_OK);

  // couldn't find anything!
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // try the control panel
  //error = ASIOControlPanel();

  // get the number of input/output channels
  error = ASIOGetChannels(&input_channels, &output_channels);
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // get the buffer size
  long min_size, max_size, preferred_size, granularity;
  error = ASIOGetBufferSize(&min_size, &max_size, &preferred_size, &granularity);
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // set the buffer size to be the preferred size
  buffer_size = preferred_size;

  // get the sample rate
  ASIOSampleRate sample_rate;
  error = ASIOGetSampleRate(&sample_rate);
  if (error != ASE_OK) exit(EXIT_FAILURE);

  //// set the sample rate
  //error = ASIOSetSampleRate(sample_rate);
  //if (error != ASE_OK) exit(EXIT_FAILURE);

  // set up the callbacks
  ASIOCallbacks callbacks;
  memset(&callbacks, 0, sizeof(ASIOCallbacks));
  callbacks.bufferSwitch = BufferSwitch;
  callbacks.asioMessage = AsioMessage;
  callbacks.bufferSwitchTimeInfo = BufferSwitchTimeInfo;
  callbacks.sampleRateDidChange = SampleRateDidChange;
  
  // allocate for the buffer infos
  buffer_infos = new ASIOBufferInfo[input_channels * output_channels];

  // set up the buffer infos
  ASIOBufferInfo* b = buffer_infos;

  // input
  for (int i = 0; i < input_channels; ++i, ++b)
  {
    memset(b->buffers, 0, 2 * sizeof(void*));
    b->channelNum = i;
    b->isInput = ASIOTrue;
  }

  // output
  for (int i = 0; i < output_channels; ++i, ++b)
  {
    memset(b->buffers, 0, 2 * sizeof(void*));
    b->channelNum = i;
    b->isInput = ASIOFalse;
  }

  // create the buffers
  error = ASIOCreateBuffers(buffer_infos, input_channels + output_channels, buffer_size, &callbacks);
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // get the channel info
  channel_infos = new ASIOChannelInfo[input_channels * output_channels];
  memset(channel_infos, 0, input_channels * output_channels * sizeof(*channel_infos));

  // set up the buffer infos
  ASIOChannelInfo* c = channel_infos;

  // input
  for (int i = 0; i < input_channels; ++i, ++c)
  {
    c->channel = i;
    c->isInput = ASIOTrue;
    error = ASIOGetChannelInfo(c);
    if (error != ASE_OK) exit(EXIT_FAILURE);
  }

  // output
  for (int i = 0; i < output_channels; ++i, ++c)
  {
    c->channel = i;
    c->isInput = ASIOFalse;
    error = ASIOGetChannelInfo(c);
    if (error != ASE_OK) exit(EXIT_FAILURE);
  }

  // get latencies
  long input_latency, output_latency;
  error = ASIOGetLatencies(&input_latency, &output_latency);
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // start
  error = ASIOStart();
  if (error != ASE_OK) exit(EXIT_FAILURE);

  for (;;)
  {
    // should have a thread event to wake us up...
    Sleep(100);
  }

  // stop
  error = ASIOStop();
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // dispose of the buffers
  error = ASIODisposeBuffers();
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // deallocate the channel infos
  delete[] channel_infos;

  // deallocate the buffer infos
  delete[] buffer_infos;

  // exit
  error = ASIOExit();
  if (error != ASE_OK) exit(EXIT_FAILURE);

  // remove the driver
  asio_drivers.removeCurrentDriver();

  for (int i = 0; i < kMaxAsioDriverLen; ++i)
    delete[] names[i];
}