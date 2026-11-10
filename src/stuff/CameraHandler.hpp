#pragma once

#include <opencv2/opencv.hpp>

#include "AudioOutput.hpp"
#include "DistanceEstimator.hpp"
#include "MarkerTemplateMatcher.hpp"

class CameraHandler {
private:
    AudioSource mySource;
    AudioOutput mySpeaker;
    DistanceEstimator distanceEstimator;
    MarkerTemplateMatcher markerMatcher;

public:
    CameraHandler();

    void run();
};