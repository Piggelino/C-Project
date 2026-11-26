#pragma once

#include "AudioSource.hpp"
#include "AudioOutput.hpp"
#include "DistanceEstimator.hpp"
#include "MarkerTemplateMatcher.hpp"
#include "CornerDetector.hpp"
#include "RotationEstimator.hpp"

class CameraHandler {
private:
    AudioSource mySource;
    AudioOutput mySpeaker;

    DistanceEstimator distanceEstimator;
    MarkerTemplateMatcher markerMatcher;
    CornerDetector cornerDetector;
    RotationEstimator rotationEstimator;

public:
    CameraHandler();

    void run();
};