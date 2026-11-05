#include <iostream>
#include "CameraHandler.hpp"
#include <string>
#include <vector>
#include "AudioOutput.hpp"
#include "DistanceEstimator.hpp"
#include "MarkerTemplateMatcher.hpp" // Verify Hiro marker pattern
#include "CornerDetector.hpp"
#include "AudioSource.hpp"


#if __has_include(<opencv2/opencv.hpp>)
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#define OPENCV_AVAILABLE 1
#else
#define OPENCV_AVAILABLE 0
#endif

int main() {
    CameraHandler camera;
    camera.run();

    return 0;
}