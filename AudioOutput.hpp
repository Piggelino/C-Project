#pragma once

#include <string>
#include "AudioSource.hpp"

class AudioOutput {
private:
    int frameCounter = 0;

    static bool playAudio(const std::string& filePath);

public:
    void updateFrequency(const AudioSource& source, double proximity);

    void resetDetection();
};