#pragma once

#include <opencv2/core.hpp>
#include <vector>

class RotationEstimator {
public:
    // Restore the physical order of the marker corners.
    bool correctCornerOrder(
        std::vector<cv::Point2f>& corners,
        int rotationIndex
    ) const;

    // Estimate the rotation angle in degrees.
    double estimateRotation(
        const std::vector<cv::Point2f>& corners
    ) const;
};