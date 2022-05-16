#include "SamplePlayer.h"
#include "SoundDevice.h"
#include "SampleRecorder.h"
#include <atomic>
#include <chrono>
#include <ctime>
#include <cstring>
#include <filesystem>
#include <memory>
#include <regex>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <iostream>
#include <wiringPi.h>
#include <wiringSerial.h>

#define DEBUGFLAG

void debug_print() { std::cerr << std::endl; }
template <typename Head, typename... Tail>
void debug_print(Head H, Tail... T) {
    std::cerr << ' ' << H;
    debug_print(T...);
}

#ifdef DEBUGFLAG
#  define DEBUG(...) std::cerr << "dbg  (" << #__VA_ARGS__ << "):", \
                     debug_print(__VA_ARGS__)
#else
#  define DEBUG(...) do {} while(0)
#endif

namespace fs = std::filesystem;

#define PLAYER_COUNT 8

auto wavDirectory = fs::path("/home/pi/openalmix/res/samples");
std::vector<std::shared_ptr<SamplePlayer>> samplePlayers(PLAYER_COUNT, nullptr);
SampleRecorder* sampleRecorder;
uint8_t recordChannel = 0;

// Thread stuff
std::vector<std::thread> playbackWorkers;
std::unique_ptr<std::thread> recordWorker;
std::atomic<bool> running{true};
std::atomic<bool> recording{false};

// Comms stuff
int serialDeviceId;

void initComms()
{
  if(wiringPiSetup() < 0)
    DEBUG("WiringPi could not start");
  if((serialDeviceId = serialOpen("/dev/serial0",115200)) < 0)
  {
    DEBUG("Serial not started:");
    DEBUG(serialDeviceId);
  }
  DEBUG("Serial init successfully...");
}

void commsHandler(std::string command)
{
  int delimit = command.find(":");
  auto controlStr = command.substr(1, delimit - 1);
  int value = std::stoi(command.substr(delimit + 1, command.length()));
  
  DEBUG(controlStr, value);
  if(controlStr[0] == 'A')
  // Adjust volume
  {
    int playerIdx = ((int)controlStr[1]) - 48;
    if (samplePlayers[playerIdx] != nullptr)
    {
      samplePlayers[playerIdx]->SetVol(value);
    }
  }
  return;
}

void commsTask(std::atomic<bool> &isRunning)
{
  std::string serialStream;
	while (isRunning) {
    while (serialDataAvail(serialDeviceId))
    {
      auto nextChar = serialGetchar(serialDeviceId);
      if ((nextChar == '\r' || nextChar == '\n') && !serialStream.empty())
      {
        commsHandler(serialStream);
        serialStream.clear();
      }
      else
      {
        serialStream.push_back(nextChar);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void initWavFiles() {
  size_t count = 0;
  for (const auto &entry : fs::directory_iterator(wavDirectory)) {
    if (count < PLAYER_COUNT) {
      if (entry.path().extension() == ".wav") {
        try {
          samplePlayers[count] = std::make_shared<SamplePlayer>(entry.path().c_str());
          DEBUG("Successfully added ", entry.path());
          count += 1;
        } catch (const std::exception &e) {
          DEBUG("Wav load error: ", e.what());
        }
      }
    }
  }
}

void initRecord(uint8_t record_channel) {
  sampleRecorder = nullptr;
  record_channel = record_channel;

  // Generate timestamp filename
  std::stringstream filename;
  time_t curr_time;
  struct tm *curr_tm;
  char time_str[15];
  time(&curr_time);
	curr_tm = localtime(&curr_time);
  strftime(time_str, 15, "%Y%e%d%H%M%S", curr_tm);
  filename << "clip_";
  filename << time_str;
  filename << ".wav";

  sampleRecorder = new SampleRecorder(filename.str().c_str());
}

void stopRecord()
{
  if (sampleRecorder != nullptr)
  {
    recording = false;
    while (!sampleRecorder->IsDone())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    samplePlayers[recordChannel] = nullptr;
    samplePlayers[recordChannel] = std::make_shared<SamplePlayer>(sampleRecorder->getFilename());
    samplePlayers[recordChannel]->Play();
    recordWorker->join();
    sampleRecorder = nullptr;
  }
  else
  {
    DEBUG("No recorder initialized");
  }
}

void recordTask(std::atomic<bool> &isRecording) {
  if (sampleRecorder != nullptr)
  {
    sampleRecorder->Record(std::ref(isRecording));
  }
  else
  {
    DEBUG("No recorder initialized");
  }
}

void audioTask(std::atomic<bool> &isRunning) {
  while (isRunning) {
    for (auto chan : samplePlayers)
    {
      if (chan != nullptr)
      {
        chan->UpdateBufferStream();
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void exit() {
  running = false;
  for (std::thread &t : playbackWorkers) {
    if (t.joinable()) {
      t.join();
    }
  }
  if (recordWorker->joinable())
  {
    recordWorker->join();
  }
}

int main() {
  std::cout << "initializing sound device...\n";
  SoundDevice::Init();

  /*
  initComms();
  initWavFiles();

  for (auto chan : samplePlayers)
  {
    if (chan != nullptr)
    {
      chan->Play();
    }
  } 

  playbackWorkers.push_back(std::thread(audioTask, std::ref(running)));
	// commsTask(running);
  */
  DEBUG("Init record");
  initRecord(0);
  DEBUG("Start record");
  recordWorker = std::make_unique<std::thread>(recordTask, std::ref(recording));
  DEBUG("Record started");
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  DEBUG("Stop record");
  stopRecord();
  DEBUG("Record stopped");
}
