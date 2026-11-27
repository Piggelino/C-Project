#include "AudioOutput.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

bool AudioOutput::playAudio(const std::string& filePath) {
#if defined(_WIN32)
    static unsigned int audioId = 0;
    static std::string activeFilePath;
    static std::vector<std::string> activeAliases;

    if (activeFilePath != filePath) {
        for (const auto& alias : activeAliases) {
            mciSendStringA(("stop " + alias).c_str(), nullptr, 0, nullptr);
            mciSendStringA(("close " + alias).c_str(), nullptr, 0, nullptr);
        }

        activeAliases.clear();
        activeFilePath = filePath;
    }

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

    activeAliases.push_back(alias);
    return true;
#elif defined(__APPLE__)
    return std::system(("afplay \"" + filePath + "\" &").c_str()) == 0;
#else
    return std::system(
        ("xdg-open \"" + filePath + "\" >/dev/null 2>&1 &").c_str()
    ) == 0;
#endif
}

void AudioOutput::updateFrequency(const AudioSource& source, double proximity) {
    if (!std::isfinite(proximity)) {
        return;
    }

    ++frameCounter;

    proximity = std::clamp(
        proximity,
        0.0,
        1.0
    );

    constexpr int slowestInterval = 20;
    constexpr int fastestInterval = 1;

    const int framesToWait = static_cast<int>(
        slowestInterval - proximity * (slowestInterval - fastestInterval)
    );

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