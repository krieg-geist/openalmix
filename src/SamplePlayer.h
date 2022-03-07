#pragma once
#include <al.h>
#include <sndfile.h>
class SamplePlayer {
public:
  void Play();
  void Pause();
  void Stop();
  void Resume();
  void SetVol(int vol);
  void SetPan(float pan);

  void UpdateBufferStream();

  ALint getSource();

  bool isPlaying();

  SamplePlayer(const char *filename);
  ~SamplePlayer();

private:
  ALuint p_Source;
  static const int BUFFER_SAMPLES = 16384;
  static const int NUM_BUFFERS = 3;
  ALuint p_Buffers[NUM_BUFFERS];
  SNDFILE *p_SndFile;
  SF_INFO p_Sfinfo;
  short *p_Membuf;
  ALenum p_Format;

  SamplePlayer() = delete;
};
