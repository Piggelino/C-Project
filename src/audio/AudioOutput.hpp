#pragma once

#include <string>
#include "AudioSource.hpp"

class AudioOutput {
private:
    int frameCounter = 0;

    static bool playAudio(const std::string& filePath);

public:
    // Calculate the playback interval without playing sound.
    static int framesBetweenSounds(double proximity);

    void updateFrequency(
        const AudioSource& source,
        double proximity
    );

    void resetDetection();
};
