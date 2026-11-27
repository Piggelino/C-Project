#pragma once

#include "AudioSource.hpp"
#include "AudioOutput.hpp"
#include "DistanceEstimator.hpp"
#include "MarkerTemplateMatcher.hpp"
#include "CornerDetector.hpp"
#include "RotationEstimator.hpp"

#include <array>

class CameraHandler {
private:
    std::array<AudioSource, 4> soundSources;
    AudioOutput mySpeaker;

    DistanceEstimator distanceEstimator;
    MarkerTemplateMatcher markerMatcher;
    CornerDetector cornerDetector;
    RotationEstimator rotationEstimator;

public:
    CameraHandler();

    void run();
};