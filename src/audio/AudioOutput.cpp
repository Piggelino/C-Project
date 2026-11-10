#include "AudioOutput.hpp"

#include <cstdlib>
#include <iostream>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

bool AudioOutput::playAudio(const std::string& filePath) {
    // Only for windows
#if defined(_WIN32)
    static unsigned int audioId = 0;
    const std::string alias = "camera_audio_" + std::to_string(++audioId);
    const std::string openCommand =
        "open \"" + filePath + "\" type mpegvideo alias " + alias;

    if (mciSendStringA(openCommand.c_str(), nullptr, 0, nullptr) != 0) {
        return false;
    }

    const std::string playCommand = "play " + alias + " from 0";
    if (mciSendStringA(playCommand.c_str(), nullptr, 0, nullptr) != 0) {
        mciSendStringA(("close " + alias).c_str(), nullptr, 0, nullptr);
        return false;
    }
    return true;
#elif defined(__APPLE__)
    return std::system(("afplay \"" + filePath + "\" &").c_str()) == 0;
#else
    return std::system(
               ("xdg-open \"" + filePath + "\" >/dev/null 2>&1 &").c_str()) ==
           0;
#endif
}

void AudioOutput::updateFrequency(const AudioSource& source, double proximity) {
    ++frameCounter;
    constexpr int slowestInterval = 20;
    constexpr int fastestInterval = 1;
    const int framesToWait = static_cast<int>(
        slowestInterval -
        proximity * (slowestInterval - fastestInterval));

    if (frameCounter >= framesToWait) {
        if (!playAudio(source.getFilePath())) {
            std::cerr << "Kunde inte spela ljudfilen: "
                      << source.getFilePath() << std::endl;
            frameCounter = 0;
            return;
        }
        frameCounter = 0;
        std::cout << "Spelar ljud med dynamisk frekvens: "
                  << source.getFilePath() << std::endl;
    }
}

void AudioOutput::resetDetection() {
    frameCounter = 0;
}