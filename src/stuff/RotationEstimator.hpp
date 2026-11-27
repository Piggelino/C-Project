#pragma once

#include <vector>
#include <opencv2/core.hpp>

class RotationEstimator {
public:
    bool correctCornerOrder(std::vector<cv::Point2f>& corners, int rotationIndex) const;
    double estimateRotation(const std::vector<cv::Point2f>& corners) const;
};