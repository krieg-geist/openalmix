#include "SamplePlayer.h"
#include "SoundDevice.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <regex>
#include <thread>
#include <vector>
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
std::vector<std::shared_ptr<SamplePlayer>> samplePlayers;

// Thread stuff
std::vector<std::thread> workers;
std::atomic<bool> running{true};

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
    if (samplePlayers.size() > playerIdx)
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
  for (const auto &entry : fs::directory_iterator(wavDirectory)) {
    if (samplePlayers.size() < PLAYER_COUNT) {
      if (entry.path().extension() == ".wav") {
        try {
          samplePlayers.push_back(
              std::make_shared<SamplePlayer>(entry.path().c_str()));
          DEBUG("Successfully added ", entry.path());
        } catch (const std::exception &e) {
          DEBUG("Wav load error: ", e.what());
        }
      }
    }
  }
}

void audioTask(std::atomic<bool> &isRunning) {
  while (isRunning) {
    for (auto chan : samplePlayers)
      chan->UpdateBufferStream();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void exit() {
  running = false;
  for (std::thread &t : workers) {
    if (t.joinable()) {
      t.join();
    }
  }
}

int main() {
  std::cout << "initializing sound device...\n";
  SoundDevice::Init();

  initComms();
  initWavFiles();

  for (auto chan : samplePlayers)
    chan->Play();

  workers.push_back(std::thread(audioTask, std::ref(running)));
	commsTask(running);
}
