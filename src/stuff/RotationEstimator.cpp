#include "RotationEstimator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

bool RotationEstimator::correctCornerOrder(
    std::vector<cv::Point2f>& corners, int rotationIndex
) const {
    if (corners.size() != 4 || rotationIndex < 0 || rotationIndex > 3)
        return false;

    std::rotate(corners.begin(), corners.begin() + rotationIndex, corners.end());
    return true;
}

double RotationEstimator::estimateRotation(const std::vector<cv::Point2f>& corners) const {
    if (corners.size() != 4)
        return std::numeric_limits<double>::quiet_NaN();

    const cv::Point2f direction = corners[1] - corners[0];
    if (cv::norm(direction) < 0.000001)
        return std::numeric_limits<double>::quiet_NaN();

    double angle = std::atan2(direction.y, direction.x) * 180.0 / CV_PI;
    if (angle < 0.0)
        angle += 360.0;
    return angle;
}
