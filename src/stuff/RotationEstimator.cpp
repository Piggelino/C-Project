#include "RotationEstimator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

// Restore the physical corner order.
bool RotationEstimator::correctCornerOrder(
    std::vector<cv::Point2f>& corners,
    int rotationIndex
) const {
    if (corners.size() != 4 ||
        rotationIndex < 0 ||
        rotationIndex > 3) {
        return false;
    }

    // Shift the corners according to the matched template.
    std::rotate(
        corners.begin(),
        corners.begin() + rotationIndex,
        corners.end()
    );

    return true;
}

// Calculate the rotation angle.
double RotationEstimator::estimateRotation(
    const std::vector<cv::Point2f>& corners
) const {
    if (corners.size() != 4) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Calculate the direction from corner A to corner B.
    cv::Point2f direction =
        corners[1] - corners[0];

    if (cv::norm(direction) < 0.000001) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Calculate the angle in radians.
    double angle = std::atan2(
        direction.y,
        direction.x
    );

    // Convert radians to degrees.
    angle = angle * 180.0 / CV_PI;

    // Normalize the angle to [0, 360).
    if (angle < 0.0) {
        angle += 360.0;
    }

    return angle;
}
