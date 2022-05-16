#pragma once
#include <al.h>
#include <sndfile.h>
#include <alc.h>
#include <atomic>
#include <string>

class SampleRecorder {
public:
  void Record(std::atomic<bool> &isRecording);
  bool StopRecord();
  bool IsDone();
  const char* getFilename();

  SampleRecorder(const char *filename);
  ~SampleRecorder();

private:
  bool armed;
  bool done;
  char *p_devName = NULL;
  const char *p_filename = NULL;
  ALsizei buffer_size = 0;
  ALshort *buffer = NULL;
  static const int CHANNELS = 2;
  static const int SAMPLE_RATE = 48000;
  size_t frame_size; // 2 channels * 16 bits
  SNDFILE *p_SndFile;
  SF_INFO p_Sfinfo;
  ALCdevice *p_Device;
  SampleRecorder() = delete;
};