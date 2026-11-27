#pragma once

#include "AudioSource.hpp"

#include <string>

class AudioOutput {
private:
    int frameCounter = 0;

    static bool playAudio(const std::string& filePath);

public:
    void updateFrequency(const AudioSource& source, double proximity);
    void resetDetection();
};