#include "SampleRecorder.h"
#include <alext.h>
#include <cstddef>
#include <iostream>
#include <malloc.h>
#include <stdexcept>
#include <thread>
#include <vector>


const char* SampleRecorder::getFilename()
{
  return p_filename;
}

void SampleRecorder::Record(std::atomic<bool> &isRecording)
{
  if (!armed)
  {
    return;
  }
  
  alcCaptureStart(p_Device);
  isRecording = true;

  auto err = ALC_NO_ERROR;
  while (isRecording && (err=alcGetError(p_Device)) == ALC_NO_ERROR)
  {
    ALCint samples = 0;
    alcGetIntegerv(p_Device, ALC_CAPTURE_SAMPLES, 1, &samples);
    if(samples < 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
    }
    if (samples > buffer_size)
    {
      ALshort *data = (ALshort*)malloc(frame_size * samples);
      free(buffer);
      buffer = data;
      buffer_size = samples;
    }
    alcCaptureSamples(p_Device, buffer, samples);
    ALCint i;
    sf_write_short(p_SndFile, buffer, CHANNELS * samples);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  alcCaptureStop(p_Device);
  alcCaptureCloseDevice(p_Device);
  sf_close(p_SndFile);
  done = true;
}

bool SampleRecorder::IsDone()
{
  return done;
}

bool SampleRecorder::StopRecord()
{
  return false;
}

SampleRecorder::SampleRecorder(const char *filename)
{
  p_filename = filename;
  // Generate info file
  p_Sfinfo = {};
  p_Sfinfo.samplerate = SAMPLE_RATE;
  p_Sfinfo.channels = CHANNELS;
  p_Sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;

  // Open sample file
  p_SndFile = sf_open(p_filename, SFM_WRITE, &p_Sfinfo);
  if (!p_SndFile)
  {
    throw std::runtime_error("could not open file for writing");
  }

  frame_size = p_Sfinfo.channels * sizeof(short);

  // Open capture device
  auto devname = "wm8960-soundcard, bcm2835-i2s-wm8960-hifi wm8960-hifi-0 (CARD=wm8960soundcard,DEV=0)";
  p_Device = alcCaptureOpenDevice(devname, SAMPLE_RATE, AL_FORMAT_STEREO16, 32768);
  if (!p_Device)
  {
    throw std::runtime_error("Could not open capture device!");
  }
  // Ready to record
  armed = true;
}

SampleRecorder::~SampleRecorder()
{
  if (p_SndFile)
    sf_close(p_SndFile);

  p_SndFile = nullptr;
}
