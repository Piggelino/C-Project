#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

class AudioSource {
private:
    std::string filePath;

public:
    explicit AudioSource(std::string path)
        : filePath(std::move(path)) {}

    const std::string& getFilePath() const {
        return filePath;
    }
};

class AudioOutput {
private:
    int frameCounter = 0;

    static bool playAudio(const std::string& filePath) {
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

public:
    void updateFrequency(const AudioSource& source, double proximity) {
        ++frameCounter;
        constexpr int slowestInterval = 50;
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

    void resetDetection() {
        frameCounter = 0;
    }
};
