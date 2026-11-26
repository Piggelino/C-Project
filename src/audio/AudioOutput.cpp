#include "AudioOutput.hpp"

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cmath>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

bool AudioOutput::playAudio(
    const std::string& filePath
) {
#if defined(_WIN32)

    static unsigned int audioId = 0;

    const std::string alias =
        "camera_audio_" + std::to_string(++audioId);

    const std::string openCommand =
        "open \"" + filePath +
        "\" type mpegvideo alias " + alias;

    if (mciSendStringA(
            openCommand.c_str(),
            nullptr,
            0,
            nullptr
        ) != 0) {
        return false;
    }

    const std::string playCommand =
        "play " + alias + " from 0";

    if (mciSendStringA(
            playCommand.c_str(),
            nullptr,
            0,
            nullptr
        ) != 0) {

        mciSendStringA(
            ("close " + alias).c_str(),
            nullptr,
            0,
            nullptr
        );

        return false;
    }

    return true;

#elif defined(__APPLE__)

    return std::system(
        ("afplay \"" + filePath + "\" &").c_str()
    ) == 0;

#else

    return std::system(
        ("xdg-open \"" + filePath +
        "\" >/dev/null 2>&1 &").c_str()
    ) == 0;

#endif
}

// Support the previous interface.
void AudioOutput::updateFrequency(
    const AudioSource& source,
    double proximity
) {
    updateFrequency(source, proximity, 0.0);
}

// Update sound based on distance and rotation.
void AudioOutput::updateFrequency(
    const AudioSource& source,
    double proximity,
    double rotation
) {
    if (!std::isfinite(proximity) ||
        !std::isfinite(rotation)) {
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

    // Calculate the original distance-based interval.
    const int distanceInterval = static_cast<int>(
        slowestInterval -
        proximity *
        (slowestInterval - fastestInterval)
    );

    // Convert rotation to radians.
    constexpr double PI = 3.14159265358979323846;

    const double radians =
        rotation * PI / 180.0;

    // Rotation changes the interval by a factor of 1–2.
    const double rotationFactor =
        1.0 + 0.5 * (1.0 - std::cos(radians));

    const int framesToWait = std::max(
        1,
        static_cast<int>(
            std::lround(
                distanceInterval * rotationFactor
            )
        )
    );

    if (frameCounter >= framesToWait) {

        if (!playAudio(source.getFilePath())) {
            std::cerr
                << "Kunde inte spela ljudfilen: "
                << source.getFilePath()
                << std::endl;

            frameCounter = 0;
            return;
        }

        frameCounter = 0;

        std::cout
            << "Spelar ljud med dynamisk frekvens: "
            << source.getFilePath()
            << std::endl;
    }
}

void AudioOutput::resetDetection() {
    frameCounter = 0;
}