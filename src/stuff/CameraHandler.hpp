#pragma once

#include "AudioSource.hpp"
#include "AudioOutput.hpp"
#include "DistanceEstimator.hpp"
#include "MarkerTemplateMatcher.hpp"
#include "CornerDetector.hpp"

class CameraHandler {
private:
    AudioSource mySource;
    AudioOutput mySpeaker;
    DistanceEstimator distanceEstimator;
    MarkerTemplateMatcher markerMatcher;
    CornerDetector cornerDetector;

public:
    CameraHandler();

    void run();
};